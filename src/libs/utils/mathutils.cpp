// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mathutils.h"

#include <QtMath>

namespace Utils::MathUtils {

/*!
    Linear interpolation:
    For x = x1 it returns y1.
    For x = x2 it returns y2.
*/
int interpolate(int x, int x1, int x2, int y1, int y2)
{
    if (x1 == x2)
        return y1; // or the middle point between y1 and y2?
    if (y1 == y2)
        return y1;
    if (x == x1)
        return y1;
    if (x == x2)
        return y2;
    const int numerator = (y2 - y1) * x + x2 * y1 - x1 * y2;
    const int denominator = x2 - x1;
    return qRound((double)numerator / denominator);
}

} // namespace Utils::Math
