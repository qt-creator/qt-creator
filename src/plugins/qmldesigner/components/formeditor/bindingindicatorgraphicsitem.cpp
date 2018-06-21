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

#include "bindingindicatorgraphicsitem.h"

#include <QPainter>

namespace QmlDesigner {

BindingIndicatorGraphicsItem::BindingIndicatorGraphicsItem(QGraphicsItem *parent) :
    QGraphicsObject(parent)
{
}

void BindingIndicatorGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    painter->save();
    QPen linePen(QColor(255, 0, 0, 255), 2);
    linePen.setCosmetic(true);
    painter->setPen(linePen);
    painter->drawLine(m_bindingLine);
    painter->restore();
}

QRectF BindingIndicatorGraphicsItem::boundingRect() const
{
    return QRectF(m_bindingLine.x1(),
                  m_bindingLine.y1(),
                  m_bindingLine.x2() - m_bindingLine.x1() + 3,
                  m_bindingLine.y2() - m_bindingLine.y1() + 3);
}

void BindingIndicatorGraphicsItem::updateBindingIndicator(const QLineF &bindingLine)
{
    m_bindingLine = bindingLine;
    update();
}

} // namespace QmlDesigner
