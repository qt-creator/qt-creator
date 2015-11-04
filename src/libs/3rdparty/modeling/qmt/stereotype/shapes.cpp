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

#include "shapes.h"

namespace qmt {

IShape *LineShape::Clone() const
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

IShape *RectShape::Clone() const
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

IShape *RoundedRectShape::Clone() const
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

IShape *CircleShape::Clone() const
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

IShape *EllipseShape::Clone() const
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

IShape *ArcShape::Clone() const
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

IShape *PathShape::Clone() const
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

}
