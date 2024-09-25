// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "synchronousimagecache.h"

#include "imagecachecollectorinterface.h"
#include "imagecachestorage.h"
#include "timestampprovider.h"

#include <thread>

namespace QmlDesigner {

namespace {

Utils::PathString createId(Utils::PathString filePath, Utils::SmallString extraId)
{
    return extraId.empty() ? Utils::PathString{filePath}
                           : Utils::PathString::join({filePath, "+", extraId});
}
} // namespace

QImage SynchronousImageCache::image(Utils::PathString filePath,
                                    Utils::SmallString extraId,
                                    const ImageCache::AuxiliaryData &auxiliaryData)
{
    const auto id = createId(filePath, extraId);

    const auto timeStamp = m_timeStampProvider.timeStamp(filePath);
    const auto entry = m_storage.fetchImage(id, timeStamp);

    if (entry)
        return *entry;

    const auto &[image, midSizeImage, smallImage] = m_collector.createImage(filePath,
                                                                            extraId,
                                                                            auxiliaryData);

    m_storage.storeImage(id, timeStamp, image, midSizeImage, smallImage);

    return image;
}

QImage SynchronousImageCache::midSizeImage(Utils::PathString filePath,
                                           Utils::SmallString extraId,
                                           const ImageCache::AuxiliaryData &auxiliaryData)
{
    const auto id = createId(filePath, extraId);

    const auto timeStamp = m_timeStampProvider.timeStamp(filePath);
    const auto entry = m_storage.fetchMidSizeImage(id, timeStamp);

    if (entry)
        return *entry;

    const auto &[image, midSizeImage, smallImage] = m_collector.createImage(filePath,
                                                                            extraId,
                                                                            auxiliaryData);

    m_storage.storeImage(id, timeStamp, image, midSizeImage, smallImage);

    return midSizeImage;
}

QImage SynchronousImageCache::smallImage(Utils::PathString filePath,
                                         Utils::SmallString extraId,
                                         const ImageCache::AuxiliaryData &auxiliaryData)
{
    const auto id = createId(filePath, extraId);

    const auto timeStamp = m_timeStampProvider.timeStamp(filePath);
    const auto entry = m_storage.fetchSmallImage(id, timeStamp);

    if (entry)
        return *entry;

    const auto &[image, midSizeImage, smallImage] = m_collector.createImage(filePath,
                                                                            extraId,
                                                                            auxiliaryData);

    m_storage.storeImage(id, timeStamp, image, midSizeImage, smallImage);

    return smallImage;
}

QIcon SynchronousImageCache::icon(Utils::PathString filePath,
                                  Utils::SmallString extraId,
                                  const ImageCache::AuxiliaryData &auxiliaryData)
{
    const auto id = createId(filePath, extraId);

    const auto timeStamp = m_timeStampProvider.timeStamp(filePath);
    const auto entry = m_storage.fetchIcon(id, timeStamp);

    if (entry)
        return *entry;

    const auto icon = m_collector.createIcon(filePath, extraId, auxiliaryData);

    m_storage.storeIcon(id, timeStamp, icon);

    return icon;
}

} // namespace QmlDesigner
