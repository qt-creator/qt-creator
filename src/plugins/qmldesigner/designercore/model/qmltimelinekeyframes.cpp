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

#include "qmltimelinekeyframes.h"
#include "abstractview.h"
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <metainfo.h>
#include <invalidmodelnodeexception.h>
#include "bindingproperty.h"
#include "qmlitemnode.h"

#include <utils/qtcassert.h>

#include <limits>

namespace QmlDesigner {

QmlTimelineFrames::QmlTimelineFrames()
{

}

QmlTimelineFrames::QmlTimelineFrames(const ModelNode &modelNode) : QmlModelNodeFacade(modelNode)
{

}

bool QmlTimelineFrames::isValid() const
{
    return isValidQmlTimelineFrames(modelNode());
}

bool QmlTimelineFrames::isValidQmlTimelineFrames(const ModelNode &modelNode)
{
    return isValidQmlModelNodeFacade(modelNode)
            && modelNode.metaInfo().isValid()
            && modelNode.metaInfo().isSubclassOf("QtQuick.Timeline.Keyframes");
}

void QmlTimelineFrames::destroy()
{
    Q_ASSERT(isValid());
    modelNode().destroy();
}

ModelNode QmlTimelineFrames::target() const
{
    if (modelNode().property("target").isBindingProperty())
        return modelNode().bindingProperty("target").resolveToModelNode();
    else
        return ModelNode(); //exception?
}

void QmlTimelineFrames::setTarget(const ModelNode &target)
{
    modelNode().bindingProperty("target").setExpression(target.id());
}


PropertyName QmlTimelineFrames::propertyName() const
{
    return modelNode().variantProperty("property").value().toString().toUtf8();
}

void QmlTimelineFrames::setPropertyName(const PropertyName &propertyName)
{
    modelNode().variantProperty("property").setValue(QString::fromUtf8(propertyName));
}

void QmlTimelineFrames::setValue(const QVariant &value, qreal currentFrame)
{

    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (qFuzzyCompare(childNode.variantProperty("frame").value().toReal(), currentFrame)) {
            childNode.variantProperty("value").setValue(value);
            return;
        }
    }

    const QList<QPair<PropertyName, QVariant> > propertyPairList{{PropertyName("frame"), QVariant(currentFrame)},
                                                                 {PropertyName("value"), value}};

    ModelNode frame = modelNode().view()->createModelNode("QtQuick.Timeline.Keyframe", 1, 0, propertyPairList);
    modelNode().defaultNodeListProperty().reparentHere(frame);
}

QVariant QmlTimelineFrames::value(qreal frame) const
{
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (qFuzzyCompare(childNode.variantProperty("frame").value().toReal(), frame)) {
            return childNode.variantProperty("value").value();
        }
    }

    return QVariant();
}

bool QmlTimelineFrames::hasKeyframe(qreal frame)
{
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        if (qFuzzyCompare(childNode.variantProperty("frame").value().toReal(), frame))
            return true;
    }

    return false;
}

qreal QmlTimelineFrames::minActualFrame() const
{
    qreal min = std::numeric_limits<double>::max();
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        QVariant value = childNode.variantProperty("frame").value();
        if (value.isValid() && value.toReal() < min)
            min = value.toReal();
    }

    return min;
}

qreal QmlTimelineFrames::maxActualFrame() const
{
    qreal max = std::numeric_limits<double>::min();
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        QVariant value = childNode.variantProperty("frame").value();
        if (value.isValid() && value.toReal() > max)
            max = value.toReal();
    }

    return max;
}

const QList<ModelNode> QmlTimelineFrames::keyframePositions() const
{
    QList<ModelNode> returnValues;
    for (const ModelNode &childNode : modelNode().defaultNodeListProperty().toModelNodeList()) {
        QVariant value = childNode.variantProperty("frame").value();
        if (value.isValid())
            returnValues.append(childNode);
    }

    return returnValues;
}

bool QmlTimelineFrames::isValidKeyframe(const ModelNode &node)
{
    return isValidQmlModelNodeFacade(node)
            && node.metaInfo().isValid()
            && node.metaInfo().isSubclassOf("QtQuick.Timeline.Keyframe");
}

QmlTimelineFrames QmlTimelineFrames::keyframesForKeyframe(const ModelNode &node)
{
    if (isValidKeyframe(node) && node.hasParentProperty()) {
        const QmlTimelineFrames timeline(node.parentProperty().parentModelNode());
        if (timeline.isValid())
            return timeline;
    }

    return QmlTimelineFrames();
}

} // QmlDesigner
