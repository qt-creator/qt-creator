// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
