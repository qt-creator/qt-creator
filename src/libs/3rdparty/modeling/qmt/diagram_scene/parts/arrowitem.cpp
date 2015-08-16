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
          _head(ArrowItem::HEAD_NONE),
          _arrow_size(10.0),
          _diamond_size(15.0),
          _arrow_item(0),
          _diamond_item(0)
    {
    }

    ~GraphicsHeadItem()
    {
    }

public:

    void setHead(ArrowItem::Head head)
    {
        if (_head != head) {
            _head = head;
        }
    }

    void setArrowSize(double arrow_size)
    {
        if (_arrow_size != arrow_size) {
            _arrow_size = arrow_size;
        }
    }

    void setDiamondSize(double diamond_size)
    {
        if (_diamond_size != diamond_size) {
            _diamond_size = diamond_size;
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
        if (_arrow_item) {
            painter->drawRect(mapRectFromItem(_arrow_item, _arrow_item->boundingRect()));
        }
        if (_diamond_item) {
            painter->drawRect(mapRectFromItem(_diamond_item, _diamond_item->boundingRect()));
        }
#endif
    }

    QPainterPath shape() const
    {
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        if (_arrow_item) {
            path.addRect(mapRectFromItem(_arrow_item, _arrow_item->boundingRect()));
        }
        if (_diamond_item) {
            path.addRect(mapRectFromItem(_diamond_item, _diamond_item->boundingRect()));
        }
        return path;
    }

public:

    double calcHeadLength() const
    {
        double length = 0.0;
        switch (_head) {
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
        return sqrt(3.0) * 0.5 * _arrow_size;
    }

    double calcDiamondLength() const
    {
        return sqrt(3.0) * _diamond_size;
    }

public:

    void update(const Style *style)
    {
        bool has_arrow = false;
        bool has_diamond = false;
        switch (_head) {
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
            if (!_arrow_item) {
                _arrow_item = new ArrowItem::GraphicsPathItem(this);
            }

            if (_head == ArrowItem::HEAD_OPEN || _head == ArrowItem::HEAD_TRIANGLE) {
                _arrow_item->setPen(style->getLinePen());
                _arrow_item->setBrush(QBrush());
            } else {
                _arrow_item->setPen(style->getLinePen());
                _arrow_item->setBrush(style->getFillBrush());
            }

            QPainterPath path;
            double h = calcArrowLength();
            path.moveTo(-h, -_arrow_size * 0.5);
            path.lineTo(0.0, 0.0);
            path.lineTo(-h, _arrow_size * 0.5);
            if (_head != ArrowItem::HEAD_OPEN) {
                path.closeSubpath();
            }
            if (has_diamond) {
                path.translate(-calcDiamondLength(), 0.0);
            }
            _arrow_item->setPath(path);
        } else if (_arrow_item) {
            _arrow_item->scene()->removeItem(_arrow_item);
            delete _arrow_item;
            _arrow_item = 0;
        }

        if (has_diamond) {
            if (!_diamond_item) {
                _diamond_item = new ArrowItem::GraphicsPathItem(this);
            }

            if (_head == ArrowItem::HEAD_DIAMOND || _head == ArrowItem::HEAD_DIAMOND_FILLED_TRIANGLE) {
                _diamond_item->setPen(style->getLinePen());
                _diamond_item->setBrush(QBrush());
            } else {
                _diamond_item->setPen(style->getLinePen());
                _diamond_item->setBrush(style->getFillBrush());
            }

            QPainterPath path;
            double h = calcDiamondLength();
            path.lineTo(-h * 0.5, -_diamond_size * 0.5);
            path.lineTo(-h, 0.0);
            path.lineTo(-h * 0.5, _diamond_size * 0.5);
            path.closeSubpath();
            _diamond_item->setPath(path);
        } else if (_diamond_item) {
            _diamond_item->scene()->removeItem(_diamond_item);
            delete _diamond_item;
            _diamond_item = 0;
        }
    }

private:

    ArrowItem::Head _head;

    double _arrow_size;

    double _diamond_size;

    ArrowItem::GraphicsPathItem *_arrow_item;

    ArrowItem::GraphicsPathItem *_diamond_item;

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
      _shaft(SHAFT_SOLID),
      _shaft_item(new GraphicsShaftItem(this)),
      _arrow_size(10.0),
      _diamond_size(15.0),
      _start_head(HEAD_NONE),
      _start_head_item(0),
      _end_head(HEAD_NONE),
      _end_head_item(0)
{
}

ArrowItem::ArrowItem(const ArrowItem &rhs, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      _shaft(rhs._shaft),
      _shaft_item(new GraphicsShaftItem(this)),
      _arrow_size(rhs._arrow_size),
      _diamond_size(rhs._diamond_size),
      _start_head(rhs._start_head),
      _start_head_item(0),
      _end_head(rhs._end_head),
      _end_head_item(0)
{
}

ArrowItem::~ArrowItem()
{
}

void ArrowItem::setShaft(ArrowItem::Shaft shaft)
{
    if (_shaft != shaft) {
        _shaft = shaft;
    }
}

void ArrowItem::setArrowSize(double arrow_size)
{
    if (_arrow_size != arrow_size) {
        _arrow_size = arrow_size;
    }
}

void ArrowItem::setDiamondSize(double diamond_size)
{
    if (_diamond_size != diamond_size) {
        _diamond_size = diamond_size;
    }
}

void ArrowItem::setStartHead(ArrowItem::Head head)
{
    if (_start_head != head) {
        _start_head = head;
    }
}

void ArrowItem::setEndHead(ArrowItem::Head head)
{
    if (_end_head != head) {
        _end_head = head;
    }
}

void ArrowItem::setPoints(const QList<QPointF> &points)
{
    _points = points;
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
    if (_start_head_item) {
        painter->drawRect(mapRectFromItem(_start_head_item, _start_head_item->boundingRect()));
    }
    if (_end_head_item) {
        painter->drawRect(mapRectFromItem(_end_head_item, _end_head_item->boundingRect()));
    }
#endif
}

QPainterPath ArrowItem::shape() const
{
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    if (_shaft_item &&_shaft_item->path() != QPainterPath()) {
        QPainterPathStroker ps;
        QPen pen = _shaft_item->pen();
        ps.setCapStyle(pen.capStyle());
        ps.setJoinStyle(pen.joinStyle());
        ps.setMiterLimit(pen.miterLimit());
        // overwrite pen width to make selection more lazy
        ps.setWidth(16.0);
        QPainterPath p = ps.createStroke(_shaft_item->path());
        path.addPath(p);
    }
    if (_start_head_item) {
        path.addRect(mapRectFromItem(_start_head_item, _start_head_item->boundingRect()));
    }
    if (_end_head_item) {
        path.addRect(mapRectFromItem(_end_head_item, _end_head_item->boundingRect()));
    }
    return path;
}

QPointF ArrowItem::calcPointAtPercent(double percentage) const
{
    return _shaft_item->path().pointAtPercent(percentage);
}

QLineF ArrowItem::getFirstLineSegment() const
{
    QMT_CHECK(_points.size() >= 2);
    return QLineF(_points[0], _points[1]);
}

QLineF ArrowItem::getLastLineSegment() const
{
    QMT_CHECK(_points.size() >= 2);
    return QLineF(_points[_points.size() - 1], _points[_points.size() - 2]);
}

double ArrowItem::getStartHeadLength() const
{
    if (_start_head_item) {
        return _start_head_item->calcHeadLength();
    }
    return 0.0;
}

double ArrowItem::getEndHeadLength() const
{
    if (_end_head_item) {
        return _end_head_item->calcHeadLength();
    }
    return 0.0;
}

void ArrowItem::update(const Style *style)
{
    updateShaft(style);
    updateHead(&_start_head_item, _start_head, style);
    updateHead(&_end_head_item, _end_head, style);
    updateGeometry();
}

void ArrowItem::updateShaft(const Style *style)
{
    QMT_CHECK(_shaft_item);

    QPen pen(style->getLinePen());
    if (_shaft == SHAFT_DASHED) {
        pen.setDashPattern(QVector<qreal>() << (4.0 / pen.widthF()) << (4.0 / pen.widthF()));
    }
    _shaft_item->setPen(pen);
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
    (*head_item)->setArrowSize(_arrow_size);
    (*head_item)->setDiamondSize(_diamond_size);
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
    QMT_CHECK(_points.size() >= 2);
    QMT_CHECK(_shaft_item);

    prepareGeometryChange();

    QPainterPath path;

    if (_start_head_item) {
        QVector2D start_direction_vector(_points[1] - _points[0]);
        start_direction_vector.normalize();
        start_direction_vector *= _start_head_item->calcHeadLength();
        path.moveTo(_points[0] + start_direction_vector.toPointF());
    } else {
        path.moveTo(_points[0]);
    }

    for (int i = 1; i < _points.size() - 1; ++i) {
        path.lineTo(_points.at(i));
    }

    if (_end_head_item) {
        QVector2D end_direction_vector(_points[_points.size() - 1] - _points[_points.size() - 2]);
        end_direction_vector.normalize();
        end_direction_vector *= _end_head_item->calcHeadLength();
        path.lineTo(_points[_points.size() - 1] - end_direction_vector.toPointF());
    } else {
        path.lineTo(_points[_points.size() - 1]);
    }

    _shaft_item->setPath(path);

    updateHeadGeometry(&_start_head_item, _points[0], _points[1]);
    updateHeadGeometry(&_end_head_item, _points[_points.size() - 1], _points[_points.size() - 2]);
}

}
