/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "processreaper.h"
#include "qtcassert.h"

#include <QCoreApplication>
#include <QProcess>
#include <QThread>
#include <QTimer>

using namespace Utils;

namespace Utils {
namespace Internal {

class Reaper final : public QObject
{
public:
    Reaper(QProcess *p, int timeoutMs);
    ~Reaper() final;

    int timeoutMs() const;
    bool isFinished() const;
    void nextIteration();

private:
    mutable QTimer m_iterationTimer;
    QProcess *m_process = nullptr;
    int m_emergencyCounter = 0;
    QProcess::ProcessState m_lastState = QProcess::NotRunning;
};

Reaper::Reaper(QProcess *p, int timeoutMs) : m_process(p)
{
    ProcessReaper::instance()->m_reapers.append(this);

    m_iterationTimer.setInterval(timeoutMs);
    m_iterationTimer.setSingleShot(true);
    connect(&m_iterationTimer, &QTimer::timeout, this, &Reaper::nextIteration);

    QMetaObject::invokeMethod(this, &Reaper::nextIteration, Qt::QueuedConnection);
}

Reaper::~Reaper()
{
    ProcessReaper::instance()->m_reapers.removeOne(this);
}

int Reaper::timeoutMs() const
{
    const int remaining = m_iterationTimer.remainingTime();
    if (remaining < 0)
        return m_iterationTimer.interval();
    m_iterationTimer.stop();
    return remaining;
}

bool Reaper::isFinished() const
{
    return !m_process;
}

void Reaper::nextIteration()
{
    QProcess::ProcessState state = m_process ? m_process->state() : QProcess::NotRunning;
    if (state == QProcess::NotRunning || m_emergencyCounter > 5) {
        delete m_process;
        m_process = nullptr;
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

} // namespace Internal

ProcessReaper::~ProcessReaper()
{
    while (!m_reapers.isEmpty()) {
        int alreadyWaited = 0;
        QList<Internal::Reaper *> toDelete;

        // push reapers along:
        for (Internal::Reaper *pr : qAsConst(m_reapers)) {
            const int timeoutMs = pr->timeoutMs();
            if (alreadyWaited < timeoutMs) {
                const unsigned long toSleep = static_cast<unsigned long>(timeoutMs - alreadyWaited);
                QThread::msleep(toSleep);
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
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
}

void ProcessReaper::reap(QProcess *process, int timeoutMs)
{
    if (!process)
        return;

    QTC_ASSERT(QThread::currentThread() == process->thread(), return );

    process->disconnect();
    if (process->state() == QProcess::NotRunning) {
        process->deleteLater();
        return;
    } else {
        process->kill();
    }

    // Neither can move object with a parent into a different thread
    // nor reaping the process with a parent makes any sense.
    process->setParent(nullptr);
    if (process->thread() != QCoreApplication::instance()->thread()) {
        process->moveToThread(QCoreApplication::instance()->thread());
        QMetaObject::invokeMethod(process, [process, timeoutMs] {
            reap(process, timeoutMs);
        }); // will be queued
        return;
    }

    new Internal::Reaper(process, timeoutMs);
}

} // namespace Utils

