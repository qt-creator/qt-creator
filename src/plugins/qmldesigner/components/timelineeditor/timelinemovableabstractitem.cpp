// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinemovableabstractitem.h"

#include "timelinegraphicsscene.h"

#include "timelineitem.h"

#include <QGraphicsSceneMouseEvent>

namespace QmlDesigner {

TimelineMovableAbstractItem::TimelineMovableAbstractItem(QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
{}

void TimelineMovableAbstractItem::setPositionInteractive(const QPointF &) {}

void TimelineMovableAbstractItem::commitPosition(const QPointF &) {}

void TimelineMovableAbstractItem::itemMoved(const QPointF & /*start*/, const QPointF &end)
{
    setPositionInteractive(end);
}

void TimelineMovableAbstractItem::itemDoubleClicked()
{
    // to be overridden by child classes if needed
}

int TimelineMovableAbstractItem::scrollOffset() const
{
    return abstractScrollGraphicsScene()->scrollOffset();
}

int TimelineMovableAbstractItem::xPosScrollOffset(int x) const
{
    return x + scrollOffset();
}

qreal TimelineMovableAbstractItem::mapFromFrameToScene(qreal x) const
{
    return TimelineConstants::sectionWidth + (x - abstractScrollGraphicsScene()->startFrame()) * rulerScaling()
           - scrollOffset() + TimelineConstants::timelineLeftOffset;
}

qreal TimelineMovableAbstractItem::mapFromSceneToFrame(qreal x) const
{
    return xPosScrollOffset(x - TimelineConstants::sectionWidth
                            - TimelineConstants::timelineLeftOffset)
               / abstractScrollGraphicsScene()->rulerScaling()
           + abstractScrollGraphicsScene()->startFrame();
}

void TimelineMovableAbstractItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
}

void TimelineMovableAbstractItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
}

void TimelineMovableAbstractItem::setClampedXPosition(qreal x,
                                                      qreal minimumWidth,
                                                      qreal maximumWidth)
{
    if (x > minimumWidth) {
        if (x < maximumWidth)
            setRect(x, rect().y(), rect().width(), rect().height());
        else
            setRect(maximumWidth, rect().y(), rect().width(), rect().height());
    } else {
        setRect(minimumWidth, rect().y(), rect().width(), rect().height());
    }
}

TimelineMovableAbstractItem *TimelineMovableAbstractItem::cast(QGraphicsItem *item)
{
    return qgraphicsitem_cast<TimelineMovableAbstractItem *>(item);
}

TimelineMovableAbstractItem *TimelineMovableAbstractItem::topMoveableItem(
    const QList<QGraphicsItem *> &items)
{
    for (auto item : items)
        if (auto castedItem = TimelineMovableAbstractItem::cast(item))
            return castedItem;

    return nullptr;
}

void TimelineMovableAbstractItem::emitScrollOffsetChanged(QGraphicsItem *item)
{
    auto movableItem = TimelineMovableAbstractItem::cast(item);
    if (movableItem)
        movableItem->scrollOffsetChanged();
}

TimelineKeyframeItem *TimelineMovableAbstractItem::asTimelineKeyframeItem(QGraphicsItem *item)
{
    auto movableItem = TimelineMovableAbstractItem::cast(item);

    if (movableItem)
        return movableItem->asTimelineKeyframeItem();

    return nullptr;
}

TimelineBarItem *TimelineMovableAbstractItem::asTimelineBarItem(QGraphicsItem *item)
{
    auto movableItem = TimelineMovableAbstractItem::cast(item);

    if (movableItem)
        return movableItem->asTimelineBarItem();

    return nullptr;
}

qreal TimelineMovableAbstractItem::rulerScaling() const
{
    return qobject_cast<AbstractScrollGraphicsScene *>(scene())->rulerScaling();
}

int TimelineMovableAbstractItem::type() const
{
    return Type;
}

AbstractScrollGraphicsScene *TimelineMovableAbstractItem::abstractScrollGraphicsScene() const
{
    return qobject_cast<AbstractScrollGraphicsScene *>(scene());
}

TimelineKeyframeItem *TimelineMovableAbstractItem::asTimelineKeyframeItem()
{
    return nullptr;
}

TimelineFrameHandle *TimelineMovableAbstractItem::asTimelineFrameHandle()
{
    return nullptr;
}


TimelineBarItem *TimelineMovableAbstractItem::asTimelineBarItem()
{
    return nullptr;
}

bool TimelineMovableAbstractItem::isLocked() const
{
    return false;
}

} // namespace QmlDesigner
