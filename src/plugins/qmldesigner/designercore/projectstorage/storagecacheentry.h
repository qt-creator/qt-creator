// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QmlDesigner {

template<typename Type, typename ViewType, typename IndexType>
class StorageCacheEntry
{
public:
    StorageCacheEntry(ViewType value, IndexType id)
        : value(value)
        , id(id)
    {}

    operator ViewType() const noexcept { return value; }
    friend bool operator==(const StorageCacheEntry &first, const StorageCacheEntry &second)
    {
        return first.id == second.id && first.value == second.value;
    }

public:
    Type value;
    IndexType id;
};

} // namespace QmlDesigner
