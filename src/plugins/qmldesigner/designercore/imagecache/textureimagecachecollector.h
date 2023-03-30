// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "imagecachecollectorinterface.h"

namespace QmlDesigner {

class TextureImageCacheCollector final : public ImageCacheCollectorInterface
{
public:
    TextureImageCacheCollector();
    ~TextureImageCacheCollector();

    void start(Utils::SmallStringView filePath,
               Utils::SmallStringView state,
               const ImageCache::AuxiliaryData &auxiliaryData,
               CaptureCallback captureCallback,
               AbortCallback abortCallback) override;

    ImageTuple createImage(Utils::SmallStringView filePath,
                           Utils::SmallStringView state,
                           const ImageCache::AuxiliaryData &auxiliaryData) override;

    QIcon createIcon(Utils::SmallStringView filePath,
                     Utils::SmallStringView state,
                     const ImageCache::AuxiliaryData &auxiliaryData) override;
};

} // namespace QmlDesigner
