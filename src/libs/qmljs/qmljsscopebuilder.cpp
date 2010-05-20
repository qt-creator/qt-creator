#include "qmljsscopebuilder.h"

#include "qmljsbind.h"
#include "qmljsinterpreter.h"
#include "qmljsevaluate.h"
#include "parser/qmljsast_p.h"

using namespace QmlJS;
using namespace QmlJS::Interpreter;
using namespace QmlJS::AST;

ScopeBuilder::ScopeBuilder(Document::Ptr doc, Interpreter::Context *context)
    : _doc(doc)
    , _context(context)
{
}

ScopeBuilder::~ScopeBuilder()
{
}

void ScopeBuilder::push(AST::Node *node)
{
    _nodes += node;

    // QML scope object
    Node *qmlObject = cast<UiObjectDefinition *>(node);
    if (! qmlObject)
        qmlObject = cast<UiObjectBinding *>(node);
    if (qmlObject)
        setQmlScopeObject(qmlObject);

    // JS scopes
    if (FunctionDeclaration *fun = cast<FunctionDeclaration *>(node)) {
        ObjectValue *functionScope = _doc->bind()->findFunctionScope(fun);
        if (functionScope)
            _context->scopeChain().jsScopes += functionScope;
    }

    _context->scopeChain().update();
}

void ScopeBuilder::push(const QList<AST::Node *> &nodes)
{
    foreach (Node *node, nodes)
        push(node);
}

void ScopeBuilder::pop()
{
    Node *toRemove = _nodes.last();
    _nodes.removeLast();

    // JS scopes
    if (cast<FunctionDeclaration *>(toRemove))
        _context->scopeChain().jsScopes.removeLast();

    // QML scope object
    if (! _nodes.isEmpty()
        && (cast<UiObjectDefinition *>(toRemove) || cast<UiObjectBinding *>(toRemove)))
        setQmlScopeObject(_nodes.last());

    _context->scopeChain().update();
}

void ScopeBuilder::setQmlScopeObject(Node *node)
{
    ScopeChain &scopeChain = _context->scopeChain();

    if (_doc->bind()->isGroupedPropertyBinding(node)) {
        UiObjectDefinition *definition = cast<UiObjectDefinition *>(node);
        if (!definition)
            return;
        const Value *v = scopeObjectLookup(definition->qualifiedTypeNameId);
        if (!v)
            return;
        const ObjectValue *object = v->asObjectValue();
        if (!object)
            return;

        scopeChain.qmlScopeObjects.clear();
        scopeChain.qmlScopeObjects += object;
    }

    const ObjectValue *scopeObject = _doc->bind()->findQmlObject(node);
    if (scopeObject) {
        scopeChain.qmlScopeObjects.clear();
        scopeChain.qmlScopeObjects += scopeObject;
    } else {
        return; // Probably syntax errors, where we're working with a "recovered" AST.
    }

    // check if the object has a Qt.ListElement or Qt.Connections ancestor
    // ### allow only signal bindings for Connections
    const ObjectValue *prototype = scopeObject->prototype(_context);
    while (prototype) {
        if (const QmlObjectValue *qmlMetaObject = dynamic_cast<const QmlObjectValue *>(prototype)) {
            if ((qmlMetaObject->className() == QLatin1String("ListElement")
                    || qmlMetaObject->className() == QLatin1String("Connections")
                    ) && qmlMetaObject->packageName() == QLatin1String("Qt")) {
                scopeChain.qmlScopeObjects.clear();
                break;
            }
        }
        prototype = prototype->prototype(_context);
    }

    // check if the object has a Qt.PropertyChanges ancestor
    prototype = scopeObject->prototype(_context);
    prototype = isPropertyChangesObject(_context, prototype);
    // find the target script binding
    if (prototype) {
        UiObjectInitializer *initializer = 0;
        if (UiObjectDefinition *definition = cast<UiObjectDefinition *>(node))
            initializer = definition->initializer;
        if (UiObjectBinding *binding = cast<UiObjectBinding *>(node))
            initializer = binding->initializer;
        if (initializer) {
            for (UiObjectMemberList *m = initializer->members; m; m = m->next) {
                if (UiScriptBinding *scriptBinding = cast<UiScriptBinding *>(m->member)) {
                    if (scriptBinding->qualifiedId
                            && scriptBinding->qualifiedId->name->asString() == QLatin1String("target")
                            && ! scriptBinding->qualifiedId->next) {
                        // ### make Evaluate understand statements.
                        if (ExpressionStatement *expStmt = cast<ExpressionStatement *>(scriptBinding->statement)) {
                            Evaluate evaluator(_context);
                            const Value *targetValue = evaluator(expStmt->expression);

                            if (const ObjectValue *target = value_cast<const ObjectValue *>(targetValue)) {
                                scopeChain.qmlScopeObjects.prepend(target);
                            } else {
                                scopeChain.qmlScopeObjects.clear();
                            }
                        }
                    }
                }
            }
        }
    }
}

const Value *ScopeBuilder::scopeObjectLookup(AST::UiQualifiedId *id)
{
    // do a name lookup on the scope objects
    const Value *result = 0;
    foreach (const ObjectValue *scopeObject, _context->scopeChain().qmlScopeObjects) {
        const ObjectValue *object = scopeObject;
        for (UiQualifiedId *it = id; it; it = it->next) {
            result = object->property(it->name->asString(), _context);
            if (!result)
                break;
            if (it->next) {
                object = result->asObjectValue();
                if (!object) {
                    result = 0;
                    break;
                }
            }
        }
        if (result)
            break;
    }

    return result;
}


const ObjectValue *ScopeBuilder::isPropertyChangesObject(Context *context,
                                                   const ObjectValue *object)
{
    const ObjectValue *prototype = object;
    while (prototype) {
        if (const QmlObjectValue *qmlMetaObject = dynamic_cast<const QmlObjectValue *>(prototype)) {
            if (qmlMetaObject->className() == QLatin1String("PropertyChanges")
                    && qmlMetaObject->packageName() == QLatin1String("Qt"))
                return prototype;
        }
        prototype = prototype->prototype(context);
    }
    return 0;
}
