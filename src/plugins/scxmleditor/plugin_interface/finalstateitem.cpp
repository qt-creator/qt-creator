// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "finalstateitem.h"

#include <QGraphicsScene>
#include <QPainter>
#include <QPalette>

using namespace ScxmlEditor::PluginInterface;

FinalStateItem::FinalStateItem(const QPointF &pos, BaseItem *parent)
    : ConnectableItem(pos, parent)
{
    setItemBoundingRect(QRectF(-20, -20, 40, 40));
    setMinimumHeight(40);
    setMinimumWidth(40);

    m_pen.setColor(qRgb(0x12, 0x12, 0x12));
    m_pen.setWidth(2);
}

void FinalStateItem::updatePolygon()
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

void FinalStateItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    ConnectableItem::paint(painter, option, widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setOpacity(getOpacity());

    QPalette::ColorGroup group = overlapping() ? QPalette::Active : QPalette::Inactive;
    m_pen.setColor(scene()->palette().color(group, QPalette::WindowText));
    painter->setPen(m_pen);
    painter->drawEllipse(boundingRect().center(), m_size, m_size);

    painter->setPen(Qt::NoPen);
    painter->setBrush(scene()->palette().color(QPalette::WindowText));
    painter->drawEllipse(boundingRect().center(), m_size * 0.8, m_size * 0.8);

    painter->restore();
}

bool FinalStateItem::canStartTransition(ItemType type) const
{
    Q_UNUSED(type)
    return false;
}
