// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QString>

#include <atomic>
#include <memory>

namespace Utils {

class ScopedTimerPrivate;

class QTCREATOR_UTILS_EXPORT ScopedTimerData
{
public:
    QString m_message;
    const char *m_fileName = nullptr;
    int m_line = 0;
    std::atomic<int64_t> *m_cumulative = nullptr;
};

class QTCREATOR_UTILS_EXPORT ScopedTimer
{
public:
    ScopedTimer(const ScopedTimerData &data);
    ~ScopedTimer();

private:
    std::unique_ptr<ScopedTimerPrivate> d;
};

} // Utils

// The "message" argument of QTC_SCOPED_TIMER() and QTC_STATIC_SCOPED_TIMER() macros is optional.
// When provided, it should evaluate to QString.

#define QTC_CONCAT_HELPER(x, y) x ## y
#define QTC_CONCAT(x, y) QTC_CONCAT_HELPER(x, y)
#define QTC_SCOPED_TIMER(message) ::Utils::ScopedTimer QTC_CONCAT(_qtc_scoped_timer_, __LINE__)\
({{message}, __FILE__, __LINE__})

// The macro below expands as follows (in one line):
// static std::atomic<int64_t> _qtc_static_scoped_timer___LINE__ = 0;
// ScopedTimer _qtc_scoped_timer___LINE__(__FILE__, __LINE__, &_qtc_static_scoped_timer___LINE__)
#define QTC_STATIC_SCOPED_TIMER(message) static std::atomic<int64_t> \
QTC_CONCAT(_qtc_static_scoped_timer_, __LINE__) = 0; \
::Utils::ScopedTimer QTC_CONCAT(_qtc_scoped_timer_, __LINE__)\
({{message}, __FILE__, __LINE__, &QTC_CONCAT(_qtc_static_scoped_timer_, __LINE__)})
