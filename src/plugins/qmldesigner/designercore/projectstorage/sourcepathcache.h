// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorage.h"
#include "projectstorageexceptions.h"
#include "projectstorageids.h"
#include "sourcepath.h"
#include "sourcepathcacheinterface.h"
#include "sourcepathcachetypes.h"
#include "sourcepathview.h"
#include "storagecache.h"

#include <modelfwd.h>
#include <sqlitetransaction.h>

#include <algorithm>
#include <utility>

namespace QmlDesigner {

template<typename ProjectStorage, typename Mutex>
class SourcePathCache final : public SourcePathCacheInterface
{
    SourcePathCache(const SourcePathCache &) = default;
    SourcePathCache &operator=(const SourcePathCache &) = default;

    template<typename Storage, typename M>
    friend class SourcePathCache;

public:
    SourcePathCache(ProjectStorage &projectStorage)
        : m_sourceContextStorageAdapter{projectStorage}
        , m_sourceStorageAdapter{projectStorage}

    {
        populateIfEmpty();
    }

    SourcePathCache(SourcePathCache &&) = default;
    SourcePathCache &operator=(SourcePathCache &&) = default;

    void populateIfEmpty() override
    {
        if (m_sourcePathCache.isEmpty()) {
            m_sourceContextPathCache.populate();
            m_sourcePathCache.populate();
        }
    }

    std::pair<SourceContextId, SourceId> sourceContextAndSourceId(
        SourcePathView sourcePath) const override
    {
        Utils::SmallStringView sourceContextPath = sourcePath.directory();

        auto sourceContextId = m_sourceContextPathCache.id(sourceContextPath);

        Utils::SmallStringView sourceName = sourcePath.name();

        auto sourceId = m_sourcePathCache.id({sourceName, sourceContextId});

        return {sourceContextId, sourceId};
    }

    SourceId sourceId(SourcePathView sourcePath) const override
    {
        return sourceContextAndSourceId(sourcePath).second;
    }

    SourceId sourceId(SourceContextId sourceContextId,
                      Utils::SmallStringView sourceName) const override
    {
        return m_sourcePathCache.id({sourceName, sourceContextId});
    }

    SourceContextId sourceContextId(Utils::SmallStringView sourceContextPath) const override
    {
        Utils::SmallStringView path = sourceContextPath.back() == '/'
                                          ? sourceContextPath.mid(0, sourceContextPath.size() - 1)
                                          : sourceContextPath;

        return m_sourceContextPathCache.id(path);
    }

    SourcePath sourcePath(SourceId sourceId) const override
    {
        if (Q_UNLIKELY(!sourceId.isValid()))
            throw NoSourcePathForInvalidSourceId();

        auto entry = m_sourcePathCache.value(sourceId);

        Utils::PathString sourceContextPath = m_sourceContextPathCache.value(entry.sourceContextId);

        return SourcePath{sourceContextPath, entry.sourceName};
    }

    Utils::PathString sourceContextPath(SourceContextId sourceContextId) const override
    {
        if (Q_UNLIKELY(!sourceContextId.isValid()))
            throw NoSourceContextPathForInvalidSourceContextId();

        return m_sourceContextPathCache.value(sourceContextId);
    }

    SourceContextId sourceContextId(SourceId sourceId) const override
    {
        if (Q_UNLIKELY(!sourceId.isValid()))
            throw NoSourcePathForInvalidSourceId();

        return m_sourcePathCache.value(sourceId).sourceContextId;
    }

private:
    class SourceContextStorageAdapter
    {
    public:
        auto fetchId(const Utils::SmallStringView sourceContextPath)
        {
            return storage.fetchSourceContextId(sourceContextPath);
        }

        auto fetchValue(SourceContextId id) { return storage.fetchSourceContextPath(id); }

        auto fetchAll() { return storage.fetchAllSourceContexts(); }

        ProjectStorage &storage;
    };

    class SourceStorageAdapter
    {
    public:
        auto fetchId(Cache::SourceNameView sourceNameView)
        {
            return storage.fetchSourceId(sourceNameView.sourceContextId, sourceNameView.sourceName);
        }

        auto fetchValue(SourceId id)
        {
            auto entry = storage.fetchSourceNameAndSourceContextId(id);

            return Cache::SourceNameEntry{std::move(entry.sourceName), entry.sourceContextId};
        }

        auto fetchAll() { return storage.fetchAllSources(); }

        ProjectStorage &storage;
    };

    static bool sourceContextLess(Utils::SmallStringView first, Utils::SmallStringView second) noexcept
    {
        return std::lexicographical_compare(first.rbegin(),
                                            first.rend(),
                                            second.rbegin(),
                                            second.rend());
    }

    static bool sourceLess(Cache::SourceNameView first, Cache::SourceNameView second) noexcept
    {
        return first < second;
    }

    using SourceContextPathCache = StorageCache<Utils::PathString,
                                                Utils::SmallStringView,
                                                SourceContextId,
                                                SourceContextStorageAdapter,
                                                Mutex,
                                                sourceContextLess,
                                                Cache::SourceContext>;
    using SourceNameCache = StorageCache<Cache::SourceNameEntry,
                                         Cache::SourceNameView,
                                         SourceId,
                                         SourceStorageAdapter,
                                         Mutex,
                                         sourceLess,
                                         Cache::Source>;

private:
    SourceContextStorageAdapter m_sourceContextStorageAdapter;
    SourceStorageAdapter m_sourceStorageAdapter;
    mutable SourceContextPathCache m_sourceContextPathCache{m_sourceContextStorageAdapter};
    mutable SourceNameCache m_sourcePathCache{m_sourceStorageAdapter};
};

} // namespace QmlDesigner
