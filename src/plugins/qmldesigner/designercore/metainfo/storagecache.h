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

#pragma once

#include "storagecachealgorithms.h"
#include "storagecacheentry.h"
#include "storagecachefwd.h"

#include <utils/algorithm.h>
#include <utils/optional.h>
#include <utils/set_algorithm.h>
#include <utils/smallstringfwd.h>

#include <algorithm>
#include <shared_mutex>
#include <vector>

namespace QmlDesigner {

class StorageCacheException : public std::exception
{
    const char *what() const noexcept override
    {
        return "StorageCache entries are in invalid state.";
    }
};

class NonLockingMutex
{
public:
    constexpr NonLockingMutex() noexcept {}
    NonLockingMutex(const NonLockingMutex&) = delete;
    NonLockingMutex& operator=(const NonLockingMutex&) = delete;
    void lock() {}
    void unlock() {}
    void lock_shared() {}
    void unlock_shared() {}
};

template<typename Type,
         typename ViewType,
         typename IndexType,
         typename Mutex,
         typename Compare,
         Compare compare = Utils::compare,
         typename CacheEntry = StorageCacheEntry<Type, ViewType, IndexType>,
         typename FetchValue = std::function<Type(IndexType)>,
         typename FetchId = std::function<IndexType(ViewType)>>
class StorageCache
{
    template<typename T, typename V, typename I, typename M, typename C, C c, typename CE, typename, typename>
    friend class StorageCache;

    using ResultType = std::conditional_t<std::is_base_of<NonLockingMutex, Mutex>::value, ViewType, Type>;

public:
    using MutexType = Mutex;
    using CacheEntries = std::vector<CacheEntry>;
    using const_iterator = typename CacheEntries::const_iterator;
    using Found = QmlDesigner::Found<const_iterator>;

    StorageCache(FetchValue fetchValue, FetchId fetchId, std::size_t reserveSize = 1024)
        : m_fetchValue{std::move(fetchValue)}
        , m_fetchId{std::move(fetchId)}
    {
        m_entries.reserve(reserveSize);
        m_indices.reserve(reserveSize);
    }

    StorageCache(const StorageCache &other)
        : m_entries(other.m_entries)
        , m_indices(other.m_indices)
    {}

    template<typename Cache>
    Cache clone()
    {
        Cache cache;
        cache.m_entries = m_entries;
        cache.m_indices = m_indices;

        return cache;
    }

    StorageCache(StorageCache &&other)
        : m_entries(std::move(other.m_entries))
        , m_indices(std::move(other.m_indices))
    {}

    StorageCache &operator=(StorageCache &&other)
    {
        m_entries = std::move(other.m_entries);
        m_indices = std::move(other.m_indices);

        return *this;
    }

    void populate(CacheEntries &&entries)
    {
        uncheckedPopulate(std::move(entries));

        checkEntries();
    }

    void uncheckedPopulate(CacheEntries &&entries)
    {
        std::sort(entries.begin(), entries.end(), [](ViewType first, ViewType second) {
            return compare(first, second) < 0;
        });

        m_entries = std::move(entries);

        int max_id = 0;

        auto found = std::max_element(m_entries.begin(),
                                      m_entries.end(),
                                      [](const auto &first, const auto &second) {
                                          return first.id < second.id;
                                      });

        if (found != m_entries.end())
            max_id = found->id + 1;

        m_indices.resize(max_id, -1);

        updateIndices();
    }

    void add(std::vector<ViewType> &&views)
    {
        auto less = [](ViewType first, ViewType second) { return compare(first, second) < 0; };

        std::sort(views.begin(), views.end(), less);

        views.erase(std::unique(views.begin(), views.end()), views.end());

        CacheEntries newCacheEntries;
        newCacheEntries.reserve(views.size());

        std::set_difference(views.begin(),
                            views.end(),
                            m_entries.begin(),
                            m_entries.end(),
                            Utils::make_iterator([&](ViewType newView) {
                                IndexType index = m_fetchId(newView);
                                newCacheEntries.emplace_back(newView, index);
                            }),
                            less);

        if (newCacheEntries.size()) {
            auto found = std::max_element(newCacheEntries.begin(),
                                          newCacheEntries.end(),
                                          [](const auto &first, const auto &second) {
                                              return first.id < second.id;
                                          });

            int max_id = found->id + 1;

            if (max_id > int(m_indices.size()))
                m_indices.resize(max_id, -1);

            CacheEntries mergedCacheEntries;
            mergedCacheEntries.reserve(newCacheEntries.size() + m_entries.size());

            std::merge(std::make_move_iterator(m_entries.begin()),
                       std::make_move_iterator(m_entries.end()),
                       std::make_move_iterator(newCacheEntries.begin()),
                       std::make_move_iterator(newCacheEntries.end()),
                       std::back_inserter(mergedCacheEntries),
                       less);

            m_entries = std::move(mergedCacheEntries);

            updateIndices();
        }
    }

    IndexType id(ViewType view)
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);

        Found found = find(view);

        if (found.wasFound)
            return found.iterator->id;

        sharedLock.unlock();
        std::lock_guard<Mutex> exclusiveLock(m_mutex);

        if (!std::is_base_of<NonLockingMutex, Mutex>::value)
            found = find(view);
        if (!found.wasFound) {
            IndexType index = insertEntry(found.iterator, view, m_fetchId(view));
            found.iterator = m_entries.begin() + index;
        }

        return found.iterator->id;
    }

    template<typename Container>
    std::vector<IndexType> ids(const Container &values)
    {
        std::vector<IndexType> ids;
        ids.reserve(values.size());

        std::transform(values.begin(), values.end(), std::back_inserter(ids), [&](const auto &values) {
            return this->id(values);
        });

        return ids;
    }

    std::vector<IndexType> ids(std::initializer_list<Type> values)
    {
        return ids<std::initializer_list<Type>>(values);
    }

    ResultType value(IndexType id)
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);

        if (IndexType(m_indices.size()) > id && m_indices.at(id) >= 0)
            return m_entries.at(m_indices.at(id)).value;

        sharedLock.unlock();
        std::lock_guard<Mutex> exclusiveLock(m_mutex);
        IndexType index;

        Type value{m_fetchValue(id)};
        index = insertEntry(find(value).iterator, value, id);

        return std::move(m_entries[index].value);
    }

    std::vector<ResultType> values(const std::vector<IndexType> &ids) const
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);

        std::vector<ResultType> values;
        values.reserve(ids.size());

        for (IndexType id : ids)
            values.emplace_back(m_entries.at(m_indices.at(id)).value);

        return values;
    }

    bool isEmpty() const { return m_entries.empty() && m_indices.empty(); }

    Mutex &mutex() const
    {
        return m_mutex;
    }

private:
    void updateIndices()
    {
        auto begin = m_entries.cbegin();
        for (auto current = begin; current != m_entries.cend(); ++current)
            m_indices[current->id] = std::distance(begin, current);
    }

    Found find(ViewType view)
    {
        return findInSorted(m_entries.cbegin(), m_entries.cend(), view, compare);
    }

    void incrementLargerOrEqualIndicesByOne(IndexType newIndex)
    {
        std::transform(m_indices.begin(),
                       m_indices.end(),
                       m_indices.begin(),
                       [&] (IndexType index) {
            return index >= newIndex ? ++index : index;
        });
    }

    void ensureSize(IndexType id)
    {
        if (m_indices.size() <= std::size_t(id))
            m_indices.resize(id + 1, -1);
    }

    IndexType insertEntry(const_iterator beforeIterator, ViewType view, IndexType id)
    {
        auto inserted = m_entries.emplace(beforeIterator, view, id);

        auto newIndex = IndexType(std::distance(m_entries.begin(), inserted));

        incrementLargerOrEqualIndicesByOne(newIndex);

        ensureSize(id);
        m_indices.at(id) = newIndex;

        return newIndex;
    }

    void checkEntries()
    {
        for (const auto &entry : m_entries) {
            if (entry.value != value(entry.id) || entry.id != id(entry.value))
                throw StorageCacheException();
        }
    }

private:
    CacheEntries m_entries;
    std::vector<IndexType> m_indices;
    mutable Mutex m_mutex;
    FetchValue m_fetchValue;
    FetchId m_fetchId;
};

} // namespace QmlDesigner
