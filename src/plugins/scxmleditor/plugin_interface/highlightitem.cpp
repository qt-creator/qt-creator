// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "highlightitem.h"
#include <QPainter>

using namespace ScxmlEditor::PluginInterface;

HighlightItem::HighlightItem(BaseItem *baseItem)
    : QGraphicsObject(nullptr)
    , m_baseItem(baseItem)
{
    m_pen = QPen(QColor(0xff, 0x00, 0x60));
    m_pen.setWidth(2);
    m_pen.setStyle(Qt::DashLine);
    m_pen.setCosmetic(true);

    setZValue(1000);
}

void HighlightItem::advance(int phase)
{
    Q_UNUSED(phase)

    prepareGeometryChange();

    if (m_baseItem) {
        setPos(m_baseItem->scenePos());
        m_boundingRect = m_baseItem->boundingRect();
    }
    update();
}

QRectF HighlightItem::boundingRect() const
{
    return m_boundingRect;
}

void HighlightItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (m_baseItem) {
        painter->save();
        painter->setRenderHints(QPainter::Antialiasing, true);

        QRectF br = m_baseItem->polygonShape().boundingRect();

        switch (m_baseItem->type()) {
        case StateType:
        case ParallelType: {
            painter->setOpacity(1.0);
            painter->setPen(m_pen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(br, 10, 10);
            break;
        }
        case InitialStateType:
        case HistoryType:
        case FinalStateType: {
            painter->setOpacity(1.0);
            painter->setPen(m_pen);
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(br);
            break;
        }
        default:
            break;
        }

        painter->restore();
    }
}
