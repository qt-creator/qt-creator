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

#include "builddependencygeneratorinterface.h"

#include <clangtool.h>
#include <sourcesmanager.h>

#include <filepathcachingfwd.h>

namespace ClangBackEnd {

class BuildDependencyCollector : public BuildDependencyGeneratorInterface
{
public:
    BuildDependencyCollector(const FilePathCachingInterface &filePathCache)
        :  m_filePathCache(filePathCache)
    {
    }

    BuildDependency create(const V2::ProjectPartContainer &projectPart) override;

    void collect();

    void setExcludedFilePaths(ClangBackEnd::FilePaths &&excludedIncludes);
    void addFiles(const FilePathIds &filePathIds,
                  const Utils::SmallStringVector &arguments);
    void addFile(FilePathId filePathId,
                 const Utils::SmallStringVector &arguments);
    void addFile(FilePath filePath,
                 const FilePathIds &sourceFileIds,
                 const Utils::SmallStringVector &arguments);
    void addUnsavedFiles(const V2::FileContainers &unsavedFiles);

    void clear();

    const FileStatuses &fileStatuses() const
    {
        return m_buildDependency.fileStatuses;
    }

    const FilePathIds &sourceFiles() const
    {
        return m_buildDependency.sourceFiles;
    }

    const UsedMacros &usedMacros() const
    {
        return m_buildDependency.usedMacros;
    }

    const SourceDependencies &sourceDependencies() const
    {
        return m_buildDependency.sourceDependencies;
    }

    const SourceEntries &includeIds()
    {
        std::sort(m_buildDependency.includes.begin(), m_buildDependency.includes.end());

        return std::move(m_buildDependency.includes);
    }

private:
    ClangTool m_clangTool;
    BuildDependency m_buildDependency;
    ClangBackEnd::FilePaths m_excludedFilePaths;
    Utils::SmallStringVector m_directories;
    SourcesManager m_sourcesManager;
    const FilePathCachingInterface &m_filePathCache;
};

} // namespace ClangBackEnd
