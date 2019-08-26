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

#include "set_algorithm.h"
#include "stringcachealgorithms.h"
#include "stringcacheentry.h"
#include "stringcachefwd.h"

#include <utils/algorithm.h>
#include <utils/optional.h>
#include <utils/smallstringfwd.h>

#include <QReadWriteLock>

#include <algorithm>
#include <shared_mutex>
#include <vector>

namespace ClangBackEnd {

class StringCacheException : public std::exception
{
    const char *what() const noexcept override
    {
        return "StringCache entries are in invalid state.";
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

class SharedMutex
{
public:
    SharedMutex() = default;

    SharedMutex(const SharedMutex&) = delete;
    SharedMutex& operator=(const SharedMutex&) = delete;

    void lock()
    {
        m_mutex.lockForWrite();
    }

    void unlock()
    {
        m_mutex.unlock();
    }

    void lock_shared()
    {
        m_mutex.lockForRead();
    }

    void unlock_shared()
    {
        m_mutex.unlock();
    }
private:
    QReadWriteLock m_mutex;
};

template<typename StringType,
         typename StringViewType,
         typename IndexType,
         typename Mutex,
         typename Compare,
         Compare compare = Utils::compare,
         typename CacheEntry = StringCacheEntry<StringType, StringViewType, IndexType>>
class StringCache
{
    template<typename T, typename V, typename I, typename M, typename C, C c, typename CE>
    friend class StringCache;

    using StringResultType = std::
        conditional_t<std::is_base_of<NonLockingMutex, Mutex>::value, StringViewType, StringType>;

public:
    using MutexType = Mutex;
    using CacheEntries = std::vector<CacheEntry>;
    using const_iterator = typename CacheEntries::const_iterator;
    using Found = ClangBackEnd::Found<const_iterator>;

    StringCache(std::size_t reserveSize = 1024)
    {
        m_strings.reserve(reserveSize);
        m_indices.reserve(reserveSize);
    }

    StringCache(const StringCache &other)
        : m_strings(other.m_strings)
        , m_indices(other.m_indices)
    {}

    template<typename Cache>
    Cache clone()
    {
        Cache cache;
        cache.m_strings = m_strings;
        cache.m_indices = m_indices;

        return cache;
    }

    StringCache(StringCache &&other)
        : m_strings(std::move(other.m_strings))
        , m_indices(std::move(other.m_indices))
    {}

    StringCache &operator=(StringCache &&other)
    {
        m_strings = std::move(other.m_strings);
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
        std::sort(entries.begin(), entries.end(), [](StringViewType first, StringViewType second) {
            return compare(first, second) < 0;
        });

        m_strings = std::move(entries);

        int max_id = 0;

        auto found = std::max_element(m_strings.begin(),
                                      m_strings.end(),
                                      [](const auto &first, const auto &second) {
                                          return first.id < second.id;
                                      });

        if (found != m_strings.end())
            max_id = found->id + 1;

        m_indices.resize(max_id, -1);

        updateIndices();
    }

    template<typename Function>
    void addStrings(std::vector<StringViewType> &&strings, Function storageFunction)
    {
        auto less = [](StringViewType first, StringViewType second) {
            return compare(first, second) < 0;
        };

        std::sort(strings.begin(), strings.end(), less);

        strings.erase(std::unique(strings.begin(), strings.end()), strings.end());

        CacheEntries newCacheEntries;
        newCacheEntries.reserve(strings.size());

        std::set_difference(strings.begin(),
                            strings.end(),
                            m_strings.begin(),
                            m_strings.end(),
                            make_iterator([&](StringViewType newString) {
                                IndexType index = storageFunction(newString);
                                newCacheEntries.emplace_back(newString, index);
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
            mergedCacheEntries.reserve(newCacheEntries.size() + m_strings.size());

            std::merge(std::make_move_iterator(m_strings.begin()),
                       std::make_move_iterator(m_strings.end()),
                       std::make_move_iterator(newCacheEntries.begin()),
                       std::make_move_iterator(newCacheEntries.end()),
                       std::back_inserter(mergedCacheEntries),
                       less);

            m_strings = std::move(mergedCacheEntries);

            updateIndices();
        }
    }

    IndexType stringId(StringViewType stringView)
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);
        Found found = find(stringView);

        if (found.wasFound)
            return found.iterator->id;

        sharedLock.unlock();
        std::lock_guard<Mutex> exclusiveLock(m_mutex);

        if (!std::is_base_of<NonLockingMutex, Mutex>::value)
            found = find(stringView);
        if (!found.wasFound) {
            IndexType index = insertString(found.iterator, stringView, IndexType(m_indices.size()));
            found.iterator = m_strings.begin() + index;
        }

        return found.iterator->id;
    }

    template <typename Container>
    std::vector<IndexType> stringIds(const Container &strings)
    {
        std::vector<IndexType> ids;
        ids.reserve(strings.size());

        std::transform(strings.begin(),
                       strings.end(),
                       std::back_inserter(ids),
                       [&] (const auto &string) { return this->stringId(string); });

        return ids;
    }

    template<typename Container, typename Function>
    std::vector<IndexType> stringIds(const Container &strings, Function storageFunction)
    {
        std::vector<IndexType> ids;
        ids.reserve(strings.size());

        std::transform(strings.begin(),
                       strings.end(),
                       std::back_inserter(ids),
                       [&](const auto &string) { return this->stringId(string, storageFunction); });

        return ids;
    }

    std::vector<IndexType> stringIds(std::initializer_list<StringType> strings)
    {
        return stringIds<std::initializer_list<StringType>>(strings);
    }

    template<typename Function>
    std::vector<IndexType> stringIds(std::initializer_list<StringType> strings,
                                     Function storageFunction)
    {
        return stringIds<std::initializer_list<StringType>>(strings, storageFunction);
    }

    StringResultType string(IndexType id) const
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);

        return m_strings.at(m_indices.at(id)).string;
    }

    template<typename Function>
    StringResultType string(IndexType id, Function storageFunction)
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);

        if (IndexType(m_indices.size()) > id && m_indices.at(id) >= 0)
            return m_strings.at(m_indices.at(id)).string;


        sharedLock.unlock();
        std::lock_guard<Mutex> exclusiveLock(m_mutex);
        IndexType index;

        StringType string{storageFunction(id)};
        index = insertString(find(string).iterator, string, id);

        return m_strings[index].string;
    }

    std::vector<StringResultType> strings(const std::vector<IndexType> &ids) const
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);

        std::vector<StringResultType> strings;
        strings.reserve(ids.size());

        for (IndexType id : ids)
            strings.emplace_back(m_strings.at(m_indices.at(id)).string);

        return strings;
    }

    bool isEmpty() const
    {
        return m_strings.empty() && m_indices.empty();
    }

    template<typename Function>
    IndexType stringId(StringViewType stringView, Function storageFunction)
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);

        Found found = find(stringView);

        if (found.wasFound)
            return found.iterator->id;

        sharedLock.unlock();
        std::lock_guard<Mutex> exclusiveLock(m_mutex);

        if (!std::is_base_of<NonLockingMutex, Mutex>::value)
            found = find(stringView);
        if (!found.wasFound) {
            IndexType index = insertString(found.iterator, stringView, storageFunction(stringView));
            found.iterator = m_strings.begin() + index;
        }

        return found.iterator->id;
    }

    Mutex &mutex() const
    {
        return m_mutex;
    }

private:
    void updateIndices()
    {
        auto begin = m_strings.cbegin();
        for (auto current = begin; current != m_strings.cend(); ++current)
            m_indices[current->id] = std::distance(begin, current);
    }

    Found find(StringViewType stringView)
    {
        return findInSorted(m_strings.cbegin(), m_strings.cend(), stringView, compare);
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

    IndexType insertString(const_iterator beforeIterator,
                           StringViewType stringView,
                           IndexType id)
    {
        auto inserted = m_strings.emplace(beforeIterator, stringView, id);

        auto newIndex = IndexType(std::distance(m_strings.begin(), inserted));

        incrementLargerOrEqualIndicesByOne(newIndex);

        ensureSize(id);
        m_indices.at(id) = newIndex;

        return newIndex;
    }

    void checkEntries()
    {
        for (const auto &entry : m_strings) {
            if (entry.string != string(entry.id) || entry.id != stringId(entry.string))
                throw StringCacheException();
        }
    }

private:
    CacheEntries m_strings;
    std::vector<IndexType> m_indices;
    mutable Mutex m_mutex;
};

} // namespace ClangBackEnd
