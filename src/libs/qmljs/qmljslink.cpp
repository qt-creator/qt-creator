/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmljslink.h"

#include "parser/qmljsast_p.h"
#include "qmljsdocument.h"
#include "qmljsbind.h"
#include "qmljsutils.h"
#include "qmljsmodelmanagerinterface.h"

#include <QFileInfo>
#include <QDir>
#include <QDebug>

using namespace LanguageUtils;
using namespace QmlJS;
using namespace QmlJS::AST;

namespace {
class ImportCacheKey
{
public:
    explicit ImportCacheKey(const ImportInfo &info)
        : type(info.type())
        , path(info.path())
        , majorVersion(info.version().majorVersion())
        , minorVersion(info.version().minorVersion())
    {}

    int type;
    QString path;
    int majorVersion;
    int minorVersion;
};

uint qHash(const ImportCacheKey &info)
{
    return ::qHash(info.type) ^ ::qHash(info.path) ^
            ::qHash(info.majorVersion) ^ ::qHash(info.minorVersion);
}

bool operator==(const ImportCacheKey &i1, const ImportCacheKey &i2)
{
    return i1.type == i2.type
            && i1.path == i2.path
            && i1.majorVersion == i2.majorVersion
            && i1.minorVersion == i2.minorVersion;
}
}


class QmlJS::LinkPrivate
{
public:
    Snapshot snapshot;
    ValueOwner *valueOwner;
    QStringList importPaths;
    LibraryInfo builtins;

    QHash<ImportCacheKey, Import> importCache;

    QHash<QString, QList<ModuleApiInfo> > importableModuleApis;

    Document::Ptr document;
    QList<DiagnosticMessage> *diagnosticMessages;

    QHash<QString, QList<DiagnosticMessage> > *allDiagnosticMessages;

    Context::ImportsPerDocument linkImports();

    void populateImportedTypes(Imports *imports, Document::Ptr doc);
    Import importFileOrDirectory(
        Document::Ptr doc,
        const ImportInfo &importInfo);
    Import importNonFile(
        Document::Ptr doc,
        const ImportInfo &importInfo);
    void importObject(Bind *bind, const QString &name, ObjectValue *object, NameId *targetNamespace);

    bool importLibrary(Document::Ptr doc,
                       const QString &libraryPath,
                       Import *import,
                       const QString &importPath = QString());
    void loadQmldirComponents(ObjectValue *import,
                              LanguageUtils::ComponentVersion version,
                              const LibraryInfo &libraryInfo,
                              const QString &libraryPath);
    void loadImplicitDirectoryImports(Imports *imports, Document::Ptr doc);
    void loadImplicitDefaultImports(Imports *imports);

    void error(const Document::Ptr &doc, const AST::SourceLocation &loc, const QString &message);
    void warning(const Document::Ptr &doc, const AST::SourceLocation &loc, const QString &message);
    void appendDiagnostic(const Document::Ptr &doc, const DiagnosticMessage &message);
};

/*!
    \class QmlJS::Link
    \brief Creates a Context for a Snapshot.
    \sa Context Snapshot

    Initializes a context by resolving imports. This is an expensive operation.

    Instead of making a fresh context, consider reusing the one maintained in the
    \l{QmlJSEditor::SemanticInfo} of a \l{QmlJSEditor::QmlJSTextEditorWidget}.
*/

Link::Link(const Snapshot &snapshot, const QStringList &importPaths, const LibraryInfo &builtins)
    : d(new LinkPrivate)
{
    d->valueOwner = new ValueOwner;
    d->snapshot = snapshot;
    d->importPaths = importPaths;
    d->builtins = builtins;

    d->diagnosticMessages = 0;
    d->allDiagnosticMessages = 0;

    ModelManagerInterface *modelManager = ModelManagerInterface::instance();
    if (modelManager) {
        ModelManagerInterface::CppDataHash cppDataHash = modelManager->cppData();

        // populate engine with types from C++
        foreach (const ModelManagerInterface::CppData &cppData, cppDataHash) {
            d->valueOwner->cppQmlTypes().load(cppData.exportedTypes);
        }

        // build an object with the context properties from C++
        ObjectValue *cppContextProperties = d->valueOwner->newObject(/* prototype = */ 0);
        foreach (const ModelManagerInterface::CppData &cppData, cppDataHash) {
            QHashIterator<QString, QString> it(cppData.contextProperties);
            while (it.hasNext()) {
                it.next();
                const Value *value = 0;
                const QString cppTypeName = it.value();
                if (!cppTypeName.isEmpty())
                    value = d->valueOwner->cppQmlTypes().objectByCppName(cppTypeName);
                if (!value)
                    value = d->valueOwner->unknownValue();
                cppContextProperties->setMember(it.key(), value);
            }
        }
        d->valueOwner->cppQmlTypes().setCppContextProperties(cppContextProperties);
    }
}

ContextPtr Link::operator()(QHash<QString, QList<DiagnosticMessage> > *messages)
{
    d->allDiagnosticMessages = messages;
    return Context::create(d->snapshot, d->valueOwner, d->linkImports());
}

ContextPtr Link::operator()(const Document::Ptr &doc, QList<DiagnosticMessage> *messages)
{
    d->document = doc;
    d->diagnosticMessages = messages;
    return Context::create(d->snapshot, d->valueOwner, d->linkImports());
}

Link::~Link()
{
    delete d;
}

Context::ImportsPerDocument LinkPrivate::linkImports()
{
    Context::ImportsPerDocument importsPerDocument;

    // load builtin objects
    if (builtins.pluginTypeInfoStatus() == LibraryInfo::DumpDone
            || builtins.pluginTypeInfoStatus() == LibraryInfo::TypeInfoFileDone) {
        valueOwner->cppQmlTypes().load(builtins.metaObjects());
    } else {
        valueOwner->cppQmlTypes().load(CppQmlTypesLoader::defaultQtObjects);
    }

    // load library objects shipped with Creator
    valueOwner->cppQmlTypes().load(CppQmlTypesLoader::defaultLibraryObjects);

    if (document) {
        // do it on document first, to make sure import errors are shown
        Imports *imports = new Imports(valueOwner);
        populateImportedTypes(imports, document);
        importsPerDocument.insert(document.data(), QSharedPointer<Imports>(imports));
    }

    foreach (Document::Ptr doc, snapshot) {
        if (doc == document)
            continue;

        Imports *imports = new Imports(valueOwner);
        populateImportedTypes(imports, doc);
        importsPerDocument.insert(doc.data(), QSharedPointer<Imports>(imports));
    }

    return importsPerDocument;
}

void LinkPrivate::populateImportedTypes(Imports *imports, Document::Ptr doc)
{
    importableModuleApis.clear();

    // implicit imports: the <default> package is always available
    loadImplicitDefaultImports(imports);

    // implicit imports:
    // qml files in the same directory are available without explicit imports
    if (doc->isQmlDocument())
        loadImplicitDirectoryImports(imports, doc);

    // explicit imports, whether directories, files or libraries
    foreach (const ImportInfo &info, doc->bind()->imports()) {
        Import import = importCache.value(ImportCacheKey(info));

        // ensure usage of the right ImportInfo, the cached import
        // can have a different 'as' clause...
        import.info = info;

        if (!import.object) {
            switch (info.type()) {
            case ImportInfo::FileImport:
            case ImportInfo::DirectoryImport:
                import = importFileOrDirectory(doc, info);
                break;
            case ImportInfo::LibraryImport:
                import = importNonFile(doc, info);
                break;
            case ImportInfo::UnknownFileImport:
                imports->setImportFailed();
                if (info.ast()) {
                    error(doc, info.ast()->fileNameToken,
                          Link::tr("file or directory not found"));
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
}

/*
    import "content"
    import "content" as Xxx
    import "content" 4.6
    import "content" 4.6 as Xxx

    import "http://www.ovi.com/" as Ovi

    import "file.js" as Foo
*/
Import LinkPrivate::importFileOrDirectory(Document::Ptr doc, const ImportInfo &importInfo)
{
    Import import;
    import.info = importInfo;
    import.object = 0;
    import.valid = true;

    const QString &path = importInfo.path();

    if (importInfo.type() == ImportInfo::DirectoryImport
            || importInfo.type() == ImportInfo::ImplicitDirectoryImport) {
        import.object = new ObjectValue(valueOwner);

        importLibrary(doc, path, &import);

        const QList<Document::Ptr> &documentsInDirectory = snapshot.documentsInDirectory(path);
        foreach (Document::Ptr importedDoc, documentsInDirectory) {
            if (importedDoc->bind()->rootObjectValue()) {
                const QString targetName = importedDoc->componentName();
                import.object->setMember(targetName, importedDoc->bind()->rootObjectValue());
            }
        }
    } else if (importInfo.type() == ImportInfo::FileImport) {
        Document::Ptr importedDoc = snapshot.document(path);
        if (importedDoc)
            import.object = importedDoc->bind()->rootObjectValue();
    }

    return import;
}

static ModuleApiInfo findBestModuleApi(const QList<ModuleApiInfo> &apis, const ComponentVersion &version)
{
    ModuleApiInfo best;
    foreach (const ModuleApiInfo &moduleApi, apis) {
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
  (import com.nokia.qt is the same as the ones above)
*/
Import LinkPrivate::importNonFile(Document::Ptr doc, const ImportInfo &importInfo)
{
    Import import;
    import.info = importInfo;
    import.object = new ObjectValue(valueOwner);
    import.valid = true;

    const QString packageName = importInfo.name();
    const ComponentVersion version = importInfo.version();

    bool importFound = false;

    const QString &packagePath = importInfo.path();
    // check the filesystem with full version
    foreach (const QString &importPath, importPaths) {
        QString libraryPath = QString::fromLatin1("%1/%2.%3").arg(importPath, packagePath, version.toString());
        if (importLibrary(doc, libraryPath, &import, importPath)) {
            importFound = true;
            break;
        }
    }
    if (!importFound) {
        // check the filesystem with major version
        foreach (const QString &importPath, importPaths) {
            QString libraryPath = QString::fromLatin1("%1/%2.%3").arg(importPath, packagePath,
                                                          QString::number(version.majorVersion()));
            if (importLibrary(doc, libraryPath, &import, importPath)) {
                importFound = true;
                break;
            }
        }
    }
    if (!importFound) {
        // check the filesystem with no version
        foreach (const QString &importPath, importPaths) {
            QString libraryPath = QString::fromLatin1("%1/%2").arg(importPath, packagePath);
            if (importLibrary(doc, libraryPath, &import, importPath)) {
                importFound = true;
                break;
            }
        }
    }

    // if there are cpp-based types for this package, use them too
    if (valueOwner->cppQmlTypes().hasModule(packageName)) {
        importFound = true;
        foreach (const CppComponentValue *object,
                 valueOwner->cppQmlTypes().createObjectsForImport(packageName, version)) {
            import.object->setMember(object->className(), object);
        }
    }

    // check module apis that previous imports may have enabled
    ModuleApiInfo moduleApi = findBestModuleApi(importableModuleApis.value(packageName), version);
    if (moduleApi.version.isValid()) {
        importFound = true;
        import.object->setPrototype(valueOwner->cppQmlTypes().objectByCppName(moduleApi.cppName));
    }

    if (!importFound && importInfo.ast()) {
        import.valid = false;
        error(doc, locationFromRange(importInfo.ast()->firstSourceLocation(),
                                     importInfo.ast()->lastSourceLocation()),
              Link::tr(
                  "QML module not found\n\n"
                  "Import paths:\n"
                  "%1\n\n"
                  "For qmake projects, use the QML_IMPORT_PATH variable to add import paths.\n"
                  "For qmlproject projects, use the importPaths property to add import paths.").arg(
                  importPaths.join(QLatin1String("\n"))));
    }

    return import;
}

bool LinkPrivate::importLibrary(Document::Ptr doc,
                         const QString &libraryPath,
                         Import *import,
                         const QString &importPath)
{
    const ImportInfo &importInfo = import->info;

    const LibraryInfo libraryInfo = snapshot.libraryInfo(libraryPath);
    if (!libraryInfo.isValid())
        return false;

    import->libraryPath = libraryPath;

    const ComponentVersion version = importInfo.version();
    SourceLocation errorLoc;
    if (const UiImport *ast = importInfo.ast())
        errorLoc = locationFromRange(ast->firstSourceLocation(), ast->lastSourceLocation());

    if (!libraryInfo.plugins().isEmpty()) {
        if (libraryInfo.pluginTypeInfoStatus() == LibraryInfo::NoTypeInfo) {
            ModelManagerInterface *modelManager = ModelManagerInterface::instance();
            if (modelManager) {
                if (importInfo.type() == ImportInfo::LibraryImport) {
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
                warning(doc, errorLoc,
                        Link::tr("QML module contains C++ plugins, currently reading type information..."));
                import->valid = false;
            }
        } else if (libraryInfo.pluginTypeInfoStatus() == LibraryInfo::DumpError
                   || libraryInfo.pluginTypeInfoStatus() == LibraryInfo::TypeInfoFileError) {
            // Only underline import if package isn't described in .qmltypes anyway
            QString packageName = importInfo.name();
            if (errorLoc.isValid() && (packageName.isEmpty() || !valueOwner->cppQmlTypes().hasModule(packageName))) {
                error(doc, errorLoc, libraryInfo.pluginTypeInfoError());
                import->valid = false;
            }
        } else {
            const QString packageName = importInfo.name();
            valueOwner->cppQmlTypes().load(libraryInfo.metaObjects(), packageName);
            foreach (const CppComponentValue *object, valueOwner->cppQmlTypes().createObjectsForImport(packageName, version)) {
                import->object->setMember(object->className(), object);
            }

            // all but no-uri module apis become available for import
            QList<ModuleApiInfo> noUriModuleApis;
            foreach (const ModuleApiInfo &moduleApi, libraryInfo.moduleApis()) {
                if (moduleApi.uri.isEmpty())
                    noUriModuleApis += moduleApi;
                else
                    importableModuleApis[moduleApi.uri] += moduleApi;
            }

            // if a module api has no uri, it shares the same name
            ModuleApiInfo sameUriModuleApi = findBestModuleApi(noUriModuleApis, version);
            if (sameUriModuleApi.version.isValid())
                import->object->setPrototype(valueOwner->cppQmlTypes().objectByCppName(sameUriModuleApi.cppName));
        }
    }

    loadQmldirComponents(import->object, version, libraryInfo, libraryPath);

    return true;
}

void LinkPrivate::error(const Document::Ptr &doc, const AST::SourceLocation &loc, const QString &message)
{
    appendDiagnostic(doc, DiagnosticMessage(DiagnosticMessage::Error, loc, message));
}

void LinkPrivate::warning(const Document::Ptr &doc, const AST::SourceLocation &loc, const QString &message)
{
    appendDiagnostic(doc, DiagnosticMessage(DiagnosticMessage::Warning, loc, message));
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
    foreach (const QmlDirParser::Component &component, libraryInfo.components()) {
        if (importedTypes.contains(component.typeName))
            continue;

        ComponentVersion componentVersion(component.majorVersion,
                                          component.minorVersion);
        if (version < componentVersion)
            continue;

        importedTypes.insert(component.typeName);
        if (Document::Ptr importedDoc = snapshot.document(
                    libraryPath + QDir::separator() + component.fileName)) {
            if (ObjectValue *v = importedDoc->bind()->rootObjectValue())
                import->setMember(component.typeName, v);
        }
    }
}

void LinkPrivate::loadImplicitDirectoryImports(Imports *imports, Document::Ptr doc)
{
    ImportInfo implcitDirectoryImportInfo = ImportInfo::implicitDirectoryImport(doc->path());

    Import directoryImport = importCache.value(ImportCacheKey(implcitDirectoryImportInfo));
    if (!directoryImport.object) {
        directoryImport = importFileOrDirectory(doc, implcitDirectoryImportInfo);
        if (directoryImport.object)
            importCache.insert(ImportCacheKey(implcitDirectoryImportInfo), directoryImport);
    }
    if (directoryImport.object)
        imports->append(directoryImport);
}

void LinkPrivate::loadImplicitDefaultImports(Imports *imports)
{
    const QString defaultPackage = CppQmlTypes::defaultPackage;
    if (valueOwner->cppQmlTypes().hasModule(defaultPackage)) {
        const ComponentVersion maxVersion(ComponentVersion::MaxVersion, ComponentVersion::MaxVersion);
        const ImportInfo info = ImportInfo::moduleImport(defaultPackage, maxVersion, QString());
        Import import = importCache.value(ImportCacheKey(info));
        if (!import.object) {
            import.valid = true;
            import.info = info;
            import.object = new ObjectValue(valueOwner);
            foreach (const CppComponentValue *object,
                     valueOwner->cppQmlTypes().createObjectsForImport(
                         defaultPackage, maxVersion)) {
                import.object->setMember(object->className(), object);
            }
            importCache.insert(ImportCacheKey(info), import);
        }
        imports->append(import);
    }
}
