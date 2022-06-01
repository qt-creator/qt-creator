/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "tasktimers.h"

#include <utils/qtcassert.h>

#include <QDateTime>

namespace ClangCodeModel::Internal {

Q_LOGGING_CATEGORY(clangdLogTiming, "qtc.clangcodemodel.clangd.timing", QtWarningMsg);

void ClangCodeModel::Internal::TaskTimer::stopTask()
{
    // This can happen due to the RAII mechanism employed with SubtaskTimer.
    // The subtask timers will expire immediately after, so this does not distort
    // the timing data.
    if (m_subtasks > 0) {
        QTC_CHECK(m_timer.isValid());
        m_elapsedMs += m_timer.elapsed();
        m_timer.invalidate();
        m_subtasks = 0;
    }
    m_started = false;
    qCDebug(clangdLogTiming).noquote().nospace() << m_task << ": took " << m_elapsedMs
                                                 << " ms in UI thread";
    m_elapsedMs = 0;
}

void TaskTimer::startSubtask()
{
    // We have some callbacks that are either synchronous or asynchronous, depending on
    // dynamic conditions. In the sync case, we will have nested subtasks, in which case
    // the inner ones must not collect timing data, as their code blocks are already covered.
    if (++m_subtasks > 1)
        return;
    if (!m_started) {
        QTC_ASSERT(m_elapsedMs == 0, m_elapsedMs = 0);
        m_started = true;
        m_finalized = false;
        qCDebug(clangdLogTiming).noquote().nospace() << m_task << ": starting";

        // Used by ThreadedSubtaskTimer to mark the end of the whole highlighting operation
        m_startTimer.restart();
    }
    qCDebug(clangdLogTiming).noquote().nospace() << m_task << ": subtask started at "
            << QDateTime::currentDateTime().time().toString("hh:mm:ss.zzz");
    QTC_CHECK(!m_timer.isValid());
    m_timer.start();
}

void TaskTimer::stopSubtask(bool isFinalizing)
{
    if (m_subtasks == 0) // See stopTask().
        return;
    if (isFinalizing)
        m_finalized = true;
    if (--m_subtasks > 0) // See startSubtask().
        return;
    qCDebug(clangdLogTiming).noquote().nospace() << m_task << ": subtask stopped at "
            << QDateTime::currentDateTime().time().toString("hh:mm:ss.zzz");
    QTC_CHECK(m_timer.isValid());
    m_elapsedMs += m_timer.elapsed();
    m_timer.invalidate();
    if (m_finalized)
        stopTask();
}

ThreadedSubtaskTimer::ThreadedSubtaskTimer(const QString &task, const TaskTimer &taskTimer)
    : m_task(task), m_taskTimer(taskTimer)
{
    qCDebug(clangdLogTiming).noquote().nospace() << m_task << ": starting thread";
    m_timer.start();
}

ThreadedSubtaskTimer::~ThreadedSubtaskTimer()
{
    qCDebug(clangdLogTiming).noquote().nospace() << m_task << ": took " << m_timer.elapsed()
                                                 << " ms in dedicated thread";

    qCDebug(clangdLogTiming).noquote().nospace() << m_task << ": Start to end: "
                                                 << m_taskTimer.startTimer().elapsed() << " ms";
}

} // namespace ClangCodeModel::Internal
