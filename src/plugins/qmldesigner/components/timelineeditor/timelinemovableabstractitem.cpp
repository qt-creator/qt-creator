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

} // namespace QmlDesigner
