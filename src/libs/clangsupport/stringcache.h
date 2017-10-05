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

#include "stringcachealgorithms.h"
#include "stringcachefwd.h"

#include <utils/optional.h>
#include <utils/smallstringview.h>
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

template <typename StringType, typename IndexType>
class StringCacheEntry
{
public:
    StringCacheEntry(StringType &&string, IndexType id)
        : string(std::move(string)),
          id(id)
    {}

    operator Utils::SmallStringView() const
    {
        return {string.data(), string.size()};
    }

    StringType string;
    IndexType id;
};

template <typename StringType, typename IndexType>
using StringCacheEntries = std::vector<StringCacheEntry<StringType, IndexType>>;

template <typename StringType,
          typename IndexType,
          typename Mutex,
          typename Compare,
          Compare compare = Utils::compare>
class StringCache
{
public:
    using CacheEntry = StringCacheEntry<StringType, IndexType>;
    using CacheEntries = StringCacheEntries<StringType, IndexType>;
    using const_iterator = typename CacheEntries::const_iterator;
    using Found = ClangBackEnd::Found<const_iterator>;

    StringCache(std::size_t reserveSize = 1024)
    {
        m_strings.reserve(reserveSize);
        m_indices.reserve(reserveSize);
    }

    void populate(CacheEntries &&entries)
    {
        uncheckedPopulate(std::move(entries));

        checkEntries();
    }

    void uncheckedPopulate(CacheEntries &&entries)
    {
        std::sort(entries.begin(),
                  entries.end(),
                  [] (Utils::SmallStringView first, Utils::SmallStringView second) {
            return compare(first, second) < 0;
        });

        m_strings = std::move(entries);
        m_indices.resize(m_strings.size());

        auto begin = m_strings.cbegin();
        for (auto current = begin; current != m_strings.end(); ++current)
            m_indices.at(current->id) = std::distance(begin, current);
    }


    IndexType stringId(Utils::SmallStringView stringView)
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);
        Found found = find(stringView);

        if (found.wasFound)
            return found.iterator->id;

        sharedLock.unlock();
        std::lock_guard<Mutex> exclusiveLock(m_mutex);

        found = find(stringView);
        if (!found.wasFound) {
            IndexType index = insertString(found.iterator, stringView, IndexType(m_indices.size()));
            found.iterator = m_strings.begin() + index;;
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

    std::vector<IndexType> stringIds(std::initializer_list<StringType> strings)
    {
        return stringIds<std::initializer_list<StringType>>(strings);
    }

    StringType string(IndexType id) const
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);

        return m_strings.at(m_indices.at(id)).string;
    }

    template<typename Function>
    StringType string(IndexType id, Function storageFunction)
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

    std::vector<StringType> strings(const std::vector<IndexType> &ids) const
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);

        std::vector<StringType> strings;
        strings.reserve(ids.size());

        std::transform(ids.begin(),
                       ids.end(),
                       std::back_inserter(strings),
                       [&] (IndexType id) { return m_strings.at(m_indices.at(id)).string; });

        return strings;
    }

    bool isEmpty() const
    {
        return m_strings.empty() && m_indices.empty();
    }

    template<typename Function>
    IndexType stringId(Utils::SmallStringView stringView, Function storageFunction)
    {
        std::shared_lock<Mutex> sharedLock(m_mutex);

        Found found = find(stringView);

        if (found.wasFound)
            return found.iterator->id;

        sharedLock.unlock();
        std::lock_guard<Mutex> exclusiveLock(m_mutex);

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
    Found find(Utils::SmallStringView stringView)
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
                           Utils::SmallStringView stringView,
                           IndexType id)
    {
        auto inserted = m_strings.emplace(beforeIterator, StringType(stringView), id);

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
