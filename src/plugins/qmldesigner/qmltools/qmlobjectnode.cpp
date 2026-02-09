// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlobjectnode.h"
#include "abstractview.h"
#include "bindingproperty.h"
#include "nodeinstance.h"
#include "nodeinstanceview.h"
#include "nodelistproperty.h"
#include "nodemetainfo.h"
#include "nodeproperty.h"
#include "qml3dnode.h"
#include "qmlitemnode.h"
#include "qmlstate.h"
#include "qmltimelinekeyframegroup.h"
#include "qmlvisualnode.h"
#include "variantproperty.h"

#include <auxiliarydataproperties.h>
#include <designersettings.h>
#include <qmldesignerutils/stringutils.h>

#include <qmltimeline.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

namespace QmlDesigner {

using NanotraceHR::keyValue;

using ModelTracing::category;

void QmlObjectNode::setVariantProperty(PropertyNameView name, const QVariant &value, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model node set variant property",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return;

    if (metaInfo().isQtQuick3DNode() && !Qml3DNode(modelNode()).handleEulerRotation(name))
        return;

    if (timelineIsActive() && currentTimeline().isRecording()) {
        modelNode().ensureIdExists();

        QmlTimelineKeyframeGroup timelineFrames(currentTimeline().keyframeGroup(modelNode(), name));

        Q_ASSERT(timelineFrames.isValid());

        qreal frame = currentTimeline().modelNode().auxiliaryDataWithDefault(currentFrameProperty).toReal();
        timelineFrames.setValue(value, frame);

        return;
    } else if (modelNode().hasId() && timelineIsActive()
               && currentTimeline().hasKeyframeGroup(modelNode(), name)) {
        QmlTimelineKeyframeGroup timelineFrames(currentTimeline().keyframeGroup(modelNode(), name));

        Q_ASSERT(timelineFrames.isValid());

        if (timelineFrames.isRecording()) {
            qreal frame = currentTimeline()
                              .modelNode()
                              .auxiliaryDataWithDefault(currentFrameProperty)
                              .toReal();
            timelineFrames.setValue(value, frame);

            return;
        }
    }

    if (isInBaseState()) {
        modelNode().variantProperty(name).setValue(value); //basestate
    } else {
        modelNode().ensureIdExists();

        QmlPropertyChanges changeSet(currentState().ensurePropertyChangesForTarget(modelNode()));
        Q_ASSERT(changeSet.isValid());
        changeSet.modelNode().variantProperty(name).setValue(value);
    }
}

void QmlObjectNode::setBindingProperty(PropertyNameView name, const QString &expression, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model node set binding property",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return;

    if (metaInfo().isQtQuick3DNode() && !Qml3DNode(modelNode()).handleEulerRotation(name))
        return;

    if (isInBaseState()) {
        modelNode().bindingProperty(name).setExpression(expression); //basestate
    } else {
        modelNode().ensureIdExists();

        QmlPropertyChanges changeSet(currentState().ensurePropertyChangesForTarget(modelNode()));
        Q_ASSERT(changeSet.isValid());
        changeSet.modelNode().bindingProperty(name).setExpression(expression);
    }
}

QmlModelState QmlObjectNode::currentState(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node current state",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (auto view = this->view())
        return QmlModelState(view->currentStateNode());
    else
        return QmlModelState();
}

QmlTimeline QmlObjectNode::currentTimeline(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node current timeline",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (isValid())
        return view()->currentTimelineNode();

    return {};
}

bool QmlObjectNode::isRootModelNode(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node is root model node",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().isRootNode();
}


/*!
    Returns the value of the property specified by \name that is based on an
    actual instance. The return value is not the value in the model, but the
    value of a real instantiated instance of this object.
*/
QVariant QmlObjectNode::instanceValue(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node instance value",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().property(name);
}

bool QmlObjectNode::hasProperty(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node has property",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    if (currentState().hasPropertyChanges(modelNode())) {
        QmlPropertyChanges propertyChanges = currentState().ensurePropertyChangesForTarget(modelNode());
        if (propertyChanges.modelNode().hasProperty(name))
            return true;
    }

    return modelNode().hasProperty(name);
}

bool QmlObjectNode::hasBindingProperty(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node has binding property",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (QmlPropertyChanges propertyChanges = currentState().propertyChangesForTarget(modelNode())) {
        if (propertyChanges.modelNode().hasBindingProperty(name))
            return true;
    }

    return modelNode().hasBindingProperty(name);
}

NodeAbstractProperty QmlObjectNode::nodeAbstractProperty(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node node abstract property",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().nodeAbstractProperty(name);
}

NodeAbstractProperty QmlObjectNode::defaultNodeAbstractProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node default node abstract property",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().defaultNodeAbstractProperty();
}

NodeProperty QmlObjectNode::nodeProperty(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node node property",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().nodeProperty(name);
}

NodeListProperty QmlObjectNode::nodeListProperty(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node node list property",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().nodeListProperty(name);
}

bool QmlObjectNode::instanceHasValue(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node instance has value",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().hasProperty(name);
}

bool QmlObjectNode::propertyAffectedByCurrentState(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node property affected by current state",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    if (currentState().isBaseState())
        return modelNode().hasProperty(name);

    if (timelineIsActive() && currentTimeline().hasTimeline(modelNode(), name))
        return true;

    if (!currentState().hasPropertyChanges(modelNode()))
        return false;

    return currentState().propertyChangesForTarget(modelNode()).modelNode().hasProperty(name);
}

QVariant QmlObjectNode::modelValue(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node model value",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    if (timelineIsActive() && currentTimeline().hasTimeline(modelNode(), name)) {
        QmlTimelineKeyframeGroup timelineFrames(currentTimeline().keyframeGroup(modelNode(), name));

        Q_ASSERT(timelineFrames.isValid());

        qreal frame = currentTimeline().modelNode().auxiliaryDataWithDefault(currentFrameProperty).toReal();

        QVariant value = timelineFrames.value(frame);

        if (!value.isValid()) //interpolation is not done in the model
            value = instanceValue(name);

        return value;
    }

    if (currentState().isBaseState())
        return modelNode().variantProperty(name).value();

    if (!currentState().hasPropertyChanges(modelNode()))
        return modelNode().variantProperty(name).value();

    QmlPropertyChanges propertyChanges(currentState().propertyChangesForTarget(modelNode()));

    if (!propertyChanges.modelNode().hasProperty(name))
        return modelNode().variantProperty(name).value();

    return propertyChanges.modelNode().variantProperty(name).value();
}

bool QmlObjectNode::isTranslatableText(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node is translatable text",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (modelNode().metaInfo().isValid() && modelNode().metaInfo().hasProperty(name)
        && modelNode().metaInfo().property(name).propertyType().isString()) {
        if (modelNode().hasBindingProperty(name)) {
            static QRegularExpression regularExpressionPattern(
                QLatin1String("^qsTr(|Id|anslate)\\(\".*\"\\)$"));
            return modelNode().bindingProperty(name).expression().contains(regularExpressionPattern);
        }

        return false;
    }

    return false;
}

QString QmlObjectNode::stripedTranslatableText(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node stroped translatable text",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (modelNode().hasBindingProperty(name)) {
        static QRegularExpression regularExpressionPattern(
                    QLatin1String("^qsTr(|Id|anslate)\\(\"(.*)\"\\)$"));
        const QRegularExpressionMatch match = regularExpressionPattern.match(
                    modelNode().bindingProperty(name).expression());
        if (match.hasMatch())
            return StringUtils::deescape(match.captured(2));
        return instanceValue(name).toString();
    }
    return instanceValue(name).toString();
}

BindingProperty QmlObjectNode::bindingProperty(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node binding property",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    if (currentState().isBaseState())
        return modelNode().bindingProperty(name);

    if (!currentState().hasPropertyChanges(modelNode()))
        return modelNode().bindingProperty(name);

    QmlPropertyChanges propertyChanges(currentState().propertyChangesForTarget(modelNode()));

    if (!propertyChanges.modelNode().hasProperty(name))
        return modelNode().bindingProperty(name);

    return propertyChanges.modelNode().bindingProperty(name);
}

QString QmlObjectNode::expression(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node expression",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return bindingProperty(name).expression();
}

/*!
    Returns \c true if the ObjectNode is in the BaseState.
*/
bool QmlObjectNode::isInBaseState(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node is in base state",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return currentState().isBaseState();
}

bool QmlObjectNode::timelineIsActive(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node timeline is active",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return currentTimeline().isValid();
}

bool QmlObjectNode::instanceCanReparent(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node instance can reparent",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (auto qmlItemNode = QmlItemNode{modelNode()})
        return qmlItemNode.instanceCanReparent();
    else
        return isInBaseState();
}

QmlPropertyChanges QmlObjectNode::ensurePropertyChangeForCurrentState(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node ensure property change for current state",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    if (currentState().isBaseState())
        return QmlPropertyChanges();

    if (!currentState().hasPropertyChanges(modelNode()))
        return QmlPropertyChanges();

    return currentState().ensurePropertyChangesForTarget(modelNode());
}

QmlPropertyChanges QmlObjectNode::propertyChangeForCurrentState(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node property change for current state",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    if (currentState().isBaseState())
        return QmlPropertyChanges();

    if (!currentState().hasPropertyChanges(modelNode()))
        return QmlPropertyChanges();

    return currentState().propertyChangesForTarget(modelNode());
}

/*!
    Deletes this object's node and its dependencies from the model.
    Everything that belongs to this Object, the ModelNode, and ChangeOperations
    is deleted from the model.
*/
void QmlObjectNode::destroy(SL sl)
{
    NanotraceHR::Tracer tracer{"qml model node destroy",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    modelNode().destroy();
}

void QmlObjectNode::ensureAliasExport(SL sl)
{
    NanotraceHR::Tracer tracer{"qml model node ensure alias export",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return;

    if (!isAliasExported()) {
        modelNode().ensureIdExists();
        ModelNode rootModelNode = view()->rootModelNode();
        rootModelNode.bindingProperty(modelNode().id().toUtf8()).
            setDynamicTypeNameAndExpression("alias", modelNode().id());
    }
}

bool QmlObjectNode::isAliasExported(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node is alias exported",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

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

QList<QmlModelState> QmlObjectNode::allAffectingStates(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node all affecting states",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    QList<QmlModelState> returnList;

    const QList<QmlModelState> states = allDefinedStates();
    for (const QmlModelState &state : states) {
        if (state.affectsModelNode(modelNode()))
            returnList.append(state);
    }
    return returnList;
}

/*!
    Returns a list of all state operations that affect this object.
*/

QList<QmlModelStateOperation> QmlObjectNode::allAffectingStatesOperations(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node all affecting states operations",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    QList<QmlModelStateOperation> returnList;
    const QList<QmlModelState> states = allDefinedStates();
    for (const QmlModelState &state : states) {
        if (state.affectsModelNode(modelNode()))
            returnList.append(state.stateOperations(modelNode()));
    }

    return returnList;
}

static QList<QmlVisualNode> allQmlVisualNodesRecursive(const QmlVisualNode &qmlItemNode)
{
    QList<QmlVisualNode> qmlVisualNodeList;

    if (qmlItemNode.isValid()) {
        qmlVisualNodeList.append(qmlItemNode);

        const QList<ModelNode> nodes = qmlItemNode.modelNode().directSubModelNodes();
        for (const ModelNode &modelNode : nodes) {
            if (QmlVisualNode::isValidQmlVisualNode(modelNode))
                qmlVisualNodeList.append(allQmlVisualNodesRecursive(modelNode));
        }
    }

    return qmlVisualNodeList;
}

QList<QmlModelState> QmlObjectNode::allDefinedStates(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node all defined states",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    QList<QmlModelState> returnList;

    QList<QmlVisualNode> allVisualNodes;

    if (QmlVisualNode::isValidQmlVisualNode(view()->rootModelNode()))
        allVisualNodes.append(allQmlVisualNodesRecursive(view()->rootModelNode()));

    for (const QmlVisualNode &node : std::as_const(allVisualNodes))
        returnList.append(node.states().allStates());

    const auto allNodes = view()->allModelNodes();
    for (const ModelNode &node : allNodes) {
        if (node.simplifiedTypeName() == "StateGroup")
            returnList.append(QmlModelStateGroup(node).allStates());
    }

    return returnList;
}

QList<QmlModelStateOperation> QmlObjectNode::allInvalidStateOperations(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node all invalid state operations",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    QList<QmlModelStateOperation> result;

    const auto allStates =  allDefinedStates();
    for (const auto &state : allStates)
        result.append(state.allInvalidStateOperations());
    return result;
}

QmlModelStateGroup QmlObjectNode::states(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node states",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (isValid())
        return QmlModelStateGroup(modelNode());
    else
        return QmlModelStateGroup();
}

QList<ModelNode> QmlObjectNode::allTimelines(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node all timelines",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    QList<ModelNode> timelineNodes;
    const auto allNodes = view()->allModelNodes();
    for (const auto &timelineNode : allNodes) {
        if (QmlTimeline::isValidQmlTimeline(timelineNode))
            timelineNodes.append(timelineNode);
    }

    return timelineNodes;
}

QList<ModelNode> QmlObjectNode::getAllConnections(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node get all connections",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    auto list = view()->allModelNodesOfType(model()->qtQmlConnectionsMetaInfo());
    return Utils::filtered(list, [this](const ModelNode &connection) {
        return connection.hasBindingProperty("target")
               && connection.bindingProperty("target").resolveToModelNode() == modelNode();
    });
}

/*!
    Removes a variant property of the object specified by \a name from the
    model.
*/

void QmlObjectNode::removeProperty(PropertyNameView name, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model node remove property",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return;

    if (isInBaseState()) {
        modelNode().removeProperty(name); //basestate
    } else {
        QmlPropertyChanges changeSet(currentState().ensurePropertyChangesForTarget(modelNode()));
        Q_ASSERT(changeSet.isValid());
        changeSet.removeProperty(name);
    }
}

QList<ModelNode> toModelNodeList(const QList<QmlObjectNode> &qmlObjectNodeList)
{
    QList<ModelNode> modelNodeList;

    for (const QmlObjectNode &qmlObjectNode : qmlObjectNodeList)
        modelNodeList.append(qmlObjectNode.modelNode());

    return modelNodeList;
}

QList<QmlObjectNode> toQmlObjectNodeList(const QList<ModelNode> &modelNodeList)
{
    QList<QmlObjectNode> qmlObjectNodeList;

    for (const ModelNode &modelNode : modelNodeList) {
        if (QmlObjectNode::isValidQmlObjectNode(modelNode))
             qmlObjectNodeList.append(modelNode);
    }

    return qmlObjectNodeList;
}

bool QmlObjectNode::isAncestorOf(const QmlObjectNode &objectNode, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node is ancestor of",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().isAncestorOf(objectNode.modelNode());
}

QVariant QmlObjectNode::instanceValue(const ModelNode &modelNode, PropertyNameView name, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model node instance value",
                               category(),
                               keyValue("caller location", sl)};

    Q_ASSERT(nodeInstanceView(modelNode)->hasInstanceForModelNode(modelNode));
    return nodeInstanceView(modelNode)->instanceForModelNode(modelNode).property(name);
}

QString QmlObjectNode::generateTranslatableText([[maybe_unused]] const QString &text,
                                                const DesignerSettings &settings,
                                                SL sl)
{
    NanotraceHR::Tracer tracer{"qml model node generate translatable text",
                               category(),
                               keyValue("caller location", sl)};

    const QString escapedText = StringUtils::escape(text);

    switch (settings.typeOfQsTrFunction()) {
        case 0:
             return QStringView(u"qsTr(\"%1\")").arg(escapedText);
        case 1:
             return QStringView(u"qsTrId(\"%1\")").arg(escapedText);
        case 2:
             return QStringView(u"qsTranslate(\"%1\", \"context\")").arg(escapedText);
        default:
            break;
    }
    return QStringView(u"qsTr(\"%1\")").arg(escapedText);
}

QString QmlObjectNode::stripedTranslatableTextFunction(const QString &text, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model node striped translatable text function",
                               category(),
                               keyValue("caller location", sl)};

    const QRegularExpression regularExpressionPattern(
        QLatin1String("^qsTr(|Id|anslate)\\(\"(.*)\"\\)$"));
    const QRegularExpressionMatch match = regularExpressionPattern.match(text);
    if (match.hasMatch())
        return StringUtils::deescape(match.captured(2));
    return text;
}

QString QmlObjectNode::convertToCorrectTranslatableFunction(const QString &text,
                                                            const DesignerSettings &designerSettings,
                                                            SL sl)
{
    NanotraceHR::Tracer tracer{"qml model node convert to correct translatable function",
                               category(),
                               keyValue("caller location", sl)};

    return generateTranslatableText(stripedTranslatableTextFunction(text), designerSettings);
}

TypeName QmlObjectNode::instanceType(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node instance type",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().instanceType(name);
}

bool QmlObjectNode::instanceHasBinding(PropertyNameView name, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node instance has binding",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().hasBindingForProperty(name);
}

const NodeInstance &QmlObjectNode::nodeInstance() const
{
    if (auto view = nodeInstanceView())
        return view->instanceForModelNode(modelNode());

    return NodeInstance::null();
}

QmlObjectNode QmlObjectNode::nodeForInstance(const NodeInstance &instance) const
{
    return QmlObjectNode(ModelNode(instance.modelNode(), view()));
}

QmlItemNode QmlObjectNode::itemForInstance(const NodeInstance &instance) const
{
    return QmlItemNode(ModelNode(instance.modelNode(), view()));
}

bool QmlObjectNode::isValidQmlObjectNode(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"is valid qml object node",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("caller location", sl)};

    return isValidQmlModelNodeFacade(modelNode);
}

bool QmlObjectNode::isValid(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node is valid",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

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

bool QmlObjectNode::hasNodeParent(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node has node parent",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().hasParentProperty();
}

bool QmlObjectNode::hasInstanceParent(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node has instance parent",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().parentId() >= 0
           && nodeInstanceView()->hasInstanceForId(nodeInstance().parentId());
}

bool QmlObjectNode::hasInstanceParentItem(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node has instance parent item",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return isValid() && nodeInstance().parentId() >= 0
           && nodeInstanceView()->hasInstanceForId(nodeInstance().parentId())
           && QmlItemNode::isItemOrWindow(view()->modelNodeForInternalId(nodeInstance().parentId()));
}

void QmlObjectNode::setParentProperty(const NodeAbstractProperty &parentProeprty, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model node set parent property",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().setParentProperty(parentProeprty);
}

QmlObjectNode QmlObjectNode::instanceParent(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node instance parent",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (hasInstanceParent())
        return nodeForInstance(nodeInstanceView()->instanceForId(nodeInstance().parentId()));

    return QmlObjectNode();
}

QmlItemNode QmlObjectNode::instanceParentItem(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node instance parent item",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (hasInstanceParentItem())
        return itemForInstance(nodeInstanceView()->instanceForId(nodeInstance().parentId()));

    return QmlItemNode();
}

QmlItemNode QmlObjectNode::modelParentItem(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node instance parent item",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().parentProperty().parentModelNode();
}

void QmlObjectNode::setId(const QString &id, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model node set id",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    modelNode().setIdWithRefactoring(id);
}

void QmlObjectNode::setNameAndId(const QString &newName, const QString &fallbackId, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model node set name and id",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return;

    VariantProperty objectNameProperty = modelNode().variantProperty("objectName");
    QString oldName = objectNameProperty.value().toString();

    if (oldName != newName) {
        const Model *model = view()->model();
        QTC_ASSERT(model, return);

        view()->executeInTransaction(__FUNCTION__, [&] {
            modelNode().setIdWithRefactoring(model->generateNewId(newName, fallbackId));
            objectNameProperty.setValue(newName);
        });
    }
}

QString QmlObjectNode::id(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node id",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().id();
}

QString QmlObjectNode::validId(SL sl)
{
    NanotraceHR::Tracer tracer{"qml model node valid id",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().validId();
}

bool QmlObjectNode::hasDefaultPropertyName(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node has default property name",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().metaInfo().hasDefaultProperty();
}

PropertyName QmlObjectNode::defaultPropertyName(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node default property name",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().metaInfo().defaultPropertyName();
}

void QmlObjectNode::setParent(const QmlObjectNode &newParent, SL sl)
{
    NanotraceHR::Tracer tracer{"qml model node set parent",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (newParent.hasDefaultPropertyName())
        newParent.modelNode().defaultNodeAbstractProperty().reparentHere(modelNode());
}

QmlItemNode QmlObjectNode::toQmlItemNode(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node to qml item node",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return QmlItemNode(modelNode());
}

QmlVisualNode QmlObjectNode::toQmlVisualNode(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node to qml visual node",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return QmlVisualNode(modelNode());
}

QString QmlObjectNode::simplifiedTypeName(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node simplified type name",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().simplifiedTypeName();
}

QStringList QmlObjectNode::allStateNames(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml model node all state names",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return nodeInstance().allStateNames();
}

} //QmlDesigner
