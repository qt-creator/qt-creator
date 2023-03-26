// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinemovetool.h"

#include "timelinegraphicsscene.h"
#include "timelinemovableabstractitem.h"
#include "timelinepropertyitem.h"
#include "timelineview.h"

#include <exception.h>

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

#include <cmath>

namespace QmlDesigner {

static QPointF mapPointToItem(TimelineMovableAbstractItem *item, const QPointF &pos)
{
    if (auto parent = item->parentItem())
        return parent->mapFromScene(pos);
    return pos;
}

QPointF mapToItem(TimelineMovableAbstractItem *item, const QPointF &pos)
{
    if (auto parent = item->parentItem())
        return parent->mapFromScene(pos);
    return pos;
}

QPointF mapToItem(TimelineMovableAbstractItem *item, QGraphicsSceneMouseEvent *event)
{
    if (auto parent = item->parentItem())
        return parent->mapFromScene(event->scenePos());
    return event->scenePos();
}

TimelineMoveTool::TimelineMoveTool(AbstractScrollGraphicsScene *scene, TimelineToolDelegate *delegate)
    : TimelineAbstractTool(scene, delegate)
{}

void TimelineMoveTool::mousePressEvent([[maybe_unused]] TimelineMovableAbstractItem *item,
                                       QGraphicsSceneMouseEvent *event)
{
    if (currentItem() && currentItem()->isLocked())
        return;

    TimelineGraphicsScene *graphicsScene = qobject_cast<TimelineGraphicsScene *>(scene());
    if (event->modifiers().testFlag(Qt::ControlModifier) && graphicsScene) { // TODO: Timeline bar animation is set as loop range. Select shortcut for this QDS-4941
        if (auto *current = currentItem()->asTimelineBarItem()) {
            qreal left = qRound(current->mapFromSceneToFrame(current->rect().left()));
            qreal right = qRound(current->mapFromSceneToFrame(current->rect().right()));
            const QList<qreal> positions = {left, right};
            graphicsScene->layoutRuler()->extendPlaybackLoop(positions, event->modifiers().testFlag(Qt::ShiftModifier));
        }
    }

    if (auto *current = currentItem()->asTimelineKeyframeItem()) {
        const qreal sourceFrame = qRound(current->mapFromSceneToFrame(current->rect().center().x()));
        const qreal targetFrame = qRound(current->mapFromSceneToFrame(event->scenePos().x()));
        m_pressKeyframeDelta = targetFrame - sourceFrame;

        if (event->modifiers().testFlag(Qt::ControlModifier) && graphicsScene) {
            const QList<qreal> positions = {sourceFrame};
            graphicsScene->layoutRuler()->extendPlaybackLoop(positions, false);
        }
    }
}

void TimelineMoveTool::mouseMoveEvent([[maybe_unused]] TimelineMovableAbstractItem *item,
                                      QGraphicsSceneMouseEvent *event)
{
    if (!currentItem())
        return;

    if (currentItem()->isLocked())
        return;

    if (auto *current = currentItem()->asTimelineKeyframeItem()) {
        // prevent dragging if deselecting a keyframe (Ctrl+click and drag a selected keyframe)
        if (!current->highlighted())
            return;

        const qreal sourceFrame = qRound(current->mapFromSceneToFrame(current->rect().center().x()));
        qreal targetFrame = qRound(current->mapFromSceneToFrame(event->scenePos().x()));
        qreal deltaFrame = targetFrame - sourceFrame - m_pressKeyframeDelta;

        const qreal minFrame = scene()->startFrame();
        const qreal maxFrame = scene()->endFrame();

        auto bbox = scene()->selectionBounds().adjusted(TimelineConstants::keyFrameSize / 2, 0,
                                                        -TimelineConstants::keyFrameSize / 2, 0);
        double firstFrame = std::round(current->mapFromSceneToFrame(bbox.left()));
        double lastFrame = std::round(current->mapFromSceneToFrame(bbox.right()));

        if (lastFrame + deltaFrame > maxFrame)
            deltaFrame = maxFrame - lastFrame;
        else if (firstFrame + deltaFrame < minFrame)
            deltaFrame = minFrame - firstFrame;

        targetFrame = sourceFrame + deltaFrame;

        if (QApplication::keyboardModifiers() & Qt::ShiftModifier) { // keyframe snapping
            qreal snappedTargetFrame = scene()->snap(targetFrame);
            deltaFrame += snappedTargetFrame - targetFrame;
            targetFrame = snappedTargetFrame;
        }

        emit scene()->statusBarMessageChanged(tr(TimelineConstants::statusBarKeyframe)
                                              .arg(targetFrame));

        const QList<TimelineKeyframeItem *> selectedKeyframes = scene()->selectedKeyframes();
        for (auto *keyframe : selectedKeyframes) {
            qreal pos = std::round(keyframe->mapFromSceneToFrame(keyframe->rect().center().x()));
            keyframe->setPosition(pos + deltaFrame);
        }

    } else {
        currentItem()->itemMoved(mapPointToItem(currentItem(), startPosition()),
                                 mapToItem(currentItem(), event));
    }
}

void TimelineMoveTool::mouseReleaseEvent([[maybe_unused]] TimelineMovableAbstractItem *item,
                                         QGraphicsSceneMouseEvent *event)
{
    if (auto *current = currentItem()) {
        if (current->asTimelineFrameHandle()) {
            double mousePos = event->scenePos().x();
            double start = current->mapFromFrameToScene(scene()->startFrame());
            double end = current->mapFromFrameToScene(scene()->endFrame());

            double limitFrame = -999999.;
            if (mousePos < start)
                limitFrame = scene()->startFrame();
            else if (mousePos > end)
                limitFrame = scene()->endFrame();

            if (limitFrame > -999999.) {
                scene()->setCurrentFrame(limitFrame);
                emit scene()->statusBarMessageChanged(
                            tr(TimelineConstants::statusBarPlayheadFrame).arg(limitFrame));
                return;
            }
        }

        scene()->abstractView()->executeInTransaction("TimelineMoveTool::mouseReleaseEvent",
                                                      [this, current]() {
            current->commitPosition(mapToItem(current, current->rect().center()));

            if (current->asTimelineKeyframeItem()) {
                double frame = std::round(
                            current->mapFromSceneToFrame(current->rect().center().x()));

                scene()->statusBarMessageChanged(
                            tr(TimelineConstants::statusBarKeyframe).arg(frame));

                const auto selectedKeyframes = scene()->selectedKeyframes();
                for (auto keyframe : selectedKeyframes) {
                    if (keyframe != current)
                        keyframe->commitPosition(mapToItem(current, keyframe->rect().center()));
                }
            }
        });
    }
}

void TimelineMoveTool::mouseDoubleClickEvent([[maybe_unused]] TimelineMovableAbstractItem *item,
                                             [[maybe_unused]] QGraphicsSceneMouseEvent *event)
{
}

void TimelineMoveTool::keyPressEvent([[maybe_unused]] QKeyEvent *keyEvent) {}

void TimelineMoveTool::keyReleaseEvent([[maybe_unused]] QKeyEvent *keyEvent) {}

} // namespace QmlDesigner
