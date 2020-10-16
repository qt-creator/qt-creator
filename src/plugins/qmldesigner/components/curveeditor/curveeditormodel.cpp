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

#include "curveeditormodel.h"
#include "curveeditorstyle.h"
#include "treeitem.h"

#include "detail/graphicsview.h"
#include "detail/selectionmodel.h"

#include "easingcurve.h"
#include "qmltimeline.h"

#include <bindingproperty.h>
#include <theme.h>
#include <variantproperty.h>

namespace DesignTools {

CurveEditorModel::CurveEditorModel(QObject *parent)
    : TreeModel(parent)
    , m_minTime(CurveEditorStyle::defaultTimeMin)
    , m_maxTime(CurveEditorStyle::defaultTimeMax)
{}

CurveEditorModel::~CurveEditorModel() {}

double CurveEditorModel::minimumTime() const
{
    return m_minTime;
}

double CurveEditorModel::maximumTime() const
{
    return m_maxTime;
}

DesignTools::CurveEditorStyle CurveEditorModel::style() const
{
    // Pseudo auto generated. See: CurveEditorStyleDialog
    DesignTools::CurveEditorStyle out;
    out.backgroundBrush = QBrush(QColor(21, 21, 21));
    out.backgroundAlternateBrush = QBrush(QColor(32, 32, 32));
    out.fontColor = QColor(255, 255, 255);
    out.gridColor = QColor(114, 116, 118);
    out.canvasMargin = 15;
    out.zoomInWidth = 99;
    out.zoomInHeight = 99;
    out.timeAxisHeight = 60;
    out.timeOffsetLeft = 10;
    out.timeOffsetRight = 10;
    out.rangeBarColor = QmlDesigner::Theme::instance()->qmlDesignerBackgroundColorDarkAlternate();
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
    m_minTime = timeline.startKeyframe();
    m_maxTime = timeline.endKeyframe();
    std::vector<DesignTools::TreeItem *> items;
    for (auto &&target : timeline.allTargets()) {
        if (DesignTools::TreeItem *item = createTopLevelItem(timeline, target))
            items.push_back(item);
    }

    reset(items);
}

void CurveEditorModel::setCurrentFrame(int frame)
{
    if (graphicsView())
        graphicsView()->setCurrentFrame(frame, false);
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

DesignTools::ValueType typeFrom(const QmlDesigner::QmlTimelineKeyframeGroup &group)
{
    if (group.valueType() == QmlDesigner::TypeName("double")
        || group.valueType() == QmlDesigner::TypeName("real")
        || group.valueType() == QmlDesigner::TypeName("float"))
        return DesignTools::ValueType::Double;

    if (group.valueType() == QmlDesigner::TypeName("boolean")
        || group.valueType() == QmlDesigner::TypeName("bool"))
        return DesignTools::ValueType::Bool;

    if (group.valueType() == QmlDesigner::TypeName("integer")
        || group.valueType() == QmlDesigner::TypeName("int"))
        return DesignTools::ValueType::Integer;

    // Ignoring: QColor / HAlignment / VAlignment
    return DesignTools::ValueType::Undefined;
}

DesignTools::TreeItem *CurveEditorModel::createTopLevelItem(const QmlDesigner::QmlTimeline &timeline,
                                                            const QmlDesigner::ModelNode &node)
{
    if (!node.isValid())
        return nullptr;

    auto *nodeItem = new DesignTools::NodeTreeItem(node.id(), QIcon(":/ICON_INSTANCE"));
    for (auto &&grp : timeline.keyframeGroupsForTarget(node)) {
        if (grp.isValid()) {
            DesignTools::AnimationCurve curve = createAnimationCurve(grp);
            if (curve.isValid()) {
                QString name = QString::fromUtf8(grp.propertyName());
                auto propertyItem = new DesignTools::PropertyTreeItem(name, curve, typeFrom(grp));

                QmlDesigner::ModelNode target = grp.modelNode();
                if (target.hasAuxiliaryData("locked"))
                    propertyItem->setLocked(true);

                if (target.hasAuxiliaryData("pinned"))
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

DesignTools::AnimationCurve CurveEditorModel::createAnimationCurve(
    const QmlDesigner::QmlTimelineKeyframeGroup &group)
{
    switch (typeFrom(group)) {
    case DesignTools::ValueType::Bool:
        return createDoubleCurve(group);

    case DesignTools::ValueType::Integer:
        return createDoubleCurve(group);

    case DesignTools::ValueType::Double:
        return createDoubleCurve(group);

    default:
        return DesignTools::AnimationCurve();
    }
}

std::vector<DesignTools::Keyframe> createKeyframes(QList<QmlDesigner::ModelNode> nodes)
{
    auto byTime = [](const auto &a, const auto &b) {
        return a.variantProperty("frame").value().toDouble()
               < b.variantProperty("frame").value().toDouble();
    };
    std::sort(nodes.begin(), nodes.end(), byTime);

    std::vector<DesignTools::Keyframe> frames;
    for (auto &&node : nodes) {
        QVariant timeVariant = node.variantProperty("frame").value();
        QVariant valueVariant = node.variantProperty("value").value();
        if (!timeVariant.isValid() || !valueVariant.isValid())
            continue;

        QPointF position(timeVariant.toDouble(), valueVariant.toDouble());

        auto keyframe = DesignTools::Keyframe(position);

        if (node.hasBindingProperty("easing.bezierCurve")) {
            QmlDesigner::EasingCurve ecurve;
            ecurve.fromString(node.bindingProperty("easing.bezierCurve").expression());
            keyframe.setData(static_cast<QEasingCurve>(ecurve));
        }
        frames.push_back(keyframe);
    }
    return frames;
}

std::vector<DesignTools::Keyframe> resolveSmallCurves(const std::vector<DesignTools::Keyframe> &frames)
{
    std::vector<DesignTools::Keyframe> out;
    for (auto &&frame : frames) {
        if (frame.hasData() && !out.empty()) {
            QEasingCurve curve = frame.data().toEasingCurve();
            // One-segment-curve: Since (0,0) is implicit => 3
            if (curve.toCubicSpline().count() == 3) {
                DesignTools::Keyframe &previous = out.back();
#if 0
                // Do not resolve when two adjacent keyframes have the same value.
                if (qFuzzyCompare(previous.position().y(), frame.position().y())) {
                    out.push_back(frame);
                    continue;
                }
#endif
                DesignTools::AnimationCurve acurve(curve, previous.position(), frame.position());
                previous.setRightHandle(acurve.keyframeAt(0).rightHandle());
                out.push_back(acurve.keyframeAt(1));
                continue;
            }
        }
        out.push_back(frame);
    }
    return out;
}

DesignTools::AnimationCurve CurveEditorModel::createDoubleCurve(
    const QmlDesigner::QmlTimelineKeyframeGroup &group)
{
    std::vector<DesignTools::Keyframe> keyframes = createKeyframes(group.keyframePositions());
    keyframes = resolveSmallCurves(keyframes);

    QString str;
    QmlDesigner::ModelNode target = group.modelNode();
    if (target.hasAuxiliaryData("unified"))
        str = target.auxiliaryData("unified").toString();

    if (str.size() == static_cast<int>(keyframes.size())) {
        for (int i = 0; i < str.size(); ++i) {
            if (str.at(i) == '1')
                keyframes[static_cast<size_t>(i)].setUnified(true);
        }
    }

    return DesignTools::AnimationCurve(keyframes);
}

} // End namespace DesignTools.
