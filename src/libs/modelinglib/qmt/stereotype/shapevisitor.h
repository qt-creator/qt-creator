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

namespace qmt {

class LineShape;
class RectShape;
class RoundedRectShape;
class CircleShape;
class EllipseShape;
class DiamondShape;
class TriangleShape;
class ArcShape;
class PathShape;

class ShapeVisitor
{
public:
    virtual ~ShapeVisitor() { }

    virtual void visitLine(LineShape *shapeLine) = 0;
    virtual void visitRect(RectShape *shapeRect) = 0;
    virtual void visitRoundedRect(RoundedRectShape *shapeRoundedRect) = 0;
    virtual void visitCircle(CircleShape *shapeCircle) = 0;
    virtual void visitEllipse(EllipseShape *shapeEllipse) = 0;
    virtual void visitDiamond(DiamondShape *shapeDiamond) = 0;
    virtual void visitTriangle(TriangleShape *shapeDiamond) = 0;
    virtual void visitArc(ArcShape *shapeArc) = 0;
    virtual void visitPath(PathShape *shapePath) = 0;
};

class ShapeConstVisitor
{
public:
    virtual ~ShapeConstVisitor() { }

    virtual void visitLine(const LineShape *shapeLine) = 0;
    virtual void visitRect(const RectShape *shapeRect) = 0;
    virtual void visitRoundedRect(const RoundedRectShape *shapeRoundedRect) = 0;
    virtual void visitCircle(const CircleShape *shapeCircle) = 0;
    virtual void visitEllipse(const EllipseShape *shapeEllipse) = 0;
    virtual void visitDiamond(const DiamondShape *shapeDiamond) = 0;
    virtual void visitTriangle(const TriangleShape *shapeDiamond) = 0;
    virtual void visitArc(const ArcShape *shapeArc) = 0;
    virtual void visitPath(const PathShape *shapePath) = 0;
};

} // namespace qmt
