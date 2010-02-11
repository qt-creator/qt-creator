#include "qmljslink.h"

#include "parser/qmljsast_p.h"
#include "qmljsdocument.h"
#include "qmljsbind.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>

using namespace QmlJS;
using namespace QmlJS::Interpreter;
using namespace QmlJS::AST;

Link::Link(Context *context, Document::Ptr currentDoc, const Snapshot &snapshot)
    : _snapshot(snapshot)
    , _context(context)
{
    _docs = reachableDocuments(currentDoc, snapshot);
    linkImports();
}

Link::~Link()
{
}

Interpreter::Engine *Link::engine()
{
    return _context->engine();
}

void Link::scopeChainAt(Document::Ptr doc, Node *currentObject)
{
    // ### TODO: This object ought to contain the global namespace additions by QML.
    _context->pushScope(engine()->globalObject());

    if (! doc)
        return;

    Bind *bind = doc->bind();
    QStringList linkedDocs; // to prevent cycles

    if (doc->qmlProgram()) {
        _context->setLookupMode(Context::QmlLookup);

        ObjectValue *scopeObject = 0;
        if (UiObjectDefinition *definition = cast<UiObjectDefinition *>(currentObject))
            scopeObject = bind->findQmlObject(definition);
        else if (UiObjectBinding *binding = cast<UiObjectBinding *>(currentObject))
            scopeObject = bind->findQmlObject(binding);

        pushScopeChainForComponent(doc, &linkedDocs, scopeObject);

        if (const ObjectValue *typeEnvironment = _context->typeEnvironment(doc.data()))
            _context->pushScope(typeEnvironment);
    } else {
        // add scope chains for all components that source this document
        foreach (Document::Ptr otherDoc, _docs) {
            if (otherDoc->bind()->includedScripts().contains(doc->fileName()))
                pushScopeChainForComponent(otherDoc, &linkedDocs);
        }

        // ### TODO: Which type environment do scripts see?

        _context->pushScope(bind->rootObjectValue());
    }

    if (FunctionDeclaration *fun = cast<FunctionDeclaration *>(currentObject)) {
        ObjectValue *activation = engine()->newObject(/*prototype = */ 0);
        for (FormalParameterList *it = fun->formals; it; it = it->next) {
            if (it->name)
                activation->setProperty(it->name->asString(), engine()->undefinedValue());
        }
        _context->pushScope(activation);
    }
}

void Link::pushScopeChainForComponent(Document::Ptr doc, QStringList *linkedDocs,
                                      ObjectValue *scopeObject)
{
    if (!doc->qmlProgram())
        return;

    linkedDocs->append(doc->fileName());

    Bind *bind = doc->bind();

    // add scopes for all components instantiating this one
    foreach (Document::Ptr otherDoc, _docs) {
        if (linkedDocs->contains(otherDoc->fileName()))
            continue;

        if (otherDoc->bind()->usesQmlPrototype(bind->rootObjectValue(), _context)) {
            // ### TODO: Depth-first insertion doesn't give us correct name shadowing.
            pushScopeChainForComponent(otherDoc, linkedDocs);
        }
    }

    if (bind->rootObjectValue())
        _context->pushScope(bind->rootObjectValue());

    if (scopeObject && scopeObject != bind->rootObjectValue())
        _context->pushScope(scopeObject);

    const QStringList &includedScripts = bind->includedScripts();
    for (int index = includedScripts.size() - 1; index != -1; --index) {
        const QString &scriptFile = includedScripts.at(index);

        if (Document::Ptr scriptDoc = _snapshot.document(scriptFile)) {
            if (scriptDoc->jsProgram()) {
                _context->pushScope(scriptDoc->bind()->rootObjectValue());
            }
        }
    }

    _context->pushScope(bind->functionEnvironment());
    _context->pushScope(bind->idEnvironment());
}

void Link::linkImports()
{
    foreach (Document::Ptr doc, _docs) {
        ObjectValue *typeEnv = engine()->newObject(/*prototype =*/0); // ### FIXME

        // Populate the _typeEnvironment with imports.
        populateImportedTypes(typeEnv, doc);

        _context->setTypeEnvironment(doc.data(), typeEnv);
    }
}

static QString componentName(const QString &fileName)
{
    QString componentName = fileName;
    int dotIndex = componentName.indexOf(QLatin1Char('.'));
    if (dotIndex != -1)
        componentName.truncate(dotIndex);
    componentName[0] = componentName[0].toUpper();
    return componentName;
}

void Link::populateImportedTypes(Interpreter::ObjectValue *typeEnv, Document::Ptr doc)
{
    if (! (doc->qmlProgram() && doc->qmlProgram()->imports))
        return;

    QFileInfo fileInfo(doc->fileName());
    const QString absolutePath = fileInfo.absolutePath();

    // implicit imports:
    // qml files in the same directory are available without explicit imports
    foreach (Document::Ptr otherDoc, _docs) {
        if (otherDoc == doc)
            continue;

        QFileInfo otherFileInfo(otherDoc->fileName());
        const QString otherAbsolutePath = otherFileInfo.absolutePath();

        if (otherAbsolutePath != absolutePath)
            continue;

        typeEnv->setProperty(componentName(otherFileInfo.fileName()),
                             otherDoc->bind()->rootObjectValue());
    }

    // explicit imports, whether directories or files
    for (UiImportList *it = doc->qmlProgram()->imports; it; it = it->next) {
        if (! it->import)
            continue;

        if (it->import->fileName) {
            importFile(typeEnv, doc, it->import, absolutePath);
        } else if (it->import->importUri) {
            importNonFile(typeEnv, doc, it->import);
        }
    }
}

/*
    import "content"
    import "content" as Xxx
    import "content" 4.6
    import "content" 4.6 as Xxx

    import "http://www.ovi.com/" as Ovi
*/
void Link::importFile(Interpreter::ObjectValue *typeEnv, Document::Ptr doc,
                      AST::UiImport *import, const QString &startPath)
{
    Q_UNUSED(doc)

    if (!import->fileName)
        return;

    QString path = startPath;
    path += QLatin1Char('/');
    path += import->fileName->asString();
    path = QDir::cleanPath(path);

    ObjectValue *importNamespace = 0;

    foreach (Document::Ptr otherDoc, _docs) {
        QFileInfo otherFileInfo(otherDoc->fileName());
        const QString otherAbsolutePath = otherFileInfo.absolutePath();

        bool directoryImport = (path == otherAbsolutePath);
        bool fileImport = (path == otherDoc->fileName());
        if (!directoryImport && !fileImport)
            continue;

        if (directoryImport && import->importId && !importNamespace) {
            importNamespace = engine()->newObject(/*prototype =*/0);
            typeEnv->setProperty(import->importId->asString(), importNamespace);
        }

        QString targetName;
        if (fileImport && import->importId) {
            targetName = import->importId->asString();
        } else {
            targetName = componentName(otherFileInfo.fileName());
        }

        ObjectValue *importInto = typeEnv;
        if (importNamespace)
            importInto = importNamespace;

        importInto->setProperty(targetName, otherDoc->bind()->rootObjectValue());
    }
}

/*
  import Qt 4.6
  import Qt 4.6 as Xxx
  (import com.nokia.qt is the same as the ones above)
*/
void Link::importNonFile(Interpreter::ObjectValue *typeEnv, Document::Ptr doc, AST::UiImport *import)
{
    ObjectValue *namespaceObject = 0;

    if (import->importId) { // with namespace we insert an object in the type env. to hold the imported types
        namespaceObject = engine()->newObject(/*prototype */ 0);
        typeEnv->setProperty(import->importId->asString(), namespaceObject);

    } else { // without namespace we insert all types directly into the type env.
        namespaceObject = typeEnv;
    }

    // try the metaobject system
    if (import->importUri) {
        const QString package = Bind::toString(import->importUri, '/');
        int majorVersion = -1; // ### TODO: Check these magic version numbers
        int minorVersion = -1; // ### TODO: Check these magic version numbers

        if (import->versionToken.isValid()) {
            const QString versionString = doc->source().mid(import->versionToken.offset, import->versionToken.length);
            const int dotIdx = versionString.indexOf(QLatin1Char('.'));
            if (dotIdx == -1) {
                // only major (which is probably invalid, but let's handle it anyway)
                majorVersion = versionString.toInt();
                minorVersion = 0; // ### TODO: Check with magic version numbers above
            } else {
                majorVersion = versionString.left(dotIdx).toInt();
                minorVersion = versionString.mid(dotIdx + 1).toInt();
            }
        }
#ifndef NO_DECLARATIVE_BACKEND
        foreach (QmlObjectValue *object, engine()->metaTypeSystem().staticTypesForImport(package, majorVersion, minorVersion)) {
            namespaceObject->setProperty(object->qmlTypeName(), object);
        }
#endif // NO_DECLARATIVE_BACKEND
    }
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

static uint qHash(Document::Ptr doc) {
    return qHash(doc.data());
}

QList<Document::Ptr> Link::reachableDocuments(Document::Ptr startDoc, const Snapshot &snapshot)
{
    QSet<Document::Ptr> docs;

    if (! startDoc)
        return docs.values();

    QMultiHash<QString, Document::Ptr> documentByPath;
    foreach (Document::Ptr doc, snapshot)
        documentByPath.insert(doc->path(), doc);

    // ### TODO: This doesn't scale well. Maybe just use the whole snapshot?
    // Find all documents that (indirectly) include startDoc
    {
        QList<Document::Ptr> todo;
        todo += startDoc;

        while (! todo.isEmpty()) {
            Document::Ptr doc = todo.takeFirst();

            docs += doc;

            Snapshot::const_iterator it, end = snapshot.end();
            for (it = snapshot.begin(); it != end; ++it) {
                Document::Ptr otherDoc = *it;
                if (docs.contains(otherDoc))
                    continue;

                QStringList localImports = otherDoc->bind()->localImports();
                if (localImports.contains(doc->fileName())
                    || localImports.contains(doc->path())
                    || otherDoc->bind()->includedScripts().contains(doc->fileName())
                ) {
                    todo += otherDoc;
                }
            }
        }
    }

    // Find all documents that are included by these (even if indirectly).
    {
        QSet<QString> processed;
        QStringList todo;
        foreach (Document::Ptr doc, docs)
            todo.append(doc->fileName());

        while (! todo.isEmpty()) {
            QString path = todo.takeFirst();

            if (processed.contains(path))
                continue;
            processed.insert(path);

            if (Document::Ptr doc = snapshot.document(path)) {
                docs += doc;

                if (doc->qmlProgram())
                    path = doc->path();
                else
                    continue;
            }

            QStringList localImports;
            foreach (Document::Ptr doc, documentByPath.values(path)) {
                if (doc->qmlProgram()) {
                    docs += doc;
                    localImports += doc->bind()->localImports();
                    localImports += doc->bind()->includedScripts();
                }
            }

            localImports.removeDuplicates();
            todo += localImports;
        }
    }

    return docs.values();
}
