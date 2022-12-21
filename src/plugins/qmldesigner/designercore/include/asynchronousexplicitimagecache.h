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

class ImageCacheStorageInterface;

class AsynchronousExplicitImageCache
{
public:
    ~AsynchronousExplicitImageCache();

    AsynchronousExplicitImageCache(ImageCacheStorageInterface &storage);

    void requestImage(Utils::PathString name,
                      ImageCache::CaptureImageCallback captureCallback,
                      ImageCache::AbortCallback abortCallback,
                      Utils::SmallString extraId = {});
    void requestSmallImage(Utils::PathString name,
                           ImageCache::CaptureImageCallback captureCallback,
                           ImageCache::AbortCallback abortCallback,
                           Utils::SmallString extraId = {});

    void clean();

private:
    enum class RequestType { Image, SmallImage, Icon };
    struct RequestEntry
    {
        RequestEntry() = default;
        RequestEntry(Utils::PathString name,
                     Utils::SmallString extraId,
                     ImageCache::CaptureImageCallback &&captureCallback,
                     ImageCache::AbortCallback &&abortCallback,
                     RequestType requestType)
            : name{std::move(name)}
            , extraId{std::move(extraId)}
            , captureCallback{std::move(captureCallback)}
            , abortCallback{std::move(abortCallback)}
            , requestType{requestType}
        {}

        Utils::PathString name;
        Utils::SmallString extraId;
        ImageCache::CaptureImageCallback captureCallback;
        ImageCache::AbortCallback abortCallback;
        RequestType requestType = RequestType::Image;
    };

    std::optional<RequestEntry> getEntry();
    void addEntry(Utils::PathString &&name,
                  Utils::SmallString &&extraId,
                  ImageCache::CaptureImageCallback &&captureCallback,
                  ImageCache::AbortCallback &&abortCallback,
                  RequestType requestType);
    void clearEntries();
    void waitForEntries();
    void stopThread();
    bool isRunning();
    static void request(Utils::SmallStringView name,
                        Utils::SmallStringView extraId,
                        AsynchronousExplicitImageCache::RequestType requestType,
                        ImageCache::CaptureImageCallback captureCallback,
                        ImageCache::AbortCallback abortCallback,
                        ImageCacheStorageInterface &storage);

private:
    void wait();

private:
    std::deque<RequestEntry> m_requestEntries;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::thread m_backgroundThread;
    ImageCacheStorageInterface &m_storage;
    bool m_finishing{false};
};

} // namespace QmlDesigner
