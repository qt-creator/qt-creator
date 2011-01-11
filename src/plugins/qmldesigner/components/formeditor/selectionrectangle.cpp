/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "selectionrectangle.h"

#include <QPen>
#include <QGraphicsScene>
#include <QtDebug>
#include <cmath>
#include <QGraphicsScene>

namespace QmlDesigner {

SelectionRectangle::SelectionRectangle(LayerItem *layerItem)
    : m_controlShape(new QGraphicsRectItem(layerItem)),
    m_layerItem(layerItem)
{
    m_controlShape->setPen(QPen(Qt::black));
    m_controlShape->setBrush(QColor(128, 128, 128, 50));
}

SelectionRectangle::~SelectionRectangle()
{
    if (m_layerItem)
        m_layerItem->scene()->removeItem(m_controlShape);
}

void SelectionRectangle::clear()
{
    hide();
}
void SelectionRectangle::show()
{
    m_controlShape->show();
}

void SelectionRectangle::hide()
{
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
