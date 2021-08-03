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
    return find(sourceId).lastModified;
}

long long FileStatusCache::fileSize(SourceId sourceId) const
{
    return find(sourceId).size;
}

void FileStatusCache::update(SourceId sourceId)
{
    auto found = std::lower_bound(m_cacheEntries.begin(),
                                  m_cacheEntries.end(),
                                  sourceId,
                                  [](const auto &first, const auto &second) {
                                      return first < second;
                                  });

    if (found != m_cacheEntries.end() && found->sourceId == sourceId)
        *found = m_fileSystem.fileStatus(sourceId);
}

void FileStatusCache::update(SourceIds sourceIds)
{
    std::set_intersection(m_cacheEntries.begin(),
                          m_cacheEntries.end(),
                          sourceIds.begin(),
                          sourceIds.end(),
                          Utils::make_iterator([&](auto &entry) {
                              entry = m_fileSystem.fileStatus(entry.sourceId);
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
                              auto fileStatus = m_fileSystem.fileStatus(entry.sourceId);
                              if (fileStatus != entry) {
                                  modifiedSourceIds.push_back(entry.sourceId);
                                  entry = fileStatus;
                              }
                          }));

    FileStatuses newEntries;
    newEntries.reserve(sourceIds.size());

    std::set_difference(sourceIds.begin(),
                        sourceIds.end(),
                        m_cacheEntries.begin(),
                        m_cacheEntries.end(),
                        Utils::make_iterator([&](SourceId newSourceId) {
                            newEntries.push_back(m_fileSystem.fileStatus(newSourceId));
                            modifiedSourceIds.push_back(newSourceId);
                        }));

    if (newEntries.size()) {
        FileStatuses mergedEntries;
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

const FileStatus &FileStatusCache::find(SourceId sourceId) const
{
    auto found = std::lower_bound(m_cacheEntries.begin(),
                                  m_cacheEntries.end(),
                                  sourceId,
                                  [](const auto &first, const auto &second) {
                                      return first < second;
                                  });

    if (found != m_cacheEntries.end() && found->sourceId == sourceId)
        return *found;

    auto inserted = m_cacheEntries.insert(found, m_fileSystem.fileStatus(sourceId));

    return *inserted;
}

} // namespace QmlDesigner
