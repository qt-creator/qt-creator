// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imagecacheauxiliarydata.h"

#include <imagecache/taskqueue.h>

#include <utils/smallstring.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>

namespace QmlDesigner {

class TimeStampProviderInterface;
class ImageCacheStorageInterface;
class ImageCacheCollectorInterface;

class AsynchronousImageFactory
{
public:
    AsynchronousImageFactory(ImageCacheStorageInterface &storage,
                             TimeStampProviderInterface &timeStampProvider,
                             ImageCacheCollectorInterface &collector);

    ~AsynchronousImageFactory();

    void generate(Utils::SmallStringView name,
                  Utils::SmallStringView extraId = {},
                  ImageCache::AuxiliaryData auxiliaryData = {});
    void clean();

private:
    struct Entry
    {
        Entry() = default;
        Entry(Utils::PathString name,
              Utils::SmallString extraId,
              ImageCache::AuxiliaryData &&auxiliaryData)
            : name{std::move(name)}
            , extraId{std::move(extraId)}
            , auxiliaryData{std::move(auxiliaryData)}
        {}

        Utils::PathString name;
        Utils::SmallString extraId;
        ImageCache::AuxiliaryData auxiliaryData;
    };

    static void request(Utils::SmallStringView name,
                        Utils::SmallStringView extraId,
                        ImageCache::AuxiliaryData auxiliaryData,
                        ImageCacheStorageInterface &storage,
                        TimeStampProviderInterface &timeStampProvider,
                        ImageCacheCollectorInterface &collector);

    struct Dispatch
    {
        void operator()(Entry &entry)
        {
            request(entry.name,
                    entry.extraId,
                    std::move(entry.auxiliaryData),
                    storage,
                    timeStampProvider,
                    collector);
        }

        ImageCacheStorageInterface &storage;
        TimeStampProviderInterface &timeStampProvider;
        ImageCacheCollectorInterface &collector;
    };

    struct Clean
    {
        void operator()(Entry &) {}
    };

private:
    ImageCacheStorageInterface &m_storage;
    TimeStampProviderInterface &m_timeStampProvider;
    ImageCacheCollectorInterface &m_collector;
    TaskQueue<Entry, Dispatch, Clean> m_taskQueue{Dispatch{m_storage, m_timeStampProvider, m_collector},
                                                  Clean{}};
};

} // namespace QmlDesigner
