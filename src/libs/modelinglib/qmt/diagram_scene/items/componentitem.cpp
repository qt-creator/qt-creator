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

#include "componentitem.h"

#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram/dcomponent.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/parts/contextlabelitem.h"
#include "qmt/diagram_scene/parts/customiconitem.h"
#include "qmt/diagram_scene/parts/editabletextitem.h"
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
    : ObjectItem(component, diagramSceneModel, parent)
{
}

ComponentItem::~ComponentItem()
{
}

void ComponentItem::update()
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
        m_customIcon = 0;
    }

    // shape
    bool deleteRects = false;
    if (!m_customIcon) {
        if (!m_shape)
            m_shape = new QGraphicsRectItem(this);
        m_shape->setBrush(style->fillBrush());
        m_shape->setPen(style->outerLinePen());
        m_shape->setZValue(SHAPE_ZVALUE);
        if (!hasPlainShape()) {
            if (!m_upperRect)
                m_upperRect = new QGraphicsRectItem(this);
            m_upperRect->setBrush(style->fillBrush());
            m_upperRect->setPen(style->outerLinePen());
            m_upperRect->setZValue(SHAPE_DETAILS_ZVALUE);
            if (!m_lowerRect)
                m_lowerRect = new QGraphicsRectItem(this);
            m_lowerRect->setBrush(style->fillBrush());
            m_lowerRect->setPen(style->outerLinePen());
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
    updateStereotypes(stereotypeIconId(), stereotypeIconDisplay(), style);

    // component name
    updateNameItem(style);

    // context
    if (showContext()) {
        if (!m_contextLabel)
            m_contextLabel = new ContextLabelItem(this);
        m_contextLabel->setFont(style->smallFont());
        m_contextLabel->setBrush(style->textBrush());
        m_contextLabel->setContext(object()->context());
    } else if (m_contextLabel) {
        m_contextLabel->scene()->removeItem(m_contextLabel);
        delete m_contextLabel;
        m_contextLabel = 0;
    }

    updateSelectionMarker(m_customIcon);

    // relation starters
    if (isFocusSelected()) {
        if (!m_relationStarter && scene()) {
            m_relationStarter = new RelationStarter(this, diagramSceneModel(), 0);
            scene()->addItem(m_relationStarter);
            m_relationStarter->setZValue(RELATION_STARTER_ZVALUE);
            m_relationStarter->addArrow(QStringLiteral("dependency"), ArrowItem::ShaftDashed, ArrowItem::HeadOpen);
        }
    } else if (m_relationStarter) {
        if (m_relationStarter->scene())
            m_relationStarter->scene()->removeItem(m_relationStarter);
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
        QRectF rect = object()->rect();
        rect.translate(object()->pos());
        polygon << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft() << rect.topLeft();
    } else if (hasPlainShape()) {
        QRectF rect = object()->rect();
        rect.translate(object()->pos());
        polygon << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft() << rect.topLeft();
    } else {
        QRectF rect = object()->rect();
        rect.translate(object()->pos());
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

QSizeF ComponentItem::minimumSize() const
{
    return calcMinimumGeometry();
}

QList<ILatchable::Latch> ComponentItem::horizontalLatches(ILatchable::Action action, bool grabbedItem) const
{
    return ObjectItem::horizontalLatches(action, grabbedItem);
}

QList<ILatchable::Latch> ComponentItem::verticalLatches(ILatchable::Action action, bool grabbedItem) const
{
    return ObjectItem::verticalLatches(action, grabbedItem);
}

QPointF ComponentItem::relationStartPos() const
{
    return pos();
}

void ComponentItem::relationDrawn(const QString &id, const QPointF &toScenePos, const QList<QPointF> &intermediatePoints)
{
    DElement *targetElement = diagramSceneModel()->findTopmostElement(toScenePos);
    if (targetElement) {
       if (id == QStringLiteral("dependency")) {
            auto dependantObject = dynamic_cast<DObject *>(targetElement);
            if (dependantObject)
                diagramSceneModel()->diagramSceneController()->createDependency(object(), dependantObject, intermediatePoints, diagramSceneModel()->diagram());
        }
    }
}

bool ComponentItem::hasPlainShape() const
{
    auto diagramComponent = dynamic_cast<DComponent *>(object());
    QMT_CHECK(diagramComponent);
    return diagramComponent->isPlainShape();
}

QSizeF ComponentItem::calcMinimumGeometry() const
{
    double width = 0.0;
    double height = 0.0;

    if (m_customIcon) {
        return stereotypeIconMinimumSize(m_customIcon->stereotypeIcon(), CUSTOM_ICON_MINIMUM_AUTO_WIDTH, CUSTOM_ICON_MINIMUM_AUTO_HEIGHT);
    }

    height += BODY_VERT_BORDER;
    if (CustomIconItem *stereotypeIconItem = this->stereotypeIconItem()) {
        width = std::max(width, stereotypeIconItem->boundingRect().width());
        height += stereotypeIconItem->boundingRect().height();
    }
    if (StereotypesItem *stereotypesItem = this->stereotypesItem()) {
        width = std::max(width, stereotypesItem->boundingRect().width());
        height += stereotypesItem->boundingRect().height();
    }
    if (nameItem()) {
        width = std::max(width, nameItem()->boundingRect().width());
        height += nameItem()->boundingRect().height();
    }
    if (m_contextLabel)
        height += m_contextLabel->height();
    height += BODY_VERT_BORDER;

    if (!hasPlainShape()) {
        width = RECT_WIDTH * 0.5 + BODY_HORIZ_BORDER + width + BODY_HORIZ_BORDER + RECT_WIDTH * 0.5;
        double minHeight = UPPER_RECT_Y + RECT_HEIGHT + RECT_Y_DISTANCE + RECT_HEIGHT + LOWER_RECT_MIN_Y;
        if (height < minHeight)
            height = minHeight;
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

    if (object()->isAutoSized()) {
        // nothing
    } else {
        QRectF rect = object()->rect();
        if (rect.width() > width)
            width = rect.width();
        if (rect.height() > height)
            height = rect.height();
    }

    // update sizes and positions
    double left = -width / 2.0;
    double right = width / 2.0;
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

    if (m_shape)
        m_shape->setRect(rect);

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
    if (CustomIconItem *stereotypeIconItem = this->stereotypeIconItem()) {
        stereotypeIconItem->setPos(right - stereotypeIconItem->boundingRect().width() - BODY_HORIZ_BORDER, y);
        y += stereotypeIconItem->boundingRect().height();
    }
    if (StereotypesItem *stereotypesItem = this->stereotypesItem()) {
        stereotypesItem->setPos(-stereotypesItem->boundingRect().width() / 2.0, y);
        y += stereotypesItem->boundingRect().height();
    }
    if (nameItem()) {
        nameItem()->setPos(-nameItem()->boundingRect().width() / 2.0, y);
        y += nameItem()->boundingRect().height();
    }
    if (m_contextLabel) {
        if (m_customIcon) {
            m_contextLabel->resetMaxWidth();
        } else {
            double maxContextWidth = width - 2 * BODY_HORIZ_BORDER - (hasPlainShape() ? 0 : RECT_WIDTH);
            m_contextLabel->setMaxWidth(maxContextWidth);
        }
        m_contextLabel->setPos(-m_contextLabel->boundingRect().width() / 2.0, y);
    }

    updateSelectionMarkerGeometry(rect);

    if (m_relationStarter)
        m_relationStarter->setPos(mapToScene(QPointF(right + 8.0, top)));

    updateAlignmentButtonsGeometry(rect);
    updateDepth();
}

} // namespace qmt
