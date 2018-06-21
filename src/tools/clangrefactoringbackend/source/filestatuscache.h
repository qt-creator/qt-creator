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

#include <filepathcachinginterface.h>

QT_FORWARD_DECLARE_CLASS(QFileInfo)

namespace ClangBackEnd {

namespace Internal {
class FileStatusCacheEntry
{
public:
    FileStatusCacheEntry(ClangBackEnd::FilePathId filePathId,
                         long long lastModified = 0)
        : filePathId(filePathId),
          lastModified(lastModified)
    {}

public:
    ClangBackEnd::FilePathId filePathId;
    long long lastModified;
};

using FileStatusCacheEntries = std::vector<FileStatusCacheEntry>;

}

class FileStatusCache
{
public:
    using size_type = Internal::FileStatusCacheEntries::size_type;

    FileStatusCache(FilePathCachingInterface &filePathCache);
    FileStatusCache &operator=(const FileStatusCache &) = delete;
    FileStatusCache(const FileStatusCache &) = delete;

    long long lastModifiedTime(ClangBackEnd::FilePathId filePathId) const;
    void update(ClangBackEnd::FilePathId filePathId);

    size_type size() const;

private:
    Internal::FileStatusCacheEntry findEntry(ClangBackEnd::FilePathId filePathId) const;
    QFileInfo qFileInfo(ClangBackEnd::FilePathId filePathId) const;

private:
    mutable Internal::FileStatusCacheEntries m_cacheEntries;
    FilePathCachingInterface &m_filePathCache;
};

} // namespace ClangBackEnd
