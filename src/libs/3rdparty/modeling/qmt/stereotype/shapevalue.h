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

#ifndef QMT_SHAPEVALUE_H
#define QMT_SHAPEVALUE_H

#include "qmt/infrastructure/qmt_global.h"

#include <QPointF>
#include <QSizeF>


namespace qmt {

class QMT_EXPORT ShapeValueF {

public:

    enum Origin {
        ORIGIN_SMART,
        ORIGIN_TOP,
        ORIGIN_LEFT = ORIGIN_TOP,
        ORIGIN_TOP_OR_LEFT = ORIGIN_TOP,
        ORIGIN_BOTTOM,
        ORIGIN_RIGHT = ORIGIN_BOTTOM,
        ORIGIN_BOTTOM_OR_RIGHT = ORIGIN_BOTTOM,
        ORIGIN_CENTER
    };

    enum Unit {
        UNIT_ABSOLUTE,
        UNIT_RELATIVE,
        UNIT_SCALED,
        UNIT_PERCENTAGE
    };

public:

    ShapeValueF()
        : m_value(0.0),
          m_unit(UNIT_RELATIVE),
          m_origin(ORIGIN_SMART)
    {
    }

    ShapeValueF(qreal value, Unit unit = UNIT_RELATIVE, Origin origin = ORIGIN_SMART)
        : m_value(value),
          m_unit(unit),
          m_origin(origin)
    {
    }

public:

    qreal getValue() const { return m_value; }

    void setValue(qreal value) { m_value = value; }

    Unit getUnit() const { return m_unit; }

    void setUnit(Unit unit) { m_unit = unit; }

    Origin getOrigin() const { return m_origin; }

    void setOrigin(Origin origin) { m_origin = origin; }

public:

    qreal mapTo(qreal origin, qreal size) const;

    qreal mapScaledTo(qreal scaled_origin, qreal original_size, qreal actual_size) const;

    qreal mapScaledTo(qreal scaled_origin, qreal original_size, qreal base_size, qreal actual_size) const;

private:

    qreal m_value;

    Unit m_unit;

    Origin m_origin;
};


class QMT_EXPORT ShapePointF {
public:

    ShapePointF()
    {
    }

    ShapePointF(const ShapeValueF &x, const ShapeValueF &y)
        : m_x(x),
          m_y(y)
    {
    }

public:

    ShapeValueF getX() const { return m_x; }

    void setX(const ShapeValueF &x) { m_x = x; }

    ShapeValueF getY() const { return m_y; }

    void setY(const ShapeValueF &y) { m_y = y; }

public:

    QPointF mapTo(const QPointF &origin, const QSizeF &size) const;

    QPointF mapScaledTo(const QPointF &scaled_origin, const QSizeF &original_size, const QSizeF &actual_size) const;

    QPointF mapScaledTo(const QPointF &scaled_origin, const QSizeF &original_size, const QSizeF &base_size, const QSizeF &actual_size) const;

private:

    ShapeValueF m_x;

    ShapeValueF m_y;
};


class QMT_EXPORT ShapeSizeF {
public:

    ShapeSizeF()
    {
    }

    ShapeSizeF(const ShapeValueF &width, const ShapeValueF &height)
        : m_width(width),
          m_height(height)
    {
    }

public:

    ShapeValueF getWidth() const { return m_width; }

    void setWidth(const ShapeValueF &width) { m_width = width; }

    ShapeValueF getHeight() const { return m_height; }

    void setHeight(const ShapeValueF &height) { m_height = height; }

public:

    QSizeF mapTo(const QPointF &origin, const QSizeF &size) const;

    QSizeF mapScaledTo(const QPointF &scaled_origin, const QSizeF &original_size, const QSizeF &actual_size) const;

    QSizeF mapScaledTo(const QPointF &scaled_origin, const QSizeF &original_size, const QSizeF &base_size, const QSizeF &actual_size) const;

private:

    ShapeValueF m_width;

    ShapeValueF m_height;
};

} // namespace qmt

#endif // QMT_SHAPEVALUE_H
