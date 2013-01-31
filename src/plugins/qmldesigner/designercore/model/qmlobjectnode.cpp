/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlobjectnode.h"
#include "qmlitemnode.h"
#include "qmlitemnode.h"
#include "qmlstate.h"
#include "variantproperty.h"
#include "nodeproperty.h"
#include <invalidmodelnodeexception.h>
#include "qmlmodelview.h"
#include "nodeinstanceview.h"
#include "nodeinstance.h"
#include "nodemetainfo.h"
#include "bindingproperty.h"
#include "nodelistproperty.h"

namespace QmlDesigner {

void QmlObjectNode::setVariantProperty(const QString &name, const QVariant &value)
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (isInBaseState()) {
        modelNode().variantProperty(name).setValue(value); //basestate
    } else {
        modelNode().validId();

        QmlPropertyChanges changeSet(currentState().propertyChanges(modelNode()));
        Q_ASSERT(changeSet.isValid());
        changeSet.modelNode().variantProperty(name) = value;
    }
}

void QmlObjectNode::setBindingProperty(const QString &name, const QString &expression)
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (isInBaseState()) {
        modelNode().bindingProperty(name) = expression; //basestate
    } else {
        modelNode().validId();

        QmlPropertyChanges changeSet(currentState().propertyChanges(modelNode()));
        Q_ASSERT(changeSet.isValid());
        changeSet.modelNode().bindingProperty(name) = expression;
    }
}

QmlModelState QmlObjectNode::currentState() const
{
    if (isValid())
        return qmlModelView()->currentState();
    else
        return QmlModelState();
}

bool QmlObjectNode::isRootModelNode() const
{
    return modelNode().isRootNode();
}


/*! \brief returns the value of a property based on an actual instance
The return value is not the value in the model, but the value of a real
instanciated instance of this object.
\return the value of this property based on the instance

*/
QVariant  QmlObjectNode::instanceValue(const QString &name) const
{
    return nodeInstance().property(name);
}


bool QmlObjectNode::hasProperty(const QString &name) const
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (currentState().hasPropertyChanges(modelNode())) {
        QmlPropertyChanges propertyChanges = currentState().propertyChanges(modelNode());
        if (propertyChanges.modelNode().hasProperty(name))
            return true;
    }

    return modelNode().hasProperty(name);
}

bool QmlObjectNode::hasBindingProperty(const QString &name) const
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (currentState().hasPropertyChanges(modelNode())) {
        QmlPropertyChanges propertyChanges = currentState().propertyChanges(modelNode());
        if (propertyChanges.modelNode().hasBindingProperty(name))
            return true;
    }

    return modelNode().hasBindingProperty(name);
}

NodeAbstractProperty QmlObjectNode::nodeAbstractProperty(const QString &name) const
{
   return modelNode().nodeAbstractProperty(name);
}

NodeProperty QmlObjectNode::nodeProperty(const QString &name) const
{
    return modelNode().nodeProperty(name);
}

NodeListProperty QmlObjectNode::nodeListProperty(const QString &name) const
{
    return modelNode().nodeListProperty(name);
}

bool QmlObjectNode::propertyAffectedByCurrentState(const QString &name) const
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (currentState().isBaseState())
        return modelNode().hasProperty(name);

    if (!currentState().hasPropertyChanges(modelNode()))
        return false;

    return currentState().propertyChanges(modelNode()).modelNode().hasProperty(name);
}

QVariant QmlObjectNode::modelValue(const QString &name) const
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (currentState().isBaseState())
        return modelNode().variantProperty(name).value();

    if (!currentState().hasPropertyChanges(modelNode()))
        return modelNode().variantProperty(name).value();

    QmlPropertyChanges propertyChanges(currentState().propertyChanges(modelNode()));

    if (!propertyChanges.modelNode().hasProperty(name))
        return modelNode().variantProperty(name).value();

    return propertyChanges.modelNode().variantProperty(name).value();
}

QString QmlObjectNode::expression(const QString &name) const
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (currentState().isBaseState())
        return modelNode().bindingProperty(name).expression();

    if (!currentState().hasPropertyChanges(modelNode()))
        return modelNode().bindingProperty(name).expression();

    QmlPropertyChanges propertyChanges(currentState().propertyChanges(modelNode()));

    if (!propertyChanges.modelNode().hasProperty(name))
        return modelNode().bindingProperty(name).expression();

    return propertyChanges.modelNode().bindingProperty(name).expression();
}

/*! \brief returns if ObjectNode is the BaseState

\return true if the ObjectNode is in the BaseState

*/
bool QmlObjectNode::isInBaseState() const
{
    return currentState().isBaseState();
}

bool QmlObjectNode::canReparent() const
{
    return isInBaseState();
}

QmlPropertyChanges QmlObjectNode::propertyChangeForCurrentState() const
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

     if (currentState().isBaseState())
         return QmlPropertyChanges();

     if (!currentState().hasPropertyChanges(modelNode()))
         return QmlPropertyChanges();

     return currentState().propertyChanges(modelNode());
}

static void removeStateOperationsForChildren(const QmlObjectNode &node)
{
    if (node.isValid()) {
        foreach (const QmlModelStateOperation &stateOperation, node.allAffectingStatesOperations()) {
            stateOperation.modelNode().destroy(); //remove of belonging StatesOperations
        }

        foreach (const QmlObjectNode &childNode, node.modelNode().allDirectSubModelNodes()) {
            removeStateOperationsForChildren(childNode);
        }
    }
}


/*! \brief Deletes this objects ModeNode and its dependencies from the model
Every thing that belongs to this Object, the ModelNode and ChangeOperations
are deleted from the model.

*/
void QmlObjectNode::destroy()
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    foreach (const QmlModelStateOperation &stateOperation, allAffectingStatesOperations()) {
        stateOperation.modelNode().destroy(); //remove of belonging StatesOperations
    }
    removeStateOperationsForChildren(modelNode());
    modelNode().destroy();
}

/*! \brief Returns a list of all states that are affecting this object.

\return list of states affecting this object
*/

QList<QmlModelState> QmlObjectNode::allAffectingStates() const
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    QList<QmlModelState> returnList;

    foreach (const QmlModelState &state, allDefinedStates()) {
        if (state.affectsModelNode(modelNode()))
            returnList.append(state);
    }
    return returnList;
}

/*! \brief Returns a list of all state operations that are affecting this object.

\return list of state operations affecting this object
*/

QList<QmlModelStateOperation> QmlObjectNode::allAffectingStatesOperations() const
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    QList<QmlModelStateOperation> returnList;
    foreach (const QmlModelState &state, allDefinedStates()) {
        if (state.affectsModelNode(modelNode()))
            returnList.append(state.stateOperations(modelNode()));
    }

    return returnList;
}

static QList<QmlItemNode> allFxItemsRecursive(const QmlItemNode &fxNode)
{
    QList<QmlItemNode> returnList;

    if (fxNode.isValid()) {
        returnList.append(fxNode);
        QList<QmlItemNode> allChildNodes;
        foreach (const ModelNode &node, fxNode.modelNode().allDirectSubModelNodes()) {
            if (QmlItemNode(node).isValid())
                allChildNodes.append(node);
        }
        foreach (const QmlItemNode &node, allChildNodes) {
            returnList.append(allFxItemsRecursive(node));
        }
    }
    return returnList;
}

QList<QmlModelState> QmlObjectNode::allDefinedStates() const
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    QList<QmlModelState> returnList;

    QList<QmlItemNode> allFxItems;

    QmlItemNode rootNode(qmlModelView()->rootModelNode());

    if (rootNode.isValid())
        allFxItems.append(allFxItemsRecursive(rootNode));

    foreach (const QmlItemNode &item, allFxItems) {
        returnList.append(item.states().allStates());
    }

    return returnList;
}


/*! \brief Removes a variant property of this object from the model

*/

void  QmlObjectNode::removeVariantProperty(const QString &name)
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (isInBaseState()) {
        modelNode().removeProperty(name); //basestate
    } else {
        QmlPropertyChanges changeSet(currentState().propertyChanges(modelNode()));
        Q_ASSERT(changeSet.isValid());
        changeSet.removeProperty(name);
    }
}

QList<ModelNode> toModelNodeList(const QList<QmlObjectNode> &fxObjectNodeList)
{
    QList<ModelNode> modelNodeList;

    foreach (const QmlObjectNode &fxObjectNode, fxObjectNodeList)
        modelNodeList.append(fxObjectNode.modelNode());

    return modelNodeList;
}

QList<QmlObjectNode> toQmlObjectNodeList(const QList<ModelNode> &modelNodeList)
{
    QList<QmlObjectNode> qmlObjectNodeList;

    foreach (const ModelNode &modelNode, modelNodeList) {
        QmlObjectNode objectNode(modelNode);
        if (objectNode.isValid())
            qmlObjectNodeList.append(objectNode);
    }

    return qmlObjectNodeList;
}

bool QmlObjectNode::isAncestorOf(const QmlObjectNode &objectNode) const
{
    return modelNode().isAncestorOf(objectNode.modelNode());
}

QVariant QmlObjectNode::instanceValue(const ModelNode &modelNode, const QString &name)
{
    QmlModelView *modelView = qobject_cast<QmlModelView*>(modelNode.view());
    if (!modelView)
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    Q_ASSERT(modelView->hasInstanceForModelNode(modelNode));
    return modelView->instanceForModelNode(modelNode).property(name);
}

QString QmlObjectNode::instanceType(const QString &name) const
{
    return nodeInstance().instanceType(name);
}

bool QmlObjectNode::instanceHasBinding(const QString &name) const
{
    QmlModelView *modelView = qobject_cast<QmlModelView*>(modelNode().view());
    if (!modelView)
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return nodeInstance().hasBindingForProperty(name);
}

NodeInstance QmlObjectNode::nodeInstance() const
{
    return qmlModelView()->nodeInstanceView()->instanceForNode(modelNode());
}

QmlObjectNode QmlObjectNode::nodeForInstance(const NodeInstance &instance) const
{
    return QmlObjectNode(ModelNode(instance.modelNode(), qmlModelView()));
}

bool QmlObjectNode::hasNodeParent() const
{
    return modelNode().hasParentProperty();
}

bool QmlObjectNode::hasInstanceParent() const
{
    return nodeInstance().parentId() >= 0 && qmlModelView()->nodeInstanceView()->hasInstanceForId(nodeInstance().parentId());
}


void QmlObjectNode::setParentProperty(const NodeAbstractProperty &parentProeprty)
{
    return modelNode().setParentProperty(parentProeprty);
}

QmlObjectNode QmlObjectNode::instanceParent() const
{
    if (hasInstanceParent())
        return nodeForInstance(qmlModelView()->nodeInstanceView()->instanceForId(nodeInstance().parentId()));

    return QmlObjectNode();
}

void QmlObjectNode::setId(const QString &id)
{
    modelNode().setId(id);
}

QString QmlObjectNode::id() const
{
    return modelNode().id();
}

QString QmlObjectNode::validId()
{
    return modelNode().validId();
}

bool QmlObjectNode::hasDefaultProperty() const
{
    return modelNode().metaInfo().hasDefaultProperty();
}

QString QmlObjectNode::defaultProperty() const
{
    return modelNode().metaInfo().defaultPropertyName();
}

void QmlObjectNode::setParent(QmlObjectNode newParent)
{
    if (newParent.hasDefaultProperty())
        newParent.modelNode().nodeAbstractProperty(newParent.defaultProperty()).reparentHere(modelNode());
}

QmlItemNode QmlObjectNode::toQmlItemNode() const
{
    return QmlItemNode(modelNode());
}

uint qHash(const QmlObjectNode &node)
{
    return qHash(node.modelNode());
}
} //QmlDesigner
