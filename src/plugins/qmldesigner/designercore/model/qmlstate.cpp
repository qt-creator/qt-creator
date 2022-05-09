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

#include "qmlstate.h"
#include "abstractview.h"
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <metainfo.h>
#include <invalidmodelnodeexception.h>
#include "bindingproperty.h"
#include "qmlchangeset.h"
#include "qmlitemnode.h"
#include "annotation.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

QmlModelState::QmlModelState()
{
}

QmlModelState::QmlModelState(const ModelNode &modelNode)
        : QmlModelNodeFacade(modelNode)
{
}

QmlPropertyChanges QmlModelState::propertyChanges(const ModelNode &node)
{
    if (!isBaseState()) {
        addChangeSetIfNotExists(node);

        const QList<ModelNode> nodes = modelNode().nodeListProperty("changes").toModelNodeList();
        for (const ModelNode &childNode : nodes) {
            //### exception if not valid QmlModelStateOperation
            if (QmlPropertyChanges::isValidQmlPropertyChanges(childNode)
                    && QmlPropertyChanges(childNode).target().isValid()
                    && QmlPropertyChanges(childNode).target() == node)
                return QmlPropertyChanges(childNode); //### exception if not valid(childNode);
        }
    }

    return QmlPropertyChanges(); //not found
}

QList<QmlModelStateOperation> QmlModelState::stateOperations(const ModelNode &node) const
{
    QList<QmlModelStateOperation> returnList;

    if (!isBaseState() &&  modelNode().hasNodeListProperty("changes")) {
        const QList<ModelNode> nodes = modelNode().nodeListProperty("changes").toModelNodeList();
        for (const ModelNode &childNode : nodes) {
            if (QmlModelStateOperation::isValidQmlModelStateOperation(childNode)) {
                QmlModelStateOperation stateOperation(childNode);
                ModelNode targetNode = stateOperation.target();
                if (targetNode.isValid() && targetNode == node)
                    returnList.append(stateOperation); //### exception if not valid(childNode);
            }
        }
    }

    return returnList; //not found
}

QList<QmlPropertyChanges> QmlModelState::propertyChanges() const
{
    QList<QmlPropertyChanges> returnList;

    if (!isBaseState() &&  modelNode().hasNodeListProperty("changes")) {
        const QList<ModelNode> nodes = modelNode().nodeListProperty("changes").toModelNodeList();
        for (const ModelNode &childNode : nodes) {
            //### exception if not valid QmlModelStateOperation
            if (QmlPropertyChanges::isValidQmlPropertyChanges(childNode))
                returnList.append(QmlPropertyChanges(childNode));
        }
    }

    return returnList;
}


bool QmlModelState::hasPropertyChanges(const ModelNode &node) const
{
    if (!isBaseState() &&  modelNode().hasNodeListProperty("changes")) {
        const QList<QmlPropertyChanges> changes = propertyChanges();
        for (const QmlPropertyChanges &changeSet : changes) {
            if (changeSet.target().isValid() && changeSet.target() == node)
                return true;
        }
    }

    return false;
}

bool QmlModelState::hasStateOperation(const ModelNode &node) const
{
    if (!isBaseState()) {
        const  QList<QmlModelStateOperation> operations = stateOperations();
        for (const  QmlModelStateOperation &stateOperation : operations) {
            if (stateOperation.target() == node)
                return true;
        }
    }
    return false;
}

QList<QmlModelStateOperation> QmlModelState::stateOperations() const
{
    //### exception if not valid
    QList<QmlModelStateOperation> returnList;

    if (!isBaseState() &&  modelNode().hasNodeListProperty("changes")) {
        const QList<ModelNode> nodes = modelNode().nodeListProperty("changes").toModelNodeList();
        for (const ModelNode &childNode : nodes) {
            //### exception if not valid QmlModelStateOperation
            if (QmlModelStateOperation::isValidQmlModelStateOperation(childNode))
                returnList.append(QmlModelStateOperation(childNode));
        }
    }

    return returnList;
}

QList<QmlModelStateOperation> QmlModelState::allInvalidStateOperations() const
{
    return Utils::filtered(stateOperations(), [](const QmlModelStateOperation &operation) {
        return !operation.target().isValid();
    });
}

/*!
    Adds a change set for \a node to this state, but only if it does not
    already exist.
*/

void QmlModelState::addChangeSetIfNotExists(const ModelNode &node)
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!hasPropertyChanges(node)) {
        ModelNode newChangeSet;

        const QByteArray typeName = "QtQuick.PropertyChanges";
        NodeMetaInfo metaInfo = modelNode().model()->metaInfo(typeName);

        int major = metaInfo.majorVersion();
        int minor = metaInfo.minorVersion();

        newChangeSet = modelNode().view()->createModelNode(typeName, major, minor);

        modelNode().nodeListProperty("changes").reparentHere(newChangeSet);

        QmlPropertyChanges(newChangeSet).setTarget(node);
        Q_ASSERT(QmlPropertyChanges::isValidQmlPropertyChanges(newChangeSet));
    }
}

void QmlModelState::removePropertyChanges(const ModelNode &node)
{
    //### exception if not valid

    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!isBaseState()) {
        QmlPropertyChanges changeSet(propertyChanges(node));
        if (changeSet.isValid())
            changeSet.modelNode().destroy();
    }
}



/*!
     Returns \c true if this state affects \a node.
*/
bool QmlModelState::affectsModelNode(const ModelNode &node) const
{
    if (isBaseState())
        return false;

    return !stateOperations(node).isEmpty();
}

QList<QmlObjectNode> QmlModelState::allAffectedNodes() const
{
    QList<QmlObjectNode> returnList;

    const QList<ModelNode> nodes = modelNode().nodeListProperty("changes").toModelNodeList();
    for (const ModelNode &childNode : nodes) {
        if (QmlModelStateOperation::isValidQmlModelStateOperation(childNode) &&
            !returnList.contains(QmlModelStateOperation(childNode).target()))
            returnList.append(QmlModelStateOperation(childNode).target());
    }

    return returnList;
}

QString QmlModelState::name() const
{
    if (isBaseState())
        return QString();

    return modelNode().variantProperty("name").value().toString();
}

void QmlModelState::setName(const QString &name)
{
    if ((!isBaseState()) && (modelNode().isValid()))
        modelNode().variantProperty("name").setValue(name);
}

bool QmlModelState::isValid() const
{
    return isValidQmlModelState(modelNode());
}

bool QmlModelState::isValidQmlModelState(const ModelNode &modelNode)
{
    return isValidQmlModelNodeFacade(modelNode)
            && modelNode.metaInfo().isValid()
            && (modelNode.metaInfo().isSubclassOf("QtQuick.State") || isBaseState(modelNode));
}

/**
  Removes state node & all subnodes.
  */
void QmlModelState::destroy()
{
    Q_ASSERT(isValid());
    modelNode().destroy();
}

/*!
    Returns \c true if this state is the base state.
*/

bool QmlModelState::isBaseState() const
{
    return isBaseState(modelNode());
}

bool QmlModelState::isBaseState(const ModelNode &modelNode)
{
    return !modelNode.isValid() || modelNode.isRootNode();
}

QmlModelState QmlModelState::duplicate(const QString &name) const
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!QmlVisualNode::isValidQmlVisualNode(modelNode().parentProperty().parentModelNode()))
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

//    QmlModelState newState(stateGroup().addState(name));
    QmlModelState newState(createQmlState(view(), {{PropertyName("name"), QVariant(name)}}));
    const QList<ModelNode> nodes = modelNode().nodeListProperty("changes").toModelNodeList();
    for (const ModelNode &childNode : nodes) {
        ModelNode newModelNode(view()->createModelNode(childNode.type(), childNode.majorVersion(), childNode.minorVersion()));
        const QList<BindingProperty> bindingProperties = childNode.bindingProperties();
        for (const BindingProperty &bindingProperty : bindingProperties)
            newModelNode.bindingProperty(bindingProperty.name()).setExpression(bindingProperty.expression());
        const QList<VariantProperty> variantProperties = childNode.variantProperties();
        for (const VariantProperty &variantProperty : variantProperties)
            newModelNode.variantProperty(variantProperty.name()).setValue(variantProperty.value());
        newState.modelNode().nodeListProperty("changes").reparentHere(newModelNode);
    }

    modelNode().parentProperty().reparentHere(newState);

    return newState;
}

QmlModelStateGroup QmlModelState::stateGroup() const
{
    QmlVisualNode parentNode(modelNode().parentProperty().parentModelNode());
    return parentNode.states();
}

ModelNode QmlModelState::createQmlState(AbstractView *view, const PropertyListType &propertyList)
{
    QTC_ASSERT(view, return {});
    QTC_CHECK(view->majorQtQuickVersion() < 3);

    const QByteArray typeName = "QtQuick.State";
    NodeMetaInfo metaInfo = view->model()->metaInfo(typeName);

    int major = metaInfo.majorVersion();
    int minor = metaInfo.minorVersion();

    return view->createModelNode(typeName, major, minor, propertyList);
}

void QmlModelState::setAsDefault()
{
    if ((!isBaseState()) && (modelNode().isValid())) {
        view()->rootModelNode().variantProperty("state").setValue(name());
    }
}

bool QmlModelState::isDefault() const
{
    if ((!isBaseState()) && (modelNode().isValid())) {
        if (view()->rootModelNode().hasProperty("state")) {
            return (view()->rootModelNode().variantProperty("state").value() == name());
        }
    }

    return false;
}

void QmlModelState::setAnnotation(const Annotation &annotation, const QString &id)
{
    if (modelNode().isValid()) {
        modelNode().setCustomId(id);
        modelNode().setAnnotation(annotation);
    }
}

Annotation QmlModelState::annotation() const
{
    if (modelNode().isValid())
        return modelNode().annotation();
    return {};
}

QString QmlModelState::annotationName() const
{
    if (modelNode().isValid())
        return modelNode().customId();
    return {};
}

bool QmlModelState::hasAnnotation() const
{
    if (modelNode().isValid())
        return modelNode().hasAnnotation() || modelNode().hasCustomId();
    return false;
}

void QmlModelState::removeAnnotation()
{
    if (modelNode().isValid()) {
        modelNode().removeCustomId();
        modelNode().removeAnnotation();
    }
}

QmlModelState QmlModelState::createBaseState(const AbstractView *view)
{
    QmlModelState qmlModelState(view->rootModelNode());

    return qmlModelState;
}

} // QmlDesigner
