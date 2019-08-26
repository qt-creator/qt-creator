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

#include <builddependenciesproviderinterface.h>
#include <clangpathwatcherinterface.h>
#include <filecontainerv2.h>
#include <filepathcachinginterface.h>
#include <generatedfilesinterface.h>
#include <projectpartcontainer.h>
#include <set_algorithm.h>
#include <usedmacrofilter.h>

#include <algorithm>
#include <utils/algorithm.h>

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

namespace {

enum class Change { System, Project, No };

Change changedSourceType(SourceEntry sourceEntry, Change oldChange)
{
    switch (sourceEntry.sourceType) {
    case SourceType::SystemInclude:
    case SourceType::TopSystemInclude:
        return Change::System;
    case SourceType::ProjectInclude:
    case SourceType::TopProjectInclude:
        if (oldChange != Change::System)
            return Change::Project;
        break;
    case SourceType::Source:
    case SourceType::UserInclude:
        break;
    }

    return oldChange;
}

FilePathIds existingSources(const FilePathIds &sources, const FilePathIds &generatedFilePathIds)
{
    FilePathIds existingSources;
    existingSources.reserve(sources.size());
    std::set_difference(sources.begin(),
                        sources.end(),
                        generatedFilePathIds.begin(),
                        generatedFilePathIds.end(),
                        std::back_inserter(existingSources));

    return existingSources;
}

} // namespace

ProjectPartsManager::UpToDataProjectParts ProjectPartsManager::update(ProjectPartContainers &&projectsParts)
{
    auto updateSystemProjectParts = filterProjectParts(projectsParts, m_projectParts);

    if (!updateSystemProjectParts.empty()) {
        auto persistentProjectParts = m_projectPartsStorage.fetchProjectParts(
            toProjectPartIds(updateSystemProjectParts));

        if (persistentProjectParts.size() > 0) {
            mergeProjectParts(persistentProjectParts);

            updateSystemProjectParts = filterProjectParts(updateSystemProjectParts,
                                                          persistentProjectParts);
        }

        if (updateSystemProjectParts.size()) {
            m_projectPartsStorage.updateProjectParts(updateSystemProjectParts);

            mergeProjectParts(updateSystemProjectParts);
        }
    }

    auto upToDateProjectParts = filterProjectParts(projectsParts, updateSystemProjectParts);

    auto updates = checkDependeciesAndTime(std::move(upToDateProjectParts),
                                           std::move(updateSystemProjectParts));

    if (updates.updateSystem.size()) {
        m_projectPartsStorage.resetIndexingTimeStamps(updates.updateSystem);
        m_precompiledHeaderStorage.deleteSystemAndProjectPrecompiledHeaders(
            toProjectPartIds(updates.updateSystem));
    }
    if (updates.updateProject.size()) {
        m_projectPartsStorage.resetIndexingTimeStamps(updates.updateProject);
        m_precompiledHeaderStorage.deleteProjectPrecompiledHeaders(
            toProjectPartIds(updates.updateProject));
    }

    return updates;
}

ProjectPartsManagerInterface::UpToDataProjectParts ProjectPartsManager::checkDependeciesAndTime(
    ProjectPartContainers &&upToDateProjectParts, ProjectPartContainers &&orignalUpdateSystemProjectParts)
{
    ProjectPartContainerReferences changeProjectParts;
    changeProjectParts.reserve(upToDateProjectParts.size());

    ProjectPartContainers updateProjectProjectParts;
    updateProjectProjectParts.reserve(upToDateProjectParts.size());

    ProjectPartContainers addedUpToDateSystemProjectParts;
    addedUpToDateSystemProjectParts.reserve(upToDateProjectParts.size());

    FilePathIds generatedFiles = m_generatedFiles.filePathIds();

    std::vector<IdPaths> watchedIdPaths;
    watchedIdPaths.reserve(upToDateProjectParts.size() * 4);

    for (ProjectPartContainer &projectPart : upToDateProjectParts) {
        auto oldSources = m_buildDependenciesProvider.createSourceEntriesFromStorage(
            projectPart.sourcePathIds, projectPart.projectPartId);

        BuildDependency buildDependency = m_buildDependenciesProvider.create(projectPart,
                                                                             Utils::clone(oldSources));

        const auto &newSources = buildDependency.sources;

        Change change = Change::No;

        std::set_symmetric_difference(newSources.begin(),
                                      newSources.end(),
                                      oldSources.begin(),
                                      oldSources.end(),
                                      make_iterator([&](SourceEntry entry) {
                                          change = changedSourceType(entry, change);
                                      }),
                                      [](SourceEntry first, SourceEntry second) {
                                          return std::tie(first.sourceId, first.sourceType)
                                                 < std::tie(second.sourceId, second.sourceType);
                                      });

        switch (change) {
        case Change::Project:
            updateProjectProjectParts.emplace_back(std::move(projectPart));
            projectPart.projectPartId = -1;
            break;
        case Change::System:
            addedUpToDateSystemProjectParts.emplace_back(std::move(projectPart));
            projectPart.projectPartId = -1;
            break;
        case Change::No:
            break;
        }

        if (change == Change::No) {
            Change change = mismatch_collect(
                newSources.begin(),
                newSources.end(),
                oldSources.begin(),
                oldSources.end(),
                Change::No,
                [](SourceEntry first, SourceEntry second) {
                    return first.timeStamp > second.timeStamp;
                },
                [](SourceEntry first, SourceEntry, Change change) {
                    return changedSourceType(first, change);
                });

            switch (change) {
            case Change::Project:
                updateProjectProjectParts.emplace_back(std::move(projectPart));
                projectPart.projectPartId = -1;
                break;
            case Change::System:
                addedUpToDateSystemProjectParts.emplace_back(std::move(projectPart));
                projectPart.projectPartId = -1;
                break;
            case Change::No:
                UsedMacroFilter usedMacroFilter{newSources, {}, {}};

                watchedIdPaths.emplace_back(projectPart.projectPartId,
                                            SourceType::Source,
                                            existingSources(usedMacroFilter.sources, generatedFiles));
                watchedIdPaths.emplace_back(projectPart.projectPartId,
                                            SourceType::UserInclude,
                                            existingSources(usedMacroFilter.userIncludes,
                                                            generatedFiles));
                watchedIdPaths.emplace_back(projectPart.projectPartId,
                                            SourceType::ProjectInclude,
                                            existingSources(usedMacroFilter.projectIncludes,
                                                            generatedFiles));
                watchedIdPaths.emplace_back(projectPart.projectPartId,
                                            SourceType::SystemInclude,
                                            existingSources(usedMacroFilter.systemIncludes,
                                                            generatedFiles));
                break;
            }
        }
    }

    if (watchedIdPaths.size())
        m_clangPathwatcher.updateIdPaths(watchedIdPaths);

    ProjectPartContainers updateSystemProjectParts;
    updateSystemProjectParts.reserve(orignalUpdateSystemProjectParts.size() + addedUpToDateSystemProjectParts.size());

    std::merge(std::make_move_iterator(orignalUpdateSystemProjectParts.begin()),
               std::make_move_iterator(orignalUpdateSystemProjectParts.end()),
               std::make_move_iterator(addedUpToDateSystemProjectParts.begin()),
               std::make_move_iterator(addedUpToDateSystemProjectParts.end()),
               std::back_inserter(updateSystemProjectParts));

    upToDateProjectParts.erase(std::remove_if(upToDateProjectParts.begin(),
                                              upToDateProjectParts.end(),
                                              [](const ProjectPartContainer &projectPart) {
                                                  return !projectPart.projectPartId.isValid();
                                              }),
                               upToDateProjectParts.end());

    return {std::move(upToDateProjectParts),
            std::move(updateSystemProjectParts),
            updateProjectProjectParts};
}

namespace {
ProjectPartContainers removed(ProjectPartContainers &&projectParts,
                              const ProjectPartIds &projectPartIds)
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

    std::set_difference(std::make_move_iterator(projectParts.begin()),
                        std::make_move_iterator(projectParts.end()),
                        projectPartIds.begin(),
                        projectPartIds.end(),
                        std::back_inserter(projectPartsWithoutIds),
                        Compare{});

    return projectPartsWithoutIds;
}
} // namespace

void ProjectPartsManager::remove(const ProjectPartIds &projectPartIds)
{
    m_projectParts = removed(std::move(m_projectParts), projectPartIds);
    m_systemDeferredProjectParts = removed(std::move(m_systemDeferredProjectParts), projectPartIds);
    m_projectDeferredProjectParts = removed(std::move(m_projectDeferredProjectParts), projectPartIds);
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

namespace {
ProjectPartContainers merge(ProjectPartContainers &&newProjectParts,
                            ProjectPartContainers &&oldProjectParts)
{
    ProjectPartContainers mergedProjectParts;
    mergedProjectParts.reserve(newProjectParts.size() + oldProjectParts.size());

    std::set_union(std::make_move_iterator(newProjectParts.begin()),
                   std::make_move_iterator(newProjectParts.end()),
                   std::make_move_iterator(oldProjectParts.begin()),
                   std::make_move_iterator(oldProjectParts.end()),
                   std::back_inserter(mergedProjectParts),
                   [](const ProjectPartContainer &first, const ProjectPartContainer &second) {
                       return first.projectPartId < second.projectPartId;
                   });

    return mergedProjectParts;
}
} // namespace

void ProjectPartsManager::updateDeferred(ProjectPartContainers &&system,
                                         ProjectPartContainers &&project)
{
    m_systemDeferredProjectParts = merge(std::move(system), std::move(m_systemDeferredProjectParts));
    m_projectDeferredProjectParts = merge(std::move(project),
                                          std::move(m_projectDeferredProjectParts));
}

ProjectPartContainers ProjectPartsManager::deferredSystemUpdates()
{
    return std::move(m_systemDeferredProjectParts);
}

ProjectPartContainers ProjectPartsManager::deferredProjectUpdates()
{
    return std::move(m_projectDeferredProjectParts);
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
