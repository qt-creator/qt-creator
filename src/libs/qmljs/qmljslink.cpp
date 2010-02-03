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

Link::Link(Document::Ptr currentDoc, const Snapshot &snapshot, Interpreter::Engine *interp)
    : _snapshot(snapshot)
    , _interp(interp)
    , _context(interp)
{
    _docs = reachableDocuments(currentDoc, snapshot);

    linkImports();
}

Link::~Link()
{
    // unset all prototypes and scopes
    foreach (Document::Ptr doc, _docs) {
        BindPtr bind = doc->bind();

        if (doc->qmlProgram()) {
            bind->_idEnvironment->setScope(0);
            bind->_functionEnvironment->setScope(0);

            foreach (ObjectValue *object, bind->_qmlObjects) {
                object->setPrototype(0);
                object->setScope(0);
            }
        } else if (doc->jsProgram()) {
            bind->_rootObjectValue->setScope(0);
        }
    }
}

static ObjectValue *pushScope(ObjectValue *next, ObjectValue *onto)
{
    onto->setScope(next);
    return next;
}

Context *Link::context()
{
    return &_context;
}

ObjectValue *Link::scopeChainAt(Document::Ptr doc, Node *currentObject)
{
    BindPtr bind = doc->bind();

    ObjectValue *scopeObject = 0;
    if (UiObjectDefinition *definition = cast<UiObjectDefinition *>(currentObject))
        scopeObject = bind->_qmlObjects.value(definition);

    if (!scopeObject)
        return bind->_interp.globalObject();

    // Build the scope chain.
    ObjectValue *scopeStart = _typeEnvironments.value(doc.data());
    ObjectValue *scope = scopeStart;
    scope = pushScope(bind->_idEnvironment, scope);
    scope = pushScope(bind->_functionEnvironment, scope);

    foreach (const QString &scriptFile, doc->bind()->includedScripts()) {
        if (Document::Ptr scriptDoc = _snapshot.document(scriptFile)) {
            if (scriptDoc->jsProgram()) {
                scope = pushScope(scriptDoc->bind()->_rootObjectValue, scope);
            }
        }
    }

    scope = pushScope(scopeObject, scope);
    if (scopeObject != bind->_rootObjectValue)
        scope = pushScope(bind->_rootObjectValue, scope);

    scope = pushScope(bind->_interp.globalObject(), scope);

    // May want to link to instantiating components from here.

    return scopeStart;
}

void Link::linkImports()
{
    foreach (Document::Ptr doc, _docs) {
        BindPtr bind = doc->bind();

        ObjectValue *typeEnv = _interp->newObject(/*prototype =*/0);
        _typeEnvironments.insert(doc.data(), typeEnv);

        // Populate the _typeEnvironment with imports.
        populateImportedTypes(typeEnv, doc);

        // Set the prototypes.
        QHashIterator<Node *, ObjectValue *> it(bind->_qmlObjects);
        while (it.hasNext()) {
            it.next();
            Node *binding = it.key();
            ObjectValue *value = it.value();

            if (UiQualifiedId *qualifiedId = qualifiedTypeNameId(binding))
                value->setPrototype(lookupType(typeEnv, qualifiedId));
        }
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
                             otherDoc->bind()->_rootObjectValue);
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
            importNamespace = _interp->newObject(/*prototype =*/0);
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

        importInto->setProperty(targetName, otherDoc->bind()->_rootObjectValue);
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
        namespaceObject = _interp->newObject(/*prototype */ 0);
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
        foreach (QmlObjectValue *object, _interp->metaTypeSystem().staticTypesForImport(package, majorVersion, minorVersion)) {
            namespaceObject->setProperty(object->qmlTypeName(), object);
        }
#endif // NO_DECLARATIVE_BACKEND
    }
}

const ObjectValue *Link::lookupType(ObjectValue *env, UiQualifiedId *id)
{
    const ObjectValue *objectValue = env;

    for (UiQualifiedId *iter = id; objectValue && iter; iter = iter->next) {
        if (! iter->name)
            return 0;

        const Value *value = objectValue->property(iter->name->asString());
        if (!value)
            return 0;

        objectValue = value->asObjectValue();
    }

    return objectValue;
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

QList<Document::Ptr> Link::reachableDocuments(Document::Ptr startDoc, const Snapshot &snapshot)
{
    QList<Document::Ptr> docs;

    QSet<QString> processed;
    QStringList todo;

    QMultiHash<QString, Document::Ptr> documentByPath;
    foreach (Document::Ptr doc, snapshot)
        documentByPath.insert(doc->path(), doc);

    todo.append(startDoc->path());

    // Find the reachable documents.
    while (! todo.isEmpty()) {
        const QString path = todo.takeFirst();

        if (processed.contains(path))
            continue;

        processed.insert(path);

        QStringList localImports;
        foreach (Document::Ptr doc, documentByPath.values(path)) {
            docs += doc;
            localImports += doc->bind()->localImports();
        }

        localImports.removeDuplicates();
        todo += localImports;
    }

    return docs;
}
