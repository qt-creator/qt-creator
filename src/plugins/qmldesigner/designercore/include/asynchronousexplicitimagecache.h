// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "asynchronousimagecacheinterface.h"

#include <imagecache/taskqueue.h>

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

    void requestImage(Utils::SmallStringView name,
                      ImageCache::CaptureImageCallback captureCallback,
                      ImageCache::AbortCallback abortCallback,
                      Utils::SmallStringView extraId = {});
    void requestMidSizeImage(Utils::SmallStringView name,
                             ImageCache::CaptureImageCallback captureCallback,
                             ImageCache::AbortCallback abortCallback,
                             Utils::SmallStringView extraId = {});
    void requestSmallImage(Utils::SmallStringView name,
                           ImageCache::CaptureImageCallback captureCallback,
                           ImageCache::AbortCallback abortCallback,
                           Utils::SmallStringView extraId = {});

    void clean();

private:
    enum class RequestType { Image, MidSizeImage, SmallImage, Icon };
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

    static void request(Utils::SmallStringView name,
                        Utils::SmallStringView extraId,
                        AsynchronousExplicitImageCache::RequestType requestType,
                        ImageCache::CaptureImageCallback captureCallback,
                        ImageCache::AbortCallback abortCallback,
                        ImageCacheStorageInterface &storage);

    struct Dispatch
    {
        void operator()(RequestEntry &entry)
        {
            request(entry.name,
                    entry.extraId,
                    entry.requestType,
                    std::move(entry.captureCallback),
                    std::move(entry.abortCallback),
                    storage);
        }

        ImageCacheStorageInterface &storage;
    };

    struct Clean
    {
        void operator()(RequestEntry &entry)
        {
            entry.abortCallback(ImageCache::AbortReason::Abort);
        }
    };

private:
    std::deque<RequestEntry> m_requestEntries;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::thread m_backgroundThread;
    ImageCacheStorageInterface &m_storage;
    TaskQueue<RequestEntry, Dispatch, Clean> m_taskQueue{Dispatch{m_storage}, Clean{}};
};

} // namespace QmlDesigner
