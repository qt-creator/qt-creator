// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bindingindicatorgraphicsitem.h"
#include "formeditortracing.h"

#include <QPainter>

namespace QmlDesigner {

using FormEditorTracing::category;

BindingIndicatorGraphicsItem::BindingIndicatorGraphicsItem(QGraphicsItem *parent) :
    QGraphicsObject(parent)
{
    NanotraceHR::Tracer tracer{"binding indicator graphics item constructor", category()};
}

void BindingIndicatorGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    NanotraceHR::Tracer tracer{"binding indicator graphics item paint", category()};

    painter->save();
    QPen linePen(QColor(255, 0, 0, 255), 2);
    linePen.setCosmetic(true);
    painter->setPen(linePen);
    painter->drawLine(m_bindingLine);
    painter->restore();
}

QRectF BindingIndicatorGraphicsItem::boundingRect() const
{
    NanotraceHR::Tracer tracer{"binding indicator graphics item bounding rect", category()};

    return {m_bindingLine.x1(),
            m_bindingLine.y1(),
            m_bindingLine.x2() - m_bindingLine.x1() + 3,
            m_bindingLine.y2() - m_bindingLine.y1() + 3};
}

void BindingIndicatorGraphicsItem::updateBindingIndicator(const QLineF &bindingLine)
{
    NanotraceHR::Tracer tracer{"binding indicator graphics item update binding indicator", category()};

    m_bindingLine = bindingLine;
    update();
}

} // namespace QmlDesigner
