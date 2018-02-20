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
#include "symbolscollectorinterface.h"
#include "symbolstorageinterface.h"
#include "clangpathwatcher.h"

#include <projectpartcontainerv2.h>
#include <filecontainerv2.h>

namespace ClangBackEnd {

class SymbolIndexer : public ClangPathWatcherNotifier
{
public:
    SymbolIndexer(SymbolsCollectorInterface &symbolsCollector,
                  SymbolStorageInterface &symbolStorage,
                  ClangPathWatcherInterface &pathWatcher,
                  FilePathCachingInterface &filePathCache,
                  FileStatusCache &fileStatusCache,
                  Sqlite::TransactionInterface &transactionInterface);

    void updateProjectParts(V2::ProjectPartContainers &&projectParts,
                            V2::FileContainers &&generatedFiles);
    void updateProjectPart(V2::ProjectPartContainer &&projectPart,
                           const V2::FileContainers &generatedFiles);

    void pathsWithIdsChanged(const Utils::SmallStringVector &ids) override;
    void pathsChanged(const FilePathIds &filePathIds) override;
    void updateChangedPath(FilePathId filePath);

    bool compilerMacrosOrIncludeSearchPathsAreDifferent(
            const V2::ProjectPartContainer &projectPart,
            const Utils::optional<ProjectPartArtefact> &optionalArtefact) const;

    FilePathIds filterChangedFiles(
            const V2::ProjectPartContainer &projectPart) const;

    FilePathIds updatableFilePathIds(const V2::ProjectPartContainer &projectPart,
                                     const Utils::optional<ProjectPartArtefact> &optionalArtefact) const;

    Utils::SmallStringVector compilerArguments(const V2::ProjectPartContainer &projectPart,
                                               const Utils::optional<ProjectPartArtefact> &optionalArtefact) const;
    Utils::SmallStringVector compilerArguments(Utils::SmallStringVector arguments,
                                               int projectPartId) const;

private:
    SymbolsCollectorInterface &m_symbolsCollector;
    SymbolStorageInterface &m_symbolStorage;
    ClangPathWatcherInterface &m_pathWatcher;
    FilePathCachingInterface &m_filePathCache;
    FileStatusCache &m_fileStatusCache;
    Sqlite::TransactionInterface &m_transactionInterface;
};

} // namespace ClangBackEnd
