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

#include "associationitem.h"

#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram/dassociation.h"
#include "qmt/diagram_scene/capabilities/intersectionable.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/parts/arrowitem.h"
#include "qmt/infrastructure/geometryutilities.h"
#include "qmt/infrastructure/qmtassert.h"
#include "qmt/style/style.h"

#include <QGraphicsScene>
#include <QFont>
#include <QPen>
#include <QBrush>
#include <QVector2D>
#include <QPair>
#include <qdebug.h>


namespace qmt {

AssociationItem::AssociationItem(DAssociation *association, DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent)
    : RelationItem(association, diagram_scene_model, parent),
      m_association(association),
      m_endAName(0),
      m_endACardinality(0),
      m_endBName(0),
      m_endBCardinality(0)
{
}

AssociationItem::~AssociationItem()
{
}

void AssociationItem::update(const Style *style)
{
    RelationItem::update(style);

    updateEndLabels(m_association->getA(), m_association->getB(), &m_endAName, &m_endACardinality, style);
    updateEndLabels(m_association->getB(), m_association->getA(), &m_endBName, &m_endBCardinality, style);

    QMT_CHECK(m_arrow);
    QGraphicsItem *end_a_item = m_diagramSceneModel->getGraphicsItem(m_association->getEndA());
    QMT_CHECK(end_a_item);
    placeEndLabels(m_arrow->getFirstLineSegment(), m_endAName, m_endACardinality, end_a_item, m_arrow->getStartHeadLength());
    QGraphicsItem *end_b_item = m_diagramSceneModel->getGraphicsItem(m_association->getEndB());
    QMT_CHECK(end_b_item);
    placeEndLabels(m_arrow->getLastLineSegment(), m_endBName, m_endBCardinality, end_b_item, m_arrow->getEndHeadLength());
}

void AssociationItem::updateEndLabels(const DAssociationEnd &end, const DAssociationEnd &other_end, QGraphicsSimpleTextItem **end_name, QGraphicsSimpleTextItem **end_cardinality, const Style *style)
{
    Q_UNUSED(end);

    if (!other_end.getName().isEmpty()) {
        if (!*end_name) {
            *end_name = new QGraphicsSimpleTextItem(this);
        }
        (*end_name)->setFont(style->getSmallFont());
        (*end_name)->setBrush(style->getTextBrush());
        (*end_name)->setText(other_end.getName());
    } else if (*end_name) {
        (*end_name)->scene()->removeItem(*end_name);
        delete *end_name;
        *end_name = 0;
    }

    if (!other_end.getCardinality().isEmpty()) {
        if (!*end_cardinality) {
            *end_cardinality = new QGraphicsSimpleTextItem(this);
        }
        (*end_cardinality)->setFont(style->getSmallFont());
        (*end_cardinality)->setBrush(style->getTextBrush());
        (*end_cardinality)->setText(other_end.getCardinality());
    } else if (*end_cardinality) {
        (*end_cardinality)->scene()->removeItem(*end_cardinality);
        delete *end_cardinality;
        *end_cardinality = 0;
    }
}

void AssociationItem::placeEndLabels(const QLineF &line_segment, QGraphicsItem *end_name, QGraphicsItem *end_cardinality, QGraphicsItem *end_item, double head_length)
{
    const double HEAD_OFFSET = head_length + 6.0;
    const double SIDE_OFFSET = 4.0;
    QPointF head_offset = QPointF(HEAD_OFFSET, 0);
    QPointF side_offset = QPointF(0.0, SIDE_OFFSET);

    double angle = GeometryUtilities::calcAngle(line_segment);
    if (angle >= -5 && angle <= 5) {
        if (end_name) {
            end_name->setPos(line_segment.p1() + head_offset + side_offset);
        }
        if (end_cardinality) {
            end_cardinality->setPos(line_segment.p1() + head_offset - side_offset - end_cardinality->boundingRect().bottomLeft());
        }
    } else if (angle <= -175 || angle >= 175) {
        if (end_name) {
            end_name->setPos(line_segment.p1() - head_offset + side_offset - end_name->boundingRect().topRight());
        }
        if (end_cardinality) {
            end_cardinality->setPos(line_segment.p1() - head_offset - side_offset - end_cardinality->boundingRect().bottomRight());
        }
    } else {
        QRectF rect;
        if (end_cardinality) {
            rect = end_cardinality->boundingRect();
        }
        if (end_name) {
            rect = rect.united(end_name->boundingRect().translated(rect.bottomLeft()));
        }

        QPointF rect_placement;
        GeometryUtilities::Side aligned_side = GeometryUtilities::SIDE_UNSPECIFIED;

        if (IIntersectionable *object_item = dynamic_cast<IIntersectionable *>(end_item)) {
            QPointF intersection_point;
            QLineF intersection_line;

            if (object_item->intersectShapeWithLine(GeometryUtilities::stretch(line_segment.translated(pos()), 2.0, 0.0), &intersection_point, &intersection_line)) {
                if (!GeometryUtilities::placeRectAtLine(rect, line_segment, HEAD_OFFSET, SIDE_OFFSET, intersection_line, &rect_placement, &aligned_side)) {
                    rect_placement = intersection_point;
                }
            } else {
                rect_placement = line_segment.p1();
            }
        } else {
            rect_placement = end_item->pos();
        }

        if (end_cardinality) {
            if (aligned_side == GeometryUtilities::SIDE_RIGHT) {
                end_cardinality->setPos(rect_placement + QPointF(rect.width() - end_cardinality->boundingRect().width(), 0.0));
            } else {
                end_cardinality->setPos(rect_placement);
            }
            rect_placement += end_cardinality->boundingRect().bottomLeft();
        }
        if (end_name) {
            if (aligned_side == GeometryUtilities::SIDE_RIGHT) {
                end_name->setPos(rect_placement + QPointF(rect.width() - end_name->boundingRect().width(), 0.0));
            } else {
                end_name->setPos(rect_placement);
            }
        }
    }
}

}
