// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sourcepath.h"
#include "sourcepathcachetypes.h"
#include "sourcepathview.h"

namespace QmlDesigner {

class SourcePathCacheInterface
{
    SourcePathCacheInterface(const SourcePathCacheInterface &) = default;
    SourcePathCacheInterface &operator=(const SourcePathCacheInterface &) = default;

public:
    SourcePathCacheInterface() = default;

    SourcePathCacheInterface(SourcePathCacheInterface &&) = default;
    SourcePathCacheInterface &operator=(SourcePathCacheInterface &&) = default;

    virtual void populateIfEmpty() = 0;

    virtual SourceId sourceId(SourcePathView sourcePath) const = 0;

    virtual SourceId sourceId(DirectoryPathId directoryPathId,
                              Utils::SmallStringView fileName) const
        = 0;

    virtual FileNameId fileNameId(Utils::SmallStringView fileName) const = 0;

    virtual DirectoryPathId directoryPathId(Utils::SmallStringView directoryPath) const = 0;

    virtual SourcePath sourcePath(SourceId sourceId) const = 0;

    virtual Utils::PathString directoryPath(DirectoryPathId directoryPathId) const = 0;

    virtual Utils::SmallString fileName(FileNameId fileNameId) const = 0;

protected:
    ~SourcePathCacheInterface() = default;
};
} // namespace QmlDesigner
