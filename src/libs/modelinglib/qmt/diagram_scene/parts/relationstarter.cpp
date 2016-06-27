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

#include "relationstarter.h"

#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/capabilities/relationable.h"
#include "qmt/infrastructure/qmtassert.h"
#include "qmt/style/stylecontroller.h"

#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QGraphicsScene>
#include <QPainter>

namespace qmt {

RelationStarter::RelationStarter(IRelationable *owner, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent)
    : QGraphicsRectItem(parent),
      m_owner(owner),
      m_diagramSceneModel(diagramSceneModel)
{
    setBrush(QBrush(QColor(192, 192, 192)));
    setPen(QPen(QColor(64, 64, 64)));
    setFlag(QGraphicsItem::ItemIsFocusable);
}

RelationStarter::~RelationStarter()
{
}

QRectF RelationStarter::boundingRect() const
{
    return rect();
}

void RelationStarter::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    painter->setPen(pen());
    painter->setBrush(brush());
    painter->drawRoundedRect(rect(), 3, 3);
    painter->restore();
}

void RelationStarter::addArrow(const QString &id, ArrowItem::Shaft shaft,
                               ArrowItem::Head endHead, ArrowItem::Head startHead)
{
    QMT_CHECK(!id.isEmpty());
    prepareGeometryChange();
    auto arrow = new ArrowItem(this);
    arrow->setArrowSize(10.0);
    arrow->setDiamondSize(15.0);
    arrow->setShaft(shaft);
    arrow->setStartHead(startHead);
    arrow->setEndHead(endHead);
    arrow->setPoints(QList<QPointF>() << QPointF(0.0, 10.0) << QPointF(15.0, 0.0));
    arrow->setPos(6.0, m_arrows.size() * 20.0 + 8.0);
    arrow->update(m_diagramSceneModel->styleController()->relationStarterStyle());
    m_arrows.append(arrow);
    m_arrowIds.insert(arrow, id);
    setRect(0.0, 0.0, 26.0, m_arrows.size() * 20.0 + 6.0);
}

void RelationStarter::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_currentPreviewArrow)
        return;
    foreach (ArrowItem *item, m_arrows) {
        if (item->boundingRect().contains(mapToItem(item, event->pos()))) {
            prepareGeometryChange();
            m_currentPreviewArrowIntermediatePoints.clear();
            m_currentPreviewArrowId = m_arrowIds.value(item);
            QMT_CHECK(!m_currentPreviewArrowId.isEmpty());
            m_currentPreviewArrow = new ArrowItem(*item);
            m_currentPreviewArrow->setPoints(QList<QPointF>() << m_owner->relationStartPos() << mapToScene(event->pos()));
            m_currentPreviewArrow->update(m_diagramSceneModel->styleController()->relationStarterStyle());
            m_currentPreviewArrow->setZValue(PREVIEW_RELATION_ZVALUE);
            scene()->addItem(m_currentPreviewArrow);
            setFocus(); // receive keyboard events
            break;
        }
    }
}

void RelationStarter::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_currentPreviewArrow)
        return;
    updateCurrentPreviewArrow(mapToScene(event->pos()));
}

void RelationStarter::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_currentPreviewArrow) {
        m_owner->relationDrawn(m_currentPreviewArrowId, mapToScene(event->pos()),
                               m_currentPreviewArrowIntermediatePoints);
        m_currentPreviewArrow->scene()->removeItem(m_currentPreviewArrow);
        delete m_currentPreviewArrow;
        m_currentPreviewArrow = 0;
        m_currentPreviewArrowIntermediatePoints.clear();
    }
}

void RelationStarter::keyPressEvent(QKeyEvent *event)
{
    if (!m_currentPreviewArrow)
        return;
    if (event->key() == Qt::Key_Shift) {
        QPointF p = m_currentPreviewArrow->lastLineSegment().p1();
        if (m_currentPreviewArrowIntermediatePoints.isEmpty() || m_currentPreviewArrowIntermediatePoints.last() != p) {
            m_currentPreviewArrowIntermediatePoints.append(p);
            // Do not update the preview arrow here because last two points are now identical which looks wired
        }
    } else if (event->key() == Qt::Key_Control) {
        if (!m_currentPreviewArrowIntermediatePoints.isEmpty()) {
            m_currentPreviewArrowIntermediatePoints.removeLast();
            updateCurrentPreviewArrow(m_currentPreviewArrow->lastLineSegment().p1());
        }
    }
}

void RelationStarter::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    if (m_currentPreviewArrow) {
        m_currentPreviewArrow->scene()->removeItem(m_currentPreviewArrow);
        delete m_currentPreviewArrow;
        m_currentPreviewArrow = 0;
        m_currentPreviewArrowIntermediatePoints.clear();
    }
}

void RelationStarter::updateCurrentPreviewArrow(const QPointF &headPoint)
{
    prepareGeometryChange();
    m_currentPreviewArrow->setPoints(QList<QPointF>() << m_owner->relationStartPos()
                                      << m_currentPreviewArrowIntermediatePoints << headPoint);
    m_currentPreviewArrow->update(m_diagramSceneModel->styleController()->relationStarterStyle());
}

} // namespace qmt
