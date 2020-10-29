/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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
