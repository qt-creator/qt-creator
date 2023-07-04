// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"
#include "storagecacheentry.h"

#include <utils/smallstring.h>

#include <vector>

namespace QmlDesigner::Cache {

class SourceNameView
{
public:
    friend bool operator==(const SourceNameView &first, const SourceNameView &second) noexcept
    {
        return first.sourceContextId == second.sourceContextId
               && first.sourceName == second.sourceName;
    }

    friend bool operator<(SourceNameView first, SourceNameView second) noexcept
    {
        return std::tie(first.sourceContextId, first.sourceName)
               < std::tie(second.sourceContextId, second.sourceName);
    }

public:
    Utils::SmallStringView sourceName;
    SourceContextId sourceContextId;
};

class SourceNameEntry
{
public:
    SourceNameEntry() = default;
    SourceNameEntry(Utils::SmallStringView sourceName, SourceContextId sourceContextId)
        : sourceName(sourceName)
        , sourceContextId(sourceContextId)
    {}

    SourceNameEntry(SourceNameView view)
        : sourceName(view.sourceName)
        , sourceContextId(view.sourceContextId)
    {}

    friend bool operator==(const SourceNameEntry &first, const SourceNameEntry &second) noexcept
    {
        return first.sourceContextId == second.sourceContextId
               && first.sourceName == second.sourceName;
    }

    friend bool operator!=(const SourceNameEntry &first, const SourceNameEntry &second) noexcept
    {
        return !(first == second);
    }

    friend bool operator==(const SourceNameEntry &first, const SourceNameView &second) noexcept
    {
        return first.sourceContextId == second.sourceContextId
               && first.sourceName == second.sourceName;
    }

    friend bool operator!=(const SourceNameEntry &first, const SourceNameView &second) noexcept
    {
        return !(first == second);
    }

    operator SourceNameView() const noexcept { return {sourceName, sourceContextId}; }

    operator Utils::SmallString() &&noexcept { return std::move(sourceName); }

public:
    Utils::SmallString sourceName;
    SourceContextId sourceContextId;
};

class SourceContext
    : public StorageCacheEntry<Utils::PathString, Utils::SmallStringView, SourceContextId>
{
    using Base = StorageCacheEntry<Utils::PathString, Utils::SmallStringView, SourceContextId>;

public:
    using Base::Base;

    friend bool operator==(const SourceContext &first, const SourceContext &second)
    {
        return first.id == second.id && first.value == second.value;
    }
};

using SourceContexts = std::vector<SourceContext>;

class Source : public StorageCacheEntry<SourceNameEntry, SourceNameView, SourceId>
{
    using Base = StorageCacheEntry<SourceNameEntry, SourceNameView, SourceId>;

public:
    using Base::Base;
    Source(Utils::SmallStringView sourceName, SourceContextId sourceContextId, SourceId sourceId)
        : Base{{sourceName, sourceContextId}, sourceId}
    {}

    friend bool operator==(const Source &first, const Source &second)
    {
        return first.id == second.id && first.value == second.value;
    }
};

using Sources = std::vector<Source>;

class SourceNameAndSourceContextId
{
public:
    constexpr SourceNameAndSourceContextId() = default;

    SourceNameAndSourceContextId(Utils::SmallStringView sourceName, SourceContextId sourceContextId)
        : sourceName{sourceName}
        , sourceContextId{sourceContextId}
    {}

    Utils::SmallString sourceName;
    SourceContextId sourceContextId;
};

} // namespace QmlDesigner::Cache
