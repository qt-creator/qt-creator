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
          m_owner(parent),
          m_pointIndex(point_index),
          m_selection(NOT_SELECTED)
    {
    }

public:

    void setPointSize(const QSizeF &point_size)
    {
        if (m_pointSize != point_size) {
            m_pointSize = point_size;
            update();
        }
    }

    void setSelection(Selection selection)
    {
        if (m_selection != selection) {
            m_selection = selection;
            update();
        }
    }

protected:

    void mousePressEvent(QGraphicsSceneMouseEvent *event)
    {
        m_startPos = mapToScene(event->pos());
        m_qualifier = event->modifiers() & Qt::ControlModifier ? DELETE_HANDLE : NONE;
        m_owner->moveHandle(m_pointIndex, QPointF(0.0, 0.0), PRESS, m_qualifier);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event)
    {
        QPointF delta = mapToScene(event->pos()) - m_startPos;
        m_owner->moveHandle(m_pointIndex, delta, MOVE, m_qualifier);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
    {
        QPointF delta = mapToScene(event->pos()) - m_startPos;
        m_owner->moveHandle(m_pointIndex, delta, RELEASE, m_qualifier);
    }

private:

    void update()
    {
        prepareGeometryChange();
        setRect(-m_pointSize.width() / 2.0, -m_pointSize.height() / 2.0, m_pointSize.width(), m_pointSize.height());
        switch (m_selection) {
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

    PathSelectionItem *m_owner;

    int m_pointIndex;

    QSizeF m_pointSize;

    Selection m_selection;

    QPointF m_startPos;

    PathSelectionItem::HandleQualifier m_qualifier;

};



PathSelectionItem::PathSelectionItem(IWindable *windable, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_windable(windable),
      m_pointSize(QSizeF(8.0, 8.0)),
      m_secondarySelected(false)
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
    foreach (const GraphicsHandleItem *handle, m_handles) {
        shape.addPath(handle->shape());
    }
    // TODO duplicate of ArrowItem::GraphicsShaftItem's shape
    QPolygonF polygon;
    for (int i = 0; i < m_handles.size(); ++i) {
        polygon.append(m_handles.at(i)->pos());
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
    if (size != m_pointSize) {
        m_pointSize = size;
        update();
    }
}

QList<QPointF> PathSelectionItem::getPoints() const
{
    QList<QPointF> points;
    foreach (GraphicsHandleItem *handle, m_handles) {
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
        if (point_index >= m_handles.size()) {
            handle = new GraphicsHandleItem(point_index, this);
            handle->setPointSize(m_pointSize);
            m_handles.append(handle);
        } else {
            handle = m_handles.at(point_index);
        }
        handle->setPos(point);
        ++point_index;
    }
    while (m_handles.size() > point_index) {
        m_handles.last()->scene()->removeItem(m_handles.last());
        delete m_handles.last();
        m_handles.removeLast();
    }
    update();
}

void PathSelectionItem::setSecondarySelected(bool secondary_selected)
{
    if (m_secondarySelected != secondary_selected) {
        m_secondarySelected = secondary_selected;
        update();
    }
}

void PathSelectionItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->modifiers() & Qt::ShiftModifier) {
        for (int i = 0; i < m_handles.size() - 1; ++i) {
            qreal distance = GeometryUtilities::calcDistancePointToLine(event->pos(), QLineF(m_handles.at(i)->pos(), m_handles.at(i+1)->pos()));
            if (distance < MAX_SELECTION_DISTANCE_FROM_PATH) {
                m_windable->insertHandle(i + 1, event->scenePos());
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
    foreach (GraphicsHandleItem *handle, m_handles) {
        handle->setPointSize(m_pointSize);
        bool is_end_point = (i == 0 || i == m_handles.size() - 1);
        handle->setSelection(m_secondarySelected
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
            m_originalHandlePos = m_windable->getHandlePos(point_index);
        }
        QPointF new_pos = m_originalHandlePos + delta_move;
        m_windable->setHandlePos(point_index, new_pos);
        if (handle_status == RELEASE) {
            m_windable->alignHandleToRaster(point_index, RASTER_WIDTH, RASTER_HEIGHT);
        }
        break;
    }
    case DELETE_HANDLE:
        if (handle_status == PRESS) {
            m_windable->deleteHandle(point_index);
        }
        break;
    }
}

}
