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

#include "qmltimelinekeyframegroup.h"
#include "abstractview.h"
#include "bindingproperty.h"
#include "qmlitemnode.h"
#include <invalidmodelnodeexception.h>
#include <metainfo.h>
#include <nodelistproperty.h>
#include <variantproperty.h>

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
    return modelNode.isValid() && modelNode.metaInfo().isValid()
           && modelNode.metaInfo().isSubclassOf("QtQuick.Timeline.KeyframeGroup");
}

void QmlTimelineKeyframeGroup::destroy()
{
    Q_ASSERT(isValid());
    modelNode().destroy();
}

ModelNode QmlTimelineKeyframeGroup::target() const
{
    if (modelNode().property("target").isBindingProperty())
        return modelNode().bindingProperty("target").resolveToModelNode();
    else
        return ModelNode(); //exception?
}

void QmlTimelineKeyframeGroup::setTarget(const ModelNode &target)
{
    QTC_ASSERT(isValid(), return );

    modelNode().bindingProperty("target").setExpression(target.id());
}

PropertyName QmlTimelineKeyframeGroup::propertyName() const
{
    QTC_ASSERT(isValid(), return {});

    return modelNode().variantProperty("property").value().toString().toUtf8();
}

void QmlTimelineKeyframeGroup::setPropertyName(const PropertyName &propertyName)
{
    QTC_ASSERT(isValid(), return );

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
    QTC_ASSERT(isValid(), return -1);

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
    QTC_ASSERT(isValid(), return false);

    return modelNode().hasAuxiliaryData("Record@Internal");
}

void QmlTimelineKeyframeGroup::toogleRecording(bool record) const
{
    QTC_ASSERT(isValid(), return );

    if (!record) {
        if (isRecording())
            modelNode().removeAuxiliaryData("Record@Internal");
    } else {
        modelNode().setAuxiliaryData("Record@Internal", true);
    }
}

QmlTimeline QmlTimelineKeyframeGroup::timeline() const
{
    QTC_ASSERT(isValid(), return {});

    if (modelNode().hasParentProperty())
        return modelNode().parentProperty().parentModelNode();

    return {};
}

void QmlTimelineKeyframeGroup::setValue(const QVariant &value, qreal currentFrame)
{
    QTC_ASSERT(isValid(), return );

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
    QTC_ASSERT(isValid(), return {});

    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (qFuzzyCompare(childNode.variantProperty("frame").value().toReal(), frame)) {
            return childNode.variantProperty("value").value();
        }
    }

    return QVariant();
}

TypeName QmlTimelineKeyframeGroup::valueType() const
{
    QTC_ASSERT(isValid(), return {});

    const ModelNode targetNode = target();

    if (targetNode.isValid() && targetNode.hasMetaInfo())
        return targetNode.metaInfo().propertyTypeName(propertyName());

    return TypeName();
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
    QTC_ASSERT(isValid(), return -1);

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
    QTC_ASSERT(isValid(), return -1);

    qreal max = std::numeric_limits<double>::min();
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        QVariant value = childNode.variantProperty("frame").value();
        if (value.isValid() && value.toReal() > max)
            max = value.toReal();
    }

    return max;
}

const QList<ModelNode> QmlTimelineKeyframeGroup::keyframes() const
{
    return modelNode().defaultNodeListProperty().toModelNodeList();
}

const QList<ModelNode> QmlTimelineKeyframeGroup::keyframePositions() const
{
    QList<ModelNode> returnValues;
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        QVariant value = childNode.variantProperty("frame").value();
        if (value.isValid())
            returnValues.append(childNode);
    }

    return returnValues;
}

bool QmlTimelineKeyframeGroup::isValidKeyframe(const ModelNode &node)
{
    return isValidQmlModelNodeFacade(node) && node.metaInfo().isValid()
           && node.metaInfo().isSubclassOf("QtQuick.Timeline.Keyframe");
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
