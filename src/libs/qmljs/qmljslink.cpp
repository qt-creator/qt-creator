/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmljslink.h"

#include "parser/qmljsast_p.h"
#include "qmljsdocument.h"
#include "qmljsbind.h"
#include "qmljscheck.h"
#include "qmljsmodelmanagerinterface.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>

using namespace LanguageUtils;
using namespace QmlJS;
using namespace QmlJS::AST;

namespace {
class ImportCacheKey
{
public:
    explicit ImportCacheKey(const ImportInfo &info)
        : type(info.type())
        , name(info.name())
        , majorVersion(info.version().majorVersion())
        , minorVersion(info.version().minorVersion())
    {}

    int type;
    QString name;
    int majorVersion;
    int minorVersion;
};

uint qHash(const ImportCacheKey &info)
{
    return ::qHash(info.type) ^ ::qHash(info.name) ^
            ::qHash(info.majorVersion) ^ ::qHash(info.minorVersion);
}

bool operator==(const ImportCacheKey &i1, const ImportCacheKey &i2)
{
    return i1.type == i2.type
            && i1.name == i2.name
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

    Document::Ptr document;
    QList<DiagnosticMessage> *diagnosticMessages;

    QHash<QString, QList<DiagnosticMessage> > *allDiagnosticMessages;

    static AST::UiQualifiedId *qualifiedTypeNameId(AST::Node *node);

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
    \brief Initializes the Context for a Document.
    \sa QmlJS::Document QmlJS::Context

    Initializes a context by resolving imports and building the root scope
    chain. Currently, this is a expensive operation.

    It's recommended to use a the \l{LookupContext} returned by
    \l{QmlJSEditor::SemanticInfo::lookupContext()} instead of building a new
    \l{Context} with \l{Link}.
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

        // populate global object with context properties from C++
        ObjectValue *global = d->valueOwner->globalObject();
        foreach (const ModelManagerInterface::CppData &cppData, cppDataHash) {
            QHashIterator<QString, QString> it(cppData.contextProperties);
            while (it.hasNext()) {
                it.next();
                const Value *value = 0;
                const QString cppTypeName = it.value();
                if (!cppTypeName.isEmpty())
                    value = d->valueOwner->cppQmlTypes().objectByCppName(cppTypeName);
                if (!value)
                    value = d->valueOwner->undefinedValue();
                global->setMember(it.key(), value);
            }
        }
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

    // the 'Qt' object is dumped even though it is not exported
    // it contains useful information, in particular on enums - add the
    // object as a prototype to our custom Qt object to offer these for completion
    const_cast<ObjectValue *>(valueOwner->qtObject())->setPrototype(valueOwner->cppQmlTypes().objectByCppName(QLatin1String("Qt")));

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
    if (! doc->qmlProgram())
        return;

    // implicit imports: the <default> package is always available
    loadImplicitDefaultImports(imports);

    // implicit imports:
    // qml files in the same directory are available without explicit imports
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
                error(doc, info.ast()->fileNameToken,
                      Link::tr("file or directory not found"));
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

    const QString &path = importInfo.name();

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

    const QString packageName = Bind::toString(importInfo.ast()->importUri, '.');
    const ComponentVersion version = importInfo.version();

    bool importFound = false;

    const QString &packagePath = importInfo.name();
    // check the filesystem with full version
    foreach (const QString &importPath, importPaths) {
        QString libraryPath = QString("%1/%2.%3").arg(importPath, packagePath, version.toString());
        if (importLibrary(doc, libraryPath, &import, importPath)) {
            importFound = true;
            break;
        }
    }
    if (!importFound) {
        // check the filesystem with major version
        foreach (const QString &importPath, importPaths) {
            QString libraryPath = QString("%1/%2.%3").arg(importPath, packagePath,
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
            QString libraryPath = QString("%1/%2").arg(importPath, packagePath);
            if (importLibrary(doc, libraryPath, &import, importPath)) {
                importFound = true;
                break;
            }
        }
    }

    // if there are cpp-based types for this package, use them too
    if (valueOwner->cppQmlTypes().hasModule(packageName)) {
        importFound = true;
        foreach (const QmlObjectValue *object,
                 valueOwner->cppQmlTypes().createObjectsForImport(packageName, version)) {
            import.object->setMember(object->className(), object);
        }
    }

    if (!importFound && importInfo.ast()) {
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
    const UiImport *ast = importInfo.ast();
    SourceLocation errorLoc;
    if (ast)
        errorLoc = locationFromRange(ast->firstSourceLocation(), ast->lastSourceLocation());

    if (!libraryInfo.plugins().isEmpty()) {
        if (libraryInfo.pluginTypeInfoStatus() == LibraryInfo::NoTypeInfo) {
            ModelManagerInterface *modelManager = ModelManagerInterface::instance();
            if (modelManager) {
                if (importInfo.type() == ImportInfo::LibraryImport) {
                    if (importInfo.version().isValid()) {
                        const QString uri = importInfo.name().replace(QDir::separator(), QLatin1Char('.'));
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
            }
        } else if (libraryInfo.pluginTypeInfoStatus() == LibraryInfo::DumpError
                   || libraryInfo.pluginTypeInfoStatus() == LibraryInfo::TypeInfoFileError) {
            // Only underline import if package isn't described in .qmltypes anyway
            QString packageName;
            if (ast && ast->importUri)
                packageName = Bind::toString(importInfo.ast()->importUri, '.');
            if (errorLoc.isValid() && (packageName.isEmpty() || !valueOwner->cppQmlTypes().hasModule(packageName))) {
                error(doc, errorLoc, libraryInfo.pluginTypeInfoError());
            }
        } else if (ast && ast->importUri) {
            const QString packageName = Bind::toString(importInfo.ast()->importUri, '.');
            valueOwner->cppQmlTypes().load(libraryInfo.metaObjects(), packageName);
            foreach (const QmlObjectValue *object, valueOwner->cppQmlTypes().createObjectsForImport(packageName, version)) {
                import->object->setMember(object->className(), object);
            }
        }
    }

    loadQmldirComponents(import->object, version, libraryInfo, libraryPath);

    return true;
}

UiQualifiedId *LinkPrivate::qualifiedTypeNameId(Node *node)
{
    if (UiObjectBinding *binding = AST::cast<UiObjectBinding *>(node))
        return binding->qualifiedTypeNameId;
    else if (UiObjectDefinition *binding = AST::cast<UiObjectDefinition *>(node))
        return binding->qualifiedTypeNameId;
    else
        return 0;
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
    if (!version.isValid()) {
        version = ComponentVersion(ComponentVersion::MaxVersion, ComponentVersion::MaxVersion);
    }


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
    ImportInfo implcitDirectoryImportInfo(
                ImportInfo::ImplicitDirectoryImport, doc->path());

    Import directoryImport = importCache.value(ImportCacheKey(implcitDirectoryImportInfo));
    if (!directoryImport.object) {
        directoryImport = importFileOrDirectory(doc, implcitDirectoryImportInfo);
        if (directoryImport.object)
            importCache.insert(ImportCacheKey(implcitDirectoryImportInfo), directoryImport);
    }
    if (directoryImport.object) {
        imports->append(directoryImport);
    }
}

void LinkPrivate::loadImplicitDefaultImports(Imports *imports)
{
    const QString defaultPackage = CppQmlTypes::defaultPackage;
    if (valueOwner->cppQmlTypes().hasModule(defaultPackage)) {
        ImportInfo info(ImportInfo::LibraryImport, defaultPackage);
        Import import = importCache.value(ImportCacheKey(info));
        if (!import.object) {
            import.info = info;
            import.object = new ObjectValue(valueOwner);
            foreach (const QmlObjectValue *object,
                     valueOwner->cppQmlTypes().createObjectsForImport(
                         defaultPackage,
                         ComponentVersion(ComponentVersion::MaxVersion, ComponentVersion::MaxVersion))) {
                import.object->setMember(object->className(), object);
            }
            importCache.insert(ImportCacheKey(info), import);
        }
        imports->append(import);
    }
}
