// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
