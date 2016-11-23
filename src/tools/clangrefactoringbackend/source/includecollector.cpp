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

namespace ClangBackEnd {

void IncludeCollector::collectIncludes()
{
    clang::tooling::ClangTool tool = createTool();

    auto excludedIncludeFileUIDs = generateExcludedIncludeFileUIDs(tool.getFiles());

    auto action = std::unique_ptr<CollectIncludesToolAction>(
                new CollectIncludesToolAction(includes, excludedIncludeFileUIDs));

    tool.run(action.get());
}

void IncludeCollector::setExcludedIncludes(Utils::SmallStringVector &&excludedIncludes)
{
    this->excludedIncludes = std::move(excludedIncludes);
}

Utils::SmallStringVector IncludeCollector::takeIncludes()
{
    std::sort(includes.begin(), includes.end());

    return std::move(includes);
}

std::vector<uint> IncludeCollector::generateExcludedIncludeFileUIDs(clang::FileManager &fileManager) const
{
    std::vector<uint> fileUIDs;
    fileUIDs.reserve(excludedIncludes.size());

    auto generateUID = [&] (const Utils::SmallString &filePath) {
        return fileManager.getFile({filePath.data(), filePath.size()})->getUID();
    };

    std::transform(excludedIncludes.begin(),
                   excludedIncludes.end(),
                   std::back_inserter(fileUIDs),
                   generateUID);

    std::sort(fileUIDs.begin(), fileUIDs.end());

    return fileUIDs;
}

} // namespace ClangBackEnd
