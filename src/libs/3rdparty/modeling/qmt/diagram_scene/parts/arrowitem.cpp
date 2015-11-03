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

#include "arrowitem.h"

#include "qmt/infrastructure/qmtassert.h"
#include "qmt/style/style.h"

#include <QPainterPath>
#include <QPen>
#include <qmath.h>
#include <QVector2D>
#include <qdebug.h>

#include <QGraphicsScene>
#include <QPainter>
#include <QPen>

namespace qmt {

class ArrowItem::GraphicsPathItem :
        public QGraphicsPathItem
{
public:
    GraphicsPathItem(QGraphicsItem *parent)
        : QGraphicsPathItem(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        QGraphicsPathItem::paint(painter, option, widget);
        painter->restore();
    }
};


class ArrowItem::GraphicsHeadItem :
        public QGraphicsItem
{
public:
    GraphicsHeadItem(QGraphicsItem *parent)
        : QGraphicsItem(parent),
          m_head(ArrowItem::HEAD_NONE),
          m_arrowSize(10.0),
          m_diamondSize(15.0),
          m_arrowItem(0),
          m_diamondItem(0)
    {
    }

    ~GraphicsHeadItem()
    {
    }

public:

    void setHead(ArrowItem::Head head)
    {
        if (m_head != head) {
            m_head = head;
        }
    }

    void setArrowSize(double arrow_size)
    {
        if (m_arrowSize != arrow_size) {
            m_arrowSize = arrow_size;
        }
    }

    void setDiamondSize(double diamond_size)
    {
        if (m_diamondSize != diamond_size) {
            m_diamondSize = diamond_size;
        }
    }

public:

    QRectF boundingRect() const
    {
        return childrenBoundingRect();
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        Q_UNUSED(painter);
        Q_UNUSED(option);
        Q_UNUSED(widget);

#if 0
        painter->setPen(QPen(Qt::blue));
        painter->drawRect(boundingRect());
        painter->setPen(QPen(Qt::red));
        painter->drawPath(shape());
        painter->setPen(QPen(Qt::green));
        if (m_arrowItem) {
            painter->drawRect(mapRectFromItem(m_arrowItem, m_arrowItem->boundingRect()));
        }
        if (m_diamondItem) {
            painter->drawRect(mapRectFromItem(m_diamondItem, m_diamondItem->boundingRect()));
        }
#endif
    }

    QPainterPath shape() const
    {
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        if (m_arrowItem) {
            path.addRect(mapRectFromItem(m_arrowItem, m_arrowItem->boundingRect()));
        }
        if (m_diamondItem) {
            path.addRect(mapRectFromItem(m_diamondItem, m_diamondItem->boundingRect()));
        }
        return path;
    }

public:

    double calcHeadLength() const
    {
        double length = 0.0;
        switch (m_head) {
        case ArrowItem::HEAD_NONE:
            break;
        case ArrowItem::HEAD_OPEN:
        case ArrowItem::HEAD_TRIANGLE:
        case ArrowItem::HEAD_FILLED_TRIANGLE:
            length = calcArrowLength();
            break;
        case ArrowItem::HEAD_DIAMOND:
        case ArrowItem::HEAD_FILLED_DIAMOND:
            length = calcDiamondLength();
            break;
        case ArrowItem::HEAD_DIAMOND_FILLED_TRIANGLE:
        case ArrowItem::HEAD_FILLED_DIAMOND_FILLED_TRIANGLE:
            length = calcDiamondLength() + calcArrowLength();
            break;
        }
        return length;
    }

private:

    double calcArrowLength() const
    {
        return sqrt(3.0) * 0.5 * m_arrowSize;
    }

    double calcDiamondLength() const
    {
        return sqrt(3.0) * m_diamondSize;
    }

public:

    void update(const Style *style)
    {
        bool has_arrow = false;
        bool has_diamond = false;
        switch (m_head) {
        case ArrowItem::HEAD_NONE:
            break;
        case ArrowItem::HEAD_OPEN:
        case ArrowItem::HEAD_TRIANGLE:
        case ArrowItem::HEAD_FILLED_TRIANGLE:
            has_arrow = true;
            break;
        case ArrowItem::HEAD_DIAMOND:
        case ArrowItem::HEAD_FILLED_DIAMOND:
            has_diamond = true;
            break;
        case ArrowItem::HEAD_DIAMOND_FILLED_TRIANGLE:
        case ArrowItem::HEAD_FILLED_DIAMOND_FILLED_TRIANGLE:
            has_arrow = true;
            has_diamond = true;
            break;
        }

        if (has_arrow) {
            if (!m_arrowItem) {
                m_arrowItem = new ArrowItem::GraphicsPathItem(this);
            }

            if (m_head == ArrowItem::HEAD_OPEN || m_head == ArrowItem::HEAD_TRIANGLE) {
                m_arrowItem->setPen(style->getLinePen());
                m_arrowItem->setBrush(QBrush());
            } else {
                m_arrowItem->setPen(style->getLinePen());
                m_arrowItem->setBrush(style->getFillBrush());
            }

            QPainterPath path;
            double h = calcArrowLength();
            path.moveTo(-h, -m_arrowSize * 0.5);
            path.lineTo(0.0, 0.0);
            path.lineTo(-h, m_arrowSize * 0.5);
            if (m_head != ArrowItem::HEAD_OPEN) {
                path.closeSubpath();
            }
            if (has_diamond) {
                path.translate(-calcDiamondLength(), 0.0);
            }
            m_arrowItem->setPath(path);
        } else if (m_arrowItem) {
            m_arrowItem->scene()->removeItem(m_arrowItem);
            delete m_arrowItem;
            m_arrowItem = 0;
        }

        if (has_diamond) {
            if (!m_diamondItem) {
                m_diamondItem = new ArrowItem::GraphicsPathItem(this);
            }

            if (m_head == ArrowItem::HEAD_DIAMOND || m_head == ArrowItem::HEAD_DIAMOND_FILLED_TRIANGLE) {
                m_diamondItem->setPen(style->getLinePen());
                m_diamondItem->setBrush(QBrush());
            } else {
                m_diamondItem->setPen(style->getLinePen());
                m_diamondItem->setBrush(style->getFillBrush());
            }

            QPainterPath path;
            double h = calcDiamondLength();
            path.lineTo(-h * 0.5, -m_diamondSize * 0.5);
            path.lineTo(-h, 0.0);
            path.lineTo(-h * 0.5, m_diamondSize * 0.5);
            path.closeSubpath();
            m_diamondItem->setPath(path);
        } else if (m_diamondItem) {
            m_diamondItem->scene()->removeItem(m_diamondItem);
            delete m_diamondItem;
            m_diamondItem = 0;
        }
    }

private:

    ArrowItem::Head m_head;

    double m_arrowSize;

    double m_diamondSize;

    ArrowItem::GraphicsPathItem *m_arrowItem;

    ArrowItem::GraphicsPathItem *m_diamondItem;

};


class ArrowItem::GraphicsShaftItem :
        public ArrowItem::GraphicsPathItem
{
public:
    GraphicsShaftItem(QGraphicsItem *parent)
        : GraphicsPathItem(parent)
    {
    }
};


ArrowItem::ArrowItem(QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_shaft(SHAFT_SOLID),
      m_shaftItem(new GraphicsShaftItem(this)),
      m_arrowSize(10.0),
      m_diamondSize(15.0),
      m_startHead(HEAD_NONE),
      m_startHeadItem(0),
      m_endHead(HEAD_NONE),
      m_endHeadItem(0)
{
}

ArrowItem::ArrowItem(const ArrowItem &rhs, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_shaft(rhs.m_shaft),
      m_shaftItem(new GraphicsShaftItem(this)),
      m_arrowSize(rhs.m_arrowSize),
      m_diamondSize(rhs.m_diamondSize),
      m_startHead(rhs.m_startHead),
      m_startHeadItem(0),
      m_endHead(rhs.m_endHead),
      m_endHeadItem(0)
{
}

ArrowItem::~ArrowItem()
{
}

void ArrowItem::setShaft(ArrowItem::Shaft shaft)
{
    if (m_shaft != shaft) {
        m_shaft = shaft;
    }
}

void ArrowItem::setArrowSize(double arrow_size)
{
    if (m_arrowSize != arrow_size) {
        m_arrowSize = arrow_size;
    }
}

void ArrowItem::setDiamondSize(double diamond_size)
{
    if (m_diamondSize != diamond_size) {
        m_diamondSize = diamond_size;
    }
}

void ArrowItem::setStartHead(ArrowItem::Head head)
{
    if (m_startHead != head) {
        m_startHead = head;
    }
}

void ArrowItem::setEndHead(ArrowItem::Head head)
{
    if (m_endHead != head) {
        m_endHead = head;
    }
}

void ArrowItem::setPoints(const QList<QPointF> &points)
{
    m_points = points;
}

QRectF ArrowItem::boundingRect() const
{
    return childrenBoundingRect();
}

void ArrowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);

#if DEBUG_PAINT_ARROW_SHAPE
    painter->setPen(QPen(Qt::blue));
    painter->drawRect(boundingRect());
    painter->setPen(QPen(Qt::red));
    painter->drawPath(shape());
    painter->setPen(QPen(Qt::green));
    if (m_startHeadItem) {
        painter->drawRect(mapRectFromItem(m_startHeadItem, m_startHeadItem->boundingRect()));
    }
    if (m_endHeadItem) {
        painter->drawRect(mapRectFromItem(m_endHeadItem, m_endHeadItem->boundingRect()));
    }
#endif
}

QPainterPath ArrowItem::shape() const
{
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    if (m_shaftItem &&m_shaftItem->path() != QPainterPath()) {
        QPainterPathStroker ps;
        QPen pen = m_shaftItem->pen();
        ps.setCapStyle(pen.capStyle());
        ps.setJoinStyle(pen.joinStyle());
        ps.setMiterLimit(pen.miterLimit());
        // overwrite pen width to make selection more lazy
        ps.setWidth(16.0);
        QPainterPath p = ps.createStroke(m_shaftItem->path());
        path.addPath(p);
    }
    if (m_startHeadItem) {
        path.addRect(mapRectFromItem(m_startHeadItem, m_startHeadItem->boundingRect()));
    }
    if (m_endHeadItem) {
        path.addRect(mapRectFromItem(m_endHeadItem, m_endHeadItem->boundingRect()));
    }
    return path;
}

QPointF ArrowItem::calcPointAtPercent(double percentage) const
{
    return m_shaftItem->path().pointAtPercent(percentage);
}

QLineF ArrowItem::getFirstLineSegment() const
{
    QMT_CHECK(m_points.size() >= 2);
    return QLineF(m_points[0], m_points[1]);
}

QLineF ArrowItem::getLastLineSegment() const
{
    QMT_CHECK(m_points.size() >= 2);
    return QLineF(m_points[m_points.size() - 1], m_points[m_points.size() - 2]);
}

double ArrowItem::getStartHeadLength() const
{
    if (m_startHeadItem) {
        return m_startHeadItem->calcHeadLength();
    }
    return 0.0;
}

double ArrowItem::getEndHeadLength() const
{
    if (m_endHeadItem) {
        return m_endHeadItem->calcHeadLength();
    }
    return 0.0;
}

void ArrowItem::update(const Style *style)
{
    updateShaft(style);
    updateHead(&m_startHeadItem, m_startHead, style);
    updateHead(&m_endHeadItem, m_endHead, style);
    updateGeometry();
}

void ArrowItem::updateShaft(const Style *style)
{
    QMT_CHECK(m_shaftItem);

    QPen pen(style->getLinePen());
    if (m_shaft == SHAFT_DASHED) {
        pen.setDashPattern(QVector<qreal>() << (4.0 / pen.widthF()) << (4.0 / pen.widthF()));
    }
    m_shaftItem->setPen(pen);
}

void ArrowItem::updateHead(GraphicsHeadItem **head_item, Head head, const Style *style)
{
    if (head == HEAD_NONE) {
        if (*head_item) {
            if ((*head_item)->scene()) {
                (*head_item)->scene()->removeItem(*head_item);
            }
            delete *head_item;
            *head_item = 0;
        }
        return;
    }
    if (!*head_item) {
        *head_item = new GraphicsHeadItem(this);
    }
    (*head_item)->setArrowSize(m_arrowSize);
    (*head_item)->setDiamondSize(m_diamondSize);
    (*head_item)->setHead(head);
    (*head_item)->update(style);
}

void ArrowItem::updateHeadGeometry(GraphicsHeadItem **head_item, const QPointF &pos, const QPointF &other_pos)
{
    if (!*head_item) {
        return;
    }

    (*head_item)->setPos(pos);

    QVector2D direction_vector(pos - other_pos);
    direction_vector.normalize();
    double angle = qAcos(direction_vector.x()) * 180.0 / 3.1415926535;
    if (direction_vector.y() > 0.0) {
        angle = -angle;
    }
    (*head_item)->setRotation(-angle);
}

void ArrowItem::updateGeometry()
{
    QMT_CHECK(m_points.size() >= 2);
    QMT_CHECK(m_shaftItem);

    prepareGeometryChange();

    QPainterPath path;

    if (m_startHeadItem) {
        QVector2D start_direction_vector(m_points[1] - m_points[0]);
        start_direction_vector.normalize();
        start_direction_vector *= m_startHeadItem->calcHeadLength();
        path.moveTo(m_points[0] + start_direction_vector.toPointF());
    } else {
        path.moveTo(m_points[0]);
    }

    for (int i = 1; i < m_points.size() - 1; ++i) {
        path.lineTo(m_points.at(i));
    }

    if (m_endHeadItem) {
        QVector2D end_direction_vector(m_points[m_points.size() - 1] - m_points[m_points.size() - 2]);
        end_direction_vector.normalize();
        end_direction_vector *= m_endHeadItem->calcHeadLength();
        path.lineTo(m_points[m_points.size() - 1] - end_direction_vector.toPointF());
    } else {
        path.lineTo(m_points[m_points.size() - 1]);
    }

    m_shaftItem->setPath(path);

    updateHeadGeometry(&m_startHeadItem, m_points[0], m_points[1]);
    updateHeadGeometry(&m_endHeadItem, m_points[m_points.size() - 1], m_points[m_points.size() - 2]);
}

}
