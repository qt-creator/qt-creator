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
        : m_pos1(pos1),
          m_pos2(pos2)
    {
    }

public:

    ShapePointF pos1() const { return m_pos1; }

    ShapePointF pos2() const { return m_pos2; }

public:

    IShape *Clone() const;

public:

    void accept(ShapeVisitor *visitor);

    void accept(ShapeConstVisitor *visitor) const;

private:

    ShapePointF m_pos1;
    ShapePointF m_pos2;
};


class QMT_EXPORT RectShape
        : public IShape
{
public:

    RectShape()
    {
    }

    RectShape(const ShapePointF &pos, const ShapeSizeF &size)
        : m_pos(pos),
          m_size(size)
    {
    }

public:

    ShapePointF pos() const { return m_pos; }

    ShapeSizeF size() const { return m_size; }

public:

    IShape *Clone() const;

public:

    void accept(ShapeVisitor *visitor);

    void accept(ShapeConstVisitor *visitor) const;

private:

    ShapePointF m_pos;
    ShapeSizeF m_size;
};


class QMT_EXPORT RoundedRectShape
        : public IShape
{
public:

    RoundedRectShape()
    {
    }

    RoundedRectShape(const ShapePointF &pos, const ShapeSizeF &size, const ShapeValueF &radius)
        : m_pos(pos),
          m_size(size),
          m_radius(radius)
    {
    }

public:

    ShapePointF pos() const { return m_pos; }

    ShapeSizeF size() const { return m_size; }

    ShapeValueF radius() const { return m_radius; }

public:

    IShape *Clone() const;

public:

    void accept(ShapeVisitor *visitor);

    void accept(ShapeConstVisitor *visitor) const;

private:

    ShapePointF m_pos;
    ShapeSizeF m_size;
    ShapeValueF m_radius;
};


class QMT_EXPORT CircleShape
        : public IShape
{
public:

    CircleShape()
    {
    }

    CircleShape(const ShapePointF &center, const ShapeValueF &radius)
        : m_center(center),
          m_radius(radius)
    {
    }

public:

    ShapePointF center() const { return m_center; }

    ShapeValueF radius() const { return m_radius; }

public:

    IShape *Clone() const;

public:

    void accept(ShapeVisitor *visitor);

    void accept(ShapeConstVisitor *visitor) const;

private:

    ShapePointF m_center;
    ShapeValueF m_radius;
};


class QMT_EXPORT EllipseShape
        : public IShape
{
public:
    EllipseShape()
    {
    }

    EllipseShape(const ShapePointF &center, const ShapeSizeF &radius)
        : m_center(center),
          m_radius(radius)
    {
    }

public:

    ShapePointF center() const { return m_center; }

    ShapeSizeF radius() const { return m_radius; }

public:

    IShape *Clone() const;

public:

    void accept(ShapeVisitor *visitor);

    void accept(ShapeConstVisitor *visitor) const;

private:

    ShapePointF m_center;
    ShapeSizeF m_radius;
};


class QMT_EXPORT ArcShape
        : public IShape
{
public:
    ArcShape()
    {
    }

    ArcShape(const ShapePointF &center, const ShapeSizeF &radius, qreal startAngle, qreal spanAngle)
        : m_center(center),
          m_radius(radius),
          m_startAngle(startAngle),
          m_spanAngle(spanAngle)
    {
    }

public:

    ShapePointF center() const { return m_center; }

    ShapeSizeF radius() const { return m_radius; }

    qreal startAngle() const { return m_startAngle; }

    qreal spanAngle() const { return m_spanAngle; }

public:

    IShape *Clone() const;

public:

    void accept(ShapeVisitor *visitor);

    void accept(ShapeConstVisitor *visitor) const;

private:

    ShapePointF m_center;
    ShapeSizeF m_radius;
    qreal m_startAngle;
    qreal m_spanAngle;
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
            : m_elementType(element),
              m_angle1(0.0),
              m_angle2(0.0)
        {
        }

        ElementType m_elementType;
        ShapePointF m_position;
        ShapeSizeF m_size;
        qreal m_angle1;
        qreal m_angle2;
    };

public:
    PathShape();

    ~PathShape();

public:

    QList<Element> elements() const { return m_elements; }

public:

    IShape *Clone() const;

public:

    void accept(ShapeVisitor *visitor);

    void accept(ShapeConstVisitor *visitor) const;

public:

    void moveTo(const ShapePointF &pos);

    void lineTo(const ShapePointF &pos);

    void arcMoveTo(const ShapePointF &center, const ShapeSizeF &radius, qreal angle);

    void arcTo(const ShapePointF &center, const ShapeSizeF &radius, qreal startAngle, qreal sweepLength);

    void close();

private:

    QList<Element> m_elements;

};

}

#endif // QMT_SHAPES_H
