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

namespace {
template<class InputIt1, class InputIt2, class Callable>
void set_intersection_call(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, Callable callable)
{
    while (first1 != last1 && first2 != last2) {
        if (*first1 < *first2) {
            ++first1;
        } else {
            if (!(*first2 < *first1))
                callable(*first1++);
            ++first2;
        }
    }
}

template<class InputIt1, class InputIt2, class Callable>
void set_difference_call(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, Callable callable)
{
    while (first1 != last1) {
        if (first2 == last2) {
            std::for_each(first1, last1, callable);
            return;
        }
        if (*first1 < *first2) {
            callable(*first1++);
        } else {
            if (!(*first2 < *first1))
                ++first1;
            ++first2;
        }
    }
}
} // namespace

void FileStatusCache::update(FilePathIds filePathIds)
{
    set_intersection_call(m_cacheEntries.begin(),
                          m_cacheEntries.end(),
                          filePathIds.begin(),
                          filePathIds.end(),
                          [&](auto &entry) {
                              entry.lastModified = m_fileSystem.lastModified(entry.filePathId);
                          });
}

FilePathIds FileStatusCache::modified(FilePathIds filePathIds) const
{
    FilePathIds modifiedFilePathIds;
    modifiedFilePathIds.reserve(filePathIds.size());

    set_intersection_call(m_cacheEntries.begin(),
                          m_cacheEntries.end(),
                          filePathIds.begin(),
                          filePathIds.end(),
                          [&](auto &entry) {
                              auto newLastModified = m_fileSystem.lastModified(entry.filePathId);
                              if (newLastModified > entry.lastModified) {
                                  modifiedFilePathIds.push_back(entry.filePathId);
                                  entry.lastModified = newLastModified;
                              }
                          });

    Internal::FileStatusCacheEntries newEntries;
    newEntries.reserve(filePathIds.size());

    set_difference_call(filePathIds.begin(),
                        filePathIds.end(),
                        m_cacheEntries.begin(),
                        m_cacheEntries.end(),
                        [&](FilePathId newFilePathId) {
                            newEntries.emplace_back(newFilePathId,
                                                    m_fileSystem.lastModified(newFilePathId));
                            modifiedFilePathIds.push_back(newFilePathId);
                        });

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
