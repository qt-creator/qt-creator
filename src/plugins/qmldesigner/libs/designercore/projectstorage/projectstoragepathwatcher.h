// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "directorypathcompressor.h"
#include "filesystem.h"
#include "projectstoragepathwatcherinterface.h"
#include "projectstoragepathwatchernotifierinterface.h"
#include "projectstoragepathwatchertypes.h"
#include "projectstoragetriggerupdateinterface.h"

#include <sourcepathstorage/storagecache.h>

#include <utils/algorithm.h>
#include <utils/set_algorithm.h>

#include <QTimer>

namespace QmlDesigner {

template<typename FileSystemWatcher, typename Timer, class SourcePathCache>
class ProjectStoragePathWatcher : public ProjectStoragePathWatcherInterface,
                                  public ProjectStorageTriggerUpdateInterface
{
public:
    ProjectStoragePathWatcher(SourcePathCache &pathCache,
                              FileStatusCache &fileStatusCache,
                              ProjectStoragePathWatcherNotifierInterface *notifier = nullptr)
        : m_fileStatusCache(fileStatusCache)
        , m_pathCache(pathCache)
        , m_notifier(notifier)
    {
        QObject::connect(&m_fileSystemWatcher,
                         &FileSystemWatcher::directoryChanged,
                         [&](const QString &path) { compressChangedDirectoryPath(path); });

        m_directoryPathCompressor.setCallback(
            [&](const QmlDesigner::SourceContextIds &sourceContextIds) {
                addChangedPathForFilePath(sourceContextIds);
            });
    }

    void updateIdPaths(const std::vector<IdPaths> &idPaths) override
    {
        const auto &[entires, ids] = convertIdPathsToWatcherEntriesAndIds(idPaths);

        addEntries(entires);

        auto notContainsdId = [&, &ids = ids](WatcherEntry entry) {
            return !std::ranges::binary_search(ids, entry.id);
        };
        removeUnusedEntries(entires, notContainsdId);
    }

    void updateContextIdPaths(const std::vector<IdPaths> &idPaths,
                              const SourceContextIds &sourceContextIds) override
    {
        const auto &[entires, ids] = convertIdPathsToWatcherEntriesAndIds(idPaths);

        addEntries(entires);

        auto notContainsId = [&, &ids = ids](WatcherEntry entry) {
            return !std::ranges::binary_search(ids, entry.id)
                   || !std::ranges::binary_search(sourceContextIds, entry.sourceContextId);
        };

        removeUnusedEntries(entires, notContainsId);
    }

    void checkForChangeInDirectory(SourceContextIds sourceContextIds) override
    {
        std::ranges::sort(sourceContextIds);
        auto removed = std::ranges::unique(sourceContextIds);
        sourceContextIds.erase(removed.begin(), removed.end());

        addChangedPathForFilePath(sourceContextIds);
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

        for (const IdPaths &idPath : idPaths)
        {
            ProjectChunkId id = idPath.id;

            ids.push_back(id);

            std::ranges::transform(idPath.sourceIds, std::back_inserter(entries), [&](SourceId sourceId) {
                return WatcherEntry{id,
                                    sourceId.contextId(),
                                    sourceId,
                                    m_fileStatusCache.lastModifiedTime(sourceId)};
            });
        }

        std::ranges::sort(entries);
        std::ranges::sort(ids);

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
            watcherEntries, &WatcherEntry::sourceContextId);

        std::ranges::sort(sourceContextIds);
        auto removed = std::ranges::unique(sourceContextIds);
        sourceContextIds.erase(removed.begin(), removed.end());

        return convertWatcherEntriesToDirectoryPathList(sourceContextIds);
    }

    WatcherEntries notWatchedEntries(const WatcherEntries &entries) const
    {
        WatcherEntries notWatchedEntries;
        notWatchedEntries.reserve(entries.size());

        std::ranges::set_difference(entries, m_watchedEntries, std::back_inserter(notWatchedEntries));

        return notWatchedEntries;
    }

    SourceContextIds notWatchedPaths(const SourceContextIds &ids) const
    {
        SourceContextIds notWatchedDirectoryIds;
        notWatchedDirectoryIds.reserve(ids.size());

        std::ranges::set_difference(ids, m_watchedEntries, std::back_inserter(notWatchedDirectoryIds));

        return notWatchedDirectoryIds;
    }

    template<typename Compare>
    WatcherEntries notAnymoreWatchedEntries(const WatcherEntries &newEntries, Compare compare) const
    {
        WatcherEntries notAnymoreWatchedEntries;
        notAnymoreWatchedEntries.reserve(m_watchedEntries.size());

        std::ranges::set_difference(m_watchedEntries,
                                    newEntries,
                                    std::back_inserter(notAnymoreWatchedEntries),
                                    compare);

        return notAnymoreWatchedEntries;
    }

    template<typename Filter>
    WatcherEntries notAnymoreWatchedEntriesWithIds(const WatcherEntries &newEntries, Filter filter) const
    {
        auto oldEntries = notAnymoreWatchedEntries(newEntries, std::ranges::less{});

        std::erase_if(oldEntries, filter);

        return oldEntries;
    }

    void mergeToWatchedEntries(const WatcherEntries &newEntries)
    {
        WatcherEntries newWatchedEntries;
        newWatchedEntries.reserve(m_watchedEntries.size() + newEntries.size());

        std::ranges::merge(m_watchedEntries, newEntries, std::back_inserter(newWatchedEntries));

        m_watchedEntries = std::move(newWatchedEntries);
    }

    static SourceContextIds uniquePaths(const WatcherEntries &pathEntries)
    {
        SourceContextIds uniqueDirectoryIds;
        uniqueDirectoryIds.reserve(pathEntries.size());

        std::ranges::unique_copy(pathEntries,
                                 std::back_inserter(uniqueDirectoryIds),
                                 {},
                                 &WatcherEntry::sourceContextId);

        return uniqueDirectoryIds;
    }

    SourceContextIds filterNotWatchedPaths(const WatcherEntries &entries) const
    {
        return notWatchedPaths(uniquePaths(entries));
    }

    const WatcherEntries &watchedEntries() const { return m_watchedEntries; }

    WatcherEntries removeIdsFromWatchedEntries(const ProjectPartIds &ids)
    {
        auto keep = [&](WatcherEntry entry) { return !std::ranges::binary_search(ids, entry.id.id); };

        auto removed = std::ranges::stable_partition(m_watchedEntries, keep);

        WatcherEntries removedEntries(removed.begin(), removed.end());

        m_watchedEntries.erase(removed.begin(), removed.end());

        return removedEntries;
    }

    void removeFromWatchedEntries(const WatcherEntries &oldEntries)
    {
        WatcherEntries newWatchedEntries;
        newWatchedEntries.reserve(m_watchedEntries.size() - oldEntries.size());

        std::ranges::set_difference(m_watchedEntries,
                                    oldEntries,
                                    std::back_inserter(newWatchedEntries));

        m_watchedEntries = std::move(newWatchedEntries);
    }

    void compressChangedDirectoryPath(const QString &path)
    {
        m_directoryPathCompressor.addSourceContextId(
            m_pathCache.sourceContextId(Utils::PathString{path}));
    }

    WatcherEntries watchedEntriesForPaths(const QmlDesigner::SourceContextIds &sourceContextIds)
    {
        WatcherEntries foundEntries;
        foundEntries.reserve(m_watchedEntries.size());

        Utils::set_greedy_intersection(
            m_watchedEntries,
            sourceContextIds,
            [&](WatcherEntry &entry) {
                m_fileStatusCache.update(entry.sourceId);
                auto currentLastModified = m_fileStatusCache.lastModifiedTime(entry.sourceId);
                if (entry.lastModified < currentLastModified) {
                    foundEntries.push_back(entry);
                    entry.lastModified = currentLastModified;
                }
            },
            {},
            &WatcherEntry::sourceContextId);

        return foundEntries;
    }

    SourceIds watchedPaths(const WatcherEntries &entries) const
    {
        auto sourceIds = Utils::transform<SourceIds>(entries, &WatcherEntry::sourceId);

        std::ranges::sort(sourceIds);
        auto removed = std::ranges::unique(sourceIds);
        sourceIds.erase(removed.begin(), removed.end());

        return sourceIds;
    }

    std::vector<IdPaths> idPathsForWatcherEntries(WatcherEntries &&foundEntries)
    {
        std::ranges::sort(foundEntries, {}, [](const WatcherEntry &entry) {
            return std::tie(entry.id, entry.sourceId);
        });

        std::vector<IdPaths> idPaths;
        idPaths.reserve(foundEntries.size());

        if (foundEntries.size()) {
            idPaths.emplace_back(foundEntries.front().id, SourceIds{});

            for (WatcherEntry entry : foundEntries) {
                if (idPaths.back().id != entry.id)
                    idPaths.emplace_back(entry.id, SourceIds{});

                idPaths.back().sourceIds.push_back(entry.sourceId);
            }
        }

        return idPaths;
    }

    void addChangedPathForFilePath(const SourceContextIds &sourceContextIds)
    {
        if (m_notifier) {
            WatcherEntries foundEntries = watchedEntriesForPaths(sourceContextIds);

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
    FileStatusCache &m_fileStatusCache;
    SourcePathCache &m_pathCache;
    ProjectStoragePathWatcherNotifierInterface *m_notifier;
    DirectoryPathCompressor<Timer> m_directoryPathCompressor;
};

} // namespace QmlDesigner
