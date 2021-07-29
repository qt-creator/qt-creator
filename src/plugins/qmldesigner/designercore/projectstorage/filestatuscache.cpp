/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include <utils/set_algorithm.h>

#include <QDateTime>
#include <QFileInfo>

namespace QmlDesigner {

long long FileStatusCache::lastModifiedTime(SourceId sourceId) const
{
    return findEntry(sourceId).lastModified;
}

void FileStatusCache::update(SourceId sourceId)
{
    auto found = std::lower_bound(m_cacheEntries.begin(),
                                  m_cacheEntries.end(),
                                  Internal::FileStatusCacheEntry{sourceId},
                                  [](const auto &first, const auto &second) {
                                      return first.sourceId < second.sourceId;
                                  });

    if (found != m_cacheEntries.end() && found->sourceId == sourceId)
        found->lastModified = m_fileSystem.lastModified(sourceId);
}

void FileStatusCache::update(SourceIds sourceIds)
{
    std::set_intersection(m_cacheEntries.begin(),
                          m_cacheEntries.end(),
                          sourceIds.begin(),
                          sourceIds.end(),
                          Utils::make_iterator([&](auto &entry) {
                              entry.lastModified = m_fileSystem.lastModified(entry.sourceId);
                          }));
}

SourceIds FileStatusCache::modified(SourceIds sourceIds) const
{
    SourceIds modifiedSourceIds;
    modifiedSourceIds.reserve(sourceIds.size());

    std::set_intersection(m_cacheEntries.begin(),
                          m_cacheEntries.end(),
                          sourceIds.begin(),
                          sourceIds.end(),
                          Utils::make_iterator([&](auto &entry) {
                              auto newLastModified = m_fileSystem.lastModified(entry.sourceId);
                              if (newLastModified > entry.lastModified) {
                                  modifiedSourceIds.push_back(entry.sourceId);
                                  entry.lastModified = newLastModified;
                              }
                          }));

    Internal::FileStatusCacheEntries newEntries;
    newEntries.reserve(sourceIds.size());

    std::set_difference(sourceIds.begin(),
                        sourceIds.end(),
                        m_cacheEntries.begin(),
                        m_cacheEntries.end(),
                        Utils::make_iterator([&](SourceId newSourceId) {
                            newEntries.emplace_back(newSourceId,
                                                    m_fileSystem.lastModified(newSourceId));
                            modifiedSourceIds.push_back(newSourceId);
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

    std::sort(modifiedSourceIds.begin(), modifiedSourceIds.end());

    return modifiedSourceIds;
}

FileStatusCache::size_type FileStatusCache::size() const
{
    return m_cacheEntries.size();
}

Internal::FileStatusCacheEntry FileStatusCache::findEntry(SourceId sourceId) const
{
    auto found = std::lower_bound(m_cacheEntries.begin(),
                                  m_cacheEntries.end(),
                                  Internal::FileStatusCacheEntry{sourceId},
                                  [](const auto &first, const auto &second) {
                                      return first.sourceId < second.sourceId;
                                  });

    if (found != m_cacheEntries.end() && found->sourceId == sourceId)
        return *found;

    auto inserted = m_cacheEntries.emplace(found, sourceId, m_fileSystem.lastModified(sourceId));

    return *inserted;
}

} // namespace QmlDesigner
