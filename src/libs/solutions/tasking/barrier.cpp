// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "barrier.h"

namespace Tasking {

// That's cut down qtcassert.{c,h} to avoid the dependency.
#define QTC_STRINGIFY_HELPER(x) #x
#define QTC_STRINGIFY(x) QTC_STRINGIFY_HELPER(x)
#define QTC_STRING(cond) qDebug("SOFT ASSERT: \"%s\" in %s: %s", cond,  __FILE__, QTC_STRINGIFY(__LINE__))
#define QTC_ASSERT(cond, action) if (Q_LIKELY(cond)) {} else { QTC_STRING(#cond); action; } do {} while (0)
#define QTC_CHECK(cond) if (cond) {} else { QTC_STRING(#cond); } do {} while (0)

void Barrier::setLimit(int value)
{
    QTC_ASSERT(!isRunning(), return);
    QTC_ASSERT(value > 0, return);

    m_limit = value;
}

void Barrier::start()
{
    QTC_ASSERT(!isRunning(), return);
    m_current = 0;
    m_result = {};
}

void Barrier::advance()
{
    // Calling advance on finished is OK
    QTC_ASSERT(isRunning() || m_result, return);
    if (!isRunning()) // no-op
        return;
    ++m_current;
    if (m_current == m_limit)
        stopWithResult(true);
}

void Barrier::stopWithResult(bool success)
{
    // Calling stopWithResult on finished is OK when the same success is passed
    QTC_ASSERT(isRunning() || (m_result && *m_result == success), return);
    if (!isRunning()) // no-op
        return;
    m_current = -1;
    m_result = success;
    emit done(success);
}

} // namespace Tasking
