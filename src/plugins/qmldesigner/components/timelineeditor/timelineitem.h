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

#pragma once

#include "timelinemovableabstractitem.h"

#include <QGraphicsRectItem>
#include <QGraphicsWidget>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QPainterPath)

namespace QmlDesigner {

class TimelineGraphicsScene;

class TimelineItem : public QGraphicsWidget
{
    Q_OBJECT

public:
    explicit TimelineItem(TimelineItem *parent = nullptr);

    static void drawLine(QPainter *painter, qreal x1, qreal y1, qreal x2, qreal y2);
    TimelineGraphicsScene *timelineScene() const;
};

class TimelineFrameHandle : public TimelineMovableAbstractItem
{
public:
    explicit TimelineFrameHandle(TimelineItem *parent = nullptr);

    void setHeight(int height);
    void setPosition(qreal frame);
    void setPositionInteractive(const QPointF &postion) override;
    void commitPosition(const QPointF &point) override;
    qreal position() const;

    TimelineFrameHandle *asTimelineFrameHandle() override;

    TimelineGraphicsScene *timelineGraphicsScene() const;

protected:
    void scrollOffsetChanged() override;
    QPainterPath shape() const override;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

private:
    QPointF mapFromGlobal(const QPoint &pos) const;
    int computeScrollSpeed() const;

    void callSetClampedXPosition(double x);
    void scrollOutOfBounds();
    void scrollOutOfBoundsMax();
    void scrollOutOfBoundsMin();

private:
    qreal m_position = 0;

    QTimer m_timer;
};

} // namespace QmlDesigner
