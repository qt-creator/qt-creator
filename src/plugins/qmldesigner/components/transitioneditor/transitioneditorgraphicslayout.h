/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#pragma once

#include "timelineitem.h"

QT_FORWARD_DECLARE_CLASS(QGraphicsLinearLayout)

namespace QmlDesigner {

class TimelineItem;
class TimelineRulerSectionItem;
class TimelinePlaceholder;

class ModelNode;

class TransitionEditorGraphicsLayout : public TimelineItem
{
    Q_OBJECT

signals:
    void rulerClicked(const QPointF &pos);

    void scaleFactorChanged(int factor);

public:
    TransitionEditorGraphicsLayout(QGraphicsScene *scene, TimelineItem *parent = nullptr);

    ~TransitionEditorGraphicsLayout() override;

public:
    double rulerWidth() const;

    double rulerScaling() const;

    double rulerDuration() const;

    double endFrame() const;

    void setWidth(int width);

    void setTransition(const ModelNode &transition);

    void setDuration(qreal duration);

    void setRulerScaleFactor(int factor);

    void invalidate();

    int maximumScrollValue() const;

    void activate();

    TimelineRulerSectionItem *ruler() const;

private:
    QGraphicsLinearLayout *m_layout = nullptr;

    TimelineRulerSectionItem *m_rulerItem = nullptr;

    TimelinePlaceholder *m_placeholder1 = nullptr;

    TimelinePlaceholder *m_placeholder2 = nullptr;
};

} // namespace QmlDesigner
