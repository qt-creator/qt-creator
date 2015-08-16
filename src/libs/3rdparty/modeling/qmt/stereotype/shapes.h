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

#ifndef QMT_SHAPES_H
#define QMT_SHAPES_H

#include "shape.h"
#include "qmt/infrastructure/qmt_global.h"

#include "shapevalue.h"
#include "shapevisitor.h"

#include <QList>


namespace qmt {

class QMT_EXPORT LineShape
        : public IShape
{
public:

    LineShape()
    {
    }

    LineShape(const ShapePointF &pos1, const ShapePointF &pos2)
        : _pos1(pos1),
          _pos2(pos2)
    {
    }

public:

    ShapePointF getPos1() const { return _pos1; }

    ShapePointF getPos2() const { return _pos2; }

public:

    IShape *Clone() const;

public:

    void accept(ShapeVisitor *visitor);

    void accept(ShapeConstVisitor *visitor) const;

private:

    ShapePointF _pos1;
    ShapePointF _pos2;
};


class QMT_EXPORT RectShape
        : public IShape
{
public:

    RectShape()
    {
    }

    RectShape(const ShapePointF &pos, const ShapeSizeF &size)
        : _pos(pos),
          _size(size)
    {
    }

public:

    ShapePointF getPos() const { return _pos; }

    ShapeSizeF getSize() const { return _size; }

public:

    IShape *Clone() const;

public:

    void accept(ShapeVisitor *visitor);

    void accept(ShapeConstVisitor *visitor) const;

private:

    ShapePointF _pos;
    ShapeSizeF _size;
};


class QMT_EXPORT RoundedRectShape
        : public IShape
{
public:

    RoundedRectShape()
    {
    }

    RoundedRectShape(const ShapePointF &pos, const ShapeSizeF &size, const ShapeValueF &radius)
        : _pos(pos),
          _size(size),
          _radius(radius)
    {
    }

public:

    ShapePointF getPos() const { return _pos; }

    ShapeSizeF getSize() const { return _size; }

    ShapeValueF getRadius() const { return _radius; }

public:

    IShape *Clone() const;

public:

    void accept(ShapeVisitor *visitor);

    void accept(ShapeConstVisitor *visitor) const;

private:

    ShapePointF _pos;
    ShapeSizeF _size;
    ShapeValueF _radius;
};


class QMT_EXPORT CircleShape
        : public IShape
{
public:

    CircleShape()
    {
    }

    CircleShape(const ShapePointF &center, const ShapeValueF &radius)
        : _center(center),
          _radius(radius)
    {
    }

public:

    ShapePointF getCenter() const { return _center; }

    ShapeValueF getRadius() const { return _radius; }

public:

    IShape *Clone() const;

public:

    void accept(ShapeVisitor *visitor);

    void accept(ShapeConstVisitor *visitor) const;

private:

    ShapePointF _center;
    ShapeValueF _radius;
};


class QMT_EXPORT EllipseShape
        : public IShape
{
public:
    EllipseShape()
    {
    }

    EllipseShape(const ShapePointF &center, const ShapeSizeF &radius)
        : _center(center),
          _radius(radius)
    {
    }

public:

    ShapePointF getCenter() const { return _center; }

    ShapeSizeF getRadius() const { return _radius; }

public:

    IShape *Clone() const;

public:

    void accept(ShapeVisitor *visitor);

    void accept(ShapeConstVisitor *visitor) const;

private:

    ShapePointF _center;
    ShapeSizeF _radius;
};


class QMT_EXPORT ArcShape
        : public IShape
{
public:
    ArcShape()
    {
    }

    ArcShape(const ShapePointF &center, const ShapeSizeF &radius, qreal start_angle, qreal span_angle)
        : _center(center),
          _radius(radius),
          _start_angle(start_angle),
          _span_angle(span_angle)
    {
    }

public:

    ShapePointF getCenter() const { return _center; }

    ShapeSizeF getRadius() const { return _radius; }

    qreal getStartAngle() const { return _start_angle; }

    qreal getSpanAngle() const { return _span_angle; }

public:

    IShape *Clone() const;

public:

    void accept(ShapeVisitor *visitor);

    void accept(ShapeConstVisitor *visitor) const;

private:

    ShapePointF _center;
    ShapeSizeF _radius;
    qreal _start_angle;
    qreal _span_angle;
};


class QMT_EXPORT PathShape
        : public IShape
{
public:

    enum ElementType {
        TYPE_NONE,
        TYPE_MOVETO,
        TYPE_LINETO,
        TYPE_ARCMOVETO,
        TYPE_ARCTO,
        TYPE_CLOSE
    };

    struct Element {
        explicit Element(ElementType element = TYPE_NONE)
            : _element_type(element),
              _angle1(0.0),
              _angle2(0.0)
        {
        }

        ElementType _element_type;
        ShapePointF _position;
        ShapeSizeF _size;
        qreal _angle1;
        qreal _angle2;
    };

public:
    PathShape();

    ~PathShape();

public:

    QList<Element> getElements() const { return _elements; }

public:

    IShape *Clone() const;

public:

    void accept(ShapeVisitor *visitor);

    void accept(ShapeConstVisitor *visitor) const;

public:

    void moveTo(const ShapePointF &pos);

    void lineTo(const ShapePointF &pos);

    void arcMoveTo(const ShapePointF &center, const ShapeSizeF &radius, qreal angle);

    void arcTo(const ShapePointF &center, const ShapeSizeF &radius, qreal start_angle, qreal sweep_length);

    void close();

private:

    QList<Element> _elements;

};

}

#endif // QMT_SHAPES_H
