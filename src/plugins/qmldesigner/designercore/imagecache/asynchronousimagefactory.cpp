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

}

void AsynchronousImageFactory::generate(Utils::SmallStringView name,
                                        Utils::SmallStringView extraId,
                                        ImageCache::AuxiliaryData auxiliaryData)
{
    m_taskQueue.addTask(name, extraId, std::move(auxiliaryData));
}

AsynchronousImageFactory::~AsynchronousImageFactory() {}

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

    auto capture = [&](const QImage &image, const QImage &midSizeImage, const QImage &smallImage) {
        storage.storeImage(id, currentModifiedTime, image, midSizeImage, smallImage);
    };

    collector.start(name,
                    extraId,
                    std::move(auxiliaryData),
                    std::move(capture),
                    ImageCache::AbortCallback{});
}

void AsynchronousImageFactory::clean()
{
    m_taskQueue.clean();
}
} // namespace QmlDesigner
