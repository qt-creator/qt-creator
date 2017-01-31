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

#include "includecollector.h"

#include "collectincludestoolaction.h"

#include <utils/smallstring.h>

namespace ClangBackEnd {

IncludeCollector::IncludeCollector(StringCache<Utils::SmallString> &filePathCache)
    :  m_filePathCache(filePathCache)
{
}

void IncludeCollector::collectIncludes()
{
    clang::tooling::ClangTool tool = createTool();

    auto excludedIncludeFileUIDs = generateExcludedIncludeFileUIDs(tool.getFiles());

    auto action = std::unique_ptr<CollectIncludesToolAction>(
                new CollectIncludesToolAction(m_includeIds, m_filePathCache, excludedIncludeFileUIDs));

    tool.run(action.get());
}

void IncludeCollector::setExcludedIncludes(Utils::SmallStringVector &&excludedIncludes)
{
    this->m_excludedIncludes = std::move(excludedIncludes);
}

std::vector<uint> IncludeCollector::takeIncludeIds()
{
    std::sort(m_includeIds.begin(), m_includeIds.end());

    return std::move(m_includeIds);
}

std::vector<uint> IncludeCollector::generateExcludedIncludeFileUIDs(clang::FileManager &fileManager) const
{
    std::vector<uint> fileUIDs;
    fileUIDs.reserve(m_excludedIncludes.size());

    for (const Utils::SmallString &filePath : m_excludedIncludes) {
        const clang::FileEntry *file = fileManager.getFile({filePath.data(), filePath.size()});

        if (file)
            fileUIDs.push_back(file->getUID());
    }

    std::sort(fileUIDs.begin(), fileUIDs.end());

    return fileUIDs;
}

} // namespace ClangBackEnd
