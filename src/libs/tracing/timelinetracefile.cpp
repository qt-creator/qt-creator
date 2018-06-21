/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "timelinetracefile.h"

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
