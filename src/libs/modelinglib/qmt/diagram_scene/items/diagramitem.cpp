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

#include "diagramitem.h"

#include "qmt/diagram/ddiagram.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/parts/customiconitem.h"
#include "qmt/diagram_scene/parts/editabletextitem.h"
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

DiagramItem::DiagramItem(DDiagram *diagram, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent)
    : ObjectItem("diagram", diagram, diagramSceneModel, parent)
{
}

DiagramItem::~DiagramItem()
{
}

void DiagramItem::update()
{
    prepareGeometryChange();
    updateStereotypeIconDisplay();

    const Style *style = adaptedStyle(stereotypeIconId());

    // custom icon
    if (stereotypeIconDisplay() == StereotypeIcon::DisplayIcon) {
        if (!m_customIcon)
            m_customIcon = new CustomIconItem(diagramSceneModel(), this);
        m_customIcon->setStereotypeIconId(stereotypeIconId());
        m_customIcon->setBaseSize(stereotypeIconMinimumSize(m_customIcon->stereotypeIcon(), CUSTOM_ICON_MINIMUM_AUTO_WIDTH, CUSTOM_ICON_MINIMUM_AUTO_HEIGHT));
        m_customIcon->setBrush(style->fillBrush());
        m_customIcon->setPen(style->outerLinePen());
        m_customIcon->setZValue(SHAPE_ZVALUE);
    } else if (m_customIcon) {
        m_customIcon->scene()->removeItem(m_customIcon);
        delete m_customIcon;
        m_customIcon = nullptr;
    }

    // shape
    if (!m_customIcon) {
        if (!m_body)
            m_body = new QGraphicsPolygonItem(this);
        m_body->setBrush(style->fillBrush());
        m_body->setPen(style->outerLinePen());
        m_body->setZValue(SHAPE_ZVALUE);
        if (!m_fold)
            m_fold = new QGraphicsPolygonItem(this);
        m_fold->setBrush(style->extraFillBrush());
        m_fold->setPen(style->outerLinePen());
        m_fold->setZValue(SHAPE_DETAILS_ZVALUE);
    } else {
        if (m_fold) {
            m_fold->scene()->removeItem(m_fold);
            delete m_fold;
            m_fold = nullptr;
        }
        if (m_body) {
            m_body->scene()->removeItem(m_body);
            delete m_body;
            m_body = nullptr;
        }
    }

    // stereotypes
    updateStereotypes(stereotypeIconId(), stereotypeIconDisplay(), style);

    // diagram name
    updateNameItem(style);

    updateSelectionMarker(m_customIcon);
    updateAlignmentButtons();
    updateGeometry();
}

bool DiagramItem::intersectShapeWithLine(const QLineF &line, QPointF *intersectionPoint, QLineF *intersectionLine) const
{
    QPolygonF polygon;
    if (m_customIcon) {
        // TODO use customIcon path as shape
        QRectF rect = object()->rect();
        rect.translate(object()->pos());
        polygon << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft() << rect.topLeft();
    } else {
        QRectF rect = object()->rect();
        rect.translate(object()->pos());
        polygon << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft() << rect.topLeft();
    }
    return GeometryUtilities::intersect(polygon, line, intersectionPoint, intersectionLine);
}

QSizeF DiagramItem::minimumSize() const
{
    return calcMinimumGeometry();
}

QSizeF DiagramItem::calcMinimumGeometry() const
{
    double width = MINIMUM_WIDTH;
    double height = 0.0;

    if (m_customIcon) {
        return stereotypeIconMinimumSize(m_customIcon->stereotypeIcon(),
                                         CUSTOM_ICON_MINIMUM_AUTO_WIDTH, CUSTOM_ICON_MINIMUM_AUTO_HEIGHT);
    }

    height += BODY_VERT_BORDER;
    if (CustomIconItem *stereotypeIconItem = this->stereotypeIconItem()) {
        width = std::max(width, stereotypeIconItem->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += std::max(FOLD_HEIGHT, stereotypeIconItem->boundingRect().height());
    } else {
        height += FOLD_HEIGHT;
    }
    if (StereotypesItem *stereotypesItem = this->stereotypesItem()) {
        width = std::max(width, stereotypesItem->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += stereotypesItem->boundingRect().height();
    }
    if (nameItem()) {
        width = std::max(width, nameItem()->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        height += nameItem()->boundingRect().height();
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

    if (object()->isAutoSized()) {
        if (!m_customIcon) {
            if (width < MINIMUM_AUTO_WIDTH)
                width = MINIMUM_AUTO_WIDTH;
            if (height < MINIMUM_AUTO_HEIGHT)
                height = MINIMUM_AUTO_HEIGHT;
        }
    } else {
        QRectF rect = object()->rect();
        if (rect.width() > width)
            width = rect.width();
        if (rect.height() > height)
            height = rect.height();
    }

    // update sizes and positions
    double left = -width / 2.0;
    double top = -height / 2.0;
    double y = top;

    setPos(object()->pos());

    QRectF rect(left, top, width, height);

    // the object is updated without calling DiagramController intentionally.
    // attribute rect is not a real attribute stored on DObject but
    // a backup for the graphics item used for manual resized and persistency.
    object()->setRect(rect);

    if (m_customIcon) {
        m_customIcon->setPos(left, top);
        m_customIcon->setActualSize(QSizeF(width, height));
        y += height;
    }

    if (m_body) {
        QPolygonF bodyPolygon;
        bodyPolygon
                << rect.topLeft()
                << rect.topRight() + QPointF(-FOLD_WIDTH, 0.0)
                << rect.topRight() + QPointF(0.0, FOLD_HEIGHT)
                << rect.bottomRight()
                << rect.bottomLeft();
        m_body->setPolygon(bodyPolygon);
    }
    if (m_fold) {
        QPolygonF foldPolygon;
        foldPolygon
                << rect.topRight() + QPointF(-FOLD_WIDTH, 0.0)
                << rect.topRight() + QPointF(0.0, FOLD_HEIGHT)
                << rect.topRight() + QPointF(-FOLD_WIDTH, FOLD_HEIGHT);
        m_fold->setPolygon(foldPolygon);
    }

    y += BODY_VERT_BORDER;
    if (!m_customIcon) {
        if (CustomIconItem *stereotypeIconItem = this->stereotypeIconItem()) {
            stereotypeIconItem->setPos(left + BODY_HORIZ_BORDER, y);
            y += std::max(FOLD_HEIGHT, stereotypeIconItem->boundingRect().height());
        } else {
            y += FOLD_HEIGHT;
        }
    }
    if (StereotypesItem *stereotypesItem = this->stereotypesItem()) {
        stereotypesItem->setPos(-stereotypesItem->boundingRect().width() / 2.0, y);
        y += stereotypesItem->boundingRect().height();
    }
    if (nameItem()) {
        nameItem()->setPos(-nameItem()->boundingRect().width() / 2.0, y);
    }

    updateSelectionMarkerGeometry(rect);
    updateAlignmentButtonsGeometry(rect);
    updateDepth();
}

} // namespace qmt
