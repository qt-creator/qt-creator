// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "barrier.h"

QT_BEGIN_NAMESPACE

namespace Tasking {

// That's cut down qtcassert.{c,h} to avoid the dependency.
#define QT_STRING(cond) qDebug("SOFT ASSERT: \"%s\" in %s: %s", cond,  __FILE__, QT_STRINGIFY(__LINE__))
#define QT_ASSERT(cond, action) if (Q_LIKELY(cond)) {} else { QT_STRING(#cond); action; } do {} while (0)

void Barrier::setLimit(int value)
{
    QT_ASSERT(!isRunning(), return);
    QT_ASSERT(value > 0, return);

    m_limit = value;
}

void Barrier::start()
{
    QT_ASSERT(!isRunning(), return);
    m_current = 0;
    m_result.reset();
}

void Barrier::advance()
{
    // Calling advance on finished is OK
    QT_ASSERT(isRunning() || m_result, return);
    if (!isRunning()) // no-op
        return;
    ++m_current;
    if (m_current == m_limit)
        stopWithResult(DoneResult::Success);
}

void Barrier::stopWithResult(DoneResult result)
{
    // Calling stopWithResult on finished is OK when the same success is passed
    QT_ASSERT(isRunning() || (m_result && *m_result == result), return);
    if (!isRunning()) // no-op
        return;
    m_current = -1;
    m_result = result;
    emit done(result, QPrivateSignal());
}

} // namespace Tasking

QT_END_NAMESPACE
