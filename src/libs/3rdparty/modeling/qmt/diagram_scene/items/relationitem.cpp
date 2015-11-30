/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#include "relationitem.h"

#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram_controller/dvoidvisitor.h"
#include "qmt/diagram/dassociation.h"
#include "qmt/diagram/ddependency.h"
#include "qmt/diagram/dinheritance.h"
#include "qmt/diagram/dobject.h"
#include "qmt/diagram/drelation.h"
#include "qmt/diagram_scene/capabilities/intersectionable.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/items/stereotypedisplayvisitor.h"
#include "qmt/diagram_scene/parts/arrowitem.h"
#include "qmt/diagram_scene/parts/pathselectionitem.h"
#include "qmt/diagram_scene/parts/stereotypesitem.h"
#include "qmt/infrastructure/geometryutilities.h"
#include "qmt/infrastructure/qmtassert.h"
#include "qmt/style/stylecontroller.h"
#include "qmt/style/styledrelation.h"
#include "qmt/style/style.h"
#include "qmt/tasks/diagramscenecontroller.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QFont>
#include <QPen>
#include <QPainter>

namespace qmt {

class RelationItem::ArrowConfigurator : public DConstVoidVisitor
{
public:
    ArrowConfigurator(DiagramSceneModel *diagramSceneModel, ArrowItem *arrow, const QList<QPointF> &points)
        : m_diagramSceneModel(diagramSceneModel),
          m_arrow(arrow),
          m_points(points)
    {
    }

    void visitDInheritance(const DInheritance *inheritance)
    {
        DObject *baseObject = m_diagramSceneModel->diagramController()->findElement<DObject>(inheritance->base(), m_diagramSceneModel->diagram());
        QMT_CHECK(baseObject);
        bool baseIsInterface = baseObject->stereotypes().contains(QStringLiteral("interface"));
        bool lollipopDisplay = false;
        if (baseIsInterface) {
            StereotypeDisplayVisitor stereotypeDisplayVisitor;
            stereotypeDisplayVisitor.setModelController(m_diagramSceneModel->diagramSceneController()->modelController());
            stereotypeDisplayVisitor.setStereotypeController(m_diagramSceneModel->stereotypeController());
            baseObject->accept(&stereotypeDisplayVisitor);
            lollipopDisplay = stereotypeDisplayVisitor.stereotypeDisplay() == DObject::StereotypeIcon;
        }
        if (lollipopDisplay) {
            m_arrow->setShaft(ArrowItem::ShaftSolid);
            m_arrow->setEndHead(ArrowItem::HeadNone);
        } else if (baseIsInterface || inheritance->stereotypes().contains(QStringLiteral("realize"))) {
            m_arrow->setShaft(ArrowItem::ShaftDashed);
            m_arrow->setEndHead(ArrowItem::HeadTriangle);
        } else {
            m_arrow->setShaft(ArrowItem::ShaftSolid);
            m_arrow->setEndHead(ArrowItem::HeadTriangle);
        }
        m_arrow->setArrowSize(16.0);
        m_arrow->setStartHead(ArrowItem::HeadNone);
        m_arrow->setPoints(m_points);
    }

    void visitDDependency(const DDependency *dependency)
    {
        ArrowItem::Head endAHead = ArrowItem::HeadNone;
        ArrowItem::Head endBHead = ArrowItem::HeadNone;
        bool isRealization = dependency->stereotypes().contains(QStringLiteral("realize"));
        switch (dependency->direction()) {
        case MDependency::AToB:
            endBHead = isRealization ? ArrowItem::HeadTriangle : ArrowItem::HeadOpen;
            break;
        case MDependency::BToA:
            endAHead = isRealization ? ArrowItem::HeadTriangle : ArrowItem::HeadOpen;
            break;
        case MDependency::Bidirectional:
            endAHead = ArrowItem::HeadOpen;
            endBHead = ArrowItem::HeadOpen;
            break;
        default:
            break;
        }

        m_arrow->setShaft(ArrowItem::ShaftDashed);
        m_arrow->setArrowSize(12.0);
        m_arrow->setStartHead(endAHead);
        m_arrow->setEndHead(endBHead);
        m_arrow->setPoints(m_points);
    }

    void visitDAssociation(const DAssociation *association)
    {
        m_arrow->setShaft(ArrowItem::ShaftSolid);
        m_arrow->setArrowSize(12.0);
        m_arrow->setDiamondSize(12.0);

        ArrowItem::Head endAHead = ArrowItem::HeadNone;
        ArrowItem::Head endBHead = ArrowItem::HeadNone;

        bool aNav = association->endA().isNavigable();
        bool bNav = association->endB().isNavigable();

        bool aFlat = association->endA().kind() == MAssociationEnd::Association;
        bool bFlat = association->endB().kind() == MAssociationEnd::Association;

        switch (association->endA().kind()) {
        case MAssociationEnd::Association:
            endAHead = ((bNav && !aNav && bFlat) || (aNav && bNav && !bFlat)) ? ArrowItem::HeadFilledTriangle : ArrowItem::HeadNone;
            break;
        case MAssociationEnd::Aggregation:
            endAHead = association->endB().isNavigable() ? ArrowItem::HeadDiamondFilledTriangle : ArrowItem::HeadDiamond;
            break;
        case MAssociationEnd::Composition:
            endAHead = association->endB().isNavigable() ? ArrowItem::HeadFilledDiamondFilledTriangle : ArrowItem::HeadFilledDiamond;
            break;
        }

        switch (association->endB().kind()) {
        case MAssociationEnd::Association:
            endBHead = ((aNav && !bNav && aFlat) || (aNav && bNav && !aFlat)) ? ArrowItem::HeadFilledTriangle : ArrowItem::HeadNone;
            break;
        case MAssociationEnd::Aggregation:
            endBHead = association->endA().isNavigable() ? ArrowItem::HeadDiamondFilledTriangle : ArrowItem::HeadDiamond;
            break;
        case MAssociationEnd::Composition:
            endBHead = association->endA().isNavigable() ? ArrowItem::HeadFilledDiamondFilledTriangle : ArrowItem::HeadFilledDiamond;
            break;
        }

        m_arrow->setStartHead(endAHead);
        m_arrow->setEndHead(endBHead);
        m_arrow->setPoints(m_points);
    }

private:
    DiagramSceneModel *m_diagramSceneModel = 0;
    ArrowItem *m_arrow = 0;
    QList<QPointF> m_points;
};

RelationItem::RelationItem(DRelation *relation, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_relation(relation),
      m_diagramSceneModel(diagramSceneModel)
{
    setFlags(QGraphicsItem::ItemIsSelectable);
}

RelationItem::~RelationItem()
{
}

QRectF RelationItem::boundingRect() const
{
    return childrenBoundingRect();
}

void RelationItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);

#ifdef DEBUG_PAINT_SHAPE
    painter->save();
    painter->setPen(Qt::red);
    painter->setBrush(Qt::blue);
    painter->drawPath(shape());
    painter->restore();
#endif
}

QPainterPath RelationItem::shape() const
{
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    if (m_arrow)
        path.addPath(m_arrow->shape().translated(m_arrow->pos()));
    if (m_name)
        path.addPath(m_name->shape().translated(m_name->pos()));
    if (m_stereotypes)
        path.addPath(m_stereotypes->shape().translated(m_stereotypes->pos()));
    if (m_selectionHandles)
        path.addPath(m_selectionHandles->shape().translated(m_selectionHandles->pos()));
    return path;
}

void RelationItem::moveDelta(const QPointF &delta)
{
    m_diagramSceneModel->diagramController()->startUpdateElement(m_relation, m_diagramSceneModel->diagram(), DiagramController::UpdateGeometry);
    QList<DRelation::IntermediatePoint> points;
    foreach (const DRelation::IntermediatePoint &point, m_relation->intermediatePoints())
        points << DRelation::IntermediatePoint(point.pos() + delta);
    m_relation->setIntermediatePoints(points);
    m_diagramSceneModel->diagramController()->finishUpdateElement(m_relation, m_diagramSceneModel->diagram(), false);
}

void RelationItem::alignItemPositionToRaster(double rasterWidth, double rasterHeight)
{
    m_diagramSceneModel->diagramController()->startUpdateElement(m_relation, m_diagramSceneModel->diagram(), DiagramController::UpdateGeometry);
    QList<DRelation::IntermediatePoint> points;
    foreach (const DRelation::IntermediatePoint &point, m_relation->intermediatePoints()) {
        QPointF pos = point.pos();
        double x = qRound(pos.x() / rasterWidth) * rasterWidth;
        double y = qRound(pos.y() / rasterHeight) * rasterHeight;
        points << DRelation::IntermediatePoint(QPointF(x,y));
    }
    m_relation->setIntermediatePoints(points);
    m_diagramSceneModel->diagramController()->finishUpdateElement(m_relation, m_diagramSceneModel->diagram(), false);
}

bool RelationItem::isSecondarySelected() const
{
    return m_isSecondarySelected;
}

void RelationItem::setSecondarySelected(bool secondarySelected)
{
    if (m_isSecondarySelected != secondarySelected) {
        m_isSecondarySelected = secondarySelected;
        update();
    }
}

bool RelationItem::isFocusSelected() const
{
    return m_isFocusSelected;
}

void RelationItem::setFocusSelected(bool focusSelected)
{
    if (m_isFocusSelected != focusSelected) {
        m_isFocusSelected = focusSelected;
        update();
    }
}

QPointF RelationItem::handlePos(int index)
{
    if (index == 0) {
        // TODO implement
        return QPointF(0,0);
    } else if (index == m_relation->intermediatePoints().size() + 1) {
        // TODO implement
        return QPointF(0,0);
    } else {
        QList<DRelation::IntermediatePoint> intermediatePoints = m_relation->intermediatePoints();
        --index;
        QMT_CHECK(index >= 0 && index < intermediatePoints.size());
        return intermediatePoints.at(index).pos();
    }
}

void RelationItem::insertHandle(int beforeIndex, const QPointF &pos)
{
    if (beforeIndex >= 1 && beforeIndex <= m_relation->intermediatePoints().size() + 1) {
        QList<DRelation::IntermediatePoint> intermediatePoints = m_relation->intermediatePoints();
        intermediatePoints.insert(beforeIndex - 1, DRelation::IntermediatePoint(pos));

        m_diagramSceneModel->diagramController()->startUpdateElement(m_relation, m_diagramSceneModel->diagram(), DiagramController::UpdateMajor);
        m_relation->setIntermediatePoints(intermediatePoints);
        m_diagramSceneModel->diagramController()->finishUpdateElement(m_relation, m_diagramSceneModel->diagram(), false);
    }
}

void RelationItem::deleteHandle(int index)
{
    if (index >= 1 && index <= m_relation->intermediatePoints().size()) {
        QList<DRelation::IntermediatePoint> intermediatePoints = m_relation->intermediatePoints();
        intermediatePoints.removeAt(index - 1);

        m_diagramSceneModel->diagramController()->startUpdateElement(m_relation, m_diagramSceneModel->diagram(), DiagramController::UpdateMajor);
        m_relation->setIntermediatePoints(intermediatePoints);
        m_diagramSceneModel->diagramController()->finishUpdateElement(m_relation, m_diagramSceneModel->diagram(), false);
    }
}

void RelationItem::setHandlePos(int index, const QPointF &pos)
{
    if (index == 0) {
        // TODO implement
    } else if (index == m_relation->intermediatePoints().size() + 1) {
        // TODO implement
    } else {
        QList<DRelation::IntermediatePoint> intermediatePoints = m_relation->intermediatePoints();
        --index;
        QMT_CHECK(index >= 0 && index < intermediatePoints.size());
        intermediatePoints[index].setPos(pos);

        m_diagramSceneModel->diagramController()->startUpdateElement(m_relation, m_diagramSceneModel->diagram(), DiagramController::UpdateMinor);
        m_relation->setIntermediatePoints(intermediatePoints);
        m_diagramSceneModel->diagramController()->finishUpdateElement(m_relation, m_diagramSceneModel->diagram(), false);
    }
}

void RelationItem::alignHandleToRaster(int index, double rasterWidth, double rasterHeight)
{
    if (index == 0) {
        // TODO implement
    } else if (index ==m_relation->intermediatePoints().size() + 1) {
        // TODO implement
    } else {
        QList<DRelation::IntermediatePoint> intermediatePoints = m_relation->intermediatePoints();
        --index;
        QMT_CHECK(index >= 0 && index < intermediatePoints.size());

        QPointF pos = intermediatePoints.at(index).pos();
        double x = qRound(pos.x() / rasterWidth) * rasterWidth;
        double y = qRound(pos.y() / rasterHeight) * rasterHeight;
        intermediatePoints[index].setPos(QPointF(x, y));

        m_diagramSceneModel->diagramController()->startUpdateElement(m_relation, m_diagramSceneModel->diagram(), DiagramController::UpdateMinor);
        m_relation->setIntermediatePoints(intermediatePoints);
        m_diagramSceneModel->diagramController()->finishUpdateElement(m_relation, m_diagramSceneModel->diagram(), false);
    }
}

void RelationItem::update()
{
    prepareGeometryChange();

    const Style *style = adaptedStyle();

    if (!m_arrow)
        m_arrow = new ArrowItem(this);

    update(style);
}

void RelationItem::update(const Style *style)
{
    QPointF endBPos = calcEndPoint(m_relation->endBUid(), m_relation->endAUid(), m_relation->intermediatePoints().size() - 1);
    QPointF endAPos = calcEndPoint(m_relation->endAUid(), endBPos, 0);

    setPos(endAPos);

    QList<QPointF> points;
    points << (endAPos - endAPos);
    foreach (const DRelation::IntermediatePoint &point, m_relation->intermediatePoints())
        points << (point.pos() - endAPos);
    points << (endBPos - endAPos);

    ArrowConfigurator visitor(m_diagramSceneModel, m_arrow, points);
    m_relation->accept(&visitor);
    m_arrow->update(style);

    if (!m_relation->name().isEmpty()) {
        if (!m_name)
            m_name = new QGraphicsSimpleTextItem(this);
        m_name->setFont(style->smallFont());
        m_name->setBrush(style->textBrush());
        m_name->setText(m_relation->name());
        m_name->setPos(m_arrow->calcPointAtPercent(0.5) + QPointF(-m_name->boundingRect().width() * 0.5, 4.0));
    } else if (m_name) {
        m_name->scene()->removeItem(m_name);
        delete m_name;
        m_name = 0;
    }

    if (!m_relation->stereotypes().isEmpty()) {
        if (!m_stereotypes)
            m_stereotypes = new StereotypesItem(this);
        m_stereotypes->setFont(style->smallFont());
        m_stereotypes->setBrush(style->textBrush());
        m_stereotypes->setStereotypes(m_relation->stereotypes());
        m_stereotypes->setPos(m_arrow->calcPointAtPercent(0.5) + QPointF(-m_stereotypes->boundingRect().width() * 0.5, -m_stereotypes->boundingRect().height() - 4.0));
    } else if (m_stereotypes) {
        m_stereotypes->scene()->removeItem(m_stereotypes);
        delete m_stereotypes;
        m_stereotypes = 0;
    }

    if (isSelected() || isSecondarySelected()) {
        if (!m_selectionHandles)
            m_selectionHandles = new PathSelectionItem(this, this);
        m_selectionHandles->setPoints(points);
        m_selectionHandles->setSecondarySelected(isSelected() ? false : isSecondarySelected());
    } else if (m_selectionHandles) {
        if (m_selectionHandles->scene())
            m_selectionHandles->scene()->removeItem(m_selectionHandles);
        delete m_selectionHandles;
        m_selectionHandles = 0;
    }

    setZValue((isSelected() || isSecondarySelected()) ? RELATION_ITEMS_ZVALUE_SELECTED : RELATION_ITEMS_ZVALUE);
}

void RelationItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
        m_diagramSceneModel->selectItem(this, event->modifiers() & Qt::ControlModifier);
}

void RelationItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
}

void RelationItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
}

const Style *RelationItem::adaptedStyle()
{
    DObject *endAObject = m_diagramSceneModel->diagramController()->findElement<DObject>(m_relation->endAUid(), m_diagramSceneModel->diagram());
    DObject *endBObject = m_diagramSceneModel->diagramController()->findElement<DObject>(m_relation->endBUid(), m_diagramSceneModel->diagram());
    StyledRelation styledRelation(m_relation, endAObject, endBObject);
    return m_diagramSceneModel->styleController()->adaptRelationStyle(styledRelation);
}

QPointF RelationItem::calcEndPoint(const Uid &end, const Uid &otherEnd, int nearestIntermediatePointIndex)
{
    QPointF otherEndPos;
    if (nearestIntermediatePointIndex >= 0 && nearestIntermediatePointIndex < m_relation->intermediatePoints().size()) {
        // otherEndPos will not be used
    } else {
        DObject *endOtherObject = m_diagramSceneModel->diagramController()->findElement<DObject>(otherEnd, m_diagramSceneModel->diagram());
        QMT_CHECK(endOtherObject);
        otherEndPos = endOtherObject->pos();
    }
    return calcEndPoint(end, otherEndPos, nearestIntermediatePointIndex);
}

QPointF RelationItem::calcEndPoint(const Uid &end, const QPointF &otherEndPos, int nearestIntermediatePointIndex)
{
    QGraphicsItem *endItem = m_diagramSceneModel->graphicsItem(end);
    QMT_CHECK(endItem);
    auto endObjectItem = dynamic_cast<IIntersectionable *>(endItem);
    QPointF endPos;
    if (endObjectItem) {
        DObject *endObject = m_diagramSceneModel->diagramController()->findElement<DObject>(end, m_diagramSceneModel->diagram());
        QMT_CHECK(endObject);
        bool preferAxis = false;
        QPointF otherPos;
        if (nearestIntermediatePointIndex >= 0 && nearestIntermediatePointIndex < m_relation->intermediatePoints().size()) {
            otherPos = m_relation->intermediatePoints().at(nearestIntermediatePointIndex).pos();
            preferAxis = true;
        } else {
            otherPos = otherEndPos;
        }

        bool ok = false;
        QLineF directLine(endObject->pos(), otherPos);
        if (preferAxis) {
            {
                QPointF axisDirection = GeometryUtilities::calcPrimaryAxisDirection(directLine);
                QLineF axis(otherPos, otherPos + axisDirection);
                QPointF projection = GeometryUtilities::calcProjection(axis, endObject->pos());
                QLineF projectedLine(projection, otherPos);
                ok = endObjectItem->intersectShapeWithLine(projectedLine, &endPos);
            }
            if (!ok) {
                QPointF axisDirection = GeometryUtilities::calcSecondaryAxisDirection(directLine);
                QLineF axis(otherPos, otherPos + axisDirection);
                QPointF projection = GeometryUtilities::calcProjection(axis, endObject->pos());
                QLineF projectedLine(projection, otherPos);
                ok = endObjectItem->intersectShapeWithLine(projectedLine, &endPos);
            }
        }
        if (!ok)
            ok = endObjectItem->intersectShapeWithLine(directLine, &endPos);
        if (!ok)
            endPos = endItem->pos();
    } else {
        endPos = endItem->pos();
    }
    return endPos;
}

} // namespace qmt
