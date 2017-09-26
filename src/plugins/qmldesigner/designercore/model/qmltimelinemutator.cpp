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

#include "qmltimelinemutator.h"
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

namespace QmlDesigner {

QmlTimelineMutator::QmlTimelineMutator()
{

}

QmlTimelineMutator::QmlTimelineMutator(const ModelNode &modelNode) : QmlModelNodeFacade(modelNode)
{

}

bool QmlTimelineMutator::isValid() const
{
    return isValidQmlTimelineMutator(modelNode());
}

bool QmlTimelineMutator::isValidQmlTimelineMutator(const ModelNode &modelNode)
{
    return isValidQmlModelNodeFacade(modelNode)
               && modelNode.metaInfo().isValid()
               && modelNode.metaInfo().isSubclassOf("QtQuick.Timeline.KeyframeMutator");
}

void QmlTimelineMutator::destroy()
{
    Q_ASSERT(isValid());
    modelNode().destroy();
}

QmlTimelineFrames QmlTimelineMutator::timelineFrames(const ModelNode &node, const PropertyName &propertyName)
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

bool QmlTimelineMutator::hasTimeline(const ModelNode &node, const PropertyName &propertyName)
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

qreal QmlTimelineMutator::startFrame() const
{
    if (isValid())
        return QmlObjectNode(modelNode()).instanceValue("startFrame").toReal();
    return 0;
}

qreal QmlTimelineMutator::endFrame() const
{
    if (isValid())
        return QmlObjectNode(modelNode()).instanceValue("endFrame").toReal();
    return 0;
}

qreal QmlTimelineMutator::currentFrame() const
{
    if (isValid())
        return QmlObjectNode(modelNode()).instanceValue("currentFrame").toReal();
    return 0;
}

void QmlTimelineMutator::addFramesIfNotExists(const ModelNode &node, const PropertyName &propertyName)
{
    if (!isValid())
        throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!hasFrames(node, propertyName)) {
        ModelNode frames = modelNode().view()->createModelNode("QtQuick.Timeline.Keyframes", 1, 0);
        modelNode().defaultNodeListProperty().reparentHere(frames);

        QmlTimelineFrames(frames).setTarget(node);
        QmlTimelineFrames(frames).setPropertyName(propertyName);

        Q_ASSERT(QmlTimelineFrames::isValidQmlTimelineFrames(frames));
    }
}


bool QmlTimelineMutator::hasFrames(const ModelNode &node, const PropertyName &propertyName) const
{
    for (const QmlTimelineFrames &frames : allTimelineFrames()) {
        if (frames.target().isValid()
                && frames.target() == node
                && frames.propertyName() == propertyName)
            return true;
    }

    return false;
}

QList<QmlTimelineFrames> QmlTimelineMutator::allTimelineFrames() const
{
    QList<QmlTimelineFrames> returnList;

    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (QmlTimelineFrames::isValidQmlTimelineFrames(childNode))
            returnList.append(QmlTimelineFrames(childNode));
    }

    return returnList;
}

} // QmlDesigner
