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

#include "shapevalue.h"

namespace qmt {

qreal ShapeValueF::mapTo(qreal origin, qreal size) const
{
    return mapScaledTo(origin, size, size, size);
}

qreal ShapeValueF::mapScaledTo(qreal scaled_origin, qreal original_size, qreal actual_size) const
{
    return mapScaledTo(scaled_origin, original_size, original_size, actual_size);
}

qreal ShapeValueF::mapScaledTo(qreal scaled_origin, qreal original_size, qreal base_size, qreal actual_size) const
{
    qreal v = 0.0;
    switch (_unit) {
    case UNIT_ABSOLUTE:
        v = _value;
        break;
    case UNIT_RELATIVE:
        v = original_size != 0 ? (_value * base_size / original_size) : _value;
        break;
    case UNIT_SCALED:
        v = original_size != 0 ? (_value * actual_size / original_size) : _value;
        break;
    case UNIT_PERCENTAGE:
        v = _value * actual_size;
        break;
    }
    switch (_origin) {
    case ORIGIN_SMART:
        v = scaled_origin + v;
        break;
    case ORIGIN_TOP_OR_LEFT:
        v = scaled_origin + v;
        break;
    case ORIGIN_BOTTOM_OR_RIGHT:
        v = actual_size - v;
        break;
    case ORIGIN_CENTER:
        v = actual_size * 0.5 + v;
        break;
    }
    return v;
}



QPointF ShapePointF::mapTo(const QPointF &origin, const QSizeF &size) const
{
    qreal x = _x.mapTo(origin.x(), size.width());
    qreal y = _y.mapTo(origin.y(), size.height());
    return QPointF(x, y);
}

QPointF ShapePointF::mapScaledTo(const QPointF &scaled_origin, const QSizeF &original_size, const QSizeF &actual_size) const
{
    qreal x = _x.mapScaledTo(scaled_origin.x(), original_size.width(), actual_size.width());
    qreal y = _y.mapScaledTo(scaled_origin.y(), original_size.height(), actual_size.height());
    return QPointF(x, y);
}

QPointF ShapePointF::mapScaledTo(const QPointF &scaled_origin, const QSizeF &original_size, const QSizeF &base_size, const QSizeF &actual_size) const
{
    qreal x = _x.mapScaledTo(scaled_origin.x(), original_size.width(), base_size.width(), actual_size.width());
    qreal y = _y.mapScaledTo(scaled_origin.y(), original_size.height(), base_size.height(), actual_size.height());
    return QPointF(x, y);
}

QSizeF ShapeSizeF::mapTo(const QPointF &origin, const QSizeF &size) const
{
    qreal w = _width.mapTo(origin.x(), size.width());
    qreal h = _height.mapTo(origin.y(), size.height());
    return QSizeF(w, h);
}

QSizeF ShapeSizeF::mapScaledTo(const QPointF &scaled_origin, const QSizeF &original_size, const QSizeF &actual_size) const
{
    qreal w = _width.mapScaledTo(scaled_origin.x(), original_size.width(), actual_size.width());
    qreal h = _height.mapScaledTo(scaled_origin.y(), original_size.height(), actual_size.height());
    return QSizeF(w, h);
}

QSizeF ShapeSizeF::mapScaledTo(const QPointF &scaled_origin, const QSizeF &original_size, const QSizeF &base_size, const QSizeF &actual_size) const
{
    qreal w = _width.mapScaledTo(scaled_origin.x(), original_size.width(), base_size.width(), actual_size.width());
    qreal h = _height.mapScaledTo(scaled_origin.y(), original_size.height(), base_size.height(), actual_size.height());
    return QSizeF(w, h);
}


} // namespace qmt
