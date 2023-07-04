// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "curveeditormodel.h"
#include "curveeditorstyle.h"
#include "detail/treeview.h"
#include "treeitem.h"

#include "detail/graphicsview.h"
#include "detail/selectionmodel.h"

#include "easingcurve.h"
#include "qmltimeline.h"

#include <bindingproperty.h>
#include <nodeabstractproperty.h>
#include <nodemetainfo.h>
#include <theme.h>
#include <variantproperty.h>

namespace QmlDesigner {

CurveEditorModel::CurveEditorModel(QObject *parent)
    : TreeModel(parent)
    , m_hasTimeline(false)
    , m_currentFrame(0)
    , m_minTime(CurveEditorStyle::defaultTimeMin)
    , m_maxTime(CurveEditorStyle::defaultTimeMax)
{}

CurveEditorModel::~CurveEditorModel() {}

int CurveEditorModel::currentFrame() const
{
    return m_currentFrame;
}

double CurveEditorModel::minimumTime() const
{
    return m_minTime;
}

double CurveEditorModel::maximumTime() const
{
    return m_maxTime;
}

CurveEditorStyle CurveEditorModel::style() const
{
    // Pseudo auto generated. See: CurveEditorStyleDialog
    CurveEditorStyle out;
    out.backgroundBrush = QmlDesigner::Theme::getColor(QmlDesigner::Theme::DSsectionHeadBackground);
    out.backgroundAlternateBrush = QmlDesigner::Theme::getColor(QmlDesigner::Theme::DSpanelBackground);
    out.fontColor = QmlDesigner::Theme::getColor(QmlDesigner::Theme::DStextColor);
    out.gridColor = QColor(114, 116, 118);
    out.canvasMargin = 15;
    out.zoomInWidth = 99;
    out.zoomInHeight = 99;
    out.timeAxisHeight = 60;
    out.timeOffsetLeft = 10;
    out.timeOffsetRight = 10;
    out.rangeBarColor = QmlDesigner::Theme::getColor(Theme::DScontrolBackground);
    out.rangeBarCapsColor = QmlDesigner::Theme::getColor(
        QmlDesigner::Theme::QmlDesigner_HighlightColor);
    out.valueAxisWidth = 60;
    out.valueOffsetTop = 10;
    out.valueOffsetBottom = 10;
    out.handleStyle.size = 10;
    out.handleStyle.lineWidth = 1;
    out.handleStyle.color = QColor(255, 255, 255);
    out.handleStyle.selectionColor = QColor(255, 255, 255);
    out.keyframeStyle.size = 14;
    out.keyframeStyle.color = QColor(172, 210, 255);
    out.keyframeStyle.selectionColor = QColor(255, 255, 255);
    out.curveStyle.width = 2;
    out.curveStyle.color = QColor(0, 200, 0);
    out.curveStyle.selectionColor = QColor(255, 255, 255);
    out.treeItemStyle.margins = 0;
    out.playhead.width = 20;
    out.playhead.radius = 4;
    out.playhead.color = QColor(200, 200, 0);
    return out;
}

void CurveEditorModel::setTimeline(const QmlDesigner::QmlTimeline &timeline)
{
    m_hasTimeline = timeline.isValid();

    if (m_hasTimeline) {
        m_currentFrame = static_cast<int>(timeline.currentKeyframe());
        m_minTime = timeline.startKeyframe();
        m_maxTime = timeline.endKeyframe();
        std::vector<TreeItem *> items;
        for (auto &&target : timeline.allTargets()) {
            if (TreeItem *item = createTopLevelItem(timeline, target))
                items.push_back(item);
        }
        reset(items);
    }
    emit timelineChanged(m_hasTimeline);
}

void CurveEditorModel::setCurrentFrame(int frame)
{
    m_currentFrame = frame;
    emit commitCurrentFrame(m_currentFrame);
}

void CurveEditorModel::setMinimumTime(double time)
{
    m_minTime = time;
    emit commitStartFrame(static_cast<int>(m_minTime));
}

void CurveEditorModel::setMaximumTime(double time)
{
    m_maxTime = time;
    emit commitEndFrame(static_cast<int>(m_maxTime));
}

void CurveEditorModel::setCurve(unsigned int id, const AnimationCurve &curve)
{
    if (TreeItem *item = find(id)) {
        if (PropertyTreeItem *propertyItem = item->asPropertyItem()) {
            propertyItem->setCurve(curve);
            emit curveChanged(propertyItem);
        }
    }
}

void CurveEditorModel::setLocked(TreeItem *item, bool val)
{
    item->setLocked(val);

    if (auto *gview = graphicsView())
        gview->setLocked(item);

    if (auto *tview = treeView())
        tview->viewport()->update();

    emit curveChanged(item);
}

void CurveEditorModel::setPinned(TreeItem *item, bool val)
{
    item->setPinned(val);

    if (auto *gview = graphicsView())
        gview->setPinned(item);

    if (auto *tview = treeView())
        tview->viewport()->update();

    emit curveChanged(item);
}

bool contains(const std::vector<TreeItem::Path> &selection, const TreeItem::Path &path)
{
    for (auto &&sel : selection)
        if (path == sel)
            return true;

    return false;
}

void CurveEditorModel::reset(const std::vector<TreeItem *> &items)
{
    std::vector<TreeItem::Path> sel;
    if (SelectionModel *sm = selectionModel())
        sel = sm->selectedPaths();

    beginResetModel();

    initialize();

    unsigned int counter = 0;
    std::vector<CurveItem *> pinned;

    for (auto *item : items) {
        item->setId(++counter);
        root()->addChild(item);
        if (auto *nti = item->asNodeItem()) {
            for (auto *pti : nti->properties()) {
                if (pti->pinned() && !contains(sel, pti->path()))
                    pinned.push_back(TreeModel::curveItem(pti));
            }
        }
    }

    endResetModel();

    graphicsView()->reset(pinned);

    if (SelectionModel *sm = selectionModel())
        sm->selectPaths(sel);
}

PropertyTreeItem::ValueType typeFrom(const QmlDesigner::QmlTimelineKeyframeGroup &group)
{
    if (group.valueType().isFloat())
        return PropertyTreeItem::ValueType::Double;

    if (group.valueType().isBool())
        return PropertyTreeItem::ValueType::Bool;

    if (group.valueType().isInteger())
        return PropertyTreeItem::ValueType::Integer;

    // Ignoring: QColor / HAlignment / VAlignment
    return PropertyTreeItem::ValueType::Undefined;
}

std::vector<QString> parentIds(const QmlDesigner::ModelNode &node)
{
    if (!node.hasParentProperty())
        return {};

    std::vector<QString> out;

    QmlDesigner::ModelNode parent = node.parentProperty().parentModelNode();
    while (parent.isValid()) {
        out.push_back(parent.id());

        if (parent.hasParentProperty())
            parent = parent.parentProperty().parentModelNode();
        else
            break;
    }
    return out;
}

TreeItem *CurveEditorModel::createTopLevelItem(const QmlDesigner::QmlTimeline &timeline,
                                               const QmlDesigner::ModelNode &node)
{
    if (!node.isValid())
        return nullptr;

    auto *nodeItem = new NodeTreeItem(node.id(), node.typeIcon(), parentIds(node));
    if (node.locked())
        nodeItem->setLocked(true);

    for (auto &&grp : timeline.keyframeGroupsForTarget(node)) {
        if (grp.isValid()) {
            AnimationCurve curve = createAnimationCurve(grp);
            if (!curve.isEmpty()) {
                QString name = QString::fromUtf8(grp.propertyName());
                auto propertyItem = new PropertyTreeItem(name, curve);

                QmlDesigner::ModelNode target = grp.modelNode();
                if (target.locked())
                    propertyItem->setLocked(true);

                if (target.hasAuxiliaryData(pinnedProperty))
                    propertyItem->setPinned(true);

                nodeItem->addChild(propertyItem);
            }
        }
    }

    if (!nodeItem->hasChildren()) {
        delete nodeItem;
        nodeItem = nullptr;
    }

    return nodeItem;
}

AnimationCurve CurveEditorModel::createAnimationCurve(const QmlDesigner::QmlTimelineKeyframeGroup &group)
{
    switch (typeFrom(group)) {
    case PropertyTreeItem::ValueType::Bool:
        return createBooleanCurve(group);

    case PropertyTreeItem::ValueType::Integer:
        return createDoubleCurve(group);

    case PropertyTreeItem::ValueType::Double:
        return createDoubleCurve(group);

    default:
        return AnimationCurve();
    }
}

std::vector<Keyframe> createKeyframes(QList<QmlDesigner::ModelNode> nodes)
{
    auto byTime = [](const auto &a, const auto &b) {
        return a.variantProperty("frame").value().toDouble()
               < b.variantProperty("frame").value().toDouble();
    };
    std::sort(nodes.begin(), nodes.end(), byTime);

    std::vector<Keyframe> frames;
    for (auto &&node : nodes) {
        QVariant timeVariant = node.variantProperty("frame").value();
        QVariant valueVariant = node.variantProperty("value").value();
        if (!timeVariant.isValid() || !valueVariant.isValid())
            continue;

        QPointF position(timeVariant.toDouble(), valueVariant.toDouble());

        auto keyframe = Keyframe(position);

        if (node.hasBindingProperty("easing.bezierCurve")) {
            QmlDesigner::EasingCurve ecurve;
            ecurve.fromString(node.bindingProperty("easing.bezierCurve").expression());
            keyframe.setData(static_cast<QEasingCurve>(ecurve));
        }
        frames.push_back(keyframe);
    }
    return frames;
}

std::vector<Keyframe> resolveSmallCurves(const std::vector<Keyframe> &frames)
{
    std::vector<Keyframe> out;
    for (auto &&frame : frames) {
        if (frame.hasData() && !out.empty()) {
            QEasingCurve curve = frame.data().toEasingCurve();
            // One-segment-curve: Since (0,0) is implicit => 3
            if (curve.toCubicSpline().size() == 3) {
                Keyframe &previous = out.back();
#if 0
                // Do not resolve when two adjacent keyframes have the same value.
                if (qFuzzyCompare(previous.position().y(), frame.position().y())) {
                    out.push_back(frame);
                    continue;
                }
#endif
                // This is just a temporary curve. ValueType does not matter
                AnimationCurve acurve(
                    AnimationCurve::ValueType::Undefined,
                    curve,
                    previous.position(),
                    frame.position());

                previous.setRightHandle(acurve.keyframeAt(0).rightHandle());
                out.push_back(acurve.keyframeAt(1));
                continue;
            }
        }
        out.push_back(frame);
    }
    return out;
}


AnimationCurve CurveEditorModel::createBooleanCurve(const QmlDesigner::QmlTimelineKeyframeGroup &group)
{
    std::vector<Keyframe> keyframes = createKeyframes(group.keyframePositions());

    for (auto& keyframe : keyframes)
        keyframe.setInterpolation(Keyframe::Interpolation::Step);

    return AnimationCurve(typeFrom(group), keyframes);
}

AnimationCurve CurveEditorModel::createDoubleCurve(const QmlDesigner::QmlTimelineKeyframeGroup &group)
{
    std::vector<Keyframe> keyframes = createKeyframes(group.keyframePositions());
    keyframes = resolveSmallCurves(keyframes);

    QString str;
    QmlDesigner::ModelNode target = group.modelNode();
    if (auto data = target.auxiliaryData(unifiedProperty))
        str = data->toString();

    if (str.size() == static_cast<int>(keyframes.size())) {
        for (int i = 0; i < str.size(); ++i) {
            if (str.at(i) == '1')
                keyframes[static_cast<size_t>(i)].setUnified(true);
        }
    }

    return AnimationCurve(typeFrom(group), keyframes);
}

} // End namespace QmlDesigner.
