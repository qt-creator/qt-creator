/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "curveeditorview.h"
#include "curveeditor.h"
#include "curveeditormodel.h"
#include "curvesegment.h"
#include "treeitem.h"

#include <bindingproperty.h>
#include <easingcurve.h>
#include <nodeabstractproperty.h>
#include <variantproperty.h>
#include <qmlstate.h>
#include <qmltimeline.h>

#include <cmath>

namespace QmlDesigner {

CurveEditorView::CurveEditorView(QObject *parent)
    : AbstractView(parent)
    , m_block(false)
    , m_model(new CurveEditorModel())
    , m_editor(new CurveEditor(m_model))
{
    Q_UNUSED(parent);
    connect(m_model, &CurveEditorModel::commitCurrentFrame, this, &CurveEditorView::commitCurrentFrame);
    connect(m_model, &CurveEditorModel::commitStartFrame, this, &CurveEditorView::commitStartFrame);
    connect(m_model, &CurveEditorModel::commitEndFrame, this, &CurveEditorView::commitEndFrame);
    connect(m_model, &CurveEditorModel::curveChanged, this, &CurveEditorView::commitKeyframes);
}

CurveEditorView::~CurveEditorView() {}

bool CurveEditorView::hasWidget() const
{
    return true;
}

WidgetInfo CurveEditorView::widgetInfo()
{
    return createWidgetInfo(
        m_editor, nullptr, "CurveEditorId", WidgetInfo::BottomPane, 0, tr("CurveEditor"));
}

void CurveEditorView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    QmlTimeline timeline = activeTimeline();
    if (timeline.isValid()) {
        m_model->setTimeline(timeline);
    }
}

void CurveEditorView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
    m_model->reset({});
}

bool dirtyfiesView(const ModelNode &node)
{
    return QmlTimeline::isValidQmlTimeline(node)
           || QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(node);
}

void CurveEditorView::nodeRemoved(const ModelNode &removedNode,
                                  const NodeAbstractProperty &parentProperty,
                                  PropertyChangeFlags propertyChange)
{
    Q_UNUSED(removedNode);
    Q_UNUSED(propertyChange);

    if (!parentProperty.isValid())
        return;

    ModelNode parent = parentProperty.parentModelNode();
    if (dirtyfiesView(parent))
        updateKeyframes();
}

void CurveEditorView::nodeReparented(const ModelNode &node,
                                     const NodeAbstractProperty &newPropertyParent,
                                     const NodeAbstractProperty &oldPropertyParent,
                                     PropertyChangeFlags propertyChange)
{
    Q_UNUSED(node);
    Q_UNUSED(newPropertyParent);
    Q_UNUSED(oldPropertyParent);
    Q_UNUSED(propertyChange);

    ModelNode parent = newPropertyParent.parentModelNode();
    if (newPropertyParent.isValid() && dirtyfiesView(parent))
        updateKeyframes();
    else if (QmlTimelineKeyframeGroup::checkKeyframesType(node))
        updateKeyframes();
}

void CurveEditorView::auxiliaryDataChanged(const ModelNode &node,
                                           const PropertyName &name,
                                           const QVariant &data)
{
    if (name == "locked") {
        if (auto *item = m_model->find(node.id())) {
            QSignalBlocker blocker(m_model);
            m_model->setLocked(item, data.toBool());
        }
    }
}

void CurveEditorView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName>> &propertyList)
{
    Q_UNUSED(propertyList);

    for (const auto &pair : propertyList) {
        if (!QmlTimeline::isValidQmlTimeline(pair.first))
            continue;

        if (pair.second == "startFrame")
            updateStartFrame(pair.first);
        else if (pair.second == "endFrame")
            updateEndFrame(pair.first);
        else if (pair.second == "currentFrame")
            updateCurrentFrame(pair.first);
    }
}

void CurveEditorView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                               PropertyChangeFlags propertyChange)
{
    Q_UNUSED(propertyList);
    Q_UNUSED(propertyChange);

    for (const auto &property : propertyList) {
        if ((property.name() == "frame" || property.name() == "value")
            && property.parentModelNode().type() == "QtQuick.Timeline.Keyframe"
            && property.parentModelNode().isValid()
            && property.parentModelNode().hasParentProperty()) {
            const ModelNode framesNode = property.parentModelNode().parentProperty().parentModelNode();
            if (QmlTimelineKeyframeGroup::isValidQmlTimelineKeyframeGroup(framesNode))
                updateKeyframes();
        }
    }
}

void CurveEditorView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                               PropertyChangeFlags propertyChange)
{
    Q_UNUSED(propertyList);
    Q_UNUSED(propertyChange);

    for (const auto &property : propertyList) {
        if (property.name() == "easing.bezierCurve") {
            updateKeyframes();
        }
    }
}

void CurveEditorView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    Q_UNUSED(propertyList);

    for (const auto &property : propertyList) {
        if (property.name() == "keyframes" && property.parentModelNode().isValid()) {
            ModelNode parent = property.parentModelNode();
            if (dirtyfiesView(parent))
                updateKeyframes();
        }
    }
}

QmlTimeline CurveEditorView::activeTimeline() const
{
    QmlModelState state = currentState();
    if (state.isBaseState()) {
        for (const ModelNode &node : allModelNodesOfType("QtQuick.Timeline.Timeline")) {
            if (QmlTimeline::isValidQmlTimeline(node)) {
                if (node.hasVariantProperty("enabled")
                    && node.variantProperty("enabled").value().toBool())
                    return QmlTimeline(node);

                return {};
            }
        }
    }

    for (const ModelNode &node : allModelNodesOfType("QtQuick.Timeline.Timeline")) {
        if (QmlTimeline::isValidQmlTimeline(node) && state.affectsModelNode(node)) {
            QmlPropertyChanges propertyChanges(state.propertyChanges(node));
            if (!propertyChanges.isValid())
                continue;

            if (node.hasVariantProperty("enabled") && node.variantProperty("enabled").value().toBool())
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
    if (node.isValid())
        return timeline.keyframeGroup(node, item->name().toLatin1());

    return QmlTimelineKeyframeGroup();
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
    if (node.isValid()) {
        if (item->locked())
            node.setAuxiliaryData("locked", true);
        else
            node.removeAuxiliaryData("locked");

        if (item->pinned())
            node.setAuxiliaryData("pinned", true);
        else
            node.removeAuxiliaryData("pinned");

        if (auto *pitem = item->asPropertyItem()) {
            if (pitem->hasUnified())
                node.setAuxiliaryData("unified", pitem->unifyString());
            else
                node.removeAuxiliaryData("unified");
        }
    }
}

void CurveEditorView::commitKeyframes(TreeItem *item)
{
    if (auto *nitem = item->asNodeItem()) {
        ModelNode node = modelNodeForId(nitem->name());
        commitAuxiliaryData(node, item);

    } else if (auto *pitem = item->asPropertyItem()) {
        QmlTimeline currentTimeline = activeTimeline();
        QmlTimelineKeyframeGroup group = timelineKeyframeGroup(currentTimeline, pitem);

        if (group.isValid()) {
            ModelNode groupNode = group.modelNode();
            commitAuxiliaryData(groupNode, item);

            auto replaceKeyframes = [&group, pitem, this]() {
                m_block = true;
                for (auto frame : group.keyframes())
                    frame.destroy();

                Keyframe previous;
                for (auto &&frame : pitem->curve().keyframes()) {
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
                            if (data.type() == static_cast<int>(QMetaType::QEasingCurve))
                                attachEasingCurve(group, pos.x(), data.value<QEasingCurve>());
                        }
                    }

                    previous = frame;
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
    timeline.modelNode().setAuxiliaryData("currentFrame@NodeInstance", frame);
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

} // namespace QmlDesigner
