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

#pragma once

#include "clangpathwatcherinterface.h"
#include "clangpathwatchernotifier.h"
#include "changedfilepathcompressor.h"
#include "filepathcachinginterface.h"
#include "stringcache.h"

#include <QTimer>

namespace ClangBackEnd {

class WatcherEntry
{
public:
    ProjectPartId id;
    FilePathId pathId;

    friend bool operator==(WatcherEntry first, WatcherEntry second)
    {
        return first.id == second.id && first.pathId == second.pathId;
    }

    friend bool operator<(WatcherEntry first, WatcherEntry second)
    {
        return std::tie(first.pathId, first.id) < std::tie(second.pathId, second.id);
    }

    friend bool operator<(WatcherEntry entry, FilePathId pathId)
    {
        return entry.pathId < pathId;
    }

    friend bool operator<(FilePathId pathId, WatcherEntry entry)
    {
        return pathId < entry.pathId;
    }

    operator FilePathId() const
    {
        return pathId;
    }
};

using WatcherEntries = std::vector<WatcherEntry>;

template <typename FileSystemWatcher,
          typename Timer>
class CLANGSUPPORT_GCCEXPORT ClangPathWatcher : public ClangPathWatcherInterface
{
public:
    ClangPathWatcher(FilePathCachingInterface &pathCache,
                     ClangPathWatcherNotifier *notifier=nullptr)
        : m_changedFilePathCompressor(pathCache),
          m_pathCache(pathCache),
          m_notifier(notifier)
    {
        QObject::connect(&m_fileSystemWatcher,
                         &FileSystemWatcher::fileChanged,
                         [&] (const QString &filePath) { compressChangedFilePath(filePath); });

        m_changedFilePathCompressor.setCallback([&] (ClangBackEnd::FilePathIds &&filePathIds) {
            addChangedPathForFilePath(std::move(filePathIds));
        });
    }

    ~ClangPathWatcher()
    {
        m_changedFilePathCompressor.setCallback([&] (FilePathIds &&) {});
    }

    void updateIdPaths(const std::vector<IdPaths> &idPaths) override
    {
        auto entriesAndIds = convertIdPathsToWatcherEntriesAndIds(idPaths);

        addEntries(entriesAndIds.first);
        removeUnusedEntries(entriesAndIds.first, entriesAndIds.second);
    }

    void removeIds(const ProjectPartIds &ids) override
    {
        auto removedEntries = removeIdsFromWatchedEntries(ids);

        auto filteredPaths = filterNotWatchedPaths(removedEntries);

        if (!filteredPaths.empty())
            m_fileSystemWatcher.removePaths(convertWatcherEntriesToQStringList(filteredPaths));
    }

    void setNotifier(ClangPathWatcherNotifier *notifier) override
    {
        m_notifier = notifier;
    }

    static std::vector<uint> idsFromIdPaths(const std::vector<IdPaths> &idPaths)
    {
        std::vector<uint> ids;
        ids.reserve(idPaths.size());

        auto extractId = [] (const IdPaths &idPath) {
            return idPath.id;
        };

        std::transform(idPaths.begin(),
                       idPaths.end(),
                       std::back_inserter(ids),
                       extractId);

        std::sort(ids.begin(), ids.end());

        return ids;
    }

    std::size_t sizeOfIdPaths(const std::vector<IdPaths> &idPaths)
    {
        auto sumSize = [] (std::size_t size, const IdPaths &idPath) {
            return size + idPath.filePathIds.size();
        };

        return std::accumulate(idPaths.begin(), idPaths.end(), std::size_t(0), sumSize);
    }

    std::pair<WatcherEntries, ProjectPartIds> convertIdPathsToWatcherEntriesAndIds(
        const std::vector<IdPaths> &idPaths)
    {
        WatcherEntries entries;
        entries.reserve(sizeOfIdPaths(idPaths));
        ProjectPartIds ids;
        ids.reserve(ids.size());

        auto outputIterator = std::back_inserter(entries);

        for (const IdPaths &idPath : idPaths)
        {
            ProjectPartId id = idPath.id;

            ids.push_back(id);

            outputIterator = std::transform(idPath.filePathIds.begin(),
                                            idPath.filePathIds.end(),
                                            outputIterator,
                                            [&] (FilePathId pathId) { return WatcherEntry{id, pathId}; });
        }

        std::sort(entries.begin(), entries.end());
        std::sort(ids.begin(), ids.end());

        return {entries, ids};
    }

    void addEntries(const WatcherEntries &entries)
    {
        auto newEntries = notWatchedEntries(entries);

        auto filteredPaths = filterNotWatchedPaths(newEntries);

        mergeToWatchedEntries(newEntries);

        if (!filteredPaths.empty())
            m_fileSystemWatcher.addPaths(convertWatcherEntriesToQStringList(filteredPaths));
    }

    void removeUnusedEntries(const WatcherEntries &entries, const ProjectPartIds &ids)
    {
        auto oldEntries = notAnymoreWatchedEntriesWithIds(entries, ids);

        removeFromWatchedEntries(oldEntries);

        auto filteredPaths = filterNotWatchedPaths(oldEntries);

        if (!filteredPaths.empty())
            m_fileSystemWatcher.removePaths(convertWatcherEntriesToQStringList(filteredPaths));
    }

    FileSystemWatcher &fileSystemWatcher()
    {
        return m_fileSystemWatcher;
    }

    QStringList convertWatcherEntriesToQStringList(
            const WatcherEntries &watcherEntries)
    {
        QStringList paths;
        paths.reserve(int(watcherEntries.size()));

        std::transform(watcherEntries.begin(),
                       watcherEntries.end(),
                       std::back_inserter(paths),
                       [&] (WatcherEntry entry) {
            return QString(m_pathCache.filePath(entry.pathId).path());
        });

        return paths;
    }

    template <typename Compare>
    WatcherEntries notWatchedEntries(const WatcherEntries &entries,
                                     Compare compare) const
    {
        WatcherEntries notWatchedEntries;
        notWatchedEntries.reserve(entries.size());

        std::set_difference(entries.begin(),
                            entries.end(),
                            m_watchedEntries.cbegin(),
                            m_watchedEntries.cend(),
                            std::back_inserter(notWatchedEntries),
                            compare);

        return notWatchedEntries;
    }

    WatcherEntries notWatchedEntries(const WatcherEntries &entries) const
    {
        return notWatchedEntries(entries, std::less<WatcherEntry>());
    }

    WatcherEntries notWatchedPaths(const WatcherEntries &entries) const
    {
        auto compare = [] (WatcherEntry first, WatcherEntry second) {
            return first.pathId < second.pathId;
        };

        return notWatchedEntries(entries, compare);
    }

    template <typename Compare>
    WatcherEntries notAnymoreWatchedEntries(
            const WatcherEntries &newEntries,
            Compare compare) const
    {
        WatcherEntries notAnymoreWatchedEntries;
        notAnymoreWatchedEntries.reserve(m_watchedEntries.size());

        std::set_difference(m_watchedEntries.cbegin(),
                            m_watchedEntries.cend(),
                            newEntries.begin(),
                            newEntries.end(),
                            std::back_inserter(notAnymoreWatchedEntries),
                            compare);

        return notAnymoreWatchedEntries;
    }

    WatcherEntries notAnymoreWatchedEntriesWithIds(const WatcherEntries &newEntries,
                                                   const ProjectPartIds &ids) const
    {
        auto oldEntries = notAnymoreWatchedEntries(newEntries, std::less<WatcherEntry>());

        auto newEnd = std::remove_if(oldEntries.begin(),
                                     oldEntries.end(),
                                     [&] (WatcherEntry entry) {
                return !std::binary_search(ids.begin(), ids.end(), entry.id);
        });

        oldEntries.erase(newEnd, oldEntries.end());

        return oldEntries;
    }

    void mergeToWatchedEntries(const WatcherEntries &newEntries)
    {
        WatcherEntries newWatchedEntries;
        newWatchedEntries.reserve(m_watchedEntries.size() + newEntries.size());

        std::merge(m_watchedEntries.cbegin(),
                   m_watchedEntries.cend(),
                   newEntries.begin(),
                   newEntries.end(),
                   std::back_inserter(newWatchedEntries));

        m_watchedEntries = std::move(newWatchedEntries);
    }

    static
    WatcherEntries uniquePaths(const WatcherEntries &pathEntries)
    {
        WatcherEntries uniqueEntries;
        uniqueEntries.reserve(pathEntries.size());

        auto compare = [] (WatcherEntry first, WatcherEntry second) {
            return first.pathId == second.pathId;
        };

        std::unique_copy(pathEntries.begin(),
                         pathEntries.end(),
                         std::back_inserter(uniqueEntries),
                         compare);

        return uniqueEntries;
    }

    WatcherEntries filterNotWatchedPaths(const WatcherEntries &entries)
    {
        return notWatchedPaths(uniquePaths(entries));
    }

    const WatcherEntries &watchedEntries() const
    {
        return m_watchedEntries;
    }

    WatcherEntries removeIdsFromWatchedEntries(const ProjectPartIds &ids)
    {
        auto keep = [&](WatcherEntry entry) {
            return !std::binary_search(ids.begin(), ids.end(), entry.id);
        };

        auto found = std::stable_partition(m_watchedEntries.begin(), m_watchedEntries.end(), keep);

        WatcherEntries removedEntries(found, m_watchedEntries.end());

        m_watchedEntries.erase(found, m_watchedEntries.end());

        return removedEntries;
    }

    void removeFromWatchedEntries(const WatcherEntries &oldEntries)
    {
        WatcherEntries newWatchedEntries;
        newWatchedEntries.reserve(m_watchedEntries.size() - oldEntries.size());

        std::set_difference(m_watchedEntries.cbegin(),
                            m_watchedEntries.cend(),
                            oldEntries.begin(),
                            oldEntries.end(),
                            std::back_inserter(newWatchedEntries));


        m_watchedEntries = newWatchedEntries;
    }

    void compressChangedFilePath(const QString &filePath)
    {
        m_changedFilePathCompressor.addFilePath(filePath);
    }

    WatcherEntries watchedEntriesForPaths(ClangBackEnd::FilePathIds &&filePathIds)
    {
        WatcherEntries foundEntries;
        foundEntries.reserve(filePathIds.size());

        for (FilePathId pathId : filePathIds) {
            auto range = std::equal_range(m_watchedEntries.begin(), m_watchedEntries.end(), pathId);
            foundEntries.insert(foundEntries.end(), range.first, range.second);
        }

        return foundEntries;
    }

    FilePathIds watchedPaths(const FilePathIds &filePathIds) const
    {
        FilePathIds watchedFilePathIds;
        watchedFilePathIds.reserve(filePathIds.size());

        std::set_intersection(m_watchedEntries.begin(),
                              m_watchedEntries.end(),
                              filePathIds.begin(),
                              filePathIds.end(),
                              std::back_inserter(watchedFilePathIds));

        return watchedFilePathIds;
    }

    ProjectPartIds idsForWatcherEntries(const WatcherEntries &foundEntries)
    {
        ProjectPartIds ids;
        ids.reserve(foundEntries.size());

        std::transform(foundEntries.begin(),
                       foundEntries.end(),
                       std::back_inserter(ids),
                       [&](WatcherEntry entry) { return entry.id; });

        return ids;
    }

    ProjectPartIds uniqueIds(ProjectPartIds &&ids)
    {
        std::sort(ids.begin(), ids.end());
        auto newEnd = std::unique(ids.begin(), ids.end());
        ids.erase(newEnd, ids.end());

        return std::move(ids);
    }

    void addChangedPathForFilePath(FilePathIds &&filePathIds)
    {
        if (m_notifier) {
            WatcherEntries foundEntries = watchedEntriesForPaths(std::move(filePathIds));

            ProjectPartIds changedIds = idsForWatcherEntries(foundEntries);

            m_notifier->pathsWithIdsChanged(uniqueIds(std::move(changedIds)));
            m_notifier->pathsChanged(watchedPaths(filePathIds));
        }
    }

    FilePathCachingInterface &pathCache()
    {
        return m_pathCache;
    }

private:
    WatcherEntries m_watchedEntries;
    ChangedFilePathCompressor<Timer> m_changedFilePathCompressor;
    FileSystemWatcher m_fileSystemWatcher;
    FilePathCachingInterface &m_pathCache;
    ClangPathWatcherNotifier *m_notifier;
};

} // namespace ClangBackEnd
