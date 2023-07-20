// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "curveeditorview.h"
#include "curveeditor.h"
#include "curveeditormodel.h"
#include "curvesegment.h"
#include "treeitem.h"

#include <auxiliarydataproperties.h>
#include <bindingproperty.h>
#include <easingcurve.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <variantproperty.h>
#include <qmlstate.h>
#include <qmltimeline.h>

#include <cmath>

namespace QmlDesigner {

CurveEditorView::CurveEditorView(ExternalDependenciesInterface &externalDepoendencies)
    : AbstractView{externalDepoendencies}
    , m_block(false)
    , m_model(new CurveEditorModel())
    , m_editor(new CurveEditor(m_model))
{
    connect(m_model, &CurveEditorModel::commitCurrentFrame, this, &CurveEditorView::commitCurrentFrame);
    connect(m_model, &CurveEditorModel::commitStartFrame, this, &CurveEditorView::commitStartFrame);
    connect(m_model, &CurveEditorModel::commitEndFrame, this, &CurveEditorView::commitEndFrame);
    connect(m_model, &CurveEditorModel::curveChanged, this, &CurveEditorView::commitKeyframes);

    connect(m_editor, &CurveEditor::viewEnabledChanged, this, [this](bool enabled){
        setEnabled(enabled);
        if (enabled)
            init();
    });
    setEnabled(false);
}

CurveEditorView::~CurveEditorView() {}

bool CurveEditorView::hasWidget() const
{
    return true;
}

WidgetInfo CurveEditorView::widgetInfo()
{
    return createWidgetInfo(m_editor, "CurveEditorId", WidgetInfo::BottomPane, 0, tr("Curves"));
}

void CurveEditorView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    if (isEnabled())
        init();
}

void CurveEditorView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
    m_model->reset({});
}

bool dirtyfiesView(const ModelNode &node)
{
    return (node.type() == "QtQuick.Timeline.Keyframe" && node.hasParentProperty())
        || QmlTimeline::isValidQmlTimeline(node)
        || QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(node);
}

void CurveEditorView::nodeRemoved([[maybe_unused]] const ModelNode &removedNode,
                                  const NodeAbstractProperty &parentProperty,
                                  [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    if (!parentProperty.isValid())
        return;

    ModelNode parent = parentProperty.parentModelNode();
    if (dirtyfiesView(parent))
        updateKeyframes();

    if (!activeTimeline().isValid())
        m_model->reset({});
}

void CurveEditorView::nodeReparented([[maybe_unused]] const ModelNode &node,
                                     [[maybe_unused]] const NodeAbstractProperty &newPropertyParent,
                                     [[maybe_unused]] const NodeAbstractProperty &oldPropertyParent,
                                     [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    ModelNode parent = newPropertyParent.parentModelNode();
    if (newPropertyParent.isValid() && dirtyfiesView(parent))
        updateKeyframes();
    else if (QmlTimelineKeyframeGroup::checkKeyframesType(node))
        updateKeyframes();
    else if (newPropertyParent.isValid() && !oldPropertyParent.isValid()) {
        if (activeTimeline().hasKeyframeGroupForTarget(node)) {
            updateKeyframes();
        }
    }
}

void CurveEditorView::auxiliaryDataChanged(const ModelNode &node,
                                           AuxiliaryDataKeyView key,
                                           const QVariant &data)
{
    if (key == lockedProperty) {
        if (auto *item = m_model->find(node.id())) {
            QSignalBlocker blocker(m_model);
            m_model->setLocked(item, data.toBool());
        }
    }
}

void CurveEditorView::instancePropertyChanged(
    [[maybe_unused]] const QList<QPair<ModelNode, PropertyName>> &propertyList)
{
    if (auto timeline = activeTimeline(); timeline.isValid()) {

        auto timelineNode = timeline.modelNode();
        for (const auto &pair : propertyList) {
            if (!QmlTimeline::isValidQmlTimeline(pair.first))
                continue;

            if (pair.first != timelineNode)
                continue;

            if (pair.second == "startFrame")
                updateStartFrame(pair.first);
            else if (pair.second == "endFrame")
                updateEndFrame(pair.first);
            else if (pair.second == "currentFrame")
                updateCurrentFrame(pair.first);
        }
    }
}

void CurveEditorView::variantPropertiesChanged([[maybe_unused]] const QList<VariantProperty> &propertyList,
                                               [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    for (const auto &property : propertyList) {
        if (dirtyfiesView(property.parentModelNode()))
            updateKeyframes();
    }
}

void CurveEditorView::bindingPropertiesChanged([[maybe_unused]] const QList<BindingProperty> &propertyList,
                                               [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    for (const auto &property : propertyList) {
        if (dirtyfiesView(property.parentModelNode()))
            updateKeyframes();
    }
}

void CurveEditorView::propertiesRemoved([[maybe_unused]] const QList<AbstractProperty> &propertyList)
{
    for (const auto &property : propertyList) {
        if (dirtyfiesView(property.parentModelNode()))
            updateKeyframes();
    }
}

QmlTimeline CurveEditorView::activeTimeline() const
{
    if (!isAttached())
        return {};

    QmlModelState state = currentState();
    if (state.isBaseState()) {
        for (const ModelNode &node : allModelNodesOfType(model()->qtQuickTimelineTimelineMetaInfo())) {
            if (QmlTimeline::isValidQmlTimeline(node)) {
                if (node.hasVariantProperty("enabled")
                    && node.variantProperty("enabled").value().toBool())
                    return QmlTimeline(node);
            }
        }
        return {};
    }

    for (const ModelNode &node : allModelNodesOfType(model()->qtQuickTimelineTimelineMetaInfo())) {
        if (QmlTimeline::isValidQmlTimeline(node) && state.affectsModelNode(node)) {
            QmlPropertyChanges propertyChanges(state.propertyChanges(node));
            if (!propertyChanges.isValid())
                continue;

            if (propertyChanges.modelNode().hasProperty("enabled") &&
                propertyChanges.modelNode().variantProperty("enabled").value().toBool())
                    return QmlTimeline(node);
        }
    }
    return {};
}

void CurveEditorView::updateKeyframes()
{
    if (m_block)
        return;

    QmlTimeline timeline = activeTimeline();
    if (timeline.isValid())
        m_model->setTimeline(timeline);
    else
        m_model->reset({});
}

void CurveEditorView::updateCurrentFrame(const ModelNode &node)
{
    if (m_editor->dragging())
        return;

    QmlTimeline timeline(node);
    if (timeline.isValid())
        m_model->setCurrentFrame(static_cast<int>(std::round(timeline.currentKeyframe())));
    else
        m_model->setCurrentFrame(0);
}

void CurveEditorView::updateStartFrame(const ModelNode &node)
{
    QmlTimeline timeline(node);
    if (timeline.isValid())
        m_model->setMinimumTime(static_cast<int>(std::round(timeline.startKeyframe())));
}

void CurveEditorView::updateEndFrame(const ModelNode &node)
{
    QmlTimeline timeline(node);
    if (timeline.isValid())
        m_model->setMaximumTime(static_cast<int>(std::round(timeline.endKeyframe())));
}

ModelNode getTargetNode(PropertyTreeItem *item, const QmlTimeline &timeline)
{
    if (const NodeTreeItem *nodeItem = item->parentNodeTreeItem()) {
        QString targetId = nodeItem->name();
        if (timeline.isValid()) {
            for (auto &&target : timeline.allTargets()) {
                if (target.isValid() && target.displayName() == targetId)
                    return target;
            }
        }
    }
    return ModelNode();
}

QmlTimelineKeyframeGroup timelineKeyframeGroup(QmlTimeline &timeline, PropertyTreeItem *item)
{
    ModelNode node = getTargetNode(item, timeline);
    return timeline.keyframeGroup(node, item->name().toLatin1());
}

void attachEasingCurve(const QmlTimelineKeyframeGroup &group, double frame, const QEasingCurve &curve)
{
    ModelNode frameNode = group.keyframe(frame);
    if (frameNode.isValid()) {
        auto expression = EasingCurve(curve).toString();
        frameNode.bindingProperty("easing.bezierCurve").setExpression(expression);
    }
}

void commitAuxiliaryData(ModelNode &node, TreeItem *item)
{
    if (item->locked())
        node.setLocked(true);
    else
        node.setLocked(false);

    if (item->pinned())
        node.setAuxiliaryData(pinnedProperty, true);
    else
        node.removeAuxiliaryData(pinnedProperty);

    if (auto *pitem = item->asPropertyItem()) {
        if (pitem->hasUnified())
            node.setAuxiliaryData(unifiedProperty, pitem->unifyString());
        else
            node.removeAuxiliaryData(unifiedProperty);
    }
}

void CurveEditorView::commitKeyframes(TreeItem *item)
{
    if (!isAttached())
        return;

    if (auto *nitem = item->asNodeItem()) {
        ModelNode node = modelNodeForId(nitem->name());
        commitAuxiliaryData(node, item);

    } else if (auto *pitem = item->asPropertyItem()) {
        QmlTimeline currentTimeline = activeTimeline();
        if (!currentTimeline.isValid())
            return;

        QmlTimelineKeyframeGroup group = timelineKeyframeGroup(currentTimeline, pitem);

        if (group.isValid()) {
            ModelNode groupNode = group.modelNode();
            commitAuxiliaryData(groupNode, item);

            auto replaceKeyframes = [&group, pitem, this]() mutable {
                m_block = true;

                for (auto& frame : group.keyframes())
                    frame.destroy();

                AnimationCurve curve = pitem->curve();
                if (curve.valueType() == AnimationCurve::ValueType::Bool) {
                    for (const auto& frame : curve.keyframes()) {
                        QPointF pos = frame.position();
                        group.setValue(QVariant(pos.y()), pos.x());
                    }
                } else {
                    Keyframe previous;
                    for (const auto& frame : curve.keyframes()) {
                        QPointF pos = frame.position();
                        group.setValue(QVariant(pos.y()), pos.x());

                        if (previous.isValid()) {
                            if (frame.interpolation() == Keyframe::Interpolation::Bezier ||
                                frame.interpolation() == Keyframe::Interpolation::Step ) {
                                CurveSegment segment(previous, frame);
                                if (segment.isValid())
                                    attachEasingCurve(group, pos.x(), segment.easingCurve());
                            } else if (frame.interpolation() == Keyframe::Interpolation::Easing) {
                                QVariant data = frame.data();
                                if (data.typeId() == static_cast<int>(QMetaType::QEasingCurve))
                                    attachEasingCurve(group, pos.x(), data.value<QEasingCurve>());
                            }
                        }
                        previous = frame;
                    }
                }
                m_block = false;
            };

            executeInTransaction("CurveEditor::commitKeyframes", replaceKeyframes);
        }
    }
}

void CurveEditorView::commitCurrentFrame(int frame)
{
    QmlTimeline timeline = activeTimeline();
    if (timeline.isValid())
        timeline.modelNode().setAuxiliaryData(currentFrameProperty, frame);
}

void CurveEditorView::commitStartFrame(int frame)
{
    QmlTimeline timeline = activeTimeline();
    if (timeline.isValid())
        timeline.modelNode().variantProperty("startFrame").setValue(frame);
}

void CurveEditorView::commitEndFrame(int frame)
{
    QmlTimeline timeline = activeTimeline();
    if (timeline.isValid())
        timeline.modelNode().variantProperty("endFrame").setValue(frame);
}

void CurveEditorView::init()
{
    m_model->setTimeline(activeTimeline());
}

} // namespace QmlDesigner
