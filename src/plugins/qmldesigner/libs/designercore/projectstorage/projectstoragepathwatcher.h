// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "directorypathcompressor.h"
#include "filesystem.h"
#include "projectstoragepathwatcherinterface.h"
#include "projectstoragepathwatchernotifierinterface.h"
#include "projectstoragepathwatchertypes.h"
#include "projectstoragetracing.h"
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
        NanotraceHR::Tracer tracer{"project storage path watcher constructor",
                                   ProjectStorageTracing::category()};

        QObject::connect(&m_fileSystemWatcher,
                         &FileSystemWatcher::directoryChanged,
                         [&](const QString &path) { compressChangedDirectoryPath(path); });

        m_directoryPathCompressor.setCallback(
            [&](const QmlDesigner::DirectoryPathIds &directoryPathIds) {
                addChangedPathForFilePath(directoryPathIds);
            });
    }

    void updateIdPaths(const std::vector<IdPaths> &idPaths) override
    {
        NanotraceHR::Tracer tracer{"project storage path watcher update id paths",
                                   ProjectStorageTracing::category()};

        const auto &[entires, ids] = convertIdPathsToWatcherEntriesAndIds(idPaths);

        addEntries(entires);

        auto containsdId = [&, &ids = ids](WatcherEntry entry) {
            return std::ranges::binary_search(ids, entry.id);
        };
        removeUnusedEntries(entires, containsdId);
    }

    void updateContextIdPaths(const std::vector<IdPaths> &idPaths,
                              const DirectoryPathIds &directoryPathIds) override
    {
        NanotraceHR::Tracer tracer{"project storage path watcher update context id paths",
                                   ProjectStorageTracing::category()};

        const auto &[entires, ids] = convertIdPathsToWatcherEntriesAndIds(idPaths);

        addEntries(entires);

        auto isInDirectory = [&](WatcherEntry entry) {
            return std::ranges::binary_search(directoryPathIds, entry.directoryPathId);
        };

        removeUnusedEntries(entires, isInDirectory);
    }

    void checkForChangeInDirectory(DirectoryPathIds directoryPathIds) override
    {
        NanotraceHR::Tracer tracer{"project storage path watcher check for change in directory",
                                   ProjectStorageTracing::category()};

        std::ranges::sort(directoryPathIds);
        auto removed = std::ranges::unique(directoryPathIds);
        directoryPathIds.erase(removed.begin(), removed.end());

        addChangedPathForFilePath(directoryPathIds);
    }

    void removeIds(const ProjectPartIds &ids) override
    {
        NanotraceHR::Tracer tracer{"project storage path watcher remove ids",
                                   ProjectStorageTracing::category()};

        auto removedEntries = removeIdsFromWatchedEntries(ids);

        auto filteredPaths = filterNotWatchedPaths(removedEntries);

        if (!filteredPaths.empty()) {
            auto paths = convertWatcherEntriesToDirectoryPathList(filteredPaths);
            tracer.tick("remove paths", NanotraceHR::keyValue("paths", paths));
            m_fileSystemWatcher.removePaths(paths);
        }
    }

    void setNotifier(ProjectStoragePathWatcherNotifierInterface *notifier) override
    {
        NanotraceHR::Tracer tracer{"project storage path watcher set notifier",
                                   ProjectStorageTracing::category()};

        m_notifier = notifier;
    }

    std::size_t sizeOfIdPaths(const std::vector<IdPaths> &idPaths)
    {
        NanotraceHR::Tracer tracer{"project storage path watcher size of id paths",
                                   ProjectStorageTracing::category()};

        auto sumSize = [](std::size_t size, const IdPaths &idPath) {
            return size + idPath.sourceIds.size();
        };

        return std::accumulate(idPaths.begin(), idPaths.end(), std::size_t(0), sumSize);
    }

    static Utils::SmallStringView parentDirectoryPath(Utils::SmallStringView directoryPath)
    {
        auto found = std::find(directoryPath.rbegin(), directoryPath.rend(), '/');

        if (found == directoryPath.rend())
            return {};

        return {directoryPath.begin(), std::prev(found.base())};
    }

    std::tuple<DirectoryPathId, DirectoryPathId> createDirectoryPathId(SourceId sourceId)
    {
        if (sourceId.fileNameId())
            return {sourceId.directoryPathId(), DirectoryPathId{}};

        auto directoryPath = m_pathCache.directoryPath(sourceId.directoryPathId());

        return {sourceId.directoryPathId(),
                m_pathCache.directoryPathId(parentDirectoryPath(directoryPath))};
    }

    std::pair<WatcherEntries, ProjectChunkIds> convertIdPathsToWatcherEntriesAndIds(
        const std::vector<IdPaths> &idPaths)
    {
        NanotraceHR::Tracer tracer{
            "project storage path watcher convert id paths to watcher entries",
            ProjectStorageTracing::category()};

        WatcherEntries entries;
        entries.reserve(sizeOfIdPaths(idPaths) * 2);
        ProjectChunkIds ids;
        ids.reserve(ids.size());

        for (const IdPaths &idPath : idPaths)
        {
            NanotraceHR::Tracer tracer{
                "project storage path watcher convert id paths to watcher entries loop id path",
                ProjectStorageTracing::category()};

            ProjectChunkId id = idPath.id;

            ids.push_back(id);

            for (SourceId sourceId : idPath.sourceIds) {
                auto fileStatus = m_fileStatusCache.find(sourceId);
                auto [directoryPathId, parentDirectoryPathId] = createDirectoryPathId(sourceId);

                tracer.tick("create watcher entry ",
                            NanotraceHR::keyValue("id", id),
                            NanotraceHR::keyValue("source id", sourceId),
                            NanotraceHR::keyValue("directory path id", directoryPathId),
                            NanotraceHR::keyValue("last modified", fileStatus.lastModified),
                            NanotraceHR::keyValue("size", fileStatus.size));

                entries.emplace_back(id,
                                     directoryPathId,
                                     sourceId,
                                     fileStatus.lastModified,
                                     fileStatus.size);
                if (parentDirectoryPathId) {
                    tracer.tick("create parent directory watcher entry ",
                                NanotraceHR::keyValue("id", id),
                                NanotraceHR::keyValue("source id", sourceId),
                                NanotraceHR::keyValue("directory path id", parentDirectoryPathId),
                                NanotraceHR::keyValue("last modified", fileStatus.lastModified),
                                NanotraceHR::keyValue("size", fileStatus.size));

                    entries.emplace_back(id,
                                         parentDirectoryPathId,
                                         sourceId,
                                         fileStatus.lastModified,
                                         fileStatus.size);
                }
            }
        }

        std::ranges::sort(entries);
        std::ranges::sort(ids);

        return {entries, ids};
    }

    void addEntries(const WatcherEntries &entries)
    {
        NanotraceHR::Tracer tracer{"project storage path watcher add entries",
                                   ProjectStorageTracing::category()};

        auto newEntries = notWatchedEntries(entries);

        auto filteredPaths = filterNotWatchedPaths(newEntries);

        mergeToWatchedEntries(newEntries);

        if (!filteredPaths.empty()) {
            auto paths = convertWatcherEntriesToDirectoryPathList(filteredPaths);
            tracer.tick("add paths", NanotraceHR::keyValue("paths", paths));
            m_fileSystemWatcher.addPaths(paths);
        }
    }

    template<typename Filter>
    void removeUnusedEntries(const WatcherEntries &entries, Filter filter)
    {
        NanotraceHR::Tracer tracer{"project storage path watcher remove unused entries",
                                   ProjectStorageTracing::category()};

        auto oldEntries = notAnymoreWatchedEntriesWithIds(entries, filter);

        removeFromWatchedEntries(oldEntries);

        auto filteredPaths = filterNotWatchedPaths(oldEntries);

        if (!filteredPaths.empty()) {
            auto paths = convertWatcherEntriesToDirectoryPathList(filteredPaths);
            tracer.tick("remove paths", NanotraceHR::keyValue("paths", paths));
            m_fileSystemWatcher.removePaths(paths);
        }
    }

    FileSystemWatcher &fileSystemWatcher()
    {
        NanotraceHR::Tracer tracer{"project storage path watcher file system watcher",
                                   ProjectStorageTracing::category()};
        return m_fileSystemWatcher;
    }

    QStringList convertWatcherEntriesToDirectoryPathList(const DirectoryPathIds &directoryPathIds) const
    {
        NanotraceHR::Tracer tracer{
            "project storage path watcher convert watcher entries to directory path list",
            ProjectStorageTracing::category()};

        return Utils::transform<QStringList>(directoryPathIds, [&](DirectoryPathId id) {
            return QString(m_pathCache.directoryPath(id));
        });
    }

    QStringList convertWatcherEntriesToDirectoryPathList(const WatcherEntries &watcherEntries) const
    {
        NanotraceHR::Tracer tracer{
            "project storage path watcher convert watcher entries to directory path list",
            ProjectStorageTracing::category()};

        DirectoryPathIds directoryPathIds = Utils::transform<DirectoryPathIds>(
            watcherEntries, &WatcherEntry::directoryPathId);

        std::ranges::sort(directoryPathIds);
        auto removed = std::ranges::unique(directoryPathIds);
        directoryPathIds.erase(removed.begin(), removed.end());

        return convertWatcherEntriesToDirectoryPathList(directoryPathIds);
    }

    WatcherEntries notWatchedEntries(const WatcherEntries &entries) const
    {
        NanotraceHR::Tracer tracer{"project storage path watcher not watched entries",
                                   ProjectStorageTracing::category()};

        WatcherEntries notWatchedEntries;
        notWatchedEntries.reserve(entries.size());

        std::ranges::set_difference(entries, m_watchedEntries, std::back_inserter(notWatchedEntries));

        return notWatchedEntries;
    }

    DirectoryPathIds notWatchedPaths(const DirectoryPathIds &ids) const
    {
        NanotraceHR::Tracer tracer{"project storage path watcher not watched paths",
                                   ProjectStorageTracing::category()};

        DirectoryPathIds notWatchedDirectoryIds;
        notWatchedDirectoryIds.reserve(ids.size());

        std::ranges::set_difference(ids, m_watchedEntries, std::back_inserter(notWatchedDirectoryIds));

        return notWatchedDirectoryIds;
    }

    static WatcherEntries notAnymoreWatchedEntries(std::ranges::input_range auto &oldEntries,
                                                   const WatcherEntries &newEntries)
    {
        NanotraceHR::Tracer tracer{"project storage path watcher not anymore watched entries",
                                   ProjectStorageTracing::category()};

        WatcherEntries notAnymoreWatchedEntries;
        notAnymoreWatchedEntries.reserve(1024);

        std::ranges::set_difference(oldEntries,
                                    newEntries,
                                    std::back_inserter(notAnymoreWatchedEntries));

        return notAnymoreWatchedEntries;
    }

    WatcherEntries notAnymoreWatchedEntriesWithIds(const WatcherEntries &newEntries,
                                                   std::predicate<const WatcherEntry &> auto filter) const
    {
        NanotraceHR::Tracer tracer{
            "project storage path watcher not anymore watched entries with ids",
            ProjectStorageTracing::category()};

        auto filteredEntries = m_watchedEntries | std::views::filter(filter);

        auto oldEntries = notAnymoreWatchedEntries(filteredEntries, newEntries);

        return oldEntries;
    }

    void mergeToWatchedEntries(const WatcherEntries &newEntries)
    {
        NanotraceHR::Tracer tracer{"project storage path watcher merge to watched entries",
                                   ProjectStorageTracing::category()};

        WatcherEntries newWatchedEntries;
        newWatchedEntries.reserve(m_watchedEntries.size() + newEntries.size());

        std::ranges::merge(m_watchedEntries, newEntries, std::back_inserter(newWatchedEntries));

        m_watchedEntries = std::move(newWatchedEntries);
    }

    static DirectoryPathIds uniquePaths(const WatcherEntries &pathEntries)
    {
        DirectoryPathIds uniqueDirectoryIds;
        uniqueDirectoryIds.reserve(pathEntries.size());

        std::ranges::unique_copy(pathEntries,
                                 std::back_inserter(uniqueDirectoryIds),
                                 {},
                                 &WatcherEntry::directoryPathId);

        return uniqueDirectoryIds;
    }

    DirectoryPathIds filterNotWatchedPaths(const WatcherEntries &entries) const
    {
        NanotraceHR::Tracer tracer{"project storage path watcher filter not watched paths",
                                   ProjectStorageTracing::category()};

        return notWatchedPaths(uniquePaths(entries));
    }

    const WatcherEntries &watchedEntries() const
    {
        NanotraceHR::Tracer tracer{"project storage path watcher watched entries",
                                   ProjectStorageTracing::category()};

        return m_watchedEntries;
    }

    WatcherEntries removeIdsFromWatchedEntries(const ProjectPartIds &ids)
    {
        NanotraceHR::Tracer tracer{"project storage path watcher remove ids from watched entries",
                                   ProjectStorageTracing::category()};

        auto keep = [&](WatcherEntry entry) { return !std::ranges::binary_search(ids, entry.id.id); };

        auto removed = std::ranges::stable_partition(m_watchedEntries, keep);

        WatcherEntries removedEntries(removed.begin(), removed.end());

        m_watchedEntries.erase(removed.begin(), removed.end());

        return removedEntries;
    }

    void removeFromWatchedEntries(const WatcherEntries &oldEntries)
    {
        NanotraceHR::Tracer tracer{"project storage path watcher remove from watched entries",
                                   ProjectStorageTracing::category()};

        WatcherEntries newWatchedEntries;
        newWatchedEntries.reserve(m_watchedEntries.size() - oldEntries.size());

        std::ranges::set_difference(m_watchedEntries,
                                    oldEntries,
                                    std::back_inserter(newWatchedEntries));

        m_watchedEntries = std::move(newWatchedEntries);

        if (oldEntries.size()) {
            SourceIds removedSourceIds = Utils::transform(oldEntries, &WatcherEntry::sourceId);
            std::ranges::sort(removedSourceIds);
            m_fileStatusCache.remove(removedSourceIds);
        }
    }

    void compressChangedDirectoryPath(const QString &path)
    {
        NanotraceHR::Tracer tracer{"project storage path watcher compress changed directory path",
                                   ProjectStorageTracing::category(),
                                   NanotraceHR::keyValue("path", path)};

        m_directoryPathCompressor.addDirectoryPathId(
            m_pathCache.directoryPathId(Utils::PathString{path}));
    }

    WatcherEntries watchedEntriesForPaths(const QmlDesigner::DirectoryPathIds &directoryPathIds)
    {
        NanotraceHR::Tracer tracer{"project storage path watcher watched entries for paths",
                                   ProjectStorageTracing::category()};

        WatcherEntries foundEntries;
        foundEntries.reserve(m_watchedEntries.size());

        Utils::set_greedy_intersection(
            m_watchedEntries,
            directoryPathIds,
            [&](WatcherEntry &entry) {
                auto fileStatus = m_fileStatusCache.updateAndFind(entry.sourceId);
                if (entry.lastModified < fileStatus.lastModified || entry.size != fileStatus.size) {
                    foundEntries.push_back(entry);
                    entry.lastModified = fileStatus.lastModified;
                    entry.size = fileStatus.size;
                }
            },
            {},
            &WatcherEntry::directoryPathId);

        return foundEntries;
    }

    SourceIds watchedPaths(const WatcherEntries &entries) const
    {
        NanotraceHR::Tracer tracer{"project storage path watcher watched paths",
                                   ProjectStorageTracing::category()};

        auto sourceIds = Utils::transform<SourceIds>(entries, &WatcherEntry::sourceId);

        std::ranges::sort(sourceIds);
        auto removed = std::ranges::unique(sourceIds);
        sourceIds.erase(removed.begin(), removed.end());

        return sourceIds;
    }

    std::vector<IdPaths> idPathsForWatcherEntries(WatcherEntries &&foundEntries)
    {
        NanotraceHR::Tracer tracer{"project storage path watcher id paths for watcher entries",
                                   ProjectStorageTracing::category()};

        std::ranges::sort(foundEntries, {}, [](const WatcherEntry &entry) {
            return std::tie(entry.id, entry.sourceId);
        });

        std::vector<IdPaths> idPaths;
        idPaths.reserve(foundEntries.size());

        if (foundEntries.size()) {
            auto *lastEntry = &idPaths.emplace_back(foundEntries.front().id, SourceIds{});

            for (WatcherEntry entry : foundEntries) {
                if (lastEntry->id != entry.id)
                    lastEntry = &idPaths.emplace_back(entry.id, SourceIds{});

                lastEntry->sourceIds.push_back(entry.sourceId);
            }
        }

        return idPaths;
    }

    void addChangedPathForFilePath(const DirectoryPathIds &directoryPathIds)
    {
        NanotraceHR::Tracer tracer{"project storage path watcher add changed path for file path",
                                   ProjectStorageTracing::category()};

        if (m_notifier) {
            WatcherEntries foundEntries = watchedEntriesForPaths(directoryPathIds);

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
