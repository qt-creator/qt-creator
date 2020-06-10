/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmljslink.h"

#include "parser/qmljsast_p.h"
#include "qmljsdocument.h"
#include "qmljsbind.h"
#include "qmljsutils.h"
#include "qmljsmodelmanagerinterface.h"
#include "qmljsconstants.h"

#include <utils/qrcparser.h>

#include <QDir>
#include <QDirIterator>

using namespace LanguageUtils;
using namespace QmlJS::AST;

namespace QmlJS {

namespace {
class ImportCacheKey
{
public:
    explicit ImportCacheKey(const ImportInfo &info)
        : m_type(info.type())
        , m_path(info.path())
        , m_majorVersion(info.version().majorVersion())
        , m_minorVersion(info.version().minorVersion())
    {}

private:
    friend uint qHash(const ImportCacheKey &);
    friend bool operator==(const ImportCacheKey &, const ImportCacheKey &);

    int m_type;
    QString m_path;
    int m_majorVersion;
    int m_minorVersion;
};

uint qHash(const ImportCacheKey &info)
{
    return ::qHash(info.m_type) ^ ::qHash(info.m_path) ^
            ::qHash(info.m_majorVersion) ^ ::qHash(info.m_minorVersion);
}

bool operator==(const ImportCacheKey &i1, const ImportCacheKey &i2)
{
    return i1.m_type == i2.m_type
            && i1.m_path == i2.m_path
            && i1.m_majorVersion == i2.m_majorVersion
            && i1.m_minorVersion == i2.m_minorVersion;
}
}

class LinkPrivate
{
public:
    Context::ImportsPerDocument linkImports();

    void populateImportedTypes(Imports *imports, const Document::Ptr &doc);
    Import importFileOrDirectory(
        const Document::Ptr &doc,
        const ImportInfo &importInfo);
    Import importNonFile(
        const Document::Ptr &doc,
        const ImportInfo &importInfo);

    bool importLibrary(const Document::Ptr &doc,
                       const QString &libraryPath,
                       Import *import, ObjectValue *targetObject,
                       const QString &importPath = QString());
    void loadQmldirComponents(ObjectValue *import,
                              LanguageUtils::ComponentVersion version,
                              const LibraryInfo &libraryInfo,
                              const QString &libraryPath);
    void loadImplicitDirectoryImports(Imports *imports, const Document::Ptr &doc);
    void loadImplicitDefaultImports(Imports *imports);

    void error(const Document::Ptr &doc, const SourceLocation &loc, const QString &message);
    void warning(const Document::Ptr &doc, const SourceLocation &loc, const QString &message);
    void appendDiagnostic(const Document::Ptr &doc, const DiagnosticMessage &message);

private:
    friend class Link;

    Snapshot m_snapshot;
    ValueOwner *m_valueOwner = nullptr;
    QStringList m_importPaths;
    QStringList m_applicationDirectories;
    LibraryInfo m_builtins;
    ViewerContext m_vContext;

    QHash<ImportCacheKey, Import> importCache;
    QHash<QString, QList<ModuleApiInfo>> importableModuleApis;

    Document::Ptr document;

    QList<DiagnosticMessage> *diagnosticMessages = nullptr;
    QHash<QString, QList<DiagnosticMessage>> *allDiagnosticMessages = nullptr;
};

/*!
    \class QmlJS::Link
    \brief The Link class creates a Context for a Snapshot.
    \sa Context Snapshot

    Initializes a context by resolving imports. This is an expensive operation.

    Instead of making a fresh context, consider reusing the one maintained in the
    \l{QmlJSEditor::SemanticInfo} of a \l{QmlJSEditor::QmlJSEditorDocument}.
*/

Link::Link(const Snapshot &snapshot, const ViewerContext &vContext, const LibraryInfo &builtins)
    : d(new LinkPrivate)
{
    d->m_valueOwner = new ValueOwner;
    d->m_snapshot = snapshot;
    d->m_importPaths = vContext.paths;
    d->m_applicationDirectories = vContext.applicationDirectories;
    d->m_builtins = builtins;
    d->m_vContext = vContext;

    d->diagnosticMessages = nullptr;
    d->allDiagnosticMessages = nullptr;

    ModelManagerInterface *modelManager = ModelManagerInterface::instance();
    if (modelManager) {
        const ModelManagerInterface::CppDataHash cppDataHash = modelManager->cppData();
        {
            // populate engine with types from C++
            for (auto it = cppDataHash.cbegin(), end = cppDataHash.cend(); it != end; ++it)
                d->m_valueOwner->cppQmlTypes().load(it.key(), it.value().exportedTypes);
        }

        // build an object with the context properties from C++
        ObjectValue *cppContextProperties = d->m_valueOwner->newObject(/* prototype = */ nullptr);
        for (const ModelManagerInterface::CppData &cppData : cppDataHash) {
            for (auto it = cppData.contextProperties.cbegin(),
                 end = cppData.contextProperties.cend();
                 it != end; ++it) {
                const Value *value = nullptr;
                const QString &cppTypeName = it.value();
                if (!cppTypeName.isEmpty())
                    value = d->m_valueOwner->cppQmlTypes().objectByCppName(cppTypeName);
                if (!value)
                    value = d->m_valueOwner->unknownValue();
                cppContextProperties->setMember(it.key(), value);
            }
        }
        d->m_valueOwner->cppQmlTypes().setCppContextProperties(cppContextProperties);
    }
}

ContextPtr Link::operator()(QHash<QString, QList<DiagnosticMessage> > *messages)
{
    d->allDiagnosticMessages = messages;
    return Context::create(d->m_snapshot, d->m_valueOwner, d->linkImports(), d->m_vContext);
}

ContextPtr Link::operator()(const Document::Ptr &doc, QList<DiagnosticMessage> *messages)
{
    d->document = doc;
    d->diagnosticMessages = messages;
    return Context::create(d->m_snapshot, d->m_valueOwner, d->linkImports(), d->m_vContext);
}

Link::~Link()
{
    delete d;
}

Context::ImportsPerDocument LinkPrivate::linkImports()
{
    Context::ImportsPerDocument importsPerDocument;

    // load builtin objects
    if (m_builtins.pluginTypeInfoStatus() == LibraryInfo::DumpDone
            || m_builtins.pluginTypeInfoStatus() == LibraryInfo::TypeInfoFileDone) {
        m_valueOwner->cppQmlTypes().load(QLatin1String("<builtins>"), m_builtins.metaObjects());
    } else {
        m_valueOwner->cppQmlTypes().load(QLatin1String("<defaults>"),
                                         CppQmlTypesLoader::defaultQtObjects);
    }

    // load library objects shipped with Creator
    m_valueOwner->cppQmlTypes().load(QLatin1String("<defaultQt4>"),
                                     CppQmlTypesLoader::defaultLibraryObjects);

    if (document) {
        // do it on document first, to make sure import errors are shown
        auto *imports = new Imports(m_valueOwner);

        // Add custom imports for the opened document
        for (const auto &provider : CustomImportsProvider::allProviders()) {
            const auto providerImports = provider->imports(m_valueOwner, document.data());
            for (const auto &import : providerImports)
                importCache.insert(ImportCacheKey(import.info), import);
        }

        populateImportedTypes(imports, document);
        importsPerDocument.insert(document.data(), QSharedPointer<Imports>(imports));
    }

    for (const Document::Ptr &doc : qAsConst(m_snapshot)) {
        if (doc == document)
            continue;

        auto *imports = new Imports(m_valueOwner);
        populateImportedTypes(imports, doc);
        importsPerDocument.insert(doc.data(), QSharedPointer<Imports>(imports));
    }

    return importsPerDocument;
}

/**
 * A workaround for prototype issues with QEasingCurve in Qt 5.15.
 *
 * In this Qt version, QEasingCurve is declared in builtins.qmltypes, but its
 * prototype, QQmlEasingValueType, is only contained in the QtQml module.
 *
 * This code attempts to resolve QEasingCurve's prototype if it hasn't been set
 * already. It's intended to be called after all CppQmlTypes have been loaded.
 */
static void workaroundQEasingCurve(CppQmlTypes &cppTypes)
{
    const CppComponentValue *easingCurve = cppTypes.objectByCppName("QEasingCurve");
    if (!easingCurve || easingCurve->prototype())
        return;

    const QString superclassName = easingCurve->metaObject()->superclassName();
    if (superclassName.isEmpty())
        return;

    const CppComponentValue *prototype = cppTypes.objectByCppName(superclassName);
    if (!prototype)
        return;

    const_cast<CppComponentValue *>(easingCurve)->setPrototype(prototype);
}

void LinkPrivate::populateImportedTypes(Imports *imports, const Document::Ptr &doc)
{
    importableModuleApis.clear();

    // implicit imports: the <default> package is always available
    loadImplicitDefaultImports(imports);

    // implicit imports:
    // qml files in the same directory are available without explicit imports
    if (doc->isQmlDocument())
        loadImplicitDirectoryImports(imports, doc);

    // explicit imports, whether directories, files or libraries
    const auto docImports = doc->bind()->imports();
    for (const ImportInfo &info : docImports) {
        Import import = importCache.value(ImportCacheKey(info));

        // ensure usage of the right ImportInfo, the cached import
        // can have a different 'as' clause...
        import.info = info;

        if (!import.object) {
            switch (info.type()) {
            case ImportType::File:
            case ImportType::Directory:
            case ImportType::QrcFile:
            case ImportType::QrcDirectory:
                import = importFileOrDirectory(doc, info);
                break;
            case ImportType::Library:
                import = importNonFile(doc, info);
                break;
            case ImportType::UnknownFile:
                imports->setImportFailed();
                if (info.ast()) {
                    error(doc, info.ast()->fileNameToken,
                          Link::tr("File or directory not found."));
                }
                break;
            default:
                break;
            }
            if (import.object)
                importCache.insert(ImportCacheKey(info), import);
        }
        if (import.object)
            imports->append(import);
    }

    workaroundQEasingCurve(m_valueOwner->cppQmlTypes());
}

/*
    import "content"
    import "content" as Xxx
    import "content" 4.6
    import "content" 4.6 as Xxx

    import "http://www.ovi.com/" as Ovi

    import "file.js" as Foo
*/
Import LinkPrivate::importFileOrDirectory(const Document::Ptr &doc, const ImportInfo &importInfo)
{
    Import import;
    import.info = importInfo;
    import.object = nullptr;
    import.valid = true;

    QString path = importInfo.path();

    if (importInfo.type() == ImportType::Directory
            || importInfo.type() == ImportType::ImplicitDirectory) {
        import.object = new ObjectValue(m_valueOwner);

        importLibrary(doc, path, &import, import.object);

        const QList<Document::Ptr> documentsInDirectory = m_snapshot.documentsInDirectory(path);
        for (const Document::Ptr &importedDoc : documentsInDirectory) {
            if (importedDoc->bind()->rootObjectValue()) {
                const QString targetName = importedDoc->componentName();
                import.object->setMember(targetName, importedDoc->bind()->rootObjectValue());
            }
        }
    } else if (importInfo.type() == ImportType::File) {
        if (Document::Ptr importedDoc = m_snapshot.document(path))
            import.object = importedDoc->bind()->rootObjectValue();
    } else if (importInfo.type() == ImportType::QrcFile) {
        QLocale locale;
        QStringList filePaths = ModelManagerInterface::instance()
                ->filesAtQrcPath(path, &locale, nullptr, ModelManagerInterface::ActiveQrcResources);
        if (filePaths.isEmpty())
            filePaths = ModelManagerInterface::instance()->filesAtQrcPath(path);
        if (!filePaths.isEmpty()) {
            if (Document::Ptr importedDoc = m_snapshot.document(filePaths.at(0)))
                import.object = importedDoc->bind()->rootObjectValue();
        }
    } else if (importInfo.type() == ImportType::QrcDirectory){
        import.object = new ObjectValue(m_valueOwner);

        importLibrary(doc, path, &import, import.object);

        const QMap<QString, QStringList> paths
                = ModelManagerInterface::instance()->filesInQrcPath(path);
        for (auto iter = paths.cbegin(), end = paths.cend(); iter != end; ++iter) {
            if (ModelManagerInterface::guessLanguageOfFile(iter.key()).isQmlLikeLanguage()) {
                Document::Ptr importedDoc = m_snapshot.document(iter.value().at(0));
                if (importedDoc && importedDoc->bind()->rootObjectValue()) {
                    const QString targetName = QFileInfo(iter.key()).baseName();
                    import.object->setMember(targetName, importedDoc->bind()->rootObjectValue());
                }
            }
        }
    }
    return import;
}

static ModuleApiInfo findBestModuleApi(const QList<ModuleApiInfo> &apis,
                                       const ComponentVersion &version)
{
    ModuleApiInfo best;
    for (const ModuleApiInfo &moduleApi : apis) {
        if (moduleApi.version <= version
                && (!best.version.isValid() || best.version < moduleApi.version)) {
            best = moduleApi;
        }
    }
    return best;
}

/*
  import Qt 4.6
  import Qt 4.6 as Xxx
*/
Import LinkPrivate::importNonFile(const Document::Ptr &doc, const ImportInfo &importInfo)
{
    Import import;
    import.info = importInfo;
    import.object = new ObjectValue(m_valueOwner);
    import.valid = true;

    const QString packageName = importInfo.name();
    const ComponentVersion version = importInfo.version();

    QString libraryPath = modulePath(packageName, version.toString(), m_importPaths);
    bool importFound = !libraryPath.isEmpty() && importLibrary(doc, libraryPath, &import, import.object);

    if (!importFound) {
        for (const QString &dir : qAsConst(m_applicationDirectories)) {
            QDirIterator it(dir, QStringList { "*.qmltypes" }, QDir::Files);

            // This adds the types to the C++ types, to be found below if applicable.
            if (it.hasNext())
                importLibrary(doc, dir, &import, import.object);
        }
    }

    // if there are cpp-based types for this package, use them too
    if (m_valueOwner->cppQmlTypes().hasModule(packageName)) {
        importFound = true;
        const auto objects = m_valueOwner->cppQmlTypes().createObjectsForImport(packageName,
                                                                                version);
        for (const CppComponentValue *object : objects)
            import.object->setMember(object->className(), object);
    }

    // check module apis that previous imports may have enabled
    ModuleApiInfo moduleApi = findBestModuleApi(importableModuleApis.value(packageName), version);
    if (moduleApi.version.isValid()) {
        importFound = true;
        import.object->setPrototype(m_valueOwner->cppQmlTypes().objectByCppName(moduleApi.cppName));
    }

    // TODO: at the moment there is not any types information on Qbs imports.
    if (doc->language() == Dialect::QmlQbs)
        importFound = true;

    if (!importFound && importInfo.ast()) {
        import.valid = false;
        error(doc, locationFromRange(importInfo.ast()->firstSourceLocation(),
                                     importInfo.ast()->lastSourceLocation()),
              Link::tr(
                  "QML module not found (%1).\n\n"
                  "Import paths:\n"
                  "%2\n\n"
                  "For qmake projects, use the QML_IMPORT_PATH variable to add import paths.\n"
                  "For Qbs projects, declare and set a qmlImportPaths property in your product "
                  "to add import paths.\n"
                  "For qmlproject projects, use the importPaths property to add import paths.\n"
                  "For CMake projects, make sure QML_IMPORT_PATH variable is in CMakeCache.txt.\n")
              .arg(importInfo.name(), m_importPaths.join(QLatin1Char('\n'))));
    }

    return import;
}

bool LinkPrivate::importLibrary(const Document::Ptr &doc,
                                const QString &libraryPath,
                                Import *import,
                                ObjectValue *targetObject,
                                const QString &importPath)
{
    const ImportInfo &importInfo = import->info;

    LibraryInfo libraryInfo = m_snapshot.libraryInfo(libraryPath);
    if (!libraryInfo.isValid())
        return false;

    import->libraryPath = libraryPath;

    const ComponentVersion version = importInfo.version();
    SourceLocation errorLoc;
    if (const UiImport *ast = importInfo.ast())
        errorLoc = locationFromRange(ast->firstSourceLocation(), ast->lastSourceLocation());

    // Load imports that are mentioned by the "import" command in a qmldir
    // file into the same targetObject, using the same version and "as".
    //
    // Note: Since this works on the same targetObject, the ModuleApi setPrototype()
    // logic will not work. But ModuleApi isn't used in Qt versions that use import
    // commands in qmldir files, and is pending removal in Qt 6.
    for (const auto &importName : libraryInfo.imports()) {
        Import subImport;
        subImport.valid = true;
        subImport.info = ImportInfo::moduleImport(importName, version, importInfo.as(), importInfo.ast());
        subImport.libraryPath = modulePath(importName, version.toString(), m_importPaths);
        bool subImportFound = importLibrary(doc, subImport.libraryPath, &subImport, targetObject, importPath);

        if (!subImportFound && errorLoc.isValid()) {
            import->valid = false;
            error(doc, errorLoc,
                  Link::tr(
                      "Implicit import '%1' of QML module '%2' not found.\n\n"
                      "Import paths:\n"
                      "%3\n\n"
                      "For qmake projects, use the QML_IMPORT_PATH variable to add import paths.\n"
                      "For Qbs projects, declare and set a qmlImportPaths property in your product "
                      "to add import paths.\n"
                      "For qmlproject projects, use the importPaths property to add import paths.\n"
                      "For CMake projects, make sure QML_IMPORT_PATH variable is in CMakeCache.txt.\n")
                  .arg(importName, importInfo.name(), m_importPaths.join(QLatin1Char('\n'))));
        } else if (!subImport.valid) {
            import->valid = false;
        }
    }

    // Load types from qmltypes or plugins
    if (!libraryInfo.plugins().isEmpty() || !libraryInfo.typeInfos().isEmpty()) {
        if (libraryInfo.pluginTypeInfoStatus() == LibraryInfo::NoTypeInfo) {
            ModelManagerInterface *modelManager = ModelManagerInterface::instance();
            if (modelManager) {
                if (importInfo.type() == ImportType::Library) {
                    if (version.isValid()) {
                        const QString uri = importInfo.name();
                        modelManager->loadPluginTypes(
                                    libraryPath, importPath,
                                    uri, version.toString());
                    }
                } else {
                    modelManager->loadPluginTypes(
                                libraryPath, libraryPath,
                                QString(), version.toString());
                }
            }
            if (errorLoc.isValid()) {
                appendDiagnostic(doc, DiagnosticMessage(
                                     Severity::ReadingTypeInfoWarning, errorLoc,
                                     Link::tr("QML module contains C++ plugins, "
                                              "currently reading type information...")));
                import->valid = false;
            }
        } else if (libraryInfo.pluginTypeInfoStatus() == LibraryInfo::DumpError
                   || libraryInfo.pluginTypeInfoStatus() == LibraryInfo::TypeInfoFileError) {
            // Only underline import if package isn't described in .qmltypes anyway
            // and is not a private package
            QString packageName = importInfo.name();
            if (errorLoc.isValid()
                    && (packageName.isEmpty()
                        || !m_valueOwner->cppQmlTypes().hasModule(packageName))
                    && !packageName.endsWith(QLatin1String("private"), Qt::CaseInsensitive)) {
                error(doc, errorLoc, libraryInfo.pluginTypeInfoError());
                import->valid = false;
            }
        } else {
            const QString packageName = importInfo.name();
            m_valueOwner->cppQmlTypes().load(libraryPath, libraryInfo.metaObjects(), packageName);
            const auto objects = m_valueOwner->cppQmlTypes().createObjectsForImport(packageName,
                                                                                    version);
            for (const CppComponentValue *object : objects)
                targetObject->setMember(object->className(), object);

            // all but no-uri module apis become available for import
            QList<ModuleApiInfo> noUriModuleApis;
            const auto moduleApis = libraryInfo.moduleApis();
            for (const ModuleApiInfo &moduleApi : moduleApis) {
                if (moduleApi.uri.isEmpty())
                    noUriModuleApis += moduleApi;
                else
                    importableModuleApis[moduleApi.uri] += moduleApi;
            }

            // if a module api has no uri, it shares the same name
            ModuleApiInfo sameUriModuleApi = findBestModuleApi(noUriModuleApis, version);
            if (sameUriModuleApi.version.isValid()) {
                targetObject->setPrototype(m_valueOwner->cppQmlTypes()
                                             .objectByCppName(sameUriModuleApi.cppName));
            }
        }
    }

    // Load types that are mentioned explicitly in the qmldir
    loadQmldirComponents(targetObject, version, libraryInfo, libraryPath);

    return true;
}

void LinkPrivate::error(const Document::Ptr &doc, const SourceLocation &loc,
                        const QString &message)
{
    appendDiagnostic(doc, DiagnosticMessage(Severity::Error, loc, message));
}

void LinkPrivate::warning(const Document::Ptr &doc, const SourceLocation &loc,
                          const QString &message)
{
    appendDiagnostic(doc, DiagnosticMessage(Severity::Warning, loc, message));
}

void LinkPrivate::appendDiagnostic(const Document::Ptr &doc, const DiagnosticMessage &message)
{
    if (diagnosticMessages && doc->fileName() == document->fileName())
        diagnosticMessages->append(message);
    if (allDiagnosticMessages)
        (*allDiagnosticMessages)[doc->fileName()].append(message);
}

void LinkPrivate::loadQmldirComponents(ObjectValue *import, ComponentVersion version,
                                const LibraryInfo &libraryInfo, const QString &libraryPath)
{
    // if the version isn't valid, import the latest
    if (!version.isValid())
        version = ComponentVersion(ComponentVersion::MaxVersion, ComponentVersion::MaxVersion);


    QSet<QString> importedTypes;
    const auto components = libraryInfo.components();
    for (const QmlDirParser::Component &component : components) {
        if (importedTypes.contains(component.typeName))
            continue;

        ComponentVersion componentVersion(component.majorVersion, component.minorVersion);
        if (version < componentVersion)
            continue;

        importedTypes.insert(component.typeName);
        if (Document::Ptr importedDoc = m_snapshot.document(
                    libraryPath + QLatin1Char('/') + component.fileName)) {
            if (ObjectValue *v = importedDoc->bind()->rootObjectValue())
                import->setMember(component.typeName, v);
        }
    }
}

void LinkPrivate::loadImplicitDirectoryImports(Imports *imports, const Document::Ptr &doc)
{
    auto processImport = [this, imports, doc](const ImportInfo &importInfo){
        Import directoryImport = importCache.value(ImportCacheKey(importInfo));
        if (!directoryImport.object) {
            directoryImport = importFileOrDirectory(doc, importInfo);
            if (directoryImport.object)
                importCache.insert(ImportCacheKey(importInfo), directoryImport);
        }
        if (directoryImport.object)
            imports->append(directoryImport);
    };

    processImport(ImportInfo::implicitDirectoryImport(doc->path()));
    const auto qrcPaths = ModelManagerInterface::instance()->qrcPathsForFile(doc->fileName());
    for (const QString &path : qrcPaths) {
        processImport(ImportInfo::qrcDirectoryImport(
                          Utils::QrcParser::qrcDirectoryPathForQrcFilePath(path)));
    }
}

void LinkPrivate::loadImplicitDefaultImports(Imports *imports)
{
    const QString defaultPackage = CppQmlTypes::defaultPackage;
    if (m_valueOwner->cppQmlTypes().hasModule(defaultPackage)) {
        const ComponentVersion maxVersion(ComponentVersion::MaxVersion,
                                          ComponentVersion::MaxVersion);
        const ImportInfo info = ImportInfo::moduleImport(defaultPackage, maxVersion, QString());
        Import import = importCache.value(ImportCacheKey(info));
        if (!import.object) {
            import.valid = true;
            import.info = info;
            import.object = new ObjectValue(m_valueOwner, QLatin1String("<defaults>"));

            const auto objects = m_valueOwner->cppQmlTypes().createObjectsForImport(defaultPackage,
                                                                                    maxVersion);
            for (const CppComponentValue *object : objects)
                import.object->setMember(object->className(), object);

            importCache.insert(ImportCacheKey(info), import);
        }
        imports->append(import);
    }
}

} // namespace QmlJS
