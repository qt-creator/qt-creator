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
#include "filepathview.h"
#include "stringcache.h"

#include <algorithm>

namespace ClangBackEnd {

template <typename FilePathStorage>
class CLANGSUPPORT_GCCEXPORT FilePathCache
{
    class FileNameView
    {
    public:
        friend bool operator==(const FileNameView &first, const FileNameView &second)
        {
            return first.directoryId == second.directoryId
                && first.fileName == second.fileName;
        }

        static
        int compare(FileNameView first, FileNameView second) noexcept
        {
            int directoryDifference = first.directoryId - second.directoryId;

            if (directoryDifference)
                return directoryDifference;

            return Utils::compare(first.fileName, second.fileName);
        }

    public:
        Utils::SmallStringView fileName;
        int directoryId;
    };

    class FileNameEntry
    {
    public:
        FileNameEntry(Utils::SmallStringView fileName, int directoryId)
            : fileName(fileName),
              directoryId(directoryId)
        {}

        FileNameEntry(FileNameView view)
            : fileName(view.fileName),
              directoryId(view.directoryId)
        {}

        friend bool operator==(const FileNameEntry &first, const FileNameEntry &second)
        {
            return first.directoryId == second.directoryId
                && first.fileName == second.fileName;
        }

        operator FileNameView() const
        {
            return {fileName, directoryId};
        }

        operator Utils::SmallString() &&
        {
            return std::move(fileName);
        }

    public:
        Utils::SmallString fileName;
        int directoryId;
    };

    using DirectoryPathCache = StringCache<Utils::PathString,
                                           Utils::SmallStringView,
                                           int,
                                           SharedMutex,
                                           decltype(&Utils::reverseCompare),
                                           Utils::reverseCompare>;
    using FileNameCache = StringCache<FileNameEntry,
                                      FileNameView,
                                      int,
                                      SharedMutex,
                                      decltype(&FileNameView::compare),
                                      FileNameView::compare>;
public:
    FilePathCache(FilePathStorage &filePathStorage)
        : m_filePathStorage(filePathStorage)
    {}

    FilePathCache(const FilePathCache &) = delete;
    FilePathCache &operator=(const FilePathCache &) = delete;

    FilePathId filePathId(FilePathView filePath) const
    {
        Utils::SmallStringView directoryPath = filePath.directory();

        int directoryId = m_directoryPathCache.stringId(directoryPath,
                                                      [&] (const Utils::SmallStringView) {
            return m_filePathStorage.fetchDirectoryId(directoryPath);
        });

        Utils::SmallStringView fileName = filePath.name();

        int fileNameId = m_fileNameCache.stringId({fileName, directoryId},
                                                  [&] (const FileNameView) {
            return m_filePathStorage.fetchSourceId(directoryId, fileName);
        });

        return fileNameId;
    }

    FilePath filePath(FilePathId filePathId) const
    {
        if (Q_UNLIKELY(!filePathId.isValid()))
            throw NoFilePathForInvalidFilePathId();

        auto fetchSoureNameAndDirectoryId = [&] (int id) {
            auto entry = m_filePathStorage.fetchSourceNameAndDirectoryId(id);
            return FileNameEntry{entry.sourceName, entry.directoryId};
        };

        FileNameEntry entry = m_fileNameCache.string(filePathId.filePathId,
                                                     fetchSoureNameAndDirectoryId);

        auto fetchDirectoryPath = [&] (int id) { return m_filePathStorage.fetchDirectoryPath(id); };

        Utils::PathString directoryPath = m_directoryPathCache.string(entry.directoryId,
                                                                      fetchDirectoryPath);

        return FilePath{directoryPath, entry.fileName};
    }

private:
    mutable DirectoryPathCache m_directoryPathCache;
    mutable FileNameCache m_fileNameCache;
    FilePathStorage &m_filePathStorage;
};

} // namespace ClangBackEnd
