// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <cmath>

QT_BEGIN_NAMESPACE
class QPainter;
class QRectF;
QT_END_NAMESPACE

namespace TerminalSolution {

// Rounds a value to a whole device pixel. At fractional coordinates every cell lands on
// a different sub-pixel phase, which seams the characters that should tile.
inline qreal snapToDevicePixel(qreal value, qreal devicePixelRatio)
{
    return std::round(value * devicePixelRatio) / devicePixelRatio;
}

// Paints c into cellRect in the painter's pen color, if it is a box drawing character,
// block element or powerline separator - those must tile. Otherwise false, nothing painted.
bool paintBoxDrawingCharacter(
    QPainter &painter, const QRectF &cellRect, char16_t c, qreal devicePixelRatio);

} // namespace TerminalSolution
