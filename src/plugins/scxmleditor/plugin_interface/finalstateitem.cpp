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

#include "finalstateitem.h"
#include <QPainter>

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

    painter->setBrush(QColor(0xff, 0xff, 0xff));
    m_pen.setColor(overlapping() ? qRgb(0xff, 0x00, 0x60) : qRgb(0x45, 0x45, 0x45));
    painter->setPen(m_pen);
    painter->drawEllipse(boundingRect().center(), m_size, m_size);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0x4d, 0x4d, 0x4d));
    painter->drawEllipse(boundingRect().center(), m_size * 0.8, m_size * 0.8);

    painter->restore();
}

bool FinalStateItem::canStartTransition(ItemType type) const
{
    Q_UNUSED(type)
    return false;
}
