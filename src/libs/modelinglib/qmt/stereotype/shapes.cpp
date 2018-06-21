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

#include "shapes.h"

namespace qmt {

IShape *LineShape::clone() const
{
    return new LineShape(*this);
}

void LineShape::accept(ShapeVisitor *visitor)
{
    visitor->visitLine(this);
}

void LineShape::accept(ShapeConstVisitor *visitor) const
{
    visitor->visitLine(this);
}

IShape *RectShape::clone() const
{
    return new RectShape(*this);
}

void RectShape::accept(ShapeVisitor *visitor)
{
    visitor->visitRect(this);
}

void RectShape::accept(ShapeConstVisitor *visitor) const
{
    visitor->visitRect(this);
}

IShape *RoundedRectShape::clone() const
{
    return new RoundedRectShape(*this);
}

void RoundedRectShape::accept(ShapeVisitor *visitor)
{
    visitor->visitRoundedRect(this);
}

void RoundedRectShape::accept(ShapeConstVisitor *visitor) const
{
    visitor->visitRoundedRect(this);
}

IShape *CircleShape::clone() const
{
    return new CircleShape(*this);
}

void CircleShape::accept(ShapeVisitor *visitor)
{
    visitor->visitCircle(this);
}

void CircleShape::accept(ShapeConstVisitor *visitor) const
{
    visitor->visitCircle(this);
}

IShape *EllipseShape::clone() const
{
    return new EllipseShape(*this);
}

void EllipseShape::accept(ShapeVisitor *visitor)
{
    visitor->visitEllipse(this);
}

void EllipseShape::accept(ShapeConstVisitor *visitor) const
{
    visitor->visitEllipse(this);
}

IShape *DiamondShape::clone() const
{
    return new DiamondShape(*this);
}

void DiamondShape::accept(ShapeVisitor *visitor)
{
    visitor->visitDiamond(this);
}

void DiamondShape::accept(ShapeConstVisitor *visitor) const
{
    visitor->visitDiamond(this);
}

IShape *TriangleShape::clone() const
{
    return new TriangleShape(*this);
}

void TriangleShape::accept(ShapeVisitor *visitor)
{
    visitor->visitTriangle(this);
}

void TriangleShape::accept(ShapeConstVisitor *visitor) const
{
    visitor->visitTriangle(this);
}

IShape *ArcShape::clone() const
{
    return new ArcShape(*this);
}

void ArcShape::accept(ShapeVisitor *visitor)
{
    visitor->visitArc(this);
}

void ArcShape::accept(ShapeConstVisitor *visitor) const
{
    visitor->visitArc(this);
}

PathShape::PathShape()
{
}

PathShape::~PathShape()
{
}

IShape *PathShape::clone() const
{
    return new PathShape(*this);
}

void PathShape::accept(ShapeVisitor *visitor)
{
    visitor->visitPath(this);
}

void PathShape::accept(ShapeConstVisitor *visitor) const
{
    visitor->visitPath(this);
}

void PathShape::moveTo(const ShapePointF &pos)
{
    Element element(TypeMoveto);
    element.m_position = pos;
    m_elements.append(element);
}

void PathShape::lineTo(const ShapePointF &pos)
{
    Element element(TypeLineto);
    element.m_position = pos;
    m_elements.append(element);
}

void PathShape::arcMoveTo(const ShapePointF &center, const ShapeSizeF &radius, qreal angle)
{
    Element element(TypeArcmoveto);
    element.m_position = center;
    element.m_size = radius;
    element.m_angle1 = angle;
    m_elements.append(element);
}

void PathShape::arcTo(const ShapePointF &center, const ShapeSizeF &radius, qreal startAngle, qreal sweepLength)
{
    Element element(TypeArcto);
    element.m_position = center;
    element.m_size = radius;
    element.m_angle1 = startAngle;
    element.m_angle2 = sweepLength;
    m_elements.append(element);
}

void PathShape::close()
{
    Element element(TypeClose);
    m_elements.append(element);
}

} // namespace qmt
