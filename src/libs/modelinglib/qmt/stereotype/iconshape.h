// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"
#include <QSizeF>

namespace qmt {

class ShapeValueF;
class ShapePointF;
class ShapeSizeF;
class ShapeConstVisitor;

class QMT_EXPORT IconShape
{
    class IconShapePrivate;

public:
    IconShape();
    IconShape(const IconShape &rhs);
    ~IconShape();

    IconShape &operator=(const IconShape &rhs);

    bool isEmpty() const;
    QSizeF size() const;
    void setSize(const QSizeF &size);

    void addLine(const ShapePointF &pos1, const ShapePointF &pos2);
    void addRect(const ShapePointF &pos, const ShapeSizeF &size);
    void addRoundedRect(const ShapePointF &pos, const ShapeSizeF &size, const ShapeValueF &radius);
    void addCircle(const ShapePointF &center, const ShapeValueF &radius);
    void addEllipse(const ShapePointF &center, const ShapeSizeF &radius);
    void addDiamond(const ShapePointF &center, const ShapeSizeF &size, bool filled);
    void addTriangle(const ShapePointF &center, const ShapeSizeF &size, bool filled);
    void addArc(const ShapePointF &center, const ShapeSizeF &radius,
                qreal startAngle, qreal spanAngle);

    void moveTo(const ShapePointF &pos);
    void lineTo(const ShapePointF &pos);
    void arcMoveTo(const ShapePointF &center, const ShapeSizeF &radius, qreal angle);
    void arcTo(const ShapePointF &center, const ShapeSizeF &radius,
               qreal startAngle, qreal sweepLength);
    void closePath();

    void visitShapes(ShapeConstVisitor *visitor) const;

private:
    IconShapePrivate *d;
};

} // namespace qmt
