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
#include "stringcache.h"

#include <QObject>

namespace ClangBackEnd {

class WatcherEntry
{
public:
    uint id;
    uint path;

    friend bool operator==(const WatcherEntry &first, const WatcherEntry &second)
    {
        return first.id == second.id && first.path == second.path;
    }

    friend bool operator<(const WatcherEntry &first, const WatcherEntry &second)
    {
        return std::tie(first.path, first.id) < std::tie(second.path, second.id);
    }

    friend bool operator<(const WatcherEntry &entry, uint path)
    {
        return entry.path < path;
    }

    friend bool operator<(uint path, const WatcherEntry &entry)
    {
        return path < entry.path;
    }
};

template <typename FileSystemWatcher>
class ClangPathWatcher : public ClangPathWatcherInterface
{
public:
    ClangPathWatcher(StringCache<Utils::SmallString> &pathCache,
                     ClangPathWatcherNotifier *notifier=nullptr)
        : m_pathCache(pathCache),
          m_notifier(notifier)
    {
        QObject::connect(&m_fileSystemWatcher,
                         &FileSystemWatcher::fileChanged,
                         [&] (const QString &filePath) { addChangedPathForFilePath(filePath); });
    }

    void updateIdPaths(const std::vector<IdPaths> &idPaths) override
    {
        auto entriesAndIds = convertIdPathsToWatcherEntriesAndIds(idPaths);

        addEntries(entriesAndIds.first);
        removeUnusedEntries(entriesAndIds.first, entriesAndIds.second);
    }

    void removeIds(const Utils::SmallStringVector &ids) override
    {
        auto removedEntries = removeIdsFromWatchedEntries(convertToIdNumbers(ids));

        auto filteredPaths = filterNotWatchedPaths(removedEntries);

        if (!filteredPaths.empty())
            m_fileSystemWatcher.removePaths(convertWatcherEntriesToQStringList(filteredPaths));
    }

    void setNotifier(ClangPathWatcherNotifier *notifier) override
    {
        m_notifier = notifier;
    }

unitttest_public:
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

    std::vector<uint> convertToIdNumbers(const Utils::SmallStringVector &ids)
    {
        std::vector<uint> idNumbers = m_idCache.stringIds(ids);

        std::sort(idNumbers.begin(), idNumbers.end());

        return idNumbers;
    }

    std::size_t sizeOfIdPaths(const std::vector<IdPaths> &idPaths)
    {
        auto sumSize = [] (std::size_t size, const IdPaths &idPath) {
            return size + idPath.paths.size();
        };

        return std::accumulate(idPaths.begin(), idPaths.end(), 0u, sumSize);
    }


    std::pair<std::vector<WatcherEntry>,std::vector<uint>>
    convertIdPathsToWatcherEntriesAndIds(const std::vector<IdPaths> &idPaths)
    {
        std::vector<WatcherEntry> entries;
        entries.reserve(sizeOfIdPaths(idPaths));
        std::vector<uint> ids;
        ids.reserve(ids.size());

        auto outputIterator = std::back_inserter(entries);

        for (const IdPaths &idPath : idPaths)
        {
            uint id = m_idCache.stringId(idPath.id);

            ids.push_back(id);

            outputIterator = std::transform(idPath.paths.begin(),
                                            idPath.paths.end(),
                                            outputIterator,
                                            [&] (uint path) { return WatcherEntry{id, path}; });
        }

        std::sort(entries.begin(), entries.end());
        std::sort(ids.begin(), ids.end());

        return {entries, ids};
    }

    void addEntries(const std::vector<WatcherEntry> &entries)
    {
        auto newEntries = notWatchedEntries(entries);

        auto filteredPaths = filterNotWatchedPaths(newEntries);

        mergeToWatchedEntries(newEntries);

        if (!filteredPaths.empty())
            m_fileSystemWatcher.addPaths(convertWatcherEntriesToQStringList(filteredPaths));
    }

    void removeUnusedEntries(const std::vector<WatcherEntry> &entries,
                             const std::vector<uint> &ids)
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
            const std::vector<WatcherEntry> &watcherEntries)
    {
        QStringList paths;
        paths.reserve(int(watcherEntries.size()));

        std::transform(watcherEntries.begin(),
                       watcherEntries.end(),
                       std::back_inserter(paths),
                       [&] (const WatcherEntry &entry) { return m_pathCache.string(entry.path); });

        return paths;
    }

    template <typename Compare>
    std::vector<WatcherEntry> notWatchedEntries(const std::vector<WatcherEntry> &entries,
                                                Compare compare) const
    {
        std::vector<WatcherEntry> notWatchedEntries;
        notWatchedEntries.reserve(entries.size());

        std::set_difference(entries.begin(),
                            entries.end(),
                            m_watchedEntries.cbegin(),
                            m_watchedEntries.cend(),
                            std::back_inserter(notWatchedEntries),
                            compare);

        return notWatchedEntries;
    }

    std::vector<WatcherEntry> notWatchedEntries(const std::vector<WatcherEntry> &entries) const
    {
        return notWatchedEntries(entries, std::less<WatcherEntry>());
    }

    std::vector<WatcherEntry> notWatchedPaths(const std::vector<WatcherEntry> &entries) const
    {
        auto compare = [] (const WatcherEntry &first, const WatcherEntry &second) {
            return first.path < second.path;
        };

        return notWatchedEntries(entries, compare);
    }

    template <typename Compare>
    std::vector<WatcherEntry> notAnymoreWatchedEntries(
            const std::vector<WatcherEntry> &newEntries,
            Compare compare) const
    {
        std::vector<WatcherEntry> notAnymoreWatchedEntries;
        notAnymoreWatchedEntries.reserve(m_watchedEntries.size());

        std::set_difference(m_watchedEntries.cbegin(),
                            m_watchedEntries.cend(),
                            newEntries.begin(),
                            newEntries.end(),
                            std::back_inserter(notAnymoreWatchedEntries),
                            compare);

        return notAnymoreWatchedEntries;
    }

    std::vector<WatcherEntry> notAnymoreWatchedEntriesWithIds(
            const std::vector<WatcherEntry> &newEntries,
            const std::vector<uint> &ids) const
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

    void mergeToWatchedEntries(const std::vector<WatcherEntry> &newEntries)
    {
        std::vector<WatcherEntry> newWatchedEntries;
        newWatchedEntries.reserve(m_watchedEntries.size() + newEntries.size());

        std::merge(m_watchedEntries.cbegin(),
                   m_watchedEntries.cend(),
                   newEntries.begin(),
                   newEntries.end(),
                   std::back_inserter(newWatchedEntries));

        m_watchedEntries = std::move(newWatchedEntries);
    }

    static
    std::vector<WatcherEntry> uniquePaths(const std::vector<WatcherEntry> &pathEntries)
    {
        std::vector<WatcherEntry> uniqueEntries;

        auto compare = [] (const WatcherEntry &first, const WatcherEntry &second) {
            return first.path == second.path;
        };

        std::unique_copy(pathEntries.begin(),
                         pathEntries.end(),
                         std::back_inserter(uniqueEntries),
                         compare);

        return uniqueEntries;
    }

    std::vector<WatcherEntry> filterNotWatchedPaths(const std::vector<WatcherEntry> &entries)
    {
        return notWatchedPaths(uniquePaths(entries));
    }

    const std::vector<WatcherEntry> &watchedEntries() const
    {
        return m_watchedEntries;
    }

    std::vector<WatcherEntry> removeIdsFromWatchedEntries(const std::vector<uint> &ids)
    {

        auto keep = [&] (const WatcherEntry &entry) {
            return !std::binary_search(ids.begin(), ids.end(), entry.id);
        };

        auto found = std::stable_partition(m_watchedEntries.begin(),
                                           m_watchedEntries.end(),
                                           keep);

        std::vector<WatcherEntry> removedEntries(found, m_watchedEntries.end());

        m_watchedEntries.erase(found, m_watchedEntries.end());

        return removedEntries;
    }

    void removeFromWatchedEntries(const std::vector<WatcherEntry> &oldEntries)
    {
        std::vector<WatcherEntry> newWatchedEntries;
        newWatchedEntries.reserve(m_watchedEntries.size() - oldEntries.size());

        std::set_difference(m_watchedEntries.cbegin(),
                            m_watchedEntries.cend(),
                            oldEntries.begin(),
                            oldEntries.end(),
                            std::back_inserter(newWatchedEntries));


        m_watchedEntries = newWatchedEntries;
    }

    void addChangedPathForFilePath(const QString &filePath)
    {
        uint pathId = m_pathCache.stringId(Utils::SmallString(filePath));

        auto range = std::equal_range(m_watchedEntries.begin(), m_watchedEntries.end(), pathId);

        Utils::SmallStringVector changedIds;

        std::transform(range.first,
                       range.second,
                       std::back_inserter(changedIds),
                       [&] (const WatcherEntry &entry) {
            return m_idCache.string(entry.id);
        });

        std::sort(changedIds.begin(), changedIds.end());

        if (m_notifier)
            m_notifier->pathsWithIdsChanged(changedIds);
    }

    StringCache<Utils::SmallString> &pathCache()
    {
        return m_pathCache;
    }

    StringCache<Utils::SmallString> &idCache()
    {
        return m_idCache;
    }

private:
    StringCache<Utils::SmallString> m_idCache;
    std::vector<WatcherEntry> m_watchedEntries;
    FileSystemWatcher m_fileSystemWatcher;
    StringCache<Utils::SmallString> &m_pathCache;
    ClangPathWatcherNotifier *m_notifier;
};

std::ostream &operator<<(std::ostream &out, const WatcherEntry &entry);

} // namespace ClangBackEnd
