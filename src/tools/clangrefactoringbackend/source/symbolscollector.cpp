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

#include "symbolscollector.h"

namespace ClangBackEnd {

SymbolsCollector::SymbolsCollector(FilePathCachingInterface &filePathCache)
    : m_collectSymbolsAction(m_symbolEntries, m_sourceLocationEntries, filePathCache),
      m_collectMacrosSourceFileCallbacks(m_symbolEntries, m_sourceLocationEntries, filePathCache),
      m_filePathCache(filePathCache)
{
}

void SymbolsCollector::addFiles(const FilePathIds &filePathIds,
                                const Utils::SmallStringVector &arguments)
{
    m_clangTool.addFiles(m_filePathCache.filePaths(filePathIds), arguments);
    m_collectMacrosSourceFileCallbacks.addSourceFiles(filePathIds);
}

void SymbolsCollector::addUnsavedFiles(const V2::FileContainers &unsavedFiles)
{
    m_clangTool.addUnsavedFiles(unsavedFiles);
}

void SymbolsCollector::clear()
{
    m_collectMacrosSourceFileCallbacks.clear();
    m_symbolEntries.clear();
    m_sourceLocationEntries.clear();
    m_clangTool = ClangTool();
}

void SymbolsCollector::collectSymbols()
{
    auto tool = m_clangTool.createTool();

    tool.run(clang::tooling::newFrontendActionFactory(&m_collectSymbolsAction,
                                                      &m_collectMacrosSourceFileCallbacks).get());
}

const SymbolEntries &SymbolsCollector::symbols() const
{
    return m_collectSymbolsAction.symbols();
}

const SourceLocationEntries &SymbolsCollector::sourceLocations() const
{
    return m_collectSymbolsAction.sourceLocations();
}

const FilePathIds &SymbolsCollector::sourceFiles() const
{
    return m_collectMacrosSourceFileCallbacks.sourceFiles();
}

const UsedMacros &SymbolsCollector::usedMacros() const
{
    return m_collectMacrosSourceFileCallbacks.usedMacros();
}

const FileStatuses &SymbolsCollector::fileStatuses() const
{
    return m_collectMacrosSourceFileCallbacks.fileStatuses();
}

const SourceDependencies &SymbolsCollector::sourceDependencies() const
{
    return m_collectMacrosSourceFileCallbacks.sourceDependencies();
}

} // namespace ClangBackEnd
