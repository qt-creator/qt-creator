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
    , _context(interp)
{
    _docs = reachableDocuments(currentDoc, snapshot);

    linkImports();
}

Link::~Link()
{
}

Context *Link::context()
{
    return &_context;
}

Interpreter::Engine *Link::engine()
{
    return _context.engine();
}

void Link::scopeChainAt(Document::Ptr doc, Node *currentObject)
{
    _context.pushScope(engine()->globalObject());

    if (! doc)
        return;

    if (doc->qmlProgram() != 0)
        _context.setLookupMode(Context::QmlLookup);

    BindPtr bind = doc->bind();

    // Build the scope chain.

    // ### FIXME: May want to link to instantiating components from here.

    if (bind->_rootObjectValue)
        _context.pushScope(bind->_rootObjectValue);

    ObjectValue *scopeObject = 0;
    if (UiObjectDefinition *definition = cast<UiObjectDefinition *>(currentObject))
        scopeObject = bind->_qmlObjects.value(definition);
    else if (UiObjectBinding *binding = cast<UiObjectBinding *>(currentObject))
        scopeObject = bind->_qmlObjects.value(binding);

    if (scopeObject && scopeObject != bind->_rootObjectValue)
        _context.pushScope(scopeObject);

    const QStringList &includedScripts = bind->includedScripts();
    for (int index = includedScripts.size() - 1; index != -1; --index) {
        const QString &scriptFile = includedScripts.at(index);

        if (Document::Ptr scriptDoc = _snapshot.document(scriptFile)) {
            if (scriptDoc->jsProgram()) {
                _context.pushScope(scriptDoc->bind()->_rootObjectValue);
            }
        }
    }

    if (bind->_functionEnvironment)
        _context.pushScope(bind->_functionEnvironment);

    if (bind->_idEnvironment)
        _context.pushScope(bind->_idEnvironment);

    if (const ObjectValue *typeEnvironment = _typeEnvironments.value(doc.data())) {
        _context.pushScope(typeEnvironment);
        _context.setTypeEnvironment(typeEnvironment);
    }
}

void Link::linkImports()
{
    foreach (Document::Ptr doc, _docs) {
        ObjectValue *typeEnv = engine()->newObject(/*prototype =*/0); // ### FIXME
        _typeEnvironments.insert(doc.data(), typeEnv);

        // Populate the _typeEnvironment with imports.
        populateImportedTypes(typeEnv, doc);
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
