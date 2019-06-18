/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "projectpartsmanager.h"

#include <projectpartcontainer.h>

#include <algorithm>

namespace ClangBackEnd {

inline namespace Pch {
ProjectPartIds toProjectPartIds(const ProjectPartContainers &projectsParts)
{
    ProjectPartIds ids;
    ids.reserve(projectsParts.size());
    std::transform(projectsParts.begin(),
                   projectsParts.end(),
                   std::back_inserter(ids),
                   [](const auto &projectPart) { return projectPart.projectPartId; });

    return ids;
}

ProjectPartsManager::UpToDataProjectParts ProjectPartsManager::update(ProjectPartContainers &&projectsParts)
{
    auto notUpToDateProjectParts = filterProjectParts(projectsParts, m_projectParts);

    if (notUpToDateProjectParts.empty())
        return {std::move(projectsParts), {}};

    auto persistentProjectParts = m_projectPartsStorage.fetchProjectParts(
        toProjectPartIds(notUpToDateProjectParts));

    if (persistentProjectParts.size() > 0) {
        mergeProjectParts(persistentProjectParts);

        notUpToDateProjectParts = filterProjectParts(notUpToDateProjectParts, persistentProjectParts);

        if (notUpToDateProjectParts.empty())
            return {};
    }

    m_projectPartsStorage.updateProjectParts(notUpToDateProjectParts);
    m_projectPartsStorage.resetIndexingTimeStamps(notUpToDateProjectParts);
    m_precompiledHeaderStorage.deleteProjectPrecompiledHeaders(
        toProjectPartIds(notUpToDateProjectParts));

    mergeProjectParts(notUpToDateProjectParts);

    auto upToDateProjectParts = filterProjectParts(projectsParts, notUpToDateProjectParts);

    return {upToDateProjectParts, notUpToDateProjectParts};
}

void ProjectPartsManager::remove(const ProjectPartIds &projectPartIds)
{
    ProjectPartContainers projectPartsWithoutIds;

    struct Compare
    {
        bool operator()(ProjectPartId first, const ProjectPartContainer &second)
        {
            return first < second.projectPartId;
        }

        bool operator()(ProjectPartId first, const ProjectPartId &second) { return first < second; }

        bool operator()(const ProjectPartContainer &first, const ProjectPartContainer &second)
        {
            return first.projectPartId < second.projectPartId;
        }

        bool operator()(const ProjectPartContainer &first, ProjectPartId second)
        {
            return first.projectPartId < second;
        }
    };

    std::set_difference(std::make_move_iterator(m_projectParts.begin()),
                        std::make_move_iterator(m_projectParts.end()),
                        projectPartIds.begin(),
                        projectPartIds.end(),
                        std::back_inserter(projectPartsWithoutIds),
                        Compare{});

    m_projectParts = std::move(projectPartsWithoutIds);
}

ProjectPartContainers ProjectPartsManager::projects(const ProjectPartIds &projectPartIds) const
{
    ProjectPartContainers projectPartsWithIds;

    struct Compare
    {
        bool operator()(ProjectPartId first, const ProjectPartContainer &second)
        {
            return first < second.projectPartId;
        }

        bool operator()(ProjectPartId first, const ProjectPartId &second) { return first < second; }

        bool operator()(const ProjectPartContainer &first, const ProjectPartContainer &second)
        {
            return first.projectPartId < second.projectPartId;
        }

        bool operator()(const ProjectPartContainer &first, ProjectPartId second)
        {
            return first.projectPartId < second;
        }
    };

    std::set_intersection(m_projectParts.begin(),
                          m_projectParts.end(),
                          projectPartIds.begin(),
                          projectPartIds.end(),
                          std::back_inserter(projectPartsWithIds),
                          Compare{});

    return projectPartsWithIds;
}

void ProjectPartsManager::updateDeferred(const ProjectPartContainers &deferredProjectsParts)
{
    ProjectPartContainerReferences deferredProjectPartPointers;
    deferredProjectPartPointers.reserve(deferredProjectsParts.size());

    std::set_intersection(m_projectParts.begin(),
                          m_projectParts.end(),
                          deferredProjectsParts.begin(),
                          deferredProjectsParts.end(),
                          std::back_inserter(deferredProjectPartPointers),
                          [](const ProjectPartContainer &first, const ProjectPartContainer &second) {
                              return first.projectPartId < second.projectPartId;
                          });

    for (ProjectPartContainer &projectPart : deferredProjectPartPointers)
        projectPart.updateIsDeferred = true;
}

ProjectPartContainers ProjectPartsManager::deferredUpdates()
{
    ProjectPartContainers deferredProjectParts;
    deferredProjectParts.reserve(m_projectParts.size());

    std::copy_if(m_projectParts.cbegin(),
                 m_projectParts.cend(),
                 std::back_inserter(deferredProjectParts),
                 [](const ProjectPartContainer &projectPart) { return projectPart.updateIsDeferred; });

    for (ProjectPartContainer &projectPart : m_projectParts)
        projectPart.updateIsDeferred = false;

    return deferredProjectParts;
}

ProjectPartContainers ProjectPartsManager::filterProjectParts(
    const ProjectPartContainers &projectsParts, const ProjectPartContainers &oldProjectParts)
{
    ProjectPartContainers updatedProjectPartContainers;
    updatedProjectPartContainers.reserve(projectsParts.size());

    std::set_difference(std::make_move_iterator(projectsParts.begin()),
                        std::make_move_iterator(projectsParts.end()),
                        oldProjectParts.begin(),
                        oldProjectParts.end(),
                        std::back_inserter(updatedProjectPartContainers));

    return updatedProjectPartContainers;
}

void ProjectPartsManager::mergeProjectParts(const ProjectPartContainers &projectsParts)
{
    ProjectPartContainers newProjectParts;
    newProjectParts.reserve(m_projectParts.size() + projectsParts.size());

    auto compare = [] (const ProjectPartContainer &first, const ProjectPartContainer &second) {
        return first.projectPartId < second.projectPartId;
    };

    std::set_union(projectsParts.begin(),
                   projectsParts.end(),
                   m_projectParts.begin(),
                   m_projectParts.end(),
                   std::back_inserter(newProjectParts),
                   compare);

    m_projectParts = newProjectParts;
}

const ProjectPartContainers &ProjectPartsManager::projectParts() const
{
    return m_projectParts;
}

} // namespace Pch

} // namespace ClangBackEnd
