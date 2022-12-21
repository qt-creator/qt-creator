// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelineactions.h"

#include "timelineutils.h"
#include "timelineview.h"

#include <auxiliarydataproperties.h>
#include <bindingproperty.h>
#include <designdocument.h>
#include <designdocumentview.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <variantproperty.h>
#include <qmlobjectnode.h>
#include <qmltimelinekeyframegroup.h>

#include <QRegularExpression>
#include <QApplication>
#include <QClipboard>

namespace QmlDesigner {

TimelineActions::TimelineActions() = default;

void TimelineActions::deleteAllKeyframesForTarget(const ModelNode &targetNode,
                                                  const QmlTimeline &timeline)
{
    targetNode.view()->executeInTransaction("TimelineActions::deleteAllKeyframesForTarget", [=](){
        if (timeline.isValid()) {
            for (auto frames : timeline.keyframeGroupsForTarget(targetNode))
                frames.destroy();
        }
    });
}

void TimelineActions::insertAllKeyframesForTarget(const ModelNode &targetNode,
                                                  const QmlTimeline &timeline)
{
    targetNode.view()->executeInTransaction("TimelineActions::insertAllKeyframesForTarget", [=](){
        auto object = QmlObjectNode(targetNode);
        if (timeline.isValid() && object.isValid()) {
            for (auto frames : timeline.keyframeGroupsForTarget(targetNode)) {
                QVariant value = object.instanceValue(frames.propertyName());
                frames.setValue(value, timeline.currentKeyframe());
            }
        }

    });
}

void TimelineActions::copyAllKeyframesForTarget(const ModelNode &targetNode,
                                                const QmlTimeline &timeline)
{
    DesignDocumentView::copyModelNodes(Utils::transform(timeline.keyframeGroupsForTarget(targetNode),
                                                        &QmlTimelineKeyframeGroup::modelNode),
                                       targetNode.view()->externalDependencies());
}

void TimelineActions::pasteKeyframesToTarget(const ModelNode &targetNode,
                                             const QmlTimeline &timeline)
{
    if (timeline.isValid()) {
        auto pasteModel = DesignDocumentView::pasteToModel(targetNode.view()->externalDependencies());

        if (!pasteModel)
            return;

        DesignDocumentView view{targetNode.view()->externalDependencies()};
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

        targetNode.view()->model()->attachView(&view);
        view.executeInTransaction("TimelineActions::pasteKeyframesToTarget", [=, &view](){

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
        });
    }
}

void TimelineActions::copyKeyframes(const QList<ModelNode> &keyframes,
                                    ExternalDependenciesInterface &externalDependencies)
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
                    node.setAuxiliaryData(AuxiliaryDataType::Document, name, bpNode.id());
            } else if (property.isVariantProperty()) {
                VariantProperty vp = property.toVariantProperty();
                node.setAuxiliaryData(AuxiliaryDataType::Document, name, vp.value());
            }
        }

        nodes << node;
    }

    DesignDocumentView::copyModelNodes(nodes, externalDependencies);
}

bool isKeyframe(const ModelNode &node)
{
    return node.metaInfo().isQtQuickTimelineKeyframe();
}

QVariant getValue(const ModelNode &node)
{
    return node.variantProperty("value").value();
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
    auto targetId = node.auxiliaryData(targetProperty);
    auto property = node.auxiliaryData(propertyProperty);

    if (targetId && property) {
        ModelNode targetNode = timelineView->modelNodeForId(targetId->toString());
        if (targetNode.isValid()) {
            for (QmlTimelineKeyframeGroup frameGrp : timeline.keyframeGroupsForTarget(targetNode)) {
                if (frameGrp.propertyName() == property->toByteArray())
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
        for (const ModelNode &keyframe : std::as_const(sortedByTime))
            result.emplace_back(keyframe, getTime(keyframe) - firstTime);
    }

    return result;
}

void TimelineActions::pasteKeyframes(AbstractView *timelineView, const QmlTimeline &timeline)
{
    auto pasteModel = DesignDocumentView::pasteToModel(timelineView->externalDependencies());

    if (!pasteModel)
        return;

    DesignDocumentView view{timelineView->externalDependencies()};
    pasteModel->attachView(&view);

    if (!view.rootModelNode().isValid())
        return;

    const qreal currentTime = timeline.currentKeyframe();

    ModelNode rootNode = view.rootModelNode();

    timelineView->executeInTransaction("TimelineActions::pasteKeyframes", [=](){
        if (isKeyframe(rootNode))
            pasteKeyframe(currentTime, rootNode, timelineView, timeline);
        else
            for (auto frame : getFramesRelative(rootNode))
                pasteKeyframe(currentTime + std::get<1>(frame),
                              std::get<0>(frame),
                              timelineView,
                              timeline);

    });
}

bool TimelineActions::clipboardContainsKeyframes()
{
    QRegularExpression rxp("\\bKeyframe\\s*{.*}", QRegularExpression::DotMatchesEverythingOption);
    return rxp.match(QApplication::clipboard()->text()).hasMatch();
}

} // namespace QmlDesigner
