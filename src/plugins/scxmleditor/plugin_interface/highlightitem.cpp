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
