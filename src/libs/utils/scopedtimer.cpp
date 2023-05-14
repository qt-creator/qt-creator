// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scopedtimer.h"

#include <QDebug>
#include <QTime>

#include <chrono>

namespace Utils {

static QString currentTime() { return QTime::currentTime().toString(Qt::ISODateWithMs); }

using namespace std::chrono;

static const char s_scoped[] = "SCOPED TIMER";
static const char s_scopedCumulative[] = "STATIC SCOPED TIMER";

class ScopedTimerPrivate
{
public:
    QString header() const {
        const char *scopedTimerType = m_data.m_cumulative ? s_scopedCumulative : s_scoped;
        const QString prefix = QLatin1String(scopedTimerType) + " [" + currentTime() + "] ";
        const QString infix = m_data.m_message.isEmpty()
            ? QLatin1String(m_data.m_fileName) + ':' + QString::number(m_data.m_line)
            : m_data.m_message;
        return prefix + infix + ' ';
    }

    const ScopedTimerData m_data;
    const time_point<system_clock, nanoseconds> m_start = system_clock::now();
};

ScopedTimer::ScopedTimer(const ScopedTimerData &data)
    : d(new ScopedTimerPrivate{data})
{
    if (d->m_data.m_cumulative)
        return;
    qDebug().noquote().nospace() << d->header() << "started";
}

static int64_t toMs(int64_t ns) { return ns / 1000000; }

ScopedTimer::~ScopedTimer()
{
    const auto elapsed = duration_cast<nanoseconds>(system_clock::now() - d->m_start);
    QString suffix;
    if (d->m_data.m_cumulative) {
        const int64_t nsOld = d->m_data.m_cumulative->fetch_add(elapsed.count());
        const int64_t msOld = toMs(nsOld);
        const int64_t nsNew = nsOld + elapsed.count();
        const int64_t msNew = toMs(nsNew);
        // Always report the first hit, and later, as not to clog the debug output,
        // only at 10ms resolution.
        if (nsOld != 0 && msOld / 10 == msNew / 10)
            return;

        suffix = "cumulative timeout: " + QString::number(msNew) + "ms";
    } else {
        suffix = "stopped with timeout: " + QString::number(toMs(elapsed.count())) + "ms";
    }
    qDebug().noquote().nospace() << d->header() << suffix;
}

} // namespace Utils
