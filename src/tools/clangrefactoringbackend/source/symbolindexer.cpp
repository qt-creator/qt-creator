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
                             BuildDependenciesStorageInterface &usedMacroAndSourceStorage,
                             ClangPathWatcherInterface &pathWatcher,
                             FilePathCachingInterface &filePathCache,
                             FileStatusCache &fileStatusCache,
                             Sqlite::TransactionInterface &transactionInterface)
    : m_symbolIndexerTaskQueue(symbolIndexerTaskQueue),
      m_symbolStorage(symbolStorage),
      m_buildDependencyStorage(usedMacroAndSourceStorage),
      m_pathWatcher(pathWatcher),
      m_filePathCache(filePathCache),
      m_fileStatusCache(fileStatusCache),
      m_transactionInterface(transactionInterface)
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
    Sqlite::ImmediateTransaction transaction{m_transactionInterface};
    const auto optionalArtefact = m_symbolStorage.fetchProjectPartArtefact(projectPart.projectPartId);
    int projectPartId = m_symbolStorage.insertOrUpdateProjectPart(
        projectPart.projectPartId,
        projectPart.toolChainArguments,
        projectPart.compilerMacros,
        projectPart.systemIncludeSearchPaths,
        projectPart.projectIncludeSearchPaths,
        projectPart.language,
        projectPart.languageVersion,
        projectPart.languageExtension);
    if (optionalArtefact)
        projectPartId = optionalArtefact->projectPartId;
    const Utils::optional<ProjectPartPch> optionalProjectPartPch = m_symbolStorage.fetchPrecompiledHeader(projectPartId);

    FilePathIds sourcePathIds = updatableFilePathIds(projectPart, optionalArtefact);
    transaction.commit();
    if (sourcePathIds.empty())
        return;

    using Builder = CommandLineBuilder<ProjectPartContainer, Utils::SmallStringVector>;
    Builder commandLineBuilder{projectPart,
                               projectPart.toolChainArguments,
                               InputFileType::Source,
                               {},
                               {},
                               optionalProjectPartPch
                                   ? FilePathView{optionalProjectPartPch.value().pchPath}
                                   : FilePathView{}};

    std::vector<SymbolIndexerTask> symbolIndexerTask;
    symbolIndexerTask.reserve(projectPart.sourcePathIds.size());
    for (FilePathId sourcePathId : projectPart.sourcePathIds) {
        auto indexing = [projectPartId,
                         arguments = commandLineBuilder.commandLine,
                         sourcePathId,
                         this](SymbolsCollectorInterface &symbolsCollector) {
            symbolsCollector.setFile(sourcePathId, arguments);

            bool success = symbolsCollector.collectSymbols();

            if (success) {
                Sqlite::ImmediateTransaction transaction{m_transactionInterface};

                m_symbolStorage.addSymbolsAndSourceLocations(symbolsCollector.symbols(),
                                                             symbolsCollector.sourceLocations());

                m_symbolStorage.updateProjectPartSources(projectPartId,
                                                         symbolsCollector.sourceFiles());

                m_buildDependencyStorage.insertOrUpdateUsedMacros(symbolsCollector.usedMacros());

                m_buildDependencyStorage.insertFileStatuses(symbolsCollector.fileStatuses());

                m_buildDependencyStorage.insertOrUpdateSourceDependencies(
                    symbolsCollector.sourceDependencies());

                transaction.commit();
            }
        };

        symbolIndexerTask.emplace_back(sourcePathId, projectPartId, std::move(indexing));
    }

    m_symbolIndexerTaskQueue.addOrUpdateTasks(std::move(symbolIndexerTask));
    m_symbolIndexerTaskQueue.processEntries();
}

void SymbolIndexer::pathsWithIdsChanged(const Utils::SmallStringVector &)
{
}

void SymbolIndexer::pathsChanged(const FilePathIds &filePathIds)
{
    std::vector<SymbolIndexerTask> symbolIndexerTask;
    symbolIndexerTask.reserve(filePathIds.size());

    for (FilePathId filePathId : filePathIds)
        updateChangedPath(filePathId, symbolIndexerTask);

    m_symbolIndexerTaskQueue.addOrUpdateTasks(std::move(symbolIndexerTask));
    m_symbolIndexerTaskQueue.processEntries();
}

void SymbolIndexer::updateChangedPath(FilePathId filePathId,
                                      std::vector<SymbolIndexerTask> &symbolIndexerTask)
{
    m_fileStatusCache.update(filePathId);

    Sqlite::DeferredTransaction transaction{m_transactionInterface};
    const Utils::optional<ProjectPartArtefact> optionalArtefact = m_symbolStorage.fetchProjectPartArtefact(
        filePathId);
    if (!optionalArtefact)
        return;

    const Utils::optional<ProjectPartPch> optionalProjectPartPch = m_symbolStorage.fetchPrecompiledHeader(
        optionalArtefact->projectPartId);
    transaction.commit();

    const ProjectPartArtefact &artefact = optionalArtefact.value();

    auto pchPath = optionalProjectPartPch ? optionalProjectPartPch.value().pchPath : FilePath{};

    CommandLineBuilder<ProjectPartArtefact, Utils::SmallStringVector>
        builder{artefact, artefact.toolChainArguments, InputFileType::Source, {}, {}, pchPath};

    auto indexing = [projectPartId = artefact.projectPartId,
                     arguments = builder.commandLine,
                     filePathId,
                     this](SymbolsCollectorInterface &symbolsCollector) {
        symbolsCollector.setFile(filePathId, arguments);

        bool success = symbolsCollector.collectSymbols();

        if (success) {
            Sqlite::ImmediateTransaction transaction{m_transactionInterface};

            m_symbolStorage.addSymbolsAndSourceLocations(symbolsCollector.symbols(),
                                                         symbolsCollector.sourceLocations());

            m_symbolStorage.updateProjectPartSources(projectPartId, symbolsCollector.sourceFiles());

            m_buildDependencyStorage.insertOrUpdateUsedMacros(symbolsCollector.usedMacros());

            m_buildDependencyStorage.insertFileStatuses(symbolsCollector.fileStatuses());

            m_buildDependencyStorage.insertOrUpdateSourceDependencies(
                symbolsCollector.sourceDependencies());

            transaction.commit();
        }
    };

    symbolIndexerTask.emplace_back(filePathId, optionalArtefact->projectPartId, std::move(indexing));
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
