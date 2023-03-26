// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
