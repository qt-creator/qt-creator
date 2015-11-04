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
        NotSelected,
        Selected,
        SecondarySelected
    };

public:

    GraphicsHandleItem(int pointIndex, PathSelectionItem *parent)
        : QGraphicsRectItem(parent),
          m_owner(parent),
          m_pointIndex(pointIndex),
          m_selection(NotSelected)
    {
    }

public:

    void setPointSize(const QSizeF &pointSize)
    {
        if (m_pointSize != pointSize) {
            m_pointSize = pointSize;
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
        m_qualifier = event->modifiers() & Qt::ControlModifier ? DeleteHandle : None;
        m_owner->moveHandle(m_pointIndex, QPointF(0.0, 0.0), Press, m_qualifier);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event)
    {
        QPointF delta = mapToScene(event->pos()) - m_startPos;
        m_owner->moveHandle(m_pointIndex, delta, Move, m_qualifier);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
    {
        QPointF delta = mapToScene(event->pos()) - m_startPos;
        m_owner->moveHandle(m_pointIndex, delta, Release, m_qualifier);
    }

private:

    void update()
    {
        prepareGeometryChange();
        setRect(-m_pointSize.width() / 2.0, -m_pointSize.height() / 2.0, m_pointSize.width(), m_pointSize.height());
        switch (m_selection) {
        case NotSelected:
            setPen(Qt::NoPen);
            setBrush(Qt::NoBrush);
            break;
        case Selected:
            setPen(QPen(Qt::black));
            setBrush(QBrush(Qt::black));
            break;
        case SecondarySelected:
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
    QPainterPath polygonPath;
    polygonPath.addPolygon(polygon);
    QPainterPathStroker ps;
    ps.setWidth(16.0);
    polygonPath = ps.createStroke(polygonPath);
    shape.addPath(polygonPath);
    return shape;
}

void PathSelectionItem::setPointSize(const QSizeF &size)
{
    if (size != m_pointSize) {
        m_pointSize = size;
        update();
    }
}

QList<QPointF> PathSelectionItem::points() const
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
    int pointIndex = 0;
    foreach (const QPointF &point, points) {
        GraphicsHandleItem *handle;
        if (pointIndex >= m_handles.size()) {
            handle = new GraphicsHandleItem(pointIndex, this);
            handle->setPointSize(m_pointSize);
            m_handles.append(handle);
        } else {
            handle = m_handles.at(pointIndex);
        }
        handle->setPos(point);
        ++pointIndex;
    }
    while (m_handles.size() > pointIndex) {
        m_handles.last()->scene()->removeItem(m_handles.last());
        delete m_handles.last();
        m_handles.removeLast();
    }
    update();
}

void PathSelectionItem::setSecondarySelected(bool secondarySelected)
{
    if (m_secondarySelected != secondarySelected) {
        m_secondarySelected = secondarySelected;
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
        bool isEndPoint = (i == 0 || i == m_handles.size() - 1);
        handle->setSelection(m_secondarySelected
                             ? (isEndPoint ? GraphicsHandleItem::NotSelected : GraphicsHandleItem::SecondarySelected)
                             : GraphicsHandleItem::Selected);
        ++i;
    }
}

void PathSelectionItem::moveHandle(int pointIndex, const QPointF &deltaMove, HandleStatus handleStatus, HandleQualifier handleQualifier)
{
    Q_UNUSED(handleStatus);

    switch (handleQualifier) {
    case None:
    {
        if (handleStatus == Press) {
            m_originalHandlePos = m_windable->handlePos(pointIndex);
        }
        QPointF newPos = m_originalHandlePos + deltaMove;
        m_windable->setHandlePos(pointIndex, newPos);
        if (handleStatus == Release) {
            m_windable->alignHandleToRaster(pointIndex, RASTER_WIDTH, RASTER_HEIGHT);
        }
        break;
    }
    case DeleteHandle:
        if (handleStatus == Press) {
            m_windable->deleteHandle(pointIndex);
        }
        break;
    }
}

}
