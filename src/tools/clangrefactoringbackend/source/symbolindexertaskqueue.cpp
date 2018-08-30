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

#include "symbolindexertaskqueue.h"

#include <symbolindexertaskschedulerinterface.h>

#include <utils/algorithm.h>

namespace ClangBackEnd {

void SymbolIndexerTaskQueue::addOrUpdateTasks(std::vector<SymbolIndexerTask> &&tasks)
{
    auto merge = [] (SymbolIndexerTask &&first, SymbolIndexerTask &&second) {
        first.callable = std::move(second.callable);

        return std::move(first);
    };

    m_tasks = Utils::setUnionMerge<std::vector<SymbolIndexerTask>>(tasks, m_tasks, merge);
}

void SymbolIndexerTaskQueue::removeTasks(const std::vector<int> &projectPartIds)
{
    auto shouldBeRemoved = [&] (const SymbolIndexerTask& task) {
        return std::binary_search(projectPartIds.begin(), projectPartIds.end(), task.projectPartId);
    };

    auto newEnd = std::remove_if(m_tasks.begin(), m_tasks.end(), shouldBeRemoved);

    m_tasks.erase(newEnd, m_tasks.end());
}

const std::vector<SymbolIndexerTask> &SymbolIndexerTaskQueue::tasks() const
{
    return m_tasks;
}

void SymbolIndexerTaskQueue::processTasks()
{
    int taskCount = m_symbolIndexerScheduler.freeSlots();

    auto newEnd = std::prev(m_tasks.end(), std::min<int>(taskCount, int(m_tasks.size())));
    m_symbolIndexerScheduler.addTasks({std::make_move_iterator(newEnd),
                                       std::make_move_iterator(m_tasks.end())});
    m_tasks.erase(newEnd, m_tasks.end());
}

} // namespace ClangBackEnd
