// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "finalstateitem.h"

#include <utils/theme/theme.h>

#include <QPainter>
#include <QPalette>

using namespace ScxmlEditor::PluginInterface;

FinalStateItem::FinalStateItem(const QPointF &pos, BaseItem *parent)
    : ConnectableItem(pos, parent)
{
    setItemBoundingRect(QRectF(-20, -20, 40, 40));
    setMinimumHeight(40);
    setMinimumWidth(40);

    m_pen.setColor(Utils::creatorColor(Utils::Theme::TextColorNormal));
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
    static const bool isDark = Utils::creatorTheme()->colorScheme() == Qt::ColorScheme::Dark;
    painter->setBrush(isDark ? QColor(0x00, 0x00, 0x00) : QColor(0xff, 0xff, 0xff));
    static const QColor colorNormal = Utils::creatorColor(Utils::Theme::TextColorNormal);
    m_pen.setColor(overlapping() ? qRgb(0xff, 0x00, 0x60) : colorNormal);
    painter->setPen(m_pen);
    painter->drawEllipse(boundingRect().center(), m_size, m_size);

    painter->setPen(Qt::NoPen);
    painter->setBrush(Utils::creatorColor(Utils::Theme::PaletteTextDisabled));
    painter->drawEllipse(boundingRect().center(), m_size * 0.8, m_size * 0.8);

    painter->restore();
}

bool FinalStateItem::canStartTransition(ItemType type) const
{
    Q_UNUSED(type)
    return false;
}
