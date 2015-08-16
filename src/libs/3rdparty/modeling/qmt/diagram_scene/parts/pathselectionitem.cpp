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

#include "pathselectionitem.h"

#include "qmt/diagram_scene/capabilities/windable.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/infrastructure/geometryutilities.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QBrush>
#include <QLineF>
#include <QPainter>

//#define DEBUG_PAINT_SHAPE


namespace qmt {

static const double MAX_SELECTION_DISTANCE_FROM_PATH = 4.0;


class PathSelectionItem::GraphicsHandleItem :
        public QGraphicsRectItem
{
public:

    enum Selection {
        NOT_SELECTED,
        SELECTED,
        SECONDARY_SELECTED
    };

public:

    GraphicsHandleItem(int point_index, PathSelectionItem *parent)
        : QGraphicsRectItem(parent),
          _owner(parent),
          _point_index(point_index),
          _selection(NOT_SELECTED)
    {
    }

public:

    void setPointSize(const QSizeF &point_size)
    {
        if (_point_size != point_size) {
            _point_size = point_size;
            update();
        }
    }

    void setSelection(Selection selection)
    {
        if (_selection != selection) {
            _selection = selection;
            update();
        }
    }

protected:

    void mousePressEvent(QGraphicsSceneMouseEvent *event)
    {
        _start_pos = mapToScene(event->pos());
        _qualifier = event->modifiers() & Qt::ControlModifier ? DELETE_HANDLE : NONE;
        _owner->moveHandle(_point_index, QPointF(0.0, 0.0), PRESS, _qualifier);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event)
    {
        QPointF delta = mapToScene(event->pos()) - _start_pos;
        _owner->moveHandle(_point_index, delta, MOVE, _qualifier);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
    {
        QPointF delta = mapToScene(event->pos()) - _start_pos;
        _owner->moveHandle(_point_index, delta, RELEASE, _qualifier);
    }

private:

    void update()
    {
        prepareGeometryChange();
        setRect(-_point_size.width() / 2.0, -_point_size.height() / 2.0, _point_size.width(), _point_size.height());
        switch (_selection) {
        case NOT_SELECTED:
            setPen(Qt::NoPen);
            setBrush(Qt::NoBrush);
            break;
        case SELECTED:
            setPen(QPen(Qt::black));
            setBrush(QBrush(Qt::black));
            break;
        case SECONDARY_SELECTED:
            setPen(QPen(Qt::lightGray));
            setBrush(Qt::NoBrush);
            break;
        }
    }

private:

    PathSelectionItem *_owner;

    int _point_index;

    QSizeF _point_size;

    Selection _selection;

    QPointF _start_pos;

    PathSelectionItem::HandleQualifier _qualifier;

};



PathSelectionItem::PathSelectionItem(IWindable *windable, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      _windable(windable),
      _point_size(QSizeF(8.0, 8.0)),
      _secondary_selected(false)
{
}

PathSelectionItem::~PathSelectionItem()
{
}

QRectF PathSelectionItem::boundingRect() const
{
    return childrenBoundingRect();
}

void PathSelectionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);

#ifdef DEBUG_PAINT_SHAPE
    painter->save();
    painter->setPen(Qt::red);
    painter->setBrush(Qt::blue);
    painter->drawPath(shape());
    painter->restore();
#endif
}

QPainterPath PathSelectionItem::shape() const
{
    QPainterPath shape;
    shape.setFillRule(Qt::WindingFill);
    foreach (const GraphicsHandleItem *handle, _handles) {
        shape.addPath(handle->shape());
    }
    // TODO duplicate of ArrowItem::GraphicsShaftItem's shape
    QPolygonF polygon;
    for (int i = 0; i < _handles.size(); ++i) {
        polygon.append(_handles.at(i)->pos());
    }
    QPainterPath polygon_path;
    polygon_path.addPolygon(polygon);
    QPainterPathStroker ps;
    ps.setWidth(16.0);
    polygon_path = ps.createStroke(polygon_path);
    shape.addPath(polygon_path);
    return shape;
}

void PathSelectionItem::setPointSize(const QSizeF &size)
{
    if (size != _point_size) {
        _point_size = size;
        update();
    }
}

QList<QPointF> PathSelectionItem::getPoints() const
{
    QList<QPointF> points;
    foreach (GraphicsHandleItem *handle, _handles) {
        points.append(handle->pos());
    }
    return points;
}

void PathSelectionItem::setPoints(const QList<QPointF> &points)
{
    prepareGeometryChange();
    int point_index = 0;
    foreach (const QPointF &point, points) {
        GraphicsHandleItem *handle;
        if (point_index >= _handles.size()) {
            handle = new GraphicsHandleItem(point_index, this);
            handle->setPointSize(_point_size);
            _handles.append(handle);
        } else {
            handle = _handles.at(point_index);
        }
        handle->setPos(point);
        ++point_index;
    }
    while (_handles.size() > point_index) {
        _handles.last()->scene()->removeItem(_handles.last());
        delete _handles.last();
        _handles.removeLast();
    }
    update();
}

void PathSelectionItem::setSecondarySelected(bool secondary_selected)
{
    if (_secondary_selected != secondary_selected) {
        _secondary_selected = secondary_selected;
        update();
    }
}

void PathSelectionItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->modifiers() & Qt::ShiftModifier) {
        for (int i = 0; i < _handles.size() - 1; ++i) {
            qreal distance = GeometryUtilities::calcDistancePointToLine(event->pos(), QLineF(_handles.at(i)->pos(), _handles.at(i+1)->pos()));
            if (distance < MAX_SELECTION_DISTANCE_FROM_PATH) {
                _windable->insertHandle(i + 1, event->scenePos());
                event->accept();
                return;
            }
        }
    } else {
        QGraphicsItem::mousePressEvent(event);
    }
}

void PathSelectionItem::update()
{
    prepareGeometryChange();
    int i = 0;
    foreach (GraphicsHandleItem *handle, _handles) {
        handle->setPointSize(_point_size);
        bool is_end_point = (i == 0 || i == _handles.size() - 1);
        handle->setSelection(_secondary_selected
                             ? (is_end_point ? GraphicsHandleItem::NOT_SELECTED : GraphicsHandleItem::SECONDARY_SELECTED)
                             : GraphicsHandleItem::SELECTED);
        ++i;
    }
}

void PathSelectionItem::moveHandle(int point_index, const QPointF &delta_move, HandleStatus handle_status, HandleQualifier handle_qualifier)
{
    Q_UNUSED(handle_status);

    switch (handle_qualifier) {
    case NONE:
    {
        if (handle_status == PRESS) {
            _original_handle_pos = _windable->getHandlePos(point_index);
        }
        QPointF new_pos = _original_handle_pos + delta_move;
        _windable->setHandlePos(point_index, new_pos);
        if (handle_status == RELEASE) {
            _windable->alignHandleToRaster(point_index, RASTER_WIDTH, RASTER_HEIGHT);
        }
        break;
    }
    case DELETE_HANDLE:
        if (handle_status == PRESS) {
            _windable->deleteHandle(point_index);
        }
        break;
    }
}

}
