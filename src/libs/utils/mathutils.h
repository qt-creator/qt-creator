// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

namespace Utils::MathUtils {

QTCREATOR_UTILS_EXPORT int interpolateLinear(int x, int x1, int x2, int y1, int y2);
QTCREATOR_UTILS_EXPORT int interpolateTangential(int x, int xHalfLife, int y1, int y2);
QTCREATOR_UTILS_EXPORT int interpolateExponential(int x, int xHalfLife, int y1, int y2);

} // namespace Utils::MathUtils
