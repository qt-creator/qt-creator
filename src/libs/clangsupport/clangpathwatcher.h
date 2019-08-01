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
#include "directorypathcompressor.h"
#include "filepathcachinginterface.h"
#include "filesystem.h"
#include "stringcache.h"

#include <utils/algorithm.h>

#include <QTimer>

namespace ClangBackEnd {

template<class InputIt1, class InputIt2, class Callable>
void set_greedy_intersection_call(
    InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2, Callable callable)
{
    while (first1 != last1 && first2 != last2) {
        if (*first1 < *first2) {
            ++first1;
        } else {
            if (*first2 < *first1)
                ++first2;
            else
                callable(*first1++);
        }
    }
}

class WatcherEntry
{
public:
    ProjectChunkId id;
    DirectoryPathId directoryPathId;
    FilePathId filePathId;
    long long lastModified = -1;

    friend bool operator==(WatcherEntry first, WatcherEntry second)
    {
        return first.id == second.id && first.directoryPathId == second.directoryPathId
               && first.filePathId == second.filePathId;
    }

    friend bool operator<(WatcherEntry first, WatcherEntry second)
    {
        return std::tie(first.directoryPathId, first.filePathId, first.id)
               < std::tie(second.directoryPathId, second.filePathId, second.id);
    }

    friend bool operator<(DirectoryPathId directoryPathId, WatcherEntry entry)
    {
        return directoryPathId < entry.directoryPathId;
    }

    friend bool operator<(WatcherEntry entry, DirectoryPathId directoryPathId)
    {
        return entry.directoryPathId < directoryPathId;
    }

    operator FilePathId() const { return filePathId; }

    operator DirectoryPathId() const { return directoryPathId; }
};

using WatcherEntries = std::vector<WatcherEntry>;

template<typename FileSystemWatcher, typename Timer>
class CLANGSUPPORT_GCCEXPORT ClangPathWatcher : public ClangPathWatcherInterface
{
public:
    ClangPathWatcher(FilePathCachingInterface &pathCache,
                     FileSystemInterface &fileSystem,
                     ClangPathWatcherNotifier *notifier = nullptr)
        : m_fileStatusCache(fileSystem)
        , m_fileSystem(fileSystem)
        , m_pathCache(pathCache)
        , m_notifier(notifier)
    {
        QObject::connect(&m_fileSystemWatcher,
                         &FileSystemWatcher::directoryChanged,
                         [&](const QString &path) { compressChangedDirectoryPath(path); });

        m_directoryPathCompressor.setCallback([&](ClangBackEnd::DirectoryPathIds &&directoryPathIds) {
            addChangedPathForFilePath(std::move(directoryPathIds));
        });
    }

    ~ClangPathWatcher()
    {
        m_directoryPathCompressor.setCallback([&](DirectoryPathIds &&) {});
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
            m_fileSystemWatcher.removePaths(convertWatcherEntriesToDirectoryPathList(filteredPaths));
    }

    void setNotifier(ClangPathWatcherNotifier *notifier) override
    {
        m_notifier = notifier;
    }

    std::size_t sizeOfIdPaths(const std::vector<IdPaths> &idPaths)
    {
        auto sumSize = [] (std::size_t size, const IdPaths &idPath) {
            return size + idPath.filePathIds.size();
        };

        return std::accumulate(idPaths.begin(), idPaths.end(), std::size_t(0), sumSize);
    }

    std::pair<WatcherEntries, ProjectChunkIds> convertIdPathsToWatcherEntriesAndIds(
        const std::vector<IdPaths> &idPaths)
    {
        WatcherEntries entries;
        entries.reserve(sizeOfIdPaths(idPaths));
        ProjectChunkIds ids;
        ids.reserve(ids.size());

        auto outputIterator = std::back_inserter(entries);

        for (const IdPaths &idPath : idPaths)
        {
            ProjectChunkId id = idPath.id;

            ids.push_back(id);

            outputIterator = std::transform(idPath.filePathIds.begin(),
                                            idPath.filePathIds.end(),
                                            outputIterator,
                                            [&](FilePathId filePathId) {
                                                return WatcherEntry{
                                                    id,
                                                    m_pathCache.directoryPathId(filePathId),
                                                    filePathId,
                                                    m_fileStatusCache.lastModifiedTime(filePathId)};
                                            });
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
            m_fileSystemWatcher.addPaths(convertWatcherEntriesToDirectoryPathList(filteredPaths));
    }

    void removeUnusedEntries(const WatcherEntries &entries, const ProjectChunkIds &ids)
    {
        auto oldEntries = notAnymoreWatchedEntriesWithIds(entries, ids);

        removeFromWatchedEntries(oldEntries);

        auto filteredPaths = filterNotWatchedPaths(oldEntries);

        if (!filteredPaths.empty())
            m_fileSystemWatcher.removePaths(convertWatcherEntriesToDirectoryPathList(filteredPaths));
    }

    FileSystemWatcher &fileSystemWatcher() { return m_fileSystemWatcher; }

    QStringList convertWatcherEntriesToDirectoryPathList(const DirectoryPathIds &directoryPathIds) const
    {
        return Utils::transform<QStringList>(directoryPathIds, [&](DirectoryPathId id) {
            return QString(m_pathCache.directoryPath(id));
        });
    }

    QStringList convertWatcherEntriesToDirectoryPathList(const WatcherEntries &watcherEntries) const
    {
        DirectoryPathIds directoryPathIds = Utils::transform<DirectoryPathIds>(
            watcherEntries, [&](WatcherEntry entry) { return entry.directoryPathId; });

        std::sort(directoryPathIds.begin(), directoryPathIds.end());
        directoryPathIds.erase(std::unique(directoryPathIds.begin(), directoryPathIds.end()),
                               directoryPathIds.end());

        return convertWatcherEntriesToDirectoryPathList(directoryPathIds);
    }

    WatcherEntries notWatchedEntries(const WatcherEntries &entries) const
    {
        WatcherEntries notWatchedEntries;
        notWatchedEntries.reserve(entries.size());

        std::set_difference(entries.begin(),
                            entries.end(),
                            m_watchedEntries.cbegin(),
                            m_watchedEntries.cend(),
                            std::back_inserter(notWatchedEntries));

        return notWatchedEntries;
    }

    DirectoryPathIds notWatchedPaths(const DirectoryPathIds &ids) const
    {
        DirectoryPathIds notWatchedDirectoryIds;
        notWatchedDirectoryIds.reserve(ids.size());

        std::set_difference(ids.begin(),
                            ids.end(),
                            m_watchedEntries.cbegin(),
                            m_watchedEntries.cend(),
                            std::back_inserter(notWatchedDirectoryIds));

        return notWatchedDirectoryIds;
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
                                                   const ProjectChunkIds &ids) const
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

    static DirectoryPathIds uniquePaths(const WatcherEntries &pathEntries)
    {
        DirectoryPathIds uniqueDirectoryIds;
        uniqueDirectoryIds.reserve(pathEntries.size());

        auto compare = [](WatcherEntry first, WatcherEntry second) {
            return first.directoryPathId == second.directoryPathId;
        };

        std::unique_copy(pathEntries.begin(),
                         pathEntries.end(),
                         std::back_inserter(uniqueDirectoryIds),
                         compare);

        return uniqueDirectoryIds;
    }

    DirectoryPathIds filterNotWatchedPaths(const WatcherEntries &entries) const
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

        m_watchedEntries = std::move(newWatchedEntries);
    }

    void compressChangedDirectoryPath(const QString &path)
    {
        m_directoryPathCompressor.addDirectoryPathId(
            m_pathCache.directoryPathId(Utils::PathString{path}));
    }

    WatcherEntries watchedEntriesForPaths(ClangBackEnd::DirectoryPathIds &&directoryPathIds)
    {
        WatcherEntries foundEntries;
        foundEntries.reserve(m_watchedEntries.size());

        set_greedy_intersection_call(m_watchedEntries.begin(),
                                     m_watchedEntries.end(),
                                     directoryPathIds.begin(),
                                     directoryPathIds.end(),
                                     [&](WatcherEntry &entry) {
                                         m_fileStatusCache.update(entry.filePathId);
                                         auto currentLastModified = m_fileStatusCache.lastModifiedTime(
                                             entry.filePathId);
                                         if (entry.lastModified < currentLastModified) {
                                             foundEntries.push_back(entry);
                                             entry.lastModified = currentLastModified;
                                         }
                                     });

        return foundEntries;
    }

    FilePathIds watchedPaths(const WatcherEntries &entries) const
    {
        auto filePathIds = Utils::transform<FilePathIds>(entries, &WatcherEntry::filePathId);

        std::sort(filePathIds.begin(), filePathIds.end());

        filePathIds.erase(std::unique(filePathIds.begin(), filePathIds.end()), filePathIds.end());

        return filePathIds;
    }

    std::vector<IdPaths> idPathsForWatcherEntries(WatcherEntries &&foundEntries)
    {
        std::sort(foundEntries.begin(), foundEntries.end(), [](WatcherEntry first, WatcherEntry second) {
            return std::tie(first.id, first.filePathId) < std::tie(second.id, second.filePathId);
        });

        std::vector<IdPaths> idPaths;
        idPaths.reserve(foundEntries.size());

        for (WatcherEntry entry : foundEntries) {
            if (idPaths.empty() || idPaths.back().id != entry.id)
                idPaths.emplace_back(entry.id, FilePathIds{});
            idPaths.back().filePathIds.push_back(entry.filePathId);
        }

        return idPaths;
    }

    void addChangedPathForFilePath(DirectoryPathIds &&directoryPathIds)
    {
        if (m_notifier) {
            WatcherEntries foundEntries = watchedEntriesForPaths(std::move(directoryPathIds));

            FilePathIds watchedFilePathIds = watchedPaths(foundEntries);

            std::vector<IdPaths> changedIdPaths = idPathsForWatcherEntries(std::move(foundEntries));

            m_notifier->pathsChanged(watchedFilePathIds);
            m_notifier->pathsWithIdsChanged(changedIdPaths);
        }
    }

    FilePathCachingInterface &pathCache()
    {
        return m_pathCache;
    }

private:
    WatcherEntries m_watchedEntries;
    FileSystemWatcher m_fileSystemWatcher;
    FileStatusCache m_fileStatusCache;
    FileSystemInterface &m_fileSystem;
    FilePathCachingInterface &m_pathCache;
    ClangPathWatcherNotifier *m_notifier;
    DirectoryPathCompressor<Timer> m_directoryPathCompressor;
};

} // namespace ClangBackEnd
