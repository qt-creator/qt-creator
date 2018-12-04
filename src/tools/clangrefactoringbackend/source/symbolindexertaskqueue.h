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

#pragma once

#include "symbolindexertaskqueueinterface.h"
#include "symbolindexertask.h"

#include <filepathid.h>
#include <progresscounter.h>
#include <taskschedulerinterface.h>

#include <utils/algorithm.h>
#include <utils/smallstringvector.h>

#include <functional>
#include <vector>

namespace Sqlite {
class TransactionInterface;
}

namespace ClangBackEnd {

class SymbolsCollectorInterface;
class SymbolStorageInterface;

class SymbolIndexerTaskQueue final : public SymbolIndexerTaskQueueInterface
{
public:
    using Task = SymbolIndexerTask::Callable;

    SymbolIndexerTaskQueue(TaskSchedulerInterface<Task> &symbolIndexerTaskScheduler,
                           ProgressCounter &progressCounter)
        : m_symbolIndexerScheduler(symbolIndexerTaskScheduler),
          m_progressCounter(progressCounter)
    {}

    void addOrUpdateTasks(std::vector<SymbolIndexerTask> &&tasks)
    {
        auto merge = [] (SymbolIndexerTask &&first, SymbolIndexerTask &&second) {
            first.callable = std::move(second.callable);

            return std::move(first);
        };

        const std::size_t oldSize = m_tasks.size();

        m_tasks = Utils::setUnionMerge<std::vector<SymbolIndexerTask>>(tasks, m_tasks, merge);

        m_progressCounter.addTotal(int(m_tasks.size() - oldSize));
    }
    void removeTasks(const std::vector<int> &projectPartIds)
    {
        auto shouldBeRemoved = [&] (const SymbolIndexerTask& task) {
            return std::binary_search(projectPartIds.begin(), projectPartIds.end(), task.projectPartId);
        };

        const std::size_t oldSize = m_tasks.size();

        auto newEnd = std::remove_if(m_tasks.begin(), m_tasks.end(), shouldBeRemoved);

        m_tasks.erase(newEnd, m_tasks.end());

        m_progressCounter.removeTotal(int(oldSize -m_tasks.size()));
    }

    const std::vector<SymbolIndexerTask> &tasks() const
    {
        return m_tasks;
    }

    void processEntries()
    {
        uint taskCount = m_symbolIndexerScheduler.slotUsage().free;

        auto newEnd = std::prev(m_tasks.end(), std::min<int>(int(taskCount), int(m_tasks.size())));
        m_symbolIndexerScheduler.addTasks({std::make_move_iterator(newEnd),
                                           std::make_move_iterator(m_tasks.end())});
        m_tasks.erase(newEnd, m_tasks.end());
    }
    void syncTasks();

private:
    std::vector<Utils::SmallString> m_projectPartIds;
    std::vector<SymbolIndexerTask> m_tasks;
    TaskSchedulerInterface<Task> &m_symbolIndexerScheduler;
    ProgressCounter &m_progressCounter;
};

} // namespace ClangBackEnd
