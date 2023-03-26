// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "curveeditorstyle.h"
#include "selectableitem.h"

namespace QmlDesigner {

class GraphicsView;
class GraphicsScene;
class Playhead;

class Selector
{
public:
    Selector();

    void paint(QPainter *painter);

    void mousePress(QMouseEvent *event, GraphicsView *view, GraphicsScene *scene);

    void mouseMove(QMouseEvent *event, GraphicsView *view, GraphicsScene *scene, Playhead &playhead);

    void mouseRelease(QMouseEvent *event, GraphicsScene *scene);

private:
    enum class SelectionTool { Undefined, Lasso, Rectangle };

    bool select(const SelectionTool &tool, const QPointF &pos, GraphicsScene *scene);

    bool pressSelection(SelectableItem::SelectionMode mode, const QPointF &pos, GraphicsScene *scene);

    bool rectangleSelection(SelectableItem::SelectionMode mode, const QPointF &pos, GraphicsScene *scene);

    bool lassoSelection(SelectableItem::SelectionMode mode, const QPointF &pos, GraphicsScene *scene);

    void clearSelection(GraphicsScene *scene);

    void applyPreSelection(GraphicsScene *scene);

    Shortcuts m_shortcuts;

    Shortcut m_shortcut;

    SelectionTool m_tool = SelectionTool::Rectangle;

    QPoint m_mouseInit;

    QPoint m_mouseCurr;

    QPainterPath m_lasso;

    QRectF m_rect;
};

} // End namespace QmlDesigner.
