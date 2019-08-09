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
#include <variantproperty.h>

namespace QmlDesigner {

AnimationCurveEditorModel::AnimationCurveEditorModel(double minTime, double maxTime)
    : CurveEditorModel()
    , m_minTime(minTime)
    , m_maxTime(maxTime)
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
    out.backgroundBrush = QBrush(QColor(55, 55, 55));
    out.backgroundAlternateBrush = QBrush(QColor(0, 0, 50));
    out.fontColor = QColor(255, 255, 255);
    out.gridColor = QColor(114, 116, 118);
    out.canvasMargin = 15;
    out.zoomInWidth = 99;
    out.zoomInHeight = 99;
    out.timeAxisHeight = 40;
    out.timeOffsetLeft = 10;
    out.timeOffsetRight = 10;
    out.rangeBarColor = QColor(46, 47, 48);
    out.rangeBarCapsColor = QColor(50, 50, 255);
    out.valueAxisWidth = 60;
    out.valueOffsetTop = 10;
    out.valueOffsetBottom = 10;
    out.handleStyle.size = 12;
    out.handleStyle.lineWidth = 1;
    out.handleStyle.color = QColor(255, 255, 255);
    out.handleStyle.selectionColor = QColor(255, 255, 255);
    out.keyframeStyle.size = 13;
    out.keyframeStyle.color = QColor(172, 210, 255);
    out.keyframeStyle.selectionColor = QColor(255, 255, 255);
    out.curveStyle.width = 1;
    out.curveStyle.color = QColor(0, 200, 0);
    out.curveStyle.selectionColor = QColor(255, 255, 255);
    return out;
}

void AnimationCurveEditorModel::setTimeline(const QmlTimeline &timeline)
{
    m_minTime = timeline.startKeyframe();
    m_maxTime = timeline.endKeyframe();

    std::vector<DesignTools::TreeItem *> items;
    for (auto &&target : timeline.allTargets())
        if (DesignTools::TreeItem *item = createTopLevelItem(timeline, target))
            items.push_back(item);

    reset(items);
}

void AnimationCurveEditorModel::setMinimumTime(double time)
{
    m_minTime = time;
}

void AnimationCurveEditorModel::setMaximumTime(double time)
{
    m_maxTime = time;
}

DesignTools::ValueType typeFrom(const QmlTimelineKeyframeGroup &group)
{
    if (group.valueType() == TypeName("double") || group.valueType() == TypeName("real"))
        return DesignTools::ValueType::Double;

    if (group.valueType() == TypeName("bool"))
        return DesignTools::ValueType::Bool;

    if (group.valueType() == TypeName("integer"))
        return DesignTools::ValueType::Integer;

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
                nodeItem->addChild(new DesignTools::PropertyTreeItem(name, curve, typeFrom(grp)));
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

DesignTools::AnimationCurve AnimationCurveEditorModel::createDoubleCurve(
    const QmlTimelineKeyframeGroup &group)
{
    std::vector<DesignTools::Keyframe> keyframes;
    for (auto &&frame : group.keyframePositions()) {
        QVariant timeVariant = frame.variantProperty("frame").value();
        QVariant valueVariant = frame.variantProperty("value").value();

        if (timeVariant.isValid() && valueVariant.isValid()) {
            QPointF position(timeVariant.toDouble(), valueFromVariant(valueVariant));
            auto keyframe = DesignTools::Keyframe(position);

            if (frame.hasBindingProperty("easing.bezierCurve")) {
                EasingCurve ecurve;
                ecurve.fromString(frame.bindingProperty("easing.bezierCurve").expression());
                keyframe.setData(static_cast<QEasingCurve>(ecurve));
            }

            keyframes.push_back(keyframe);
        }
    }
    return DesignTools::AnimationCurve(keyframes);
}

double AnimationCurveEditorModel::valueFromVariant(const QVariant &variant)
{
    return variant.toDouble();
}

void AnimationCurveEditorModel::reset(const std::vector<DesignTools::TreeItem *> &items)
{
    beginResetModel();

    initialize();

    unsigned int counter = 0;
    for (auto *item : items) {
        item->setId(++counter);
        root()->addChild(item);
    }

    endResetModel();
}

} // namespace QmlDesigner
