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

#include "packageitem.h"

#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram/dpackage.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/parts/contextlabelitem.h"
#include "qmt/diagram_scene/parts/customiconitem.h"
#include "qmt/diagram_scene/parts/relationstarter.h"
#include "qmt/diagram_scene/parts/stereotypesitem.h"
#include "qmt/infrastructure/geometryutilities.h"
#include "qmt/stereotype/stereotypecontroller.h"
#include "qmt/stereotype/stereotypeicon.h"
#include "qmt/style/style.h"
#include "qmt/tasks/diagramscenecontroller.h"

#include <QGraphicsScene>
#include <QBrush>
#include <QPen>
#include <QFont>

#include <algorithm>


namespace qmt {

static const qreal TAB_HORIZ_BORDER = 4.0;
static const qreal TAB_VERT_BORDER = 4.0;
static const qreal TAB_MIN_RIGHT_SPACE = 16.0;
static const qreal BODY_VERT_BORDER = 4.0;
static const qreal BODY_HORIZ_BORDER = 4.0;
static const qreal BODY_MIN_HEIGHT = 24.0;
static const qreal MINIMUM_AUTO_WIDTH = 100.0;
static const qreal MINIMUM_AUTO_HEIGHT = 70.0;


struct PackageItem::ShapeGeometry {
    ShapeGeometry(const QSizeF &minimum_size, const QSizeF &minimum_tab_size)
        : m_minimumSize(minimum_size),
          m_minimumTabSize(minimum_tab_size)
    {
    }

    QSizeF m_minimumSize;
    QSizeF m_minimumTabSize;
};


PackageItem::PackageItem(DPackage *package, DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent)
    : ObjectItem(package, diagram_scene_model, parent),
      m_customIcon(0),
      m_shape(0),
      m_packageName(0),
      m_contextLabel(0),
      m_relationStarter(0)
{
}

PackageItem::~PackageItem()
{
}

void PackageItem::update()
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
        if (!m_shape) {
            m_shape = new QGraphicsPolygonItem(this);
        }
        m_shape->setBrush(style->getFillBrush());
        m_shape->setPen(style->getOuterLinePen());
        m_shape->setZValue(SHAPE_ZVALUE);
    } else if (m_shape) {
        m_shape->scene()->removeItem(m_shape);
        delete m_shape;
        m_shape = 0;
    }

    // stereotypes
    updateStereotypes(getStereotypeIconId(), getStereotypeIconDisplay(), style);

    // package name
    if (!m_packageName) {
        m_packageName = new QGraphicsSimpleTextItem(this);
    }
    m_packageName->setBrush(style->getTextBrush());
    m_packageName->setFont(style->getHeaderFont());
    m_packageName->setText(getObject()->getName());

    // context
    if (showContext()) {
        if (!m_contextLabel) {
            m_contextLabel = new ContextLabelItem(this);
        }
        m_contextLabel->setFont(style->getSmallFont());
        m_contextLabel->setBrush(style->getTextBrush());
        m_contextLabel->setContext(getObject()->getContext());
    } else if (m_contextLabel) {
        m_contextLabel->scene()->removeItem(m_contextLabel);
        delete m_contextLabel;
        m_contextLabel = 0;
    }

    updateSelectionMarker(m_customIcon);

    // relation starters
    if (isFocusSelected()) {
        if (!m_relationStarter) {
            m_relationStarter = new RelationStarter(this, getDiagramSceneModel(), 0);
            scene()->addItem(m_relationStarter);
            m_relationStarter->setZValue(RELATION_STARTER_ZVALUE);
            m_relationStarter->addArrow(QStringLiteral("dependency"), ArrowItem::SHAFT_DASHED, ArrowItem::HEAD_OPEN);
        }
    } else if (m_relationStarter) {
        scene()->removeItem(m_relationStarter);
        delete m_relationStarter;
        m_relationStarter = 0;
    }

    updateAlignmentButtons();

    updateGeometry();
}

bool PackageItem::intersectShapeWithLine(const QLineF &line, QPointF *intersection_point, QLineF *intersection_line) const
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
        ShapeGeometry shape = calcMinimumGeometry();
        polygon << rect.topLeft() << (rect.topLeft() + QPointF(shape.m_minimumTabSize.width(), 0.0))
                << (rect.topLeft() + QPointF(shape.m_minimumTabSize.width(), shape.m_minimumTabSize.height()))
                << rect.topRight() + QPointF(0.0, shape.m_minimumTabSize.height())
                << rect.bottomRight() << rect.bottomLeft() << rect.topLeft();
    }
    return GeometryUtilities::intersect(polygon, line, intersection_point, intersection_line);
}

QSizeF PackageItem::getMinimumSize() const
{
    ShapeGeometry geometry = calcMinimumGeometry();
    return geometry.m_minimumSize;
}

QList<ILatchable::Latch> PackageItem::getHorizontalLatches(ILatchable::Action action, bool grabbed_item) const
{
    return ObjectItem::getHorizontalLatches(action, grabbed_item);
}

QList<ILatchable::Latch> PackageItem::getVerticalLatches(ILatchable::Action action, bool grabbed_item) const
{
    return ObjectItem::getVerticalLatches(action, grabbed_item);
}

#if 0
QList<qreal> PackageItem::getHorizontalLatches() const
{
    QRectF rect = getObject()->getRect();
    rect.translate(getObject()->getPos());
    return QList<qreal>() << rect.left() << rect.center().x() << rect.right();
}

QList<qreal> PackageItem::getVerticalLatches() const
{
    QRectF rect = getObject()->getRect();
    rect.translate(getObject()->getPos());
    ShapeGeometry shape = calcMinimumGeometry();
    return QList<qreal>() << rect.topLeft().y() << (rect.topLeft() + QPointF(0.0, shape.m_minimumTabSize.height())).y() << rect.center().y() << rect.bottomRight().y();
}
#endif

QPointF PackageItem::getRelationStartPos() const
{
    return pos();
}

void PackageItem::relationDrawn(const QString &id, const QPointF &to_scene_pos, const QList<QPointF> &intermediate_points)
{
    DElement *target_element = getDiagramSceneModel()->findTopmostElement(to_scene_pos);
    if (target_element) {
        if (id == QStringLiteral("dependency")) {
            DObject *dependant_object = dynamic_cast<DObject *>(target_element);
            if (dependant_object) {
                getDiagramSceneModel()->getDiagramSceneController()->createDependency(getObject(), dependant_object, intermediate_points, getDiagramSceneModel()->getDiagram());
            }
        }
    }
}

PackageItem::ShapeGeometry PackageItem::calcMinimumGeometry() const
{
    double width = 0.0;
    double height = 0.0;
    double tab_height = 0.0;
    double tab_width = 0.0;

    if (m_customIcon) {
        return ShapeGeometry(getStereotypeIconMinimumSize(m_customIcon->getStereotypeIcon(), CUSTOM_ICON_MINIMUM_AUTO_WIDTH, CUSTOM_ICON_MINIMUM_AUTO_HEIGHT), QSizeF(tab_width, tab_height));
    }

    double body_height = 0.0;
    double body_width = 0.0;

    tab_height += TAB_VERT_BORDER;
    if (StereotypesItem *stereotypes_item = getStereotypesItem()) {
        tab_width = std::max(tab_width, stereotypes_item->boundingRect().width() + 2 * TAB_HORIZ_BORDER);
        tab_height += stereotypes_item->boundingRect().height();
    }
    if (m_packageName) {
        tab_width = std::max(tab_width, m_packageName->boundingRect().width() + 2 * TAB_HORIZ_BORDER);
        tab_height += m_packageName->boundingRect().height();
    }
    tab_height += TAB_VERT_BORDER;
    width = std::max(width, tab_width + TAB_MIN_RIGHT_SPACE);
    height += tab_height;

    body_height = BODY_VERT_BORDER;
    if (CustomIconItem *stereotype_icon_item = getStereotypeIconItem()) {
        body_width = std::max(body_width, stereotype_icon_item->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        body_height += stereotype_icon_item->boundingRect().height();
    }
    if (m_contextLabel) {
        body_height += m_contextLabel->getHeight();
    }
    body_height += BODY_VERT_BORDER;
    body_height = std::max(body_height, BODY_MIN_HEIGHT);
    width = std::max(width, body_width);
    height += body_height;

    return ShapeGeometry(GeometryUtilities::ensureMinimumRasterSize(QSizeF(width, height), 2 * RASTER_WIDTH, 2 * RASTER_HEIGHT), QSizeF(tab_width, tab_height));
}

void PackageItem::updateGeometry()
{
    prepareGeometryChange();

    ShapeGeometry geometry = calcMinimumGeometry();

    double width = geometry.m_minimumSize.width();
    double height = geometry.m_minimumSize.height();
    double tab_width = geometry.m_minimumTabSize.width();
    double tab_height = geometry.m_minimumTabSize.height();

    // calc width and height
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
    double right = width / 2.0;
    double top = -height / 2.0;
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

        y += BODY_VERT_BORDER;
        if (StereotypesItem *stereotypes_item = getStereotypesItem()) {
            stereotypes_item->setPos(-stereotypes_item->boundingRect().width() / 2.0, y);
            y += stereotypes_item->boundingRect().height();
        }
        if (m_packageName) {
            m_packageName->setPos(-m_packageName->boundingRect().width() / 2.0, y);
            y += m_packageName->boundingRect().height();
        }
        if (m_contextLabel) {
            m_contextLabel->resetMaxWidth();
            m_contextLabel->setPos(-m_contextLabel->boundingRect().width() / 2.0, y);
            y += m_contextLabel->boundingRect().height();
        }
    } else if (m_shape) {
        QPolygonF polygon;
        polygon << rect.topLeft()
                << QPointF(left + tab_width, top)
                << QPointF(left + tab_width, top + tab_height)
                << QPointF(right, top + tab_height)
                << rect.bottomRight()
                << rect.bottomLeft();
        m_shape->setPolygon(polygon);

        y += TAB_VERT_BORDER;
        if (StereotypesItem *stereotypes_item = getStereotypesItem()) {
            stereotypes_item->setPos(left + TAB_HORIZ_BORDER, y);
            y += stereotypes_item->boundingRect().height();
        }
        if (m_packageName) {
            m_packageName->setPos(left + TAB_HORIZ_BORDER, y);
            y += m_packageName->boundingRect().height();
        }
        y += TAB_VERT_BORDER;
        y += BODY_VERT_BORDER;
        if (CustomIconItem *stereotype_icon_item = getStereotypeIconItem()) {
            stereotype_icon_item->setPos(right - stereotype_icon_item->boundingRect().width() - BODY_HORIZ_BORDER, y);
            y += stereotype_icon_item->boundingRect().height();
        }
        if (m_contextLabel) {
            m_contextLabel->setMaxWidth(width - 2 * BODY_HORIZ_BORDER);
            m_contextLabel->setPos(-m_contextLabel->boundingRect().width() / 2.0, y);
            y += m_contextLabel->boundingRect().height();
        }
    }

    updateSelectionMarkerGeometry(rect);

    if (m_relationStarter) {
        m_relationStarter->setPos(mapToScene(QPointF(right + 8.0, top)));
    }

    updateAlignmentButtonsGeometry(rect);

    updateDepth();
}

}
