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

void SymbolIndexerTaskQueue::removeTasks(const Utils::SmallStringVector &projectPartIds)
{
    std::vector<std::size_t> ids = projectPartNumberIds(projectPartIds);

    auto shouldBeRemoved = [&] (const SymbolIndexerTask& task) {
        return std::binary_search(ids.begin(), ids.end(), task.projectPartId);
    };

    auto newEnd = std::remove_if(m_tasks.begin(), m_tasks.end(), shouldBeRemoved);

    m_tasks.erase(newEnd, m_tasks.end());
}

const std::vector<SymbolIndexerTask> &SymbolIndexerTaskQueue::tasks() const
{
    return m_tasks;
}

std::size_t SymbolIndexerTaskQueue::projectPartNumberId(Utils::SmallStringView projectPartId)
{
    auto found = std::find(m_projectPartIds.begin(), m_projectPartIds.end(), projectPartId);

    if (found != m_projectPartIds.end())
        return std::size_t(std::distance(m_projectPartIds.begin(), found));

    m_projectPartIds.emplace_back(projectPartId);

    return m_projectPartIds.size() - 1;
}

std::vector<std::size_t> SymbolIndexerTaskQueue::projectPartNumberIds(const Utils::SmallStringVector &projectPartIds)
{
    std::vector<std::size_t> ids;
    std::transform(projectPartIds.begin(),
                   projectPartIds.end(),
                   std::back_inserter(ids),
                   [&] (Utils::SmallStringView projectPartId) {
        return projectPartNumberId(projectPartId);
    });

    std::sort(ids.begin(), ids.end());

    return ids;
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
