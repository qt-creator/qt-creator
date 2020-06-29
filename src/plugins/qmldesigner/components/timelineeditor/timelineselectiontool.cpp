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

#include "timelineselectiontool.h"
#include "timelineconstants.h"
#include "timelinegraphicsscene.h"
#include "timelinemovableabstractitem.h"
#include "timelinepropertyitem.h"
#include "timelinetooldelegate.h"

#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QGraphicsSceneMouseEvent>
#include <QScrollBar>
#include <QPen>

namespace QmlDesigner {

TimelineSelectionTool::TimelineSelectionTool(AbstractScrollGraphicsScene *scene,
                                             TimelineToolDelegate *delegate)
    : TimelineAbstractTool(scene, delegate)
    , m_selectionRect(new QGraphicsRectItem)
{
    scene->addItem(m_selectionRect);
    QPen pen(Qt::black);
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setCosmetic(true);
    m_selectionRect->setPen(pen);
    m_selectionRect->setBrush(QColor(128, 128, 128, 50));
    m_selectionRect->setZValue(100);
    m_selectionRect->hide();
}

TimelineSelectionTool::~TimelineSelectionTool() = default;

SelectionMode TimelineSelectionTool::selectionMode(QGraphicsSceneMouseEvent *event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        if (event->modifiers().testFlag(Qt::ShiftModifier))
            return SelectionMode::Add;
        else
            return SelectionMode::Toggle;
    }

    return SelectionMode::New;
}

void TimelineSelectionTool::mousePressEvent(TimelineMovableAbstractItem *item,
                                            QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(item)
    Q_UNUSED(event)

    if (event->buttons() == Qt::LeftButton && selectionMode(event) == SelectionMode::New)
        deselect();
}

void TimelineSelectionTool::mouseMoveEvent(TimelineMovableAbstractItem *item,
                                           QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(item)

    if (event->buttons() == Qt::LeftButton) {
        auto endPoint = event->scenePos();

        const qreal xMin = TimelineConstants::sectionWidth;
        const qreal xMax = scene()->graphicsView()->width()
                           - TimelineConstants::timelineLeftOffset - 1;
        const qreal yMin = qMax(TimelineConstants::rulerHeight,
                                scene()->graphicsView()->verticalScrollBar()->value());
        const qreal yMax = yMin + scene()->graphicsView()->height() - 1;

        endPoint.rx() = qBound(xMin, endPoint.x(), xMax);
        endPoint.ry() = qBound(yMin, endPoint.y(), yMax);

        m_selectionRect->setRect(QRectF(startPosition(), endPoint).normalized());
        m_selectionRect->show();

        aboutToSelect(selectionMode(event),
                      scene()->items(m_selectionRect->rect(), Qt::IntersectsItemBoundingRect));
    }
}

void TimelineSelectionTool::mouseReleaseEvent(TimelineMovableAbstractItem *item,
                                              QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(item)

    commitSelection(selectionMode(event));

    reset();
}

void TimelineSelectionTool::mouseDoubleClickEvent(TimelineMovableAbstractItem *item,
                                                  QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)

    if (item)
        item->itemDoubleClicked();

    reset();
}

void TimelineSelectionTool::keyPressEvent(QKeyEvent *keyEvent)
{
    Q_UNUSED(keyEvent)
}

void TimelineSelectionTool::keyReleaseEvent(QKeyEvent *keyEvent)
{
    Q_UNUSED(keyEvent)
}

void TimelineSelectionTool::deselect()
{
    resetHighlights();
    scene()->clearSelection();
    delegate()->clearSelection();
}

void TimelineSelectionTool::reset()
{
    m_selectionRect->hide();
    m_selectionRect->setRect(0, 0, 0, 0);
    resetHighlights();
}

void TimelineSelectionTool::resetHighlights()
{
    for (auto *keyframe : m_aboutToSelectBuffer)
        if (scene()->isKeyframeSelected(keyframe))
            keyframe->setHighlighted(true);
        else
            keyframe->setHighlighted(false);

    m_aboutToSelectBuffer.clear();
}

void TimelineSelectionTool::aboutToSelect(SelectionMode mode, QList<QGraphicsItem *> items)
{
    resetHighlights();

    for (auto *item : items) {
        if (auto *keyframe = TimelineMovableAbstractItem::asTimelineKeyframeItem(item)) {
            // if keyframe's center isn't inside m_selectionRect, discard it
            if (!m_selectionRect->rect().contains(keyframe->rect().center() + item->scenePos()))
                continue;

            if (mode == SelectionMode::Remove)
                keyframe->setHighlighted(false);
            else if (mode == SelectionMode::Toggle)
                if (scene()->isKeyframeSelected(keyframe))
                    keyframe->setHighlighted(false);
                else
                    keyframe->setHighlighted(true);
            else
                keyframe->setHighlighted(true);

            m_aboutToSelectBuffer << keyframe;
        }
    }
}

void TimelineSelectionTool::commitSelection(SelectionMode mode)
{
    scene()->selectKeyframes(mode, m_aboutToSelectBuffer);
    m_aboutToSelectBuffer.clear();
}

} // End namespace QmlDesigner.
