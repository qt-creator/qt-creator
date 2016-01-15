/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "alignlineitem.h"

#include <QPen>
#include <QBrush>
#include <QGraphicsScene>

namespace qmt {

AlignLineItem::AlignLineItem(Direction direction, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_direction(direction),
      m_alignLine(new QGraphicsLineItem(this)),
      m_highlightLine(new QGraphicsLineItem(this))
{
    m_alignLine->setPen(QPen(QBrush(Qt::blue), 1.0, Qt::DotLine));
    m_highlightLine->setPen(QPen(QBrush(Qt::red), 1.0, Qt::SolidLine));
    m_highlightLine->setZValue(1);
}

AlignLineItem::~AlignLineItem()
{
}

void AlignLineItem::setLine(qreal pos)
{
    setLine(pos, 0.0, 0.0);
}

void AlignLineItem::setLine(qreal pos, qreal otherPos1, qreal otherPos2)
{
    // parameters are given in scene coordinates and are not mapped to this object's
    // coordinbate system because it is a trivial conversion.(in the way how AlignLineItem
    // is used)
    QRectF sceneRect = scene()->sceneRect();
    switch (m_direction) {
    case Horizontal:
        m_alignLine->setLine(sceneRect.left() + 1.0, pos, sceneRect.right() - 1.0, pos);
        m_highlightLine->setLine(otherPos1, pos, otherPos2, pos);
        break;
    case Vertical:
        m_alignLine->setLine(pos, sceneRect.top() + 1.0, pos, sceneRect.bottom() - 1.0);
        m_highlightLine->setLine(pos, otherPos1, pos, otherPos2);
        break;
    }
    m_highlightLine->setVisible(otherPos1 - otherPos2 != 0);
}

QRectF AlignLineItem::boundingRect() const
{
    return childrenBoundingRect();
}

void AlignLineItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

} // namespace qmt
