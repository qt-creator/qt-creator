/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "reaper.h"
#include "reaper_p.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QThread>

namespace Core {
namespace Internal {

static ReaperPrivate *d = nullptr;

ProcessReaper::ProcessReaper(QProcess *p, int timeoutMs) : m_process(p)
{
    d->m_reapers.append(this);

    m_iterationTimer.setInterval(timeoutMs);
    m_iterationTimer.setSingleShot(true);
    connect(&m_iterationTimer, &QTimer::timeout, this, &ProcessReaper::nextIteration);

    QTimer::singleShot(0, this, &ProcessReaper::nextIteration);
    m_futureInterface.reportStarted();
}

ProcessReaper::~ProcessReaper()
{
    d->m_reapers.removeOne(this);
}

int ProcessReaper::timeoutMs() const
{
    const int remaining = m_iterationTimer.remainingTime();
    if (remaining < 0)
        return m_iterationTimer.interval();
    m_iterationTimer.stop();
    return remaining;
}

bool ProcessReaper::isFinished() const
{
    return !m_process;
}

void ProcessReaper::nextIteration()
{
    QProcess::ProcessState state = m_process ? m_process->state() : QProcess::NotRunning;
    if (state == QProcess::NotRunning || m_emergencyCounter > 5) {
        delete m_process;
        m_process = nullptr;
        m_futureInterface.reportFinished();
        return;
    }

    if (state == QProcess::Starting) {
        if (m_lastState == QProcess::Starting)
            m_process->kill();
    } else if (state == QProcess::Running) {
        if (m_lastState == QProcess::Running)
            m_process->kill();
        else
            m_process->terminate();
    }

    m_lastState = state;
    m_iterationTimer.start();

    ++m_emergencyCounter;
}

ReaperPrivate::ReaperPrivate()
{
    d = this;
}

ReaperPrivate::~ReaperPrivate()
{
    while (!m_reapers.isEmpty()) {
        int alreadyWaited = 0;
        QList<ProcessReaper *> toDelete;

        // push reapers along:
        foreach (ProcessReaper *pr, m_reapers) {
            const int timeoutMs = pr->timeoutMs();
            if (alreadyWaited < timeoutMs) {
                const unsigned long toSleep = static_cast<unsigned long>(timeoutMs - alreadyWaited);
                QThread::msleep(toSleep);
                alreadyWaited += toSleep;
            }

            pr->nextIteration();

            if (pr->isFinished())
                toDelete.append(pr);
        }

        // Clean out reapers that finished in the meantime
        qDeleteAll(toDelete);
        toDelete.clear();
    }

    d = nullptr;
}

} // namespace Internal

namespace Reaper {

void reap(QProcess *process, int timeoutMs)
{
    if (!process)
        return;

    QTC_ASSERT(Internal::d, return);

    new Internal::ProcessReaper(process, timeoutMs);
}

} // namespace Reaper
} // namespace Core

