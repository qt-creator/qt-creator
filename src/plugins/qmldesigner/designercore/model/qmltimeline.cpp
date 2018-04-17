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

#include "qmltimeline.h"
#include "qmltimelinekeyframes.h"
#include "abstractview.h"
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <nodelistproperty.h>
#include <metainfo.h>
#include <invalidmodelnodeexception.h>
#include "bindingproperty.h"
#include "qmlitemnode.h"

#include <utils/qtcassert.h>

#include <limits>

namespace QmlDesigner {

QmlTimeline::QmlTimeline()
{

}

QmlTimeline::QmlTimeline(const ModelNode &modelNode) : QmlModelNodeFacade(modelNode)
{

}

bool QmlTimeline::isValid() const
{
    return isValidQmlTimeline(modelNode());
}

bool QmlTimeline::isValidQmlTimeline(const ModelNode &modelNode)
{
    return isValidQmlModelNodeFacade(modelNode)
               && modelNode.metaInfo().isValid()
               && modelNode.metaInfo().isSubclassOf("QtQuick.Timeline.Timeline");
}

void QmlTimeline::destroy()
{
    Q_ASSERT(isValid());
    modelNode().destroy();
}

QmlTimelineFrames QmlTimeline::timelineFrames(const ModelNode &node, const PropertyName &propertyName)
{
    if (isValid()) {
        addFramesIfNotExists(node, propertyName);
        for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
            if (QmlTimelineFrames::isValidQmlTimelineFrames(childNode)) {
                const QmlTimelineFrames frames(childNode);

                if (frames.target().isValid()
                        && frames.target() == node
                        && frames.propertyName() == propertyName)
                    return frames;
            }
        }
    }

    return QmlTimelineFrames(); //not found
}

bool QmlTimeline::hasTimeline(const ModelNode &node, const PropertyName &propertyName)
{
    if (isValid()) {
        for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
            if (QmlTimelineFrames::isValidQmlTimelineFrames(childNode)) {
                const QmlTimelineFrames frames(childNode);

                if (frames.target().isValid()
                        && frames.target() == node
                        && frames.propertyName() == propertyName)
                    return true;
            }
        }
    }
    return false;
}

qreal QmlTimeline::startFrame() const
{
    if (isValid())
        return QmlObjectNode(modelNode()).instanceValue("startFrame").toReal();
    return 0;
}

qreal QmlTimeline::endFrame() const
{
    if (isValid())
        return QmlObjectNode(modelNode()).instanceValue("endFrame").toReal();
    return 0;
}

qreal QmlTimeline::currentFrame() const
{
    if (isValid())
        return QmlObjectNode(modelNode()).instanceValue("currentFrame").toReal();
    return 0;
}

qreal QmlTimeline::duration() const
{
    return endFrame() - startFrame();
}

bool QmlTimeline::isEnabled() const
{
    return QmlObjectNode(modelNode()).modelValue("enabled").toBool();
}

qreal QmlTimeline::minActualFrame(const ModelNode &target) const
{
    qreal min = std::numeric_limits<double>::max();

    for (const QmlTimelineFrames &frames : framesForTarget(target)) {
        qreal value = frames.minActualFrame();
        if (value < min)
            min = value;
    }

    return min;
}

qreal QmlTimeline::maxActualFrame(const ModelNode &target) const
{
    qreal max = std::numeric_limits<double>::min();

    for (const QmlTimelineFrames &frames : framesForTarget(target)) {
        qreal value = frames.maxActualFrame();
        if (value > max)
            max = value;
    }

    return max;
}

void QmlTimeline::moveAllFrames(const ModelNode &target, qreal offset)
{
    for (QmlTimelineFrames &frames : framesForTarget(target))
        frames.moveAllFrames(offset);

}

void QmlTimeline::scaleAllFrames(const ModelNode &target, qreal factor)
{
    for (QmlTimelineFrames &frames : framesForTarget(target))
        frames.scaleAllFrames(factor);
}

QList<ModelNode> QmlTimeline::allTargets() const
{
    QList<ModelNode> result;
    if (isValid()) {
        for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
            if (QmlTimelineFrames::isValidQmlTimelineFrames(childNode)) {
                const QmlTimelineFrames frames(childNode);
                if (!result.contains(frames.target()))
                    result.append(frames.target());
            }
        }
    }
    return result;
}

QList<QmlTimelineFrames> QmlTimeline::framesForTarget(const ModelNode &target) const
{
     QList<QmlTimelineFrames> result;
     if (isValid()) {
         for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
             if (QmlTimelineFrames::isValidQmlTimelineFrames(childNode)) {
                 const QmlTimelineFrames frames(childNode);
                 if (frames.target() == target)
                     result.append(frames);
             }
         }
     }
     return result;
}

void QmlTimeline::destroyFramesForTarget(const ModelNode &target)
{
    for (QmlTimelineFrames frames : framesForTarget(target))
        frames.destroy();
}

bool QmlTimeline::hasActiveTimeline(AbstractView *view)
{
    if (view && view->isAttached()) {
        if (!view->model()->hasImport(Import::createLibraryImport("QtQuick.Timeline", "1.0"), true, true))
            return false;

        const ModelNode root = view->rootModelNode();
        if (root.isValid())
            for (const ModelNode &child : root.directSubModelNodes()) {
                if (QmlTimeline::isValidQmlTimeline(child))
                    return QmlTimeline(child).isEnabled();
            }
    }

    return false;
}

void QmlTimeline::addFramesIfNotExists(const ModelNode &node, const PropertyName &propertyName)
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!hasFrames(node, propertyName)) {
        ModelNode frames = modelNode().view()->createModelNode("QtQuick.Timeline.KeyframeGroup", 1, 0);
        modelNode().defaultNodeListProperty().reparentHere(frames);

        QmlTimelineFrames(frames).setTarget(node);
        QmlTimelineFrames(frames).setPropertyName(propertyName);

        Q_ASSERT(QmlTimelineFrames::isValidQmlTimelineFrames(frames));
    }
}


bool QmlTimeline::hasFrames(const ModelNode &node, const PropertyName &propertyName) const
{
    for (const QmlTimelineFrames &frames : allTimelineFrames()) {
        if (frames.target().isValid()
                && frames.target() == node
                && frames.propertyName() == propertyName)
            return true;
    }

    return false;
}

QList<QmlTimelineFrames> QmlTimeline::allTimelineFrames() const
{
    QList<QmlTimelineFrames> returnList;

    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (QmlTimelineFrames::isValidQmlTimelineFrames(childNode))
            returnList.append(QmlTimelineFrames(childNode));
    }

    return returnList;
}

} // QmlDesigner
