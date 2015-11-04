/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    m_highlightLine->setPen(QPen(QBrush(Qt::red), 2.0, Qt::DotLine));
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

}
