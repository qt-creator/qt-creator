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
