// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "historyitem.h"
#include <QPainter>

using namespace ScxmlEditor::PluginInterface;

HistoryItem::HistoryItem(const QPointF &pos, BaseItem *parent)
    : ConnectableItem(pos, parent)
{
    setItemBoundingRect(QRectF(-20, -20, 40, 40));
    setMinimumHeight(40);
    setMinimumWidth(40);

    m_pen.setColor(qRgb(0x12, 0x12, 0x12));
    m_pen.setWidth(2);
}

void HistoryItem::updatePolygon()
{
    QRectF r = boundingRect();
    m_size = qMin(r.width() * 0.45, r.height() * 0.45);
    QPointF center = r.center();

    m_polygon.clear();
    m_polygon << (center + QPointF(-m_size, -m_size))
              << (center + QPointF(m_size, -m_size))
              << (center + QPointF(m_size, m_size))
              << (center + QPointF(-m_size, m_size))
              << (center + QPointF(-m_size, -m_size));
}

void HistoryItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    ConnectableItem::paint(painter, option, widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setOpacity(getOpacity());

    painter->setBrush(QColor(0xff, 0xff, 0xff));
    m_pen.setColor(overlapping() ? qRgb(0xff, 0x00, 0x60) : qRgb(0x45, 0x45, 0x45));
    painter->setPen(m_pen);
    painter->drawEllipse(boundingRect().center(), m_size, m_size);

    painter->drawText(boundingRect(), Qt::AlignCenter, QLatin1String(tagValue("type") == "deep" ? "H*" : "H"));
    painter->restore();
}

bool HistoryItem::canStartTransition(ItemType type) const
{
    if (outputTransitionCount() > 0)
        return false;

    switch (type) {
    case UnknownType:
    case StateType:
    case ParallelType:
        return true;
    default:
        return false;
    }
}
