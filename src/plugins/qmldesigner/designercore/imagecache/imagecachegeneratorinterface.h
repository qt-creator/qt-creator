// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <imagecacheauxiliarydata.h>
#include <sqlitetimestamp.h>
#include <utils/smallstringview.h>

#include <QImage>

namespace QmlDesigner {

class ImageCacheGeneratorInterface
{
public:
    virtual void generateImage(Utils::SmallStringView name,
                               Utils::SmallStringView extraId,
                               Sqlite::TimeStamp timeStamp,
                               ImageCache::CaptureImageWithScaledImagesCallback &&captureCallback,
                               ImageCache::AbortCallback &&abortCallback,
                               ImageCache::AuxiliaryData &&auxiliaryData)
        = 0;

    virtual void clean() = 0;
    virtual void waitForFinished() = 0;

protected:
    ~ImageCacheGeneratorInterface() = default;
};

} // namespace QmlDesigner
