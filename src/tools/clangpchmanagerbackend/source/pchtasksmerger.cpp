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

#include "pchtasksmerger.h"

#include "pchtaskqueueinterface.h"

namespace ClangBackEnd {

void PchTasksMerger::mergeTasks(PchTaskSets &&taskSets,
                                Utils::SmallStringVector && /*toolChainArguments*/)
{
    mergeSystemTasks(taskSets);
    addProjectTasksToQueue(taskSets);
    m_pchTaskQueue.processEntries();
}

void PchTasksMerger::removePchTasks(const ProjectPartIds &projectPartIds)
{
    m_pchTaskQueue.removePchTasks(projectPartIds);
}

template<typename Container, typename Result = std::remove_const_t<std::remove_reference_t<Container>>>
Result merge(Container &&first, Container &&second)
{
    Result result;
    result.reserve(first.size() + second.size());

    std::set_union(std::make_move_iterator(first.begin()),
                   std::make_move_iterator(first.end()),
                   std::make_move_iterator(second.begin()),
                   std::make_move_iterator(second.end()),
                   std::back_inserter(result));

    return result;
}

CompilerMacros PchTasksMerger::mergeMacros(const CompilerMacros &firstCompilerMacros,
                                           const CompilerMacros &secondCompilerMacros)
{
    return merge(firstCompilerMacros, secondCompilerMacros);
}

bool PchTasksMerger::hasDuplicates(const CompilerMacros &compilerMacros)
{
    auto found = std::adjacent_find(compilerMacros.begin(),
                                    compilerMacros.end(),
                                    [](const CompilerMacro &first, const CompilerMacro &second) {
                                        return first.key == second.key;
                                    });

    return found != compilerMacros.end();
}

IncludeSearchPaths mergeIncludeSearchPaths(IncludeSearchPaths &&first, IncludeSearchPaths &&second)
{
    if (first.size() > second.size())
        return merge(first, second);

    return merge(first, second);
}

bool PchTasksMerger::mergePchTasks(PchTask &firstTask, PchTask &secondTask)
{
    if (firstTask.language != secondTask.language || firstTask.isMerged || secondTask.isMerged)
        return false;

    CompilerMacros macros = mergeMacros(firstTask.compilerMacros, secondTask.compilerMacros);

    secondTask.isMerged = !hasDuplicates(macros);


    if (secondTask.isMerged && firstTask.language == secondTask.language) {
        firstTask.projectPartIds = merge(std::move(firstTask.projectPartIds),
                                         std::move(secondTask.projectPartIds));
        firstTask.includes = merge(std::move(firstTask.includes), std::move(secondTask.includes));
        firstTask.sources = merge(std::move(firstTask.sources),
                                      std::move(secondTask.sources));
        firstTask.compilerMacros = std::move(macros);
        firstTask.systemIncludeSearchPaths = mergeIncludeSearchPaths(
            std::move(firstTask.systemIncludeSearchPaths),
            std::move(secondTask.systemIncludeSearchPaths));

        firstTask.languageVersion = std::max(firstTask.languageVersion, secondTask.languageVersion);
        firstTask.languageExtension = firstTask.languageExtension | secondTask.languageExtension;
    }

    return secondTask.isMerged;
}

void PchTasksMerger::mergePchTasks(PchTasks &tasks)
{
    auto begin = tasks.begin();

    while (begin != tasks.end()) {
        begin = std::find_if(begin, tasks.end(), [](const auto &task) { return !task.isMerged; });
        if (begin != tasks.end()) {
            PchTask &baseTask = *begin;
            ++begin;

            std::for_each(begin, tasks.end(), [&](PchTask &currentTask) {
                mergePchTasks(baseTask, currentTask);
            });
        }
    }
}

void PchTasksMerger::addProjectTasksToQueue(PchTaskSets &taskSets)
{
    PchTasks projectTasks;
    projectTasks.reserve(taskSets.size());

    for (PchTaskSet &taskSet : taskSets)
        projectTasks.push_back(std::move(taskSet.project));

    m_pchTaskQueue.addProjectPchTasks(std::move(projectTasks));
}

void PchTasksMerger::mergeSystemTasks(PchTaskSets &taskSets)
{
    PchTasks systemTasks;
    systemTasks.reserve(taskSets.size());

    for (PchTaskSet &taskSet : taskSets)
        systemTasks.push_back(std::move(taskSet.system));

    mergePchTasks(systemTasks);

    systemTasks.erase(std::remove_if(systemTasks.begin(),
                                     systemTasks.end(),
                                     [](const PchTask &task) { return task.isMerged; }),
                      systemTasks.end());

    m_pchTaskQueue.addSystemPchTasks(std::move(systemTasks));
}

} // namespace ClangBackEnd
