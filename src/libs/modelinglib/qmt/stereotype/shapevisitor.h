// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
