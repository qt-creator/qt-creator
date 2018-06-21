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

namespace ClangBackEnd {

SymbolIndexer::SymbolIndexer(SymbolsCollectorInterface &symbolsCollector,
                             SymbolStorageInterface &symbolStorage,
                             ClangPathWatcherInterface &pathWatcher,
                             FilePathCachingInterface &filePathCache,
                             FileStatusCache &fileStatusCache,
                             Sqlite::TransactionInterface &transactionInterface)
    : m_symbolsCollector(symbolsCollector),
      m_symbolStorage(symbolStorage),
      m_pathWatcher(pathWatcher),
      m_filePathCache(filePathCache),
      m_fileStatusCache(fileStatusCache),
      m_transactionInterface(transactionInterface)
{
    pathWatcher.setNotifier(this);
}

void SymbolIndexer::updateProjectParts(V2::ProjectPartContainers &&projectParts, V2::FileContainers &&generatedFiles)
{
        for (V2::ProjectPartContainer &projectPart : projectParts)
            updateProjectPart(std::move(projectPart), generatedFiles);
}

void SymbolIndexer::updateProjectPart(V2::ProjectPartContainer &&projectPart,
                                      const V2::FileContainers &generatedFiles)
{
    m_symbolsCollector.clear();

    const Utils::optional<ProjectPartArtefact> optionalArtefact = m_symbolStorage.fetchProjectPartArtefact(
                projectPart.projectPartId);

    FilePathIds sourcePathIds = updatableFilePathIds(projectPart, optionalArtefact);

    if (!sourcePathIds.empty()) {
        m_symbolsCollector.addFiles(projectPart.sourcePathIds,
                                    compilerArguments(projectPart, optionalArtefact));

        m_symbolsCollector.addUnsavedFiles(generatedFiles);

        m_symbolsCollector.collectSymbols();

        Sqlite::ImmediateTransaction transaction{m_transactionInterface};

        m_symbolStorage.addSymbolsAndSourceLocations(m_symbolsCollector.symbols(),
                                                     m_symbolsCollector.sourceLocations());

        m_symbolStorage.insertOrUpdateProjectPart(projectPart.projectPartId,
                                                  projectPart.arguments,
                                                  projectPart.compilerMacros,
                                                  projectPart.includeSearchPaths);
        m_symbolStorage.updateProjectPartSources(projectPart.projectPartId,
                                                 m_symbolsCollector.sourceFiles());

        m_symbolStorage.insertOrUpdateUsedMacros(m_symbolsCollector.usedMacros());

        m_symbolStorage.insertFileStatuses(m_symbolsCollector.fileStatuses());

        m_symbolStorage.insertOrUpdateSourceDependencies(m_symbolsCollector.sourceDependencies());

        transaction.commit();
    }
}

void SymbolIndexer::pathsWithIdsChanged(const Utils::SmallStringVector &)
{
}

void SymbolIndexer::pathsChanged(const FilePathIds &filePathIds)
{
    for (FilePathId filePathId : filePathIds)
        updateChangedPath(filePathId);
}

void SymbolIndexer::updateChangedPath(FilePathId filePathId)
{
    m_symbolsCollector.clear();
    m_fileStatusCache.update(filePathId);

    const Utils::optional<ProjectPartArtefact> optionalArtefact = m_symbolStorage.fetchProjectPartArtefact(filePathId);

    if (optionalArtefact && !optionalArtefact.value().compilerArguments.empty()) {
        const ProjectPartArtefact &artefact = optionalArtefact.value();

        m_symbolsCollector.addFiles({filePathId},
                                    compilerArguments(artefact.compilerArguments, artefact.projectPartId));

        m_symbolsCollector.collectSymbols();

        Sqlite::ImmediateTransaction transaction{m_transactionInterface};

        m_symbolStorage.addSymbolsAndSourceLocations(m_symbolsCollector.symbols(),
                                                     m_symbolsCollector.sourceLocations());

        m_symbolStorage.updateProjectPartSources(artefact.projectPartId,
                                                 m_symbolsCollector.sourceFiles());

        m_symbolStorage.insertOrUpdateUsedMacros(m_symbolsCollector.usedMacros());

        m_symbolStorage.insertFileStatuses(m_symbolsCollector.fileStatuses());

        m_symbolStorage.insertOrUpdateSourceDependencies(m_symbolsCollector.sourceDependencies());

        transaction.commit();
    }
}

bool SymbolIndexer::compilerMacrosOrIncludeSearchPathsAreDifferent(
        const V2::ProjectPartContainer &projectPart,
        const Utils::optional<ProjectPartArtefact> &optionalArtefact) const
{
    if (optionalArtefact) {
        const ProjectPartArtefact &artefact = optionalArtefact.value();
        return projectPart.compilerMacros != artefact.compilerMacros
             || projectPart.includeSearchPaths != artefact.includeSearchPaths;
    }

    return true;
}

FilePathIds SymbolIndexer::filterChangedFiles(const V2::ProjectPartContainer &projectPart) const
{
    FilePathIds ids;
    ids.reserve(projectPart.sourcePathIds.size());

    for (const FilePathId &sourceId : projectPart.sourcePathIds) {
        long long oldLastModified = m_symbolStorage.fetchLowestLastModifiedTime(sourceId);
        long long currentLastModified =  m_fileStatusCache.lastModifiedTime(sourceId);
        if (oldLastModified < currentLastModified)
            ids.push_back(sourceId);
    }

    return ids;
}

FilePathIds SymbolIndexer::updatableFilePathIds(const V2::ProjectPartContainer &projectPart,
                                                const Utils::optional<ProjectPartArtefact> &optionalArtefact) const
{
    if (compilerMacrosOrIncludeSearchPathsAreDifferent(projectPart, optionalArtefact))
        return projectPart.sourcePathIds;

    return filterChangedFiles(projectPart);
}

Utils::SmallStringVector SymbolIndexer::compilerArguments(
        Utils::SmallStringVector arguments,
        int projectPartId) const
{
    Utils::optional<ProjectPartPch> optionalProjectPartPch =  m_symbolStorage.fetchPrecompiledHeader(projectPartId);

    if (optionalProjectPartPch) {
        arguments.emplace_back("-Xclang");
        arguments.emplace_back("-include-pch");
        arguments.emplace_back("-Xclang");
        arguments.emplace_back(std::move(optionalProjectPartPch.value().pchPath));
    }

    return arguments;
}

Utils::SmallStringVector SymbolIndexer::compilerArguments(
        const V2::ProjectPartContainer &projectPart,
        const Utils::optional<ProjectPartArtefact> &optionalProjectPartArtefact) const
{
    if (optionalProjectPartArtefact)
        return compilerArguments(projectPart.arguments,
                                 optionalProjectPartArtefact.value().projectPartId);

    return projectPart.arguments;
}

} // namespace ClangBackEnd
