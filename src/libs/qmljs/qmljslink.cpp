/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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

    linkImports();
    initializeScopeChain();
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

void Link::initializeScopeChain()
{
    Q_D(Link);

    ScopeChain &scopeChain = d->context->scopeChain();

    // ### TODO: This object ought to contain the global namespace additions by QML.
    scopeChain.globalScope = engine()->globalObject();

    if (! d->doc) {
        scopeChain.update();
        return;
    }

    Bind *bind = d->doc->bind();
    QHash<Document *, ScopeChain::QmlComponentChain *> componentScopes;

    ScopeChain::QmlComponentChain *chain = new ScopeChain::QmlComponentChain;
    scopeChain.qmlComponentScope = QSharedPointer<const ScopeChain::QmlComponentChain>(chain);
    if (d->doc->qmlProgram()) {
        componentScopes.insert(d->doc.data(), chain);
        makeComponentChain(d->doc, chain, &componentScopes);

        if (const TypeEnvironment *typeEnvironment = d->context->typeEnvironment(d->doc.data()))
            scopeChain.qmlTypes = typeEnvironment;
    } else {
        // add scope chains for all components that import this file
        foreach (Document::Ptr otherDoc, d->snapshot) {
            foreach (const ImportInfo &import, otherDoc->bind()->imports()) {
                if (import.type() == ImportInfo::FileImport && d->doc->fileName() == import.name()) {
                    ScopeChain::QmlComponentChain *component = new ScopeChain::QmlComponentChain;
                    componentScopes.insert(otherDoc.data(), component);
                    chain->instantiatingComponents += component;
                    makeComponentChain(otherDoc, component, &componentScopes);
                }
            }
        }

        // ### TODO: Which type environment do scripts see?

        if (bind->rootObjectValue())
            scopeChain.jsScopes += bind->rootObjectValue();
    }

    scopeChain.update();
}

void Link::makeComponentChain(
        Document::Ptr doc,
        ScopeChain::QmlComponentChain *target,
        QHash<Document *, ScopeChain::QmlComponentChain *> *components)
{
    Q_D(Link);

    if (!doc->qmlProgram())
        return;

    Bind *bind = doc->bind();

    // add scopes for all components instantiating this one
    foreach (Document::Ptr otherDoc, d->snapshot) {
        if (otherDoc == doc)
            continue;
        if (otherDoc->bind()->usesQmlPrototype(bind->rootObjectValue(), d->context)) {
            if (components->contains(otherDoc.data())) {
//                target->instantiatingComponents += components->value(otherDoc.data());
            } else {
                ScopeChain::QmlComponentChain *component = new ScopeChain::QmlComponentChain;
                components->insert(otherDoc.data(), component);
                target->instantiatingComponents += component;

                makeComponentChain(otherDoc, component, components);
            }
        }
    }

    // build this component scope
    target->document = doc;
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

    if (! (doc->qmlProgram() && doc->qmlProgram()->imports))
        return;

    // implicit imports:
    // qml files in the same directory are available without explicit imports
    ImportInfo implcitImportInfo(ImportInfo::ImplicitDirectoryImport, doc->path());
    ObjectValue *directoryImport = d->importCache.value(ImportCacheKey(implcitImportInfo));
    if (!directoryImport) {
        directoryImport = importFile(doc, implcitImportInfo);
        if (directoryImport)
            d->importCache.insert(ImportCacheKey(implcitImportInfo), directoryImport);
    }
    if (directoryImport)
        typeEnv->addImport(directoryImport, implcitImportInfo);

    // explicit imports, whether directories, files or libraries
    foreach (const ImportInfo &info, doc->bind()->imports()) {
        ObjectValue *import = d->importCache.value(ImportCacheKey(info));
        if (!import) {
            switch (info.type()) {
            case ImportInfo::FileImport:
            case ImportInfo::DirectoryImport:
                import = importFile(doc, info);
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
ObjectValue *Link::importFile(Document::Ptr, const ImportInfo &importInfo)
{
    Q_D(Link);

    ObjectValue *import = 0;
    const QString &path = importInfo.name();

    if (importInfo.type() == ImportInfo::DirectoryImport
            || importInfo.type() == ImportInfo::ImplicitDirectoryImport) {
        import = new ObjectValue(engine());
        const QList<Document::Ptr> &documentsInDirectory = d->snapshot.documentsInDirectory(path);
        foreach (Document::Ptr importedDoc, documentsInDirectory) {
            if (importedDoc->bind()->rootObjectValue()) {
                const QString targetName = importedDoc->componentName();
                import->setProperty(targetName, importedDoc->bind()->rootObjectValue());
            }
        }
    } else if (importInfo.type() == ImportInfo::FileImport) {
        Document::Ptr importedDoc = d->snapshot.document(path);
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

        const LibraryInfo libraryInfo = d->snapshot.libraryInfo(libraryPath);
        if (!libraryInfo.isValid())
            continue;

        importFound = true;

        if (!libraryInfo.plugins().isEmpty()) {
            if (libraryInfo.metaObjects().isEmpty()) {
                ModelManagerInterface *modelManager = ModelManagerInterface::instance();
                if (modelManager)
                    modelManager->loadPluginTypes(libraryPath, importPath, packageName);
            } else {
                engine()->cppQmlTypes().load(engine(), libraryInfo.metaObjects());
            }
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

        break;
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
