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
using namespace QmlJS::Interpreter;
using namespace QmlJS::AST;

namespace {
class ImportCacheKey
{
public:
    explicit ImportCacheKey(const Interpreter::ImportInfo &info)
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
    Interpreter::Context *context;
    QStringList importPaths;

    QHash<ImportCacheKey, Import> importCache;

    Document::Ptr doc;
    QList<DiagnosticMessage> *diagnosticMessages;

    QHash<QString, QList<DiagnosticMessage> > *allDiagnosticMessages;
};

/*!
    \class QmlJS::Link
    \brief Initializes the Context for a Document.
    \sa QmlJS::Document QmlJS::Interpreter::Context

    Initializes a context by resolving imports and building the root scope
    chain. Currently, this is a expensive operation.

    It's recommended to use a the \l{LookupContext} returned by
    \l{QmlJSEditor::SemanticInfo::lookupContext()} instead of building a new
    \l{Context} with \l{Link}.
*/

Link::Link(Context *context, const Snapshot &snapshot, const QStringList &importPaths)
    : d_ptr(new LinkPrivate)
{
    Q_D(Link);
    d->context = context;
    d->snapshot = snapshot;
    d->importPaths = importPaths;

    d->diagnosticMessages = 0;
    d->allDiagnosticMessages = 0;

    // populate engine with types from C++
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();
    if (modelManager) {
        foreach (const QList<FakeMetaObject::ConstPtr> &cppTypes, modelManager->cppQmlTypes()) {
            engine()->cppQmlTypes().load(engine(), cppTypes);
        }
    }
}

void Link::operator()(QHash<QString, QList<DiagnosticMessage> > *messages)
{
    Q_D(Link);
    d->allDiagnosticMessages = messages;
    linkImports();
}

void Link::operator()(const Document::Ptr &doc, QList<DiagnosticMessage> *messages)
{
    Q_D(Link);
    d->doc = doc;
    d->diagnosticMessages = messages;
    linkImports();
}

Link::~Link()
{
}

Interpreter::Engine *Link::engine()
{
    Q_D(Link);
    return d->context->engine();
}

void Link::linkImports()
{
    Q_D(Link);

    if (d->doc) {
        // do it on d->doc first, to make sure import errors are shown
        Imports *imports = new Imports(engine());
        populateImportedTypes(imports, d->doc);
        d->context->setImports(d->doc.data(), imports);
    }

    foreach (Document::Ptr doc, d->snapshot) {
        if (doc == d->doc)
            continue;

        Imports *imports = new Imports(engine());
        populateImportedTypes(imports, doc);
        d->context->setImports(doc.data(), imports);
    }
}

void Link::populateImportedTypes(Imports *imports, Document::Ptr doc)
{
    Q_D(Link);

    if (! doc->qmlProgram())
        return;

    // implicit imports: the <default> package is always available
    loadImplicitDefaultImports(imports);

    // implicit imports:
    // qml files in the same directory are available without explicit imports
    loadImplicitDirectoryImports(imports, doc);

    // explicit imports, whether directories, files or libraries
    foreach (const ImportInfo &info, doc->bind()->imports()) {
        Import import = d->importCache.value(ImportCacheKey(info));

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
            default:
                break;
            }
            if (import.object)
                d->importCache.insert(ImportCacheKey(info), import);
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
Import Link::importFileOrDirectory(Document::Ptr doc, const ImportInfo &importInfo)
{
    Q_D(Link);

    Import import;
    import.info = importInfo;
    import.object = 0;

    const QString &path = importInfo.name();

    if (importInfo.type() == ImportInfo::DirectoryImport
            || importInfo.type() == ImportInfo::ImplicitDirectoryImport) {
        import.object = new ObjectValue(engine());

        importLibrary(doc, path, &import);

        const QList<Document::Ptr> &documentsInDirectory = d->snapshot.documentsInDirectory(path);
        foreach (Document::Ptr importedDoc, documentsInDirectory) {
            if (importedDoc->bind()->rootObjectValue()) {
                const QString targetName = importedDoc->componentName();
                import.object->setMember(targetName, importedDoc->bind()->rootObjectValue());
            }
        }
    } else if (importInfo.type() == ImportInfo::FileImport) {
        Document::Ptr importedDoc = d->snapshot.document(path);
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
Import Link::importNonFile(Document::Ptr doc, const ImportInfo &importInfo)
{
    Q_D(Link);

    Import import;
    import.info = importInfo;
    import.object = new ObjectValue(engine());

    const QString packageName = Bind::toString(importInfo.ast()->importUri, '.');
    const ComponentVersion version = importInfo.version();

    bool importFound = false;

    const QString &packagePath = importInfo.name();
    // check the filesystem with full version
    foreach (const QString &importPath, d->importPaths) {
        QString libraryPath = QString("%1/%2.%3").arg(importPath, packagePath, version.toString());
        if (importLibrary(doc, libraryPath, &import, importPath)) {
            importFound = true;
            break;
        }
    }
    if (!importFound) {
        // check the filesystem with major version
        foreach (const QString &importPath, d->importPaths) {
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
        foreach (const QString &importPath, d->importPaths) {
            QString libraryPath = QString("%1/%2").arg(importPath, packagePath);
            if (importLibrary(doc, libraryPath, &import, importPath)) {
                importFound = true;
                break;
            }
        }
    }

    // if there are cpp-based types for this package, use them too
    if (engine()->cppQmlTypes().hasPackage(packageName)) {
        importFound = true;
        foreach (QmlObjectValue *object,
                 engine()->cppQmlTypes().typesForImport(packageName, version)) {
            import.object->setMember(object->className(), object);
        }
    }

    if (!importFound && importInfo.ast()) {
        error(doc, locationFromRange(importInfo.ast()->firstSourceLocation(),
                                     importInfo.ast()->lastSourceLocation()),
              tr("package not found"));
    }

    return import;
}

bool Link::importLibrary(Document::Ptr doc,
                         const QString &libraryPath,
                         Import *import,
                         const QString &importPath)
{
    Q_D(Link);

    const ImportInfo &importInfo = import->info;

    const LibraryInfo libraryInfo = d->snapshot.libraryInfo(libraryPath);
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
                        tr("Library contains C++ plugins, type dump is in progress."));
            }
        } else if (libraryInfo.pluginTypeInfoStatus() == LibraryInfo::DumpError
                   || libraryInfo.pluginTypeInfoStatus() == LibraryInfo::TypeInfoFileError) {
            ModelManagerInterface *modelManager = ModelManagerInterface::instance();

            // Only underline import if package/version isn't described in .qmltypes anyway
            const QmlJS::ModelManagerInterface::BuiltinPackagesHash builtinPackages
                    = modelManager->builtinPackages();
            const QString packageName = importInfo.name().replace(QDir::separator(), QLatin1Char('.'));
            if (!builtinPackages.value(packageName).contains(importInfo.version())) {
                if (errorLoc.isValid()) {
                    error(doc, errorLoc, libraryInfo.pluginTypeInfoError());
                }
            }
        } else {
            QList<QmlObjectValue *> loadedObjects =
                    engine()->cppQmlTypes().load(engine(), libraryInfo.metaObjects());
            foreach (QmlObjectValue *object, loadedObjects) {
                if (object->packageName().isEmpty()) {
                    import->object->setMember(object->className(), object);
                }
            }
        }
    }

    loadQmldirComponents(import->object, version, libraryInfo, libraryPath);

    return true;
}

UiQualifiedId *Link::qualifiedTypeNameId(Node *node)
{
    if (UiObjectBinding *binding = AST::cast<UiObjectBinding *>(node))
        return binding->qualifiedTypeNameId;
    else if (UiObjectDefinition *binding = AST::cast<UiObjectDefinition *>(node))
        return binding->qualifiedTypeNameId;
    else
        return 0;
}

void Link::error(const Document::Ptr &doc, const AST::SourceLocation &loc, const QString &message)
{
    appendDiagnostic(doc, DiagnosticMessage(DiagnosticMessage::Error, loc, message));
}

void Link::warning(const Document::Ptr &doc, const AST::SourceLocation &loc, const QString &message)
{
    appendDiagnostic(doc, DiagnosticMessage(DiagnosticMessage::Warning, loc, message));
}

void Link::appendDiagnostic(const Document::Ptr &doc, const DiagnosticMessage &message)
{
    Q_D(Link);

    if (d->diagnosticMessages && doc->fileName() == d->doc->fileName())
        d->diagnosticMessages->append(message);
    if (d->allDiagnosticMessages)
        (*d->allDiagnosticMessages)[doc->fileName()].append(message);
}

void Link::loadQmldirComponents(Interpreter::ObjectValue *import, ComponentVersion version,
                                const LibraryInfo &libraryInfo, const QString &libraryPath)
{
    Q_D(Link);

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
        if (Document::Ptr importedDoc = d->snapshot.document(
                    libraryPath + QDir::separator() + component.fileName)) {
            if (ObjectValue *v = importedDoc->bind()->rootObjectValue())
                import->setMember(component.typeName, v);
        }
    }
}

void Link::loadImplicitDirectoryImports(Imports *imports, Document::Ptr doc)
{
    Q_D(Link);

    ImportInfo implcitDirectoryImportInfo(
                ImportInfo::ImplicitDirectoryImport, doc->path());

    Import directoryImport = d->importCache.value(ImportCacheKey(implcitDirectoryImportInfo));
    if (!directoryImport.object) {
        directoryImport = importFileOrDirectory(doc, implcitDirectoryImportInfo);
        if (directoryImport.object)
            d->importCache.insert(ImportCacheKey(implcitDirectoryImportInfo), directoryImport);
    }
    if (directoryImport.object) {
        imports->append(directoryImport);
    }
}

void Link::loadImplicitDefaultImports(Imports *imports)
{
    Q_D(Link);

    const QString defaultPackage = CppQmlTypes::defaultPackage;
    if (engine()->cppQmlTypes().hasPackage(defaultPackage)) {
        ImportInfo info(ImportInfo::LibraryImport, defaultPackage);
        Import import = d->importCache.value(ImportCacheKey(info));
        if (!import.object) {
            import.info = info;
            import.object = new ObjectValue(engine());
            foreach (QmlObjectValue *object,
                     engine()->cppQmlTypes().typesForImport(defaultPackage, ComponentVersion())) {
                import.object->setMember(object->className(), object);
            }
            d->importCache.insert(ImportCacheKey(info), import);
        }
        imports->append(import);
    }
}
