// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"
#include "storagecacheentry.h"

#include <utils/smallstring.h>

#include <vector>

namespace QmlDesigner::Cache {


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

class SourceName : public StorageCacheEntry<Utils::PathString, Utils::SmallStringView, SourceNameId>
{
    using Base = StorageCacheEntry<Utils::PathString, Utils::SmallStringView, SourceNameId>;

public:
    using Base::Base;

    friend bool operator==(const SourceName &first, const SourceName &second)
    {
        return first.id == second.id && first.value == second.value;
    }
};

using SourceNames = std::vector<SourceName>;

} // namespace QmlDesigner::Cache
