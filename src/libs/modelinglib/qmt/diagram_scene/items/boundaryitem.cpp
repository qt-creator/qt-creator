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

class BoundaryItem::BoundaryTextItem : public QGraphicsTextItem
{
public:
    explicit BoundaryTextItem(QGraphicsItem *parent)
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

BoundaryItem::BoundaryItem(DBoundary *boundary, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_boundary(boundary),
      m_diagramSceneModel(diagramSceneModel)
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
    QMT_CHECK(!m_isUpdating);
    m_isUpdating = true;

    prepareGeometryChange();

    const Style *style = adaptedStyle();

    // text
    if (!m_textItem) {
        m_textItem = new BoundaryTextItem(this);
        m_textItem->setTextInteractionFlags(Qt::TextEditorInteraction);
        m_textItem->installSceneEventFilter(this);
        QObject::connect(m_textItem->document(), &QTextDocument::contentsChanged, m_textItem,
                         [=]() { this->onContentsChanged(); } );
    }
    m_textItem->setFont(style->normalFont());
    m_textItem->setDefaultTextColor(style->textBrush().color());
    if (!m_isChanged) {
        m_textItem->setTextWidth(-1);
        m_textItem->setPlainText(m_boundary->text());
    }

    // item shown if annotation has no text and is not selected
    if (m_textItem->document()->isEmpty() && isSelected()) {
        if (!m_noTextItem)
            m_noTextItem = new QGraphicsRectItem(this);
        m_noTextItem->setPen(QPen(QBrush(QColor(192, 192, 192)), 1, Qt::DashDotLine));
    } else if (m_noTextItem) {
        m_noTextItem->scene()->removeItem(m_noTextItem);
        delete m_noTextItem;
        m_noTextItem = 0;
    }

    // item shown if annotation has no text and is not selected
    if (!m_borderItem)
        m_borderItem = new QGraphicsRectItem(this);
    m_borderItem->setPen(QPen(QBrush(Qt::black), 1, Qt::DashLine));

    updateSelectionMarker();
    updateGeometry();
    setZValue(BOUNDARY_ITEMS_ZVALUE);

    m_isUpdating = false;
}

QPointF BoundaryItem::pos() const
{
    return m_boundary->pos();
}

QRectF BoundaryItem::rect() const
{
    return m_boundary->rect();
}

QSizeF BoundaryItem::minimumSize() const
{
    return calcMinimumGeometry();
}

void BoundaryItem::setPosAndRect(const QPointF &originalPos, const QRectF &originalRect, const QPointF &topLeftDelta,
                                 const QPointF &bottomRightDelta)
{
    QPointF newPos = originalPos;
    QRectF newRect = originalRect;
    GeometryUtilities::adjustPosAndRect(&newPos, &newRect, topLeftDelta, bottomRightDelta, QPointF(0.5, 0.5));
    if (newPos != m_boundary->pos() || newRect != m_boundary->rect()) {
        m_diagramSceneModel->diagramController()->startUpdateElement(m_boundary, m_diagramSceneModel->diagram(), DiagramController::UpdateGeometry);
        m_boundary->setPos(newPos);
        m_boundary->setRect(newRect);
        m_diagramSceneModel->diagramController()->finishUpdateElement(m_boundary, m_diagramSceneModel->diagram(), false);
    }
}

void BoundaryItem::alignItemSizeToRaster(IResizable::Side adjustHorizontalSide, IResizable::Side adjustVerticalSide,
                                         double rasterWidth, double rasterHeight)
{
    QPointF pos = m_boundary->pos();
    QRectF rect = m_boundary->rect();

    double horizDelta = rect.width() - qRound(rect.width() / rasterWidth) * rasterWidth;
    double vertDelta = rect.height() - qRound(rect.height() / rasterHeight) * rasterHeight;

    // make sure the new size is at least the minimum size
    QSizeF minimumSize = this->minimumSize();
    while (rect.width() + horizDelta < minimumSize.width())
        horizDelta += rasterWidth;
    while (rect.height() + vertDelta < minimumSize.height())
        vertDelta += rasterHeight;

    double leftDelta = 0.0;
    double rightDelta = 0.0;
    double topDelta = 0.0;
    double bottomDelta = 0.0;

    switch (adjustHorizontalSide) {
    case IResizable::SideNone:
        break;
    case IResizable::SideLeftOrTop:
        leftDelta = horizDelta;
        break;
    case IResizable::SideRightOrBottom:
        rightDelta = -horizDelta;
        break;
    }

    switch (adjustVerticalSide) {
    case IResizable::SideNone:
        break;
    case IResizable::SideLeftOrTop:
        topDelta = vertDelta;
        break;
    case IResizable::SideRightOrBottom:
        bottomDelta = -vertDelta;
        break;
    }

    QPointF topLeftDelta(leftDelta, topDelta);
    QPointF bottomRightDelta(rightDelta, bottomDelta);
    setPosAndRect(pos, rect, topLeftDelta, bottomRightDelta);
}

void BoundaryItem::moveDelta(const QPointF &delta)
{
    m_diagramSceneModel->diagramController()->startUpdateElement(m_boundary, m_diagramSceneModel->diagram(), DiagramController::UpdateGeometry);
    m_boundary->setPos(m_boundary->pos() + delta);
    m_diagramSceneModel->diagramController()->finishUpdateElement(m_boundary, m_diagramSceneModel->diagram(), false);
}

void BoundaryItem::alignItemPositionToRaster(double rasterWidth, double rasterHeight)
{
    QPointF pos = m_boundary->pos();
    QRectF rect = m_boundary->rect();
    QPointF topLeft = pos + rect.topLeft();

    double leftDelta = qRound(topLeft.x() / rasterWidth) * rasterWidth - topLeft.x();
    double topDelta = qRound(topLeft.y() / rasterHeight) * rasterHeight - topLeft.y();
    QPointF topLeftDelta(leftDelta, topDelta);

    setPosAndRect(pos, rect, topLeftDelta, topLeftDelta);
}

bool BoundaryItem::isSecondarySelected() const
{
    return m_isSecondarySelected;
}

void BoundaryItem::setSecondarySelected(bool secondarySelected)
{
    if (m_isSecondarySelected != secondarySelected) {
        m_isSecondarySelected = secondarySelected;
        update();
    }
}

bool BoundaryItem::isFocusSelected() const
{
    return m_isFocusSelected;
}

void BoundaryItem::setFocusSelected(bool focusSelected)
{
    if (m_isFocusSelected != focusSelected) {
        m_isFocusSelected = focusSelected;
        update();
    }
}

bool BoundaryItem::isEditable() const
{
    return true;
}

void BoundaryItem::edit()
{
    if (m_textItem)
        m_textItem->setFocus();
}

void BoundaryItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
        m_diagramSceneModel->selectItem(this, event->modifiers() & Qt::ControlModifier);
    if (event->buttons() & Qt::LeftButton)
        m_diagramSceneModel->moveSelectedItems(this, QPointF(0.0, 0.0));
}

void BoundaryItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
        m_diagramSceneModel->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
}

void BoundaryItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_diagramSceneModel->moveSelectedItems(this, event->scenePos() - event->lastScenePos());
        if (event->scenePos() != event->buttonDownScenePos(Qt::LeftButton))
            m_diagramSceneModel->alignSelectedItemsPositionOnRaster();
    }
}

void BoundaryItem::updateSelectionMarker()
{
    if (isSelected() || m_isSecondarySelected) {
        if (!m_selectionMarker)
            m_selectionMarker = new RectangularSelectionItem(this, this);
        m_selectionMarker->setSecondarySelected(isSelected() ? false : m_isSecondarySelected);
    } else if (m_selectionMarker) {
        if (m_selectionMarker->scene())
            m_selectionMarker->scene()->removeItem(m_selectionMarker);
        delete m_selectionMarker;
        m_selectionMarker = 0;
    }
}

void BoundaryItem::updateSelectionMarkerGeometry(const QRectF &boundaryRect)
{
    if (m_selectionMarker)
        m_selectionMarker->setRect(boundaryRect);
}

const Style *BoundaryItem::adaptedStyle()
{
    return m_diagramSceneModel->styleController()->adaptBoundaryStyle(m_boundary);
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
    QMT_CHECK(!m_isChanged);
    m_isChanged = true;

    if (!m_isUpdating) {
        QString plainText = m_textItem->toPlainText();
        if (m_boundary->text() != plainText) {
            m_diagramSceneModel->diagramController()->startUpdateElement(m_boundary, m_diagramSceneModel->diagram(), DiagramController::UpdateMinor);
            m_boundary->setText(plainText);
            m_diagramSceneModel->diagramController()->finishUpdateElement(m_boundary, m_diagramSceneModel->diagram(), false);
        }
    }

    m_isChanged = false;
}

QSizeF BoundaryItem::calcMinimumGeometry() const
{
    qreal width = MINIMUM_INNER_WIDTH + 2 * CONTENTS_BORDER_HORIZONTAL;
    qreal height = MINIMUM_INNER_HEIGHT + 2 * CONTENTS_BORDER_VERTICAL;

    if (m_textItem) {
        m_textItem->setTextWidth(-1);
        QSizeF textSize = m_textItem->document()->size();
        qreal textWidth = textSize.width() + 2 * CONTENTS_BORDER_HORIZONTAL;
        if (textWidth > width)
            width = textWidth;
        qreal textHeight = textSize.height() + 2 * CONTENTS_BORDER_VERTICAL;
        if (textHeight > height)
            height = textHeight;
    }
    return GeometryUtilities::ensureMinimumRasterSize(QSizeF(width, height), 2 * RASTER_WIDTH, 2 * RASTER_HEIGHT);
}

void BoundaryItem::updateGeometry()
{
    prepareGeometryChange();

    QSizeF geometry = calcMinimumGeometry();
    qreal width = geometry.width();
    qreal height = geometry.height();

    qreal textWidth = 0.0;
    qreal textHeight = 0.0;
    if (m_textItem) {
        m_textItem->setTextWidth(-1);
        QSizeF textSize = m_textItem->document()->size();
        textWidth = textSize.width();
        textHeight = textSize.height();
    }

    QRectF boundaryRect = m_boundary->rect();
    if (boundaryRect.width() > width)
        width = boundaryRect.width();
    if (boundaryRect.height() > height)
        height = boundaryRect.height();

    // update sizes and positions
    double left = -width / 2.0;
    double top = -height / 2.0;

    setPos(m_boundary->pos());

    QRectF rect(left, top, width, height);

    // the object is updated without calling DiagramController intentionally.
    // attribute rect is not a real attribute stored on DObject but
    // a backup for the graphics item used for manual resized and persistency.
    m_boundary->setRect(rect);
    if (m_borderItem)
        m_borderItem->setRect(rect);
    if (m_noTextItem)
        m_noTextItem->setRect(QRectF(-textWidth / 2, top + CONTENTS_BORDER_VERTICAL, textWidth, textHeight));
    if (m_textItem)
        m_textItem->setPos(-textWidth / 2.0, top + CONTENTS_BORDER_VERTICAL);
    updateSelectionMarkerGeometry(rect);
}

} // namespace qmt
