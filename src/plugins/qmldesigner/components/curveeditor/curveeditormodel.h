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

class CurveEditorModel : public TreeModel
{
    Q_OBJECT

signals:
    void commitCurrentFrame(int frame);

    void commitStartFrame(int frame);

    void commitEndFrame(int frame);

    void timelineChanged(bool valid);

    void curveChanged(TreeItem *item);

public:
    CurveEditorModel(QObject *parent = nullptr);

    ~CurveEditorModel() override;

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

    AnimationCurve createDoubleCurve(const QmlDesigner::QmlTimelineKeyframeGroup &group);

    bool m_hasTimeline = false;

    double m_minTime = 0.;

    double m_maxTime = 0.;
};

} // End namespace QmlDesigner.
