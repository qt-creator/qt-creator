// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <imagecacheauxiliarydata.h>
#include <utils/smallstringview.h>

#include <QIcon>
#include <QImage>

namespace QmlDesigner {

class ImageCacheCollectorInterface
{
public:
    using CaptureCallback = ImageCache::CaptureImageWithSmallImageCallback;
    using AbortCallback = ImageCache::AbortCallback;
    using ImagePair = std::pair<QImage, QImage>;

    virtual void start(Utils::SmallStringView filePath,
                       Utils::SmallStringView extraId,
                       const ImageCache::AuxiliaryData &auxiliaryData,
                       CaptureCallback captureCallback,
                       AbortCallback abortCallback)
        = 0;

    virtual ImagePair createImage(Utils::SmallStringView filePath,
                                  Utils::SmallStringView extraId,
                                  const ImageCache::AuxiliaryData &auxiliaryData)
        = 0;
    virtual QIcon createIcon(Utils::SmallStringView filePath,
                             Utils::SmallStringView extraId,
                             const ImageCache::AuxiliaryData &auxiliaryData)
        = 0;

protected:
    ~ImageCacheCollectorInterface() = default;
};
} // namespace QmlDesigner
