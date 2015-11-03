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

#include "diagramitem.h"

#include "qmt/diagram/ddiagram.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/parts/customiconitem.h"
#include "qmt/diagram_scene/parts/stereotypesitem.h"
#include "qmt/infrastructure/geometryutilities.h"
#include "qmt/stereotype/stereotypecontroller.h"
#include "qmt/stereotype/stereotypeicon.h"
#include "qmt/style/style.h"
#include "qmt/tasks/diagramscenecontroller.h"

#include <QGraphicsScene>
#include <QPointF>
#include <QFont>
#include <QBrush>
#include <QPen>


namespace qmt {

static const qreal MINIMUM_AUTO_WIDTH = 60.0;
static const qreal MINIMUM_AUTO_HEIGHT = 60.0;
static const qreal MINIMUM_WIDTH = 40.0;
static const qreal FOLD_WIDTH = 15.0;
static const qreal FOLD_HEIGHT = 15.0;
static const qreal BODY_HORIZ_BORDER = 4.0;
static const qreal BODY_VERT_BORDER = 4.0;


DiagramItem::DiagramItem(DDiagram *diagram, DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent)
    : ObjectItem(diagram, diagram_scene_model, parent),
      m_customIcon(0),
      m_body(0),
      m_fold(0),
      m_diagramName(0)
{
}

DiagramItem::~DiagramItem()
{
}

void DiagramItem::update()
{
    prepareGeometryChange();

    updateStereotypeIconDisplay();

    const Style *style = getAdaptedStyle(getStereotypeIconId());

    // custom icon
    if (getStereotypeIconDisplay() == StereotypeIcon::DISPLAY_ICON) {
        if (!m_customIcon) {
            m_customIcon = new CustomIconItem(getDiagramSceneModel(), this);
        }
        m_customIcon->setStereotypeIconId(getStereotypeIconId());
        m_customIcon->setBaseSize(getStereotypeIconMinimumSize(m_customIcon->getStereotypeIcon(), CUSTOM_ICON_MINIMUM_AUTO_WIDTH, CUSTOM_ICON_MINIMUM_AUTO_HEIGHT));
        m_customIcon->setBrush(style->getFillBrush());
        m_customIcon->setPen(style->getOuterLinePen());
        m_customIcon->setZValue(SHAPE_ZVALUE);
    } else if (m_customIcon) {
        m_customIcon->scene()->removeItem(m_customIcon);
        delete m_customIcon;
        m_customIcon = 0;
    }

    // shape
    if (!m_customIcon) {
        if (!m_body) {
            m_body = new QGraphicsPolygonItem(this);
        }
        m_body->setBrush(style->getFillBrush());
        m_body->setPen(style->getOuterLinePen());
        m_body->setZValue(SHAPE_ZVALUE);
        if (!m_fold) {
            m_fold = new QGraphicsPolygonItem(this);
        }
        m_fold->setBrush(style->getExtraFillBrush());
        m_fold->setPen(style->getOuterLinePen());
        m_fold->setZValue(SHAPE_DETAILS_ZVALUE);
    } else {
        if (m_fold) {
            m_fold->scene()->removeItem(m_fold);
            delete m_fold;
            m_fold = 0;
        }
        if (m_body) {
            m_body->scene()->removeItem(m_body);
            delete m_body;
            m_body = 0;
        }
    }

    // stereotypes
    updateStereotypes(getStereotypeIconId(), getStereotypeIconDisplay(), style);

    // diagram name
    if (!m_diagramName) {
        m_diagramName = new QGraphicsSimpleTextItem(this);
    }
    m_diagramName->setFont(style->getHeaderFont());
    m_diagramName->setBrush(style->getTextBrush());
    m_diagramName->setText(getObject()->getName());

    updateSelectionMarker(m_customIcon);

    updateAlignmentButtons();

    updateGeometry();
}

bool DiagramItem::intersectShapeWithLine(const QLineF &line, QPointF *intersection_point, QLineF *intersection_line) const
{
    QPolygonF polygon;
    if (m_customIcon) {
        // TODO use custom_icon path as shape
        QRectF rect = getObject()->getRect();
        rect.translate(getObject()->getPos());
        polygon << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft() << rect.topLeft();
    } else {
        QRectF rect = getObject()->getRect();
        rect.translate(getObject()->getPos());
        polygon << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft() << rect.topLeft();
    }
    return GeometryUtilities::intersect(polygon, line, intersection_point, intersection_line);
}

QSizeF DiagramItem::getMinimumSize() const
{
    return calcMinimumGeometry();
}

QSizeF DiagramItem::calcMinimumGeometry() const
{
    double width = MINIMUM_WIDTH;
    double height = 0.0;

    if (m_customIcon) {
        return getStereotypeIconMinimumSize(m_customIcon->getStereotypeIcon(), CUSTOM_ICON_MINIMUM_AUTO_WIDTH, CUSTOM_ICON_MINIMUM_AUTO_HEIGHT);
    }

    height += BODY_VERT_BORDER;
    if (CustomIconItem *stereotype_icon_item = getStereotypeIconItem()) {
        width = std::max(width, stereotype_icon_item->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += std::max(FOLD_HEIGHT, stereotype_icon_item->boundingRect().height());
    } else {
        height += FOLD_HEIGHT;
    }
    if (StereotypesItem *stereotypes_item = getStereotypesItem()) {
        width = std::max(width, stereotypes_item->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += stereotypes_item->boundingRect().height();
    }
    if (m_diagramName) {
        width = std::max(width, m_diagramName->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += m_diagramName->boundingRect().height();
    }
    height += BODY_VERT_BORDER;

    return GeometryUtilities::ensureMinimumRasterSize(QSizeF(width, height), 2 * RASTER_WIDTH, 2 * RASTER_HEIGHT);
}

void DiagramItem::updateGeometry()
{
    prepareGeometryChange();

    // calc width and height
    double width = 0.0;
    double height = 0.0;

    QSizeF geometry = calcMinimumGeometry();
    width = geometry.width();
    height = geometry.height();

    if (getObject()->hasAutoSize()) {
        if (!m_customIcon) {
            if (width < MINIMUM_AUTO_WIDTH) {
                width = MINIMUM_AUTO_WIDTH;
            }
            if (height < MINIMUM_AUTO_HEIGHT) {
                height = MINIMUM_AUTO_HEIGHT;
            }
        }
    } else {
        QRectF rect = getObject()->getRect();
        if (rect.width() > width) {
            width = rect.width();
        }
        if (rect.height() > height) {
            height = rect.height();
        }
    }

    // update sizes and positions
    double left = -width / 2.0;
    //double right = width / 2.0;
    double top = -height / 2.0;
    //double bottom = height / 2.0;
    double y = top;

    setPos(getObject()->getPos());

    QRectF rect(left, top, width, height);

    // the object is updated without calling DiagramController intentionally.
    // attribute rect is not a real attribute stored on DObject but
    // a backup for the graphics item used for manual resized and persistency.
    getObject()->setRect(rect);

    if (m_customIcon) {
        m_customIcon->setPos(left, top);
        m_customIcon->setActualSize(QSizeF(width, height));
        y += height;
    }

    if (m_body) {
        QPolygonF body_polygon;
        body_polygon
                << rect.topLeft()
                << rect.topRight() + QPointF(-FOLD_WIDTH, 0.0)
                << rect.topRight() + QPointF(0.0, FOLD_HEIGHT)
                << rect.bottomRight()
                << rect.bottomLeft();
        m_body->setPolygon(body_polygon);
    }
    if (m_fold) {
        QPolygonF fold_polygon;
        fold_polygon
                << rect.topRight() + QPointF(-FOLD_WIDTH, 0.0)
                << rect.topRight() + QPointF(0.0, FOLD_HEIGHT)
                << rect.topRight() + QPointF(-FOLD_WIDTH, FOLD_HEIGHT);
        m_fold->setPolygon(fold_polygon);
    }

    y += BODY_VERT_BORDER;
    if (!m_customIcon) {
        if (CustomIconItem *stereotype_icon_item = getStereotypeIconItem()) {
            stereotype_icon_item->setPos(left + BODY_HORIZ_BORDER, y);
            y += std::max(FOLD_HEIGHT, stereotype_icon_item->boundingRect().height());
        } else {
            y += FOLD_HEIGHT;
        }
    }
    if (StereotypesItem *stereotypes_item = getStereotypesItem()) {
        stereotypes_item->setPos(-stereotypes_item->boundingRect().width() / 2.0, y);
        y += stereotypes_item->boundingRect().height();
    }
    if (m_diagramName) {
        m_diagramName->setPos(-m_diagramName->boundingRect().width() / 2.0, y);
        y += m_diagramName->boundingRect().height();
    }

    updateSelectionMarkerGeometry(rect);

    updateAlignmentButtonsGeometry(rect);

    updateDepth();

}

}
