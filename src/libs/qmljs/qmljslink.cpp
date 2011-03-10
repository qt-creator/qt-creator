/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmljslink.h"

#include "parser/qmljsast_p.h"
#include "qmljsdocument.h"
#include "qmljsbind.h"
#include "qmljscheck.h"
#include "qmljsscopebuilder.h"
#include "qmljsmodelmanagerinterface.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>

#include <limits>

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
    Document::Ptr doc;
    Snapshot snapshot;
    Interpreter::Context *context;
    QStringList importPaths;

    QHash<ImportCacheKey, Interpreter::ObjectValue *> importCache;

    QList<DiagnosticMessage> diagnosticMessages;
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

Link::Link(Context *context, const Document::Ptr &doc, const Snapshot &snapshot,
           const QStringList &importPaths)
    : d_ptr(new LinkPrivate)
{
    Q_D(Link);
    d->context = context;
    d->doc = doc;
    d->snapshot = snapshot;
    d->importPaths = importPaths;

    // populate engine with types from C++
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();
    if (modelManager) {
        foreach (const QList<FakeMetaObject::ConstPtr> &cppTypes, modelManager->cppQmlTypes()) {
            engine()->cppQmlTypes().load(engine(), cppTypes);
        }
    }

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

QList<DiagnosticMessage> Link::diagnosticMessages() const
{
    Q_D(const Link);
    return d->diagnosticMessages;
}

void Link::linkImports()
{
    Q_D(Link);

    // do it on d->doc first, to make sure import errors are shown
    TypeEnvironment *typeEnv = new TypeEnvironment(engine());
    populateImportedTypes(typeEnv, d->doc);
    d->context->setTypeEnvironment(d->doc.data(), typeEnv);

    foreach (Document::Ptr doc, d->snapshot) {
        if (doc == d->doc)
            continue;

        TypeEnvironment *typeEnv = new TypeEnvironment(engine());
        populateImportedTypes(typeEnv, doc);
        d->context->setTypeEnvironment(doc.data(), typeEnv);
    }
}

void Link::populateImportedTypes(TypeEnvironment *typeEnv, Document::Ptr doc)
{
    Q_D(Link);

    if (! doc->qmlProgram())
        return;

    // implicit imports: the <default> package is always available
    loadImplicitDefaultImports(typeEnv);

    // implicit imports:
    // qml files in the same directory are available without explicit imports
    loadImplicitDirectoryImports(typeEnv, doc);

    // explicit imports, whether directories, files or libraries
    foreach (const ImportInfo &info, doc->bind()->imports()) {
        ObjectValue *import = d->importCache.value(ImportCacheKey(info));
        if (!import) {
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
            if (import)
                d->importCache.insert(ImportCacheKey(info), import);
        }
        if (import)
            typeEnv->addImport(import, info);
    }
}

/*
    import "content"
    import "content" as Xxx
    import "content" 4.6
    import "content" 4.6 as Xxx

    import "http://www.ovi.com/" as Ovi
*/
ObjectValue *Link::importFileOrDirectory(Document::Ptr doc, const ImportInfo &importInfo)
{
    Q_D(Link);

    ObjectValue *import = 0;
    const QString &path = importInfo.name();

    if (importInfo.type() == ImportInfo::DirectoryImport
            || importInfo.type() == ImportInfo::ImplicitDirectoryImport) {
        import = new ObjectValue(engine());

        importLibrary(doc, import, path, importInfo);

        const QList<Document::Ptr> &documentsInDirectory = d->snapshot.documentsInDirectory(path);
        foreach (Document::Ptr importedDoc, documentsInDirectory) {
            if (importedDoc->bind()->rootObjectValue()) {
                const QString targetName = importedDoc->componentName();
                import->setProperty(targetName, importedDoc->bind()->rootObjectValue());
            }
        }
    } else if (importInfo.type() == ImportInfo::FileImport) {
        Document::Ptr importedDoc = d->snapshot.document(path);
        if (importedDoc)
            import = importedDoc->bind()->rootObjectValue();
    }

    return import;
}

/*
  import Qt 4.6
  import Qt 4.6 as Xxx
  (import com.nokia.qt is the same as the ones above)
*/
ObjectValue *Link::importNonFile(Document::Ptr doc, const ImportInfo &importInfo)
{
    Q_D(Link);

    ObjectValue *import = new ObjectValue(engine());
    const QString packageName = Bind::toString(importInfo.ast()->importUri, '.');
    const ComponentVersion version = importInfo.version();

    bool importFound = false;

    // check the filesystem
    const QString &packagePath = importInfo.name();
    foreach (const QString &importPath, d->importPaths) {
        QString libraryPath = importPath;
        libraryPath += QDir::separator();
        libraryPath += packagePath;

        if (importLibrary(doc, import, libraryPath, importInfo, importPath)) {
            importFound = true;
            break;
        }
    }

    // if there are cpp-based types for this package, use them too
    if (engine()->cppQmlTypes().hasPackage(packageName)) {
        importFound = true;
        foreach (QmlObjectValue *object,
                 engine()->cppQmlTypes().typesForImport(packageName, version)) {
            import->setProperty(object->className(), object);
        }
    }

    if (!importFound) {
        error(doc, locationFromRange(importInfo.ast()->firstSourceLocation(),
                                     importInfo.ast()->lastSourceLocation()),
              tr("package not found"));
    }

    return import;
}

bool Link::importLibrary(Document::Ptr doc, Interpreter::ObjectValue *import,
                         const QString &libraryPath,
                         const Interpreter::ImportInfo &importInfo,
                         const QString &importPath)
{
    Q_D(Link);

    const LibraryInfo libraryInfo = d->snapshot.libraryInfo(libraryPath);
    if (!libraryInfo.isValid())
        return false;

    const ComponentVersion version = importInfo.version();
    const UiImport *ast = importInfo.ast();
    SourceLocation errorLoc;
    if (ast)
        errorLoc = locationFromRange(ast->firstSourceLocation(), ast->lastSourceLocation());

    if (!libraryInfo.plugins().isEmpty()) {
        if (libraryInfo.dumpStatus() == LibraryInfo::DumpNotStartedOrRunning) {
            ModelManagerInterface *modelManager = ModelManagerInterface::instance();
            if (modelManager) {
                if (importInfo.type() == ImportInfo::LibraryImport) {
                    modelManager->loadPluginTypes(
                                libraryPath, importPath,
                                importInfo.name(), version.toString());
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
        } else if (libraryInfo.dumpStatus() == LibraryInfo::DumpError) {
            if (errorLoc.isValid()) {
                error(doc, errorLoc, libraryInfo.dumpError());
            }
        } else {
            QList<QmlObjectValue *> loadedObjects =
                    engine()->cppQmlTypes().load(engine(), libraryInfo.metaObjects());
            foreach (QmlObjectValue *object, loadedObjects) {
                if (object->packageName().isEmpty()) {
                    import->setProperty(object->className(), object);
                }
            }
        }
    }

    loadQmldirComponents(import, version, libraryInfo, libraryPath);

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
    Q_D(Link);

    if (doc->fileName() == d->doc->fileName())
        d->diagnosticMessages.append(DiagnosticMessage(DiagnosticMessage::Error, loc, message));
}

void Link::warning(const Document::Ptr &doc, const AST::SourceLocation &loc, const QString &message)
{
    Q_D(Link);

    if (doc->fileName() == d->doc->fileName())
        d->diagnosticMessages.append(DiagnosticMessage(DiagnosticMessage::Warning, loc, message));
}

void Link::loadQmldirComponents(Interpreter::ObjectValue *import, ComponentVersion version,
                                const LibraryInfo &libraryInfo, const QString &libraryPath)
{
    Q_D(Link);

    // if the version isn't valid, import the latest
    if (!version.isValid()) {
        const int maxVersion = std::numeric_limits<int>::max();
        version = ComponentVersion(maxVersion, maxVersion);
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
                import->setProperty(component.typeName, v);
        }
    }
}

void Link::loadImplicitDirectoryImports(TypeEnvironment *typeEnv, Document::Ptr doc)
{
    Q_D(Link);

    ImportInfo implcitDirectoryImportInfo(
                ImportInfo::ImplicitDirectoryImport, doc->path());

    ObjectValue *directoryImport = d->importCache.value(ImportCacheKey(implcitDirectoryImportInfo));
    if (!directoryImport) {
        directoryImport = importFileOrDirectory(doc, implcitDirectoryImportInfo);
        if (directoryImport)
            d->importCache.insert(ImportCacheKey(implcitDirectoryImportInfo), directoryImport);
    }
    if (directoryImport)
        typeEnv->addImport(directoryImport, implcitDirectoryImportInfo);
}

void Link::loadImplicitDefaultImports(TypeEnvironment *typeEnv)
{
    Q_D(Link);

    const QString defaultPackage = CppQmlTypes::defaultPackage;
    if (engine()->cppQmlTypes().hasPackage(defaultPackage)) {
        ImportInfo info(ImportInfo::LibraryImport, defaultPackage);
        ObjectValue *import = d->importCache.value(ImportCacheKey(info));
        if (!import) {
            import = new ObjectValue(engine());
            foreach (QmlObjectValue *object,
                     engine()->cppQmlTypes().typesForImport(defaultPackage, ComponentVersion())) {
                import->setProperty(object->className(), object);
            }
            d->importCache.insert(ImportCacheKey(info), import);
        }
        typeEnv->addImport(import, info);
    }
}
