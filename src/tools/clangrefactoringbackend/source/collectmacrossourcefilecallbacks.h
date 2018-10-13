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

#include "filestatus.h"
#include "sourcedependency.h"
#include "sourcelocationentry.h"
#include "symbolentry.h"
#include "usedmacro.h"

#include <filepathcachinginterface.h>

#include <clang/Tooling/Tooling.h>

namespace ClangBackEnd {

class SourcesManager;

class CollectMacrosSourceFileCallbacks : public clang::tooling::SourceFileCallbacks
{
public:
    CollectMacrosSourceFileCallbacks(SymbolEntries &symbolEntries,
                                     SourceLocationEntries &sourceLocationEntries,
                                     FilePathCachingInterface &filePathCache,
                                     SourcesManager &sourcesManager)
        : m_symbolEntries(symbolEntries),
          m_sourceLocationEntries(sourceLocationEntries),
          m_filePathCache(filePathCache),
          m_sourcesManager(sourcesManager)
    {
    }

    bool handleBeginSource(clang::CompilerInstance &compilerInstance) override;

    const FilePathIds &sourceFiles() const
    {
        return m_sourceFiles;
    }

    void addSourceFiles(const FilePathIds &filePathIds)
    {
        m_sourceFiles = filePathIds;
    }

    void clear()
    {
        m_sourceFiles.clear();
        m_usedMacros.clear();
        m_fileStatuses.clear();
        m_sourceDependencies.clear();
    }

    const UsedMacros &usedMacros() const
    {
        return m_usedMacros;
    }

    const FileStatuses &fileStatuses() const
    {
        return m_fileStatuses;
    }

    const SourceDependencies &sourceDependencies() const
    {
        return m_sourceDependencies;
    }

private:
    FilePathIds m_sourceFiles;
    UsedMacros m_usedMacros;
    FileStatuses m_fileStatuses;
    SourceDependencies m_sourceDependencies;
    SymbolEntries &m_symbolEntries;
    SourceLocationEntries &m_sourceLocationEntries;
    FilePathCachingInterface &m_filePathCache;
    SourcesManager &m_sourcesManager;
};

} // namespace ClangBackEnd
