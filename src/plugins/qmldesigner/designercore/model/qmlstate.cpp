// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    if (isValid() && !isBaseState()) {
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

    if (!isBaseState() && modelNode().hasNodeListProperty("changes")) {
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

    if (!isBaseState() && modelNode().hasNodeListProperty("changes")) {
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
    if (!isBaseState() && modelNode().hasNodeListProperty("changes")) {
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

    if (!isBaseState() && modelNode().hasNodeListProperty("changes")) {
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
        return;

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
        return;

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
    if (!isValid())
        return false;

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
           && (modelNode.metaInfo().isQtQuickState() || isBaseState(modelNode));
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
        return {};

    QmlModelState newState(createQmlState(view(), {{PropertyName("name"), QVariant(name)}}));

    if (hasExtend())
        newState.setExtend(extend());

    const QList<ModelNode> nodes = modelNode().nodeListProperty("changes").toModelNodeList();
    for (const ModelNode &childNode : nodes) {
        ModelNode newModelNode(view()->createModelNode(childNode.type(),
                                                       childNode.majorVersion(),
                                                       childNode.minorVersion()));

        for (const BindingProperty &bindingProperty : childNode.bindingProperties()) {
            auto property = newModelNode.bindingProperty(bindingProperty.name());
            property.setExpression(bindingProperty.expression());
        }

        const QList<BindingProperty> bindingProperties = childNode.bindingProperties();
        for (const BindingProperty &bindingProperty : bindingProperties)
            newModelNode.bindingProperty(bindingProperty.name())
                .setExpression(bindingProperty.expression());

        for (const VariantProperty &variantProperty : childNode.variantProperties()) {
            auto property = newModelNode.variantProperty(variantProperty.name());
            property.setValue(variantProperty.value());
        }
        newState.modelNode().nodeListProperty("changes").reparentHere(newModelNode);
    }

    modelNode().parentProperty().reparentHere(newState);

    return newState;
}

QmlModelStateGroup QmlModelState::stateGroup() const
{
    QmlObjectNode parentNode(modelNode().parentProperty().parentModelNode());
    return parentNode.states();
}

ModelNode QmlModelState::createQmlState(AbstractView *view, const PropertyListType &propertyList)
{
    QTC_ASSERT(view, return {});

    const QByteArray typeName = "QtQuick.State";
    NodeMetaInfo metaInfo = view->model()->metaInfo(typeName);

    int major = metaInfo.majorVersion();
    int minor = metaInfo.minorVersion();

    return view->createModelNode(typeName, major, minor, propertyList);
}

void QmlModelState::setAsDefault()
{
    if ((!isBaseState()) && (modelNode().isValid())) {
        stateGroup().modelNode().variantProperty("state").setValue(name());
    }
}

bool QmlModelState::isDefault() const
{
    if ((!isBaseState()) && (modelNode().isValid())) {
        if (stateGroup().modelNode().hasProperty("state")) {
            return (stateGroup().modelNode().variantProperty("state").value() == name());
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

QString QmlModelState::extend() const
{
    if (isBaseState())
        return QString();

    return modelNode().variantProperty("extend").value().toString();
}

void QmlModelState::setExtend(const QString &name)
{
    if ((!isBaseState()) && (modelNode().isValid()))
        modelNode().variantProperty("extend").setValue(name);
}

bool QmlModelState::hasExtend() const
{
    if (!isBaseState() && modelNode().isValid())
        return modelNode().hasVariantProperty("extend");

    return false;
}

QmlModelState QmlModelState::createBaseState(const AbstractView *view)
{
    QmlModelState qmlModelState(view->rootModelNode());

    return qmlModelState;
}

} // QmlDesigner
