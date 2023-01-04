// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
