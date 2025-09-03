// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltimeline.h"
#include "abstractview.h"
#include "bindingproperty.h"
#include "qmlitemnode.h"
#include "qmltimelinekeyframegroup.h"

#include <auxiliarydataproperties.h>
#include <nodelistproperty.h>
#include <variantproperty.h>

#include <utils/qtcassert.h>

#include <limits>

namespace QmlDesigner {

using NanotraceHR::keyValue;

using ModelTracing::category;

bool QmlTimeline::isValid(SL sl) const
{
    return isValidQmlTimeline(modelNode(), sl);
}

bool QmlTimeline::isValidQmlTimeline(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline is valid",
                               category(),
                               keyValue("model node", modelNode),
                               keyValue("caller location", sl)};

    return isValidQmlModelNodeFacade(modelNode) && modelNode.metaInfo().isQtQuickTimelineTimeline();
}

void QmlTimeline::destroy(SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline destroy",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};
    modelNode().destroy();
}

QmlTimelineKeyframeGroup QmlTimeline::keyframeGroup(const ModelNode &node,
                                                    PropertyNameView propertyName,
                                                    SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline keyframe group",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target node", node),
                               keyValue("property name", propertyName),
                               keyValue("caller location", sl)};

    if (isValid()) {
        addKeyframeGroupIfNotExists(node, propertyName);
        for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
            if (QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(childNode)) {
                const QmlTimelineKeyframeGroup frames(childNode);

                if (frames.target().isValid()
                        && frames.target() == node
                        && frames.propertyName() == propertyName)
                    return frames;
            }
        }
    }

    return QmlTimelineKeyframeGroup(); //not found
}

bool QmlTimeline::hasTimeline(const ModelNode &node, PropertyNameView propertyName, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline has timeline",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target node", node),
                               keyValue("property name", propertyName),
                               keyValue("caller location", sl)};

    if (isValid()) {
        for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
            if (QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(childNode)) {
                const QmlTimelineKeyframeGroup frames(childNode);
                if (frames.target().isValid() && frames.target() == node
                        && (frames.propertyName() == propertyName
                            || (frames.propertyName().contains('.')
                                && frames.propertyName().startsWith(propertyName)))) {
                    return true;
                }
            }
        }
    }
    return false;
}

qreal QmlTimeline::startKeyframe(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline start keyframe",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (isValid())
        return QmlObjectNode(modelNode()).modelValue("startFrame").toReal();
    return 0;
}

qreal QmlTimeline::endKeyframe(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline end keyframe",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (isValid())
        return QmlObjectNode(modelNode()).modelValue("endFrame").toReal();
    return 0;
}

qreal QmlTimeline::currentKeyframe(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline current keyframe",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (isValid())
        return QmlObjectNode(modelNode()).instanceValue("currentFrame").toReal();
    return 0;
}

qreal QmlTimeline::duration(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline duration",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return endKeyframe() - startKeyframe();
}

bool QmlTimeline::isEnabled(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline is enabled",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    return QmlObjectNode(modelNode()).modelValue("enabled").toBool();
}

qreal QmlTimeline::minActualKeyframe(const ModelNode &target, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline min actual keyframe",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target node", target),
                               keyValue("caller location", sl)};

    qreal min = std::numeric_limits<double>::max();

    for (const QmlTimelineKeyframeGroup &frames : keyframeGroupsForTarget(target)) {
        qreal value = frames.minActualKeyframe();
        if (value < min)
            min = value;
    }

    return min;
}

qreal QmlTimeline::maxActualKeyframe(const ModelNode &target, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline max actual keyframe",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target node", target),
                               keyValue("caller location", sl)};

    qreal max = std::numeric_limits<double>::min();

    for (const QmlTimelineKeyframeGroup &frames : keyframeGroupsForTarget(target)) {
        qreal value = frames.maxActualKeyframe();
        if (value > max)
            max = value;
    }

    return max;
}

void QmlTimeline::moveAllKeyframes(const ModelNode &target, qreal offset, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline move all keyframes",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target node", target),
                               keyValue("offset", offset),
                               keyValue("caller location", sl)};

    for (QmlTimelineKeyframeGroup &frames : keyframeGroupsForTarget(target))
        frames.moveAllKeyframes(offset);

}

void QmlTimeline::scaleAllKeyframes(const ModelNode &target, qreal factor, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline scale all keyframes",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target node", target),
                               keyValue("factor", factor),
                               keyValue("caller location", sl)};

    for (QmlTimelineKeyframeGroup &frames : keyframeGroupsForTarget(target))
        frames.scaleAllKeyframes(factor);
}

QList<ModelNode> QmlTimeline::allTargets(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline all targets",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};
    QList<ModelNode> result;
    if (isValid()) {
        for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
            if (QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(childNode)) {
                const QmlTimelineKeyframeGroup frames(childNode);
                if (!result.contains(frames.target()))
                    result.append(frames.target());
            }
        }
    }
    return result;
}

QList<QmlTimelineKeyframeGroup> QmlTimeline::keyframeGroupsForTarget(const ModelNode &target, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline keyframe groups for target",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target node", target),
                               keyValue("caller location", sl)};

    QList<QmlTimelineKeyframeGroup> result;
    if (isValid()) {
        for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
            if (QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(childNode)) {
                const QmlTimelineKeyframeGroup frames(childNode);
                if (frames.target() == target)
                    result.append(frames);
            }
        }
    }
     return result;
}

void QmlTimeline::destroyKeyframesForTarget(const ModelNode &target, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline destroy keyframes for target",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target node", target),
                               keyValue("caller location", sl)};

    for (QmlTimelineKeyframeGroup frames : keyframeGroupsForTarget(target))
        frames.destroy();
}

void QmlTimeline::removeKeyframesForTargetAndProperty(const ModelNode &target,
                                                      PropertyNameView propertyName,
                                                      SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline remove keyframes for target and property",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target node", target),
                               keyValue("property name", propertyName),
                               keyValue("caller location", sl)};

    for (QmlTimelineKeyframeGroup frames : keyframeGroupsForTarget(target)) {
        if (frames.propertyName() == propertyName)
            frames.destroy();
    }
}

bool QmlTimeline::hasActiveTimeline(AbstractView *view, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline has active timeline",
                               category(),
                               keyValue("view", view),
                               keyValue("caller location", sl)};

    if (view && view->isAttached()) {
        if (!view->model()->hasImport(Import::createLibraryImport("QtQuick.Timeline", "1.0"), true, true))
            return false;

        return isValidQmlTimeline(view->currentTimelineNode());
    }

    return false;
}

bool QmlTimeline::isRecording(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline is recording",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    QTC_ASSERT(isValid(), return false);

    return modelNode().hasAuxiliaryData(recordProperty);
}

void QmlTimeline::toogleRecording(bool record, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline toggle recording",
                               category(),
                               keyValue("model node", *this),
                               keyValue("recording", record),
                               keyValue("caller location", sl)};

    if (!isValid())
        return;

    if (!record) {
        if (isRecording())
            modelNode().removeAuxiliaryData(recordProperty);
    } else {
        modelNode().setAuxiliaryData(recordProperty, true);
    }
}

void QmlTimeline::resetGroupRecording(SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline reset group recording",
                               category(),
                               keyValue("model node", *this),
                               keyValue("caller location", sl)};

    if (!isValid())
        return;

    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(childNode)) {
            const QmlTimelineKeyframeGroup frames(childNode);
            frames.toogleRecording(false);
        }
    }
}

void QmlTimeline::addKeyframeGroupIfNotExists(const ModelNode &node, PropertyNameView propertyName)
{
    if (!isValid())
        return;

    if (!hasKeyframeGroup(node, propertyName)) {
        ModelNode frames = modelNode().view()->createModelNode("KeyframeGroup", 1, 0);

        modelNode().defaultNodeListProperty().reparentHere(frames);

        QmlTimelineKeyframeGroup(frames).setTarget(node);
        QmlTimelineKeyframeGroup(frames).setPropertyName(propertyName);

        Q_ASSERT(QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(frames));
    }
}

bool QmlTimeline::hasKeyframeGroup(const ModelNode &node, PropertyNameView propertyName, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline has keyframe group",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target node", node),
                               keyValue("property name", propertyName),
                               keyValue("caller location", sl)};

    for (const QmlTimelineKeyframeGroup &frames : allKeyframeGroups()) {
        if (frames.target().isValid()
                && frames.target() == node
                && frames.propertyName() == propertyName)
            return true;
    }

    return false;
}

bool QmlTimeline::hasKeyframeGroupForTarget(const ModelNode &node, SL sl) const
{
    NanotraceHR::Tracer tracer{"qml timeline has keyframe group for target",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target node", node),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    for (const QmlTimelineKeyframeGroup &frames : allKeyframeGroups()) {
        if (frames.target().isValid() && frames.target() == node)
            return true;
    }

    return false;
}

void QmlTimeline::insertKeyframe(const ModelNode &target, PropertyNameView propertyName, SL sl)
{
    NanotraceHR::Tracer tracer{"qml timeline insert keyframe",
                               category(),
                               keyValue("model node", *this),
                               keyValue("target node", target),
                               keyValue("property name", propertyName),
                               keyValue("caller location", sl)};

    ModelNode targetNode = target;
    QmlTimelineKeyframeGroup timelineFrames(keyframeGroup(targetNode, propertyName));

    QTC_ASSERT(timelineFrames.isValid(), return );

    const qreal frame = modelNode().auxiliaryDataWithDefault(currentFrameProperty).toReal();
    const QVariant value = QmlObjectNode(targetNode).instanceValue(propertyName);

    timelineFrames.setValue(value, frame);
}

QList<QmlTimelineKeyframeGroup> QmlTimeline::allKeyframeGroups() const
{
    QList<QmlTimelineKeyframeGroup> returnList;

    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(childNode))
            returnList.emplace_back(childNode);
    }

    return returnList;
}

} // QmlDesigner
