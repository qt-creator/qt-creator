// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "tracing_global.h"
#include <QString>
#include <chrono>

namespace Timeline {
QString TRACING_EXPORT formatTime(
    std::chrono::nanoseconds time,
    std::chrono::nanoseconds referenceTime = std::chrono::nanoseconds::max());
QString TRACING_EXPORT formatDataSize(qint64 bytes);
QString TRACING_EXPORT formatNumber(qint64 value);
QString TRACING_EXPORT formatPercentage(qint64 percent);
QString TRACING_EXPORT formatPixels(qint64 pixels);

}
