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

#include "timelineconstants.h"

#include <QCoreApplication>
#include <QGraphicsRectItem>

namespace QmlDesigner {

class AbstractScrollGraphicsScene;
class TimelineKeyframeItem;
class TimelineFrameHandle;

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

protected:
    int scrollOffset() const;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    void setClampedXPosition(qreal x, qreal min, qreal max);
    AbstractScrollGraphicsScene *abstractScrollGraphicsScene() const;
};

} // namespace QmlDesigner
