// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scopedtimer.h"

#include <QByteArray>
#include <QDebug>
#include <QTime>

#include <chrono>

namespace Utils {

static QString currentTime() { return QTime::currentTime().toString(Qt::ISODateWithMs); }

using namespace std::chrono;

class ScopedTimerPrivate
{
public:
    const char *m_fileName = nullptr;
    const int m_line = 0;
    std::atomic<int64_t> *m_cumulative = nullptr;
    const time_point<system_clock, nanoseconds> m_start = system_clock::now();
};

static const char s_scoped[] = "SCOPED TIMER";
static const char s_scopedCumulative[] = "STATIC SCOPED TIMER";

ScopedTimer::ScopedTimer(const char *fileName, int line, std::atomic<int64_t> *cumulative)
    : d(new ScopedTimerPrivate{fileName, line, cumulative})
{
    if (d->m_cumulative)
        return;
    qDebug().noquote().nospace() << s_scoped << " [" << currentTime() << "] in " << d->m_fileName
                                 << ':' << d->m_line << " started";
}

static int64_t toMs(int64_t ns) { return ns / 1000000; }

ScopedTimer::~ScopedTimer()
{
    const auto elapsed = duration_cast<nanoseconds>(system_clock::now() - d->m_start);
    QString suffix;
    if (d->m_cumulative) {
        const int64_t nsOld = d->m_cumulative->fetch_add(elapsed.count());
        const int64_t msOld = toMs(nsOld);
        const int64_t nsNew = nsOld + elapsed.count();
        const int64_t msNew = toMs(nsNew);
        // Always report the first hit, and later, as not to clog the debug output,
        // only at 10ms resolution.
        if (nsOld != 0 && msOld / 10 == msNew / 10)
            return;

        suffix = " cumulative timeout: " + QString::number(msNew) + "ms";
    } else {
        suffix = " stopped with timeout: " + QString::number(toMs(elapsed.count())) + "ms";
    }
    const char *header = d->m_cumulative ? s_scopedCumulative : s_scoped;
    qDebug().noquote().nospace() << header << " [" << currentTime() << "] in " << d->m_fileName
                                 << ':' << d->m_line << suffix;
}

} // namespace Utils
