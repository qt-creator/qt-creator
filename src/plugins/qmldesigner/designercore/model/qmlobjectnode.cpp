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
#include "stringutils.h"
#include "variantproperty.h"

#include <auxiliarydataproperties.h>
#include <designersettings.h>
#include <invalidmodelnodeexception.h>

#include <qmltimeline.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

namespace QmlDesigner {

void QmlObjectNode::setVariantProperty(const PropertyName &name, const QVariant &value)
{
    if (!isValid())
        return;

    if (metaInfo().isQtQuick3DNode() && !Qml3DNode(modelNode()).handleEulerRotation(name))
        return;

    if (timelineIsActive() && currentTimeline().isRecording()) {
        modelNode().validId();

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
        modelNode().validId();

        QmlPropertyChanges changeSet(currentState().propertyChanges(modelNode()));
        Q_ASSERT(changeSet.isValid());
        changeSet.modelNode().variantProperty(name).setValue(value);
    }
}

void QmlObjectNode::setBindingProperty(const PropertyName &name, const QString &expression)
{
    if (!isValid())
        return;

    if (metaInfo().isQtQuick3DNode() && !Qml3DNode(modelNode()).handleEulerRotation(name))
        return;

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

QmlTimeline QmlObjectNode::currentTimeline() const
{
    if (isValid())
        return view()->currentTimeline();
    else
        return QmlTimeline();
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
        return false;

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
        return false;

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
        return false;

    if (currentState().isBaseState())
        return modelNode().hasProperty(name);

    if (timelineIsActive() && currentTimeline().hasTimeline(modelNode(), name))
        return true;

    if (!currentState().hasPropertyChanges(modelNode()))
        return false;

    return currentState().propertyChanges(modelNode()).modelNode().hasProperty(name);
}

QVariant QmlObjectNode::modelValue(const PropertyName &name) const
{
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

    QmlPropertyChanges propertyChanges(currentState().propertyChanges(modelNode()));

    if (!propertyChanges.modelNode().hasProperty(name))
        return modelNode().variantProperty(name).value();

    return propertyChanges.modelNode().variantProperty(name).value();
}

bool QmlObjectNode::isTranslatableText(const PropertyName &name) const
{
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

QString QmlObjectNode::stripedTranslatableText(const PropertyName &name) const
{
    if (modelNode().hasBindingProperty(name)) {
        static QRegularExpression regularExpressionPattern(
                    QLatin1String("^qsTr(|Id|anslate)\\(\"(.*)\"\\)$"));
        const QRegularExpressionMatch match = regularExpressionPattern.match(
                    modelNode().bindingProperty(name).expression());
        if (match.hasMatch())
            return deescape(match.captured(2));
        return instanceValue(name).toString();
    }
    return instanceValue(name).toString();
}

BindingProperty QmlObjectNode::bindingProperty(const PropertyName &name) const
{
    if (!isValid())
        return {};

    if (currentState().isBaseState())
        return modelNode().bindingProperty(name);

    if (!currentState().hasPropertyChanges(modelNode()))
        return modelNode().bindingProperty(name);

    QmlPropertyChanges propertyChanges(currentState().propertyChanges(modelNode()));

    if (!propertyChanges.modelNode().hasProperty(name))
        return modelNode().bindingProperty(name);

    return propertyChanges.modelNode().bindingProperty(name);
}

QString QmlObjectNode::expression(const PropertyName &name) const
{
    return bindingProperty(name).expression();
}

/*!
    Returns \c true if the ObjectNode is in the BaseState.
*/
bool QmlObjectNode::isInBaseState() const
{
    return currentState().isBaseState();
}

bool QmlObjectNode::timelineIsActive() const
{
    return currentTimeline().isValid();
}

bool QmlObjectNode::instanceCanReparent() const
{
    if (auto qmlItemNode = QmlItemNode{modelNode()})
        return qmlItemNode.instanceCanReparent();
    else
        return isInBaseState();
}

QmlPropertyChanges QmlObjectNode::propertyChangeForCurrentState() const
{
    if (!isValid())
        return {};

    if (currentState().isBaseState())
        return QmlPropertyChanges();

    if (!currentState().hasPropertyChanges(modelNode()))
        return QmlPropertyChanges();

    return currentState().propertyChanges(modelNode());
}

/*!
    Deletes this object's node and its dependencies from the model.
    Everything that belongs to this Object, the ModelNode, and ChangeOperations
    is deleted from the model.
*/
void QmlObjectNode::destroy()
{
    modelNode().destroy();
}

void QmlObjectNode::ensureAliasExport()
{
    if (!isValid())
        return;

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

QList<QmlModelStateOperation> QmlObjectNode::allAffectingStatesOperations() const
{
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

QList<QmlModelState> QmlObjectNode::allDefinedStates() const
{
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

QList<QmlModelStateOperation> QmlObjectNode::allInvalidStateOperations() const
{
    QList<QmlModelStateOperation> result;

    const auto allStates =  allDefinedStates();
    for (const auto &state : allStates)
        result.append(state.allInvalidStateOperations());
    return result;
}

QmlModelStateGroup QmlObjectNode::states() const
{
    if (isValid())
        return QmlModelStateGroup(modelNode());
    else
        return QmlModelStateGroup();
}

QList<ModelNode> QmlObjectNode::allTimelines() const
{
    QList<ModelNode> timelineNodes;
    const auto allNodes = view()->allModelNodes();
    for (const auto &timelineNode : allNodes) {
        if (QmlTimeline::isValidQmlTimeline(timelineNode))
            timelineNodes.append(timelineNode);
    }

    return timelineNodes;
}

QList<ModelNode> QmlObjectNode::getAllConnections() const
{
    if (!isValid())
        return {};

    auto list = view()->allModelNodesOfType(model()->qtQuickConnectionsMetaInfo());
    return Utils::filtered(list, [this](const ModelNode &connection) {
        return connection.hasBindingProperty("target")
               && connection.bindingProperty("target").resolveToModelNode() == modelNode();
    });
}

/*!
    Removes a variant property of the object specified by \a name from the
    model.
*/

void  QmlObjectNode::removeProperty(const PropertyName &name)
{
    if (!isValid())
        return;

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

bool QmlObjectNode::isAncestorOf(const QmlObjectNode &objectNode) const
{
    return modelNode().isAncestorOf(objectNode.modelNode());
}

QVariant QmlObjectNode::instanceValue(const ModelNode &modelNode, const PropertyName &name)
{
    Q_ASSERT(modelNode.view()->nodeInstanceView()->hasInstanceForModelNode(modelNode));
    return modelNode.view()->nodeInstanceView()->instanceForModelNode(modelNode).property(name);
}

QString QmlObjectNode::generateTranslatableText([[maybe_unused]] const QString &text,
                                                const DesignerSettings &settings)
{
    const QString escapedText = escape(text);

    if (settings.value(DesignerSettingsKey::TYPE_OF_QSTR_FUNCTION).toInt())
        switch (settings.value(DesignerSettingsKey::TYPE_OF_QSTR_FUNCTION).toInt()) {
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

QString QmlObjectNode::stripedTranslatableTextFunction(const QString &text)
{
    const QRegularExpression regularExpressionPattern(
                QLatin1String("^qsTr(|Id|anslate)\\(\"(.*)\"\\)$"));
    const QRegularExpressionMatch match = regularExpressionPattern.match(text);
    if (match.hasMatch())
        return deescape(match.captured(2));
    return text;
}

QString QmlObjectNode::convertToCorrectTranslatableFunction(const QString &text,
                                                            const DesignerSettings &designerSettings)
{
    return generateTranslatableText(stripedTranslatableTextFunction(text), designerSettings);
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
    return isValid()
           && nodeInstance().parentId() >= 0
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

QmlItemNode QmlObjectNode::modelParentItem() const
{
    return modelNode().parentProperty().parentModelNode();
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

QmlVisualNode QmlObjectNode::toQmlVisualNode() const
{
     return QmlVisualNode(modelNode());
}

QString QmlObjectNode::simplifiedTypeName() const
{
    return modelNode().simplifiedTypeName();
}

QStringList QmlObjectNode::allStateNames() const
{
    return nodeInstance().allStateNames();
}

} //QmlDesigner
