// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    return {m_bindingLine.x1(),
                  m_bindingLine.y1(),
                  m_bindingLine.x2() - m_bindingLine.x1() + 3,
                  m_bindingLine.y2() - m_bindingLine.y1() + 3};
}

void BindingIndicatorGraphicsItem::updateBindingIndicator(const QLineF &bindingLine)
{
    m_bindingLine = bindingLine;
    update();
}

} // namespace QmlDesigner
