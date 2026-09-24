// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "async.h"

#include <QApplication>
#include <qapplicationstatic.h>

#if defined(__EMSCRIPTEN__) && !QT_CONFIG(thread)
#include <QElapsedTimer>

#include <emscripten.h>
#endif

namespace Utils {

#if defined(__EMSCRIPTEN__) && !QT_CONFIG(thread)

/*!
    Lets the browser run while a long computation is in progress.

    Without thread support asyncRun() runs its callable on the main thread. Call this
    from the inner loop of such a callable to keep the user interface alive: it suspends
    there and resumes once the browser has had its turn. Suspending saves and restores
    the whole call stack, which is expensive enough that this only happens every few
    milliseconds, however often it is called, and never while another caller is
    suspended in it.

    Does nothing where asyncRun() has a thread to run on, which includes a Qt for
    WebAssembly built with thread support.
*/
void asyncYield()
{
    // Saving and restoring the whole call stack is far too expensive to do once per loop
    // iteration. Yielding every 16 ms is enough for a 60 Hz interface to keep up.
    constexpr qint64 yieldIntervalMs = 16;

    // Whatever runs while one caller is suspended here, a second load for example, must
    // not suspend as well: Asyncify, which a Qt without JSPI builds on, can hold only one
    // suspended call stack. It runs on without yielding instead.
    static bool suspended = false;
    if (suspended)
        return;

    static QElapsedTimer sinceLastYield;
    if (!sinceLastYield.isValid())
        sinceLastYield.start();
    else if (sinceLastYield.elapsed() < yieldIntervalMs)
        return;

    suspended = true;
    emscripten_sleep(0);
    suspended = false;
    sinceLastYield.restart();
}

#endif // defined(__EMSCRIPTEN__) && !QT_CONFIG(thread)

static int s_maxThreadCount = INT_MAX;

class AsyncThreadPool : public QThreadPool
{
public:
    AsyncThreadPool(QThread::Priority priority) {
        setThreadPriority(priority);
        setMaxThreadCount(s_maxThreadCount);
        moveToThread(qApp->thread());
    }
};

Q_APPLICATION_STATIC(AsyncThreadPool, s_idle,         QThread::IdlePriority);
Q_APPLICATION_STATIC(AsyncThreadPool, s_lowest,       QThread::LowestPriority);
Q_APPLICATION_STATIC(AsyncThreadPool, s_low,          QThread::LowPriority);
Q_APPLICATION_STATIC(AsyncThreadPool, s_normal,       QThread::NormalPriority);
Q_APPLICATION_STATIC(AsyncThreadPool, s_high,         QThread::HighPriority);
Q_APPLICATION_STATIC(AsyncThreadPool, s_highest,      QThread::HighestPriority);
Q_APPLICATION_STATIC(AsyncThreadPool, s_timeCritical, QThread::TimeCriticalPriority);
Q_APPLICATION_STATIC(AsyncThreadPool, s_inherit,      QThread::InheritPriority);

QThreadPool *asyncThreadPool(QThread::Priority priority)
{
    switch (priority) {
    case QThread::IdlePriority         : return s_idle;
    case QThread::LowestPriority       : return s_lowest;
    case QThread::LowPriority          : return s_low;
    case QThread::NormalPriority       : return s_normal;
    case QThread::HighPriority         : return s_high;
    case QThread::HighestPriority      : return s_highest;
    case QThread::TimeCriticalPriority : return s_timeCritical;
    case QThread::InheritPriority      : return s_inherit;
    }
    return nullptr;
}

} // namespace Utils
