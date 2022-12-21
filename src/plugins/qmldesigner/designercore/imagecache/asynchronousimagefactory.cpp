// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "asynchronousimagefactory.h"

#include "imagecachecollector.h"
#include "imagecachegenerator.h"
#include "imagecachestorage.h"
#include "timestampproviderinterface.h"

namespace QmlDesigner {

AsynchronousImageFactory::AsynchronousImageFactory(ImageCacheStorageInterface &storage,
                                                   TimeStampProviderInterface &timeStampProvider,
                                                   ImageCacheCollectorInterface &collector)
    : m_storage(storage)
    , m_timeStampProvider(timeStampProvider)
    , m_collector(collector)
{
    m_backgroundThread = std::thread{[this] {
        while (isRunning()) {
            if (auto entry = getEntry(); entry) {
                request(entry->name,
                        entry->extraId,
                        std::move(entry->auxiliaryData),
                        m_storage,
                        m_timeStampProvider,
                        m_collector);
            }

            waitForEntries();
        }
    }};
}

AsynchronousImageFactory::~AsynchronousImageFactory()
{
    clean();
    wait();
}

void AsynchronousImageFactory::generate(Utils::PathString name,
                                        Utils::SmallString extraId,
                                        ImageCache::AuxiliaryData auxiliaryData)
{
    addEntry(std::move(name), std::move(extraId), std::move(auxiliaryData));
    m_condition.notify_all();
}

void AsynchronousImageFactory::addEntry(Utils::PathString &&name,
                                        Utils::SmallString &&extraId,
                                        ImageCache::AuxiliaryData &&auxiliaryData)
{
    std::unique_lock lock{m_mutex};

    m_entries.emplace_back(std::move(name), std::move(extraId), std::move(auxiliaryData));
}

bool AsynchronousImageFactory::isRunning()
{
    std::unique_lock lock{m_mutex};
    return !m_finishing || m_entries.size();
}

void AsynchronousImageFactory::waitForEntries()
{
    std::unique_lock lock{m_mutex};
    if (m_entries.empty())
        m_condition.wait(lock, [&] { return m_entries.size() || m_finishing; });
}

std::optional<AsynchronousImageFactory::Entry> AsynchronousImageFactory::getEntry()
{
    std::unique_lock lock{m_mutex};

    if (m_entries.empty())
        return {};

    Entry entry = m_entries.front();
    m_entries.pop_front();

    return {entry};
}

void AsynchronousImageFactory::request(Utils::SmallStringView name,
                                       Utils::SmallStringView extraId,
                                       ImageCache::AuxiliaryData auxiliaryData,
                                       ImageCacheStorageInterface &storage,
                                       TimeStampProviderInterface &timeStampProvider,
                                       ImageCacheCollectorInterface &collector)
{
    const auto id = extraId.empty() ? Utils::PathString{name}
                                    : Utils::PathString::join({name, "+", extraId});

    const auto currentModifiedTime = timeStampProvider.timeStamp(name);
    const auto storageModifiedTime = storage.fetchModifiedImageTime(id);
    const auto pause = timeStampProvider.pause();

    if (currentModifiedTime < (storageModifiedTime + pause))
        return;

    auto capture = [=](const QImage &image, const QImage &smallImage) {
        m_storage.storeImage(id, currentModifiedTime, image, smallImage);
    };

    collector.start(name,
                    extraId,
                    std::move(auxiliaryData),
                    std::move(capture),
                    ImageCache::AbortCallback{});
}

void AsynchronousImageFactory::clean()
{
    clearEntries();
}

void AsynchronousImageFactory::wait()
{
    stopThread();
    m_condition.notify_all();
    if (m_backgroundThread.joinable())
        m_backgroundThread.join();
}

void AsynchronousImageFactory::clearEntries()
{
    std::unique_lock lock{m_mutex};
    m_entries.clear();
}

void AsynchronousImageFactory::stopThread()
{
    std::unique_lock lock{m_mutex};
    m_finishing = true;
}

} // namespace QmlDesigner
