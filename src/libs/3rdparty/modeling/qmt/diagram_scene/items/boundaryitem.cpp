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

#include "boundaryitem.h"

#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram/dboundary.h"
#include "qmt/diagram_scene/capabilities/moveable.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/parts/rectangularselectionitem.h"
#include "qmt/infrastructure/geometryutilities.h"
#include "qmt/infrastructure/qmtassert.h"
#include "qmt/style/stylecontroller.h"
#include "qmt/style/style.h"

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QTextDocument>
#include <QTextFrame>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QCoreApplication>


namespace qmt {

static const qreal MINIMUM_INNER_WIDTH = 22.0;
static const qreal MINIMUM_INNER_HEIGHT = 22.0;
static const qreal CONTENTS_BORDER_VERTICAL = 4.0;
static const qreal CONTENTS_BORDER_HORIZONTAL = 4.0;


class BoundaryItem::BoundaryTextItem :
        public QGraphicsTextItem
{
public:
    BoundaryTextItem(QGraphicsItem *parent)
        : QGraphicsTextItem(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        QMT_CHECK(option);

        QStyleOptionGraphicsItem option2(*option);
        option2.state &= ~(QStyle::State_Selected | QStyle::State_HasFocus);
        QGraphicsTextItem::paint(painter, &option2, widget);
    }
};


BoundaryItem::BoundaryItem(DBoundary *boundary, DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_boundary(boundary),
      m_diagramSceneModel(diagram_scene_model),
      m_secondarySelected(false),
      m_focusSelected(false),
      m_selectionMarker(0),
      m_borderItem(0),
      m_noTextItem(0),
      m_textItem(0),
      m_onUpdate(false),
      m_onChanged(false)
{
    setFlags(QGraphicsItem::ItemIsSelectable);
}

BoundaryItem::~BoundaryItem()
{
}

QRectF BoundaryItem::boundingRect() const
{
    return childrenBoundingRect();
}

void BoundaryItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void BoundaryItem::update()
{
    QMT_CHECK(!m_onUpdate);
    m_onUpdate = true;

    prepareGeometryChange();

    const Style *style = getAdaptedStyle();

    // text
    if (!m_textItem) {
        m_textItem = new BoundaryTextItem(this);
        m_textItem->setTextInteractionFlags(Qt::TextEditorInteraction);
        m_textItem->installSceneEventFilter(this);
        QObject::connect(m_textItem->document(), &QTextDocument::contentsChanged, m_textItem,
                         [=]() { this->onContentsChanged(); } );
    }
    m_textItem->setFont(style->getNormalFont());
    m_textItem->setDefaultTextColor(style->getTextBrush().color());
    if (!m_onChanged) {
        m_textItem->setTextWidth(-1);
        m_textItem->setPlainText(m_boundary->getText());
    }

    // item shown if annotation has no text and is not selected
    if (m_textItem->document()->isEmpty() && isSelected()) {
        if (!m_noTextItem) {
            m_noTextItem = new QGraphicsRectItem(this);
        }
        m_noTextItem->setPen(QPen(QBrush(QColor(192, 192, 192)), 1, Qt::DashDotLine));
    } else if (m_noTextItem) {
        m_noTextItem->scene()->removeItem(m_noTextItem);
        delete m_noTextItem;
        m_noTextItem = 0;
    }

    // item shown if annotation has no text and is not selected
    if (!m_borderItem) {
        m_borderItem = new QGraphicsRectItem(this);
    }
    m_borderItem->setPen(QPen(QBrush(Qt::black), 1, Qt::DashLine));

    updateSelectionMarker();

    updateGeometry();

    setZValue(BOUNDARY_ITEMS_ZVALUE);

    m_onUpdate = false;
}

QPointF BoundaryItem::getPos() const
{
    return m_boundary->getPos();
}

QRectF BoundaryItem::getRect() const
{
    return m_boundary->getRect();
}

QSizeF BoundaryItem::getMinimumSize() const
{
    return calcMinimumGeometry();
}

void BoundaryItem::setPosAndRect(const QPointF &original_pos, const QRectF &original_rect, const QPointF &top_left_delta, const QPointF &bottom_right_delta)
{
    QPointF new_pos = original_pos;
    QRectF new_rect = original_rect;
    GeometryUtilities::adjustPosAndRect(&new_pos, &new_rect, top_left_delta, bottom_right_delta, QPointF(0.5, 0.5));
    if (new_pos != m_boundary->getPos() || new_rect != m_boundary->getRect()) {
        m_diagramSceneModel->getDiagramController()->startUpdateElement(m_boundary, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_GEOMETRY);
        m_boundary->setPos(new_pos);
        m_boundary->setRect(new_rect);
        m_diagramSceneModel->getDiagramController()->finishUpdateElement(m_boundary, m_diagramSceneModel->getDiagram(), false);
    }
}

void BoundaryItem::alignItemSizeToRaster(IResizable::Side adjust_horizontal_side, IResizable::Side adjust_vertical_side, double raster_width, double raster_height)
{
    QPointF pos = m_boundary->getPos();
    QRectF rect = m_boundary->getRect();

    double horiz_delta = rect.width() - qRound(rect.width() / raster_width) * raster_width;
    double vert_delta = rect.height() - qRound(rect.height() / raster_height) * raster_height;

    // make sure the new size is at least the minimum size
    QSizeF minimum_size = getMinimumSize();
    while (rect.width() + horiz_delta < minimum_size.width()) {
        horiz_delta += raster_width;
    }
    while (rect.height() + vert_delta < minimum_size.height()) {
        vert_delta += raster_height;
    }

    double left_delta = 0.0;
    double right_delta = 0.0;
    double top_delta = 0.0;
    double bottom_delta = 0.0;

    switch (adjust_horizontal_side) {
    case IResizable::SIDE_NONE:
        break;
    case IResizable::SIDE_LEFT_OR_TOP:
        left_delta = horiz_delta;
        break;
    case IResizable::SIDE_RIGHT_OR_BOTTOM:
        right_delta = -horiz_delta;
        break;
    }

    switch (adjust_vertical_side) {
    case IResizable::SIDE_NONE:
        break;
    case IResizable::SIDE_LEFT_OR_TOP:
        top_delta = vert_delta;
        break;
    case IResizable::SIDE_RIGHT_OR_BOTTOM:
        bottom_delta = -vert_delta;
        break;
    }

    QPointF top_left_delta(left_delta, top_delta);
    QPointF bottom_right_delta(right_delta, bottom_delta);
    setPosAndRect(pos, rect, top_left_delta, bottom_right_delta);
}

void BoundaryItem::moveDelta(const QPointF &delta)
{
    m_diagramSceneModel->getDiagramController()->startUpdateElement(m_boundary, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_GEOMETRY);
    m_boundary->setPos(m_boundary->getPos() + delta);
    m_diagramSceneModel->getDiagramController()->finishUpdateElement(m_boundary, m_diagramSceneModel->getDiagram(), false);
}

void BoundaryItem::alignItemPositionToRaster(double raster_width, double raster_height)
{
    QPointF pos = m_boundary->getPos();
    QRectF rect = m_boundary->getRect();
    QPointF top_left = pos + rect.topLeft();

    double left_delta = qRound(top_left.x() / raster_width) * raster_width - top_left.x();
    double top_delta = qRound(top_left.y() / raster_height) * raster_height - top_left.y();
    QPointF top_left_delta(left_delta, top_delta);

    setPosAndRect(pos, rect, top_left_delta, top_left_delta);
}

bool BoundaryItem::isSecondarySelected() const
{
    return m_secondarySelected;
}

void BoundaryItem::setSecondarySelected(bool secondary_selected)
{
    if (m_secondarySelected != secondary_selected) {
        m_secondarySelected = secondary_selected;
        update();
    }
}

bool BoundaryItem::isFocusSelected() const
{
    return m_focusSelected;
}

void BoundaryItem::setFocusSelected(bool focus_selected)
{
    if (m_focusSelected != focus_selected) {
        m_focusSelected = focus_selected;
        update();
    }
}

bool BoundaryItem::isEditable() const
{
    return true;
}

void BoundaryItem::edit()
{
    if (m_textItem) {
        m_textItem->setFocus();
    }
}

void BoundaryItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        m_diagramSceneModel->selectItem(this, event->modifiers() & Qt::ControlModifier);
    }
    if (event->buttons() & Qt::LeftButton) {
        m_diagramSceneModel->moveSelectedItems(this, QPointF(0.0, 0.0));
    }
}

void BoundaryItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        m_diagramSceneModel->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
    }
}

void BoundaryItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_diagramSceneModel->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
        if (event->scenePos() != event->buttonDownScenePos(Qt::LeftButton)) {
            m_diagramSceneModel->alignSelectedItemsPositionOnRaster();
        }
    }
}

void BoundaryItem::updateSelectionMarker()
{
    if (isSelected() || m_secondarySelected) {
        if (!m_selectionMarker) {
            m_selectionMarker = new RectangularSelectionItem(this, this);
        }
        m_selectionMarker->setSecondarySelected(isSelected() ? false : m_secondarySelected);
    } else if (m_selectionMarker) {
        if (m_selectionMarker->scene()) {
            m_selectionMarker->scene()->removeItem(m_selectionMarker);
        }
        delete m_selectionMarker;
        m_selectionMarker = 0;
    }
}

void BoundaryItem::updateSelectionMarkerGeometry(const QRectF &boundary_rect)
{
    if (m_selectionMarker) {
        m_selectionMarker->setRect(boundary_rect);
    }
}

const Style *BoundaryItem::getAdaptedStyle()
{
    return m_diagramSceneModel->getStyleController()->adaptBoundaryStyle(m_boundary);
}

bool BoundaryItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    if (watched == m_textItem) {
        if (event->type() == QEvent::FocusIn) {
            scene()->clearSelection();
            setSelected(true);
        }
    }
    return false;
}

void BoundaryItem::onContentsChanged()
{
    QMT_CHECK(!m_onChanged);
    m_onChanged = true;

    if (!m_onUpdate) {
        QString plain_text = m_textItem->toPlainText();
        if (m_boundary->getText() != plain_text) {
            m_diagramSceneModel->getDiagramController()->startUpdateElement(m_boundary, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_MINOR);
            m_boundary->setText(plain_text);
            m_diagramSceneModel->getDiagramController()->finishUpdateElement(m_boundary, m_diagramSceneModel->getDiagram(), false);
        }
    }

    m_onChanged = false;
}

QSizeF BoundaryItem::calcMinimumGeometry() const
{
    qreal width = MINIMUM_INNER_WIDTH + 2 * CONTENTS_BORDER_HORIZONTAL;
    qreal height = MINIMUM_INNER_HEIGHT + 2 * CONTENTS_BORDER_VERTICAL;

    if (m_textItem) {
        m_textItem->setTextWidth(-1);
        QSizeF text_size = m_textItem->document()->size();
        qreal text_width = text_size.width() + 2 * CONTENTS_BORDER_HORIZONTAL;
        if (text_width > width) {
            width = text_width;
        }
        qreal text_height = text_size.height() + 2 * CONTENTS_BORDER_VERTICAL;
        if (text_height > height) {
            height = text_height;
        }
    }
    return GeometryUtilities::ensureMinimumRasterSize(QSizeF(width, height), 2 * RASTER_WIDTH, 2 * RASTER_HEIGHT);
}

void BoundaryItem::updateGeometry()
{
    prepareGeometryChange();

    QSizeF geometry = calcMinimumGeometry();
    qreal width = geometry.width();
    qreal height = geometry.height();

    qreal text_width = 0.0;
    qreal text_height = 0.0;
    if (m_textItem) {
        m_textItem->setTextWidth(-1);
        QSizeF text_size = m_textItem->document()->size();
        text_width = text_size.width();
        text_height = text_size.height();
    }

    QRectF boundary_rect = m_boundary->getRect();
    if (boundary_rect.width() > width) {
        width = boundary_rect.width();
    }
    if (boundary_rect.height() > height) {
        height = boundary_rect.height();
    }

    // update sizes and positions
    double left = -width / 2.0;
    double top = -height / 2.0;

    setPos(m_boundary->getPos());

    QRectF rect(left, top, width, height);

    // the object is updated without calling DiagramController intentionally.
    // attribute rect is not a real attribute stored on DObject but
    // a backup for the graphics item used for manual resized and persistency.
    m_boundary->setRect(rect);

    if (m_borderItem) {
        m_borderItem->setRect(rect);
    }

    if (m_noTextItem) {
        m_noTextItem->setRect(QRectF(-text_width / 2, top + CONTENTS_BORDER_VERTICAL, text_width, text_height));
    }

    if (m_textItem) {
        m_textItem->setPos(-text_width / 2.0, top + CONTENTS_BORDER_VERTICAL);
    }

    updateSelectionMarkerGeometry(rect);
}

}
