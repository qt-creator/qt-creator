/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlobjectnode.h"
#include "qmlitemnode.h"
#include "qmlstate.h"
#include "variantproperty.h"
#include "nodeproperty.h"
#include <invalidmodelnodeexception.h>
#include "abstractview.h"
#include "nodeinstance.h"
#include "nodemetainfo.h"
#include "bindingproperty.h"
#include "nodelistproperty.h"
#include "nodeinstanceview.h"
#ifndef QMLDESIGNER_TEST
#include <qmldesignerplugin.h>
#endif

namespace QmlDesigner {

void QmlObjectNode::setVariantProperty(const PropertyName &name, const QVariant &value)
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (isInBaseState()) {
        modelNode().variantProperty(name).setValue(value); //basestate
    } else {
        modelNode().validId();

        QmlPropertyChanges changeSet(currentState().propertyChanges(modelNode()));
        Q_ASSERT(changeSet.isValid());
        changeSet.modelNode().variantProperty(name).setValue(value);
    }
}

void QmlObjectNode::setBindingProperty(const PropertyName &name, const QString &expression)
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (isInBaseState()) {
        modelNode().bindingProperty(name).setExpression(expression); //basestate
    } else {
        modelNode().validId();

        QmlPropertyChanges changeSet(currentState().propertyChanges(modelNode()));
        Q_ASSERT(changeSet.isValid());
        changeSet.modelNode().bindingProperty(name).setExpression(expression);
    }
}

QmlModelState QmlObjectNode::currentState() const
{
    if (isValid())
        return QmlModelState(view()->currentStateNode());
    else
        return QmlModelState();
}

bool QmlObjectNode::isRootModelNode() const
{
    return modelNode().isRootNode();
}


/*!
    Returns the value of the property specified by \name that is based on an
    actual instance. The return value is not the value in the model, but the
    value of a real instantiated instance of this object.
*/
QVariant  QmlObjectNode::instanceValue(const PropertyName &name) const
{
    return nodeInstance().property(name);
}


bool QmlObjectNode::hasProperty(const PropertyName &name) const
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

bool QmlObjectNode::hasBindingProperty(const PropertyName &name) const
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

NodeAbstractProperty QmlObjectNode::nodeAbstractProperty(const PropertyName &name) const
{
    return modelNode().nodeAbstractProperty(name);
}

NodeAbstractProperty QmlObjectNode::defaultNodeAbstractProperty() const
{
    return modelNode().defaultNodeAbstractProperty();
}

NodeProperty QmlObjectNode::nodeProperty(const PropertyName &name) const
{
    return modelNode().nodeProperty(name);
}

NodeListProperty QmlObjectNode::nodeListProperty(const PropertyName &name) const
{
    return modelNode().nodeListProperty(name);
}

bool QmlObjectNode::instanceHasValue(const PropertyName &name) const
{
    return nodeInstance().hasProperty(name);
}

bool QmlObjectNode::propertyAffectedByCurrentState(const PropertyName &name) const
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (currentState().isBaseState())
        return modelNode().hasProperty(name);

    if (!currentState().hasPropertyChanges(modelNode()))
        return false;

    return currentState().propertyChanges(modelNode()).modelNode().hasProperty(name);
}

QVariant QmlObjectNode::modelValue(const PropertyName &name) const
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

bool QmlObjectNode::isTranslatableText(const PropertyName &name) const
{
    if (modelNode().metaInfo().isValid() && modelNode().metaInfo().hasProperty(name))
        if (modelNode().metaInfo().propertyTypeName(name) == "QString" || modelNode().metaInfo().propertyTypeName(name) == "string") {
            if (modelNode().hasBindingProperty(name)) {
                static QRegExp regularExpressionPatter(QLatin1String("qsTr(?:|Id)\\((\".*\")\\)"));
                return regularExpressionPatter.exactMatch(modelNode().bindingProperty(name).expression());
            }

            return false;
        }

    return false;
}

QString QmlObjectNode::stripedTranslatableText(const PropertyName &name) const
{
    if (modelNode().hasBindingProperty(name)) {
        static QRegExp regularExpressionPatter(QLatin1String("qsTr(?:|Id)\\(\"(.*)\"\\)"));
        if (regularExpressionPatter.exactMatch(modelNode().bindingProperty(name).expression()))
            return regularExpressionPatter.cap(1);
    } else {
        return modelNode().variantProperty(name).value().toString();
    }

    return QString();
}

QString QmlObjectNode::expression(const PropertyName &name) const
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

/*!
    Returns \c true if the ObjectNode is in the BaseState.
*/
bool QmlObjectNode::isInBaseState() const
{
    return currentState().isBaseState();
}

bool QmlObjectNode::instanceCanReparent() const
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

        foreach (const QmlObjectNode &childNode, node.modelNode().directSubModelNodes()) {
            removeStateOperationsForChildren(childNode);
        }
    }
}

static void removeAliasExports(const QmlObjectNode &node)
{

    PropertyName propertyName = node.id().toUtf8();

    ModelNode rootNode = node.view()->rootModelNode();
    bool hasAliasExport = !propertyName.isEmpty()
            && rootNode.isValid()
            && rootNode.hasBindingProperty(propertyName)
            && rootNode.bindingProperty(propertyName).isAliasExport();

    if (hasAliasExport)
        rootNode.removeProperty(propertyName);

    foreach (const ModelNode &childNode, node.modelNode().directSubModelNodes()) {
        removeAliasExports(childNode);
    }

}

/*!
    Deletes this object's node and its dependencies from the model.
    Everything that belongs to this Object, the ModelNode, and ChangeOperations
    is deleted from the model.
*/
void QmlObjectNode::destroy()
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    removeAliasExports(modelNode());

    foreach (QmlModelStateOperation stateOperation, allAffectingStatesOperations()) {
        stateOperation.modelNode().destroy(); //remove of belonging StatesOperations
    }
    removeStateOperationsForChildren(modelNode());
    modelNode().destroy();
}

void QmlObjectNode::ensureAliasExport()
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!isAliasExported()) {
        modelNode().validId();
        ModelNode rootModelNode = view()->rootModelNode();
        rootModelNode.bindingProperty(modelNode().id().toUtf8()).
            setDynamicTypeNameAndExpression("alias", modelNode().id());
    }
}

bool QmlObjectNode::isAliasExported() const
{

    if (modelNode().isValid() && !modelNode().id().isEmpty()) {
         PropertyName modelNodeId = modelNode().id().toUtf8();
         ModelNode rootModelNode = view()->rootModelNode();
         Q_ASSERT(rootModelNode.isValid());
         if (rootModelNode.hasBindingProperty(modelNodeId)
                 && rootModelNode.bindingProperty(modelNodeId).isDynamic()
                 && rootModelNode.bindingProperty(modelNodeId).expression() == modelNode().id())
             return true;
    }

    return false;
}

/*!
    Returns a list of states the affect this object.
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

/*!
    Returns a list of all state operations that affect this object.
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

static QList<QmlItemNode> allQmlItemsRecursive(const QmlItemNode &qmlItemNode)
{
    QList<QmlItemNode> qmlItemNodeList;

    if (qmlItemNode.isValid()) {
        qmlItemNodeList.append(qmlItemNode);

        foreach (const ModelNode &modelNode, qmlItemNode.modelNode().directSubModelNodes()) {
            if (QmlItemNode::isValidQmlItemNode(modelNode))
                qmlItemNodeList.append(allQmlItemsRecursive(modelNode));
        }
    }

    return qmlItemNodeList;
}

QList<QmlModelState> QmlObjectNode::allDefinedStates() const
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    QList<QmlModelState> returnList;

    QList<QmlItemNode> allQmlItems;

    if (QmlItemNode::isValidQmlItemNode(view()->rootModelNode()))
        allQmlItems.append(allQmlItemsRecursive(view()->rootModelNode()));

    foreach (const QmlItemNode &item, allQmlItems) {
        returnList.append(item.states().allStates());
    }

    return returnList;
}


/*!
    Removes a variant property of the object specified by \a name from the
    model.
*/

void  QmlObjectNode::removeProperty(const PropertyName &name)
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

QList<ModelNode> toModelNodeList(const QList<QmlObjectNode> &qmlObjectNodeList)
{
    QList<ModelNode> modelNodeList;

    foreach (const QmlObjectNode &qmlObjectNode, qmlObjectNodeList)
        modelNodeList.append(qmlObjectNode.modelNode());

    return modelNodeList;
}

QList<QmlObjectNode> toQmlObjectNodeList(const QList<ModelNode> &modelNodeList)
{
    QList<QmlObjectNode> qmlObjectNodeList;

    foreach (const ModelNode &modelNode, modelNodeList) {
        if (QmlObjectNode::isValidQmlObjectNode(modelNode))
             qmlObjectNodeList.append(modelNode);
    }

    return qmlObjectNodeList;
}

bool QmlObjectNode::isAncestorOf(const QmlObjectNode &objectNode) const
{
    return modelNode().isAncestorOf(objectNode.modelNode());
}

QVariant QmlObjectNode::instanceValue(const ModelNode &modelNode, const PropertyName &name)
{
    Q_ASSERT(modelNode.view()->nodeInstanceView()->hasInstanceForModelNode(modelNode));
    return modelNode.view()->nodeInstanceView()->instanceForModelNode(modelNode).property(name);
}

QString QmlObjectNode::generateTranslatableText(const QString &text)
{
#ifndef QMLDESIGNER_TEST

    if (QmlDesignerPlugin::instance()->settings().value(
            DesignerSettingsKey::TYPE_OF_QSTR_FUNCTION).toInt())

        switch (QmlDesignerPlugin::instance()->settings().value(
                    DesignerSettingsKey::TYPE_OF_QSTR_FUNCTION).toInt()) {
        case 0: return QString(QStringLiteral("qsTr(\"%1\")")).arg(text);
        case 1: return QString(QStringLiteral("qsTrId(\"%1\")")).arg(text);
        case 2: return QString(QStringLiteral("qsTranslate(\"\"\"%1\")")).arg(text);
        default:
            break;

        }
    return QString(QStringLiteral("qsTr(\"%1\")")).arg(text);
#else
    Q_UNUSED(text);
    return QString();
#endif
}

TypeName QmlObjectNode::instanceType(const PropertyName &name) const
{
    return nodeInstance().instanceType(name);
}

bool QmlObjectNode::instanceHasBinding(const PropertyName &name) const
{
    return nodeInstance().hasBindingForProperty(name);
}

NodeInstance QmlObjectNode::nodeInstance() const
{
    return nodeInstanceView()->instanceForModelNode(modelNode());
}

QmlObjectNode QmlObjectNode::nodeForInstance(const NodeInstance &instance) const
{
    return QmlObjectNode(ModelNode(instance.modelNode(), view()));
}

QmlItemNode QmlObjectNode::itemForInstance(const NodeInstance &instance) const
{
    return QmlItemNode(ModelNode(instance.modelNode(), view()));
}

QmlObjectNode::QmlObjectNode()
    : QmlModelNodeFacade()
{
}

QmlObjectNode::QmlObjectNode(const ModelNode &modelNode)
    : QmlModelNodeFacade(modelNode)
{
}

bool QmlObjectNode::isValidQmlObjectNode(const ModelNode &modelNode)
{
    return isValidQmlModelNodeFacade(modelNode);
}

bool QmlObjectNode::isValid() const
{
    return isValidQmlObjectNode(modelNode());
}

bool QmlObjectNode::hasError() const
{
    if (isValid())
        return nodeInstance().hasError();
    else
        qDebug() << "called hasError() on an invalid QmlObjectNode";
    return false;
}

QString QmlObjectNode::error() const
{
    if (hasError())
        return nodeInstance().error();
    return QString();
}

bool QmlObjectNode::hasNodeParent() const
{
    return modelNode().hasParentProperty();
}

bool QmlObjectNode::hasInstanceParent() const
{
    return nodeInstance().parentId() >= 0 && nodeInstanceView()->hasInstanceForId(nodeInstance().parentId());
}

bool QmlObjectNode::hasInstanceParentItem() const
{
    return nodeInstance().parentId() >= 0
            && nodeInstanceView()->hasInstanceForId(nodeInstance().parentId())
            && QmlItemNode::isItemOrWindow(view()->modelNodeForInternalId(nodeInstance().parentId()));
}


void QmlObjectNode::setParentProperty(const NodeAbstractProperty &parentProeprty)
{
    return modelNode().setParentProperty(parentProeprty);
}

QmlObjectNode QmlObjectNode::instanceParent() const
{
    if (hasInstanceParent())
        return nodeForInstance(nodeInstanceView()->instanceForId(nodeInstance().parentId()));

    return QmlObjectNode();
}

QmlItemNode QmlObjectNode::instanceParentItem() const
{
    if (hasInstanceParentItem())
        return itemForInstance(nodeInstanceView()->instanceForId(nodeInstance().parentId()));

    return QmlItemNode();
}

void QmlObjectNode::setId(const QString &id)
{
    modelNode().setIdWithRefactoring(id);
}

QString QmlObjectNode::id() const
{
    return modelNode().id();
}

QString QmlObjectNode::validId()
{
    return modelNode().validId();
}

bool QmlObjectNode::hasDefaultPropertyName() const
{
    return modelNode().metaInfo().hasDefaultProperty();
}

PropertyName QmlObjectNode::defaultPropertyName() const
{
    return modelNode().metaInfo().defaultPropertyName();
}

void QmlObjectNode::setParent(const QmlObjectNode &newParent)
{
    if (newParent.hasDefaultPropertyName())
        newParent.modelNode().defaultNodeAbstractProperty().reparentHere(modelNode());
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
