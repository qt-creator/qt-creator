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

#include "iconshape.h"

#include "qmt/stereotype/shapevalue.h"
#include "qmt/stereotype/shape.h"
#include "qmt/stereotype/shapes.h"
#include "qmt/stereotype/shapevisitor.h"

#include <QList>
#include <QPainter>

template<class T>
QList<T *> CloneAll(const QList<T *> &rhs)
{
    QList<T *> result;
    foreach (T *t, rhs)
        result.append(t != 0 ? t->Clone() : 0);
    return result;
}

namespace qmt {

class IconShape::IconShapePrivate
{
public:
    IconShapePrivate() = default;

    IconShapePrivate(const IconShapePrivate &rhs)
        : m_shapes(CloneAll(rhs.m_shapes))
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
            m_shapes = CloneAll(rhs.m_shapes);
        }
        return *this;
    }

    PathShape *activePath();

    QList<IShape *> m_shapes;
};

PathShape *IconShape::IconShapePrivate::activePath()
{
    PathShape *pathShape = 0;
    if (m_shapes.count() > 0)
        pathShape = dynamic_cast<PathShape *>(m_shapes.last());
    if (pathShape == 0) {
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
    foreach (IShape *p, d->m_shapes)
        p->accept(visitor);
}

} // namespace qmt
