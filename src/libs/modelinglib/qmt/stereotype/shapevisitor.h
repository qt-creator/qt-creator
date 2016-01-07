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
    virtual ~ShapeVisitor() { }

    virtual void visitLine(LineShape *shapeLine) = 0;
    virtual void visitRect(RectShape *shapeRect) = 0;
    virtual void visitRoundedRect(RoundedRectShape *shapeRoundedRect) = 0;
    virtual void visitCircle(CircleShape *shapeCircle) = 0;
    virtual void visitEllipse(EllipseShape *shapeEllipse) = 0;
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
    virtual void visitArc(const ArcShape *shapeArc) = 0;
    virtual void visitPath(const PathShape *shapePath) = 0;
};

} // namespace qmt

#endif // QMT_SHAPEVISITOR_H
