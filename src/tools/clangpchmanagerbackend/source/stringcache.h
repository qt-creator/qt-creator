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

#include "clangpchmanagerbackend_global.h"

#include <utils/smallstringview.h>

#include <algorithm>
#include <vector>

namespace ClangBackEnd {

template <typename StringType>
class StringCache
{
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

        StringType string;
        uint id;
    };

    using StringCacheEntries = std::vector<StringCacheEntry>;
    using const_iterator = typename StringCacheEntries::const_iterator;

    class Found
    {
    public:
        typename StringCacheEntries::const_iterator iterator;
        bool wasFound;
    };


public:
    StringCache()
    {
        m_strings.reserve(1024);
        m_indices.reserve(1024);
    }

    uint stringId(Utils::SmallStringView stringView)
    {
        Found found = find(stringView);

        if (!found.wasFound)
            return insertString(found.iterator, stringView);

        return found.iterator->id;
    }

    std::vector<uint> stringIds(const std::vector<StringType> &strings)
    {
        std::vector<uint> ids;
        ids.reserve(strings.size());

        std::transform(strings.begin(),
                       strings.end(),
                       std::back_inserter(ids),
                       [&] (const StringType &string) { return stringId(string); });

        return ids;
    }

    const StringType &string(uint id) const
    {
        return m_strings.at(m_indices.at(id)).string;
    }

    std::vector<StringType> strings(const std::vector<uint> &ids) const
    {
        std::vector<StringType> strings;
        strings.reserve(ids.size());

        std::transform(ids.begin(),
                       ids.end(),
                       std::back_inserter(strings),
                       [&] (uint id) { return m_strings.at(m_indices.at(id)).string; });

        return strings;
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
        auto id = uint(m_strings.size());

        auto inserted = m_strings.emplace(beforeIterator, StringType(stringView), id);

        auto newIndex = uint(std::distance(m_strings.begin(), inserted));

        incrementLargerOrEqualIndicesByOne(newIndex);

        m_indices.push_back(newIndex);

        return id;
    }

private:
    StringCacheEntries m_strings;
    std::vector<uint> m_indices;
};

} // namespace ClangBackEnd
