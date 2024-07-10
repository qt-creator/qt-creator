// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    updateCustomIcon(style);

    // shape
    if (!customIconItem()) {
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

    updateSelectionMarker(customIconItem());
    updateAlignmentButtons();
    updateGeometry();
}

bool DiagramItem::intersectShapeWithLine(const QLineF &line, QPointF *intersectionPoint, QLineF *intersectionLine) const
{
    if (customIconItem()) {
        QList<QPolygonF> polygons = customIconItem()->outline();
        for (int i = 0; i < polygons.size(); ++i)
            polygons[i].translate(object()->pos() + object()->rect().topLeft());
        if (shapeIcon().textAlignment() == qmt::StereotypeIcon::TextalignBelow) {
            if (nameItem()) {
                QPolygonF polygon(nameItem()->boundingRect());
                polygon.translate(object()->pos() + nameItem()->pos());
                polygons.append(polygon);
            }
        }
        return GeometryUtilities::intersect(polygons, line, nullptr, intersectionPoint, intersectionLine);
    }
    QPolygonF polygon;
    QRectF rect = object()->rect();
    rect.translate(object()->pos());
    // TODO use real outline
    polygon << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft() << rect.topLeft();
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

    if (customIconItem()) {
        QSizeF sz = customIconItemMinimumSize(customIconItem(),
                                              CUSTOM_ICON_MINIMUM_AUTO_WIDTH, CUSTOM_ICON_MINIMUM_AUTO_HEIGHT);
        if (shapeIcon().textAlignment() != qmt::StereotypeIcon::TextalignTop
                && shapeIcon().textAlignment() != qmt::StereotypeIcon::TextalignCenter)
            return sz;
        width = sz.width();
    }

    height += BODY_VERT_BORDER;
    if (CustomIconItem *stereotypeIconItem = this->stereotypeIconItem()) {
        width = std::max(width, stereotypeIconItem->boundingRect().width());
        height += std::max(FOLD_HEIGHT, stereotypeIconItem->boundingRect().height());
    } else {
        height += FOLD_HEIGHT;
    }
    if (StereotypesItem *stereotypesItem = this->stereotypesItem()) {
        width = std::max(width, stereotypesItem->boundingRect().width());
        height += stereotypesItem->boundingRect().height();
    }
    if (nameItem()) {
        width = std::max(width, nameItem()->boundingRect().width());
        height += nameItem()->boundingRect().height();
    }
    height += BODY_VERT_BORDER;

    width = BODY_HORIZ_BORDER + width + BODY_HORIZ_BORDER;

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
        correctAutoSize(customIconItem(), width, height, MINIMUM_AUTO_WIDTH, MINIMUM_AUTO_HEIGHT);
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

    if (customIconItem()) {
        customIconItem()->setPos(left, top);
        customIconItem()->setActualSize(QSizeF(width, height));
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

    if (customIconItem()) {
        switch (shapeIcon().textAlignment()) {
        case qmt::StereotypeIcon::TextalignBelow:
            y += height + BODY_VERT_BORDER;
            break;
        case qmt::StereotypeIcon::TextalignCenter:
        {
            double h = 0.0;
            if (CustomIconItem *stereotypeIconItem = this->stereotypeIconItem())
                h += stereotypeIconItem->boundingRect().height();
            if (StereotypesItem *stereotypesItem = this->stereotypesItem())
                h += stereotypesItem->boundingRect().height();
            if (nameItem())
                h += nameItem()->boundingRect().height();
            y = top + (height - h) / 2.0;
            break;
        }
        case qmt::StereotypeIcon::TextalignNone:
            // nothing to do
            break;
        case qmt::StereotypeIcon::TextalignTop:
            y += BODY_VERT_BORDER;
            break;
        }
    } else {
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
