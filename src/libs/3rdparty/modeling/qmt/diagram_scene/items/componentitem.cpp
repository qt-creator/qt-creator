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

#include "componentitem.h"

#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram/dcomponent.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/parts/contextlabelitem.h"
#include "qmt/diagram_scene/parts/customiconitem.h"
#include "qmt/diagram_scene/parts/relationstarter.h"
#include "qmt/diagram_scene/parts/stereotypesitem.h"
#include "qmt/infrastructure/geometryutilities.h"
#include "qmt/infrastructure/qmtassert.h"
#include "qmt/stereotype/stereotypecontroller.h"
#include "qmt/stereotype/stereotypeicon.h"
#include "qmt/style/style.h"
#include "qmt/tasks/diagramscenecontroller.h"

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QBrush>
#include <QPen>
#include <QFont>

#include <algorithm>


namespace qmt {

static const qreal RECT_HEIGHT = 15.0;
static const qreal RECT_WIDTH = 45.0;
static const qreal UPPER_RECT_Y = 10.0;
static const qreal RECT_Y_DISTANCE = 10.0;
static const qreal LOWER_RECT_MIN_Y = 10.0;
static const qreal BODY_VERT_BORDER = 4.0;
static const qreal BODY_HORIZ_BORDER = 4.0;


ComponentItem::ComponentItem(DComponent *component, DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent)
    : ObjectItem(component, diagram_scene_model, parent),
      _custom_icon(0),
      _shape(0),
      _upper_rect(0),
      _lower_rect(0),
      _component_name(0),
      _context_label(0),
      _relation_starter(0)
{
}

ComponentItem::~ComponentItem()
{
}

void ComponentItem::update()
{
    prepareGeometryChange();

    updateStereotypeIconDisplay();

    const Style *style = getAdaptedStyle(getStereotypeIconId());

    // custom icon
    if (getStereotypeIconDisplay() == StereotypeIcon::DISPLAY_ICON) {
        if (!_custom_icon) {
            _custom_icon = new CustomIconItem(getDiagramSceneModel(), this);
        }
        _custom_icon->setStereotypeIconId(getStereotypeIconId());
        _custom_icon->setBaseSize(getStereotypeIconMinimumSize(_custom_icon->getStereotypeIcon(), CUSTOM_ICON_MINIMUM_AUTO_WIDTH, CUSTOM_ICON_MINIMUM_AUTO_HEIGHT));
        _custom_icon->setBrush(style->getFillBrush());
        _custom_icon->setPen(style->getOuterLinePen());
        _custom_icon->setZValue(SHAPE_ZVALUE);
    } else if (_custom_icon) {
        _custom_icon->scene()->removeItem(_custom_icon);
        delete _custom_icon;
        _custom_icon = 0;
    }

    // shape
    bool delete_rects = false;
    if (!_custom_icon) {
        if (!_shape) {
            _shape = new QGraphicsRectItem(this);
        }
        _shape->setBrush(style->getFillBrush());
        _shape->setPen(style->getOuterLinePen());
        _shape->setZValue(SHAPE_ZVALUE);
        if (!hasPlainShape()) {
            if (!_upper_rect) {
                _upper_rect = new QGraphicsRectItem(this);
            }
            _upper_rect->setBrush(style->getFillBrush());
            _upper_rect->setPen(style->getOuterLinePen());
            _upper_rect->setZValue(SHAPE_DETAILS_ZVALUE);
            if (!_lower_rect) {
                _lower_rect = new QGraphicsRectItem(this);
            }
            _lower_rect->setBrush(style->getFillBrush());
            _lower_rect->setPen(style->getOuterLinePen());
            _lower_rect->setZValue(SHAPE_DETAILS_ZVALUE);
        } else {
            delete_rects = true;
        }
    } else {
        delete_rects = true;
        if (_shape) {
            _shape->scene()->removeItem(_shape);
            delete _shape;
            _shape = 0;
        }
    }
    if (delete_rects) {
        if (_lower_rect) {
            _lower_rect->scene()->removeItem(_lower_rect);
            delete _lower_rect;
            _lower_rect = 0;
        }
        if (_upper_rect) {
            _upper_rect->scene()->removeItem(_upper_rect);
            delete _upper_rect;
            _upper_rect = 0;
        }
    }

    // stereotypes
    updateStereotypes(getStereotypeIconId(), getStereotypeIconDisplay(), style);

    // component name
    if (!_component_name) {
        _component_name = new QGraphicsSimpleTextItem(this);
    }
    _component_name->setFont(style->getHeaderFont());
    _component_name->setBrush(style->getTextBrush());
    _component_name->setText(getObject()->getName());

    // context
    if (showContext()) {
        if (!_context_label) {
            _context_label = new ContextLabelItem(this);
        }
        _context_label->setFont(style->getSmallFont());
        _context_label->setBrush(style->getTextBrush());
        _context_label->setContext(getObject()->getContext());
    } else if (_context_label) {
        _context_label->scene()->removeItem(_context_label);
        delete _context_label;
        _context_label = 0;
    }

    updateSelectionMarker(_custom_icon);

    // relation starters
    if (isFocusSelected()) {
        if (!_relation_starter && scene()) {
            _relation_starter = new RelationStarter(this, getDiagramSceneModel(), 0);
            scene()->addItem(_relation_starter);
            _relation_starter->setZValue(RELATION_STARTER_ZVALUE);
            _relation_starter->addArrow(QStringLiteral("dependency"), ArrowItem::SHAFT_DASHED, ArrowItem::HEAD_OPEN);
        }
    } else if (_relation_starter) {
        if (_relation_starter->scene()) {
            _relation_starter->scene()->removeItem(_relation_starter);
        }
        delete _relation_starter;
        _relation_starter = 0;
    }

    updateAlignmentButtons();

    updateGeometry();
}

bool ComponentItem::intersectShapeWithLine(const QLineF &line, QPointF *intersection_point, QLineF *intersection_line) const
{
    QPolygonF polygon;
    if (_custom_icon) {
        // TODO use custom_icon path as shape
        QRectF rect = getObject()->getRect();
        rect.translate(getObject()->getPos());
        polygon << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft() << rect.topLeft();
    } else if (hasPlainShape()){
        QRectF rect = getObject()->getRect();
        rect.translate(getObject()->getPos());
        polygon << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft() << rect.topLeft();
    } else {
        QRectF rect = getObject()->getRect();
        rect.translate(getObject()->getPos());
        polygon << rect.topLeft()
                << rect.topRight()
                << rect.bottomRight()
                << rect.bottomLeft()
                << rect.bottomLeft() + QPointF(0, UPPER_RECT_Y + RECT_HEIGHT + RECT_Y_DISTANCE + RECT_HEIGHT)
                << rect.bottomLeft() + QPointF(-RECT_WIDTH * 0.5, UPPER_RECT_Y + RECT_HEIGHT + RECT_Y_DISTANCE + RECT_HEIGHT)
                << rect.bottomLeft() + QPointF(-RECT_WIDTH * 0.5, UPPER_RECT_Y)
                << rect.bottomLeft() + QPointF(0, UPPER_RECT_Y)
                << rect.topLeft();
    }
    return GeometryUtilities::intersect(polygon, line, intersection_point, intersection_line);
}

QSizeF ComponentItem::getMinimumSize() const
{
    return calcMinimumGeometry();
}

QList<ILatchable::Latch> ComponentItem::getHorizontalLatches(ILatchable::Action action, bool grabbed_item) const
{
    return ObjectItem::getHorizontalLatches(action, grabbed_item);
}

QList<ILatchable::Latch> ComponentItem::getVerticalLatches(ILatchable::Action action, bool grabbed_item) const
{
    return ObjectItem::getVerticalLatches(action, grabbed_item);
}

#if 0
QList<qreal> ComponentItem::getHorizontalLatches() const
{
    QRectF rect = getObject()->getRect();
    rect.translate(getObject()->getPos());
    return QList<qreal>() << (rect.left() - RECT_WIDTH * 0.5) << rect.left() << rect.right();
}

QList<qreal> ComponentItem::getVerticalLatches() const
{
    QRectF rect = getObject()->getRect();
    rect.translate(getObject()->getPos());
    return QList<qreal>() << rect.top() << rect.bottom();
}
#endif

QPointF ComponentItem::getRelationStartPos() const
{
    return pos();
}

void ComponentItem::relationDrawn(const QString &id, const QPointF &to_scene_pos, const QList<QPointF> &intermediate_points)
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

bool ComponentItem::hasPlainShape() const
{
    DComponent *diagram_component = dynamic_cast<DComponent *>(getObject());
    QMT_CHECK(diagram_component);
    return diagram_component->getPlainShape();
}

QSizeF ComponentItem::calcMinimumGeometry() const
{
    double width = 0.0;
    double height = 0.0;

    if (_custom_icon) {
        return getStereotypeIconMinimumSize(_custom_icon->getStereotypeIcon(), CUSTOM_ICON_MINIMUM_AUTO_WIDTH, CUSTOM_ICON_MINIMUM_AUTO_HEIGHT);
    }

    height += BODY_VERT_BORDER;
    if (CustomIconItem *stereotype_icon_item = getStereotypeIconItem()) {
        width = std::max(width, stereotype_icon_item->boundingRect().width());
        height += stereotype_icon_item->boundingRect().height();
    }
    if (StereotypesItem *stereotypes_item = getStereotypesItem()) {
        width = std::max(width, stereotypes_item->boundingRect().width());
        height += stereotypes_item->boundingRect().height();
    }
    if (_component_name) {
        width = std::max(width, _component_name->boundingRect().width());
        height += _component_name->boundingRect().height();
    }
    if (_context_label) {
        height += _context_label->getHeight();
    }
    height += BODY_VERT_BORDER;

    if (!hasPlainShape()) {
        width = RECT_WIDTH * 0.5 + BODY_HORIZ_BORDER + width + BODY_HORIZ_BORDER + RECT_WIDTH * 0.5;
        double min_height = UPPER_RECT_Y + RECT_HEIGHT + RECT_Y_DISTANCE + RECT_HEIGHT + LOWER_RECT_MIN_Y;
        if (height < min_height) {
            height = min_height;
        }
    } else {
        width = BODY_HORIZ_BORDER + width + BODY_HORIZ_BORDER;
    }

    return GeometryUtilities::ensureMinimumRasterSize(QSizeF(width, height), 2 * RASTER_WIDTH, 2 * RASTER_HEIGHT);
}

void ComponentItem::updateGeometry()
{
    prepareGeometryChange();

    // calc width and height
    double width = 0.0;
    double height = 0.0;

    QSizeF geometry = calcMinimumGeometry();
    width = geometry.width();
    height = geometry.height();

    if (getObject()->hasAutoSize()) {
        // nothing
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
    //double bottom = height / 2.0;
    double y = top;

    setPos(getObject()->getPos());

    QRectF rect(left, top, width, height);

    // the object is updated without calling DiagramController intentionally.
    // attribute rect is not a real attribute stored on DObject but
    // a backup for the graphics item used for manual resized and persistency.
    getObject()->setRect(rect);

    if (_custom_icon) {
        _custom_icon->setPos(left, top);
        _custom_icon->setActualSize(QSizeF(width, height));
        y += height;
    }

    if (_shape) {
        _shape->setRect(rect);
    }

    if (_upper_rect) {
        QRectF upper_rect(0, 0, RECT_WIDTH, RECT_HEIGHT);
        _upper_rect->setRect(upper_rect);
        _upper_rect->setPos(left - RECT_WIDTH * 0.5, top + UPPER_RECT_Y);
    }

    if (_lower_rect) {
        QRectF lower_rect(0, 0, RECT_WIDTH, RECT_HEIGHT);
        _lower_rect->setRect(lower_rect);
        _lower_rect->setPos(left - RECT_WIDTH * 0.5, top + UPPER_RECT_Y + RECT_HEIGHT + RECT_Y_DISTANCE);
    }

    y += BODY_VERT_BORDER;
    if (CustomIconItem *stereotype_icon_item = getStereotypeIconItem()) {
        stereotype_icon_item->setPos(right - stereotype_icon_item->boundingRect().width() - BODY_HORIZ_BORDER, y);
        y += stereotype_icon_item->boundingRect().height();
    }
    if (StereotypesItem *stereotypes_item = getStereotypesItem()) {
        stereotypes_item->setPos(-stereotypes_item->boundingRect().width() / 2.0, y);
        y += stereotypes_item->boundingRect().height();
    }
    if (_component_name) {
        _component_name->setPos(-_component_name->boundingRect().width() / 2.0, y);
        y += _component_name->boundingRect().height();
    }
    if (_context_label) {
        if (_custom_icon) {
            _context_label->resetMaxWidth();
        } else {
            double max_context_width = width - 2 * BODY_HORIZ_BORDER - (hasPlainShape() ? 0 : RECT_WIDTH);
            _context_label->setMaxWidth(max_context_width);
        }
        _context_label->setPos(-_context_label->boundingRect().width() / 2.0, y);
        y += _context_label->boundingRect().height();
    }

    updateSelectionMarkerGeometry(rect);

    if (_relation_starter) {
        _relation_starter->setPos(mapToScene(QPointF(right + 8.0, top)));
    }

    updateAlignmentButtonsGeometry(rect);

    updateDepth();
}

}
