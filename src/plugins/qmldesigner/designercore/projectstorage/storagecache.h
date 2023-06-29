// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "nonlockingmutex.h"
#include "projectstorageids.h"
#include "storagecacheentry.h"
#include "storagecachefwd.h"

#include <utils/algorithm.h>
#include <utils/set_algorithm.h>
#include <utils/smallstringfwd.h>

#include <algorithm>
#include <optional>
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

template<typename Type,
         typename ViewType,
         typename IndexType,
         typename Storage,
         typename Mutex,
         bool (*compare)(ViewType, ViewType),
         class CacheEntry = StorageCacheEntry<Type, ViewType, IndexType>>
class StorageCache
{
    using ResultType = std::conditional_t<std::is_base_of<NonLockingMutex, Mutex>::value, ViewType, Type>;
    using IndexDatabaseType = typename IndexType::DatabaseType;
    class StorageCacheIndex
    {
    public:
        constexpr explicit StorageCacheIndex() = default;

        StorageCacheIndex(const char *) = delete;

        template<typename IntegerType>
        constexpr explicit StorageCacheIndex(IntegerType id) noexcept
            : id{static_cast<std::size_t>(id)}
        {}

        constexpr StorageCacheIndex operator=(std::ptrdiff_t newId) noexcept
        {
            id = static_cast<std::size_t>(newId);

            return *this;
        }

        constexpr StorageCacheIndex operator+(int amount) const noexcept
        {
            return StorageCacheIndex{id + amount};
        }

        constexpr friend bool operator==(StorageCacheIndex first, StorageCacheIndex second) noexcept
        {
            return first.id == second.id && first.isValid() && second.isValid();
        }

        constexpr friend bool operator<(StorageCacheIndex first, StorageCacheIndex second) noexcept
        {
            return first.id < second.id;
        }

        constexpr friend bool operator>=(StorageCacheIndex first, StorageCacheIndex second) noexcept
        {
            return first.id >= second.id;
        }

        constexpr bool isValid() const noexcept
        {
            return id != std::numeric_limits<std::size_t>::max();
        }

        explicit operator std::size_t() const noexcept { return static_cast<std::size_t>(id); }

    public:
        std::size_t id = std::numeric_limits<std::size_t>::max();
    };

public:
    using MutexType = Mutex;
    using CacheEntries = std::vector<CacheEntry>;
    using const_iterator = typename CacheEntries::const_iterator;

    StorageCache(Storage storage, std::size_t reserveSize = 1024)
        : m_storage{std::move(storage)}
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

    StorageCache(StorageCache &&other) noexcept
        : m_entries(std::move(other.m_entries))
        , m_indices(std::move(other.m_indices))
    {}

    StorageCache &operator=(StorageCache &&other) noexcept
    {
        m_entries = std::move(other.m_entries);
        m_indices = std::move(other.m_indices);

        return *this;
    }

    void populate()
    {
        uncheckedPopulate();

        checkEntries();
    }

    void uncheckedPopulate()
    {
        m_entries = m_storage.fetchAll();

        std::sort(m_entries.begin(), m_entries.end(), [](ViewType first, ViewType second) {
            return compare(first, second);
        });

        std::size_t max_id = 0;

        auto found = std::max_element(m_entries.begin(),
                                      m_entries.end(),
                                      [](const auto &first, const auto &second) {
                                          return first.id < second.id;
                                      });

        if (found != m_entries.end())
            max_id = static_cast<std::size_t>(found->id);

        m_indices.resize(max_id);

        updateIndices();
    }

    void add(std::vector<ViewType> &&views)
    {
        auto less = [](ViewType first, ViewType second) { return compare(first, second); };

        std::sort(views.begin(), views.end(), less);

        views.erase(std::unique(views.begin(), views.end()), views.end());

        CacheEntries newCacheEntries;
        newCacheEntries.reserve(views.size());

        std::set_difference(views.begin(),
                            views.end(),
                            m_entries.begin(),
                            m_entries.end(),
                            Utils::make_iterator([&](ViewType newView) {
                                IndexType index = m_storage.fetchId(newView);
                                newCacheEntries.emplace_back(newView, index);
                            }),
                            less);

        if (newCacheEntries.size()) {
            auto found = std::max_element(newCacheEntries.begin(),
                                          newCacheEntries.end(),
                                          [](const auto &first, const auto &second) {
                                              return first.id < second.id;
                                          });

            auto max_id = static_cast<std::size_t>(found->id);

            if (max_id > m_indices.size())
                m_indices.resize(max_id);

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

        auto found = find(view);

        if (found != m_entries.end())
            return found->id;

        sharedLock.unlock();
        std::lock_guard<Mutex> exclusiveLock(m_mutex);

        if (!std::is_base_of<NonLockingMutex, Mutex>::value)
            found = find(view);
        if (found == m_entries.end())
            found = insertEntry(found, view, m_storage.fetchId(view));

        return found->id;
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

        if (IndexType::create(static_cast<IndexDatabaseType>(m_indices.size()) + 1) > id) {
            if (StorageCacheIndex indirectionIndex = m_indices.at(static_cast<std::size_t>(id) - 1);
                indirectionIndex.isValid()) {
                return m_entries.at(static_cast<std::size_t>(indirectionIndex)).value;
            }
        }

        sharedLock.unlock();
        std::lock_guard<Mutex> exclusiveLock(m_mutex);

        Type value{m_storage.fetchValue(id)};
        auto interator = insertEntry(find(value), value, id);

        return interator->value;
    }

    std::vector<ResultType> values(const std::vector<IndexType> &ids) const
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);

        std::vector<ResultType> values;
        values.reserve(ids.size());

        for (IndexType id : ids) {
            values.emplace_back(
                m_entries
                    .at(static_cast<std::size_t>(m_indices.at(static_cast<std::size_t>(id) - 1)))
                    .value);
        }
        return values;
    }

    bool isEmpty() const { return m_entries.empty() && m_indices.empty(); }

    Mutex &mutex() const { return m_mutex; }

private:
    void updateIndices()
    {
        auto begin = m_entries.cbegin();
        for (auto current = begin; current != m_entries.cend(); ++current) {
            if (current->id)
                m_indices[static_cast<std::size_t>(current->id) - 1] = std::distance(begin, current);
        }
    }

    template<typename Entries>
    static auto find(Entries &&entries, ViewType view)
    {
        auto begin = entries.begin();
        auto end = entries.end();
        auto found = std::lower_bound(begin, end, view, compare);

        if (found == entries.end()) {
            return entries.end();
        }

        auto value = *found;

        if (value == view) {
            return found;
        }

        return entries.end();
    }

    IndexType id(ViewType view) const
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);

        auto found = find(view);

        if (found != m_entries.end()) {
            return found->id;
        }

        return IndexType();
    }

    ResultType value(IndexType id) const
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);

        if (IndexType::create(static_cast<IndexDatabaseType>(m_indices.size()) + 1) > id) {
            if (StorageCacheIndex indirectionIndex = m_indices.at(static_cast<std::size_t>(id) - 1);
                indirectionIndex.isValid()) {
                return m_entries.at(static_cast<std::size_t>(indirectionIndex)).value;
            }
        }

        return ResultType();
    }

    auto find(ViewType view) const { return find(m_entries, view); }

    auto find(ViewType view) { return find(m_entries, view); }

    void incrementLargerOrEqualIndicesByOne(StorageCacheIndex newIndirectionIndex)
    {
        std::transform(m_indices.begin(), m_indices.end(), m_indices.begin(), [&](StorageCacheIndex index) {
            return index >= newIndirectionIndex ? index + 1 : index;
        });
    }

    void ensureSize(std::size_t size)
    {
        if (m_indices.size() <= size)
            m_indices.resize(size + 1);
    }

    auto insertEntry(const_iterator beforeIterator, ViewType view, IndexType id)
    {
        auto inserted = m_entries.emplace(beforeIterator, view, id);

        StorageCacheIndex newIndirectionIndex{std::distance(m_entries.begin(), inserted)};

        incrementLargerOrEqualIndicesByOne(newIndirectionIndex);

        auto indirectionIndex = static_cast<std::size_t>(id) - 1;
        ensureSize(indirectionIndex);
        m_indices.at(indirectionIndex) = newIndirectionIndex;

        return inserted;
    }

    void checkEntries() const
    {
        for (const auto &entry : m_entries) {
            if (entry.value != value(entry.id) || entry.id != id(entry.value))
                throw StorageCacheException();
        }
    }

private:
    CacheEntries m_entries;
    std::vector<StorageCacheIndex> m_indices;
    mutable Mutex m_mutex;
    Storage m_storage;
};

} // namespace QmlDesigner
