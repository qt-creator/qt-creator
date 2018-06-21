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

#include "anchorindicatorgraphicsitem.h"

#include <QPainter>
#include <QPainterPath>

#include <QGraphicsScene>
#include <QGraphicsView>

const int AngleDegree = 16;

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
        return 180 * AngleDegree;
    case AnchorLineLeft:
        return 90 * AngleDegree;
    case AnchorLineRight:
        return 270 * AngleDegree;
    default:
        return 0;
    }
}

void AnchorIndicatorGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /* option */, QWidget * /* widget */)
{
    painter->save();

    QPen linePen(QColor(0, 0, 0, 150));
    linePen.setCosmetic(true);
    linePen.setDashPattern({3., 2.});

    painter->setPen(linePen);

    painter->drawLine(m_startPoint, m_firstControlPoint);
    painter->drawLine(m_firstControlPoint, m_secondControlPoint);
    painter->drawLine(m_secondControlPoint, m_endPoint);

    linePen.setColor(QColor(255, 255, 255, 150));
    linePen.setDashPattern({2., 3.});
    linePen.setDashOffset(2.);

    painter->setPen(linePen);

    painter->drawLine(m_startPoint, m_firstControlPoint);
    painter->drawLine(m_firstControlPoint, m_secondControlPoint);
    painter->drawLine(m_secondControlPoint, m_endPoint);

    qreal zoomFactor = 1;
    if (QGraphicsView* view = scene()->views().at(0))
        zoomFactor = view->matrix().m11();
    qreal bumpSize = 8 / zoomFactor;
    QRectF bumpRectangle(0., 0., bumpSize, bumpSize);

    QPen greenPen(Qt::green, 2);
    greenPen.setCosmetic(true);
    painter->setPen(greenPen);
    painter->drawLine(m_sourceAnchorLineFirstPoint, m_sourceAnchorLineSecondPoint);

    bumpRectangle.moveTo(m_startPoint.x() - bumpSize / 2, m_startPoint.y() - bumpSize / 2);
    painter->setBrush(painter->pen().color());
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->drawChord(bumpRectangle, startAngleForAnchorLine(m_sourceAnchorLineType), 180 * AngleDegree);
    painter->setRenderHint(QPainter::Antialiasing, false);

    QPen bluePen(Qt::blue, 2);
    bluePen.setCosmetic(true);
    painter->setPen(bluePen);
    painter->drawLine(m_targetAnchorLineFirstPoint, m_targetAnchorLineSecondPoint);

    bumpRectangle.moveTo(m_endPoint.x() - bumpSize / 2, m_endPoint.y() - bumpSize / 2);
    painter->setBrush(painter->pen().color());
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->drawChord(bumpRectangle, startAngleForAnchorLine(m_targetAnchorLineType), 180 * AngleDegree);
    painter->setRenderHint(QPainter::Antialiasing, false);

    painter->restore();
}

QRectF AnchorIndicatorGraphicsItem::boundingRect() const
{
    return m_boundingRect;
}

static QPointF createParentAnchorPoint(const QmlItemNode &parentQmlItemNode, AnchorLineType anchorLineType, const QmlItemNode &childQmlItemNode)
{
    QRectF parentBoundingRect = parentQmlItemNode.instanceSceneTransform().mapRect(parentQmlItemNode.instanceBoundingRect());
    QRectF childBoundingRect = childQmlItemNode.instanceSceneTransform().mapRect(childQmlItemNode.instanceBoundingRect());

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
    QRectF boundingRect = qmlItemNode.instanceBoundingRect();

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
    QRectF boundingRectangle = anchorLine.qmlItemNode().instanceBoundingRect();

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
