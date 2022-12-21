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
    m_backgroundThread = std::thread{[this] {
        while (isRunning()) {
            if (auto entry = getEntry(); entry) {
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

            waitForEntries();
        }
    }};
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
    const auto entry = requestType == RequestType::Image ? storage.fetchImage(id, timeStamp)
                                                         : storage.fetchSmallImage(id, timeStamp);

    if (entry) {
        if (entry->isNull())
            abortCallback(ImageCache::AbortReason::Failed);
        else
            captureCallback(*entry);
    } else {
        auto callback = [captureCallback = std::move(captureCallback),
                         requestType](const QImage &image, const QImage &smallImage) {
            captureCallback(requestType == RequestType::Image ? image : smallImage);
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

void AsynchronousImageCache::requestImage(Utils::PathString name,
                                          ImageCache::CaptureImageCallback captureCallback,
                                          ImageCache::AbortCallback abortCallback,
                                          Utils::SmallString extraId,
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

void AsynchronousImageCache::requestSmallImage(Utils::PathString name,
                                               ImageCache::CaptureImageCallback captureCallback,
                                               ImageCache::AbortCallback abortCallback,
                                               Utils::SmallString extraId,
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

std::optional<AsynchronousImageCache::Entry> AsynchronousImageCache::getEntry()
{
    std::unique_lock lock{m_mutex};

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

void AsynchronousImageCache::waitForEntries()
{
    std::unique_lock lock{m_mutex};
    if (m_entries.empty())
        m_condition.wait(lock, [&] { return m_entries.size() || m_finishing; });
}

void AsynchronousImageCache::stopThread()
{
    std::unique_lock lock{m_mutex};
    m_finishing = true;
}

bool AsynchronousImageCache::isRunning()
{
    std::unique_lock lock{m_mutex};
    return !m_finishing || m_entries.size();
}

} // namespace QmlDesigner
