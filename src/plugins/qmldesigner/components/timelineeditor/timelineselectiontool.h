// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineabstracttool.h"

#include <QList>

QT_FORWARD_DECLARE_CLASS(QGraphicsItem)
QT_FORWARD_DECLARE_CLASS(QGraphicsRectItem)

namespace QmlDesigner {

class TimelineToolDelegate;

class TimelineKeyframeItem;
class TimelineBarItem;
class TimelineGraphicsScene;

enum class SelectionMode { New, Add, Remove, Toggle };

class TimelineSelectionTool : public TimelineAbstractTool
{
public:
    explicit TimelineSelectionTool(AbstractScrollGraphicsScene *scene, TimelineToolDelegate *delegate);

    ~TimelineSelectionTool() override;

    static SelectionMode selectionMode(QGraphicsSceneMouseEvent *event);

    void mousePressEvent(TimelineMovableAbstractItem *item,
                         QGraphicsSceneMouseEvent *event) override;

    void mouseMoveEvent(TimelineMovableAbstractItem *item, QGraphicsSceneMouseEvent *event) override;

    void mouseReleaseEvent(TimelineMovableAbstractItem *item,
                           QGraphicsSceneMouseEvent *event) override;

    void mouseDoubleClickEvent(TimelineMovableAbstractItem *item,
                               QGraphicsSceneMouseEvent *event) override;

    void keyPressEvent(QKeyEvent *keyEvent) override;

    void keyReleaseEvent(QKeyEvent *keyEvent) override;

private:
    void deselect();

    void reset();

    void resetHighlights();

    void aboutToSelect(SelectionMode mode, QList<QGraphicsItem *> items);

    void commitSelection(SelectionMode mode);

private:
    QGraphicsRectItem *m_selectionRect;

    QList<TimelineKeyframeItem *> m_aboutToSelectBuffer;
    QList<qreal> m_playbackLoopTimeSteps;
};

} // namespace QmlDesigner
