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

#include "pathselectionitem.h"

#include "qmt/diagram_scene/capabilities/windable.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/infrastructure/geometryutilities.h"
#include "qmt/infrastructure/qmtassert.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QBrush>
#include <QLineF>
#include <QPainter>
#include <QKeyEvent>

namespace qmt {

static const double MAX_SELECTION_DISTANCE_FROM_PATH = 4.0;

class PathSelectionItem::GraphicsHandleItem : public QGraphicsRectItem
{
public:
    enum Selection {
        NotSelected,
        Selected,
        SecondarySelected
    };

    GraphicsHandleItem(int pointIndex, PathSelectionItem *parent)
        : QGraphicsRectItem(parent),
          m_owner(parent),
          m_pointIndex(pointIndex)
    {
        setFlag(QGraphicsItem::ItemIsFocusable);
    }

    void setPointIndex(int pointIndex)
    {
        m_pointIndex = pointIndex;
    }

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
        m_startPos = event->scenePos();
        m_lastPos = m_startPos;
        m_qualifier = event->modifiers() & Qt::ControlModifier ? DeleteHandle : None;
        m_owner->moveHandle(m_pointIndex, QPointF(0.0, 0.0), Press, m_qualifier);
        setFocus();
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event)
    {
        m_lastPos = event->scenePos();
        QPointF delta = m_lastPos - m_startPos;
        m_owner->moveHandle(m_pointIndex, delta, Move, m_qualifier);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
    {
        m_lastPos = event->scenePos();
        QPointF delta = m_lastPos - m_startPos;
        m_owner->moveHandle(m_pointIndex, delta, Release, m_qualifier);
        clearFocus();
    }

    void keyPressEvent(QKeyEvent *event)
    {
        m_owner->keyPressed(m_pointIndex, event, m_lastPos);
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

    PathSelectionItem *m_owner = 0;
    int m_pointIndex = -1;
    QSizeF m_pointSize;
    Selection m_selection = NotSelected;
    QPointF m_startPos;
    QPointF m_lastPos;
    PathSelectionItem::HandleQualifier m_qualifier = PathSelectionItem::None;
};

PathSelectionItem::PathSelectionItem(IWindable *windable, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_windable(windable),
      m_pointSize(QSizeF(8.0, 8.0))
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
    foreach (const GraphicsHandleItem *handle, m_handles)
        shape.addPath(handle->shape());
    // TODO duplicate of ArrowItem::GraphicsShaftItem's shape
    QPolygonF polygon;
    for (int i = 0; i < m_handles.size(); ++i)
        polygon.append(m_handles.at(i)->pos());
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
    foreach (GraphicsHandleItem *handle, m_handles)
        points.append(handle->pos());
    return points;
}

void PathSelectionItem::setPoints(const QList<QPointF> &points)
{
    QMT_CHECK(points.size() >= 2);
    prepareGeometryChange();

    GraphicsHandleItem *focusEndBItem = 0;
    if (!m_handles.isEmpty() && m_focusHandleItem == m_handles.last()) {
        focusEndBItem = m_focusHandleItem;
        m_handles.removeLast();
    }
    int pointIndex = 0;
    foreach (const QPointF &point, points) {
        GraphicsHandleItem *handle;
        if (focusEndBItem && pointIndex == points.size() - 1) {
            handle = focusEndBItem;
            handle->setPointIndex(pointIndex);
            m_handles.insert(pointIndex, handle);
            focusEndBItem = 0;
        } else if (pointIndex >= m_handles.size()) {
            handle = new GraphicsHandleItem(pointIndex, this);
            handle->setPointSize(m_pointSize);
            m_handles.append(handle);
        } else {
            handle = m_handles.at(pointIndex);
        }
        handle->setPos(point);
        ++pointIndex;
    }
    QMT_CHECK(!focusEndBItem);
    while (m_handles.size() > pointIndex) {
        m_handles.last()->scene()->removeItem(m_handles.last());
        delete m_handles.last();
        m_handles.removeLast();
    }
    update();
}

void PathSelectionItem::setSecondarySelected(bool secondarySelected)
{
    if (m_isSecondarySelected != secondarySelected) {
        m_isSecondarySelected = secondarySelected;
        update();
    }
}

void PathSelectionItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->modifiers() & Qt::ShiftModifier) {
        for (int i = 0; i < m_handles.size() - 1; ++i) {
            qreal distance = GeometryUtilities::calcDistancePointToLine(event->pos(), QLineF(m_handles.at(i)->pos(), m_handles.at(i+1)->pos()));
            if (distance < MAX_SELECTION_DISTANCE_FROM_PATH) {
                m_windable->insertHandle(i + 1, event->scenePos(), RASTER_WIDTH, RASTER_HEIGHT);
                event->accept();
                return;
            }
        }
    } else {
        QGraphicsItem::mousePressEvent(event);
    }
}

bool PathSelectionItem::isEndHandle(int pointIndex) const
{
    return pointIndex == 0 || pointIndex == m_handles.size() - 1;
}

void PathSelectionItem::update()
{
    prepareGeometryChange();
    int i = 0;
    foreach (GraphicsHandleItem *handle, m_handles) {
        handle->setPointSize(m_pointSize);
        handle->setSelection(m_isSecondarySelected
                             ? (isEndHandle(i) ? GraphicsHandleItem::NotSelected : GraphicsHandleItem::SecondarySelected)
                             : GraphicsHandleItem::Selected);
        ++i;
    }
}

void PathSelectionItem::moveHandle(int pointIndex, const QPointF &deltaMove, HandleStatus handleStatus, HandleQualifier handleQualifier)
{
    switch (handleQualifier) {
    case None:
    {
        if (handleStatus == Press) {
            m_focusHandleItem = m_handles.at(pointIndex);
            m_originalHandlePos = m_windable->grabHandle(pointIndex);
        }
        QPointF newPos = m_originalHandlePos + deltaMove;
        m_windable->setHandlePos(pointIndex, newPos);
        if (handleStatus == Release) {
            m_windable->dropHandle(pointIndex, RASTER_WIDTH, RASTER_HEIGHT);
            m_focusHandleItem = 0;
        }
        break;
    }
    case DeleteHandle:
        if (handleStatus == Press)
            m_windable->deleteHandle(pointIndex);
        break;
    }
}

void PathSelectionItem::keyPressed(int pointIndex, QKeyEvent *event, const QPointF &pos)
{
    if (isEndHandle(pointIndex)) {
        if (event->key() == Qt::Key_Shift)
            m_windable->insertHandle(pointIndex, pos, RASTER_WIDTH, RASTER_HEIGHT);
        else if (event->key() == Qt::Key_Control)
            m_windable->deleteHandle(pointIndex);
    }
}

} // namespace qmt
