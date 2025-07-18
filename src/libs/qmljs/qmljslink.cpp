// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljslink.h"

#include "parser/qmljsast_p.h"

#include "qmljsbind.h"
#include "qmljsconstants.h"
#include "qmljsdocument.h"
#include "qmljsmodelmanagerinterface.h"
#include "qmljstr.h"
#include "qmljsutils.h"

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/qrcparser.h>

using namespace LanguageUtils;
using namespace QmlJS::AST;
using namespace Utils;

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
    friend size_t qHash(const ImportCacheKey &);
    friend bool operator==(const ImportCacheKey &, const ImportCacheKey &);

    int m_type;
    QString m_path;
    int m_majorVersion;
    int m_minorVersion;
};

size_t qHash(const ImportCacheKey &info)
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
                       const FilePath &libraryPath,
                       Import *import,
                       ObjectValue *targetObject,
                       const FilePath &importPath = FilePath(),
                       bool optional = false);
    void loadQmldirComponents(ObjectValue *import,
                              LanguageUtils::ComponentVersion version,
                              const LibraryInfo &libraryInfo,
                              const FilePath &libraryPath);
    void loadImplicitDirectoryImports(Imports *imports, const Document::Ptr &doc);
    void loadImplicitDefaultImports(Imports *imports);
    void loadImplicitBuiltinsImports(Imports *imports);

    void error(const Document::Ptr &doc, const SourceLocation &loc, const QString &message);
    void warning(const Document::Ptr &doc, const SourceLocation &loc, const QString &message);
    void appendDiagnostic(const Document::Ptr &doc, const DiagnosticMessage &message);

private:
    friend class Link;

    void loadImplicitImports(Imports *imports, const QString &packageName, const QString &moduleName);

    Snapshot m_snapshot;
    ValueOwner *m_valueOwner = nullptr;
    FilePaths m_importPaths;
    FilePaths m_applicationDirectories;
    LibraryInfo m_builtins;
    ViewerContext m_vContext;

    QHash<ImportCacheKey, Import> importCache;
    QHash<QString, QList<ModuleApiInfo>> importableModuleApis;

    Document::Ptr document;

    QList<DiagnosticMessage> *diagnosticMessages = nullptr;
    QHash<FilePath, QList<DiagnosticMessage>> *allDiagnosticMessages = nullptr;
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
    const FilePaths list(vContext.paths.begin(), vContext.paths.end());
    d->m_importPaths = list;
    d->m_applicationDirectories = vContext.applicationDirectories;
    d->m_builtins = builtins;
    d->m_vContext = vContext;

    d->diagnosticMessages = nullptr;
    d->allDiagnosticMessages = nullptr;

    QFutureInterface<void> fi;
    fi.reportStarted();
    ModelManagerInterface *modelManager = ModelManagerInterface::instanceForFuture(fi.future());
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
    fi.reportFinished();
}

ContextPtr Link::operator()(QHash<FilePath, QList<DiagnosticMessage>> *messages)
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
                                         CppQmlTypesLoader::defaultQtObjects());
    }

    // load library objects shipped with Creator
    m_valueOwner->cppQmlTypes().load(QLatin1String("<defaultQt4>"),
                                     CppQmlTypesLoader::defaultLibraryObjects());

    if (document) {
        // do it on document first, to make sure import errors are shown
        auto *imports = new Imports(m_valueOwner);

        // Add custom imports for the opened document
        for (const auto &provider : CustomImportsProvider::allProviders()) {
            const QList<Import> providerImports = provider->imports(m_valueOwner,
                                                           document.data(),
                                                           &m_snapshot);
            for (const Import &import : providerImports)
                importCache.insert(ImportCacheKey(import.info), import);
            provider->loadBuiltins(&importsPerDocument,
                                   imports,
                                   document.data(),
                                   m_valueOwner,
                                   &m_snapshot);
            m_importPaths = provider->prioritizeImportPaths(document.data(), m_importPaths);
        }

        populateImportedTypes(imports, document);
        importsPerDocument.insert(document.data(), QSharedPointer<Imports>(imports));
    }

    for (const Document::Ptr &doc : std::as_const(m_snapshot)) {
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

    // implicit imports: the <default> package is always available (except when the QML package was loaded).
    // In the latter case, still try to load the <default>'s module <defaults>.
    loadImplicitDefaultImports(imports);
    // Load the <builtins> import, if the QML package was loaded.
    loadImplicitBuiltinsImports(imports);

    // implicit imports:
    // qml files in the same directory are available without explicit imports
    if (doc->isQmlDocument())
        loadImplicitDirectoryImports(imports, doc);

    // explicit imports, whether directories, files or libraries
    const QList<ImportInfo> docImports = doc->bind()->imports();
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
                          Tr::tr("File or directory not found."));
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

    QString pathStr = importInfo.path();
    FilePath path = FilePath::fromString(pathStr);

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
        FilePaths filePaths = ModelManagerInterface::instance()
                                    ->filesAtQrcPath(pathStr,
                                                     &locale,
                                                     nullptr,
                                                     ModelManagerInterface::ActiveQrcResources);
        if (filePaths.isEmpty())
            filePaths = ModelManagerInterface::instance()->filesAtQrcPath(pathStr);
        if (!filePaths.isEmpty()) {
            if (Document::Ptr importedDoc = m_snapshot.document(filePaths.at(0)))
                import.object = importedDoc->bind()->rootObjectValue();
        }
    } else if (importInfo.type() == ImportType::QrcDirectory){
        import.object = new ObjectValue(m_valueOwner);

        importLibrary(doc, path, &import, import.object);

        const QMap<QString, FilePaths> paths = ModelManagerInterface::instance()->filesInQrcPath(
            pathStr);
        for (auto iter = paths.cbegin(), end = paths.cend(); iter != end; ++iter) {
            if (ModelManagerInterface::guessLanguageOfFile(FilePath::fromString(iter.key()))
                    .isQmlLikeLanguage()) {
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

    const FilePaths libraryPaths = modulePaths(packageName,
                                               version.toString(),
                                               m_importPaths + m_applicationDirectories);

    bool importFound = false;
    for (const FilePath &libPath : libraryPaths) {
        importFound = !libPath.isEmpty() && importLibrary(doc, libPath, &import, import.object);
        if (importFound)
            break;
    }

    if (!importFound) {
        for (const FilePath &dir : std::as_const(m_applicationDirectories)) {
            FilePaths qmltypes = dir.dirEntries(
                FileFilter(QStringList{"*.qmltypes"}, QDir::Files));

            // This adds the types to the C++ types, to be found below if applicable.
            if (!qmltypes.isEmpty())
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
        error(doc,
              locationFromRange(importInfo.ast()->firstSourceLocation(),
                                importInfo.ast()->lastSourceLocation()),
              Tr::tr(
                  "QML module not found (%1).\n\n"
                  "Import paths:\n"
                  "%2\n\n"
                  "For qmake projects, use the QML_IMPORT_PATH variable to add import paths.\n"
                  "For Qbs projects, declare and set a qmlImportPaths property in your product "
                  "to add import paths.\n"
                  "For qmlproject projects, use the importPaths property to add import paths.\n"
                  "For CMake projects, make sure QML_IMPORT_PATH variable is in CMakeCache.txt.\n"
                  "For qmlRegister... calls, make sure that you define the Module URI as a string literal.\n")
                  .arg(importInfo.name(), m_importPaths.toUserOutput("\n")));
    }

    return import;
}

bool LinkPrivate::importLibrary(const Document::Ptr &doc,
                                const FilePath &libraryPath,
                                Import *import,
                                ObjectValue *targetObject,
                                const FilePath &importPath,
                                bool optional)
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
    for (const auto &toImport : libraryInfo.imports()) {
        QString importName = toImport.module;

        // These implicit imports do not work reliable, since each type
        // is only added to one import/module. If a type is added here,
        // it is not added to the actual module, because of caching.
        //
        // In case of QtQuick3D.MaterialEditor this leads to a reproducible issue.
        // As a workaround we simply skip all implicit imports for QtQuick3D.
        if (importName == "QtQuick3D")
            continue;

        ComponentVersion vNow = toImport.version;
        /* There was a period in which no version == auto
         * Required for QtQuick imports less than 2.15
         */
        if ((toImport.flags & QmlDirParser::Import::Auto)
                || !vNow.isValid())
            vNow = version;
        Import subImport;
        subImport.valid = true;
        subImport.info = ImportInfo::moduleImport(importName, vNow, importInfo.as(), importInfo.ast());

        const FilePaths libraryPaths = modulePaths(importName, vNow.toString(), m_importPaths);
        subImport.libraryPath = libraryPaths.value(0); // first is the best match

        bool subImportFound = importLibrary(doc, subImport.libraryPath, &subImport, targetObject, importPath, true);

        if (!subImportFound && errorLoc.isValid()) {
            if (!(optional || (toImport.flags & QmlDirParser::Import::Optional))) {
                import->valid = false;
                error(doc,
                      errorLoc,
                      Tr::tr("Implicit import '%1' of QML module '%2' not found.\n\n"
                             "Import paths:\n"
                             "%3\n\n"
                             "For qmake projects, use the QML_IMPORT_PATH variable to add import "
                             "paths.\n"
                             "For Qbs projects, declare and set a qmlImportPaths property in "
                             "your product "
                             "to add import paths.\n"
                             "For qmlproject projects, use the importPaths property to add "
                             "import paths.\n"
                             "For CMake projects, make sure QML_IMPORT_PATH variable is in "
                             "CMakeCache.txt.\n")
                      .arg(importName, importInfo.name(), m_importPaths.toUserOutput("\n")));
            }
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
            if (!optional && errorLoc.isValid()) {
                appendDiagnostic(doc, DiagnosticMessage(
                                     Severity::ReadingTypeInfoWarning, errorLoc,
                                     Tr::tr("QML module contains C++ plugins, "
                                            "currently reading type information... %1")
                                     .arg(import->info.name())));
                import->valid = false;
            }
        } else if (libraryInfo.pluginTypeInfoStatus() == LibraryInfo::DumpError
                   || libraryInfo.pluginTypeInfoStatus() == LibraryInfo::TypeInfoFileError) {
            // Only underline import if package isn't described in .qmltypes anyway
            // and is not a private package
            QString packageName = importInfo.name();
            if (!optional && errorLoc.isValid()
                    && (packageName.isEmpty()
                        || !m_valueOwner->cppQmlTypes().hasModule(packageName))
                    && !packageName.endsWith(QLatin1String("private"), Qt::CaseInsensitive)) {
                error(doc, errorLoc, libraryInfo.pluginTypeInfoError());
                import->valid = false;
            }
        } else {
            const QString packageName = importInfo.name();
            m_valueOwner->cppQmlTypes().load(libraryPath.toUrlishString(),
                                             libraryInfo.metaObjects(),
                                             packageName);
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

void LinkPrivate::loadQmldirComponents(ObjectValue *import,
                                       ComponentVersion version,
                                       const LibraryInfo &libraryInfo,
                                       const FilePath &libraryPath)
{
    // if the version isn't valid, import the latest
    if (!version.isValid())
        version = ComponentVersion(ComponentVersion::MaxVersion, ComponentVersion::MaxVersion);


    QSet<QString> importedTypes;
    const auto components = libraryInfo.components();
    for (const QmlDirParser::Component &component : components) {
        ComponentVersion componentVersion(component.majorVersion, component.minorVersion);
        if (version < componentVersion)
            continue;

        if (!Utils::insert(importedTypes, component.typeName))
            continue;
        if (Document::Ptr importedDoc = m_snapshot.document(
                libraryPath.pathAppended(component.fileName))) {
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

    processImport(ImportInfo::implicitDirectoryImport(doc->path().path()));
    const auto qrcPaths = ModelManagerInterface::instance()->qrcPathsForFile(doc->fileName());
    for (const QString &path : qrcPaths) {
        processImport(ImportInfo::qrcDirectoryImport(
                          QrcParser::qrcDirectoryPathForQrcFilePath(path)));
    }
}

void LinkPrivate::loadImplicitImports(Imports *imports, const QString &packageName, const QString &moduleName)
{
    if (m_valueOwner->cppQmlTypes().hasModule(packageName)) {
        const ComponentVersion maxVersion(ComponentVersion::MaxVersion,
                                          ComponentVersion::MaxVersion);
        const ImportInfo info = ImportInfo::moduleImport(packageName, maxVersion, QString());
        Import import = importCache.value(ImportCacheKey(info));
        if (!import.object) {
            import.valid = true;
            import.info = info;
            import.object = new ObjectValue(m_valueOwner, moduleName);

            const auto objects = m_valueOwner->cppQmlTypes().createObjectsForImport(packageName,
                                                                                    maxVersion);
            for (const CppComponentValue *object : objects)
                import.object->setMember(object->className(), object);

            importCache.insert(ImportCacheKey(info), import);
        }
        imports->append(import);
    }
}

void LinkPrivate::loadImplicitDefaultImports(Imports *imports)
{
    loadImplicitImports(imports, CppQmlTypes::defaultPackage, QLatin1String("<defaults>"));
}

void LinkPrivate::loadImplicitBuiltinsImports(Imports *imports)
{
    loadImplicitImports(imports, QLatin1String("QML"), QLatin1String("<builtins>"));
}

} // namespace QmlJS
