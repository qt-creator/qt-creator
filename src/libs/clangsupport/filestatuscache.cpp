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
#include "filesystem.h"

#include <set_algorithm.h>

#include <utils/algorithm.h>

#include <QDateTime>
#include <QFileInfo>

namespace ClangBackEnd {

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

    if (found != m_cacheEntries.end() && found->filePathId == filePathId)
        found->lastModified = m_fileSystem.lastModified(filePathId);
}

void FileStatusCache::update(FilePathIds filePathIds)
{
    std::set_intersection(m_cacheEntries.begin(),
                          m_cacheEntries.end(),
                          filePathIds.begin(),
                          filePathIds.end(),
                          make_iterator([&](auto &entry) {
                              entry.lastModified = m_fileSystem.lastModified(entry.filePathId);
                          }));
}

FilePathIds FileStatusCache::modified(FilePathIds filePathIds) const
{
    FilePathIds modifiedFilePathIds;
    modifiedFilePathIds.reserve(filePathIds.size());

    std::set_intersection(m_cacheEntries.begin(),
                          m_cacheEntries.end(),
                          filePathIds.begin(),
                          filePathIds.end(),
                          make_iterator([&](auto &entry) {
                              auto newLastModified = m_fileSystem.lastModified(entry.filePathId);
                              if (newLastModified > entry.lastModified) {
                                  modifiedFilePathIds.push_back(entry.filePathId);
                                  entry.lastModified = newLastModified;
                              }
                          }));

    Internal::FileStatusCacheEntries newEntries;
    newEntries.reserve(filePathIds.size());

    std::set_difference(filePathIds.begin(),
                        filePathIds.end(),
                        m_cacheEntries.begin(),
                        m_cacheEntries.end(),
                        make_iterator([&](FilePathId newFilePathId) {
                            newEntries.emplace_back(newFilePathId,
                                                    m_fileSystem.lastModified(newFilePathId));
                            modifiedFilePathIds.push_back(newFilePathId);
                        }));

    if (newEntries.size()) {
        Internal::FileStatusCacheEntries mergedEntries;
        mergedEntries.reserve(m_cacheEntries.size() + newEntries.size());

        std::set_union(newEntries.begin(),
                       newEntries.end(),
                       m_cacheEntries.begin(),
                       m_cacheEntries.end(),
                       std::back_inserter(mergedEntries));

        m_cacheEntries = std::move(mergedEntries);
    }

    std::sort(modifiedFilePathIds.begin(), modifiedFilePathIds.end());

    return modifiedFilePathIds;
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

    auto inserted = m_cacheEntries.emplace(found, filePathId, m_fileSystem.lastModified(filePathId));

    return *inserted;
}

} // namespace ClangBackEnd
