// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imagecacheauxiliarydata.h"

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

    void generate(Utils::PathString name,
                  Utils::SmallString extraId = {},
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

    void addEntry(Utils::PathString &&name,
                  Utils::SmallString &&extraId,
                  ImageCache::AuxiliaryData &&auxiliaryData);
    bool isRunning();
    void waitForEntries();
    std::optional<Entry> getEntry();
    void request(Utils::SmallStringView name,
                 Utils::SmallStringView extraId,
                 ImageCache::AuxiliaryData auxiliaryData,
                 ImageCacheStorageInterface &storage,
                 TimeStampProviderInterface &timeStampProvider,
                 ImageCacheCollectorInterface &collector);
    void wait();
    void clearEntries();
    void stopThread();

private:
    std::deque<Entry> m_entries;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::thread m_backgroundThread;
    ImageCacheStorageInterface &m_storage;
    TimeStampProviderInterface &m_timeStampProvider;
    ImageCacheCollectorInterface &m_collector;
    bool m_finishing{false};
};

} // namespace QmlDesigner
