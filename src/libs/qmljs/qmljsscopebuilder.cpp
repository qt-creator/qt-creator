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
        ObjectValue *activation = _context->engine()->newObject(/*prototype = */ 0);
        for (FormalParameterList *it = fun->formals; it; it = it->next) {
            if (it->name)
                activation->setProperty(it->name->asString(), _context->engine()->undefinedValue());
        }
        _context->scopeChain().jsScopes += activation;
    }

    _context->scopeChain().update();
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

    scopeChain.qmlScopeObjects.clear();

    const ObjectValue *scopeObject = _doc->bind()->findQmlObject(node);
    if (scopeObject) {
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
    while (prototype) {
        if (const QmlObjectValue *qmlMetaObject = dynamic_cast<const QmlObjectValue *>(prototype)) {
            if (qmlMetaObject->className() == QLatin1String("PropertyChanges")
                    && qmlMetaObject->packageName() == QLatin1String("Qt"))
                break;
        }
        prototype = prototype->prototype(_context);
    }
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
