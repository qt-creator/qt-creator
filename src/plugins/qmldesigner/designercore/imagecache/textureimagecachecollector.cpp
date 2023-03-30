// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "textureimagecachecollector.h"

#include <utils/asset.h>
#include <utils/hdrimage.h>
#include <utils/smallstring.h>
#include <utils/stylehelper.h>

namespace QmlDesigner {

TextureImageCacheCollector::TextureImageCacheCollector() = default;
TextureImageCacheCollector::~TextureImageCacheCollector() = default;

void TextureImageCacheCollector::start(Utils::SmallStringView name,
                                       Utils::SmallStringView,
                                       const ImageCache::AuxiliaryData &,
                                       CaptureCallback captureCallback,
                                       AbortCallback abortCallback)
{
    Asset asset {QString(name)};
    QImage image;

    if (asset.isImage()) {
        image = QImage(Utils::StyleHelper::dpiSpecificImageFile(asset.id()));
    } else if (asset.isHdrFile()) {
        HdrImage hdr{asset.id()};
        if (!hdr.image().isNull())
            image = hdr.image().copy(); // Copy to ensure image data survives HdrImage destruction
    }

    if (image.isNull())
        abortCallback(ImageCache::AbortReason::Failed);
    else
        image = image.scaled(QSize{300, 300}, Qt::KeepAspectRatio);

    captureCallback({}, image, {});
}

ImageCacheCollectorInterface::ImageTuple TextureImageCacheCollector::createImage(
        Utils::SmallStringView, Utils::SmallStringView, const ImageCache::AuxiliaryData &)
{
    return {};
}

QIcon TextureImageCacheCollector::createIcon(Utils::SmallStringView,
                                             Utils::SmallStringView,
                                             const ImageCache::AuxiliaryData &)
{
    return {};
}

} // namespace QmlDesigner
