/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "usedmacrosandsourcescollector.h"

#include "collectusedmacroactionfactory.h"

namespace ClangBackEnd {

void UsedMacroAndSourcesCollector::addFiles(const FilePathIds &filePathIds,
                                            const Utils::SmallStringVector &arguments)
{
    m_clangTool.addFiles(m_filePathCache.filePaths(filePathIds), arguments);
    m_sourceFiles.insert(m_sourceFiles.end(), filePathIds.begin(), filePathIds.end());
}

void UsedMacroAndSourcesCollector::addFile(FilePathId filePathId, const Utils::SmallStringVector &arguments)
{
    addFiles({filePathId}, arguments);
}

void UsedMacroAndSourcesCollector::collect()
{
    clang::tooling::ClangTool tool = m_clangTool.createTool();

    auto action = std::make_unique<CollectUsedMacrosToolActionFactory>(
                m_usedMacros,
                m_filePathCache,
                m_sourceDependencies,
                m_sourceFiles,
                m_fileStatuses);

    tool.run(action.get());
}

void UsedMacroAndSourcesCollector::clear()
{
    m_clangTool = ClangTool();
    m_usedMacros.clear();
    m_sourceFiles.clear();
    m_fileStatuses.clear();
    m_sourceDependencies.clear();
}

} // namespace ClangBackEnd
