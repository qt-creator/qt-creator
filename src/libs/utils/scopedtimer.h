// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <atomic>
#include <memory>

namespace Utils {

class ScopedTimerPrivate;

class QTCREATOR_UTILS_EXPORT ScopedTimer
{
public:
    ScopedTimer(const char *fileName, int line, std::atomic<int64_t> *cumulative = nullptr);
    ~ScopedTimer();

private:
    std::unique_ptr<ScopedTimerPrivate> d;
};

} // Utils

#define QTC_CONCAT_HELPER(x, y) x ## y
#define QTC_CONCAT(x, y) QTC_CONCAT_HELPER(x, y)
#define QTC_SCOPED_TIMER() ::Utils::ScopedTimer QTC_CONCAT(_qtc_scoped_timer_, __LINE__)\
(__FILE__, __LINE__)

// The macro below expands as follows (in one line):
// static std::atomic<int64_t> _qtc_static_scoped_timer___LINE__ = 0;
// ScopedTimer _qtc_scoped_timer___LINE__(__FILE__, __LINE__, &_qtc_static_scoped_timer___LINE__)
#define QTC_STATIC_SCOPED_TIMER() static std::atomic<int64_t> \
QTC_CONCAT(_qtc_static_scoped_timer_, __LINE__) = 0; \
::Utils::ScopedTimer QTC_CONCAT(_qtc_scoped_timer_, __LINE__)\
(__FILE__, __LINE__, &QTC_CONCAT(_qtc_static_scoped_timer_, __LINE__))
