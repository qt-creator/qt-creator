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

#include "timelinemovetool.h"
#include "timelineselectiontool.h"

#include <memory>

namespace QmlDesigner
{

class TimelineGraphicsScene;

class TimelineToolDelegate
{
public:
    TimelineToolDelegate(AbstractScrollGraphicsScene* scene);

    QPointF startPoint() const;

    TimelineMovableAbstractItem* item() const;

public:
    void mousePressEvent(TimelineMovableAbstractItem *item, QGraphicsSceneMouseEvent *event);

    void mouseMoveEvent(TimelineMovableAbstractItem *item, QGraphicsSceneMouseEvent *event);

    void mouseReleaseEvent(TimelineMovableAbstractItem *item, QGraphicsSceneMouseEvent *event);

    void mouseDoubleClickEvent(TimelineMovableAbstractItem *item, QGraphicsSceneMouseEvent *event);

    void clearSelection();

private:
    bool hitCanvas(QGraphicsSceneMouseEvent *event);

    void reset();

    void setItem(TimelineMovableAbstractItem *item, const Qt::KeyboardModifiers& modifiers = Qt::NoModifier);

private:
    static const int dragDistance = 20;

    AbstractScrollGraphicsScene* m_scene;

    QPointF m_start;

    TimelineMovableAbstractItem *m_item = nullptr;

    std::unique_ptr< TimelineMoveTool > m_moveTool;

    std::unique_ptr< TimelineSelectionTool > m_selectTool;

    TimelineAbstractTool *m_currentTool = nullptr;
};

} // End namespace QmlDesigner.
