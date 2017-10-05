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

#include "filepathexceptions.h"
#include "filepathid.h"
#include "filepath.h"
#include "stringcache.h"

#include <utils/smallstringview.h>

#include <algorithm>

namespace ClangBackEnd {

class FilePathCacheBase
{
public:
    static
    std::ptrdiff_t lastSlashIndex(Utils::SmallStringView filePath)
    {
        auto foundReverse = std::find(filePath.rbegin(), filePath.rend(), '/');
        auto found = foundReverse.base();
        --found;

        return std::distance(filePath.begin(), found);
    }

    static
    Utils::SmallStringView directoryPath(Utils::SmallStringView filePath, std::ptrdiff_t slashIndex)
    {
        return {filePath.data(), std::size_t(std::max(std::ptrdiff_t(0), slashIndex))};
    }

    static
    Utils::SmallStringView fileName(Utils::SmallStringView filePath, std::ptrdiff_t slashIndex)
    {
        return {filePath.data() + slashIndex + 1, filePath.size() - std::size_t(slashIndex) - 1};
    }
};

template <typename FilePathStorage>
class FilePathCache final : private FilePathCacheBase
{
    using DirectoryPathCache = StringCache<Utils::PathString,
                                           int,
                                           SharedMutex,
                                           decltype(&Utils::reverseCompare),
                                           Utils::reverseCompare>;
    using FileNameCache = StringCache<Utils::SmallString,
                                      int,
                                      SharedMutex,
                                      decltype(&Utils::compare),
                                      Utils::compare>;
public:
    FilePathCache(FilePathStorage &filePathStorage)
        : m_filePathStorage(filePathStorage)
    {}

    FilePathId filePathId(Utils::SmallStringView filePath) const
    {
        std::ptrdiff_t slashIndex = lastSlashIndex(filePath);

        Utils::SmallStringView directoryPath = this->directoryPath(filePath, slashIndex);
        int directoryId = m_directyPathCache.stringId(directoryPath,
                                                      [&] (const Utils::SmallStringView) {
            return m_filePathStorage.fetchDirectoryId(directoryPath);
        });

        Utils::SmallStringView fileName = this->fileName(filePath, slashIndex);

        int fileNameId = m_fileNameCache.stringId(fileName,
                                                  [&] (const Utils::SmallStringView) {
            return m_filePathStorage.fetchSourceId(directoryId, fileName);
        });

        return {directoryId, fileNameId};
    }

    FilePath filePath(FilePathId filePathId) const
    {
        if (Q_UNLIKELY(!filePathId.isValid()))
            throw NoFilePathForInvalidFilePathId();

        auto fetchFilePath = [&] (int id) { return m_filePathStorage.fetchDirectoryPath(id); };

        Utils::PathString directoryPath = m_directyPathCache.string(filePathId.directoryId,
                                                                    fetchFilePath);


        auto fetchSoureName = [&] (int id) { return m_filePathStorage.fetchSourceName(id); };

        Utils::SmallString fileName = m_fileNameCache.string(filePathId.fileNameId,
                                                             fetchSoureName);

        return {directoryPath, fileName};
    }

private:
    mutable DirectoryPathCache m_directyPathCache;
    mutable FileNameCache m_fileNameCache;
    FilePathStorage &m_filePathStorage;
};

} // namespace ClangBackEnd
