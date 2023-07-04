// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

void TimelineSelectionTool::mousePressEvent([[maybe_unused]] TimelineMovableAbstractItem *item,
                                            [[maybe_unused]] QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton && selectionMode(event) == SelectionMode::New)
        deselect();
}

void TimelineSelectionTool::mouseMoveEvent([[maybe_unused]] TimelineMovableAbstractItem *item,
                                           QGraphicsSceneMouseEvent *event)
{
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

void TimelineSelectionTool::mouseReleaseEvent([[maybe_unused]] TimelineMovableAbstractItem *item,
                                              QGraphicsSceneMouseEvent *event)
{
    commitSelection(selectionMode(event));

    reset();
}

void TimelineSelectionTool::mouseDoubleClickEvent(TimelineMovableAbstractItem *item,
                                                  [[maybe_unused]] QGraphicsSceneMouseEvent *event)
{
    if (item)
        item->itemDoubleClicked();

    reset();
}

void TimelineSelectionTool::keyPressEvent([[maybe_unused]] QKeyEvent *keyEvent) {}

void TimelineSelectionTool::keyReleaseEvent([[maybe_unused]] QKeyEvent *keyEvent) {}

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
    for (auto *keyframe : std::as_const(m_aboutToSelectBuffer))
        if (scene()->isKeyframeSelected(keyframe))
            keyframe->setHighlighted(true);
        else
            keyframe->setHighlighted(false);

    m_aboutToSelectBuffer.clear();
}

void TimelineSelectionTool::aboutToSelect(SelectionMode mode, QList<QGraphicsItem *> items)
{
    resetHighlights();
    m_playbackLoopTimeSteps.clear();

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

            if (mode == SelectionMode::Toggle || mode == SelectionMode::Add)  // TODO: Timeline keyframe highlight with selection tool is set as or added to loop range. Select shortcut for this QDS-4941
                m_playbackLoopTimeSteps << keyframe->mapFromSceneToFrame((keyframe->rect().center() + item->scenePos()).x());

            m_aboutToSelectBuffer << keyframe;
        } else if (auto *barItem = TimelineMovableAbstractItem::asTimelineBarItem(item)) {
            QRectF rect = barItem->rect();
            QPointF center = rect.center() + item->scenePos();
            QPointF left = QPointF(rect.left(), 0) + item->scenePos();
            QPointF right = QPointF(rect.right(), 0) + item->scenePos();
            if (mode == SelectionMode::Add) {  // TODO: Timeline bar item highlight with selection tool is added to loop range. Select shortcut for this QDS-4941
                if (m_selectionRect->rect().contains(left))
                    m_playbackLoopTimeSteps << barItem->mapFromSceneToFrame(left.x());
                if (m_selectionRect->rect().contains(right))
                    m_playbackLoopTimeSteps << barItem->mapFromSceneToFrame(right.x());
                if (m_selectionRect->rect().contains(center))
                    m_playbackLoopTimeSteps << barItem->mapFromSceneToFrame(center.x());
            } else if (mode == SelectionMode::Toggle && m_selectionRect->rect().contains(center)) { // TODO: Timeline bar item highlight with selection tool is set as loop range. Select shortcut for this QDS-4941
                m_playbackLoopTimeSteps << barItem->mapFromSceneToFrame(left.x());
                m_playbackLoopTimeSteps << barItem->mapFromSceneToFrame(right.x());
            }
        }
    }
}

void TimelineSelectionTool::commitSelection(SelectionMode mode)
{
    if (m_playbackLoopTimeSteps.size())
        qobject_cast<TimelineGraphicsScene *>(scene())->layoutRuler()->extendPlaybackLoop(m_playbackLoopTimeSteps,
                                                                                          mode == SelectionMode::Toggle);  // TODO: Highlighting items with selection tool is set or added to loop range. Select shortcut for this QDS-4941
    scene()->selectKeyframes(mode, m_aboutToSelectBuffer);
    m_aboutToSelectBuffer.clear();
    m_playbackLoopTimeSteps.clear();
}

} // End namespace QmlDesigner.
