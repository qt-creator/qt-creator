// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinetooldelegate.h"

#include "timelineconstants.h"
#include "timelinegraphicsscene.h"
#include "timelinemovableabstractitem.h"
#include "timelinemovetool.h"
#include "timelineselectiontool.h"

#include <QGraphicsSceneMouseEvent>

#include "timelinepropertyitem.h"
#include <QDebug>

namespace QmlDesigner {

TimelineToolDelegate::TimelineToolDelegate(AbstractScrollGraphicsScene *scene)
    : m_scene(scene)
    , m_start()
    , m_moveTool(new TimelineMoveTool(scene, this))
    , m_selectTool(new TimelineSelectionTool(scene, this))
{}

QPointF TimelineToolDelegate::startPoint() const
{
    return m_start;
}

TimelineMovableAbstractItem *TimelineToolDelegate::item() const
{
    return m_item;
}

void TimelineToolDelegate::mousePressEvent(TimelineMovableAbstractItem *item,
                                           QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton && hitCanvas(event)) {

        m_start = event->scenePos();
        if (item) {
            setItem(item, event->modifiers());
            m_currentTool = m_moveTool.get();
        } else
            m_currentTool = m_selectTool.get();

    } else if (event->buttons() == Qt::RightButton && event->modifiers() == Qt::NoModifier
               && hitCanvas(event) && item) {

        setItem(item, Qt::NoModifier);
        reset();

    } else
        m_currentTool = nullptr;

    if (m_currentTool)
        m_currentTool->mousePressEvent(item, event);
}

void TimelineToolDelegate::mouseMoveEvent(TimelineMovableAbstractItem *item,
                                          QGraphicsSceneMouseEvent *event)
{
    if (m_currentTool)
        m_currentTool->mouseMoveEvent(item, event);
}

void TimelineToolDelegate::mouseReleaseEvent(TimelineMovableAbstractItem *item,
                                             QGraphicsSceneMouseEvent *event)
{
    if (m_currentTool)
        m_currentTool->mouseReleaseEvent(item, event);

    reset();
}

void TimelineToolDelegate::mouseDoubleClickEvent(TimelineMovableAbstractItem *item,
                                                 QGraphicsSceneMouseEvent *event)
{
    if (hitCanvas(event)) {
        m_currentTool = m_selectTool.get();
        m_currentTool->mouseDoubleClickEvent(item, event);
    }

    reset();
}

void TimelineToolDelegate::clearSelection()
{
    if (auto *keyframe = TimelineMovableAbstractItem::asTimelineKeyframeItem(m_item))
        keyframe->setHighlighted(false);

    m_item = nullptr;
}

void TimelineToolDelegate::setItem(TimelineMovableAbstractItem *item,
                                   const Qt::KeyboardModifiers &modifiers)
{
    if (item) {
        setItem(nullptr);

        if (auto *keyframe = TimelineMovableAbstractItem::asTimelineKeyframeItem(item)) {
            if (modifiers.testFlag(Qt::ControlModifier)) {
                if (m_scene->isKeyframeSelected(keyframe))
                    m_scene->selectKeyframes(SelectionMode::Remove, {keyframe});
                else
                    m_scene->selectKeyframes(SelectionMode::Add, {keyframe});
            } else {
                if (!m_scene->isKeyframeSelected(keyframe))
                    m_scene->selectKeyframes(SelectionMode::New, {keyframe});
            }
        }

    } else if (m_item) {
        if (auto *keyframe = TimelineMovableAbstractItem::asTimelineKeyframeItem(m_item))
            if (!m_scene->isKeyframeSelected(keyframe))
                keyframe->setHighlighted(false);
    }

    m_item = item;
}

bool TimelineToolDelegate::hitCanvas(QGraphicsSceneMouseEvent *event)
{
    return event->scenePos().x() > TimelineConstants::sectionWidth;
}

void TimelineToolDelegate::reset()
{
    setItem(nullptr);

    m_currentTool = nullptr;

    m_start = QPointF();
}

} // End namespace QmlDesigner.
