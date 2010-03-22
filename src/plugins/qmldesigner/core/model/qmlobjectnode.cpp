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

namespace QmlDesigner {

void QmlObjectNode::setVariantProperty(const QString &name, const QVariant &value)
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (isInBaseState()) {
        modelNode().variantProperty(name) = value; //basestate
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
    Q_ASSERT(qmlModelView()->hasInstanceForModelNode(modelNode()));
    return qmlModelView()->instanceForModelNode(modelNode()).property(name);
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
        foreach (QmlModelStateOperation stateOperation, node.allAffectingStatesOperations()) {
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

    foreach (QmlModelStateOperation stateOperation, allAffectingStatesOperations()) {
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

    foreach(const QmlObjectNode &fxObjectNode, fxObjectNodeList)
        modelNodeList.append(fxObjectNode.modelNode());

    return modelNodeList;
}

QList<QmlObjectNode> toQmlObjectNodeList(const QList<ModelNode> &modelNodeList)
{
    QList<QmlObjectNode> qmlObjectNodeList;

    foreach(const ModelNode &modelNode, modelNodeList) {
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
    return nodeInstance().hasParent();
}


void QmlObjectNode::setParentProperty(const NodeAbstractProperty &parentProeprty)
{
    return modelNode().setParentProperty(parentProeprty);
}

QmlObjectNode QmlObjectNode::instanceParent() const
{
    return nodeForInstance(nodeInstance().parent());
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

void QmlObjectNode::setParent(QmlObjectNode newParent)
{
    newParent.modelNode().nodeListProperty("data").reparentHere(modelNode());
    //TODO use metasystem for default Property
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
