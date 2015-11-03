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

#include "annotationitem.h"

#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram/dannotation.h"
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

static const qreal MINIMUM_TEXT_WIDTH = 20.0;
static const qreal CONTENTS_BORDER_VERTICAL = 4.0;
static const qreal CONTENTS_BORDER_HORIZONTAL = 4.0;


class AnnotationItem::AnnotationTextItem :
        public QGraphicsTextItem
{
public:
    AnnotationTextItem(QGraphicsItem *parent)
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


AnnotationItem::AnnotationItem(DAnnotation *annotation, DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_annotation(annotation),
      m_diagramSceneModel(diagram_scene_model),
      m_secondarySelected(false),
      m_focusSelected(false),
      m_selectionMarker(0),
      m_noTextItem(0),
      m_textItem(0),
      m_onUpdate(false),
      m_onChanged(false)
{
    setFlags(QGraphicsItem::ItemIsSelectable);
}

AnnotationItem::~AnnotationItem()
{
}

QRectF AnnotationItem::boundingRect() const
{
    return childrenBoundingRect();
}

void AnnotationItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void AnnotationItem::update()
{
    QMT_CHECK(!m_onUpdate);
    m_onUpdate = true;

    prepareGeometryChange();

    const Style *style = getAdaptedStyle();

    // text
    if (!m_textItem) {
        m_textItem = new AnnotationTextItem(this);
        m_textItem->setTextInteractionFlags(Qt::TextEditorInteraction);
        m_textItem->installSceneEventFilter(this);
        QObject::connect(m_textItem->document(), &QTextDocument::contentsChanged, m_textItem,
                         [=]() { this->onContentsChanged(); } );
    }
    m_textItem->setFont(style->getNormalFont());
    m_textItem->setDefaultTextColor(style->getTextBrush().color());
    if (!m_onChanged) {
        m_textItem->setPlainText(getAnnotation()->getText());
    }

    // item shown if annotation has no text and is not selected
    if (!m_noTextItem) {
        m_noTextItem = new QGraphicsRectItem(this);
    }
    m_noTextItem->setPen(QPen(QBrush(QColor(192, 192, 192)), 1, Qt::DashDotLine));
    m_noTextItem->setVisible(!isSelected() && m_textItem->document()->isEmpty());

    updateSelectionMarker();

    updateGeometry();

    setZValue(ANNOTATION_ITEMS_ZVALUE);

    m_onUpdate = false;
}

QPointF AnnotationItem::getPos() const
{
    return m_annotation->getPos();
}

QRectF AnnotationItem::getRect() const
{
    return m_annotation->getRect();
}

QSizeF AnnotationItem::getMinimumSize() const
{
    return calcMinimumGeometry();
}

void AnnotationItem::setPosAndRect(const QPointF &original_pos, const QRectF &original_rect, const QPointF &top_left_delta, const QPointF &bottom_right_delta)
{
    QPointF new_pos = original_pos;
    QRectF new_rect = original_rect;
    GeometryUtilities::adjustPosAndRect(&new_pos, &new_rect, top_left_delta, bottom_right_delta, QPointF(0.0, 0.0));
    if (new_pos != m_annotation->getPos() || new_rect != m_annotation->getRect()) {
        m_diagramSceneModel->getDiagramController()->startUpdateElement(m_annotation, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_GEOMETRY);
        m_annotation->setPos(new_pos);
        if (new_rect.size() != m_annotation->getRect().size()) {
            m_annotation->setAutoSize(false);
        }
        m_annotation->setRect(new_rect);
        m_diagramSceneModel->getDiagramController()->finishUpdateElement(m_annotation, m_diagramSceneModel->getDiagram(), false);
    }
}

void AnnotationItem::alignItemSizeToRaster(Side adjust_horizontal_side, Side adjust_vertical_side, double raster_width, double raster_height)
{
    Q_UNUSED(adjust_horizontal_side);
    Q_UNUSED(adjust_vertical_side);
    Q_UNUSED(raster_width);
    Q_UNUSED(raster_height);
}

void AnnotationItem::moveDelta(const QPointF &delta)
{
    m_diagramSceneModel->getDiagramController()->startUpdateElement(m_annotation, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_GEOMETRY);
    m_annotation->setPos(m_annotation->getPos() + delta);
    m_diagramSceneModel->getDiagramController()->finishUpdateElement(m_annotation, m_diagramSceneModel->getDiagram(), false);
}

void AnnotationItem::alignItemPositionToRaster(double raster_width, double raster_height)
{
    QPointF pos = m_annotation->getPos();
    QRectF rect = m_annotation->getRect();
    QPointF top_left = pos + rect.topLeft();

    double left_delta = qRound(top_left.x() / raster_width) * raster_width - top_left.x();
    double top_delta = qRound(top_left.y() / raster_height) * raster_height - top_left.y();
    QPointF top_left_delta(left_delta, top_delta);

    setPosAndRect(pos, rect, top_left_delta, top_left_delta);
}

bool AnnotationItem::isSecondarySelected() const
{
    return m_secondarySelected;
}

void AnnotationItem::setSecondarySelected(bool secondary_selected)
{
    if (m_secondarySelected != secondary_selected) {
        m_secondarySelected = secondary_selected;
        update();
    }
}

bool AnnotationItem::isFocusSelected() const
{
    return m_focusSelected;
}

void AnnotationItem::setFocusSelected(bool focus_selected)
{
    if (m_focusSelected != focus_selected) {
        m_focusSelected = focus_selected;
        update();
    }
}

bool AnnotationItem::isEditable() const
{
    return true;
}

void AnnotationItem::edit()
{
    if (m_textItem) {
        m_textItem->setFocus();
    }
}

void AnnotationItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        m_diagramSceneModel->selectItem(this, event->modifiers() & Qt::ControlModifier);
    }
    if (event->buttons() & Qt::LeftButton) {
        m_diagramSceneModel->moveSelectedItems(this, QPointF(0.0, 0.0));
    }
}

void AnnotationItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        m_diagramSceneModel->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
    }
}

void AnnotationItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_diagramSceneModel->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
        if (event->scenePos() != event->buttonDownScenePos(Qt::LeftButton)) {
            m_diagramSceneModel->alignSelectedItemsPositionOnRaster();
        }
    }
}

void AnnotationItem::updateSelectionMarker()
{
    if (isSelected() || m_secondarySelected) {
        if (!m_selectionMarker) {
            m_selectionMarker = new RectangularSelectionItem(this, this);
            m_selectionMarker->setShowBorder(true);
            m_selectionMarker->setFreedom(RectangularSelectionItem::FREEDOM_HORIZONTAL_ONLY);
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

void AnnotationItem::updateSelectionMarkerGeometry(const QRectF &annotation_rect)
{
    if (m_selectionMarker) {
        m_selectionMarker->setRect(annotation_rect);
    }
}

const Style *AnnotationItem::getAdaptedStyle()
{
    return m_diagramSceneModel->getStyleController()->adaptAnnotationStyle(m_annotation);
}

bool AnnotationItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    if (watched == m_textItem) {
        if (event->type() == QEvent::FocusIn) {
            scene()->clearSelection();
            setSelected(true);
        }
    }
    return false;
}

void AnnotationItem::onContentsChanged()
{
    QMT_CHECK(!m_onChanged);
    m_onChanged = true;

    if (!m_onUpdate) {
        QString plain_text = m_textItem->toPlainText();
        if (m_annotation->getText() != plain_text) {
            m_diagramSceneModel->getDiagramController()->startUpdateElement(m_annotation, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_MINOR);
            m_annotation->setText(plain_text);
            m_diagramSceneModel->getDiagramController()->finishUpdateElement(m_annotation, m_diagramSceneModel->getDiagram(), false);
        }
    }

    m_onChanged = false;
}

QSizeF AnnotationItem::calcMinimumGeometry() const
{
    qreal width = MINIMUM_TEXT_WIDTH + 2 * CONTENTS_BORDER_HORIZONTAL;
    qreal height = 0.0; // irrelevant; cannot be modified by user and will always be overwritten

    if (getAnnotation()->hasAutoSize()) {
        if (m_textItem) {
            m_textItem->setTextWidth(-1);
            QSizeF text_size = m_textItem->document()->size();
            width = text_size.width() + 2 * CONTENTS_BORDER_HORIZONTAL;
        }
    }
    return QSizeF(width, height);
}

void AnnotationItem::updateGeometry()
{
    prepareGeometryChange();

    QSizeF geometry = calcMinimumGeometry();
    qreal width = geometry.width();
    qreal height = geometry.height();

    if (getAnnotation()->hasAutoSize()) {
        if (m_textItem) {
            m_textItem->setTextWidth(-1);
            QSizeF text_size = m_textItem->document()->size();
            width = text_size.width() + 2 * CONTENTS_BORDER_HORIZONTAL;
            height = text_size.height() + 2 * CONTENTS_BORDER_VERTICAL;
        }
    } else {
        QRectF rect = getAnnotation()->getRect();
        width = rect.width();
        if (m_textItem) {
            m_textItem->setTextWidth(width - 2 * CONTENTS_BORDER_HORIZONTAL);
            height = m_textItem->document()->size().height() + 2 * CONTENTS_BORDER_VERTICAL;
        }
    }

    // update sizes and positions
    double left = 0.0;
    double top = 0.0;

    setPos(getAnnotation()->getPos());

    QRectF rect(left, top, width, height);

    // the object is updated without calling DiagramController intentionally.
    // attribute rect is not a real attribute stored on DObject but
    // a backup for the graphics item used for manual resized and persistency.
    getAnnotation()->setRect(rect);

    if (m_noTextItem) {
        m_noTextItem->setRect(rect);
    }

    if (m_textItem) {
        m_textItem->setPos(left + CONTENTS_BORDER_HORIZONTAL, top + CONTENTS_BORDER_VERTICAL);
    }

    updateSelectionMarkerGeometry(rect);
}

}
