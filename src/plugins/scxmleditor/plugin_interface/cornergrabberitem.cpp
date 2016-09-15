/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cornergrabberitem.h"

#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

using namespace ScxmlEditor::PluginInterface;

CornerGrabberItem::CornerGrabberItem(QGraphicsItem *parent, Qt::CursorShape cshape)
    : QGraphicsObject(parent)
    , m_cursorShape(cshape)
{
    setFlag(ItemIgnoresTransformations, true);
    setParentItem(parent);
    installSceneEventFilter(parent);
    setZValue(500);
    m_rect = QRectF(-5, -5, 10, 10);
    m_drawingRect = m_rect.adjusted(2, 2, -2, -2);

    setAcceptHoverEvents(true);
}

void CornerGrabberItem::updateCursor()
{
    setCursor(m_selected ? m_cursorShape : Qt::ArrowCursor);
}

void CornerGrabberItem::setSelected(bool para)
{
    m_selected = para;
    updateCursor();
    update();
}

void CornerGrabberItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *e)
{
    Q_UNUSED(e)
    setSelected(false);
}

void CornerGrabberItem::hoverEnterEvent(QGraphicsSceneHoverEvent *e)
{
    Q_UNUSED(e)
    setSelected(true);
}

void CornerGrabberItem::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    m_pressPoint = e->pos();
}

void CornerGrabberItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(isEnabled() ? QColor(0x62, 0x62, 0xf9) : QColor(0x12, 0x12, 0x12));

    if (m_grabberType == Square)
        painter->drawRect(m_drawingRect);
    else
        painter->drawEllipse(m_drawingRect);

    painter->restore();
}
