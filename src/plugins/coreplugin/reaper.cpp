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

namespace Core {
namespace Internal {

static ReaperPrivate *d = nullptr;

ProcessReaper::ProcessReaper(QProcess *p, int timeoutMs) : m_process(p)
{
    m_iterationTimer.setInterval(timeoutMs);
    m_iterationTimer.setSingleShot(true);
    connect(&m_iterationTimer, &QTimer::timeout, this, &ProcessReaper::nextIteration);

    QTimer::singleShot(0, this, &ProcessReaper::nextIteration);
    m_futureInterface.reportStarted();
}

void ProcessReaper::nextIteration()
{
    QProcess::ProcessState state = m_process->state();
    if (state == QProcess::NotRunning || m_emergencyCounter > 5) {
        delete m_process;
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
    d = nullptr;
}

} // namespace Internal

namespace Reaper {

void reap(QProcess *process, int timeoutMs)
{
    if (!process)
        return;

    QTC_ASSERT(Internal::d, return);

    auto reaper = new Internal::ProcessReaper(process, timeoutMs);
    QFuture<void> f = reaper->future();

    Internal::d->m_synchronizer.addFuture(f);
    auto watcher = new QFutureWatcher<void>();

    QObject::connect(watcher, &QFutureWatcher<void>::finished, [watcher, reaper]() {
        watcher->deleteLater();

        const QList<QFuture<void>> futures = Utils::filtered(Internal::d->m_synchronizer.futures(),
                                                             [reaper](const QFuture<void> &f) { return reaper->future() != f; });
        for (const QFuture<void> &f : futures)
            Internal::d->m_synchronizer.addFuture(f);

        delete reaper;
    });
    watcher->setFuture(f);
}

} // namespace Reaper
} // namespace Core

