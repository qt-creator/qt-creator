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

#include <tracing/qmldesignertracing.h>

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
        : m_directoryPathStorageAdapter{storage}
        , m_fileNameStorageAdapter{storage}

    {
        NanotraceHR::Tracer tracer{"source path cache constructor",
                                   SourcePathStorageTracing::category()};
        populateIfEmpty();
    }

    SourcePathCache(SourcePathCache &&) = default;
    SourcePathCache &operator=(SourcePathCache &&) = default;

    void populateIfEmpty() override
    {
        NanotraceHR::Tracer tracer{"source path cache populate if empty",
                                   SourcePathStorageTracing::category()};
        if (m_fileNameCache.isEmpty()) {
            tracer.tick("is not empty");
            m_directoryPathCache.populate();
            m_fileNameCache.populate();
        }
    }

    SourceId sourceId(SourcePathView sourcePath) const override
    {
        NanotraceHR::Tracer tracer{"source path cache source id",
                                   SourcePathStorageTracing::category()};

        Utils::SmallStringView directoryPath = sourcePath.directory();

        auto directoryPathId = m_directoryPathCache.id(directoryPath);

        Utils::SmallStringView fileName = sourcePath.name();

        auto fileNameId = m_fileNameCache.id(fileName);

        return SourceId::create(directoryPathId, fileNameId);
    }

    FileNameId fileNameId(Utils::SmallStringView fileName) const override
    {
        NanotraceHR::Tracer tracer{"source path cache file name id",
                                   SourcePathStorageTracing::category()};
        return m_fileNameCache.id(fileName);
    }

    SourceId sourceId(DirectoryPathId directoryPathId, Utils::SmallStringView fileName) const override
    {
        NanotraceHR::Tracer tracer{"source path cache source id from directory path id",
                                   SourcePathStorageTracing::category()};
        FileNameId fileNameId = m_fileNameCache.id(fileName);

        return SourceId::create(directoryPathId, fileNameId);
    }

    DirectoryPathId directoryPathId(Utils::SmallStringView directoryPath) const override
    {
        NanotraceHR::Tracer tracer{"source path cache directory path id",
                                   SourcePathStorageTracing::category()};

        Utils::SmallStringView path = directoryPath.back() == '/'
                                          ? directoryPath.substr(0, directoryPath.size() - 1)
                                          : directoryPath;

        return m_directoryPathCache.id(path);
    }

    SourcePath sourcePath(SourceId sourceId) const override
    {
        NanotraceHR::Tracer tracer{"source path cache source path",
                                   SourcePathStorageTracing::category()};

        if (!sourceId) [[unlikely]]
            throw NoSourcePathForInvalidSourceId();

        Utils::SmallString fileName;
        if (auto fileNameId = sourceId.fileNameId())
            fileName = m_fileNameCache.value(fileNameId);

        Utils::PathString directoryPath = m_directoryPathCache.value(sourceId.directoryPathId());

        return SourcePath{directoryPath, fileName};
    }

    Utils::PathString directoryPath(DirectoryPathId directoryPathId) const override
    {
        NanotraceHR::Tracer tracer{"source path cache directory path",
                                   SourcePathStorageTracing::category()};

        if (!directoryPathId) [[unlikely]]
            throw NoDirectoryPathForInvalidDirectoryPathId();

        return m_directoryPathCache.value(directoryPathId);
    }

    Utils::SmallString fileName(FileNameId fileNameId) const override
    {
        NanotraceHR::Tracer tracer{"source path cache file name",
                                   SourcePathStorageTracing::category()};

        if (!fileNameId) [[unlikely]]
            throw NoFileNameForInvalidFileNameId();

        return m_fileNameCache.value(fileNameId);
    }

private:
    class DirectoryPathStorageAdapter
    {
    public:
        auto fetchId(Utils::SmallStringView directoryPath)
        {
            return storage.fetchDirectoryPathId(directoryPath);
        }

        auto fetchValue(DirectoryPathId id) { return storage.fetchDirectoryPath(id); }

        auto fetchAll() { return storage.fetchAllDirectoryPaths(); }

        Storage &storage;
    };

    class FileNameStorageAdapter
    {
    public:
        auto fetchId(Utils::SmallStringView fileNameView)
        {
            return storage.fetchFileNameId(fileNameView);
        }

        auto fetchValue(FileNameId id) { return storage.fetchFileName(id); }

        auto fetchAll() { return storage.fetchAllFileNames(); }

        Storage &storage;
    };

    struct Less
    {
        bool operator()(Utils::SmallStringView first, Utils::SmallStringView second) const noexcept
        {
            return first < second;
        }
    };

    using DirectoryPathCache = StorageCache<Utils::PathString,
                                            Utils::SmallStringView,
                                            DirectoryPathId,
                                            DirectoryPathStorageAdapter,
                                            Mutex,
                                            Less,
                                            Cache::DirectoryPath>;
    using FileNameCache = StorageCache<Utils::SmallString,
                                       Utils::SmallStringView,
                                       FileNameId,
                                       FileNameStorageAdapter,
                                       Mutex,
                                       Less,
                                       Cache::FileName>;

private:
    DirectoryPathStorageAdapter m_directoryPathStorageAdapter;
    FileNameStorageAdapter m_fileNameStorageAdapter;
    mutable DirectoryPathCache m_directoryPathCache{m_directoryPathStorageAdapter};
    mutable FileNameCache m_fileNameCache{m_fileNameStorageAdapter};
};

} // namespace QmlDesigner
