/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "relationitem.h"

#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram_controller/dvoidvisitor.h"
#include "qmt/diagram/dassociation.h"
#include "qmt/diagram/dconnection.h"
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
#include "qmt/stereotype/customrelation.h"
#include "qmt/stereotype/stereotypecontroller.h"
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
        QMT_ASSERT(baseObject, return);
        bool baseIsInterface = false;
        bool lollipopDisplay = false;
        if (baseObject) {
            baseIsInterface = baseObject->stereotypes().contains("interface");
            if (baseIsInterface) {
                StereotypeDisplayVisitor stereotypeDisplayVisitor;
                stereotypeDisplayVisitor.setModelController(m_diagramSceneModel->diagramSceneController()->modelController());
                stereotypeDisplayVisitor.setStereotypeController(m_diagramSceneModel->stereotypeController());
                baseObject->accept(&stereotypeDisplayVisitor);
                lollipopDisplay = stereotypeDisplayVisitor.stereotypeDisplay() == DObject::StereotypeIcon;
            }
        }
        if (lollipopDisplay) {
            m_arrow->setShaft(ArrowItem::ShaftSolid);
            m_arrow->setEndHead(ArrowItem::HeadNone);
        } else if (baseIsInterface || inheritance->stereotypes().contains("realize")) {
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
        bool isRealization = dependency->stereotypes().contains("realize");
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

    void visitDConnection(const DConnection *connection)
    {

        ArrowItem::Shaft shaft = ArrowItem::ShaftSolid;
        ArrowItem::Head endAHead = ArrowItem::HeadNone;
        ArrowItem::Head endBHead = ArrowItem::HeadNone;

        CustomRelation customRelation = m_diagramSceneModel->stereotypeController()->findCustomRelation(connection->customRelationId());
        if (!customRelation.isNull()) {
            // TODO support custom shapes
            static const QHash<CustomRelation::ShaftPattern, ArrowItem::Shaft> shaft2shaft = {
                { CustomRelation::ShaftPattern::Solid, ArrowItem::ShaftSolid },
                { CustomRelation::ShaftPattern::Dash, ArrowItem::ShaftDashed },
                { CustomRelation::ShaftPattern::Dot, ArrowItem::ShaftDot },
                { CustomRelation::ShaftPattern::DashDot, ArrowItem::ShaftDashDot },
                { CustomRelation::ShaftPattern::DashDotDot, ArrowItem::ShaftDashDotDot },
            };
            static const QHash<CustomRelation::Head, ArrowItem::Head> head2head = {
                { CustomRelation::Head::None, ArrowItem::HeadNone },
                { CustomRelation::Head::Shape, ArrowItem::HeadNone },
                { CustomRelation::Head::Arrow, ArrowItem::HeadOpen },
                { CustomRelation::Head::Triangle, ArrowItem::HeadTriangle },
                { CustomRelation::Head::FilledTriangle, ArrowItem::HeadFilledTriangle },
                { CustomRelation::Head::Diamond, ArrowItem::HeadDiamond },
                { CustomRelation::Head::FilledDiamond, ArrowItem::HeadFilledDiamond },
            };
            shaft = shaft2shaft.value(customRelation.shaftPattern());
            endAHead = head2head.value(customRelation.endA().head());
            endBHead = head2head.value(customRelation.endB().head());
            // TODO color
        }

        m_arrow->setShaft(shaft);
        m_arrow->setArrowSize(12.0);
        m_arrow->setDiamondSize(12.0);
        m_arrow->setStartHead(endAHead);
        m_arrow->setEndHead(endBHead);
        m_arrow->setPoints(m_points);
    }

private:
    DiagramSceneModel *m_diagramSceneModel = nullptr;
    ArrowItem *m_arrow = nullptr;
    QList<QPointF> m_points;
};

bool RelationItem::m_grabbedEndA = false;
bool RelationItem::m_grabbedEndB = false;
QPointF RelationItem::m_grabbedEndPos;

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

QRectF RelationItem::getSecondarySelectionBoundary()
{
    return QRectF();
}

void RelationItem::setBoundarySelected(const QRectF &boundary, bool secondary)
{
    // TODO make individual intermediate points selectable
    Q_UNUSED(boundary)
    Q_UNUSED(secondary)
}

QPointF RelationItem::grabHandle(int index)
{
    if (index == 0) {
        m_grabbedEndA = true;
        QPointF endBPos = calcEndPoint(m_relation->endBUid(), m_relation->endAUid(), m_relation->intermediatePoints().size() - 1);
        QPointF endAPos = calcEndPoint(m_relation->endAUid(), endBPos, 0);
        m_grabbedEndPos = endAPos;
        return endAPos;
    } else if (index == m_relation->intermediatePoints().size() + 1) {
        m_grabbedEndB = true;
        QPointF endBPos = calcEndPoint(m_relation->endBUid(), m_relation->endAUid(), m_relation->intermediatePoints().size() - 1);
        m_grabbedEndPos = endBPos;
        return endBPos;
    } else {
        QList<DRelation::IntermediatePoint> intermediatePoints = m_relation->intermediatePoints();
        --index;
        QMT_ASSERT(index >= 0 && index < intermediatePoints.size(), return QPointF());
        return intermediatePoints.at(index).pos();
    }
}

void RelationItem::insertHandle(int beforeIndex, const QPointF &pos, double rasterWidth, double rasterHeight)
{
    if (beforeIndex == 0)
        ++beforeIndex;
    if (beforeIndex >= 1 && beforeIndex <= m_relation->intermediatePoints().size() + 1) {
        QList<DRelation::IntermediatePoint> intermediatePoints = m_relation->intermediatePoints();
        double x = qRound(pos.x() / rasterWidth) * rasterWidth;
        double y = qRound(pos.y() / rasterHeight) * rasterHeight;
        intermediatePoints.insert(beforeIndex - 1, DRelation::IntermediatePoint(QPointF(x, y)));

        m_diagramSceneModel->diagramController()->startUpdateElement(m_relation, m_diagramSceneModel->diagram(), DiagramController::UpdateMajor);
        m_relation->setIntermediatePoints(intermediatePoints);
        m_diagramSceneModel->diagramController()->finishUpdateElement(m_relation, m_diagramSceneModel->diagram(), false);
    }
}

void RelationItem::deleteHandle(int index)
{
    if (index == 0)
        ++index;
    else if (index == m_relation->intermediatePoints().size() + 1)
        --index;
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
        m_grabbedEndPos = pos;
        update();
    } else if (index == m_relation->intermediatePoints().size() + 1) {
        m_grabbedEndPos = pos;
        update();
    } else {
        QList<DRelation::IntermediatePoint> intermediatePoints = m_relation->intermediatePoints();
        --index;
        QMT_ASSERT(index >= 0 && index < intermediatePoints.size(), return);
        intermediatePoints[index].setPos(pos);

        m_diagramSceneModel->diagramController()->startUpdateElement(m_relation, m_diagramSceneModel->diagram(), DiagramController::UpdateMinor);
        m_relation->setIntermediatePoints(intermediatePoints);
        m_diagramSceneModel->diagramController()->finishUpdateElement(m_relation, m_diagramSceneModel->diagram(), false);
    }
}

void RelationItem::dropHandle(int index, double rasterWidth, double rasterHeight)
{
    if (index == 0) {
        m_grabbedEndA = false;
        DObject *targetObject = m_diagramSceneModel->findTopmostObject(m_grabbedEndPos);
        if (!m_diagramSceneModel->diagramSceneController()->relocateRelationEndA(m_relation, targetObject))
            update();
    } else if (index == m_relation->intermediatePoints().size() + 1) {
        m_grabbedEndB = false;
        DObject *targetObject = m_diagramSceneModel->findTopmostObject(m_grabbedEndPos);
        if (!m_diagramSceneModel->diagramSceneController()->relocateRelationEndB(m_relation, targetObject))
            update();
    } else {
        QList<DRelation::IntermediatePoint> intermediatePoints = m_relation->intermediatePoints();
        --index;
        QMT_ASSERT(index >= 0 && index < intermediatePoints.size(), return);

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
    QPointF endAPos;
    QPointF endBPos;

    if (m_grabbedEndA) {
        endAPos = m_grabbedEndPos;
        endBPos = calcEndPoint(m_relation->endBUid(), endAPos, m_relation->intermediatePoints().size() - 1);
    } else if (m_grabbedEndB) {
        endBPos = m_grabbedEndPos;
        endAPos = calcEndPoint(m_relation->endAUid(), endBPos, 0);
    } else {
        endBPos = calcEndPoint(m_relation->endBUid(), m_relation->endAUid(), m_relation->intermediatePoints().size() - 1);
        endAPos = calcEndPoint(m_relation->endAUid(), endBPos, 0);
    }

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
        m_name = nullptr;
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
        m_stereotypes = nullptr;
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
        m_selectionHandles = nullptr;
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
        QMT_ASSERT(endOtherObject, return QPointF());
        if (endOtherObject)
            otherEndPos = endOtherObject->pos();
    }
    return calcEndPoint(end, otherEndPos, nearestIntermediatePointIndex);
}

QPointF RelationItem::calcEndPoint(const Uid &end, const QPointF &otherEndPos, int nearestIntermediatePointIndex)
{
    QGraphicsItem *endItem = m_diagramSceneModel->graphicsItem(end);
    if (!endItem)
        return QPointF(0, 0);
    auto endObjectItem = dynamic_cast<IIntersectionable *>(endItem);
    QPointF endPos;
    if (endObjectItem) {
        DObject *endObject = m_diagramSceneModel->diagramController()->findElement<DObject>(end, m_diagramSceneModel->diagram());
        QMT_ASSERT(endObject, return QPointF());
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
