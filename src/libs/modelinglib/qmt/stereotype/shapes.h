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

#pragma once

#include "shape.h"
#include "qmt/infrastructure/qmt_global.h"

#include "shapevalue.h"
#include "shapevisitor.h"

#include <QList>

namespace qmt {

class QMT_EXPORT LineShape : public IShape
{
public:
    LineShape() = default;

    LineShape(const ShapePointF &pos1, const ShapePointF &pos2)
        : m_pos1(pos1),
          m_pos2(pos2)
    {
    }

    ShapePointF pos1() const { return m_pos1; }
    ShapePointF pos2() const { return m_pos2; }

    IShape *clone() const override;
    void accept(ShapeVisitor *visitor) override;
    void accept(ShapeConstVisitor *visitor) const override;

private:
    ShapePointF m_pos1;
    ShapePointF m_pos2;
};

class QMT_EXPORT RectShape : public IShape
{
public:
    RectShape() = default;

    RectShape(const ShapePointF &pos, const ShapeSizeF &size)
        : m_pos(pos),
          m_size(size)
    {
    }

    ShapePointF pos() const { return m_pos; }
    ShapeSizeF size() const { return m_size; }

    IShape *clone() const override;
    void accept(ShapeVisitor *visitor) override;
    void accept(ShapeConstVisitor *visitor) const override;

private:
    ShapePointF m_pos;
    ShapeSizeF m_size;
};

class QMT_EXPORT RoundedRectShape : public IShape
{
public:
    RoundedRectShape() = default;

    RoundedRectShape(const ShapePointF &pos, const ShapeSizeF &size, const ShapeValueF &radius)
        : m_pos(pos),
          m_size(size),
          m_radius(radius)
    {
    }

    ShapePointF pos() const { return m_pos; }
    ShapeSizeF size() const { return m_size; }
    ShapeValueF radius() const { return m_radius; }

    IShape *clone() const override;
    void accept(ShapeVisitor *visitor) override;
    void accept(ShapeConstVisitor *visitor) const override;

private:
    ShapePointF m_pos;
    ShapeSizeF m_size;
    ShapeValueF m_radius;
};

class QMT_EXPORT CircleShape : public IShape
{
public:
    CircleShape() = default;

    CircleShape(const ShapePointF &center, const ShapeValueF &radius)
        : m_center(center),
          m_radius(radius)
    {
    }

    ShapePointF center() const { return m_center; }
    ShapeValueF radius() const { return m_radius; }

    IShape *clone() const override;
    void accept(ShapeVisitor *visitor) override;
    void accept(ShapeConstVisitor *visitor) const override;

private:
    ShapePointF m_center;
    ShapeValueF m_radius;
};

class QMT_EXPORT EllipseShape : public IShape
{
public:
    EllipseShape() = default;

    EllipseShape(const ShapePointF &center, const ShapeSizeF &radius)
        : m_center(center),
          m_radius(radius)
    {
    }

    ShapePointF center() const { return m_center; }
    ShapeSizeF radius() const { return m_radius; }

    IShape *clone() const override;
    void accept(ShapeVisitor *visitor) override;
    void accept(ShapeConstVisitor *visitor) const override;

private:
    ShapePointF m_center;
    ShapeSizeF m_radius;
};

class QMT_EXPORT DiamondShape : public IShape
{
public:
    DiamondShape() = default;

    DiamondShape(const ShapePointF &center, const ShapeSizeF &size, bool filled)
        : m_center(center),
          m_size(size),
          m_filled(filled)
    {
    }

    ShapePointF center() const { return m_center; }
    ShapeSizeF size() const { return m_size; }
    bool filled() const { return m_filled; }

    IShape *clone() const override;
    void accept(ShapeVisitor *visitor) override;
    void accept(ShapeConstVisitor *visitor) const override;

private:
    ShapePointF m_center;
    ShapeSizeF m_size;
    bool m_filled = false;
};

class QMT_EXPORT TriangleShape : public IShape
{
public:
    TriangleShape() = default;

    TriangleShape(const ShapePointF &center, const ShapeSizeF &size, bool filled)
        : m_center(center),
          m_size(size),
          m_filled(filled)
    {
    }

    ShapePointF center() const { return m_center; }
    ShapeSizeF size() const { return m_size; }
    bool filled() const { return m_filled; }

    IShape *clone() const override;
    void accept(ShapeVisitor *visitor) override;
    void accept(ShapeConstVisitor *visitor) const override;

private:
    ShapePointF m_center;
    ShapeSizeF m_size;
    bool m_filled = false;
};

class QMT_EXPORT ArcShape : public IShape
{
public:
    ArcShape() = default;

    ArcShape(const ShapePointF &center, const ShapeSizeF &radius, qreal startAngle, qreal spanAngle)
        : m_center(center),
          m_radius(radius),
          m_startAngle(startAngle),
          m_spanAngle(spanAngle)
    {
    }

    ShapePointF center() const { return m_center; }
    ShapeSizeF radius() const { return m_radius; }
    qreal startAngle() const { return m_startAngle; }
    qreal spanAngle() const { return m_spanAngle; }

    IShape *clone() const override;
    void accept(ShapeVisitor *visitor) override;
    void accept(ShapeConstVisitor *visitor) const override;

private:
    ShapePointF m_center;
    ShapeSizeF m_radius;
    qreal m_startAngle = 0.0;
    qreal m_spanAngle = 0.0;
};

class QMT_EXPORT PathShape : public IShape
{
public:
    enum ElementType {
        TypeNone,
        TypeMoveto,
        TypeLineto,
        TypeArcmoveto,
        TypeArcto,
        TypeClose
    };

    class Element
    {
    public:
        explicit Element(ElementType element = TypeNone)
            : m_elementType(element)
        {
        }

        ElementType m_elementType = TypeNone;
        ShapePointF m_position;
        ShapeSizeF m_size;
        qreal m_angle1 = 0.0;
        qreal m_angle2 = 0.0;
    };

    PathShape();
    ~PathShape() override;

    QList<Element> elements() const { return m_elements; }

    IShape *clone() const override;
    void accept(ShapeVisitor *visitor) override;
    void accept(ShapeConstVisitor *visitor) const override;

    void moveTo(const ShapePointF &pos);
    void lineTo(const ShapePointF &pos);
    void arcMoveTo(const ShapePointF &center, const ShapeSizeF &radius, qreal angle);
    void arcTo(const ShapePointF &center, const ShapeSizeF &radius,
               qreal startAngle, qreal sweepLength);
    void close();

private:
    QList<Element> m_elements;
};

} // namespace qmt
