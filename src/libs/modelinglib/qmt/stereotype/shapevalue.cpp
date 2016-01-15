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

#include "shapevalue.h"

namespace qmt {

qreal ShapeValueF::mapTo(qreal origin, qreal size) const
{
    return mapScaledTo(origin, size, size, size);
}

qreal ShapeValueF::mapScaledTo(qreal scaledOrigin, qreal originalSize, qreal actualSize) const
{
    return mapScaledTo(scaledOrigin, originalSize, originalSize, actualSize);
}

qreal ShapeValueF::mapScaledTo(qreal scaledOrigin, qreal originalSize, qreal baseSize, qreal actualSize) const
{
    qreal v = 0.0;
    switch (m_unit) {
    case UnitAbsolute:
        v = m_value;
        break;
    case UnitRelative:
        v = originalSize != 0 ? (m_value * baseSize / originalSize) : m_value;
        break;
    case UnitScaled:
        v = originalSize != 0 ? (m_value * actualSize / originalSize) : m_value;
        break;
    case UnitPercentage:
        v = m_value * actualSize;
        break;
    }
    switch (m_origin) {
    case OriginSmart:
        v = scaledOrigin + v;
        break;
    case OriginTopOrLeft:
        v = scaledOrigin + v;
        break;
    case OriginBottomOrRight:
        v = actualSize - v;
        break;
    case OriginCenter:
        v = actualSize * 0.5 + v;
        break;
    }
    return v;
}

QPointF ShapePointF::mapTo(const QPointF &origin, const QSizeF &size) const
{
    qreal x = m_x.mapTo(origin.x(), size.width());
    qreal y = m_y.mapTo(origin.y(), size.height());
    return QPointF(x, y);
}

QPointF ShapePointF::mapScaledTo(const QPointF &scaledOrigin, const QSizeF &originalSize,
                                 const QSizeF &actualSize) const
{
    qreal x = m_x.mapScaledTo(scaledOrigin.x(), originalSize.width(), actualSize.width());
    qreal y = m_y.mapScaledTo(scaledOrigin.y(), originalSize.height(), actualSize.height());
    return QPointF(x, y);
}

QPointF ShapePointF::mapScaledTo(const QPointF &scaledOrigin, const QSizeF &originalSize, const QSizeF &baseSize,
                                 const QSizeF &actualSize) const
{
    qreal x = m_x.mapScaledTo(scaledOrigin.x(), originalSize.width(), baseSize.width(), actualSize.width());
    qreal y = m_y.mapScaledTo(scaledOrigin.y(), originalSize.height(), baseSize.height(), actualSize.height());
    return QPointF(x, y);
}

QSizeF ShapeSizeF::mapTo(const QPointF &origin, const QSizeF &size) const
{
    qreal w = m_width.mapTo(origin.x(), size.width());
    qreal h = m_height.mapTo(origin.y(), size.height());
    return QSizeF(w, h);
}

QSizeF ShapeSizeF::mapScaledTo(const QPointF &scaledOrigin, const QSizeF &originalSize, const QSizeF &actualSize) const
{
    qreal w = m_width.mapScaledTo(scaledOrigin.x(), originalSize.width(), actualSize.width());
    qreal h = m_height.mapScaledTo(scaledOrigin.y(), originalSize.height(), actualSize.height());
    return QSizeF(w, h);
}

QSizeF ShapeSizeF::mapScaledTo(const QPointF &scaledOrigin, const QSizeF &originalSize, const QSizeF &baseSize,
                               const QSizeF &actualSize) const
{
    qreal w = m_width.mapScaledTo(scaledOrigin.x(), originalSize.width(), baseSize.width(), actualSize.width());
    qreal h = m_height.mapScaledTo(scaledOrigin.y(), originalSize.height(), baseSize.height(), actualSize.height());
    return QSizeF(w, h);
}

} // namespace qmt
