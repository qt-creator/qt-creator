// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "controlelement.h"

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include "layeritem.h"
#include <QDebug>


namespace QmlDesigner {

ControlElement::ControlElement(LayerItem *layerItem)
    : m_controlShape(new QGraphicsRectItem(layerItem))
{
    QPen pen;
    pen.setCosmetic(true);
    pen.setStyle(Qt::DashLine);
    pen.setColor(Qt::blue);
    m_controlShape->setPen(pen);
}

ControlElement::~ControlElement()
{
    delete m_controlShape;
}

void ControlElement::hide()
{
    m_controlShape->hide();
}

void ControlElement::setBoundingRect(const QRectF &rect)
{
    m_controlShape->show();
    m_controlShape->setRect(m_controlShape->mapFromScene(rect).boundingRect());
}

}
