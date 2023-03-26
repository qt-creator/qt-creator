// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imagecacheauxiliarydata.h"

#include <utils/smallstring.h>

#include <QIcon>
#include <QImage>

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace QmlDesigner {

class TimeStampProviderInterface;
class ImageCacheStorageInterface;
class ImageCacheGeneratorInterface;
class ImageCacheCollectorInterface;

class SynchronousImageCache
{
public:
    using CaptureImageCallback = std::function<void(const QImage &)>;
    using AbortCallback = std::function<void()>;

    SynchronousImageCache(ImageCacheStorageInterface &storage,
                          TimeStampProviderInterface &timeStampProvider,
                          ImageCacheCollectorInterface &collector)
        : m_storage(storage)
        , m_timeStampProvider(timeStampProvider)
        , m_collector(collector)
    {}

    QImage image(Utils::PathString filePath,
                 Utils::SmallString extraId = {},
                 const ImageCache::AuxiliaryData &auxiliaryData = {});
    QImage midSizeImage(Utils::PathString filePath,
                        Utils::SmallString extraId = {},
                        const ImageCache::AuxiliaryData &auxiliaryData = {});
    QImage smallImage(Utils::PathString filePath,
                      Utils::SmallString extraId = {},
                      const ImageCache::AuxiliaryData &auxiliaryData = {});
    QIcon icon(Utils::PathString filePath,
               Utils::SmallString extraId = {},
               const ImageCache::AuxiliaryData &auxiliaryData = {});

private:
    ImageCacheStorageInterface &m_storage;
    TimeStampProviderInterface &m_timeStampProvider;
    ImageCacheCollectorInterface &m_collector;
};

} // namespace QmlDesigner
