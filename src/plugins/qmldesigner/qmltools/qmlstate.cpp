// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlstate.h"
#include "abstractview.h"
#include <nodelistproperty.h>
#include <variantproperty.h>
#include "bindingproperty.h"
#include "qmlchangeset.h"
#include "qmlitemnode.h"
#include "annotation.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

using NanotraceHR::keyValue;

using ModelTracing::category;

QmlPropertyChanges QmlModelState::ensurePropertyChangesForTarget(const ModelNode &node, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model state ensure property changes for target",
                               category(),
                               keyValue("model node", *this),
                               keyValue("node", node),
                               keyValue("caller location", sl)};

    if (!isBaseState() && isValid()) {
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

QmlPropertyChanges QmlModelState::propertyChangesForTarget(const ModelNode &node, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model state property changes for target",
                               category(),
                               keyValue("model node", *this),
                               keyValue("node", node),
                               keyValue("caller location", sl)};

    if (!isBaseState() && isValid()) {
        const QList<ModelNode> nodes = modelNode().nodeListProperty("changes").toModelNodeList();
        for (const ModelNode &childNode : nodes) {
            if (QmlPropertyChanges::isValidQmlPropertyChanges(childNode)
                && QmlPropertyChanges(childNode).target().isValid()
                && QmlPropertyChanges(childNode).target() == node)
                return QmlPropertyChanges(childNode); //### exception if not valid(childNode);
        }
    }

    return QmlPropertyChanges(); //not found
}

QList<QmlModelStateOperation> QmlModelState::stateOperations(const ModelNode &node, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state state operations",
                               category(),
                               keyValue("model node", *this),
                               keyValue("node", node),
                               keyValue("caller location", sl)};

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

QList<QmlPropertyChanges> QmlModelState::propertyChanges(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state property changes",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    QList<QmlPropertyChanges> returnList;

    if (!isBaseState() && modelNode().hasNodeListProperty("changes")) {
        const QList<ModelNode> nodes = modelNode().nodeListProperty("changes").toModelNodeList();
        for (const ModelNode &childNode : nodes) {
            //### exception if not valid QmlModelStateOperation
            if (QmlPropertyChanges::isValidQmlPropertyChanges(childNode))
                returnList.emplace_back(childNode);
        }
    }

    return returnList;
}

bool QmlModelState::hasPropertyChanges(const ModelNode &node, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state has property changes",
                               category(),
                               keyValue("model node", *this),
                               keyValue("node", node),
                               keyValue("caller location", sl)};

    if (!isBaseState() && modelNode().hasNodeListProperty("changes")) {
        const QList<QmlPropertyChanges> changes = propertyChanges();
        for (const QmlPropertyChanges &changeSet : changes) {
            if (changeSet.target().isValid() && changeSet.target() == node)
                return true;
        }
    }

    return false;
}

bool QmlModelState::hasStateOperation(const ModelNode &node, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state has state operation",
                               category(),
                               keyValue("model node", *this),
                               keyValue("node", node),
                               keyValue("caller location", sl)};

    if (!isBaseState()) {
        const  QList<QmlModelStateOperation> operations = stateOperations();
        for (const  QmlModelStateOperation &stateOperation : operations) {
            if (stateOperation.target() == node)
                return true;
        }
    }
    return false;
}

QList<QmlModelStateOperation> QmlModelState::stateOperations(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state state operations",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    //### exception if not valid
    QList<QmlModelStateOperation> returnList;

    if (!isBaseState() && modelNode().hasNodeListProperty("changes")) {
        const QList<ModelNode> nodes = modelNode().nodeListProperty("changes").toModelNodeList();
        for (const ModelNode &childNode : nodes) {
            //### exception if not valid QmlModelStateOperation
            if (QmlModelStateOperation::isValidQmlModelStateOperation(childNode))
                returnList.emplace_back(childNode);
        }
    }

    return returnList;
}

QList<QmlModelStateOperation> QmlModelState::allInvalidStateOperations(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state all invalid state operations",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

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
    if (!hasPropertyChanges(node)) {
        ModelNode newChangeSet;

#ifdef QDS_USE_PROJECTSTORAGE
        newChangeSet = modelNode().view()->createModelNode("PropertyChanges");
#else
        const QByteArray typeName = "QtQuick.PropertyChanges";
        NodeMetaInfo metaInfo = modelNode().model()->metaInfo(typeName);

        int major = metaInfo.majorVersion();
        int minor = metaInfo.minorVersion();

        newChangeSet = modelNode().view()->createModelNode(typeName, major, minor);
#endif

        modelNode().nodeListProperty("changes").reparentHere(newChangeSet);

        QmlPropertyChanges(newChangeSet).setTarget(node);
        Q_ASSERT(QmlPropertyChanges::isValidQmlPropertyChanges(newChangeSet));
    }
}

void QmlModelState::removePropertyChanges(const ModelNode &node, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model state remove property changes",
                               category(),
                               keyValue("model node", *this),
                               keyValue("node", node),
                               keyValue("caller location", sl)};

    if (!isValid())
        return;

    if (!isBaseState()) {
        QmlPropertyChanges changeSet(propertyChangesForTarget(node));
        if (changeSet.isValid())
            changeSet.modelNode().destroy();
    }
}



/*!
     Returns \c true if this state affects \a node.
*/
bool QmlModelState::affectsModelNode(const ModelNode &node, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state affects model node",
                               category(),
                               keyValue("model node", *this),
                               keyValue("node", node),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    if (isBaseState())
        return false;

    return !stateOperations(node).isEmpty();
}

QList<QmlObjectNode> QmlModelState::allAffectedNodes(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state all affected nodes",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    QList<QmlObjectNode> returnList;

    const QList<ModelNode> nodes = modelNode().nodeListProperty("changes").toModelNodeList();
    for (const ModelNode &childNode : nodes) {
        if (QmlModelStateOperation::isValidQmlModelStateOperation(childNode) &&
            !returnList.contains(QmlModelStateOperation(childNode).target()))
            returnList.emplace_back(QmlModelStateOperation(childNode).target());
    }

    return returnList;
}

QString QmlModelState::name(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state name",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (isBaseState())
        return QString();

    return modelNode().variantProperty("name").value().toString();
}

void QmlModelState::setName(const QString &name, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model state set name",
                               category(),
                               keyValue("model node", *this),
                               keyValue("name", name),
                               keyValue("caller location", sl)};

    if ((!isBaseState()) && (modelNode().isValid()))
        modelNode().variantProperty("name").setValue(name);
}

bool QmlModelState::isValid(SL sl) const
{
    return isValidQmlModelState(modelNode(), sl);
}

bool QmlModelState::isValidQmlModelState(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"is valid qml model state",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("caller location", sl)};

    return isValidQmlModelNodeFacade(modelNode)
           && (isBaseState(modelNode) || modelNode.metaInfo().isQtQuickState());
}

/**
  Removes state node & all subnodes.
  */
void QmlModelState::destroy(SL sl)
{
    NanotraceHR::Tracer tracer{"qml model state destroy",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    modelNode().destroy();
}

/*!
    Returns \c true if this state is the base state.
*/

bool QmlModelState::isBaseState(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state is base state",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return isBaseState(modelNode());
}

bool QmlModelState::isBaseState(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"is base state",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("caller location", sl)};

    return !modelNode.isValid() || modelNode.isRootNode();
}

QmlModelState QmlModelState::duplicate(const QString &name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state duplicate",
                               category(),
                               keyValue("model node", *this),
                               keyValue("name", name),
                               keyValue("caller location", sl)};

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

QmlModelStateGroup QmlModelState::stateGroup(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state state group",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    QmlObjectNode parentNode(modelNode().parentProperty().parentModelNode());
    return parentNode.states();
}

ModelNode QmlModelState::createQmlState(AbstractView *view, const PropertyListType &propertyList, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model state create",
                               category(),
                               keyValue("view", view),
                               keyValue("caller location", sl)};

    if (!view)
        return {};

#ifdef QDS_USE_PROJECTSTORAGE
    return view->createModelNode("State", propertyList);
#else
    const QByteArray typeName = "QtQuick.State";
    NodeMetaInfo metaInfo = view->model()->metaInfo(typeName);

    int major = metaInfo.majorVersion();
    int minor = metaInfo.minorVersion();

    return view->createModelNode(typeName, major, minor, propertyList);
#endif
}

void QmlModelState::setAsDefault(SL sl)
{
    NanotraceHR::Tracer tracer{"qml model state set as default",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if ((!isBaseState()) && (modelNode().isValid())) {
        stateGroup().modelNode().variantProperty("state").setValue(name());
    }
}

bool QmlModelState::isDefault(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state is default",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if ((!isBaseState()) && (modelNode().isValid())) {
        if (stateGroup().modelNode().hasProperty("state")) {
            return (stateGroup().modelNode().variantProperty("state").value() == name());
        }
    }

    return false;
}

void QmlModelState::setAnnotation(const Annotation &annotation, const QString &id, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model state set annotation",
                               category(),
                               keyValue("model node", *this),
                               keyValue("id", id),
                               keyValue("caller location", sl)};

    if (modelNode().isValid()) {
        modelNode().setCustomId(id);
        modelNode().setAnnotation(annotation);
    }
}

Annotation QmlModelState::annotation(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state annotation",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (modelNode().isValid())
        return modelNode().annotation();
    return {};
}

QString QmlModelState::annotationName(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state annotation name",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (modelNode().isValid())
        return modelNode().customId();
    return {};
}

bool QmlModelState::hasAnnotation(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state has annotation",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (modelNode().isValid())
        return modelNode().hasAnnotation() || modelNode().hasCustomId();
    return false;
}

void QmlModelState::removeAnnotation(SL sl)
{
    NanotraceHR::Tracer tracer{"qml model state remove annotation",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (modelNode().isValid()) {
        modelNode().removeCustomId();
        modelNode().removeAnnotation();
    }
}

QString QmlModelState::extend(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state extend",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (isBaseState())
        return QString();

    return modelNode().variantProperty("extend").value().toString();
}

void QmlModelState::setExtend(const QString &name, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model state set extend",
                               category(),
                               keyValue("model node", *this),
                               keyValue("name", name),
                               keyValue("caller location", sl)};

    if ((!isBaseState()) && (modelNode().isValid()))
        modelNode().variantProperty("extend").setValue(name);
}

bool QmlModelState::hasExtend(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model state has extend",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isBaseState() && modelNode().isValid())
        return modelNode().hasVariantProperty("extend");

    return false;
}

QmlModelState QmlModelState::createBaseState(const AbstractView *view, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model state create base state",
                               category(),
                               keyValue("view", view),
                               keyValue("caller location", sl)};

    QmlModelState qmlModelState(view->rootModelNode());

    return qmlModelState;
}

} // QmlDesigner
