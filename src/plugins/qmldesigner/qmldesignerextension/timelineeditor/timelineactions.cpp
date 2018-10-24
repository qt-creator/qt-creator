/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "timelineactions.h"

#include "timelineutils.h"
#include "timelineview.h"

#include <bindingproperty.h>
#include <designdocument.h>
#include <designdocumentview.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <rewritertransaction.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <variantproperty.h>
#include <qmldesignerplugin.h>
#include <qmlobjectnode.h>
#include <qmltimelinekeyframegroup.h>

namespace QmlDesigner {

TimelineActions::TimelineActions() = default;

void TimelineActions::deleteAllKeyframesForTarget(const ModelNode &targetNode,
                                                  const QmlTimeline &timeline)
{
    try {
        RewriterTransaction transaction(targetNode.view()->beginRewriterTransaction(
            "TimelineActions::deleteAllKeyframesForTarget"));

        if (timeline.isValid()) {
            for (auto frames : timeline.keyframeGroupsForTarget(targetNode))
                frames.destroy();
        }

        transaction.commit();
    } catch (const Exception &e) {
        e.showException();
    }
}

void TimelineActions::insertAllKeyframesForTarget(const ModelNode &targetNode,
                                                  const QmlTimeline &timeline)
{
    try {
        RewriterTransaction transaction(targetNode.view()->beginRewriterTransaction(
            "TimelineGraphicsScene::insertAllKeyframesForTarget"));

        auto object = QmlObjectNode(targetNode);
        if (timeline.isValid() && object.isValid()) {
            for (auto frames : timeline.keyframeGroupsForTarget(targetNode)) {
                QVariant value = object.instanceValue(frames.propertyName());
                frames.setValue(value, timeline.currentKeyframe());
            }
        }

        transaction.commit();
    } catch (const Exception &e) {
        e.showException();
    }
}

void TimelineActions::copyAllKeyframesForTarget(const ModelNode &targetNode,
                                                const QmlTimeline &timeline)
{
    DesignDocumentView::copyModelNodes(Utils::transform(timeline.keyframeGroupsForTarget(targetNode),
                                                        &QmlTimelineKeyframeGroup::modelNode));
}

void TimelineActions::pasteKeyframesToTarget(const ModelNode &targetNode,
                                             const QmlTimeline &timeline)
{
    if (timeline.isValid()) {
        QScopedPointer<Model> pasteModel(DesignDocumentView::pasteToModel());

        if (!pasteModel)
            return;

        DesignDocumentView view;
        pasteModel->attachView(&view);

        if (!view.rootModelNode().isValid())
            return;

        ModelNode rootNode = view.rootModelNode();

        //Sanity check
        if (!QmlTimelineKeyframeGroup::checkKeyframesType(rootNode)) {
            for (const ModelNode &node : rootNode.directSubModelNodes())
                if (!QmlTimelineKeyframeGroup::checkKeyframesType(node))
                    return;
        }

        pasteModel->detachView(&view);

        try {
            targetNode.view()->model()->attachView(&view);

            RewriterTransaction transaction(
                view.beginRewriterTransaction("TimelineActions::pasteKeyframesToTarget"));

            ModelNode nonConstTargetNode = targetNode;
            nonConstTargetNode.validId();

            if (QmlTimelineKeyframeGroup::checkKeyframesType(rootNode)) {
                /* Single selection */

                ModelNode newNode = view.insertModel(rootNode);
                QmlTimelineKeyframeGroup frames(newNode);
                frames.setTarget(targetNode);

                timeline.modelNode().defaultNodeListProperty().reparentHere(newNode);

            } else {
                /* Multi selection */
                for (const ModelNode &node : rootNode.directSubModelNodes()) {
                    ModelNode newNode = view.insertModel(node);
                    QmlTimelineKeyframeGroup frames(newNode);
                    frames.setTarget(targetNode);
                    timeline.modelNode().defaultNodeListProperty().reparentHere(newNode);
                }
            }

            transaction.commit();
        } catch (const Exception &e) {
            e.showException();
        }
    }
}

void TimelineActions::copyKeyframes(const QList<ModelNode> &keyframes)
{
    QList<ModelNode> nodes;
    for (const auto &node : keyframes) {
        NodeAbstractProperty pp = node.parentProperty();
        QTC_ASSERT(pp.isValid(), return );

        ModelNode parentModelNode = pp.parentModelNode();
        for (const auto &property : parentModelNode.properties()) {
            auto name = property.name();
            if (property.isBindingProperty()) {
                BindingProperty bp = property.toBindingProperty();
                ModelNode bpNode = bp.resolveToModelNode();
                if (bpNode.isValid())
                    node.setAuxiliaryData(name, bpNode.id());
            } else if (property.isVariantProperty()) {
                VariantProperty vp = property.toVariantProperty();
                node.setAuxiliaryData(name, vp.value());
            }
        }

        nodes << node;
    }

    DesignDocumentView::copyModelNodes(nodes);
}

bool isKeyframe(const ModelNode &node)
{
    return node.isValid() && node.metaInfo().isValid()
           && node.metaInfo().isSubclassOf("QtQuick.Timeline.Keyframe");
}

QVariant getValue(const ModelNode &node)
{
    if (node.isValid())
        return node.variantProperty("value").value();

    return QVariant();
}

qreal getTime(const ModelNode &node)
{
    Q_ASSERT(node.isValid());
    Q_ASSERT(node.hasProperty("frame"));

    return node.variantProperty("frame").value().toReal();
}

QmlTimelineKeyframeGroup getFrameGroup(const ModelNode &node,
                                       AbstractView *timelineView,
                                       const QmlTimeline &timeline)
{
    QVariant targetId = node.auxiliaryData("target");
    QVariant property = node.auxiliaryData("property");

    if (targetId.isValid() && property.isValid()) {
        ModelNode targetNode = timelineView->modelNodeForId(targetId.toString());
        if (targetNode.isValid()) {
            for (QmlTimelineKeyframeGroup frameGrp : timeline.keyframeGroupsForTarget(targetNode)) {
                if (frameGrp.propertyName() == property.toByteArray())
                    return frameGrp;
            }
        }
    }
    return QmlTimelineKeyframeGroup();
}

void pasteKeyframe(const qreal expectedTime,
                   const ModelNode &keyframe,
                   AbstractView *timelineView,
                   const QmlTimeline &timeline)
{
    QmlTimelineKeyframeGroup group = getFrameGroup(keyframe, timelineView, timeline);
    if (group.isValid()) {
        const qreal clampedTime = TimelineUtils::clamp(expectedTime,
                                                       timeline.startKeyframe(),
                                                       timeline.endKeyframe());

        // Create a new frame ...
        group.setValue(getValue(keyframe), clampedTime);

        // ... look it up by time ...
        for (const ModelNode &key : group.keyframePositions()) {
            qreal time = key.variantProperty("frame").value().toReal();
            if (qFuzzyCompare(clampedTime, time)) {
                // ... and transfer the properties.
                for (const auto &property : keyframe.properties()) {
                    if (property.name() == "frame" || property.name() == "value")
                        continue;

                    if (property.isVariantProperty()) {
                        auto vp = property.toVariantProperty();
                        key.variantProperty(vp.name()).setValue(vp.value());
                    } else if (property.isBindingProperty()) {
                        auto bp = property.toBindingProperty();
                        key.bindingProperty(bp.name()).setExpression(bp.expression());
                    }
                }
            }
        }
    }
}

std::vector<std::tuple<ModelNode, qreal>> getFramesRelative(const ModelNode &parent)
{
    auto byTime = [](const ModelNode &lhs, const ModelNode &rhs) {
        return getTime(lhs) < getTime(rhs);
    };

    std::vector<std::tuple<ModelNode, qreal>> result;

    QList<ModelNode> sortedByTime;
    QList<ModelNode> subs(parent.directSubModelNodes());

    std::copy_if(subs.begin(), subs.end(), std::back_inserter(sortedByTime), &isKeyframe);
    std::sort(sortedByTime.begin(), sortedByTime.end(), byTime);

    if (!sortedByTime.empty()) {
        qreal firstTime = getTime(sortedByTime.first());
        for (ModelNode keyframe : sortedByTime)
            result.emplace_back(keyframe, getTime(keyframe) - firstTime);
    }

    return result;
}

void TimelineActions::pasteKeyframes(AbstractView *timelineView, const QmlTimeline &timeline)
{
    QScopedPointer<Model> pasteModel(DesignDocumentView::pasteToModel());

    if (!pasteModel)
        return;

    DesignDocumentView view;
    pasteModel->attachView(&view);

    if (!view.rootModelNode().isValid())
        return;

    const qreal currentTime = timeline.currentKeyframe();

    ModelNode rootNode = view.rootModelNode();

    try {
        RewriterTransaction transaction(
            timelineView->beginRewriterTransaction("TimelineActions::pasteKeyframes"));
        if (isKeyframe(rootNode))
            pasteKeyframe(currentTime, rootNode, timelineView, timeline);
        else
            for (auto frame : getFramesRelative(rootNode))
                pasteKeyframe(currentTime + std::get<1>(frame),
                              std::get<0>(frame),
                              timelineView,
                              timeline);

        transaction.commit();
    } catch (const Exception &e) {
        e.showException();
    }
}

bool TimelineActions::clipboardContainsKeyframes()
{
    QScopedPointer<Model> pasteModel(DesignDocumentView::pasteToModel());

    if (!pasteModel)
        return false;

    DesignDocumentView view;
    pasteModel->attachView(&view);

    if (!view.rootModelNode().isValid())
        return false;

    ModelNode rootNode = view.rootModelNode();

    if (!rootNode.hasAnySubModelNodes())
        return false;

    //Sanity check
    if (!QmlTimelineKeyframeGroup::checkKeyframesType(rootNode)) {
        for (const ModelNode &node : rootNode.directSubModelNodes())
            if (!QmlTimelineKeyframeGroup::checkKeyframesType(node))
                return false;
    }

    return true;
}

} // namespace QmlDesigner
