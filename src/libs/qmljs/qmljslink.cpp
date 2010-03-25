#include "qmljslink.h"

#include "parser/qmljsast_p.h"
#include "qmljsdocument.h"
#include "qmljsbind.h"
#include "qmljsscopebuilder.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

using namespace QmlJS;
using namespace QmlJS::Interpreter;
using namespace QmlJS::AST;

Link::Link(Context *context, const Snapshot &snapshot,
           const QStringList &importPaths)
    : _snapshot(snapshot)
    , _context(context)
    , _importPaths(importPaths)
{
    foreach (Document::Ptr doc, snapshot)
        _documentByPath.insert(doc->path(), doc);
    linkImports();
}

Link::~Link()
{
}

Interpreter::Engine *Link::engine()
{
    return _context->engine();
}

QList<DiagnosticMessage> Link::diagnosticMessages() const
{
    return _diagnosticMessages;
}

void Link::scopeChainAt(Document::Ptr doc, const QList<Node *> &astPath)
{
    ScopeChain &scopeChain = _context->scopeChain();

    // ### TODO: This object ought to contain the global namespace additions by QML.
    scopeChain.globalScope = engine()->globalObject();

    if (! doc) {
        scopeChain.update();
        return;
    }

    Bind *bind = doc->bind();
    QHash<Document *, ScopeChain::QmlComponentChain *> componentScopes;

    if (doc->qmlProgram()) {
        _context->setLookupMode(Context::QmlLookup);

        scopeChain.qmlComponentScope.clear();
        componentScopes.insert(doc.data(), &scopeChain.qmlComponentScope);
        makeComponentChain(doc, &scopeChain.qmlComponentScope, &componentScopes);

        if (const ObjectValue *typeEnvironment = _context->typeEnvironment(doc.data()))
            scopeChain.qmlTypes = typeEnvironment;
    } else {
        // the global scope of a js file does not see the instantiating component
        if (astPath.size() > 0) {
            // add scope chains for all components that source this document
            foreach (Document::Ptr otherDoc, _snapshot) {
                if (otherDoc->bind()->includedScripts().contains(doc->fileName())) {
                    ScopeChain::QmlComponentChain *component = new ScopeChain::QmlComponentChain;
                    componentScopes.insert(otherDoc.data(), component);
                    scopeChain.qmlComponentScope.instantiatingComponents += component;
                    makeComponentChain(otherDoc, component, &componentScopes);
                }
            }

            // ### TODO: Which type environment do scripts see?
        }

        scopeChain.jsScopes += bind->rootObjectValue();
    }

    if (astPath.isEmpty()) {
        scopeChain.update();
    } else {
        ScopeBuilder scopeBuilder(doc, _context);
        foreach (Node *node, astPath)
            scopeBuilder.push(node);
    }
}

void Link::makeComponentChain(
        Document::Ptr doc,
        ScopeChain::QmlComponentChain *target,
        QHash<Document *, ScopeChain::QmlComponentChain *> *components)
{
    if (!doc->qmlProgram())
        return;

    Bind *bind = doc->bind();

    // add scopes for all components instantiating this one
    foreach (Document::Ptr otherDoc, _snapshot) {
        if (otherDoc == doc)
            continue;
        if (otherDoc->bind()->usesQmlPrototype(bind->rootObjectValue(), _context)) {
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
    if (bind->rootObjectValue())
        target->rootObject = bind->rootObjectValue();

    const QStringList &includedScripts = bind->includedScripts();
    for (int index = includedScripts.size() - 1; index != -1; --index) {
        const QString &scriptFile = includedScripts.at(index);

        if (Document::Ptr scriptDoc = _snapshot.document(scriptFile)) {
            if (scriptDoc->jsProgram())
                target->functionScopes += scriptDoc->bind()->rootObjectValue();
        }
    }

    target->functionScopes += bind->functionEnvironment();
    target->ids = bind->idEnvironment();
}

void Link::linkImports()
{
    foreach (Document::Ptr doc, _snapshot) {
        ObjectValue *typeEnv = engine()->newObject(/*prototype =*/0); // ### FIXME

        // Populate the _typeEnvironment with imports.
        populateImportedTypes(typeEnv, doc);

        _context->setTypeEnvironment(doc.data(), typeEnv);
    }
}

static QString componentName(const QString &fileName)
{
    QString componentName = fileName;
    int sepIndex = componentName.lastIndexOf(QDir::separator());
    if (sepIndex != -1)
        componentName.remove(0, sepIndex + 1);
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

    // Add the implicitly available Script type
    const ObjectValue *scriptValue = engine()->metaTypeSystem().staticTypeForImport("Script");
    if (scriptValue)
        typeEnv->setProperty("Script", scriptValue);

    // implicit imports:
    // qml files in the same directory are available without explicit imports
    foreach (Document::Ptr otherDoc, _documentByPath.values(doc->path())) {
        if (otherDoc == doc)
            continue;

        typeEnv->setProperty(componentName(otherDoc->fileName()),
                             otherDoc->bind()->rootObjectValue());
    }

    // explicit imports, whether directories or files
    for (UiImportList *it = doc->qmlProgram()->imports; it; it = it->next) {
        if (! it->import)
            continue;

        if (it->import->fileName) {
            importFile(typeEnv, doc, it->import);
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
                      AST::UiImport *import)
{
    Q_UNUSED(doc)

    if (!import->fileName)
        return;

    QString path = doc->path();
    path += QLatin1Char('/');
    path += import->fileName->asString();
    path = QDir::cleanPath(path);

    ObjectValue *importNamespace = typeEnv;

    // directory import
    if (_documentByPath.contains(path)) {
        if (import->importId) {
            importNamespace = engine()->newObject(/*prototype =*/0);
            typeEnv->setProperty(import->importId->asString(), importNamespace);
        }

        foreach (Document::Ptr importedDoc, _documentByPath.values(path)) {
            const QString targetName = componentName(importedDoc->fileName());
            importNamespace->setProperty(targetName, importedDoc->bind()->rootObjectValue());
        }
    }
    // file import
    else if (Document::Ptr importedDoc = _snapshot.document(path)) {
        QString targetName;
        if (import->importId) {
            targetName = import->importId->asString();
        } else {
            targetName = componentName(importedDoc->fileName());
        }

        importNamespace->setProperty(targetName, importedDoc->bind()->rootObjectValue());
    } else {
        _diagnosticMessages.append(DiagnosticMessage(
                DiagnosticMessage::Error, import->fileNameToken,
                QCoreApplication::translate("QmlJS::Link", "could not find file or directory")));
    }
}

static SourceLocation locationFromRange(const SourceLocation &start,
                                        const SourceLocation &end)
{
    return SourceLocation(start.offset,
                          end.end() - start.begin(),
                          start.startLine,
                          start.startColumn);
}

/*
  import Qt 4.6
  import Qt 4.6 as Xxx
  (import com.nokia.qt is the same as the ones above)
*/
void Link::importNonFile(Interpreter::ObjectValue *typeEnv, Document::Ptr doc, AST::UiImport *import)
{
    if (! import->importUri)
        return;

    ObjectValue *namespaceObject = 0;

    if (import->importId) { // with namespace we insert an object in the type env. to hold the imported types
        namespaceObject = engine()->newObject(/*prototype */ 0);
        typeEnv->setProperty(import->importId->asString(), namespaceObject);

    } else { // without namespace we insert all types directly into the type env.
        namespaceObject = typeEnv;
    }

    const QString package = Bind::toString(import->importUri, '/');
    int majorVersion = QmlObjectValue::NoVersion;
    int minorVersion = QmlObjectValue::NoVersion;

    if (import->versionToken.isValid()) {
        const QString versionString = doc->source().mid(import->versionToken.offset, import->versionToken.length);
        const int dotIdx = versionString.indexOf(QLatin1Char('.'));
        if (dotIdx == -1) {
            _diagnosticMessages.append(DiagnosticMessage(
                    DiagnosticMessage::Error, import->versionToken,
                    QCoreApplication::translate("QmlJS::Link", "expected two numbers separated by a dot")));
            return;
        } else {
            majorVersion = versionString.left(dotIdx).toInt();
            minorVersion = versionString.mid(dotIdx + 1).toInt();
        }
    } else {
        _diagnosticMessages.append(DiagnosticMessage(
                DiagnosticMessage::Error,
                locationFromRange(import->firstSourceLocation(), import->lastSourceLocation()),
                QCoreApplication::translate("QmlJS::Link", "package import requires a version number")));
        return;
    }

    // if the package is in the meta type system, use it
    if (engine()->metaTypeSystem().hasPackage(package)) {
        foreach (QmlObjectValue *object, engine()->metaTypeSystem().staticTypesForImport(package, majorVersion, minorVersion)) {
            namespaceObject->setProperty(object->className(), object);
        }
        return;
    } else {
        // check the filesystem
        QStringList localImportPaths = _importPaths;
        localImportPaths.prepend(doc->path());
        foreach (const QString &importPath, localImportPaths) {
            QDir dir(importPath);
            if (!dir.cd(package))
                continue;

            const LibraryInfo libraryInfo = _snapshot.libraryInfo(dir.path());
            if (!libraryInfo.isValid())
                continue;

            QSet<QString> importedTypes;
            foreach (const QmlDirParser::Component &component, libraryInfo.components()) {
                if (importedTypes.contains(component.typeName))
                    continue;

                if (component.majorVersion > majorVersion
                    || (component.majorVersion == majorVersion
                        && component.minorVersion > minorVersion))
                    continue;

                importedTypes.insert(component.typeName);
                if (Document::Ptr importedDoc = _snapshot.document(dir.filePath(component.fileName))) {
                    namespaceObject->setProperty(component.typeName, importedDoc->bind()->rootObjectValue());
                }
            }

            return;
        }
    }

    _diagnosticMessages.append(DiagnosticMessage(
            DiagnosticMessage::Error,
            locationFromRange(import->firstSourceLocation(), import->lastSourceLocation()),
            QCoreApplication::translate("QmlJS::Link", "package not found")));
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
