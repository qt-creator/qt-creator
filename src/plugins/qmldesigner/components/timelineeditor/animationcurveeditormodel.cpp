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

#include "animationcurveeditormodel.h"
#include "curveeditor/curveeditorstyle.h"
#include "curveeditor/treeitem.h"
#include "easingcurve.h"
#include "qmltimeline.h"

#include <bindingproperty.h>
#include <theme.h>
#include <variantproperty.h>

namespace QmlDesigner {

AnimationCurveEditorModel::AnimationCurveEditorModel(double minTime, double maxTime)
    : CurveEditorModel(minTime, maxTime)
{}

AnimationCurveEditorModel::~AnimationCurveEditorModel() {}

double AnimationCurveEditorModel::minimumTime() const
{
    return m_minTime;
}

double AnimationCurveEditorModel::maximumTime() const
{
    return m_maxTime;
}

DesignTools::CurveEditorStyle AnimationCurveEditorModel::style() const
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
    out.rangeBarColor = Theme::instance()->qmlDesignerBackgroundColorDarkAlternate();
    out.rangeBarCapsColor = Theme::getColor(Theme::QmlDesigner_HighlightColor);
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

void AnimationCurveEditorModel::setTimeline(const QmlTimeline &timeline)
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

DesignTools::ValueType typeFrom(const QmlTimelineKeyframeGroup &group)
{
    if (group.valueType() == TypeName("double") || group.valueType() == TypeName("real")
        || group.valueType() == TypeName("float"))
        return DesignTools::ValueType::Double;

    if (group.valueType() == TypeName("boolean") || group.valueType() == TypeName("bool"))
        return DesignTools::ValueType::Bool;

    if (group.valueType() == TypeName("integer") || group.valueType() == TypeName("int"))
        return DesignTools::ValueType::Integer;

    // Ignoring: QColor / HAlignment / VAlignment
    return DesignTools::ValueType::Undefined;
}

DesignTools::TreeItem *AnimationCurveEditorModel::createTopLevelItem(const QmlTimeline &timeline,
                                                                     const ModelNode &node)
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

                ModelNode target = grp.modelNode();
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

DesignTools::AnimationCurve AnimationCurveEditorModel::createAnimationCurve(
    const QmlTimelineKeyframeGroup &group)
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

std::vector<DesignTools::Keyframe> createKeyframes(QList<ModelNode> nodes)
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
            EasingCurve ecurve;
            ecurve.fromString(node.bindingProperty("easing.bezierCurve").expression());
            keyframe.setData(static_cast<QEasingCurve>(ecurve));
        }
        frames.push_back(keyframe);
    }
    return frames;
}

std::vector<DesignTools::Keyframe> resolveSmallCurves(
    const std::vector<DesignTools::Keyframe> &frames)
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

DesignTools::AnimationCurve AnimationCurveEditorModel::createDoubleCurve(
    const QmlTimelineKeyframeGroup &group)
{
    std::vector<DesignTools::Keyframe> keyframes = createKeyframes(group.keyframePositions());
    keyframes = resolveSmallCurves(keyframes);

    QString str;
    ModelNode target = group.modelNode();
    if (target.hasAuxiliaryData("unified"))
        str = target.auxiliaryData("unified").toString();

    if (str.size() == static_cast<int>(keyframes.size())) {
        for (int i = 0; i < str.size(); ++i) {
            if (str.at(i) == '1')
                keyframes[i].setUnified(true);
        }
    }

    return DesignTools::AnimationCurve(keyframes);
}

} // namespace QmlDesigner
