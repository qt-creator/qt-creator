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

#ifndef QMT_SHAPEVISITOR_H
#define QMT_SHAPEVISITOR_H

namespace qmt {

class LineShape;
class RectShape;
class RoundedRectShape;
class CircleShape;
class EllipseShape;
class ArcShape;
class PathShape;

class ShapeVisitor
{
public:

    ~ShapeVisitor() { }

public:

    virtual void visitLine(LineShape *shape_line) = 0;

    virtual void visitRect(RectShape *shape_rect) = 0;

    virtual void visitRoundedRect(RoundedRectShape *shape_rounded_rect) = 0;

    virtual void visitCircle(CircleShape *shape_circle) = 0;

    virtual void visitEllipse(EllipseShape *shape_ellipse) = 0;

    virtual void visitArc(ArcShape *shape_arc) = 0;

    virtual void visitPath(PathShape *shape_path) = 0;
};


class ShapeConstVisitor
{
public:

    ~ShapeConstVisitor() { }

public:

    virtual void visitLine(const LineShape *shape_line) = 0;

    virtual void visitRect(const RectShape *shape_rect) = 0;

    virtual void visitRoundedRect(const RoundedRectShape *shape_rounded_rect) = 0;

    virtual void visitCircle(const CircleShape *shape_circle) = 0;

    virtual void visitEllipse(const EllipseShape *shape_ellipse) = 0;

    virtual void visitArc(const ArcShape *shape_arc) = 0;

    virtual void visitPath(const PathShape *shape_path) = 0;
};

}

#endif // QMT_SHAPEVISITOR_H
