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

#pragma once

#include "filestatuscache.h"
#include "symbolindexertaskqueueinterface.h"
#include "symbolstorageinterface.h"
#include "builddependenciesstorageinterface.h"

#include <clangpathwatcher.h>
#include <filecontainerv2.h>
#include <modifiedtimecheckerinterface.h>
#include <precompiledheaderstorageinterface.h>
#include <projectpartcontainer.h>
#include <projectpartsstorageinterface.h>

namespace ClangBackEnd {

class SymbolsCollectorInterface;
class Environment;

class SymbolIndexer final : public ClangPathWatcherNotifier
{
public:
    SymbolIndexer(SymbolIndexerTaskQueueInterface &symbolIndexerTaskQueue,
                  SymbolStorageInterface &symbolStorage,
                  BuildDependenciesStorageInterface &buildDependenciesStorage,
                  PrecompiledHeaderStorageInterface &precompiledHeaderStorage,
                  ClangPathWatcherInterface &pathWatcher,
                  FilePathCachingInterface &filePathCache,
                  FileStatusCache &fileStatusCache,
                  Sqlite::TransactionInterface &transactionInterface,
                  ProjectPartsStorageInterface &projectPartsStorage,
                  ModifiedTimeCheckerInterface<SourceTimeStamps> &modifiedTimeChecker,
                  const Environment &environment);

    void updateProjectParts(ProjectPartContainers &&projectParts);
    void updateProjectPart(ProjectPartContainer &&projectPart);

    void pathsWithIdsChanged(const std::vector<IdPaths> &idPaths) override;
    void pathsChanged(const FilePathIds &filePathIds) override;
    void updateChangedPath(FilePathId filePath,
                           std::vector<SymbolIndexerTask> &symbolIndexerTask);

    bool compilerMacrosOrIncludeSearchPathsAreDifferent(
            const ProjectPartContainer &projectPart,
            const Utils::optional<ProjectPartArtefact> &optionalArtefact) const;

    FilePathIds filterChangedFiles(
            const ProjectPartContainer &projectPart) const;

    FilePathIds updatableFilePathIds(const ProjectPartContainer &projectPart,
                                     const Utils::optional<ProjectPartArtefact> &optionalArtefact) const;

private:
    FilePathIds filterProjectPartSources(const FilePathIds &filePathIds) const;

private:
    SymbolIndexerTaskQueueInterface &m_symbolIndexerTaskQueue;
    SymbolStorageInterface &m_symbolStorage;
    BuildDependenciesStorageInterface &m_buildDependencyStorage;
    PrecompiledHeaderStorageInterface &m_precompiledHeaderStorage;
    ClangPathWatcherInterface &m_pathWatcher;
    FilePathCachingInterface &m_filePathCache;
    FileStatusCache &m_fileStatusCache;
    Sqlite::TransactionInterface &m_transactionInterface;
    ProjectPartsStorageInterface &m_projectPartsStorage;
    ModifiedTimeCheckerInterface<SourceTimeStamps> &m_modifiedTimeChecker;
    const Environment &m_environment;
};

} // namespace ClangBackEnd
