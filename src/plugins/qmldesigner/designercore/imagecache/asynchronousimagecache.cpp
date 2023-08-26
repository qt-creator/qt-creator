// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "asynchronousimagecache.h"

#include "imagecachegenerator.h"
#include "imagecachestorage.h"
#include "timestampprovider.h"

#include <thread>

namespace QmlDesigner {

AsynchronousImageCache::AsynchronousImageCache(ImageCacheStorageInterface &storage,
                                               ImageCacheGeneratorInterface &generator,
                                               TimeStampProviderInterface &timeStampProvider)
    : m_storage(storage)
    , m_generator(generator)
    , m_timeStampProvider(timeStampProvider)
{
}

AsynchronousImageCache::~AsynchronousImageCache()
{
    clean();
    wait();
}

void AsynchronousImageCache::request(Utils::SmallStringView name,
                                     Utils::SmallStringView extraId,
                                     AsynchronousImageCache::RequestType requestType,
                                     ImageCache::CaptureImageCallback captureCallback,
                                     ImageCache::AbortCallback abortCallback,
                                     ImageCache::AuxiliaryData auxiliaryData,
                                     ImageCacheStorageInterface &storage,
                                     ImageCacheGeneratorInterface &generator,
                                     TimeStampProviderInterface &timeStampProvider)
{
    const auto id = extraId.empty() ? Utils::PathString{name}
                                    : Utils::PathString::join({name, "+", extraId});

    const auto timeStamp = timeStampProvider.timeStamp(name);
    auto requestImageFromStorage = [&](RequestType requestType) {
        switch (requestType) {
        case RequestType::Image:
            return storage.fetchImage(id, timeStamp);
        case RequestType::MidSizeImage:
            return storage.fetchMidSizeImage(id, timeStamp);
        case RequestType::SmallImage:
            return storage.fetchSmallImage(id, timeStamp);
        default:
            break;
        }

        return storage.fetchImage(id, timeStamp);
    };
    const auto entry = requestImageFromStorage(requestType);

    if (entry) {
        if (entry->isNull())
            abortCallback(ImageCache::AbortReason::Failed);
        else
            captureCallback(*entry);
    } else {
        auto callback =
            [captureCallback = std::move(captureCallback),
             requestType](const QImage &image, const QImage &midSizeImage, const QImage &smallImage) {
                auto selectImage = [](RequestType requestType,
                                      const QImage &image,
                                      const QImage &midSizeImage,
                                      const QImage &smallImage) {
                    switch (requestType) {
                    case RequestType::Image:
                        return image;
                    case RequestType::MidSizeImage:
                        return midSizeImage;
                    case RequestType::SmallImage:
                        return smallImage;
                    default:
                        break;
                    }

                    return image;
                };
                captureCallback(selectImage(requestType, image, midSizeImage, smallImage));
            };
        generator.generateImage(name,
                                extraId,
                                timeStamp,
                                std::move(callback),
                                std::move(abortCallback),
                                std::move(auxiliaryData));
    }
}

void AsynchronousImageCache::wait()
{
    stopThread();
    m_condition.notify_all();
    if (m_backgroundThread.joinable())
        m_backgroundThread.join();
}

void AsynchronousImageCache::requestImage(Utils::SmallStringView name,
                                          ImageCache::CaptureImageCallback captureCallback,
                                          ImageCache::AbortCallback abortCallback,
                                          Utils::SmallStringView extraId,
                                          ImageCache::AuxiliaryData auxiliaryData)
{
    addEntry(std::move(name),
             std::move(extraId),
             std::move(captureCallback),
             std::move(abortCallback),
             std::move(auxiliaryData),
             RequestType::Image);
    m_condition.notify_all();
}

void AsynchronousImageCache::requestMidSizeImage(Utils::SmallStringView name,
                                                 ImageCache::CaptureImageCallback captureCallback,
                                                 ImageCache::AbortCallback abortCallback,
                                                 Utils::SmallStringView extraId,
                                                 ImageCache::AuxiliaryData auxiliaryData)
{
    addEntry(std::move(name),
             std::move(extraId),
             std::move(captureCallback),
             std::move(abortCallback),
             std::move(auxiliaryData),
             RequestType::MidSizeImage);
    m_condition.notify_all();
}

void AsynchronousImageCache::requestSmallImage(Utils::SmallStringView name,
                                               ImageCache::CaptureImageCallback captureCallback,
                                               ImageCache::AbortCallback abortCallback,
                                               Utils::SmallStringView extraId,
                                               ImageCache::AuxiliaryData auxiliaryData)
{
    addEntry(std::move(name),
             std::move(extraId),
             std::move(captureCallback),
             std::move(abortCallback),
             std::move(auxiliaryData),
             RequestType::SmallImage);
    m_condition.notify_all();
}

void AsynchronousImageCache::clean()
{
    clearEntries();
    m_generator.clean();
}

std::optional<AsynchronousImageCache::Entry> AsynchronousImageCache::getEntry(
    std::unique_lock<std::mutex> lock)
{
    auto l = std::move(lock);
    if (m_entries.empty())
        return {};

    Entry entry = m_entries.front();
    m_entries.pop_front();

    return {entry};
}

void AsynchronousImageCache::addEntry(Utils::PathString &&name,
                                      Utils::SmallString &&extraId,
                                      ImageCache::CaptureImageCallback &&captureCallback,
                                      ImageCache::AbortCallback &&abortCallback,
                                      ImageCache::AuxiliaryData &&auxiliaryData,
                                      RequestType requestType)
{
    std::unique_lock lock{m_mutex};

    ensureThreadIsRunning();

    m_entries.emplace_back(std::move(name),
                           std::move(extraId),
                           std::move(captureCallback),
                           std::move(abortCallback),
                           std::move(auxiliaryData),
                           requestType);
}

void AsynchronousImageCache::clearEntries()
{
    std::unique_lock lock{m_mutex};
    for (Entry &entry : m_entries)
        entry.abortCallback(ImageCache::AbortReason::Abort);
    m_entries.clear();
}

std::tuple<std::unique_lock<std::mutex>, bool> AsynchronousImageCache::waitForEntries()
{
    using namespace std::literals::chrono_literals;
    std::unique_lock lock{m_mutex};
    if (m_finishing)
        return {std::move(lock), true};
    if (m_entries.empty()) {
        auto timedOutWithoutEntriesOrFinishing = !m_condition.wait_for(lock, 10min, [&] {
            return m_entries.size() || m_finishing;
        });

        if (timedOutWithoutEntriesOrFinishing || m_finishing) {
            m_sleeping = true;
            return {std::move(lock), true};
        }
    }
    return {std::move(lock), false};
}

void AsynchronousImageCache::stopThread()
{
    std::unique_lock lock{m_mutex};
    m_finishing = true;
}

void AsynchronousImageCache::ensureThreadIsRunning()
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
                        std::move(entry->auxiliaryData),
                        m_storage,
                        m_generator,
                        m_timeStampProvider);
            }
        }
    }};
}

} // namespace QmlDesigner
