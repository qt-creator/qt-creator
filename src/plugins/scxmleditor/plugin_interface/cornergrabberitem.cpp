// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
