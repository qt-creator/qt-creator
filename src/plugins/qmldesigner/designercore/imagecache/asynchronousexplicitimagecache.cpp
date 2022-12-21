// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "asynchronousexplicitimagecache.h"

#include "imagecachestorage.h"

#include <thread>

namespace QmlDesigner {

AsynchronousExplicitImageCache::AsynchronousExplicitImageCache(ImageCacheStorageInterface &storage)
    : m_storage(storage)
{
    m_backgroundThread = std::thread{[this] {
        while (isRunning()) {
            if (auto entry = getEntry(); entry) {
                request(entry->name,
                        entry->extraId,
                        entry->requestType,
                        std::move(entry->captureCallback),
                        std::move(entry->abortCallback),
                        m_storage);
            }

            waitForEntries();
        }
    }};
}

AsynchronousExplicitImageCache::~AsynchronousExplicitImageCache()
{
    clean();
    wait();
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

    const auto entry = requestType == RequestType::Image
                           ? storage.fetchImage(id, Sqlite::TimeStamp{})
                           : storage.fetchSmallImage(id, Sqlite::TimeStamp{});

    if (entry && !entry->isNull())
        captureCallback(*entry);
    else
        abortCallback(ImageCache::AbortReason::Failed);
}

void AsynchronousExplicitImageCache::wait()
{
    stopThread();
    m_condition.notify_all();
    if (m_backgroundThread.joinable())
        m_backgroundThread.join();
}

void AsynchronousExplicitImageCache::requestImage(Utils::PathString name,
                                                  ImageCache::CaptureImageCallback captureCallback,
                                                  ImageCache::AbortCallback abortCallback,
                                                  Utils::SmallString extraId)
{
    addEntry(std::move(name),
             std::move(extraId),
             std::move(captureCallback),
             std::move(abortCallback),
             RequestType::Image);
    m_condition.notify_all();
}

void AsynchronousExplicitImageCache::requestSmallImage(Utils::PathString name,
                                                       ImageCache::CaptureImageCallback captureCallback,
                                                       ImageCache::AbortCallback abortCallback,
                                                       Utils::SmallString extraId)
{
    addEntry(std::move(name),
             std::move(extraId),
             std::move(captureCallback),
             std::move(abortCallback),
             RequestType::SmallImage);
    m_condition.notify_all();
}

void AsynchronousExplicitImageCache::clean()
{
    clearEntries();
}

std::optional<AsynchronousExplicitImageCache::RequestEntry> AsynchronousExplicitImageCache::getEntry()
{
    std::unique_lock lock{m_mutex};

    if (m_requestEntries.empty())
        return {};

    RequestEntry entry = m_requestEntries.front();
    m_requestEntries.pop_front();

    return {entry};
}

void AsynchronousExplicitImageCache::addEntry(Utils::PathString &&name,
                                              Utils::SmallString &&extraId,
                                              ImageCache::CaptureImageCallback &&captureCallback,
                                              ImageCache::AbortCallback &&abortCallback,
                                              RequestType requestType)
{
    std::unique_lock lock{m_mutex};

    m_requestEntries.emplace_back(std::move(name),
                                  std::move(extraId),
                                  std::move(captureCallback),
                                  std::move(abortCallback),
                                  requestType);
}

void AsynchronousExplicitImageCache::clearEntries()
{
    std::unique_lock lock{m_mutex};
    for (RequestEntry &entry : m_requestEntries)
        entry.abortCallback(ImageCache::AbortReason::Abort);
    m_requestEntries.clear();
}

void AsynchronousExplicitImageCache::waitForEntries()
{
    std::unique_lock lock{m_mutex};
    if (m_requestEntries.empty())
        m_condition.wait(lock, [&] { return m_requestEntries.size() || m_finishing; });
}

void AsynchronousExplicitImageCache::stopThread()
{
    std::unique_lock lock{m_mutex};
    m_finishing = true;
}

bool AsynchronousExplicitImageCache::isRunning()
{
    std::unique_lock lock{m_mutex};
    return !m_finishing || m_requestEntries.size();
}

} // namespace QmlDesigner
