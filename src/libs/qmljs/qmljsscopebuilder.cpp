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

#include "qmljsscopebuilder.h"

#include "qmljsbind.h"
#include "qmljscontext.h"
#include "qmljsevaluate.h"
#include "qmljsscopechain.h"
#include "parser/qmljsast_p.h"

using namespace QmlJS;
using namespace QmlJS::AST;

ScopeBuilder::ScopeBuilder(ScopeChain *scopeChain)
    : _scopeChain(scopeChain)
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
    switch (node->kind) {
    case Node::Kind_UiScriptBinding:
    case Node::Kind_FunctionDeclaration:
    case Node::Kind_FunctionExpression:
    case Node::Kind_UiPublicMember:
    {
        ObjectValue *scope = _scopeChain->document()->bind()->findAttachedJSScope(node);
        if (scope) {
            QList<const ObjectValue *> jsScopes = _scopeChain->jsScopes();
            jsScopes += scope;
            _scopeChain->setJsScopes(jsScopes);
        }
        break;
    }
    default:
        break;
    }
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
    switch (toRemove->kind) {
    case Node::Kind_UiScriptBinding:
    case Node::Kind_FunctionDeclaration:
    case Node::Kind_FunctionExpression:
    case Node::Kind_UiPublicMember:
    {
        ObjectValue *scope = _scopeChain->document()->bind()->findAttachedJSScope(toRemove);
        if (scope) {
            QList<const ObjectValue *> jsScopes = _scopeChain->jsScopes();
            if (!jsScopes.isEmpty()) {
                jsScopes.removeLast();
                _scopeChain->setJsScopes(jsScopes);
            }
        }
        break;
    }
    default:
        break;
    }

    // QML scope object
    if (! _nodes.isEmpty()
        && (cast<UiObjectDefinition *>(toRemove) || cast<UiObjectBinding *>(toRemove)))
        setQmlScopeObject(_nodes.last());
}

void ScopeBuilder::setQmlScopeObject(Node *node)
{
    QList<const ObjectValue *> qmlScopeObjects;
    if (_scopeChain->document()->bind()->isGroupedPropertyBinding(node)) {
        UiObjectDefinition *definition = cast<UiObjectDefinition *>(node);
        if (!definition)
            return;
        const Value *v = scopeObjectLookup(definition->qualifiedTypeNameId);
        if (!v)
            return;
        const ObjectValue *object = v->asObjectValue();
        if (!object)
            return;

        qmlScopeObjects += object;
        _scopeChain->setQmlScopeObjects(qmlScopeObjects);
        return;
    }

    const ObjectValue *scopeObject = _scopeChain->document()->bind()->findQmlObject(node);
    if (scopeObject) {
        qmlScopeObjects += scopeObject;
    } else {
        return; // Probably syntax errors, where we're working with a "recovered" AST.
    }

    // check if the object has a Qt.ListElement or Qt.Connections ancestor
    // ### allow only signal bindings for Connections
    PrototypeIterator iter(scopeObject, _scopeChain->context());
    iter.next();
    while (iter.hasNext()) {
        const ObjectValue *prototype = iter.next();
        if (const QmlObjectValue *qmlMetaObject = dynamic_cast<const QmlObjectValue *>(prototype)) {
            if ((qmlMetaObject->className() == QLatin1String("ListElement")
                    || qmlMetaObject->className() == QLatin1String("Connections")
                    ) && (qmlMetaObject->moduleName() == QLatin1String("Qt")
                          || qmlMetaObject->moduleName() == QLatin1String("QtQuick"))) {
                qmlScopeObjects.clear();
                break;
            }
        }
    }

    // check if the object has a Qt.PropertyChanges ancestor
    const ObjectValue *prototype = scopeObject->prototype(_scopeChain->context());
    prototype = isPropertyChangesObject(_scopeChain->context(), prototype);
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
                            && scriptBinding->qualifiedId->name == QLatin1String("target")
                            && ! scriptBinding->qualifiedId->next) {
                        Evaluate evaluator(_scopeChain);
                        const Value *targetValue = evaluator(scriptBinding->statement);

                        if (const ObjectValue *target = value_cast<const ObjectValue *>(targetValue)) {
                            qmlScopeObjects.prepend(target);
                        } else {
                            qmlScopeObjects.clear();
                        }
                    }
                }
            }
        }
    }

    _scopeChain->setQmlScopeObjects(qmlScopeObjects);
}

const Value *ScopeBuilder::scopeObjectLookup(AST::UiQualifiedId *id)
{
    // do a name lookup on the scope objects
    const Value *result = 0;
    foreach (const ObjectValue *scopeObject, _scopeChain->qmlScopeObjects()) {
        const ObjectValue *object = scopeObject;
        for (UiQualifiedId *it = id; it; it = it->next) {
            if (it->name.isEmpty())
                return 0;
            result = object->lookupMember(it->name.toString(), _scopeChain->context());
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


const ObjectValue *ScopeBuilder::isPropertyChangesObject(const ContextPtr &context,
                                                   const ObjectValue *object)
{
    PrototypeIterator iter(object, context);
    while (iter.hasNext()) {
        const ObjectValue *prototype = iter.next();
        if (const QmlObjectValue *qmlMetaObject = dynamic_cast<const QmlObjectValue *>(prototype)) {
            if (qmlMetaObject->className() == QLatin1String("PropertyChanges")
                    && (qmlMetaObject->moduleName() == QLatin1String("Qt")
                        || qmlMetaObject->moduleName() == QLatin1String("QtQuick")))
                return prototype;
        }
    }
    return 0;
}
