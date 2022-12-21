// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltimeline.h"
#include "abstractview.h"
#include "bindingproperty.h"
#include "qmlitemnode.h"
#include "qmltimelinekeyframegroup.h"

#include <auxiliarydataproperties.h>
#include <invalidmodelnodeexception.h>
#include <metainfo.h>
#include <nodelistproperty.h>
#include <variantproperty.h>

#include <utils/qtcassert.h>

#include <limits>

namespace QmlDesigner {

QmlTimeline::QmlTimeline() = default;

QmlTimeline::QmlTimeline(const ModelNode &modelNode) : QmlModelNodeFacade(modelNode)
{

}

bool QmlTimeline::isValid() const
{
    return isValidQmlTimeline(modelNode());
}

bool QmlTimeline::isValidQmlTimeline(const ModelNode &modelNode)
{
    return isValidQmlModelNodeFacade(modelNode) && modelNode.metaInfo().isQtQuickTimelineTimeline();
}

void QmlTimeline::destroy()
{
    Q_ASSERT(isValid());
    modelNode().destroy();
}

QmlTimelineKeyframeGroup QmlTimeline::keyframeGroup(const ModelNode &node, const PropertyName &propertyName)
{
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

bool QmlTimeline::hasTimeline(const ModelNode &node, const PropertyName &propertyName)
{
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

qreal QmlTimeline::startKeyframe() const
{
    if (isValid())
        return QmlObjectNode(modelNode()).modelValue("startFrame").toReal();
    return 0;
}

qreal QmlTimeline::endKeyframe() const
{
    if (isValid())
        return QmlObjectNode(modelNode()).modelValue("endFrame").toReal();
    return 0;
}

qreal QmlTimeline::currentKeyframe() const
{
    if (isValid())
        return QmlObjectNode(modelNode()).instanceValue("currentFrame").toReal();
    return 0;
}

qreal QmlTimeline::duration() const
{
    return endKeyframe() - startKeyframe();
}

bool QmlTimeline::isEnabled() const
{
    return QmlObjectNode(modelNode()).modelValue("enabled").toBool();
}

qreal QmlTimeline::minActualKeyframe(const ModelNode &target) const
{
    qreal min = std::numeric_limits<double>::max();

    for (const QmlTimelineKeyframeGroup &frames : keyframeGroupsForTarget(target)) {
        qreal value = frames.minActualKeyframe();
        if (value < min)
            min = value;
    }

    return min;
}

qreal QmlTimeline::maxActualKeyframe(const ModelNode &target) const
{
    qreal max = std::numeric_limits<double>::min();

    for (const QmlTimelineKeyframeGroup &frames : keyframeGroupsForTarget(target)) {
        qreal value = frames.maxActualKeyframe();
        if (value > max)
            max = value;
    }

    return max;
}

void QmlTimeline::moveAllKeyframes(const ModelNode &target, qreal offset)
{
    for (QmlTimelineKeyframeGroup &frames : keyframeGroupsForTarget(target))
        frames.moveAllKeyframes(offset);

}

void QmlTimeline::scaleAllKeyframes(const ModelNode &target, qreal factor)
{
    for (QmlTimelineKeyframeGroup &frames : keyframeGroupsForTarget(target))
        frames.scaleAllKeyframes(factor);
}

QList<ModelNode> QmlTimeline::allTargets() const
{
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

QList<QmlTimelineKeyframeGroup> QmlTimeline::keyframeGroupsForTarget(const ModelNode &target) const
{
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

void QmlTimeline::destroyKeyframesForTarget(const ModelNode &target)
{
    for (QmlTimelineKeyframeGroup frames : keyframeGroupsForTarget(target))
        frames.destroy();
}

void QmlTimeline::removeKeyframesForTargetAndProperty(const ModelNode &target,
                                                      const PropertyName &propertyName)
{
    for (QmlTimelineKeyframeGroup frames : keyframeGroupsForTarget(target)) {
        if (frames.propertyName() == propertyName)
            frames.destroy();
    }
}

bool QmlTimeline::hasActiveTimeline(AbstractView *view)
{
    if (view && view->isAttached()) {
        if (!view->model()->hasImport(Import::createLibraryImport("QtQuick.Timeline", "1.0"), true, true))
            return false;

        return view->currentTimeline().isValid();
    }

    return false;
}

bool QmlTimeline::isRecording() const
{
    QTC_ASSERT(isValid(), return false);

    return modelNode().hasAuxiliaryData(recordProperty);
}

void QmlTimeline::toogleRecording(bool record) const
{
    QTC_ASSERT(isValid(), return);

    if (!record) {
        if (isRecording())
            modelNode().removeAuxiliaryData(recordProperty);
    } else {
        modelNode().setAuxiliaryData(recordProperty, true);
    }
}

void QmlTimeline::resetGroupRecording() const
{
    QTC_ASSERT(isValid(), return);

    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(childNode)) {
            const QmlTimelineKeyframeGroup frames(childNode);
            frames.toogleRecording(false);
        }
    }
}

void QmlTimeline::addKeyframeGroupIfNotExists(const ModelNode &node, const PropertyName &propertyName)
{
    if (!isValid())
        return;

    if (!hasKeyframeGroup(node, propertyName)) {
        ModelNode frames = modelNode().view()->createModelNode("QtQuick.Timeline.KeyframeGroup", 1, 0);

        modelNode().defaultNodeListProperty().reparentHere(frames);

        QmlTimelineKeyframeGroup(frames).setTarget(node);
        QmlTimelineKeyframeGroup(frames).setPropertyName(propertyName);

        Q_ASSERT(QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(frames));
    }
}


bool QmlTimeline::hasKeyframeGroup(const ModelNode &node, const PropertyName &propertyName) const
{
    for (const QmlTimelineKeyframeGroup &frames : allKeyframeGroups()) {
        if (frames.target().isValid()
                && frames.target() == node
                && frames.propertyName() == propertyName)
            return true;
    }

    return false;
}

bool QmlTimeline::hasKeyframeGroupForTarget(const ModelNode &node) const
{
    if (!isValid())
        return false;

    for (const QmlTimelineKeyframeGroup &frames : allKeyframeGroups()) {
        if (frames.target().isValid() && frames.target() == node)
            return true;
    }

    return false;
}

void QmlTimeline::insertKeyframe(const ModelNode &target, const PropertyName &propertyName)
{
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
            returnList.append(QmlTimelineKeyframeGroup(childNode));
    }

    return returnList;
}

} // QmlDesigner
