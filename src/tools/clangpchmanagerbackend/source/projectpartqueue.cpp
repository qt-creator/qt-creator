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

#include "projectpartqueue.h"

#include <pchcreatorinterface.h>
#include <precompiledheaderstorageinterface.h>
#include <sqlitetransaction.h>

namespace ClangBackEnd {

void ProjectPartQueue::addProjectParts(V2::ProjectPartContainers &&projectParts)
{
    auto compare = [](const V2::ProjectPartContainer &first, const V2::ProjectPartContainer &second) {
        return first.projectPartId < second.projectPartId;
    };

    V2::ProjectPartContainers mergedProjectParts;
    mergedProjectParts.reserve(m_projectParts.size() + projectParts.size());
    std::set_union(std::make_move_iterator(projectParts.begin()),
                   std::make_move_iterator(projectParts.end()),
                   std::make_move_iterator(m_projectParts.begin()),
                   std::make_move_iterator(m_projectParts.end()),
                   std::back_inserter(mergedProjectParts),
                   compare);

    m_projectParts = std::move(mergedProjectParts);
}

class CompareDifference
{
public:
    bool operator()(const V2::ProjectPartContainer &first, const Utils::SmallString &second)
    {
        return first.projectPartId < second;
    }

    bool operator()(const Utils::SmallString &first, const V2::ProjectPartContainer &second)
    {
        return first < second.projectPartId;
    }
};

void ProjectPartQueue::removeProjectParts(const Utils::SmallStringVector &projectsPartIds)
{
    V2::ProjectPartContainers notToBeRemovedProjectParts;
    notToBeRemovedProjectParts.reserve(m_projectParts.size());
    std::set_difference(std::make_move_iterator(m_projectParts.begin()),
                        std::make_move_iterator(m_projectParts.end()),
                        projectsPartIds.begin(),
                        projectsPartIds.end(),
                        std::back_inserter(notToBeRemovedProjectParts),
                        CompareDifference{});

    m_projectParts = std::move(notToBeRemovedProjectParts);
}

void ProjectPartQueue::processEntries()
{
    uint taskCount = m_taskScheduler.freeSlots();

    auto newEnd = std::prev(m_projectParts.end(), std::min<int>(int(taskCount), int(m_projectParts.size())));
    m_taskScheduler.addTasks(
                createPchTasks({std::make_move_iterator(newEnd),
                                std::make_move_iterator(m_projectParts.end())}));
    m_projectParts.erase(newEnd, m_projectParts.end());
}

const V2::ProjectPartContainers &ProjectPartQueue::projectParts() const
{
    return m_projectParts;
}

std::vector<ProjectPartQueue::Task> ProjectPartQueue::createPchTasks(
        V2::ProjectPartContainers &&projectParts) const
{
    std::vector<Task> tasks;
    tasks.reserve(projectParts.size());

    auto convert = [this] (auto &&projectPart) {
        return [projectPart=std::move(projectPart), this] (PchCreatorInterface &pchCreator) {
            pchCreator.generatePch(projectPart);
            const auto &projectPartPch = pchCreator.projectPartPch();
            Sqlite::ImmediateTransaction transaction(m_transactionsInterface);
            if (projectPartPch.pchPath.empty()) {
                m_precompiledHeaderStorage.deletePrecompiledHeader(projectPartPch.projectPartId);
            } else {
                m_precompiledHeaderStorage.insertPrecompiledHeader(projectPartPch.projectPartId,
                                                                   projectPartPch.pchPath,
                                                                   projectPartPch.lastModified);
            }
            transaction.commit();
        };
    };

    std::transform(std::make_move_iterator(projectParts.begin()),
                   std::make_move_iterator(projectParts.end()),
                   std::back_inserter(tasks),
                   convert);

    return tasks;
}

} // namespace ClangBackEnd
