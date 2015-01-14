/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "anchorindicatorgraphicsitem.h"

#include <QPainter>
#include <QPainterPath>

namespace QmlDesigner {

AnchorIndicatorGraphicsItem::AnchorIndicatorGraphicsItem(QGraphicsItem *parent) :
    QGraphicsObject(parent)
{
    setZValue(-3);
}

int startAngleForAnchorLine(const AnchorLineType &anchorLineType)
{
    switch (anchorLineType) {
    case AnchorLineTop:
        return 0;
    case AnchorLineBottom:
        return 180 * 16;
    case AnchorLineLeft:
        return 90 * 16;
    case AnchorLineRight:
        return 270 * 16;
    default:
        return 0;
    }
}

void AnchorIndicatorGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /* option */, QWidget * /* widget */)
{
    painter->save();

    QPen linePen(QColor(0, 0, 0, 150));
    linePen.setDashPattern(QVector<double>() << 3. << 2.);

    painter->setPen(linePen);

    painter->drawLine(m_startPoint, m_firstControlPoint);
    painter->drawLine(m_firstControlPoint, m_secondControlPoint);
    painter->drawLine(m_secondControlPoint, m_endPoint);

    linePen.setColor(QColor(255, 255, 255, 150));
    linePen.setDashPattern(QVector<double>() << 2. << 3.);
    linePen.setDashOffset(2.);

    painter->setPen(linePen);

    painter->drawLine(m_startPoint, m_firstControlPoint);
    painter->drawLine(m_firstControlPoint, m_secondControlPoint);
    painter->drawLine(m_secondControlPoint, m_endPoint);

    static QRectF bumpRectangle(0., 0., 8., 8.);

    painter->setPen(QPen(QColor(0, 255 , 0), 2));
    painter->drawLine(m_sourceAnchorLineFirstPoint, m_sourceAnchorLineSecondPoint);

    bumpRectangle.moveTo(m_startPoint.x() - 4., m_startPoint.y() - 4.);
    painter->setBrush(painter->pen().color());
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->drawChord(bumpRectangle, startAngleForAnchorLine(m_sourceAnchorLineType), 180 * 16);
    painter->setRenderHint(QPainter::Antialiasing, false);

    painter->setPen(QPen(QColor(0, 0 , 255), 2));
    painter->drawLine(m_targetAnchorLineFirstPoint, m_targetAnchorLineSecondPoint);

    bumpRectangle.moveTo(m_endPoint.x() - 4., m_endPoint.y() - 4.);
    painter->setBrush(painter->pen().color());
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->drawChord(bumpRectangle, startAngleForAnchorLine(m_targetAnchorLineType), 180 * 16);
    painter->setRenderHint(QPainter::Antialiasing, false);

    painter->restore();
}

QRectF AnchorIndicatorGraphicsItem::boundingRect() const
{
    return m_boundingRect;
}

static QPointF createParentAnchorPoint(const QmlItemNode &parentQmlItemNode, AnchorLineType anchorLineType, const QmlItemNode &childQmlItemNode)
{
    QRectF parentBoundingRect = parentQmlItemNode.instanceSceneTransform().mapRect(parentQmlItemNode.instanceBoundingRect().adjusted(0., 0., 1., 1.));
    QRectF childBoundingRect = childQmlItemNode.instanceSceneTransform().mapRect(childQmlItemNode.instanceBoundingRect().adjusted(0., 0., 1., 1.));

    QPointF anchorPoint;

    switch (anchorLineType) {
    case AnchorLineTop:
        anchorPoint = QPointF(childBoundingRect.center().x(), parentBoundingRect.top());
        break;
    case AnchorLineBottom:
        anchorPoint = QPointF(childBoundingRect.center().x(), parentBoundingRect.bottom());
        break;
    case AnchorLineLeft:
        anchorPoint = QPointF(parentBoundingRect.left(), childBoundingRect.center().y());
        break;
    case AnchorLineRight:
        anchorPoint = QPointF(parentBoundingRect.right(), childBoundingRect.center().y());
        break;
    default:
        break;
    }

    return anchorPoint;
}

static QPointF createAnchorPoint(const QmlItemNode &qmlItemNode, AnchorLineType anchorLineType)
{
    QRectF boundingRect = qmlItemNode.instanceBoundingRect().adjusted(0., 0., 1., 1.);

    QPointF anchorPoint;

    switch (anchorLineType) {
    case AnchorLineTop:
        anchorPoint = QPointF(boundingRect.center().x(), boundingRect.top());
        break;
    case AnchorLineBottom:
        anchorPoint = QPointF(boundingRect.center().x(), boundingRect.bottom());
        break;
    case AnchorLineLeft:
        anchorPoint = QPointF(boundingRect.left(), boundingRect.center().y());
        break;
    case AnchorLineRight:
        anchorPoint = QPointF(boundingRect.right(), boundingRect.center().y());
        break;
    default:
        break;
    }

    return qmlItemNode.instanceSceneTransform().map(anchorPoint);
}

static QPointF createControlPoint(const QPointF &firstEditPoint, AnchorLineType anchorLineType, const QPointF &secondEditPoint)
{
    QPointF controlPoint = firstEditPoint;

    switch (anchorLineType) {
    case AnchorLineTop:
    case AnchorLineBottom:
        controlPoint.ry() += (secondEditPoint.y() - firstEditPoint.y()) / 2.0;
        break;
    case AnchorLineLeft:
    case AnchorLineRight:
        controlPoint.rx() += (secondEditPoint.x() - firstEditPoint.x()) / 2.0;
        break;
    default:
        break;
    }

    return controlPoint;
}

static void updateAnchorLinePoints(QPointF *firstPoint, QPointF *secondPoint, const AnchorLine &anchorLine)
{
    QRectF boundingRectangle = anchorLine.qmlItemNode().instanceBoundingRect().adjusted(0., 0., 1., 1.);

    switch (anchorLine.type()) {
    case AnchorLineTop:
        *firstPoint = boundingRectangle.topLeft();
        *secondPoint = boundingRectangle.topRight();
        break;
    case AnchorLineBottom:
        *firstPoint = boundingRectangle.bottomLeft();
        *secondPoint = boundingRectangle.bottomRight();
        break;
    case AnchorLineLeft:
        *firstPoint = boundingRectangle.topLeft();
        *secondPoint = boundingRectangle.bottomLeft();
        break;
    case AnchorLineRight:
        *firstPoint = boundingRectangle.topRight();
        *secondPoint = boundingRectangle.bottomRight();
        break;
    default:
        break;
    }

    *firstPoint = anchorLine.qmlItemNode().instanceSceneTransform().map(*firstPoint);
    *secondPoint = anchorLine.qmlItemNode().instanceSceneTransform().map(*secondPoint);
}

void AnchorIndicatorGraphicsItem::updateAnchorIndicator(const AnchorLine &sourceAnchorLine,
                                                        const AnchorLine &targetAnchorLine)
{
    if (sourceAnchorLine.qmlItemNode().isValid() && targetAnchorLine.qmlItemNode().isValid()) {
        m_sourceAnchorLineType = sourceAnchorLine.type();
        m_targetAnchorLineType = targetAnchorLine.type();

        m_startPoint = createAnchorPoint(sourceAnchorLine.qmlItemNode(), sourceAnchorLine.type());

        if (targetAnchorLine.qmlItemNode() == sourceAnchorLine.qmlItemNode().instanceParentItem())
            m_endPoint = createParentAnchorPoint(targetAnchorLine.qmlItemNode(), targetAnchorLine.type(), sourceAnchorLine.qmlItemNode());
        else
            m_endPoint = createAnchorPoint(targetAnchorLine.qmlItemNode(), targetAnchorLine.type());

        m_firstControlPoint = createControlPoint(m_startPoint, sourceAnchorLine.type(), m_endPoint);
        m_secondControlPoint = createControlPoint(m_endPoint, targetAnchorLine.type(), m_startPoint);

        updateAnchorLinePoints(&m_sourceAnchorLineFirstPoint, &m_sourceAnchorLineSecondPoint, sourceAnchorLine);
        updateAnchorLinePoints(&m_targetAnchorLineFirstPoint, &m_targetAnchorLineSecondPoint, targetAnchorLine);

        updateBoundingRect();
        update();
    }
}

void AnchorIndicatorGraphicsItem::updateBoundingRect()
{
    QPolygonF controlPolygon(QVector<QPointF>()
                             << m_startPoint
                             << m_firstControlPoint
                             << m_secondControlPoint
                             << m_endPoint
                             << m_sourceAnchorLineFirstPoint
                             << m_sourceAnchorLineSecondPoint
                             << m_targetAnchorLineFirstPoint
                             << m_targetAnchorLineSecondPoint
                             );

    m_boundingRect = controlPolygon.boundingRect().adjusted(-10., -10., 10., 10.);
}
AnchorLineType AnchorIndicatorGraphicsItem::sourceAnchorLineType() const
{
    return m_sourceAnchorLineType;
}

void AnchorIndicatorGraphicsItem::setSourceAnchorLineType(const AnchorLineType &sourceAnchorLineType)
{
    m_sourceAnchorLineType = sourceAnchorLineType;
}




} // namespace QmlDesigner
