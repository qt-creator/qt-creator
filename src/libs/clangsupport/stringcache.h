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

#include <utils/smallstringview.h>
#include <utils/smallstringfwd.h>

#include <algorithm>
#include <mutex>
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
    uint id;
};

template <typename StringType, typename IndexType>
using StringCacheEntries = std::vector<StringCacheEntry<StringType, IndexType>>;

using FileCacheCacheEntry = StringCacheEntry<Utils::PathString, FilePathIndex>;
using FileCacheCacheEntries = std::vector<FileCacheCacheEntry>;

template <typename StringType,
          typename IndexType,
          typename Mutex,
          typename Compare,
          Compare compare = Utils::compare>
class StringCache
{
    using CacheEntry = StringCacheEntry<StringType, IndexType>;
    using CacheEnties = StringCacheEntries<StringType, IndexType>;
    using const_iterator = typename CacheEnties::const_iterator;
    using Found = ClangBackEnd::Found<const_iterator>;
public:
    StringCache()
    {
        m_strings.reserve(1024);
        m_indices.reserve(1024);
    }

    void populate(CacheEnties &&entries)
    {
        uncheckedPopulate(std::move(entries));

        checkEntries();
    }

    void uncheckedPopulate(CacheEnties &&entries)
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
        std::lock_guard<Mutex> lock(m_mutex);

        return ungardedStringId(stringView);
    }

    template <typename Container>
    std::vector<IndexType> stringIds(const Container &strings)
    {
        std::lock_guard<Mutex> lock(m_mutex);

        std::vector<IndexType> ids;
        ids.reserve(strings.size());

        std::transform(strings.begin(),
                       strings.end(),
                       std::back_inserter(ids),
                       [&] (const auto &string) { return this->ungardedStringId(string); });

        return ids;
    }

    std::vector<IndexType> stringIds(std::initializer_list<StringType> strings)
    {
        return stringIds<std::initializer_list<StringType>>(strings);
    }

    Utils::SmallStringView string(IndexType id) const
    {
        std::lock_guard<Mutex> lock(m_mutex);

        return m_strings.at(m_indices.at(id)).string;
    }

    std::vector<StringType> strings(const std::vector<IndexType> &ids) const
    {
        std::lock_guard<Mutex> lock(m_mutex);

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

private:
    IndexType ungardedStringId(Utils::SmallStringView stringView)
    {
        Found found = find(stringView);

        if (!found.wasFound)
            return insertString(found.iterator, stringView);

        return found.iterator->id;
    }

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

    IndexType insertString(const_iterator beforeIterator,
                      Utils::SmallStringView stringView)
    {
        auto id = IndexType(m_indices.size());

        auto inserted = m_strings.emplace(beforeIterator, StringType(stringView), id);

        auto newIndex = IndexType(std::distance(m_strings.begin(), inserted));

        incrementLargerOrEqualIndicesByOne(newIndex);

        m_indices.push_back(newIndex);

        return id;
    }

    void checkEntries()
    {
        for (const auto &entry : m_strings) {
            if (entry.string != string(entry.id) || entry.id != stringId(entry.string))
                throw StringCacheException();
        }
    }

private:
    CacheEnties m_strings;
    std::vector<IndexType> m_indices;
    mutable Mutex m_mutex;
};

using FilePathIndices = std::vector<FilePathIndex>;

} // namespace ClangBackEnd
