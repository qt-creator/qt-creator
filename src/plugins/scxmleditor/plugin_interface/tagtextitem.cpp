// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tagtextitem.h"

#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QLabel>
#include <QPainter>

using namespace ScxmlEditor::PluginInterface;

TagTextItem::TagTextItem(QGraphicsItem *parent)
    : QGraphicsObject(parent)
{
    setFlag(ItemIsMovable, true);
    setFlag(ItemIsFocusable, true);
    setFlag(ItemIsSelectable, true);
    m_textItem = new TextItem(this);
    connect(m_textItem, &TextItem::textChanged, this, [this] { emit textChanged(); });
    connect(m_textItem, &TextItem::textReady,
            this, [this](const QString &text) { emit textReady(text); });
    connect(m_textItem, &TextItem::selected, this, [this](bool sel) { emit selected(sel); });
    setAcceptHoverEvents(true);
}

QRectF TagTextItem::boundingRect() const
{
    return m_textItem->boundingRect().adjusted(-8, -8, 8, 8);
}

void TagTextItem::setText(const QString &text)
{
    m_textItem->setText(text);
}

void TagTextItem::setDefaultTextColor(const QColor &col)
{
    m_textItem->setDefaultTextColor(col);
}

bool TagTextItem::needIgnore(const QPointF sPos)
{
    // If we found QuickTransition-item or CornerGrabber at this point, we must ignore mouse press here
    // So we can press QuickTransition/CornerGrabber item although there is transition lines front of these items
    const QList<QGraphicsItem *> items = scene()->items(sPos);
    for (QGraphicsItem *item : items) {
        if (item->type() == QuickTransitionType || (item->type() == CornerGrabberType && item->parentItem() != this))
            return true;
    }

    return false;
}

void TagTextItem::hoverEnterEvent(QGraphicsSceneHoverEvent *e)
{
    if (needIgnore(e->scenePos())) {
        e->ignore();
        return;
    }

    setCursor(Qt::SizeAllCursor);
    QGraphicsObject::hoverEnterEvent(e);
}

void TagTextItem::hoverMoveEvent(QGraphicsSceneHoverEvent *e)
{
    if (needIgnore(e->scenePos())) {
        setCursor(Qt::ArrowCursor);
        e->ignore();
        return;
    }

    setCursor(Qt::SizeAllCursor);
    QGraphicsObject::hoverEnterEvent(e);
}

void TagTextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *e)
{
    setCursor(Qt::ArrowCursor);
    QGraphicsObject::hoverLeaveEvent(e);
}

void TagTextItem::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    if (needIgnore(e->scenePos())) {
        e->ignore();
        return;
    }

    m_startPos = pos();
    QGraphicsObject::mousePressEvent(e);
}

void TagTextItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
    m_movePoint += (pos() - m_startPos);
    emit movePointChanged();
    QGraphicsObject::mouseReleaseEvent(e);
}

void TagTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(widget)
}

void TagTextItem::resetMovePoint(const QPointF &point)
{
    m_movePoint = point;
}

QPointF TagTextItem::movePoint() const
{
    return m_movePoint;
}
