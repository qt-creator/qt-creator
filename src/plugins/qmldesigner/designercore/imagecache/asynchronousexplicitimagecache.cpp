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

void AsynchronousExplicitImageCache::wait()
{
    stopThread();
    m_condition.notify_all();
    if (m_backgroundThread.joinable())
        m_backgroundThread.join();
}

void AsynchronousExplicitImageCache::requestImage(Utils::SmallStringView name,
                                                  ImageCache::CaptureImageCallback captureCallback,
                                                  ImageCache::AbortCallback abortCallback,
                                                  Utils::SmallStringView extraId)
{
    addEntry(name, extraId, std::move(captureCallback), std::move(abortCallback), RequestType::Image);
    m_condition.notify_all();
}

void AsynchronousExplicitImageCache::requestMidSizeImage(Utils::SmallStringView name,
                                                         ImageCache::CaptureImageCallback captureCallback,
                                                         ImageCache::AbortCallback abortCallback,
                                                         Utils::SmallStringView extraId)
{
    addEntry(name, extraId, std::move(captureCallback), std::move(abortCallback), RequestType::MidSizeImage);
    m_condition.notify_all();
}

void AsynchronousExplicitImageCache::requestSmallImage(Utils::SmallStringView name,
                                                       ImageCache::CaptureImageCallback captureCallback,
                                                       ImageCache::AbortCallback abortCallback,
                                                       Utils::SmallStringView extraId)
{
    addEntry(name, extraId, std::move(captureCallback), std::move(abortCallback), RequestType::SmallImage);
    m_condition.notify_all();
}

void AsynchronousExplicitImageCache::clean()
{
    clearEntries();
}

std::optional<AsynchronousExplicitImageCache::RequestEntry> AsynchronousExplicitImageCache::getEntry(
    std::unique_lock<std::mutex> lock)
{
    auto l = std::move(lock);

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

    ensureThreadIsRunning();

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

std::tuple<std::unique_lock<std::mutex>, bool> AsynchronousExplicitImageCache::waitForEntries()
{
    using namespace std::literals::chrono_literals;
    std::unique_lock lock{m_mutex};
    if (m_finishing)
        return {std::move(lock), true};
    if (m_requestEntries.empty()) {
        auto timedOutWithoutEntriesOrFinishing = !m_condition.wait_for(lock, 10min, [&] {
            return m_requestEntries.size() || m_finishing;
        });

        if (timedOutWithoutEntriesOrFinishing || m_finishing) {
            m_sleeping = true;
            return {std::move(lock), true};
        }
    }
    return {std::move(lock), false};
}

void AsynchronousExplicitImageCache::stopThread()
{
    std::unique_lock lock{m_mutex};
    m_finishing = true;
}

void AsynchronousExplicitImageCache::ensureThreadIsRunning()
{
    if (m_finishing)
        return;

    if (!m_sleeping)
        return;

    if (m_backgroundThread.joinable())
        m_backgroundThread.join();

    m_sleeping = false;

    m_backgroundThread = std::thread{[this] {
        while (true) {
            auto [lock, abort] = waitForEntries();
            if (abort)
                return;

            if (auto entry = getEntry(std::move(lock)); entry) {
                request(entry->name,
                        entry->extraId,
                        entry->requestType,
                        std::move(entry->captureCallback),
                        std::move(entry->abortCallback),
                        m_storage);
            }
        }
    }};
}

} // namespace QmlDesigner
