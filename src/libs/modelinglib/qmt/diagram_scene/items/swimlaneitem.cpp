/****************************************************************************
**
** Copyright (C) 2017 Jochen Becher
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

#include "swimlaneitem.h"

#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram/dswimlane.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/infrastructure/qmtassert.h"
#include "qmt/style/stylecontroller.h"
#include "qmt/style/style.h"

#include <QBrush>
#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPen>

namespace qmt {

static const qreal SWIMLANE_LENGTH = 100000;
static const qreal SWIMLANE_WIDTH = 16;
static const qreal SWIMLANE_MARKER_WIDTH = 8;

SwimlaneItem::SwimlaneItem(DSwimlane *swimlane, DiagramSceneModel *diagramSceneModel,
                           QGraphicsItem *parent)
    : QGraphicsItem (parent),
      m_swimlane(swimlane),
      m_diagramSceneModel(diagramSceneModel)
{
    setFlags(QGraphicsItem::ItemIsSelectable);
}

SwimlaneItem::~SwimlaneItem()
{
}

QRectF SwimlaneItem::boundingRect() const
{
    if (m_swimlane->isHorizontal())
        return QRectF(-SWIMLANE_LENGTH/2, -SWIMLANE_WIDTH/2, SWIMLANE_LENGTH, SWIMLANE_WIDTH);
    else
        return QRectF(-SWIMLANE_WIDTH/2, -SWIMLANE_LENGTH/2, SWIMLANE_WIDTH, SWIMLANE_LENGTH);
}

void SwimlaneItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void SwimlaneItem::update()
{
    QMT_CHECK(!m_isUpdating);
    m_isUpdating = true;

    prepareGeometryChange();

    const Style *style = adaptedStyle();
    Q_UNUSED(style);

    // swimline line
    if (!m_lineItem)
        m_lineItem = new QGraphicsLineItem(this);
    m_lineItem->setPen(QPen(QBrush(Qt::black), 1, Qt::DashLine));

    updateSelectionMarker();
    updateGeometry();
    setZValue(SWIMLANE_ITEMS_ZVALUE);

    m_isUpdating = false;
}

void SwimlaneItem::moveDelta(const QPointF &delta)
{
    m_diagramSceneModel->diagramController()->startUpdateElement(m_swimlane, m_diagramSceneModel->diagram(),
                                                                 DiagramController::UpdateGeometry);
    if (m_swimlane->isHorizontal())
        m_swimlane->setPos(m_swimlane->pos() + delta.y());
    else
        m_swimlane->setPos(m_swimlane->pos() + delta.x());
    m_diagramSceneModel->diagramController()->finishUpdateElement(m_swimlane, m_diagramSceneModel->diagram(), false);
}

void SwimlaneItem::alignItemPositionToRaster(double rasterWidth, double rasterHeight)
{
    if (m_swimlane->isHorizontal()) {
        qreal yDelta = qRound(m_swimlane->pos() / rasterHeight) * rasterHeight - m_swimlane->pos();
        moveDelta(QPointF(0, yDelta));
    } else {
        qreal xDelta = qRound(m_swimlane->pos() / rasterWidth) * rasterWidth - m_swimlane->pos();
        moveDelta(QPointF(xDelta, 0));
    }
}

bool SwimlaneItem::isSecondarySelected() const
{
    return m_secondarySelected;
}

void SwimlaneItem::setSecondarySelected(bool secondarySelected)
{
    if (m_secondarySelected != secondarySelected) {
        m_secondarySelected = secondarySelected;
        update();
    }
}

bool SwimlaneItem::isFocusSelected() const
{
    return false;
}

void SwimlaneItem::setFocusSelected(bool focusSelected)
{
    Q_UNUSED(focusSelected);
}

QRectF SwimlaneItem::getSecondarySelectionBoundary()
{
    QRectF boundary;
    if (m_swimlane->isHorizontal())
        boundary = QRectF(-SWIMLANE_LENGTH/2, pos().y(), SWIMLANE_LENGTH, SWIMLANE_LENGTH);
    else
        boundary = QRectF(pos().x(), -SWIMLANE_LENGTH/2, SWIMLANE_LENGTH, SWIMLANE_LENGTH);
    return boundary;
}

void SwimlaneItem::setBoundarySelected(const QRectF &boundary, bool secondary)
{
    qreal pos = m_swimlane->pos();
    bool c;

    if (m_swimlane->isHorizontal())
        c = pos >= boundary.top() && pos <= boundary.bottom() && boundary.top() > -SWIMLANE_LENGTH/2;
    else
        c = pos >= boundary.left() && pos <= boundary.right() && boundary.left() > -SWIMLANE_LENGTH/2;
    if (c) {
        if (secondary)
            setSecondarySelected(true);
        else
            setSelected(true);
    }
}

void SwimlaneItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        bool multiSelect = (event->modifiers() & Qt::ControlModifier) != 0;
        m_diagramSceneModel->selectItem(this, multiSelect);
    }
}

void SwimlaneItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPointF delta = event->scenePos() - event->lastScenePos();
        if (m_swimlane->isHorizontal())
            delta.setX(0.0);
        else
            delta.setY(0.0);
        m_diagramSceneModel->moveSelectedItems(this, delta);
    }
}

void SwimlaneItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (event->scenePos() != event->buttonDownScenePos(Qt::LeftButton))
            m_diagramSceneModel->alignSelectedItemsPositionOnRaster();
    }
}

void SwimlaneItem::updateSelectionMarker()
{
    if (isSelected() || m_secondarySelected) {
        if (!m_selectionMarker)
            m_selectionMarker = new QGraphicsRectItem(this);
        m_selectionMarker->setBrush(isSelected() ? Qt::lightGray : Qt::transparent);
        m_selectionMarker->setPen(isSelected() ? Qt::NoPen : QPen(Qt::lightGray));
        m_selectionMarker->setZValue(-1);
    } else if (m_selectionMarker) {
        if (m_selectionMarker->scene())
            m_selectionMarker->scene()->removeItem(m_selectionMarker);
        delete m_selectionMarker;
        m_selectionMarker = nullptr;
    }
}

const Style *SwimlaneItem::adaptedStyle()
{
    return m_diagramSceneModel->styleController()->adaptSwimlaneStyle(m_swimlane);
}

QSizeF SwimlaneItem::calcMinimumGeometry() const
{
    if (m_swimlane->isHorizontal())
        return QSizeF(SWIMLANE_LENGTH, SWIMLANE_WIDTH);
    else
        return QSizeF(SWIMLANE_WIDTH, SWIMLANE_LENGTH);
}

void SwimlaneItem::updateGeometry()
{
    prepareGeometryChange();

    QSizeF geometry = calcMinimumGeometry();
    qreal width = geometry.width();
    qreal height = geometry.height();

    if (m_swimlane->isHorizontal()) {
        setPos(0, m_swimlane->pos());
        if (m_lineItem)
            m_lineItem->setLine(-width / 2, 0, width / 2, 0);
        if (m_selectionMarker)
            m_selectionMarker->setRect(-width / 2, -SWIMLANE_MARKER_WIDTH / 2, width, SWIMLANE_MARKER_WIDTH);
    } else {
        setPos(m_swimlane->pos(), 0);
        if (m_lineItem)
            m_lineItem->setLine(0, -height / 2, 0, height / 2);
        if (m_selectionMarker)
            m_selectionMarker->setRect(-SWIMLANE_MARKER_WIDTH / 2, -height / 2, SWIMLANE_MARKER_WIDTH, height);
    }
}

} // namespace qmt
