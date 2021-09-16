/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "projectstorageexceptions.h"
#include "projectstorageids.h"
#include "sourcepath.h"
#include "sourcepathcachetypes.h"
#include "sourcepathview.h"
#include "storagecache.h"

#include <sqlitetransaction.h>

#include <algorithm>

namespace QmlDesigner {

template<typename ProjectStorage, typename Mutex = NonLockingMutex>
class SourcePathCache
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

    void populateIfEmpty()
    {
        if (m_sourcePathCache.isEmpty()) {
            m_sourceContextPathCache.populate();
            m_sourcePathCache.populate();
        }
    }

    SourceId sourceId(SourcePathView sourcePath) const
    {
        Utils::SmallStringView sourceContextPath = sourcePath.directory();

        auto sourceContextId = m_sourceContextPathCache.id(sourceContextPath);

        Utils::SmallStringView sourceName = sourcePath.name();

        return m_sourcePathCache.id({sourceName, sourceContextId});
    }

    SourceId sourceId(SourceContextId sourceContextId, Utils::SmallStringView sourceName) const
    {
        return m_sourcePathCache.id({sourceName, sourceContextId});
    }

    SourceContextId sourceContextId(Utils::SmallStringView sourceContextPath) const
    {
        Utils::SmallStringView path = sourceContextPath.back() == '/'
                                          ? sourceContextPath.mid(0, sourceContextPath.size() - 1)
                                          : sourceContextPath;

        return m_sourceContextPathCache.id(path);
    }

    SourcePath sourcePath(SourceId sourceId) const
    {
        if (Q_UNLIKELY(!sourceId.isValid()))
            throw NoSourcePathForInvalidSourceId();

        auto entry = m_sourcePathCache.value(sourceId);

        Utils::PathString sourceContextPath = m_sourceContextPathCache.value(entry.sourceContextId);

        return SourcePath{sourceContextPath, entry.sourceName};
    }

    Utils::PathString sourceContextPath(SourceContextId sourceContextId) const
    {
        if (Q_UNLIKELY(!sourceContextId.isValid()))
            throw NoSourceContextPathForInvalidSourceContextId();

        return m_sourceContextPathCache.value(sourceContextId);
    }

    SourceContextId sourceContextId(SourceId sourceId) const
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
        return Utils::reverseCompare(first, second) < 0;
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
