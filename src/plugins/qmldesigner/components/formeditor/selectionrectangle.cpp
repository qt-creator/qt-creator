// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "selectionrectangle.h"
#include "formeditortracing.h"

#include <QPen>
#include <QGraphicsScene>
#include <QDebug>
#include <cmath>

namespace QmlDesigner {

using FormEditorTracing::category;

SelectionRectangle::SelectionRectangle(LayerItem *layerItem)
    : m_controlShape(new QGraphicsRectItem(layerItem)),
    m_layerItem(layerItem)
{
    NanotraceHR::Tracer tracer{"selection rectangle constructor", category()};

    QPen pen(Qt::black);
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setCosmetic(true);
    m_controlShape->setPen(pen);
    m_controlShape->setBrush(QColor(128, 128, 128, 50));
}

SelectionRectangle::~SelectionRectangle()
{
    NanotraceHR::Tracer tracer{"selection rectangle destructor", category()};

    if (m_controlShape) {
        if (m_controlShape->scene())
            m_controlShape->scene()->removeItem(m_controlShape);
        delete  m_controlShape;
    }
}

void SelectionRectangle::clear()
{
    NanotraceHR::Tracer tracer{"selection rectangle clear", category()};

    hide();
}
void SelectionRectangle::show()
{
    NanotraceHR::Tracer tracer{"selection rectangle show", category()};

    m_controlShape->setParentItem(m_layerItem.data());
    m_controlShape->show();
}

void SelectionRectangle::hide()
{
    NanotraceHR::Tracer tracer{"selection rectangle hide", category()};

    m_controlShape->setParentItem(nullptr);
    m_controlShape->hide();
}

QRectF SelectionRectangle::rect() const
{
    NanotraceHR::Tracer tracer{"selection rectangle rect", category()};

    return m_controlShape->mapFromScene(m_controlShape->rect()).boundingRect();
}

void SelectionRectangle::setRect(const QPointF &firstPoint,
                                 const QPointF &secondPoint)
{
    NanotraceHR::Tracer tracer{"selection rectangle set rect", category()};

    double firstX = std::floor(firstPoint.x()) + 0.5;
    double firstY = std::floor(firstPoint.y()) + 0.5;
    double secondX = std::floor(secondPoint.x()) + 0.5;
    double secondY = std::floor(secondPoint.y()) + 0.5;
    QPointF topLeftPoint(firstX < secondX ? firstX : secondX, firstY < secondY ? firstY : secondY);
    QPointF bottomRightPoint(firstX > secondX ? firstX : secondX, firstY > secondY ? firstY : secondY);

    QRectF rect(topLeftPoint, bottomRightPoint);
    m_controlShape->setRect(rect);
}

}
