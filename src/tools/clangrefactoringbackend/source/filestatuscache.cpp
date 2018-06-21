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

#include "filestatuscache.h"

#include <QDateTime>
#include <QFileInfo>

namespace ClangBackEnd {

FileStatusCache::FileStatusCache(FilePathCachingInterface &filePathCache)
    : m_filePathCache(filePathCache)
{

}

long long FileStatusCache::lastModifiedTime(FilePathId filePathId) const
{
    return findEntry(filePathId).lastModified;
}

void FileStatusCache::update(FilePathId filePathId)
{
    auto found = std::lower_bound(m_cacheEntries.begin(),
                                  m_cacheEntries.end(),
                                  Internal::FileStatusCacheEntry{filePathId},
                                  [] (const auto &first, const auto &second) {
        return first.filePathId < second.filePathId;
    });

    if (found != m_cacheEntries.end() && found->filePathId == filePathId) {
        QFileInfo fileInfo = qFileInfo(filePathId);
        found->lastModified = fileInfo.lastModified().toMSecsSinceEpoch() / 1000;
    }
}

FileStatusCache::size_type FileStatusCache::size() const
{
    return m_cacheEntries.size();
}

Internal::FileStatusCacheEntry FileStatusCache::findEntry(FilePathId filePathId) const
{
    auto found = std::lower_bound(m_cacheEntries.begin(),
                                  m_cacheEntries.end(),
                                  Internal::FileStatusCacheEntry{filePathId},
                                  [] (const auto &first, const auto &second) {
        return first.filePathId < second.filePathId;
    });

    if (found != m_cacheEntries.end() && found->filePathId == filePathId)
        return *found;

    QFileInfo fileInfo = qFileInfo(filePathId);
    auto inserted = m_cacheEntries.emplace(found,
                                           filePathId,
                                           fileInfo.lastModified().toMSecsSinceEpoch() / 1000);

    return *inserted;
}

QFileInfo FileStatusCache::qFileInfo(FilePathId filePathId) const
{
    QFileInfo fileInfo(QString(m_filePathCache.filePath(filePathId)));

    fileInfo.refresh();

    return fileInfo;
}

} // namespace ClangBackEnd
