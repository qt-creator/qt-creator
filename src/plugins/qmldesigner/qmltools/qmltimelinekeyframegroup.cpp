// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltimelinekeyframegroup.h"
#include "abstractview.h"
#include "bindingproperty.h"
#include "qmlitemnode.h"

#include <auxiliarydataproperties.h>
#include <nodelistproperty.h>
#include <variantproperty.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <cmath>
#include <limits>

namespace QmlDesigner {

using NanotraceHR::keyValue;

using ModelTracing::category;

bool QmlTimelineKeyframeGroup::isValid(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group is valid",
                               category(),
                               keyValue("mode node", *this),
                               keyValue("caller location", sl)};

    return isValidQmlTimelineKeyframeGroup(modelNode());
}

bool QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group is valid",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("caller location", sl)};

    return modelNode.isValid() && modelNode.metaInfo().isQtQuickTimelineKeyframeGroup();
}

void QmlTimelineKeyframeGroup::destroy(SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group destroy",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    modelNode().destroy();
}

ModelNode QmlTimelineKeyframeGroup::target(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group target",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().bindingProperty("target").resolveToModelNode();
}

void QmlTimelineKeyframeGroup::setTarget(const ModelNode &target, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group target",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target", target),
                               keyValue("caller location", sl)};

    ModelNode nonConstTarget = target;

    modelNode().bindingProperty("target").setExpression(nonConstTarget.validId());
}

PropertyName QmlTimelineKeyframeGroup::propertyName(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group property name",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().variantProperty("property").value().toString().toUtf8();
}

void QmlTimelineKeyframeGroup::setPropertyName(PropertyNameView propertyName, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group property name",
                               category(),
                               keyValue("model node", *this),
                               keyValue("property name", propertyName),
                               keyValue("caller location", sl)};

    modelNode().variantProperty("property").setValue(QString::fromUtf8(propertyName));
}

int QmlTimelineKeyframeGroup::getSupposedTargetIndex(qreal newFrame, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group target index",
                               category(),
                               keyValue("model node", *this),
                               keyValue("new frame", newFrame),
                               keyValue("caller location", sl)};

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

int QmlTimelineKeyframeGroup::indexOfKeyframe(const ModelNode &frame, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group index of key frame",
                               category(),
                               keyValue("model node", *this),
                               keyValue("frame", frame),
                               keyValue("caller location", sl)};

    if (!isValid())
        return -1;

    return modelNode().defaultNodeListProperty().indexOf(frame);
}

void QmlTimelineKeyframeGroup::slideKeyframe(int /*sourceIndex*/, int /*targetIndex*/, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group slide key frame",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};
    /*
    if (targetIndex != sourceIndex)
        modelNode().defaultNodeListProperty().slide(sourceIndex, targetIndex);
    */
}

bool QmlTimelineKeyframeGroup::isRecording(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group is recording",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};
    if (!isValid())
        return false;

    return modelNode().hasAuxiliaryData(recordProperty);
}

void QmlTimelineKeyframeGroup::toogleRecording(bool record, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group toggle recording",
                               category(),
                               keyValue("model node", *this),
                               keyValue("record", record),
                               keyValue("caller location", sl)};

    QTC_CHECK(isValid());

    if (!record) {
        if (isRecording())
            modelNode().removeAuxiliaryData(recordProperty);
    } else {
        modelNode().setAuxiliaryData(recordProperty, true);
    }
}

QmlTimeline QmlTimelineKeyframeGroup::timeline(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group timeline",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    QTC_CHECK(isValid());

    return modelNode().parentProperty().parentModelNode();
}

bool QmlTimelineKeyframeGroup::isDangling(const ModelNode &node, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group is dangling",
                               category(),
                               keyValue("model node", node),
                               keyValue("caller location", sl)};

    QmlTimelineKeyframeGroup group{node};
    return group.isDangling();
}

bool QmlTimelineKeyframeGroup::isDangling(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group is dangling",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return !target().isValid() || keyframes().isEmpty();
}

void QmlTimelineKeyframeGroup::setValue(const QVariant &value, qreal currentFrame, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group set value",
                               category(),
                               keyValue("model node", *this),
                               keyValue("value", value),
                               keyValue("current frame", currentFrame),
                               keyValue("caller location", sl)};

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

    ModelNode frame = modelNode().view()->createModelNode("Keyframe", 1, 0, propertyPairList);
    NodeListProperty nodeListProperty = modelNode().defaultNodeListProperty();

    const int sourceIndex = nodeListProperty.count();
    const int targetIndex = getSupposedTargetIndex(currentFrame);

    nodeListProperty.reparentHere(frame);

    slideKeyframe(sourceIndex, targetIndex);
}

QVariant QmlTimelineKeyframeGroup::value(qreal frame, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group value",
                               category(),
                               keyValue("model node", *this),
                               keyValue("frame", frame),
                               keyValue("caller location", sl)};

    QTC_CHECK(isValid());

    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (qFuzzyCompare(childNode.variantProperty("frame").value().toReal(), frame)) {
            return childNode.variantProperty("value").value();
        }
    }

    return QVariant();
}

NodeMetaInfo QmlTimelineKeyframeGroup::valueType(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group value type",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    QTC_CHECK(isValid());

    const ModelNode targetNode = target();

    if (targetNode.isValid() && targetNode.hasMetaInfo())
        return targetNode.metaInfo().property(propertyName()).propertyType();

    return {};
}

bool QmlTimelineKeyframeGroup::hasKeyframe(qreal frame, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group has key frame",
                               category(),
                               keyValue("model node", *this),
                               keyValue("frame", frame),
                               keyValue("caller location", sl)};

    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (qFuzzyCompare(childNode.variantProperty("frame").value().toReal(), frame))
            return true;
    }

    return false;
}

ModelNode QmlTimelineKeyframeGroup::keyframe(qreal frame, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group key frame",
                               category(),
                               keyValue("model node", *this),
                               keyValue("frame", frame),
                               keyValue("caller location", sl)};

    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (qFuzzyCompare(childNode.variantProperty("frame").value().toReal(), frame))
            return childNode;
    }

    return ModelNode();
}

qreal QmlTimelineKeyframeGroup::minActualKeyframe(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group min actual key frame",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    QTC_CHECK(isValid());

    qreal min = std::numeric_limits<double>::max();
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        QVariant value = childNode.variantProperty("frame").value();
        if (value.isValid() && value.toReal() < min)
            min = value.toReal();
    }

    return min;
}

qreal QmlTimelineKeyframeGroup::maxActualKeyframe(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group max actual key frame",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    QTC_CHECK(isValid());

    qreal max = std::numeric_limits<double>::lowest();
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        QVariant value = childNode.variantProperty("frame").value();
        if (value.isValid() && value.toReal() > max)
            max = value.toReal();
    }

    return max;
}

QList<ModelNode> QmlTimelineKeyframeGroup::keyframes(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group key frames",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return modelNode().defaultNodeListProperty().toModelNodeList();
}

QList<ModelNode> QmlTimelineKeyframeGroup::keyframePositions(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group key frame positions",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return Utils::filtered(modelNode().defaultNodeListProperty().toModelNodeList(), [](auto &&node) {
        return node.variantProperty("frame").value().isValid();
    });
}

bool QmlTimelineKeyframeGroup::isValidKeyframe(const ModelNode &node, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group is valid key frame",
                               category(),
                               keyValue("model node", node),
                               keyValue("caller location", sl)};

    return isValidQmlModelNodeFacade(node) && node.metaInfo().isQtQuickTimelineKeyframe();
}

bool QmlTimelineKeyframeGroup::checkKeyframesType(const ModelNode &node, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group check keyframes type",
                               category(),
                               keyValue("model node", node),
                               keyValue("caller location", sl)};

    return node.isValid() && node.metaInfo().isQtQuickTimelineKeyframeGroup();
}

QmlTimelineKeyframeGroup QmlTimelineKeyframeGroup::keyframeGroupForKeyframe(const ModelNode &node,
                                                                            SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group for key frame",
                               category(),
                               keyValue("model node", node),
                               keyValue("caller location", sl)};

    if (isValidKeyframe(node) && node.hasParentProperty()) {
        const QmlTimelineKeyframeGroup timeline(node.parentProperty().parentModelNode());
        if (timeline.isValid())
            return timeline;
    }

    return QmlTimelineKeyframeGroup();
}

QList<QmlTimelineKeyframeGroup> QmlTimelineKeyframeGroup::allInvalidTimelineKeyframeGroups(
    AbstractView *view, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group all invalid timeline key frame groups",
                               category(),
                               keyValue("view", view),
                               keyValue("caller location", sl)};

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

void QmlTimelineKeyframeGroup::moveAllKeyframes(qreal offset, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group move all key frames",
                               category(),
                               keyValue("model node", *this),
                               keyValue("offset", offset),
                               keyValue("caller location", sl)};

    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        auto property = childNode.variantProperty("frame");
        if (property.isValid())
            property.setValue(std::round(property.value().toReal() + offset));
    }
}

void QmlTimelineKeyframeGroup::scaleAllKeyframes(qreal factor, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline key frame group scale all key frames",
                               category(),
                               keyValue("model node", *this),
                               keyValue("factor", factor),
                               keyValue("caller location", sl)};

    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        auto property = childNode.variantProperty("frame");

        if (property.isValid())
            property.setValue(std::round(property.value().toReal() * factor));
    }
}

} // namespace QmlDesigner
