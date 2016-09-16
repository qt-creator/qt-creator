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

#include "selectionrectangle.h"

#include <QPen>
#include <QGraphicsScene>
#include <QDebug>
#include <cmath>

namespace QmlDesigner {

SelectionRectangle::SelectionRectangle(LayerItem *layerItem)
    : m_controlShape(new QGraphicsRectItem(layerItem)),
    m_layerItem(layerItem)
{
    QPen pen(Qt::black);
    pen.setJoinStyle(Qt::MiterJoin);
    pen.setCosmetic(true);
    m_controlShape->setPen(pen);
    m_controlShape->setBrush(QColor(128, 128, 128, 50));
}

SelectionRectangle::~SelectionRectangle()
{
    if (m_controlShape) {
        if (m_controlShape->scene())
            m_controlShape->scene()->removeItem(m_controlShape);
        delete  m_controlShape;
    }
}

void SelectionRectangle::clear()
{
    hide();
}
void SelectionRectangle::show()
{
    m_controlShape->setParentItem(m_layerItem.data());
    m_controlShape->show();
}

void SelectionRectangle::hide()
{
    m_controlShape->setParentItem(0);
    m_controlShape->hide();
}

QRectF SelectionRectangle::rect() const
{
    return m_controlShape->mapFromScene(m_controlShape->rect()).boundingRect();
}

void SelectionRectangle::setRect(const QPointF &firstPoint,
                                 const QPointF &secondPoint)
{
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
