// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sourcepath.h"
#include "sourcepathcacheinterface.h"
#include "sourcepathcachetypes.h"
#include "sourcepathexceptions.h"
#include "sourcepathstorage.h"
#include "sourcepathview.h"
#include "storagecache.h"

#include <modelfwd.h>
#include <sourcepathids.h>
#include <sqlitetransaction.h>

#include <algorithm>
#include <utility>

namespace QmlDesigner {

template<typename Storage, typename Mutex>
class SourcePathCache final : public SourcePathCacheInterface
{
    SourcePathCache(const SourcePathCache &) = default;
    SourcePathCache &operator=(const SourcePathCache &) = default;

    template<typename S, typename M>
    friend class SourcePathCache;

public:
    SourcePathCache(Storage &storage)
        : m_sourceContextStorageAdapter{storage}
        , m_sourceNameStorageAdapter{storage}

    {
        populateIfEmpty();
    }

    SourcePathCache(SourcePathCache &&) = default;
    SourcePathCache &operator=(SourcePathCache &&) = default;

    void populateIfEmpty() override
    {
        if (m_sourceNameCache.isEmpty()) {
            m_sourceContextPathCache.populate();
            m_sourceNameCache.populate();
        }
    }

    SourceId sourceId(SourcePathView sourcePath) const override
    {
        Utils::SmallStringView sourceContextPath = sourcePath.directory();

        auto sourceContextId = m_sourceContextPathCache.id(sourceContextPath);

        Utils::SmallStringView sourceName = sourcePath.name();

        auto sourceNameId = m_sourceNameCache.id(sourceName);

        return SourceId::create(sourceNameId, sourceContextId);
    }

    SourceNameId sourceNameId(Utils::SmallStringView sourceName) const override
    {
        return m_sourceNameCache.id(sourceName);
    }

    SourceId sourceId(SourceContextId sourceContextId, Utils::SmallStringView sourceName) const override
    {
        SourceNameId sourceNameId = m_sourceNameCache.id(sourceName);

        return SourceId::create(sourceNameId, sourceContextId);
    }

    SourceContextId sourceContextId(Utils::SmallStringView sourceContextPath) const override
    {
        Utils::SmallStringView path = sourceContextPath.back() == '/'
                                          ? sourceContextPath.substr(0, sourceContextPath.size() - 1)
                                          : sourceContextPath;

        return m_sourceContextPathCache.id(path);
    }

    SourcePath sourcePath(SourceId sourceId) const override
    {
        if (!sourceId) [[unlikely]]
            throw NoSourcePathForInvalidSourceId();

        auto sourceName = m_sourceNameCache.value(sourceId.mainId());

        Utils::PathString sourceContextPath = m_sourceContextPathCache.value(sourceId.contextId());

        return SourcePath{sourceContextPath, sourceName};
    }

    Utils::PathString sourceContextPath(SourceContextId sourceContextId) const override
    {
        if (!sourceContextId) [[unlikely]]
            throw NoSourceContextPathForInvalidSourceContextId();

        return m_sourceContextPathCache.value(sourceContextId);
    }

    Utils::SmallString sourceName(SourceNameId sourceNameId) const override
    {
        if (!sourceNameId) [[unlikely]]
            throw NoSourceNameForInvalidSourceNameId();

        return m_sourceNameCache.value(sourceNameId);
    }

private:
    class SourceContextStorageAdapter
    {
    public:
        auto fetchId(Utils::SmallStringView sourceContextPath)
        {
            return storage.fetchSourceContextId(sourceContextPath);
        }

        auto fetchValue(SourceContextId id) { return storage.fetchSourceContextPath(id); }

        auto fetchAll() { return storage.fetchAllSourceContexts(); }

        Storage &storage;
    };

    class SourceNameStorageAdapter
    {
    public:
        auto fetchId(Utils::SmallStringView sourceNameView)
        {
            return storage.fetchSourceNameId(sourceNameView);
        }

        auto fetchValue(SourceNameId id) { return storage.fetchSourceName(id); }

        auto fetchAll() { return storage.fetchAllSourceNames(); }

        Storage &storage;
    };

    static bool sourceLess(Utils::SmallStringView first, Utils::SmallStringView second) noexcept
    {
        return std::lexicographical_compare(first.rbegin(),
                                            first.rend(),
                                            second.rbegin(),
                                            second.rend());
    }

    using SourceContextPathCache = StorageCache<Utils::PathString,
                                                Utils::SmallStringView,
                                                SourceContextId,
                                                SourceContextStorageAdapter,
                                                Mutex,
                                                sourceLess,
                                                Cache::SourceContext>;
    using SourceNameCache = StorageCache<Utils::PathString,
                                         Utils::SmallStringView,
                                         SourceNameId,
                                         SourceNameStorageAdapter,
                                         Mutex,
                                         sourceLess,
                                         Cache::SourceName>;

private:
    SourceContextStorageAdapter m_sourceContextStorageAdapter;
    SourceNameStorageAdapter m_sourceNameStorageAdapter;
    mutable SourceContextPathCache m_sourceContextPathCache{m_sourceContextStorageAdapter};
    mutable SourceNameCache m_sourceNameCache{m_sourceNameStorageAdapter};
};

} // namespace QmlDesigner
