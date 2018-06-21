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

class AnnotationItem::AnnotationTextItem : public QGraphicsTextItem
{
public:
    explicit AnnotationTextItem(QGraphicsItem *parent)
        : QGraphicsTextItem(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        QMT_ASSERT(option, return);

        QStyleOptionGraphicsItem option2(*option);
        option2.state &= ~(QStyle::State_Selected | QStyle::State_HasFocus);
        QGraphicsTextItem::paint(painter, &option2, widget);
    }
};

AnnotationItem::AnnotationItem(DAnnotation *annotation, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_annotation(annotation),
      m_diagramSceneModel(diagramSceneModel)
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
    QMT_CHECK(!m_isUpdating);
    m_isUpdating = true;

    prepareGeometryChange();

    const Style *style = adaptedStyle();

    // text
    if (!m_textItem) {
        m_textItem = new AnnotationTextItem(this);
        m_textItem->setTextInteractionFlags(Qt::TextEditorInteraction);
        m_textItem->installSceneEventFilter(this);
        QObject::connect(m_textItem->document(), &QTextDocument::contentsChanged, m_textItem,
                         [=]() { this->onContentsChanged(); } );
    }
    m_textItem->setFont(style->normalFont());
    m_textItem->setDefaultTextColor(style->textBrush().color());
    if (!m_isChanged)
        m_textItem->setPlainText(annotation()->text());

    // item shown if annotation has no text and is not selected
    if (!m_noTextItem)
        m_noTextItem = new QGraphicsRectItem(this);
    m_noTextItem->setPen(QPen(QBrush(QColor(192, 192, 192)), 1, Qt::DashDotLine));
    m_noTextItem->setVisible(!isSelected() && m_textItem->document()->isEmpty());

    updateSelectionMarker();
    updateGeometry();
    setZValue(ANNOTATION_ITEMS_ZVALUE);

    m_isUpdating = false;
}

QPointF AnnotationItem::pos() const
{
    return m_annotation->pos();
}

QRectF AnnotationItem::rect() const
{
    return m_annotation->rect();
}

QSizeF AnnotationItem::minimumSize() const
{
    return calcMinimumGeometry();
}

void AnnotationItem::setPosAndRect(const QPointF &originalPos, const QRectF &originalRect,
                                   const QPointF &topLeftDelta, const QPointF &bottomRightDelta)
{
    QPointF newPos = originalPos;
    QRectF newRect = originalRect;
    GeometryUtilities::adjustPosAndRect(&newPos, &newRect, topLeftDelta, bottomRightDelta, QPointF(0.0, 0.0));
    if (newPos != m_annotation->pos() || newRect != m_annotation->rect()) {
        m_diagramSceneModel->diagramController()->startUpdateElement(m_annotation, m_diagramSceneModel->diagram(), DiagramController::UpdateGeometry);
        m_annotation->setPos(newPos);
        if (newRect.size() != m_annotation->rect().size())
            m_annotation->setAutoSized(false);
        m_annotation->setRect(newRect);
        m_diagramSceneModel->diagramController()->finishUpdateElement(m_annotation, m_diagramSceneModel->diagram(), false);
    }
}

void AnnotationItem::alignItemSizeToRaster(Side adjustHorizontalSide, Side adjustVerticalSide,
                                           double rasterWidth, double rasterHeight)
{
    Q_UNUSED(adjustHorizontalSide);
    Q_UNUSED(adjustVerticalSide);
    Q_UNUSED(rasterWidth);
    Q_UNUSED(rasterHeight);
}

void AnnotationItem::moveDelta(const QPointF &delta)
{
    m_diagramSceneModel->diagramController()->startUpdateElement(m_annotation, m_diagramSceneModel->diagram(), DiagramController::UpdateGeometry);
    m_annotation->setPos(m_annotation->pos() + delta);
    m_diagramSceneModel->diagramController()->finishUpdateElement(m_annotation, m_diagramSceneModel->diagram(), false);
}

void AnnotationItem::alignItemPositionToRaster(double rasterWidth, double rasterHeight)
{
    QPointF pos = m_annotation->pos();
    QRectF rect = m_annotation->rect();
    QPointF topLeft = pos + rect.topLeft();

    double leftDelta = qRound(topLeft.x() / rasterWidth) * rasterWidth - topLeft.x();
    double topDelta = qRound(topLeft.y() / rasterHeight) * rasterHeight - topLeft.y();
    QPointF topLeftDelta(leftDelta, topDelta);

    setPosAndRect(pos, rect, topLeftDelta, topLeftDelta);
}

bool AnnotationItem::isSecondarySelected() const
{
    return m_isSecondarySelected;
}

void AnnotationItem::setSecondarySelected(bool secondarySelected)
{
    if (m_isSecondarySelected != secondarySelected) {
        m_isSecondarySelected = secondarySelected;
        update();
    }
}

bool AnnotationItem::isFocusSelected() const
{
    return m_isFocusSelected;
}

void AnnotationItem::setFocusSelected(bool focusSelected)
{
    if (m_isFocusSelected != focusSelected) {
        m_isFocusSelected = focusSelected;
        update();
    }
}

QRectF AnnotationItem::getSecondarySelectionBoundary()
{
    return QRectF();
}

void AnnotationItem::setBoundarySelected(const QRectF &boundary, bool secondary)
{
    if (boundary.contains(mapRectToScene(boundingRect()))) {
        if (secondary)
            setSecondarySelected(true);
        else
            setSelected(true);
    }
}

bool AnnotationItem::isEditable() const
{
    return true;
}

void AnnotationItem::edit()
{
    if (m_textItem)
        m_textItem->setFocus();
}

void AnnotationItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
        m_diagramSceneModel->selectItem(this, event->modifiers() & Qt::ControlModifier);
    if (event->buttons() & Qt::LeftButton)
        m_diagramSceneModel->moveSelectedItems(this, QPointF(0.0, 0.0));
}

void AnnotationItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
        m_diagramSceneModel->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
}

void AnnotationItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_diagramSceneModel->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
        if (event->scenePos() != event->buttonDownScenePos(Qt::LeftButton))
            m_diagramSceneModel->alignSelectedItemsPositionOnRaster();
    }
}

void AnnotationItem::updateSelectionMarker()
{
    if (isSelected() || m_isSecondarySelected) {
        if (!m_selectionMarker) {
            m_selectionMarker = new RectangularSelectionItem(this, this);
            m_selectionMarker->setShowBorder(true);
            m_selectionMarker->setFreedom(RectangularSelectionItem::FreedomHorizontalOnly);
        }
        m_selectionMarker->setSecondarySelected(isSelected() ? false : m_isSecondarySelected);
    } else if (m_selectionMarker) {
        if (m_selectionMarker->scene())
            m_selectionMarker->scene()->removeItem(m_selectionMarker);
        delete m_selectionMarker;
        m_selectionMarker = nullptr;
    }
}

void AnnotationItem::updateSelectionMarkerGeometry(const QRectF &annotationRect)
{
    if (m_selectionMarker)
        m_selectionMarker->setRect(annotationRect);
}

const Style *AnnotationItem::adaptedStyle()
{
    return m_diagramSceneModel->styleController()->adaptAnnotationStyle(m_annotation);
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
    QMT_CHECK(!m_isChanged);
    m_isChanged = true;

    if (!m_isUpdating) {
        QString plainText = m_textItem->toPlainText();
        if (m_annotation->text() != plainText) {
            m_diagramSceneModel->diagramController()->startUpdateElement(m_annotation, m_diagramSceneModel->diagram(), DiagramController::UpdateMinor);
            m_annotation->setText(plainText);
            m_diagramSceneModel->diagramController()->finishUpdateElement(m_annotation, m_diagramSceneModel->diagram(), false);
        }
    }

    m_isChanged = false;
}

QSizeF AnnotationItem::calcMinimumGeometry() const
{
    qreal width = MINIMUM_TEXT_WIDTH + 2 * CONTENTS_BORDER_HORIZONTAL;
    qreal height = 0.0; // irrelevant; cannot be modified by user and will always be overwritten

    if (annotation()->isAutoSized()) {
        if (m_textItem) {
            m_textItem->setTextWidth(-1);
            QSizeF textSize = m_textItem->document()->size();
            width = textSize.width() + 2 * CONTENTS_BORDER_HORIZONTAL;
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

    if (annotation()->isAutoSized()) {
        if (m_textItem) {
            m_textItem->setTextWidth(-1);
            QSizeF textSize = m_textItem->document()->size();
            width = textSize.width() + 2 * CONTENTS_BORDER_HORIZONTAL;
            height = textSize.height() + 2 * CONTENTS_BORDER_VERTICAL;
        }
    } else {
        QRectF rect = annotation()->rect();
        width = rect.width();
        if (m_textItem) {
            m_textItem->setTextWidth(width - 2 * CONTENTS_BORDER_HORIZONTAL);
            height = m_textItem->document()->size().height() + 2 * CONTENTS_BORDER_VERTICAL;
        }
    }

    // update sizes and positions
    double left = 0.0;
    double top = 0.0;

    setPos(annotation()->pos());

    QRectF rect(left, top, width, height);

    // the object is updated without calling DiagramController intentionally.
    // attribute rect is not a real attribute stored on DObject but
    // a backup for the graphics item used for manual resized and persistency.
    annotation()->setRect(rect);

    if (m_noTextItem)
        m_noTextItem->setRect(rect);

    if (m_textItem)
        m_textItem->setPos(left + CONTENTS_BORDER_HORIZONTAL, top + CONTENTS_BORDER_VERTICAL);

    updateSelectionMarkerGeometry(rect);
}

} // namespace qmt
