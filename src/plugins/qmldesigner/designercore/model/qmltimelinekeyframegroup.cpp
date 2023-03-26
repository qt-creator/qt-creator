// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltimelinekeyframegroup.h"
#include "abstractview.h"
#include "bindingproperty.h"
#include "qmlitemnode.h"

#include <auxiliarydataproperties.h>
#include <invalidmodelnodeexception.h>
#include <metainfo.h>
#include <nodelistproperty.h>
#include <variantproperty.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <cmath>
#include <limits>

namespace QmlDesigner {

QmlTimelineKeyframeGroup::QmlTimelineKeyframeGroup() = default;

QmlTimelineKeyframeGroup::QmlTimelineKeyframeGroup(const ModelNode &modelNode)
    : QmlModelNodeFacade(modelNode)
{}

bool QmlTimelineKeyframeGroup::isValid() const
{
    return isValidQmlTimelineKeyframeGroup(modelNode());
}

bool QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(const ModelNode &modelNode)
{
    return modelNode.isValid() && modelNode.metaInfo().isQtQuickTimelineKeyframeGroup();
}

void QmlTimelineKeyframeGroup::destroy()
{
    modelNode().destroy();
}

ModelNode QmlTimelineKeyframeGroup::target() const
{
    return modelNode().bindingProperty("target").resolveToModelNode();
}

void QmlTimelineKeyframeGroup::setTarget(const ModelNode &target)
{
    ModelNode nonConstTarget = target;

    modelNode().bindingProperty("target").setExpression(nonConstTarget.validId());
}

PropertyName QmlTimelineKeyframeGroup::propertyName() const
{
    return modelNode().variantProperty("property").value().toString().toUtf8();
}

void QmlTimelineKeyframeGroup::setPropertyName(const PropertyName &propertyName)
{
    modelNode().variantProperty("property").setValue(QString::fromUtf8(propertyName));
}

int QmlTimelineKeyframeGroup::getSupposedTargetIndex(qreal newFrame) const
{
    const NodeListProperty nodeListProperty = modelNode().defaultNodeListProperty();
    int i = 0;
    for (const auto &node : nodeListProperty.toModelNodeList()) {
        if (node.hasVariantProperty("frame")) {
            const qreal currentFrame = node.variantProperty("frame").value().toReal();
            if (!qFuzzyCompare(currentFrame, newFrame)) { //Ignore the frame itself
                if (currentFrame > newFrame)
                    return i;
                ++i;
            }
        }
    }

    return nodeListProperty.count();
}

int QmlTimelineKeyframeGroup::indexOfKeyframe(const ModelNode &frame) const
{
    if (!isValid())
        return -1;

    return modelNode().defaultNodeListProperty().indexOf(frame);
}

void QmlTimelineKeyframeGroup::slideKeyframe(int /*sourceIndex*/, int /*targetIndex*/)
{
    /*
    if (targetIndex != sourceIndex)
        modelNode().defaultNodeListProperty().slide(sourceIndex, targetIndex);
    */
}

bool QmlTimelineKeyframeGroup::isRecording() const
{
    if (!isValid())
        return false;

    return modelNode().hasAuxiliaryData(recordProperty);
}

void QmlTimelineKeyframeGroup::toogleRecording(bool record) const
{
    QTC_CHECK(isValid());

    if (!record) {
        if (isRecording())
            modelNode().removeAuxiliaryData(recordProperty);
    } else {
        modelNode().setAuxiliaryData(recordProperty, true);
    }
}

QmlTimeline QmlTimelineKeyframeGroup::timeline() const
{
    QTC_CHECK(isValid());

    return modelNode().parentProperty().parentModelNode();
}

bool QmlTimelineKeyframeGroup::isDangling(const ModelNode &node)
{
    QmlTimelineKeyframeGroup group{node};
    return group.isDangling();
}

bool QmlTimelineKeyframeGroup::isDangling() const
{
    return !target().isValid() || keyframes().isEmpty();
}

void QmlTimelineKeyframeGroup::setValue(const QVariant &value, qreal currentFrame)
{
    if (!isValid())
        return;

    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (qFuzzyCompare(childNode.variantProperty("frame").value().toReal(), currentFrame)) {
            childNode.variantProperty("value").setValue(value);
            return;
        }
    }

    const QList<QPair<PropertyName, QVariant>> propertyPairList{{PropertyName("frame"),
                                                                 QVariant(currentFrame)},
                                                                {PropertyName("value"), value}};

    ModelNode frame = modelNode().view()->createModelNode("QtQuick.Timeline.Keyframe",
                                                          1,
                                                          0,
                                                          propertyPairList);
    NodeListProperty nodeListProperty = modelNode().defaultNodeListProperty();

    const int sourceIndex = nodeListProperty.count();
    const int targetIndex = getSupposedTargetIndex(currentFrame);

    nodeListProperty.reparentHere(frame);

    slideKeyframe(sourceIndex, targetIndex);
}

QVariant QmlTimelineKeyframeGroup::value(qreal frame) const
{
    QTC_CHECK(isValid());

    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (qFuzzyCompare(childNode.variantProperty("frame").value().toReal(), frame)) {
            return childNode.variantProperty("value").value();
        }
    }

    return QVariant();
}

NodeMetaInfo QmlTimelineKeyframeGroup::valueType() const
{
    QTC_CHECK(isValid());

    const ModelNode targetNode = target();

    if (targetNode.isValid() && targetNode.hasMetaInfo())
        return targetNode.metaInfo().property(propertyName()).propertyType();

    return {};
}

bool QmlTimelineKeyframeGroup::hasKeyframe(qreal frame)
{
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (qFuzzyCompare(childNode.variantProperty("frame").value().toReal(), frame))
            return true;
    }

    return false;
}

ModelNode QmlTimelineKeyframeGroup::keyframe(qreal frame) const
{
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (qFuzzyCompare(childNode.variantProperty("frame").value().toReal(), frame))
            return childNode;
    }

    return ModelNode();
}

qreal QmlTimelineKeyframeGroup::minActualKeyframe() const
{
    QTC_CHECK(isValid());

    qreal min = std::numeric_limits<double>::max();
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        QVariant value = childNode.variantProperty("frame").value();
        if (value.isValid() && value.toReal() < min)
            min = value.toReal();
    }

    return min;
}

qreal QmlTimelineKeyframeGroup::maxActualKeyframe() const
{
    QTC_CHECK(isValid());

    qreal max = std::numeric_limits<double>::lowest();
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        QVariant value = childNode.variantProperty("frame").value();
        if (value.isValid() && value.toReal() > max)
            max = value.toReal();
    }

    return max;
}

QList<ModelNode> QmlTimelineKeyframeGroup::keyframes() const
{
    return modelNode().defaultNodeListProperty().toModelNodeList();
}

QList<ModelNode> QmlTimelineKeyframeGroup::keyframePositions() const
{
    return Utils::filtered(modelNode().defaultNodeListProperty().toModelNodeList(), [](auto &&node) {
        return node.variantProperty("frame").value().isValid();
    });
}

bool QmlTimelineKeyframeGroup::isValidKeyframe(const ModelNode &node)
{
    return isValidQmlModelNodeFacade(node) && node.metaInfo().isQtQuickTimelineKeyframe();
}

bool QmlTimelineKeyframeGroup::checkKeyframesType(const ModelNode &node)
{
    return node.isValid() && node.type() == "QtQuick.Timeline.KeyframeGroup";
}

QmlTimelineKeyframeGroup QmlTimelineKeyframeGroup::keyframeGroupForKeyframe(const ModelNode &node)
{
    if (isValidKeyframe(node) && node.hasParentProperty()) {
        const QmlTimelineKeyframeGroup timeline(node.parentProperty().parentModelNode());
        if (timeline.isValid())
            return timeline;
    }

    return QmlTimelineKeyframeGroup();
}

QList<QmlTimelineKeyframeGroup> QmlTimelineKeyframeGroup::allInvalidTimelineKeyframeGroups(
    AbstractView *view)
{
    QTC_CHECK(view);
    QTC_CHECK(view->model());

    if (!view || !view->model())
        return {};

    const auto groups = view->rootModelNode().subModelNodesOfType(
        view->model()->qtQuickTimelineKeyframeGroupMetaInfo());

    return Utils::filteredCast<QList<QmlTimelineKeyframeGroup>>(groups, [](auto &&group) {
        return isDangling(group);
    });
}

void QmlTimelineKeyframeGroup::moveAllKeyframes(qreal offset)
{
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        auto property = childNode.variantProperty("frame");
        if (property.isValid())
            property.setValue(std::round(property.value().toReal() + offset));
    }
}

void QmlTimelineKeyframeGroup::scaleAllKeyframes(qreal factor)
{
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        auto property = childNode.variantProperty("frame");

        if (property.isValid())
            property.setValue(std::round(property.value().toReal() * factor));
    }
}

} // namespace QmlDesigner
