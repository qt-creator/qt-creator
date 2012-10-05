/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "selectionrectangle.h"

#include <QPen>
#include <QGraphicsScene>
#include <QDebug>
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
