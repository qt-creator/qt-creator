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

#include "pchmanagerserver.h"

#include <builddependenciesstorage.h>
#include <filepathcachinginterface.h>
#include <filepathview.h>
#include <pchmanagerclientinterface.h>
#include <pchtaskgeneratorinterface.h>
#include <precompiledheadersupdatedmessage.h>
#include <progressmessage.h>
#include <removegeneratedfilesmessage.h>
#include <removeprojectpartsmessage.h>
#include <updategeneratedfilesmessage.h>
#include <updateprojectpartsmessage.h>

#include <utils/algorithm.h>
#include <utils/smallstring.h>

#include <QApplication>
#include <QDirIterator>

#include <algorithm>

namespace ClangBackEnd {

PchManagerServer::PchManagerServer(ClangPathWatcherInterface &fileSystemWatcher,
                                   PchTaskGeneratorInterface &pchTaskGenerator,
                                   ProjectPartsManagerInterface &projectParts,
                                   GeneratedFilesInterface &generatedFiles,
                                   BuildDependenciesStorageInterface &buildDependenciesStorage,
                                   FilePathCachingInterface &filePathCache)
    : m_fileSystemWatcher(fileSystemWatcher)
    , m_pchTaskGenerator(pchTaskGenerator)
    , m_projectPartsManager(projectParts)
    , m_generatedFiles(generatedFiles)
    , m_buildDependenciesStorage(buildDependenciesStorage)
    , m_filePathCache(filePathCache)
{
    m_fileSystemWatcher.setNotifier(this);
}

void PchManagerServer::end()
{
    QCoreApplication::exit();
}

namespace {
ProjectPartIds toProjectPartIds(const ProjectPartContainers &projectParts)
{
    return Utils::transform<ProjectPartIds>(projectParts, [](const auto &projectPart) {
        return projectPart.projectPartId;
    });
}

FilePaths gatherPathsInIncludePaths(const ProjectPartContainers &projectParts)
{
    FilePaths filePaths;

    for (const ProjectPartContainer &projectPart : projectParts) {
        for (const IncludeSearchPath &searchPath : projectPart.projectIncludeSearchPaths) {
            QDirIterator directoryIterator(QString{searchPath.path},
                                           {"*.h", "*.hpp"},
                                           QDir::Files,
                                           QDirIterator::FollowSymlinks
                                               | QDirIterator::Subdirectories);
            while (directoryIterator.hasNext()) {
                filePaths.emplace_back(directoryIterator.next());
            }
        }
        for (const IncludeSearchPath &searchPath : projectPart.systemIncludeSearchPaths) {
            QDirIterator directoryIterator(QString{searchPath.path},
                                           {"*.h", "*.hpp"},
                                           QDir::Files,
                                           QDirIterator::FollowSymlinks);
            while (directoryIterator.hasNext()) {
                filePaths.emplace_back(directoryIterator.next());
            }
        }
    }

    return filePaths;
}

} // namespace

void PchManagerServer::updateProjectParts(UpdateProjectPartsMessage &&message)
{
    m_filePathCache.populateIfEmpty();

    m_filePathCache.addFilePaths(gatherPathsInIncludePaths(message.projectsParts));

    m_toolChainsArgumentsCache.update(message.projectsParts, message.toolChainArguments);

    auto upToDateProjectParts = m_projectPartsManager.update(message.takeProjectsParts());

    if (m_generatedFiles.isValid()) {
        m_pchTaskGenerator.addProjectParts(std::move(upToDateProjectParts.updateSystem),
                                           message.toolChainArguments.clone());
        m_pchTaskGenerator.addNonSystemProjectParts(std::move(upToDateProjectParts.updateProject),
                                                    std::move(message.toolChainArguments));
    } else  {
        m_projectPartsManager.updateDeferred(std::move(upToDateProjectParts.updateSystem),
                                             std::move(upToDateProjectParts.updateProject));
    }

    client()->precompiledHeadersUpdated(toProjectPartIds(upToDateProjectParts.upToDate));
}

void PchManagerServer::removeProjectParts(RemoveProjectPartsMessage &&message)
{
    m_fileSystemWatcher.removeIds(message.projectsPartIds);

    m_projectPartsManager.remove(message.projectsPartIds);

    m_pchTaskGenerator.removeProjectParts(message.projectsPartIds);

    m_toolChainsArgumentsCache.remove(message.projectsPartIds);
}

namespace {
ProjectPartIds projectPartIds(const ProjectPartContainers &projectParts)
{
    ProjectPartIds ids;
    ids.reserve(projectParts.size());

    std::transform(projectParts.cbegin(),
                   projectParts.cend(),
                   std::back_inserter(ids),
                   [](const ProjectPartContainer &projectPart) { return projectPart.projectPartId; });

    return ids;
}
}

void PchManagerServer::updateGeneratedFiles(UpdateGeneratedFilesMessage &&message)
{
    m_generatedFiles.update(message.takeGeneratedFiles());

    if (m_generatedFiles.isValid()) {
        ProjectPartContainers deferredSystems = m_projectPartsManager.deferredSystemUpdates();
        ArgumentsEntries systemEntries = m_toolChainsArgumentsCache.arguments(
            projectPartIds(deferredSystems));

        for (ArgumentsEntry &entry : systemEntries) {
            m_pchTaskGenerator.addProjectParts(std::move(deferredSystems), std::move(entry.arguments));
        }

        ProjectPartContainers deferredProjects = m_projectPartsManager.deferredProjectUpdates();
        ArgumentsEntries projectEntries = m_toolChainsArgumentsCache.arguments(
            projectPartIds(deferredProjects));

        for (ArgumentsEntry &entry : projectEntries) {
            m_pchTaskGenerator.addNonSystemProjectParts(std::move(deferredProjects),
                                                        std::move(entry.arguments));
        }
    }
}

void PchManagerServer::removeGeneratedFiles(RemoveGeneratedFilesMessage &&message)
{
    m_generatedFiles.remove(message.takeGeneratedFiles());
}

namespace {
struct FilterResults
{
    ProjectPartIds systemIds;
    ProjectPartIds projectIds;
    ProjectPartIds userIds;
};

ProjectPartIds removeIds(const ProjectPartIds &subtrahend, const ProjectPartIds &minuend)
{
    ProjectPartIds difference;
    difference.reserve(subtrahend.size());

    std::set_difference(subtrahend.begin(),
                        subtrahend.end(),
                        minuend.begin(),
                        minuend.end(),
                        std::back_inserter(difference));

    return difference;
}

FilterResults pchProjectPartIds(const std::vector<IdPaths> &idPaths)
{
    ProjectPartIds changedUserProjectPartIds;
    changedUserProjectPartIds.reserve(idPaths.size());

    ProjectPartIds changedSystemPchProjectPartIds;
    changedSystemPchProjectPartIds.reserve(idPaths.size());

    ProjectPartIds changedProjectPchProjectPartIds;
    changedProjectPchProjectPartIds.reserve(idPaths.size());

    for (const IdPaths &idPath : idPaths) {
        switch (idPath.id.sourceType) {
        case SourceType::TopSystemInclude:
        case SourceType::SystemInclude:
            changedSystemPchProjectPartIds.push_back(idPath.id.id);
            break;
        case SourceType::TopProjectInclude:
        case SourceType::ProjectInclude:
            changedProjectPchProjectPartIds.push_back(idPath.id.id);
            break;
        case SourceType::UserInclude:
        case SourceType::Source:
            changedUserProjectPartIds.push_back(idPath.id.id);
            break;
        }
    }

    changedSystemPchProjectPartIds.erase(std::unique(changedSystemPchProjectPartIds.begin(),
                                                     changedSystemPchProjectPartIds.end()),
                                         changedSystemPchProjectPartIds.end());
    changedProjectPchProjectPartIds.erase(std::unique(changedProjectPchProjectPartIds.begin(),
                                                      changedProjectPchProjectPartIds.end()),
                                          changedProjectPchProjectPartIds.end());
    changedUserProjectPartIds.erase(std::unique(changedUserProjectPartIds.begin(),
                                                changedUserProjectPartIds.end()),
                                    changedUserProjectPartIds.end());

    changedProjectPchProjectPartIds = removeIds(changedProjectPchProjectPartIds,
                                                changedSystemPchProjectPartIds);

    changedUserProjectPartIds = removeIds(changedUserProjectPartIds, changedSystemPchProjectPartIds);
    changedUserProjectPartIds = removeIds(changedUserProjectPartIds, changedProjectPchProjectPartIds);

    return {std::move(changedSystemPchProjectPartIds),
            std::move(changedProjectPchProjectPartIds),
            std::move(changedUserProjectPartIds)};
}
} // namespace

void PchManagerServer::pathsWithIdsChanged(const std::vector<IdPaths> &idPaths)
{
    auto changedProjectPartIds = pchProjectPartIds(idPaths);

    addCompleteProjectParts(changedProjectPartIds.systemIds);

    addNonSystemProjectParts(changedProjectPartIds.projectIds);

    client()->precompiledHeadersUpdated(std::move(changedProjectPartIds.userIds));
}

void PchManagerServer::pathsChanged(const FilePathIds &filePathIds)
{
    m_buildDependenciesStorage.insertOrUpdateIndexingTimeStamps(filePathIds, -1);
}

void PchManagerServer::setPchCreationProgress(int progress, int total)
{
    client()->progress({ProgressType::PrecompiledHeader, progress, total});
}

void PchManagerServer::setDependencyCreationProgress(int progress, int total)
{
    client()->progress({ProgressType::DependencyCreation, progress, total});
}

void PchManagerServer::addCompleteProjectParts(const ProjectPartIds &projectPartIds)
{
    ArgumentsEntries entries = m_toolChainsArgumentsCache.arguments(projectPartIds);

    for (ArgumentsEntry &entry : entries) {
        m_pchTaskGenerator.addProjectParts(m_projectPartsManager.projects(entry.ids),
                                           std::move(entry.arguments));
    }
}

void PchManagerServer::addNonSystemProjectParts(const ProjectPartIds &projectPartIds)
{
    ArgumentsEntries entries = m_toolChainsArgumentsCache.arguments(projectPartIds);

    for (ArgumentsEntry &entry : entries) {
        m_pchTaskGenerator.addNonSystemProjectParts(m_projectPartsManager.projects(entry.ids),
                                                    std::move(entry.arguments));
    }
}

} // namespace ClangBackEnd
