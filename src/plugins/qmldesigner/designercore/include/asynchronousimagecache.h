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

    void requestImage(Utils::SmallStringView name,
                      ImageCache::CaptureImageCallback captureCallback,
                      ImageCache::AbortCallback abortCallback,
                      Utils::SmallStringView extraId = {},
                      ImageCache::AuxiliaryData auxiliaryData = {}) override;
    void requestMidSizeImage(Utils::SmallStringView name,
                             ImageCache::CaptureImageCallback captureCallback,
                             ImageCache::AbortCallback abortCallback,
                             Utils::SmallStringView extraId = {},
                             ImageCache::AuxiliaryData auxiliaryData = {}) override;
    void requestSmallImage(Utils::SmallStringView name,
                           ImageCache::CaptureImageCallback captureCallback,
                           ImageCache::AbortCallback abortCallback,
                           Utils::SmallStringView extraId = {},
                           ImageCache::AuxiliaryData auxiliaryData = {}) override;

    void clean();

private:
    enum class RequestType { Image, MidSizeImage, SmallImage, Icon };
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

    [[nodiscard]] std::optional<Entry> getEntry(std::unique_lock<std::mutex> lock);
    void addEntry(Utils::PathString &&name,
                  Utils::SmallString &&extraId,
                  ImageCache::CaptureImageCallback &&captureCallback,
                  ImageCache::AbortCallback &&abortCallback,
                  ImageCache::AuxiliaryData &&auxiliaryData,
                  RequestType requestType);
    void clearEntries();
    [[nodiscard]] std::tuple<std::unique_lock<std::mutex>, bool> waitForEntries();
    void stopThread();
    void ensureThreadIsRunning();
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
    bool m_sleeping{true};
};

} // namespace QmlDesigner
