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

TimelineToolDelegate::TimelineToolDelegate(TimelineGraphicsScene *scene)
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
    if (m_currentTool)
        m_currentTool->mouseDoubleClickEvent(item, event);

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
