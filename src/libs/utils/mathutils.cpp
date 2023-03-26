// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mathutils.h"

#include <QtMath>

namespace Utils::MathUtils {

/*!
    Linear interpolation:
    For x = x1 it returns y1.
    For x = x2 it returns y2.
*/
int interpolateLinear(int x, int x1, int x2, int y1, int y2)
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
    return qRound(double(numerator) / denominator);
}

/*!
    Tangential interpolation:
    For x = 0 it returns y1.
    For x = xHalfLife it returns 50 % of the distance between y1 and y2.
    For x = infinity it returns y2.
*/
int interpolateTangential(int x, int xHalfLife, int y1, int y2)
{
    if (x == 0)
        return y1;
    if (y1 == y2)
        return y1;
    const double angle = atan2(double(x), double(xHalfLife));
    const double result = y1 + (y2 - y1) * angle * 2 / M_PI;
    return qRound(result);
}

/*!
    Exponential interpolation:
    For x = 0 it returns y1.
    For x = xHalfLife it returns 50 % of the distance between y1 and y2.
    For x = infinity it returns y2.
*/
int interpolateExponential(int x, int xHalfLife, int y1, int y2)
{
    if (x == 0)
        return y1;
    if (y1 == y2)
        return y1;
    const double exponent = pow(0.5, double(x) / xHalfLife);
    const double result = y1 + (y2 - y1) * (1.0 - exponent);
    return qRound(result);
}

} // namespace Utils::MathUtils
