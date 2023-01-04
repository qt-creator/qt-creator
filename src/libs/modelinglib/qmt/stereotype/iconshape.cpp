// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iconshape.h"

#include "qmt/stereotype/shapevalue.h"
#include "qmt/stereotype/shape.h"
#include "qmt/stereotype/shapes.h"
#include "qmt/stereotype/shapevisitor.h"

#include <QList>
#include <QPainter>

template<class T>
QList<T *> cloneAll(const QList<T *> &rhs)
{
    QList<T *> result;
    for (T *t : rhs)
        result.append(t ? t->clone() : nullptr);
    return result;
}

namespace qmt {

class IconShape::IconShapePrivate
{
public:
    IconShapePrivate() = default;

    IconShapePrivate(const IconShapePrivate &rhs)
        : m_shapes(cloneAll(rhs.m_shapes))
    {
    }

    ~IconShapePrivate()
    {
        qDeleteAll(m_shapes);
    }

    IconShapePrivate &operator=(const IconShapePrivate &rhs)
    {
        if (this != &rhs) {
            qDeleteAll(m_shapes);
            m_shapes = cloneAll(rhs.m_shapes);
        }
        return *this;
    }

    PathShape *activePath();

    QList<IShape *> m_shapes;
};

PathShape *IconShape::IconShapePrivate::activePath()
{
    PathShape *pathShape = nullptr;
    if (!m_shapes.isEmpty())
        pathShape = dynamic_cast<PathShape *>(m_shapes.last());
    if (!pathShape) {
        pathShape = new PathShape();
        m_shapes.append(pathShape);
    }
    return pathShape;
}

IconShape::IconShape()
    : d(new IconShapePrivate)
{
}

IconShape::IconShape(const IconShape &rhs)
    : d(new IconShapePrivate(*rhs.d))
{
}

IconShape::~IconShape()
{
    delete d;
}

IconShape &IconShape::operator=(const IconShape &rhs)
{
    if (this != &rhs)
        *d = *rhs.d;
    return *this;
}

bool IconShape::isEmpty() const
{
    return d->m_shapes.isEmpty();
}

void IconShape::addLine(const ShapePointF &pos1, const ShapePointF &pos2)
{
    d->m_shapes.append(new LineShape(pos1, pos2));
}

void IconShape::addRect(const ShapePointF &pos, const ShapeSizeF &size)
{
    d->m_shapes.append(new RectShape(pos, size));
}

void IconShape::addRoundedRect(const ShapePointF &pos, const ShapeSizeF &size, const ShapeValueF &radius)
{
    d->m_shapes.append(new RoundedRectShape(pos, size, radius));
}

void IconShape::addCircle(const ShapePointF &center, const ShapeValueF &radius)
{
    d->m_shapes.append(new CircleShape(center, radius));
}

void IconShape::addEllipse(const ShapePointF &center, const ShapeSizeF &radius)
{
    d->m_shapes.append(new EllipseShape(center, radius));
}

void IconShape::addDiamond(const ShapePointF &center, const ShapeSizeF &size, bool filled)
{
    d->m_shapes.append(new DiamondShape(center, size, filled));
}

void IconShape::addTriangle(const ShapePointF &center, const ShapeSizeF &size, bool filled)
{
    d->m_shapes.append(new TriangleShape(center, size, filled));
}

void IconShape::addArc(const ShapePointF &center, const ShapeSizeF &radius, qreal startAngle, qreal spanAngle)
{
    d->m_shapes.append(new ArcShape(center, radius, startAngle, spanAngle));
}

void IconShape::moveTo(const ShapePointF &pos)
{
    d->activePath()->moveTo(pos);
}

void IconShape::lineTo(const ShapePointF &pos)
{
    d->activePath()->lineTo(pos);
}

void IconShape::arcMoveTo(const ShapePointF &center, const ShapeSizeF &radius, qreal angle)
{
    d->activePath()->arcMoveTo(center, radius, angle);
}

void IconShape::arcTo(const ShapePointF &center, const ShapeSizeF &radius, qreal startAngle, qreal sweepLength)
{
    d->activePath()->arcTo(center, radius, startAngle, sweepLength);
}

void IconShape::closePath()
{
    d->activePath()->close();
}

void IconShape::visitShapes(ShapeConstVisitor *visitor) const
{
    for (IShape *p : d->m_shapes)
        p->accept(visitor);
}

} // namespace qmt
