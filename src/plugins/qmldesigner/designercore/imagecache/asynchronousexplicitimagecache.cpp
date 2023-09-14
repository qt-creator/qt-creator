// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "asynchronousexplicitimagecache.h"

#include "imagecachestorage.h"

#include <thread>

namespace QmlDesigner {

AsynchronousExplicitImageCache::AsynchronousExplicitImageCache(ImageCacheStorageInterface &storage)
    : m_storage(storage)
{

}

AsynchronousExplicitImageCache::~AsynchronousExplicitImageCache()
{
}

void AsynchronousExplicitImageCache::request(Utils::SmallStringView name,
                                             Utils::SmallStringView extraId,
                                             AsynchronousExplicitImageCache::RequestType requestType,
                                             ImageCache::CaptureImageCallback captureCallback,
                                             ImageCache::AbortCallback abortCallback,
                                             ImageCacheStorageInterface &storage)
{
    const auto id = extraId.empty() ? Utils::PathString{name}
                                    : Utils::PathString::join({name, "+", extraId});

    auto requestImageFromStorage = [&](RequestType requestType) {
        switch (requestType) {
        case RequestType::Image:
            return storage.fetchImage(id, Sqlite::TimeStamp{});
        case RequestType::MidSizeImage:
            return storage.fetchMidSizeImage(id, Sqlite::TimeStamp{});
        case RequestType::SmallImage:
            return storage.fetchSmallImage(id, Sqlite::TimeStamp{});
        default:
            break;
        }

        return storage.fetchImage(id, Sqlite::TimeStamp{});
    };
    const auto entry = requestImageFromStorage(requestType);

    if (entry) {
        if (entry->isNull())
            abortCallback(ImageCache::AbortReason::Failed);
        else
            captureCallback(*entry);
    } else {
        abortCallback(ImageCache::AbortReason::NoEntry);
    }
}

void AsynchronousExplicitImageCache::requestImage(Utils::SmallStringView name,
                                                  ImageCache::CaptureImageCallback captureCallback,
                                                  ImageCache::AbortCallback abortCallback,
                                                  Utils::SmallStringView extraId)
{
    m_taskQueue.addTask(name,
                        extraId,
                        std::move(captureCallback),
                        std::move(abortCallback),
                        RequestType::Image);
}

void AsynchronousExplicitImageCache::requestMidSizeImage(Utils::SmallStringView name,
                                                         ImageCache::CaptureImageCallback captureCallback,
                                                         ImageCache::AbortCallback abortCallback,
                                                         Utils::SmallStringView extraId)
{
    m_taskQueue.addTask(name,
                        extraId,
                        std::move(captureCallback),
                        std::move(abortCallback),
                        RequestType::MidSizeImage);
}

void AsynchronousExplicitImageCache::requestSmallImage(Utils::SmallStringView name,
                                                       ImageCache::CaptureImageCallback captureCallback,
                                                       ImageCache::AbortCallback abortCallback,
                                                       Utils::SmallStringView extraId)
{
    m_taskQueue.addTask(name,
                        extraId,
                        std::move(captureCallback),
                        std::move(abortCallback),
                        RequestType::SmallImage);
}

void AsynchronousExplicitImageCache::clean()
{
    m_taskQueue.clean();
}

} // namespace QmlDesigner
