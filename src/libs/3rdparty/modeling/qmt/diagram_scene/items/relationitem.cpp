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

class RelationItem::ArrowConfigurator :
        public DConstVoidVisitor
{
public:
    ArrowConfigurator(DiagramSceneModel *diagram_scene_model, ArrowItem *arrow, const QList<QPointF> &points)
        : m_diagramSceneModel(diagram_scene_model),
          m_arrow(arrow),
          m_points(points)
    {
    }

    void visitDInheritance(const DInheritance *inheritance)
    {
        Q_UNUSED(inheritance);

        DObject *base_object = m_diagramSceneModel->getDiagramController()->findElement<DObject>(inheritance->getBase(), m_diagramSceneModel->getDiagram());
        QMT_CHECK(base_object);
        bool base_is_interface = base_object->getStereotypes().contains(QStringLiteral("interface"));
        bool lollipop_display = false;
        if (base_is_interface) {
            StereotypeDisplayVisitor stereotype_display_visitor;
            stereotype_display_visitor.setModelController(m_diagramSceneModel->getDiagramSceneController()->getModelController());
            stereotype_display_visitor.setStereotypeController(m_diagramSceneModel->getStereotypeController());
            base_object->accept(&stereotype_display_visitor);
            lollipop_display = stereotype_display_visitor.getStereotypeDisplay() == DObject::STEREOTYPE_ICON;
        }
        if (lollipop_display) {
            m_arrow->setShaft(ArrowItem::SHAFT_SOLID);
            m_arrow->setEndHead(ArrowItem::HEAD_NONE);
        } else if (base_is_interface || inheritance->getStereotypes().contains(QStringLiteral("realize"))) {
            m_arrow->setShaft(ArrowItem::SHAFT_DASHED);
            m_arrow->setEndHead(ArrowItem::HEAD_TRIANGLE);
        } else {
            m_arrow->setShaft(ArrowItem::SHAFT_SOLID);
            m_arrow->setEndHead(ArrowItem::HEAD_TRIANGLE);
        }
        m_arrow->setArrowSize(16.0);
        m_arrow->setStartHead(ArrowItem::HEAD_NONE);
        m_arrow->setPoints(m_points);
    }

    void visitDDependency(const DDependency *dependency)
    {
        Q_UNUSED(dependency);

        ArrowItem::Head end_a_head = ArrowItem::HEAD_NONE;
        ArrowItem::Head end_b_head = ArrowItem::HEAD_NONE;
        bool is_realization = dependency->getStereotypes().contains(QStringLiteral("realize"));
        switch (dependency->getDirection()) {
        case MDependency::A_TO_B:
            end_b_head = is_realization ? ArrowItem::HEAD_TRIANGLE : ArrowItem::HEAD_OPEN;
            break;
        case MDependency::B_TO_A:
            end_a_head = is_realization ? ArrowItem::HEAD_TRIANGLE : ArrowItem::HEAD_OPEN;
            break;
        case MDependency::BIDIRECTIONAL:
            end_a_head = ArrowItem::HEAD_OPEN;
            end_b_head = ArrowItem::HEAD_OPEN;
            break;
        default:
            break;
        }

        m_arrow->setShaft(ArrowItem::SHAFT_DASHED);
        m_arrow->setArrowSize(12.0);
        m_arrow->setStartHead(end_a_head);
        m_arrow->setEndHead(end_b_head);
        m_arrow->setPoints(m_points);
    }

    void visitDAssociation(const DAssociation *association)
    {
        Q_UNUSED(association);

        m_arrow->setShaft(ArrowItem::SHAFT_SOLID);
        m_arrow->setArrowSize(12.0);
        m_arrow->setDiamondSize(12.0);

        ArrowItem::Head end_a_head = ArrowItem::HEAD_NONE;
        ArrowItem::Head end_b_head = ArrowItem::HEAD_NONE;

        bool a_nav = association->getA().isNavigable();
        bool b_nav = association->getB().isNavigable();

        bool a_flat = association->getA().getKind() == MAssociationEnd::ASSOCIATION;
        bool b_flat = association->getB().getKind() == MAssociationEnd::ASSOCIATION;

        switch (association->getA().getKind()) {
        case MAssociationEnd::ASSOCIATION:
            end_a_head = ((b_nav && !a_nav && b_flat) || (a_nav && b_nav && !b_flat)) ? ArrowItem::HEAD_FILLED_TRIANGLE : ArrowItem::HEAD_NONE;
            break;
        case MAssociationEnd::AGGREGATION:
            end_a_head = association->getB().isNavigable() ? ArrowItem::HEAD_DIAMOND_FILLED_TRIANGLE : ArrowItem::HEAD_DIAMOND;
            break;
        case MAssociationEnd::COMPOSITION:
            end_a_head = association->getB().isNavigable() ? ArrowItem::HEAD_FILLED_DIAMOND_FILLED_TRIANGLE : ArrowItem::HEAD_FILLED_DIAMOND;
            break;
        }

        switch (association->getB().getKind()) {
        case MAssociationEnd::ASSOCIATION:
            end_b_head = ((a_nav && !b_nav && a_flat) || (a_nav && b_nav && !a_flat)) ? ArrowItem::HEAD_FILLED_TRIANGLE : ArrowItem::HEAD_NONE;
            break;
        case MAssociationEnd::AGGREGATION:
            end_b_head = association->getA().isNavigable() ? ArrowItem::HEAD_DIAMOND_FILLED_TRIANGLE : ArrowItem::HEAD_DIAMOND;
            break;
        case MAssociationEnd::COMPOSITION:
            end_b_head = association->getA().isNavigable() ? ArrowItem::HEAD_FILLED_DIAMOND_FILLED_TRIANGLE : ArrowItem::HEAD_FILLED_DIAMOND;
            break;
        }

        m_arrow->setStartHead(end_a_head);
        m_arrow->setEndHead(end_b_head);
        m_arrow->setPoints(m_points);
    }

private:

    DiagramSceneModel *m_diagramSceneModel;

    ArrowItem *m_arrow;

    QList<QPointF> m_points;
};


RelationItem::RelationItem(DRelation *relation, DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_relation(relation),
      m_diagramSceneModel(diagram_scene_model),
      m_secondarySelected(false),
      m_focusSelected(false),
      m_arrow(0),
      m_name(0),
      m_stereotypes(0),
      m_selectionHandles(0)
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

#ifdef DEBUG_PAINT_SELECTION_SHAPE
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
    if (m_arrow) {
        path.addPath(m_arrow->shape().translated(m_arrow->pos()));
    }
    if (m_name) {
        path.addPath(m_name->shape().translated(m_name->pos()));
    }
    if (m_stereotypes) {
        path.addPath(m_stereotypes->shape().translated(m_stereotypes->pos()));
    }
    if (m_selectionHandles) {
        path.addPath(m_selectionHandles->shape().translated(m_selectionHandles->pos()));
    }
    return path;
}

void RelationItem::moveDelta(const QPointF &delta)
{
    m_diagramSceneModel->getDiagramController()->startUpdateElement(m_relation, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_GEOMETRY);
    QList<DRelation::IntermediatePoint> points;
    foreach (const DRelation::IntermediatePoint &point, m_relation->getIntermediatePoints()) {
        points << DRelation::IntermediatePoint(point.getPos() + delta);
    }
    m_relation->setIntermediatePoints(points);
    m_diagramSceneModel->getDiagramController()->finishUpdateElement(m_relation, m_diagramSceneModel->getDiagram(), false);
}

void RelationItem::alignItemPositionToRaster(double raster_width, double raster_height)
{
    m_diagramSceneModel->getDiagramController()->startUpdateElement(m_relation, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_GEOMETRY);
    QList<DRelation::IntermediatePoint> points;
    foreach (const DRelation::IntermediatePoint &point, m_relation->getIntermediatePoints()) {
        QPointF pos = point.getPos();
        double x = qRound(pos.x() / raster_width) * raster_width;
        double y = qRound(pos.y() / raster_height) * raster_height;
        points << DRelation::IntermediatePoint(QPointF(x,y));
    }
    m_relation->setIntermediatePoints(points);
    m_diagramSceneModel->getDiagramController()->finishUpdateElement(m_relation, m_diagramSceneModel->getDiagram(), false);
}

bool RelationItem::isSecondarySelected() const
{
    return m_secondarySelected;
}

void RelationItem::setSecondarySelected(bool secondary_selected)
{
    if (m_secondarySelected != secondary_selected) {
        m_secondarySelected = secondary_selected;
        update();
    }
}

bool RelationItem::isFocusSelected() const
{
    return m_focusSelected;
}

void RelationItem::setFocusSelected(bool focus_selected)
{
    if (m_focusSelected != focus_selected) {
        m_focusSelected = focus_selected;
        update();
    }
}

QPointF RelationItem::getHandlePos(int index)
{
    if (index == 0) {
        // TODO implement
        return QPointF(0,0);
    } else if (index == m_relation->getIntermediatePoints().size() + 1) {
        // TODO implement
        return QPointF(0,0);
    } else {
        QList<DRelation::IntermediatePoint> intermediate_points = m_relation->getIntermediatePoints();
        --index;
        QMT_CHECK(index >= 0 && index < intermediate_points.size());
        return intermediate_points.at(index).getPos();
    }
}

void RelationItem::insertHandle(int before_index, const QPointF &pos)
{
    if (before_index >= 1 && before_index <= m_relation->getIntermediatePoints().size() + 1) {
        QList<DRelation::IntermediatePoint> intermediate_points = m_relation->getIntermediatePoints();
        intermediate_points.insert(before_index - 1, DRelation::IntermediatePoint(pos));

        m_diagramSceneModel->getDiagramController()->startUpdateElement(m_relation, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_MAJOR);
        m_relation->setIntermediatePoints(intermediate_points);
        m_diagramSceneModel->getDiagramController()->finishUpdateElement(m_relation, m_diagramSceneModel->getDiagram(), false);
    }
}

void RelationItem::deleteHandle(int index)
{
    if (index >= 1 && index <= m_relation->getIntermediatePoints().size()) {
        QList<DRelation::IntermediatePoint> intermediate_points = m_relation->getIntermediatePoints();
        intermediate_points.removeAt(index - 1);

        m_diagramSceneModel->getDiagramController()->startUpdateElement(m_relation, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_MAJOR);
        m_relation->setIntermediatePoints(intermediate_points);
        m_diagramSceneModel->getDiagramController()->finishUpdateElement(m_relation, m_diagramSceneModel->getDiagram(), false);
    }
}

void RelationItem::setHandlePos(int index, const QPointF &pos)
{
    if (index == 0) {
        // TODO implement
    } else if (index == m_relation->getIntermediatePoints().size() + 1) {
        // TODO implement
    } else {
        QList<DRelation::IntermediatePoint> intermediate_points = m_relation->getIntermediatePoints();
        --index;
        QMT_CHECK(index >= 0 && index < intermediate_points.size());
        intermediate_points[index].setPos(pos);

        m_diagramSceneModel->getDiagramController()->startUpdateElement(m_relation, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_MINOR);
        m_relation->setIntermediatePoints(intermediate_points);
        m_diagramSceneModel->getDiagramController()->finishUpdateElement(m_relation, m_diagramSceneModel->getDiagram(), false);
    }
}

void RelationItem::alignHandleToRaster(int index, double raster_width, double raster_height)
{
    if (index == 0) {
        // TODO implement
    } else if (index ==m_relation->getIntermediatePoints().size() + 1) {
        // TODO implement
    } else {
        QList<DRelation::IntermediatePoint> intermediate_points = m_relation->getIntermediatePoints();
        --index;
        QMT_CHECK(index >= 0 && index < intermediate_points.size());

        QPointF pos = intermediate_points.at(index).getPos();
        double x = qRound(pos.x() / raster_width) * raster_width;
        double y = qRound(pos.y() / raster_height) * raster_height;
        intermediate_points[index].setPos(QPointF(x, y));

        m_diagramSceneModel->getDiagramController()->startUpdateElement(m_relation, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_MINOR);
        m_relation->setIntermediatePoints(intermediate_points);
        m_diagramSceneModel->getDiagramController()->finishUpdateElement(m_relation, m_diagramSceneModel->getDiagram(), false);
    }
}

void RelationItem::update()
{
    prepareGeometryChange();

    const Style *style = getAdaptedStyle();

    if (!m_arrow) {
        m_arrow = new ArrowItem(this);
    }

    update(style);
}

void RelationItem::update(const Style *style)
{
    QPointF end_b_pos = calcEndPoint(m_relation->getEndB(), m_relation->getEndA(), m_relation->getIntermediatePoints().size() - 1);
    QPointF end_a_pos = calcEndPoint(m_relation->getEndA(), end_b_pos, 0);

    setPos(end_a_pos);

    QList<QPointF> points;
    points << (end_a_pos - end_a_pos);
    foreach (const DRelation::IntermediatePoint &point, m_relation->getIntermediatePoints()) {
        points << (point.getPos() - end_a_pos);
    }
    points << (end_b_pos - end_a_pos);

    ArrowConfigurator visitor(m_diagramSceneModel, m_arrow, points);
    m_relation->accept(&visitor);
    m_arrow->update(style);

    if (!m_relation->getName().isEmpty()) {
        if (!m_name) {
            m_name = new QGraphicsSimpleTextItem(this);
        }
        m_name->setFont(style->getSmallFont());
        m_name->setBrush(style->getTextBrush());
        m_name->setText(m_relation->getName());
        m_name->setPos(m_arrow->calcPointAtPercent(0.5) + QPointF(-m_name->boundingRect().width() * 0.5, 4.0));
    } else if (m_name) {
        m_name->scene()->removeItem(m_name);
        delete m_name;
        m_name = 0;
    }

    if (!m_relation->getStereotypes().isEmpty()) {
        if (!m_stereotypes) {
            m_stereotypes = new StereotypesItem(this);
        }
        m_stereotypes->setFont(style->getSmallFont());
        m_stereotypes->setBrush(style->getTextBrush());
        m_stereotypes->setStereotypes(m_relation->getStereotypes());
        m_stereotypes->setPos(m_arrow->calcPointAtPercent(0.5) + QPointF(-m_stereotypes->boundingRect().width() * 0.5, -m_stereotypes->boundingRect().height() - 4.0));
    } else if (m_stereotypes) {
        m_stereotypes->scene()->removeItem(m_stereotypes);
        delete m_stereotypes;
        m_stereotypes = 0;
    }

    if (isSelected() || isSecondarySelected()) {
        if (!m_selectionHandles) {
            m_selectionHandles = new PathSelectionItem(this, this);
        }
        m_selectionHandles->setPoints(points);
        m_selectionHandles->setSecondarySelected(isSelected() ? false : isSecondarySelected());
    } else if (m_selectionHandles) {
        if (m_selectionHandles->scene()) {
            m_selectionHandles->scene()->removeItem(m_selectionHandles);
        }
        delete m_selectionHandles;
        m_selectionHandles = 0;
    }

    setZValue((isSelected() || isSecondarySelected()) ? RELATION_ITEMS_ZVALUE_SELECTED : RELATION_ITEMS_ZVALUE);
}

void RelationItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        m_diagramSceneModel->selectItem(this, event->modifiers() & Qt::ControlModifier);
    }
}

void RelationItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
}

void RelationItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
}

const Style *RelationItem::getAdaptedStyle()
{
    DObject *end_a_object = m_diagramSceneModel->getDiagramController()->findElement<DObject>(m_relation->getEndA(), m_diagramSceneModel->getDiagram());
    DObject *end_b_object = m_diagramSceneModel->getDiagramController()->findElement<DObject>(m_relation->getEndB(), m_diagramSceneModel->getDiagram());
    StyledRelation styled_relation(m_relation, end_a_object, end_b_object);
    return m_diagramSceneModel->getStyleController()->adaptRelationStyle(styled_relation);
}

QPointF RelationItem::calcEndPoint(const Uid &end, const Uid &other_end, int nearest_intermediate_point_index)
{
    QPointF other_end_pos;
    if (nearest_intermediate_point_index >= 0 && nearest_intermediate_point_index < m_relation->getIntermediatePoints().size()) {
        // other_end_pos will not be used
    } else {
        DObject *end_other_object = m_diagramSceneModel->getDiagramController()->findElement<DObject>(other_end, m_diagramSceneModel->getDiagram());
        QMT_CHECK(end_other_object);
        other_end_pos = end_other_object->getPos();
    }
    return calcEndPoint(end, other_end_pos, nearest_intermediate_point_index);
}

QPointF RelationItem::calcEndPoint(const Uid &end, const QPointF &other_end_pos, int nearest_intermediate_point_index)
{
    QGraphicsItem *end_item = m_diagramSceneModel->getGraphicsItem(end);
    QMT_CHECK(end_item);
    IIntersectionable *end_object_item = dynamic_cast<IIntersectionable *>(end_item);
    QPointF end_pos;
    if (end_object_item) {
        DObject *end_object = m_diagramSceneModel->getDiagramController()->findElement<DObject>(end, m_diagramSceneModel->getDiagram());
        QMT_CHECK(end_object);
        bool prefer_axis = false;
        QPointF other_pos;
        if (nearest_intermediate_point_index >= 0 && nearest_intermediate_point_index < m_relation->getIntermediatePoints().size()) {
            other_pos = m_relation->getIntermediatePoints().at(nearest_intermediate_point_index).getPos();
            prefer_axis = true;
        } else {
            other_pos = other_end_pos;
        }

        bool ok = false;
        QLineF direct_line(end_object->getPos(), other_pos);
        if (prefer_axis) {
            {
                QPointF axis_direction = GeometryUtilities::calcPrimaryAxisDirection(direct_line);
                QLineF axis(other_pos, other_pos + axis_direction);
                QPointF projection = GeometryUtilities::calcProjection(axis, end_object->getPos());
                QLineF projected_line(projection, other_pos);
                ok = end_object_item->intersectShapeWithLine(projected_line, &end_pos);
            }
            if (!ok) {
                QPointF axis_direction = GeometryUtilities::calcSecondaryAxisDirection(direct_line);
                QLineF axis(other_pos, other_pos + axis_direction);
                QPointF projection = GeometryUtilities::calcProjection(axis, end_object->getPos());
                QLineF projected_line(projection, other_pos);
                ok = end_object_item->intersectShapeWithLine(projected_line, &end_pos);
            }
        }
        if (!ok) {
            ok = end_object_item->intersectShapeWithLine(direct_line, &end_pos);
        }
        if (!ok) {
            end_pos = end_item->pos();
        }
    } else {
        end_pos = end_item->pos();
    }
    return end_pos;
}

}
