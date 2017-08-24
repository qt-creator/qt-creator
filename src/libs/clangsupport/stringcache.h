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

template <typename StringType>
class StringCacheEntry
{
public:
    StringCacheEntry(StringType &&string, uint id)
        : string(std::move(string)),
          id(id)
    {}

    friend bool operator<(const StringCacheEntry &entry, Utils::SmallStringView stringView)
    {
        return entry.string < stringView;
    }

    friend bool operator<(Utils::SmallStringView stringView, const StringCacheEntry &entry)
    {
        return stringView < entry.string;
    }

    friend bool operator<(const StringCacheEntry &first, const StringCacheEntry &second)
    {
        return first.string < second.string;
    }

    StringType string;
    uint id;
};

template <typename StringType>
using StringCacheEntries = std::vector<StringCacheEntry<StringType>>;

template <typename StringType,
          typename Mutex = NonLockingMutex>
class StringCache
{
    using const_iterator = typename StringCacheEntries<StringType>::const_iterator;

    class Found
    {
    public:
        typename StringCacheEntries<StringType>::const_iterator iterator;
        bool wasFound;
    };

public:
    StringCache()
    {
        m_strings.reserve(1024);
        m_indices.reserve(1024);
    }

    void populate(StringCacheEntries<StringType> &&entries)
    {
        uncheckedPopulate(std::move(entries));

        checkEntries();
    }

    void uncheckedPopulate(StringCacheEntries<StringType> &&entries)
    {
        std::sort(entries.begin(), entries.end());

        m_strings = std::move(entries);
        m_indices.resize(m_strings.size());

        auto begin = m_strings.cbegin();
        for (auto current = begin; current != m_strings.end(); ++current)
            m_indices.at(current->id) = std::distance(begin, current);
    }


    uint stringId(Utils::SmallStringView stringView)
    {
        std::lock_guard<Mutex> lock(m_mutex);

        Found found = find(stringView);

        if (!found.wasFound)
            return insertString(found.iterator, stringView);

        return found.iterator->id;
    }

    template <typename Container>
    std::vector<uint> stringIds(const Container &strings)
    {
        std::lock_guard<Mutex> lock(m_mutex);

        std::vector<uint> ids;
        ids.reserve(strings.size());

        std::transform(strings.begin(),
                       strings.end(),
                       std::back_inserter(ids),
                       [&] (const auto &string) { return this->stringId(string); });

        return ids;
    }

    std::vector<uint> stringIds(std::initializer_list<StringType> strings)
    {
        return stringIds<std::initializer_list<StringType>>(strings);
    }

    Utils::SmallStringView string(uint id) const
    {
        std::lock_guard<Mutex> lock(m_mutex);

        return m_strings.at(m_indices.at(id)).string;
    }

    std::vector<StringType> strings(const std::vector<uint> &ids) const
    {
        std::lock_guard<Mutex> lock(m_mutex);

        std::vector<StringType> strings;
        strings.reserve(ids.size());

        std::transform(ids.begin(),
                       ids.end(),
                       std::back_inserter(strings),
                       [&] (uint id) { return m_strings.at(m_indices.at(id)).string; });

        return strings;
    }

    bool isEmpty() const
    {
        return m_strings.empty() && m_indices.empty();
    }

private:
    Found find(Utils::SmallStringView stringView)
    {
        auto range = std::equal_range(m_strings.cbegin(), m_strings.cend(), stringView);

        return {range.first, range.first != range.second};
    }

    void incrementLargerOrEqualIndicesByOne(uint newIndex)
    {
        std::transform(m_indices.begin(),
                       m_indices.end(),
                       m_indices.begin(),
                       [&] (uint index) {
            return index >= newIndex ? ++index : index;
        });
    }

    uint insertString(const_iterator beforeIterator,
                      Utils::SmallStringView stringView)
    {
        auto id = uint(m_indices.size());

        auto inserted = m_strings.emplace(beforeIterator, StringType(stringView), id);

        auto newIndex = uint(std::distance(m_strings.begin(), inserted));

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
    StringCacheEntries<StringType> m_strings;
    std::vector<uint> m_indices;
    mutable Mutex m_mutex;
};

template <typename Mutex = NonLockingMutex>
using FilePathCache = StringCache<Utils::PathString, Mutex>;

} // namespace ClangBackEnd
