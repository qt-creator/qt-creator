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

    virtual std::pair<SourceContextId, SourceId>
    sourceContextAndSourceId(SourcePathView sourcePath) const = 0;

    virtual SourceId sourceId(SourcePathView sourcePath) const = 0;

    virtual SourceId sourceId(SourceContextId sourceContextId,
                              Utils::SmallStringView sourceName) const
        = 0;

    virtual SourceContextId sourceContextId(Utils::SmallStringView sourceContextPath) const = 0;

    virtual SourcePath sourcePath(SourceId sourceId) const = 0;

    virtual Utils::PathString sourceContextPath(SourceContextId sourceContextId) const = 0;
    virtual SourceContextId sourceContextId(SourceId sourceId) const = 0;

protected:
    ~SourcePathCacheInterface() = default;
};
} // namespace QmlDesigner
