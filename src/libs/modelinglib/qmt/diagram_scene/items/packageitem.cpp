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

#include "packageitem.h"

#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram/dpackage.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/parts/contextlabelitem.h"
#include "qmt/diagram_scene/parts/customiconitem.h"
#include "qmt/diagram_scene/parts/editabletextitem.h"
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

class PackageItem::ShapeGeometry
{
public:
    ShapeGeometry(const QSizeF &minimumSize, const QSizeF &minimumTabSize)
        : m_minimumSize(minimumSize),
          m_minimumTabSize(minimumTabSize)
    {
    }

    QSizeF m_minimumSize;
    QSizeF m_minimumTabSize;
};

PackageItem::PackageItem(DPackage *package, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent)
    : ObjectItem(package, diagramSceneModel, parent)
{
}

PackageItem::~PackageItem()
{
}

void PackageItem::update()
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
    if (!m_customIcon) {
        if (!m_shape)
            m_shape = new QGraphicsPolygonItem(this);
        m_shape->setBrush(style->fillBrush());
        m_shape->setPen(style->outerLinePen());
        m_shape->setZValue(SHAPE_ZVALUE);
    } else if (m_shape) {
        m_shape->scene()->removeItem(m_shape);
        delete m_shape;
        m_shape = 0;
    }

    // stereotypes
    updateStereotypes(stereotypeIconId(), stereotypeIconDisplay(), style);

    // package name
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
        if (!m_relationStarter) {
            m_relationStarter = new RelationStarter(this, diagramSceneModel(), 0);
            scene()->addItem(m_relationStarter);
            m_relationStarter->setZValue(RELATION_STARTER_ZVALUE);
            m_relationStarter->addArrow(QStringLiteral("dependency"), ArrowItem::ShaftDashed, ArrowItem::HeadOpen);
        }
    } else if (m_relationStarter) {
        scene()->removeItem(m_relationStarter);
        delete m_relationStarter;
        m_relationStarter = 0;
    }

    updateAlignmentButtons();
    updateGeometry();
}

bool PackageItem::intersectShapeWithLine(const QLineF &line, QPointF *intersectionPoint, QLineF *intersectionLine) const
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
        ShapeGeometry shape = calcMinimumGeometry();
        polygon << rect.topLeft() << (rect.topLeft() + QPointF(shape.m_minimumTabSize.width(), 0.0))
                << (rect.topLeft() + QPointF(shape.m_minimumTabSize.width(), shape.m_minimumTabSize.height()))
                << rect.topRight() + QPointF(0.0, shape.m_minimumTabSize.height())
                << rect.bottomRight() << rect.bottomLeft() << rect.topLeft();
    }
    return GeometryUtilities::intersect(polygon, line, intersectionPoint, intersectionLine);
}

QSizeF PackageItem::minimumSize() const
{
    ShapeGeometry geometry = calcMinimumGeometry();
    return geometry.m_minimumSize;
}

QList<ILatchable::Latch> PackageItem::horizontalLatches(ILatchable::Action action, bool grabbedItem) const
{
    return ObjectItem::horizontalLatches(action, grabbedItem);
}

QList<ILatchable::Latch> PackageItem::verticalLatches(ILatchable::Action action, bool grabbedItem) const
{
    return ObjectItem::verticalLatches(action, grabbedItem);
}

QPointF PackageItem::relationStartPos() const
{
    return pos();
}

void PackageItem::relationDrawn(const QString &id, const QPointF &toScenePos, const QList<QPointF> &intermediatePoints)
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

PackageItem::ShapeGeometry PackageItem::calcMinimumGeometry() const
{
    double width = 0.0;
    double height = 0.0;
    double tabHeight = 0.0;
    double tabWidth = 0.0;

    if (m_customIcon) {
        return ShapeGeometry(stereotypeIconMinimumSize(m_customIcon->stereotypeIcon(), CUSTOM_ICON_MINIMUM_AUTO_WIDTH,
                                                       CUSTOM_ICON_MINIMUM_AUTO_HEIGHT), QSizeF(tabWidth, tabHeight));
    }
    double bodyHeight = 0.0;
    double bodyWidth = 0.0;

    tabHeight += TAB_VERT_BORDER;
    if (StereotypesItem *stereotypesItem = this->stereotypesItem()) {
        tabWidth = std::max(tabWidth, stereotypesItem->boundingRect().width() + 2 * TAB_HORIZ_BORDER);
        tabHeight += stereotypesItem->boundingRect().height();
    }
    if (nameItem()) {
        tabWidth = std::max(tabWidth, nameItem()->boundingRect().width() + 2 * TAB_HORIZ_BORDER);
        tabHeight += nameItem()->boundingRect().height();
    }
    tabHeight += TAB_VERT_BORDER;
    width = std::max(width, tabWidth + TAB_MIN_RIGHT_SPACE);
    height += tabHeight;

    bodyHeight = BODY_VERT_BORDER;
    if (CustomIconItem *stereotypeIconItem = this->stereotypeIconItem()) {
        bodyWidth = std::max(bodyWidth, stereotypeIconItem->boundingRect().width() + 2 * BODY_HORIZ_BORDER);
        bodyHeight += stereotypeIconItem->boundingRect().height();
    }
    if (m_contextLabel)
        bodyHeight += m_contextLabel->height();
    bodyHeight += BODY_VERT_BORDER;
    bodyHeight = std::max(bodyHeight, BODY_MIN_HEIGHT);
    width = std::max(width, bodyWidth);
    height += bodyHeight;

    return ShapeGeometry(GeometryUtilities::ensureMinimumRasterSize(QSizeF(width, height), 2 * RASTER_WIDTH, 2 * RASTER_HEIGHT), QSizeF(tabWidth, tabHeight));
}

void PackageItem::updateGeometry()
{
    prepareGeometryChange();

    ShapeGeometry geometry = calcMinimumGeometry();

    double width = geometry.m_minimumSize.width();
    double height = geometry.m_minimumSize.height();
    double tabWidth = geometry.m_minimumTabSize.width();
    double tabHeight = geometry.m_minimumTabSize.height();

    // calc width and height
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

        y += BODY_VERT_BORDER;
        if (StereotypesItem *stereotypesItem = this->stereotypesItem()) {
            stereotypesItem->setPos(-stereotypesItem->boundingRect().width() / 2.0, y);
            y += stereotypesItem->boundingRect().height();
        }
        if (nameItem()) {
            nameItem()->setPos(-nameItem()->boundingRect().width() / 2.0, y);
            y += nameItem()->boundingRect().height();
        }
        if (m_contextLabel) {
            m_contextLabel->resetMaxWidth();
            m_contextLabel->setPos(-m_contextLabel->boundingRect().width() / 2.0, y);
            y += m_contextLabel->boundingRect().height();
        }
    } else if (m_shape) {
        QPolygonF polygon;
        polygon << rect.topLeft()
                << QPointF(left + tabWidth, top)
                << QPointF(left + tabWidth, top + tabHeight)
                << QPointF(right, top + tabHeight)
                << rect.bottomRight()
                << rect.bottomLeft();
        m_shape->setPolygon(polygon);

        y += TAB_VERT_BORDER;
        if (StereotypesItem *stereotypesItem = this->stereotypesItem()) {
            stereotypesItem->setPos(left + TAB_HORIZ_BORDER, y);
            y += stereotypesItem->boundingRect().height();
        }
        if (nameItem()) {
            nameItem()->setPos(left + TAB_HORIZ_BORDER, y);
            y += nameItem()->boundingRect().height();
        }
        y += TAB_VERT_BORDER;
        y += BODY_VERT_BORDER;
        if (CustomIconItem *stereotypeIconItem = this->stereotypeIconItem()) {
            stereotypeIconItem->setPos(right - stereotypeIconItem->boundingRect().width() - BODY_HORIZ_BORDER, y);
            y += stereotypeIconItem->boundingRect().height();
        }
        if (m_contextLabel) {
            m_contextLabel->setMaxWidth(width - 2 * BODY_HORIZ_BORDER);
            m_contextLabel->setPos(-m_contextLabel->boundingRect().width() / 2.0, y);
            y += m_contextLabel->boundingRect().height();
        }
    }

    updateSelectionMarkerGeometry(rect);

    if (m_relationStarter)
        m_relationStarter->setPos(mapToScene(QPointF(right + 8.0, top)));

    updateAlignmentButtonsGeometry(rect);
    updateDepth();
}

} // namespace qmt
