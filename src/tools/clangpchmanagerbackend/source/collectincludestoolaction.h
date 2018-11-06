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

#pragma once

#include "collectincludesaction.h"

#include <filepathcachingfwd.h>
#include <filepathid.h>

#include <clang/Tooling/Tooling.h>

namespace ClangBackEnd {

class CollectIncludesToolAction final : public clang::tooling::FrontendActionFactory
{
public:
    CollectIncludesToolAction(FilePathIds &includeIds,
                              FilePathIds &topIncludeIds,
                              FilePathIds &topsSystemIncludeIds,
                              const FilePathCachingInterface &filePathCache,
                              const Utils::PathStringVector &excludedIncludes,
                              UsedMacros &usedMacros,
                              SourcesManager &sourcesManager,
                              SourceDependencies &sourceDependencies,
                              FilePathIds &sourceFiles,
                              FileStatuses &fileStatuses)
        : m_includeIds(includeIds),
          m_topIncludeIds(topIncludeIds),
          m_topsSystemIncludeIds(topsSystemIncludeIds),
          m_filePathCache(filePathCache),
          m_excludedIncludes(excludedIncludes),
          m_usedMacros(usedMacros),
          m_sourcesManager(sourcesManager),
          m_sourceDependencies(sourceDependencies),
          m_sourceFiles(sourceFiles),
          m_fileStatuses(fileStatuses)
    {}


    bool runInvocation(std::shared_ptr<clang::CompilerInvocation> invocation,
                       clang::FileManager *fileManager,
                       std::shared_ptr<clang::PCHContainerOperations> pchContainerOperations,
                       clang::DiagnosticConsumer *diagnosticConsumer) override
    {
        if (m_excludedIncludeUIDs.empty())
            m_excludedIncludeUIDs = generateExcludedIncludeFileUIDs(*fileManager);

        return clang::tooling::FrontendActionFactory::runInvocation(invocation,
                                                                    fileManager,
                                                                    pchContainerOperations,
                                                                    diagnosticConsumer);
    }

    clang::FrontendAction *create() override
    {
        return new CollectIncludesAction(m_includeIds,
                                         m_topIncludeIds,
                                         m_topsSystemIncludeIds,
                                         m_filePathCache,
                                         m_excludedIncludeUIDs,
                                         m_alreadyIncludedFileUIDs,
                                         m_usedMacros,
                                         m_sourcesManager,
                                         m_sourceDependencies,
                                         m_sourceFiles,
                                         m_fileStatuses);
    }

    std::vector<uint> generateExcludedIncludeFileUIDs(clang::FileManager &fileManager) const
    {
        std::vector<uint> fileUIDs;
        fileUIDs.reserve(m_excludedIncludes.size());

        for (const Utils::PathString &filePath : m_excludedIncludes) {
            const clang::FileEntry *file = fileManager.getFile({filePath.data(), filePath.size()}, true);

            if (file)
                fileUIDs.push_back(file->getUID());
        }

        std::sort(fileUIDs.begin(), fileUIDs.end());

        return fileUIDs;
    }

private:
    std::vector<uint> m_alreadyIncludedFileUIDs;
    std::vector<uint> m_excludedIncludeUIDs;
    FilePathIds &m_includeIds;
    FilePathIds &m_topIncludeIds;
    FilePathIds &m_topsSystemIncludeIds;
    const FilePathCachingInterface &m_filePathCache;
    const Utils::PathStringVector &m_excludedIncludes;
    UsedMacros &m_usedMacros;
    SourcesManager &m_sourcesManager;
    SourceDependencies &m_sourceDependencies;
    FilePathIds &m_sourceFiles;
    FileStatuses &m_fileStatuses;
};

} // namespace ClangBackEnd
