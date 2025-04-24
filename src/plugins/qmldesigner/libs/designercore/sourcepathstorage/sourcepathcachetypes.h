// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"
#include "storagecacheentry.h"

#include <utils/smallstring.h>

#include <vector>

namespace QmlDesigner::Cache {


class DirectoryPath
    : public StorageCacheEntry<Utils::PathString, Utils::SmallStringView, DirectoryPathId>
{
    using Base = StorageCacheEntry<Utils::PathString, Utils::SmallStringView, DirectoryPathId>;

public:
    using Base::Base;

    friend bool operator==(const DirectoryPath &first, const DirectoryPath &second)
    {
        return first.id == second.id && first.value == second.value;
    }
};

using DirectoryPaths = std::vector<DirectoryPath>;

class FileName : public StorageCacheEntry<Utils::PathString, Utils::SmallStringView, FileNameId>
{
    using Base = StorageCacheEntry<Utils::PathString, Utils::SmallStringView, FileNameId>;

public:
    using Base::Base;

    friend bool operator==(const FileName &first, const FileName &second)
    {
        return first.id == second.id && first.value == second.value;
    }
};

using FileNames = std::vector<FileName>;

} // namespace QmlDesigner::Cache
