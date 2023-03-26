// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QGraphicsSceneMouseEvent)
QT_FORWARD_DECLARE_CLASS(QKeyEvent)
QT_FORWARD_DECLARE_CLASS(QPointF)

namespace QmlDesigner {

enum class ToolType { Move, Select };

class TimelineMovableAbstractItem;
class AbstractScrollGraphicsScene;
class TimelineToolDelegate;

class TimelineAbstractTool
{
public:
    explicit TimelineAbstractTool(AbstractScrollGraphicsScene *scene);
    explicit TimelineAbstractTool(AbstractScrollGraphicsScene *scene, TimelineToolDelegate *delegate);
    virtual ~TimelineAbstractTool();

    virtual void mousePressEvent(TimelineMovableAbstractItem *item, QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseMoveEvent(TimelineMovableAbstractItem *item, QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseReleaseEvent(TimelineMovableAbstractItem *item,
                                   QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseDoubleClickEvent(TimelineMovableAbstractItem *item,
                                       QGraphicsSceneMouseEvent *event) = 0;

    virtual void keyPressEvent(QKeyEvent *keyEvent) = 0;
    virtual void keyReleaseEvent(QKeyEvent *keyEvent) = 0;

    AbstractScrollGraphicsScene *scene() const;

    TimelineToolDelegate *delegate() const;

    QPointF startPosition() const;

    TimelineMovableAbstractItem *currentItem() const;

private:
    AbstractScrollGraphicsScene *m_scene;

    TimelineToolDelegate *m_delegate;
};

} // namespace QmlDesigner
