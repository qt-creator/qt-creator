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


ComponentItem::ComponentItem(DComponent *component, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent)
    : ObjectItem(component, diagramSceneModel, parent),
      m_customIcon(0),
      m_shape(0),
      m_upperRect(0),
      m_lowerRect(0),
      m_componentName(0),
      m_contextLabel(0),
      m_relationStarter(0)
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
    bool deleteRects = false;
    if (!m_customIcon) {
        if (!m_shape) {
            m_shape = new QGraphicsRectItem(this);
        }
        m_shape->setBrush(style->getFillBrush());
        m_shape->setPen(style->getOuterLinePen());
        m_shape->setZValue(SHAPE_ZVALUE);
        if (!hasPlainShape()) {
            if (!m_upperRect) {
                m_upperRect = new QGraphicsRectItem(this);
            }
            m_upperRect->setBrush(style->getFillBrush());
            m_upperRect->setPen(style->getOuterLinePen());
            m_upperRect->setZValue(SHAPE_DETAILS_ZVALUE);
            if (!m_lowerRect) {
                m_lowerRect = new QGraphicsRectItem(this);
            }
            m_lowerRect->setBrush(style->getFillBrush());
            m_lowerRect->setPen(style->getOuterLinePen());
            m_lowerRect->setZValue(SHAPE_DETAILS_ZVALUE);
        } else {
            deleteRects = true;
        }
    } else {
        deleteRects = true;
        if (m_shape) {
            m_shape->scene()->removeItem(m_shape);
            delete m_shape;
            m_shape = 0;
        }
    }
    if (deleteRects) {
        if (m_lowerRect) {
            m_lowerRect->scene()->removeItem(m_lowerRect);
            delete m_lowerRect;
            m_lowerRect = 0;
        }
        if (m_upperRect) {
            m_upperRect->scene()->removeItem(m_upperRect);
            delete m_upperRect;
            m_upperRect = 0;
        }
    }

    // stereotypes
    updateStereotypes(getStereotypeIconId(), getStereotypeIconDisplay(), style);

    // component name
    if (!m_componentName) {
        m_componentName = new QGraphicsSimpleTextItem(this);
    }
    m_componentName->setFont(style->getHeaderFont());
    m_componentName->setBrush(style->getTextBrush());
    m_componentName->setText(getObject()->getName());

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
        if (!m_relationStarter && scene()) {
            m_relationStarter = new RelationStarter(this, getDiagramSceneModel(), 0);
            scene()->addItem(m_relationStarter);
            m_relationStarter->setZValue(RELATION_STARTER_ZVALUE);
            m_relationStarter->addArrow(QStringLiteral("dependency"), ArrowItem::SHAFT_DASHED, ArrowItem::HEAD_OPEN);
        }
    } else if (m_relationStarter) {
        if (m_relationStarter->scene()) {
            m_relationStarter->scene()->removeItem(m_relationStarter);
        }
        delete m_relationStarter;
        m_relationStarter = 0;
    }

    updateAlignmentButtons();

    updateGeometry();
}

bool ComponentItem::intersectShapeWithLine(const QLineF &line, QPointF *intersectionPoint, QLineF *intersectionLine) const
{
    QPolygonF polygon;
    if (m_customIcon) {
        // TODO use customIcon path as shape
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
    return GeometryUtilities::intersect(polygon, line, intersectionPoint, intersectionLine);
}

QSizeF ComponentItem::getMinimumSize() const
{
    return calcMinimumGeometry();
}

QList<ILatchable::Latch> ComponentItem::getHorizontalLatches(ILatchable::Action action, bool grabbedItem) const
{
    return ObjectItem::getHorizontalLatches(action, grabbedItem);
}

QList<ILatchable::Latch> ComponentItem::getVerticalLatches(ILatchable::Action action, bool grabbedItem) const
{
    return ObjectItem::getVerticalLatches(action, grabbedItem);
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

void ComponentItem::relationDrawn(const QString &id, const QPointF &toScenePos, const QList<QPointF> &intermediatePoints)
{
    DElement *targetElement = getDiagramSceneModel()->findTopmostElement(toScenePos);
    if (targetElement) {
       if (id == QStringLiteral("dependency")) {
            DObject *dependantObject = dynamic_cast<DObject *>(targetElement);
            if (dependantObject) {
                getDiagramSceneModel()->getDiagramSceneController()->createDependency(getObject(), dependantObject, intermediatePoints, getDiagramSceneModel()->getDiagram());
            }
        }
    }
}

bool ComponentItem::hasPlainShape() const
{
    DComponent *diagramComponent = dynamic_cast<DComponent *>(getObject());
    QMT_CHECK(diagramComponent);
    return diagramComponent->getPlainShape();
}

QSizeF ComponentItem::calcMinimumGeometry() const
{
    double width = 0.0;
    double height = 0.0;

    if (m_customIcon) {
        return getStereotypeIconMinimumSize(m_customIcon->getStereotypeIcon(), CUSTOM_ICON_MINIMUM_AUTO_WIDTH, CUSTOM_ICON_MINIMUM_AUTO_HEIGHT);
    }

    height += BODY_VERT_BORDER;
    if (CustomIconItem *stereotypeIconItem = getStereotypeIconItem()) {
        width = std::max(width, stereotypeIconItem->boundingRect().width());
        height += stereotypeIconItem->boundingRect().height();
    }
    if (StereotypesItem *stereotypesItem = getStereotypesItem()) {
        width = std::max(width, stereotypesItem->boundingRect().width());
        height += stereotypesItem->boundingRect().height();
    }
    if (m_componentName) {
        width = std::max(width, m_componentName->boundingRect().width());
        height += m_componentName->boundingRect().height();
    }
    if (m_contextLabel) {
        height += m_contextLabel->getHeight();
    }
    height += BODY_VERT_BORDER;

    if (!hasPlainShape()) {
        width = RECT_WIDTH * 0.5 + BODY_HORIZ_BORDER + width + BODY_HORIZ_BORDER + RECT_WIDTH * 0.5;
        double minHeight = UPPER_RECT_Y + RECT_HEIGHT + RECT_Y_DISTANCE + RECT_HEIGHT + LOWER_RECT_MIN_Y;
        if (height < minHeight) {
            height = minHeight;
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

    if (m_customIcon) {
        m_customIcon->setPos(left, top);
        m_customIcon->setActualSize(QSizeF(width, height));
        y += height;
    }

    if (m_shape) {
        m_shape->setRect(rect);
    }

    if (m_upperRect) {
        QRectF upperRect(0, 0, RECT_WIDTH, RECT_HEIGHT);
        m_upperRect->setRect(upperRect);
        m_upperRect->setPos(left - RECT_WIDTH * 0.5, top + UPPER_RECT_Y);
    }

    if (m_lowerRect) {
        QRectF lowerRect(0, 0, RECT_WIDTH, RECT_HEIGHT);
        m_lowerRect->setRect(lowerRect);
        m_lowerRect->setPos(left - RECT_WIDTH * 0.5, top + UPPER_RECT_Y + RECT_HEIGHT + RECT_Y_DISTANCE);
    }

    y += BODY_VERT_BORDER;
    if (CustomIconItem *stereotypeIconItem = getStereotypeIconItem()) {
        stereotypeIconItem->setPos(right - stereotypeIconItem->boundingRect().width() - BODY_HORIZ_BORDER, y);
        y += stereotypeIconItem->boundingRect().height();
    }
    if (StereotypesItem *stereotypesItem = getStereotypesItem()) {
        stereotypesItem->setPos(-stereotypesItem->boundingRect().width() / 2.0, y);
        y += stereotypesItem->boundingRect().height();
    }
    if (m_componentName) {
        m_componentName->setPos(-m_componentName->boundingRect().width() / 2.0, y);
        y += m_componentName->boundingRect().height();
    }
    if (m_contextLabel) {
        if (m_customIcon) {
            m_contextLabel->resetMaxWidth();
        } else {
            double maxContextWidth = width - 2 * BODY_HORIZ_BORDER - (hasPlainShape() ? 0 : RECT_WIDTH);
            m_contextLabel->setMaxWidth(maxContextWidth);
        }
        m_contextLabel->setPos(-m_contextLabel->boundingRect().width() / 2.0, y);
        y += m_contextLabel->boundingRect().height();
    }

    updateSelectionMarkerGeometry(rect);

    if (m_relationStarter) {
        m_relationStarter->setPos(mapToScene(QPointF(right + 8.0, top)));
    }

    updateAlignmentButtonsGeometry(rect);

    updateDepth();
}

}
