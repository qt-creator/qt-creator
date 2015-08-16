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

#include <QDebug>

namespace qmt {

RelationStarter::RelationStarter(IRelationable *owner, DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent)
    : QGraphicsRectItem(parent),
      _owner(owner),
      _diagram_scene_model(diagram_scene_model),
      _current_preview_arrow(0)
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
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(pen());
    painter->setBrush(brush());
    painter->drawRoundedRect(rect(), 3, 3);
    painter->restore();
}

void RelationStarter::addArrow(const QString &id, ArrowItem::Shaft shaft, ArrowItem::Head end_head, ArrowItem::Head start_head)
{
    QMT_CHECK(!id.isEmpty());
    prepareGeometryChange();
    ArrowItem *arrow = new ArrowItem(this);
    arrow->setArrowSize(10.0);
    arrow->setDiamondSize(15.0);
    arrow->setShaft(shaft);
    arrow->setStartHead(start_head);
    arrow->setEndHead(end_head);
    arrow->setPoints(QList<QPointF>() << QPointF(0.0, 10.0) << QPointF(15.0, 0.0));
    arrow->setPos(6.0, _arrows.size() * 20.0 + 8.0);
    arrow->update(_diagram_scene_model->getStyleController()->getRelationStarterStyle());
    _arrows.append(arrow);
    _arrow_ids.insert(arrow, id);
    setRect(0.0, 0.0, 26.0, _arrows.size() * 20.0 + 6.0);
}

void RelationStarter::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    _current_preview_arrow = 0;
    foreach (ArrowItem *item, _arrows) {
        if (item->boundingRect().contains(mapToItem(item, event->pos()))) {
            prepareGeometryChange();
            _current_preview_arrow_intermediate_points.clear();
            _current_preview_arrow_id = _arrow_ids.value(item);
            QMT_CHECK(!_current_preview_arrow_id.isEmpty());
            _current_preview_arrow = new ArrowItem(*item);
            _current_preview_arrow->setPoints(QList<QPointF>() << _owner->getRelationStartPos() << mapToScene(event->pos()));
            _current_preview_arrow->update(_diagram_scene_model->getStyleController()->getRelationStarterStyle());
            _current_preview_arrow->setZValue(PREVIEW_RELATION_ZVALUE);
            scene()->addItem(_current_preview_arrow);
            setFocus(); // receive keyboard events
            break;
        }
    }
}

void RelationStarter::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!_current_preview_arrow) {
        return;
    }
    updateCurrentPreviewArrow(mapToScene(event->pos()));
}

void RelationStarter::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (_current_preview_arrow) {
        _owner->relationDrawn(_current_preview_arrow_id, mapToScene(event->pos()), _current_preview_arrow_intermediate_points);
        _current_preview_arrow->scene()->removeItem(_current_preview_arrow);
        delete _current_preview_arrow;
        _current_preview_arrow = 0;
        _current_preview_arrow_intermediate_points.clear();
    }
}

void RelationStarter::keyPressEvent(QKeyEvent *event)
{
    if (!_current_preview_arrow) {
        return;
    }
    if (event->key() == Qt::Key_Shift) {
        QPointF p = _current_preview_arrow->getLastLineSegment().p1();
        if (_current_preview_arrow_intermediate_points.isEmpty() || _current_preview_arrow_intermediate_points.last() != p) {
            _current_preview_arrow_intermediate_points.append(p);
            // Do not update the preview arrow here because last two points are now identical which looks wired
        }
    } else if (event->key() == Qt::Key_Control) {
        if (!_current_preview_arrow_intermediate_points.isEmpty()) {
            _current_preview_arrow_intermediate_points.removeLast();
            updateCurrentPreviewArrow(_current_preview_arrow->getLastLineSegment().p1());
        }
    }
}

void RelationStarter::updateCurrentPreviewArrow(const QPointF &head_point)
{
    prepareGeometryChange();
    _current_preview_arrow->setPoints(QList<QPointF>() << _owner->getRelationStartPos()
                                      << _current_preview_arrow_intermediate_points << head_point);
    _current_preview_arrow->update(_diagram_scene_model->getStyleController()->getRelationStarterStyle());
}

}
