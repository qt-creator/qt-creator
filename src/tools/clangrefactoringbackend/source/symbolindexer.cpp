/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "symbolindexer.h"

#include "symbolscollectorinterface.h"
#include "symbolindexertaskqueue.h"

#include <commandlinebuilder.h>
#include <environment.h>

#include <chrono>
#include <iostream>

namespace ClangBackEnd {

using namespace std::chrono;

class Timer
{
public:
    Timer(Utils::SmallStringView name)
        : name(name)
    {}

    void commit()
    {
        auto end = steady_clock::now();
        auto time_difference = duration_cast<milliseconds>(end - begin);
        begin = end;
        std::cerr << name << " " << timePoint++ << ": " << time_difference.count() << "\n";
    }

private:
    Utils::SmallString name;
    time_point<steady_clock> begin{steady_clock::now()};
    int timePoint = 1;
};

SymbolIndexer::SymbolIndexer(SymbolIndexerTaskQueueInterface &symbolIndexerTaskQueue,
                             SymbolStorageInterface &symbolStorage,
                             BuildDependenciesStorageInterface &buildDependenciesStorage,
                             PrecompiledHeaderStorageInterface &precompiledHeaderStorage,
                             ClangPathWatcherInterface &pathWatcher,
                             FilePathCachingInterface &filePathCache,
                             FileStatusCache &fileStatusCache,
                             Sqlite::TransactionInterface &transactionInterface,
                             ProjectPartsStorageInterface &projectPartsStorage,
                             ModifiedTimeCheckerInterface<SourceTimeStamps> &modifiedTimeChecker,
                             const Environment &environment)
    : m_symbolIndexerTaskQueue(symbolIndexerTaskQueue)
    , m_symbolStorage(symbolStorage)
    , m_buildDependencyStorage(buildDependenciesStorage)
    , m_precompiledHeaderStorage(precompiledHeaderStorage)
    , m_pathWatcher(pathWatcher)
    , m_filePathCache(filePathCache)
    , m_fileStatusCache(fileStatusCache)
    , m_transactionInterface(transactionInterface)
    , m_projectPartsStorage(projectPartsStorage)
    , m_modifiedTimeChecker(modifiedTimeChecker)
    , m_environment(environment)
{
    pathWatcher.setNotifier(this);
}

void SymbolIndexer::updateProjectParts(ProjectPartContainers &&projectParts)
{
        for (ProjectPartContainer &projectPart : projectParts)
            updateProjectPart(std::move(projectPart));
}

void SymbolIndexer::updateProjectPart(ProjectPartContainer &&projectPart)
{
    ProjectPartId projectPartId = projectPart.projectPartId;

    std::vector<SymbolIndexerTask> symbolIndexerTask;
    symbolIndexerTask.reserve(projectPart.sourcePathIds.size());
    for (FilePathId sourcePathId : projectPart.sourcePathIds) {
        SourceTimeStamps dependentTimeStamps = m_symbolStorage.fetchIncludedIndexingTimeStamps(
            sourcePathId);

        if (!m_modifiedTimeChecker.isUpToDate(dependentTimeStamps)) {
            auto indexing = [projectPart,
                             sourcePathId,
                             preIncludeSearchPath = m_environment.preIncludeSearchPath(),
                             this](SymbolsCollectorInterface &symbolsCollector) {
                auto collect = [&](const FilePath &pchPath) {
                    using Builder = CommandLineBuilder<ProjectPartContainer, Utils::SmallStringVector>;
                    Builder commandLineBuilder{projectPart,
                                               projectPart.toolChainArguments,
                                               InputFileType::Source,
                                               {},
                                               {},
                                               pchPath,
                                               preIncludeSearchPath};
                    symbolsCollector.setFile(sourcePathId, commandLineBuilder.commandLine);

                    return symbolsCollector.collectSymbols();
                };

                auto store = [&] {
                    Sqlite::ImmediateTransaction transaction{m_transactionInterface};
                    m_symbolStorage.insertOrUpdateIndexingTimeStamps(symbolsCollector.fileStatuses());
                    m_symbolStorage.addSymbolsAndSourceLocations(symbolsCollector.symbols(),
                                                                 symbolsCollector.sourceLocations());
                    transaction.commit();
                };

                const PchPaths pchPaths = m_precompiledHeaderStorage.fetchPrecompiledHeaders(
                    projectPart.projectPartId);

                if (pchPaths.projectPchPath.size() && collect(pchPaths.projectPchPath)) {
                    store();
                } else if (pchPaths.systemPchPath.size() && collect(pchPaths.systemPchPath)) {
                    store();
                } else if (collect({})) {
                    store();
                }
            };

            symbolIndexerTask.emplace_back(sourcePathId, projectPartId, std::move(indexing));
        }
    }

    m_pathWatcher.updateIdPaths(
        {{projectPartId, m_buildDependencyStorage.fetchSources(projectPartId)}});
    m_symbolIndexerTaskQueue.addOrUpdateTasks(std::move(symbolIndexerTask));
    m_symbolIndexerTaskQueue.processEntries();
}

void SymbolIndexer::pathsWithIdsChanged(const ProjectPartIds &) {}

void SymbolIndexer::pathsChanged(const FilePathIds &filePathIds)
{
    FilePathIds dependentSourcePathIds = m_symbolStorage.fetchDependentSourceIds(filePathIds);

    std::vector<SymbolIndexerTask> symbolIndexerTask;
    symbolIndexerTask.reserve(dependentSourcePathIds.size());

    for (FilePathId dependentSourcePathId : dependentSourcePathIds)
        updateChangedPath(dependentSourcePathId, symbolIndexerTask);

    m_symbolIndexerTaskQueue.addOrUpdateTasks(std::move(symbolIndexerTask));
    m_symbolIndexerTaskQueue.processEntries();
}

void SymbolIndexer::updateChangedPath(FilePathId filePathId,
                                      std::vector<SymbolIndexerTask> &symbolIndexerTask)
{
    m_fileStatusCache.update(filePathId);

    Sqlite::DeferredTransaction transaction{m_transactionInterface};
    const Utils::optional<ProjectPartArtefact>
        optionalArtefact = m_projectPartsStorage.fetchProjectPartArtefact(filePathId);
    if (!optionalArtefact)
        return;
    transaction.commit();

    ProjectPartId projectPartId = optionalArtefact->projectPartId;

    SourceTimeStamps dependentTimeStamps = m_symbolStorage.fetchIncludedIndexingTimeStamps(filePathId);

    auto indexing = [optionalArtefact = std::move(optionalArtefact),
                     filePathId,
                     preIncludeSearchPath = m_environment.preIncludeSearchPath(),
                     this](SymbolsCollectorInterface &symbolsCollector) {
        auto collect = [&](const FilePath &pchPath) {
            const ProjectPartArtefact &artefact = *optionalArtefact;

            using Builder = CommandLineBuilder<ProjectPartArtefact, Utils::SmallStringVector>;
            Builder builder{artefact,
                            artefact.toolChainArguments,
                            InputFileType::Source,
                            {},
                            {},
                            pchPath,
                            preIncludeSearchPath};

            symbolsCollector.setFile(filePathId, builder.commandLine);

            return symbolsCollector.collectSymbols();
        };

        auto store = [&] {
            Sqlite::ImmediateTransaction transaction{m_transactionInterface};
            m_symbolStorage.insertOrUpdateIndexingTimeStamps(symbolsCollector.fileStatuses());
            m_symbolStorage.addSymbolsAndSourceLocations(symbolsCollector.symbols(),
                                                         symbolsCollector.sourceLocations());
            transaction.commit();
        };

        const PchPaths pchPaths = m_precompiledHeaderStorage.fetchPrecompiledHeaders(
            optionalArtefact->projectPartId);

        if (pchPaths.projectPchPath.size() && collect(pchPaths.projectPchPath)) {
            store();
        } else if (pchPaths.systemPchPath.size() && collect(pchPaths.systemPchPath)) {
            store();
        } else if (collect({})) {
            store();
        }
    };

    symbolIndexerTask.emplace_back(filePathId, projectPartId, std::move(indexing));
}

bool SymbolIndexer::compilerMacrosOrIncludeSearchPathsAreDifferent(
        const ProjectPartContainer &projectPart,
        const Utils::optional<ProjectPartArtefact> &optionalArtefact) const
{
    if (optionalArtefact) {
        const ProjectPartArtefact &artefact = optionalArtefact.value();
        return projectPart.compilerMacros != artefact.compilerMacros
               || projectPart.systemIncludeSearchPaths != artefact.systemIncludeSearchPaths
               || projectPart.projectIncludeSearchPaths != artefact.projectIncludeSearchPaths;
    }

    return true;
}

FilePathIds SymbolIndexer::filterChangedFiles(const ProjectPartContainer &projectPart) const
{
    FilePathIds ids;
    ids.reserve(projectPart.sourcePathIds.size());

    for (const FilePathId &sourceId : projectPart.sourcePathIds) {
        long long oldLastModified = m_buildDependencyStorage.fetchLowestLastModifiedTime(sourceId);
        long long currentLastModified =  m_fileStatusCache.lastModifiedTime(sourceId);
        if (oldLastModified < currentLastModified)
            ids.push_back(sourceId);
    }

    return ids;
}

FilePathIds SymbolIndexer::updatableFilePathIds(const ProjectPartContainer &projectPart,
                                                const Utils::optional<ProjectPartArtefact> &optionalArtefact) const
{
    if (compilerMacrosOrIncludeSearchPathsAreDifferent(projectPart, optionalArtefact))
        return projectPart.sourcePathIds;

    return filterChangedFiles(projectPart);
}

} // namespace ClangBackEnd
