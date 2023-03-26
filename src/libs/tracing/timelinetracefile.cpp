// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinetracefile.h"

#include <QIODevice>

namespace Timeline {

TimelineTraceFile::TimelineTraceFile(QObject *parent) : QObject(parent)
{
}

void TimelineTraceFile::setFuture(const QFutureInterface<void> &future)
{
    m_future = future;
    m_future.setProgressRange(MinimumProgress, MaximumProgress);
    m_future.setProgressValue(MinimumProgress);
}

QFutureInterface<void> &TimelineTraceFile::future()
{
    return m_future;
}

bool TimelineTraceFile::isProgressUpdateNeeded() const
{
    return m_future.isProgressUpdateNeeded() || m_future.progressValue() == 0;
}

void TimelineTraceFile::addProgressValue(int progressValue)
{
    m_future.setProgressValue(qMax(static_cast<int>(MinimumProgress),
                                   qMin(m_future.progressValue() + progressValue,
                                        static_cast<int>(MaximumProgress))));
}

void TimelineTraceFile::setDeviceProgress(QIODevice *device)
{
    m_future.setProgressValue(MinimumProgress + device->pos()
                              * (MaximumProgress - MinimumProgress) / device->size());
}

void TimelineTraceFile::fail(const QString &message)
{
    emit error(message);
    m_future.cancel();
}

void TimelineTraceFile::finish()
{
    m_future.reportFinished();
}

bool TimelineTraceFile::isCanceled() const
{
    return m_future.isCanceled();
}

void TimelineTraceFile::setTraceTime(qint64 traceStart, qint64 traceEnd, qint64 measuredTime)
{
    m_traceStart = traceStart;
    m_traceEnd = traceEnd;
    m_measuredTime = measuredTime;
}

} // namespace Timeline
