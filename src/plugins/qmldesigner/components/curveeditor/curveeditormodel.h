// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "detail/treemodel.h"

#include <qmltimelinekeyframegroup.h>

#include <vector>

QT_BEGIN_NAMESPACE
class QPointF;
QT_END_NAMESPACE

namespace QmlDesigner {

struct CurveEditorStyle;

class AnimationCurve;
class PropertyTreeItem;
class TreeItem;

inline constexpr AuxiliaryDataKeyView pinnedProperty{AuxiliaryDataType::Document, "pinned"};
inline constexpr AuxiliaryDataKeyView unifiedProperty{AuxiliaryDataType::Document, "unified"};

class CurveEditorModel : public TreeModel
{
    Q_OBJECT

signals:
    void setStatusLineMsg(const QString& msg);

    void commitCurrentFrame(int frame);

    void commitStartFrame(int frame);

    void commitEndFrame(int frame);

    void timelineChanged(bool valid);

    void curveChanged(TreeItem *item);

public:
    CurveEditorModel(QObject *parent = nullptr);

    ~CurveEditorModel() override;

    int currentFrame() const;

    double minimumTime() const;

    double maximumTime() const;

    CurveEditorStyle style() const;

public:
    void setTimeline(const QmlDesigner::QmlTimeline &timeline);

    void setCurrentFrame(int frame);

    void setMinimumTime(double time);

    void setMaximumTime(double time);

    void setCurve(unsigned int id, const AnimationCurve &curve);

    void setLocked(TreeItem *item, bool val);

    void setPinned(TreeItem *item, bool val);

    void reset(const std::vector<TreeItem *> &items);

private:
    TreeItem *createTopLevelItem(const QmlDesigner::QmlTimeline &timeline,
                                 const QmlDesigner::ModelNode &node);

    AnimationCurve createAnimationCurve(const QmlDesigner::QmlTimelineKeyframeGroup &group);

    AnimationCurve createBooleanCurve(const QmlDesigner::QmlTimelineKeyframeGroup &group);

    AnimationCurve createDoubleCurve(const QmlDesigner::QmlTimelineKeyframeGroup &group);

    bool m_hasTimeline = false;

    int m_currentFrame = 0;

    double m_minTime = 0.;

    double m_maxTime = 0.;
};

} // End namespace QmlDesigner.
