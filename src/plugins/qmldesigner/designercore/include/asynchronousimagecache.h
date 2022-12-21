// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "asynchronousimagecacheinterface.h"

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

namespace QmlDesigner {

class TimeStampProviderInterface;
class ImageCacheStorageInterface;
class ImageCacheGeneratorInterface;
class ImageCacheCollectorInterface;

class AsynchronousImageCache final : public AsynchronousImageCacheInterface
{
public:
    ~AsynchronousImageCache();

    AsynchronousImageCache(ImageCacheStorageInterface &storage,
                           ImageCacheGeneratorInterface &generator,
                           TimeStampProviderInterface &timeStampProvider);

    void requestImage(Utils::PathString name,
                      ImageCache::CaptureImageCallback captureCallback,
                      ImageCache::AbortCallback abortCallback,
                      Utils::SmallString extraId = {},
                      ImageCache::AuxiliaryData auxiliaryData = {}) override;
    void requestSmallImage(Utils::PathString name,
                           ImageCache::CaptureImageCallback captureCallback,
                           ImageCache::AbortCallback abortCallback,
                           Utils::SmallString extraId = {},
                           ImageCache::AuxiliaryData auxiliaryData = {}) override;

    void clean();

private:
    enum class RequestType { Image, SmallImage, Icon };
    struct Entry
    {
        Entry() = default;
        Entry(Utils::PathString name,
              Utils::SmallString extraId,
              ImageCache::CaptureImageCallback &&captureCallback,
              ImageCache::AbortCallback &&abortCallback,
              ImageCache::AuxiliaryData &&auxiliaryData,
              RequestType requestType)
            : name{std::move(name)}
            , extraId{std::move(extraId)}
            , captureCallback{std::move(captureCallback)}
            , abortCallback{std::move(abortCallback)}
            , auxiliaryData{std::move(auxiliaryData)}
            , requestType{requestType}
        {}

        Utils::PathString name;
        Utils::SmallString extraId;
        ImageCache::CaptureImageCallback captureCallback;
        ImageCache::AbortCallback abortCallback;
        ImageCache::AuxiliaryData auxiliaryData;
        RequestType requestType = RequestType::Image;
    };

    std::optional<Entry> getEntry();
    void addEntry(Utils::PathString &&name,
                  Utils::SmallString &&extraId,
                  ImageCache::CaptureImageCallback &&captureCallback,
                  ImageCache::AbortCallback &&abortCallback,
                  ImageCache::AuxiliaryData &&auxiliaryData,
                  RequestType requestType);
    void clearEntries();
    void waitForEntries();
    void stopThread();
    bool isRunning();
    static void request(Utils::SmallStringView name,
                        Utils::SmallStringView extraId,
                        AsynchronousImageCache::RequestType requestType,
                        ImageCache::CaptureImageCallback captureCallback,
                        ImageCache::AbortCallback abortCallback,
                        ImageCache::AuxiliaryData auxiliaryData,
                        ImageCacheStorageInterface &storage,
                        ImageCacheGeneratorInterface &generator,
                        TimeStampProviderInterface &timeStampProvider);

private:
    void wait();

private:
    std::deque<Entry> m_entries;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::thread m_backgroundThread;
    ImageCacheStorageInterface &m_storage;
    ImageCacheGeneratorInterface &m_generator;
    TimeStampProviderInterface &m_timeStampProvider;
    bool m_finishing{false};
};

} // namespace QmlDesigner
