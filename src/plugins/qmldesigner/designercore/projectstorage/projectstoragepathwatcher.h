// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "directorypathcompressor.h"
#include "filesystem.h"
#include "projectstoragepathwatcherinterface.h"
#include "projectstoragepathwatchernotifierinterface.h"
#include "projectstoragepathwatchertypes.h"
#include "storagecache.h"

#include <utils/algorithm.h>

#include <QTimer>

namespace QmlDesigner {

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

template<typename FileSystemWatcher, typename Timer, class SourcePathCache>
class ProjectStoragePathWatcher : public ProjectStoragePathWatcherInterface
{
public:
    ProjectStoragePathWatcher(SourcePathCache &pathCache,
                              FileSystemInterface &fileSystem,
                              ProjectStoragePathWatcherNotifierInterface *notifier = nullptr)
        : m_fileStatusCache(fileSystem)
        , m_fileSystem(fileSystem)
        , m_pathCache(pathCache)
        , m_notifier(notifier)
    {
        QObject::connect(&m_fileSystemWatcher,
                         &FileSystemWatcher::directoryChanged,
                         [&](const QString &path) { compressChangedDirectoryPath(path); });

        m_directoryPathCompressor.setCallback([&](QmlDesigner::SourceContextIds &&sourceContextIds) {
            addChangedPathForFilePath(std::move(sourceContextIds));
        });
    }

    ~ProjectStoragePathWatcher()
    {
        m_directoryPathCompressor.setCallback([&](SourceContextIds &&) {});
    }

    void updateIdPaths(const std::vector<IdPaths> &idPaths) override
    {
        const auto &[entires, ids] = convertIdPathsToWatcherEntriesAndIds(idPaths);

        addEntries(entires);

        auto notContainsdId = [&, &ids = ids](WatcherEntry entry) {
            return !std::binary_search(ids.begin(), ids.end(), entry.id);
        };
        removeUnusedEntries(entires, notContainsdId);
    }

    void updateContextIdPaths(const std::vector<IdPaths> &idPaths,
                              const SourceContextIds &sourceContextIds) override
    {
        const auto &[entires, ids] = convertIdPathsToWatcherEntriesAndIds(idPaths);

        addEntries(entires);

        auto notContainsdId = [&, &ids = ids](WatcherEntry entry) {
            return !std::binary_search(ids.begin(), ids.end(), entry.id)
                   || !std::binary_search(sourceContextIds.begin(),
                                          sourceContextIds.end(),
                                          entry.sourceContextId);
        };

        removeUnusedEntries(entires, notContainsdId);
    }

    void removeIds(const ProjectPartIds &ids) override
    {
        auto removedEntries = removeIdsFromWatchedEntries(ids);

        auto filteredPaths = filterNotWatchedPaths(removedEntries);

        if (!filteredPaths.empty())
            m_fileSystemWatcher.removePaths(convertWatcherEntriesToDirectoryPathList(filteredPaths));
    }

    void setNotifier(ProjectStoragePathWatcherNotifierInterface *notifier) override
    {
        m_notifier = notifier;
    }

    std::size_t sizeOfIdPaths(const std::vector<IdPaths> &idPaths)
    {
        auto sumSize = [](std::size_t size, const IdPaths &idPath) {
            return size + idPath.sourceIds.size();
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

            outputIterator = std::transform(
                idPath.sourceIds.begin(), idPath.sourceIds.end(), outputIterator, [&](SourceId sourceId) {
                    return WatcherEntry{id,
                                        m_pathCache.sourceContextId(sourceId),
                                        sourceId,
                                        m_fileStatusCache.lastModifiedTime(sourceId)};
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

    template<typename Filter>
    void removeUnusedEntries(const WatcherEntries &entries, Filter filter)
    {
        auto oldEntries = notAnymoreWatchedEntriesWithIds(entries, filter);

        removeFromWatchedEntries(oldEntries);

        auto filteredPaths = filterNotWatchedPaths(oldEntries);

        if (!filteredPaths.empty())
            m_fileSystemWatcher.removePaths(convertWatcherEntriesToDirectoryPathList(filteredPaths));
    }

    FileSystemWatcher &fileSystemWatcher() { return m_fileSystemWatcher; }

    QStringList convertWatcherEntriesToDirectoryPathList(const SourceContextIds &sourceContextIds) const
    {
        return Utils::transform<QStringList>(sourceContextIds, [&](SourceContextId id) {
            return QString(m_pathCache.sourceContextPath(id));
        });
    }

    QStringList convertWatcherEntriesToDirectoryPathList(const WatcherEntries &watcherEntries) const
    {
        SourceContextIds sourceContextIds = Utils::transform<SourceContextIds>(
            watcherEntries, [&](WatcherEntry entry) { return entry.sourceContextId; });

        std::sort(sourceContextIds.begin(), sourceContextIds.end());
        sourceContextIds.erase(std::unique(sourceContextIds.begin(), sourceContextIds.end()),
                               sourceContextIds.end());

        return convertWatcherEntriesToDirectoryPathList(sourceContextIds);
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

    SourceContextIds notWatchedPaths(const SourceContextIds &ids) const
    {
        SourceContextIds notWatchedDirectoryIds;
        notWatchedDirectoryIds.reserve(ids.size());

        std::set_difference(ids.begin(),
                            ids.end(),
                            m_watchedEntries.cbegin(),
                            m_watchedEntries.cend(),
                            std::back_inserter(notWatchedDirectoryIds));

        return notWatchedDirectoryIds;
    }

    template<typename Compare>
    WatcherEntries notAnymoreWatchedEntries(const WatcherEntries &newEntries, Compare compare) const
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

    template<typename Filter>
    WatcherEntries notAnymoreWatchedEntriesWithIds(const WatcherEntries &newEntries, Filter filter) const
    {
        auto oldEntries = notAnymoreWatchedEntries(newEntries, std::less<WatcherEntry>());

        auto newEnd = std::remove_if(oldEntries.begin(), oldEntries.end(), filter);

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

    static SourceContextIds uniquePaths(const WatcherEntries &pathEntries)
    {
        SourceContextIds uniqueDirectoryIds;
        uniqueDirectoryIds.reserve(pathEntries.size());

        auto compare = [](WatcherEntry first, WatcherEntry second) {
            return first.sourceContextId == second.sourceContextId;
        };

        std::unique_copy(pathEntries.begin(),
                         pathEntries.end(),
                         std::back_inserter(uniqueDirectoryIds),
                         compare);

        return uniqueDirectoryIds;
    }

    SourceContextIds filterNotWatchedPaths(const WatcherEntries &entries) const
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
        m_directoryPathCompressor.addSourceContextId(
            m_pathCache.sourceContextId(Utils::PathString{path}));
    }

    WatcherEntries watchedEntriesForPaths(QmlDesigner::SourceContextIds &&sourceContextIds)
    {
        WatcherEntries foundEntries;
        foundEntries.reserve(m_watchedEntries.size());

        set_greedy_intersection_call(m_watchedEntries.begin(),
                                     m_watchedEntries.end(),
                                     sourceContextIds.begin(),
                                     sourceContextIds.end(),
                                     [&](WatcherEntry &entry) {
                                         m_fileStatusCache.update(entry.sourceId);
                                         auto currentLastModified = m_fileStatusCache.lastModifiedTime(
                                             entry.sourceId);
                                         if (entry.lastModified < currentLastModified) {
                                             foundEntries.push_back(entry);
                                             entry.lastModified = currentLastModified;
                                         }
                                     });

        return foundEntries;
    }

    SourceIds watchedPaths(const WatcherEntries &entries) const
    {
        auto sourceIds = Utils::transform<SourceIds>(entries, &WatcherEntry::sourceId);

        std::sort(sourceIds.begin(), sourceIds.end());

        sourceIds.erase(std::unique(sourceIds.begin(), sourceIds.end()), sourceIds.end());

        return sourceIds;
    }

    std::vector<IdPaths> idPathsForWatcherEntries(WatcherEntries &&foundEntries)
    {
        std::sort(foundEntries.begin(), foundEntries.end(), [](WatcherEntry first, WatcherEntry second) {
            return std::tie(first.id, first.sourceId) < std::tie(second.id, second.sourceId);
        });

        std::vector<IdPaths> idPaths;
        idPaths.reserve(foundEntries.size());

        for (WatcherEntry entry : foundEntries) {
            if (idPaths.empty() || idPaths.back().id != entry.id)
                idPaths.emplace_back(entry.id, SourceIds{});
            idPaths.back().sourceIds.push_back(entry.sourceId);
        }

        return idPaths;
    }

    void addChangedPathForFilePath(SourceContextIds &&sourceContextIds)
    {
        if (m_notifier) {
            WatcherEntries foundEntries = watchedEntriesForPaths(std::move(sourceContextIds));

            SourceIds watchedSourceIds = watchedPaths(foundEntries);

            std::vector<IdPaths> changedIdPaths = idPathsForWatcherEntries(std::move(foundEntries));

            m_notifier->pathsChanged(watchedSourceIds);
            m_notifier->pathsWithIdsChanged(changedIdPaths);
        }
    }

    SourcePathCache &pathCache() { return m_pathCache; }

private:
    WatcherEntries m_watchedEntries;
    FileSystemWatcher m_fileSystemWatcher;
    FileStatusCache m_fileStatusCache;
    FileSystemInterface &m_fileSystem;
    SourcePathCache &m_pathCache;
    ProjectStoragePathWatcherNotifierInterface *m_notifier;
    DirectoryPathCompressor<Timer> m_directoryPathCompressor;
};

} // namespace QmlDesigner
