// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <map>

namespace QmlDesigner {

namespace MapUtils {
template<typename Key, typename... Arguments>
auto find(std::map<Key, Arguments...> &map, const Key &key)
{
    struct FoundIterator
    {
        typename std::map<Key, Arguments...>::iterator iterator;
        bool found = false;

        explicit operator bool() const { return found; }
    };

    auto iterator = map.lower_bound(key);
    bool found = iterator != map.end() && iterator->first == key;

    return FoundIterator{iterator, found};
}
} // namespace MapUtils

} // namespace QmlDesigner
