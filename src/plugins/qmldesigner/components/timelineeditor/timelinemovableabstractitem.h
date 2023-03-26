// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineconstants.h"

#include <QCoreApplication>
#include <QGraphicsRectItem>

namespace QmlDesigner {

class AbstractScrollGraphicsScene;
class TimelineKeyframeItem;
class TimelineFrameHandle;
class TimelineBarItem;

class TimelineMovableAbstractItem : public QGraphicsRectItem
{
    Q_DECLARE_TR_FUNCTIONS(TimelineMovableAbstractItem)

public:
    enum { Type = TimelineConstants::moveableAbstractItemUserType };

    explicit TimelineMovableAbstractItem(QGraphicsItem *item);

    int type() const override;

    static TimelineMovableAbstractItem *cast(QGraphicsItem *item);
    static TimelineMovableAbstractItem *topMoveableItem(const QList<QGraphicsItem *> &items);
    static void emitScrollOffsetChanged(QGraphicsItem *item);
    static TimelineKeyframeItem *asTimelineKeyframeItem(QGraphicsItem *item);
    static TimelineBarItem *asTimelineBarItem(QGraphicsItem *item);

    qreal rulerScaling() const;

    virtual void setPositionInteractive(const QPointF &point);
    virtual void commitPosition(const QPointF &point);
    virtual void itemMoved(const QPointF &start, const QPointF &end);
    virtual void itemDoubleClicked();

    int xPosScrollOffset(int x) const;

    qreal mapFromFrameToScene(qreal x) const;
    qreal mapFromSceneToFrame(qreal x) const;

    virtual void scrollOffsetChanged() = 0;

    virtual TimelineKeyframeItem *asTimelineKeyframeItem();
    virtual TimelineFrameHandle *asTimelineFrameHandle();
    virtual TimelineBarItem *asTimelineBarItem();

    virtual bool isLocked() const;

protected:
    int scrollOffset() const;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    void setClampedXPosition(qreal x, qreal min, qreal max);
    AbstractScrollGraphicsScene *abstractScrollGraphicsScene() const;
};

} // namespace QmlDesigner
