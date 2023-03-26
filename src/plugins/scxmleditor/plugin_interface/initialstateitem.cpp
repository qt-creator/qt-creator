// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "initialstateitem.h"
#include "initialwarningitem.h"
#include "graphicsitemprovider.h"
#include "scxmleditorconstants.h"
#include "scxmluifactory.h"

#include <QByteArray>
#include <QDataStream>
#include <QPainter>

using namespace ScxmlEditor::PluginInterface;

InitialStateItem::InitialStateItem(const QPointF &pos, BaseItem *parent)
    : ConnectableItem(pos, parent)
{
    setItemBoundingRect(QRectF(-20, -20, 40, 40));
    setMinimumHeight(40);
    setMinimumWidth(40);
    m_pen.setColor(qRgb(0x12, 0x12, 0x12));
    m_pen.setWidth(2);

    checkWarningItems();
}

void InitialStateItem::checkWarningItems()
{
    ScxmlUiFactory *uifactory = uiFactory();
    if (uifactory) {
        auto provider = static_cast<GraphicsItemProvider*>(uifactory->object("graphicsItemProvider"));
        if (provider) {
            if (!m_warningItem)
                m_warningItem = static_cast<InitialWarningItem*>(provider->createWarningItem(Constants::C_STATE_WARNING_INITIAL, this));
        }
    }
}

QVariant InitialStateItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant retValue = ConnectableItem::itemChange(change, value);

    switch (change) {
    case QGraphicsItem::ItemSceneHasChanged:
        checkWarningItems();
        break;
    default:
        break;
    }

    return retValue;
}

void InitialStateItem::updatePolygon()
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

    if (m_warningItem)
        m_warningItem->updatePos();
}

void InitialStateItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    ConnectableItem::paint(painter, option, widget);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setOpacity(getOpacity());

    m_pen.setColor(overlapping() ? qRgb(0xff, 0x00, 0x60) : qRgb(0x45, 0x45, 0x45));
    painter->setPen(m_pen);
    painter->setBrush(QColor(0x4d, 0x4d, 0x4d));
    painter->drawEllipse(boundingRect().center(), m_size, m_size);

    painter->restore();
}

InitialWarningItem *InitialStateItem::warningItem() const
{
    return m_warningItem;
}

bool InitialStateItem::canStartTransition(ItemType type) const
{
    if (transitionCount() > 0)
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

void InitialStateItem::checkWarnings()
{
    if (m_warningItem)
        m_warningItem->check();
}
