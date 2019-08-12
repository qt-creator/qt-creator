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

#include <QMouseEvent>
#include <QPainterPath>
#include <QPoint>
#include <QRectF>

namespace DesignTools {

class GraphicsView;
class Playhead;

enum class SelectionTool { Undefined, Lasso, Rectangle };

class Selector
{
public:
    Selector();

    void paint(QPainter *painter);

    void mousePress(QMouseEvent *event, GraphicsView *view);

    void mouseMove(QMouseEvent *event, GraphicsView *view, Playhead &playhead);

    void mouseRelease(QMouseEvent *event, GraphicsView *view);

private:
    bool isOverSelectedKeyframe(const QPointF &pos, GraphicsView *view);

    bool select(const SelectionTool &tool, const QPointF &pos, GraphicsView *view);

    bool pressSelection(SelectionMode mode, const QPointF &pos, GraphicsView *view);

    bool rectangleSelection(SelectionMode mode, const QPointF &pos, GraphicsView *view);

    bool lassoSelection(SelectionMode mode, const QPointF &pos, GraphicsView *view);

    void clearSelection(GraphicsView *view);

    void applyPreSelection(GraphicsView *view);

    Shortcuts m_shortcuts = Shortcuts();

    Shortcut m_shortcut;

    SelectionTool m_tool = SelectionTool::Rectangle;

    QPoint m_mouseInit = QPoint();

    QPoint m_mouseCurr = QPoint();

    QPainterPath m_lasso = QPainterPath();

    QRectF m_rect = QRectF();
};

} // End namespace DesignTools.
