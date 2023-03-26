// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imagecachecollectorinterface.h"

namespace QmlDesigner {

class ImageCacheFontCollector final : public ImageCacheCollectorInterface
{
public:
    ImageCacheFontCollector();

    ~ImageCacheFontCollector();

    void start(Utils::SmallStringView filePath,
               Utils::SmallStringView extraId,
               const ImageCache::AuxiliaryData &auxiliaryData,
               CaptureCallback captureCallback,
               AbortCallback abortCallback) override;

    ImageTuple createImage(Utils::SmallStringView filePath,
                           Utils::SmallStringView extraId,
                           const ImageCache::AuxiliaryData &auxiliaryData) override;

    QIcon createIcon(Utils::SmallStringView filePath,
                     Utils::SmallStringView extraId,
                     const ImageCache::AuxiliaryData &auxiliaryData) override;
};

} // namespace QmlDesigner
