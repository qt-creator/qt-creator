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

#include <clangtool.h>
#include <filestatus.h>
#include <sourcedependency.h>
#include <sourcesmanager.h>
#include <usedmacro.h>

#include <filepathcachingfwd.h>

namespace ClangBackEnd {

class IncludeCollector : public ClangTool
{
public:
    IncludeCollector(const FilePathCachingInterface &filePathCache)
        :  m_filePathCache(filePathCache)
    {
    }

    void collect();

    void setExcludedIncludes(Utils::PathStringVector &&excludedIncludes);
    void addFiles(const FilePathIds &filePathIds,
                  const Utils::SmallStringVector &arguments);
    void addFile(FilePathId filePathId,
                 const Utils::SmallStringVector &arguments);
    void addFile(FilePath filePath,
                 const FilePathIds &sourceFileIds,
                 const Utils::SmallStringVector &arguments);

    void clear();

    const FileStatuses &fileStatuses() const
    {
        return m_fileStatuses;
    }

    const FilePathIds &sourceFiles() const
    {
        return m_sourceFiles;
    }

    const UsedMacros &usedMacros() const
    {
        return m_usedMacros;
    }

    const SourceDependencies &sourceDependencies() const
    {
        return m_sourceDependencies;
    }

    FilePathIds takeIncludeIds()
    {
        std::sort(m_includeIds.begin(), m_includeIds.end());

        return std::move(m_includeIds);
    }

    FilePathIds takeTopIncludeIds()
    {
        std::sort(m_topIncludeIds.begin(), m_topIncludeIds.end());

        return std::move(m_topIncludeIds);
    }

    FilePathIds takeTopsSystemIncludeIds()
    {
        std::sort(m_topsSystemIncludeIds.begin(), m_topsSystemIncludeIds.end());

        return std::move(m_topsSystemIncludeIds);
    }

private:
    ClangTool m_clangTool;
    Utils::PathStringVector m_excludedIncludes;
    FilePathIds m_includeIds;
    FilePathIds m_topIncludeIds;
    FilePathIds m_topsSystemIncludeIds;
    Utils::SmallStringVector m_directories;
    SourcesManager m_sourcesManager;
    UsedMacros m_usedMacros;
    FilePathIds m_sourceFiles;
    SourceDependencies m_sourceDependencies;
    FileStatuses m_fileStatuses;
    const FilePathCachingInterface &m_filePathCache;
};

} // namespace ClangBackEnd
