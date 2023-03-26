// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QString>

namespace ClangCodeModel::Internal {

Q_DECLARE_LOGGING_CATEGORY(clangdLogTiming);

// TODO: Generalize, document interface and move to Utils?
class TaskTimer
{
public:
    TaskTimer(const QString &task) : m_task(task) {}

    void stopTask();
    void startSubtask();
    void stopSubtask(bool isFinalizing);

    QElapsedTimer startTimer() const { return m_startTimer; }

private:
    const QString m_task;
    QElapsedTimer m_timer;
    QElapsedTimer m_startTimer;
    qint64 m_elapsedMs = 0;
    int m_subtasks = 0;
    bool m_started = false;
    bool m_finalized = false;
};

class SubtaskTimer
{
public:
    SubtaskTimer(TaskTimer &timer) : m_timer(timer) { m_timer.startSubtask(); }
    ~SubtaskTimer() { m_timer.stopSubtask(m_isFinalizing); }

protected:
    void makeFinalizing() { m_isFinalizing = true; }

private:
    TaskTimer &m_timer;
    bool m_isFinalizing = false;
};

class FinalizingSubtaskTimer : public SubtaskTimer
{
public:
    FinalizingSubtaskTimer(TaskTimer &timer) : SubtaskTimer(timer) { makeFinalizing(); }
};

class ThreadedSubtaskTimer
{
public:
    ThreadedSubtaskTimer(const QString &task, const TaskTimer &taskTimer);
    ~ThreadedSubtaskTimer();

private:
    const QString m_task;
    QElapsedTimer m_timer;
    const TaskTimer &m_taskTimer;
};

} // namespace ClangCodeModel::Internal
