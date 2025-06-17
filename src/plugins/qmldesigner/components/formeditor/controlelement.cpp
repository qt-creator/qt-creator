// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "controlelement.h"
#include "formeditortracing.h"

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include "layeritem.h"
#include <QDebug>


namespace QmlDesigner {

using FormEditorTracing::category;

ControlElement::ControlElement(LayerItem *layerItem)
    : m_controlShape(new QGraphicsRectItem(layerItem))
{
    NanotraceHR::Tracer tracer{"control element constructor", category()};

    QPen pen;
    pen.setCosmetic(true);
    pen.setStyle(Qt::DashLine);
    pen.setColor(Qt::blue);
    m_controlShape->setPen(pen);
}

ControlElement::~ControlElement()
{
    NanotraceHR::Tracer tracer{"control element destructor", category()};

    delete m_controlShape;
}

void ControlElement::hide()
{
    NanotraceHR::Tracer tracer{"control element hide", category()};

    m_controlShape->hide();
}

void ControlElement::setBoundingRect(const QRectF &rect)
{
    NanotraceHR::Tracer tracer{"control element set bounding rect", category()};

    m_controlShape->show();
    m_controlShape->setRect(m_controlShape->mapFromScene(rect).boundingRect());
}

}
