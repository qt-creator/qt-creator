// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <imagecachecollectorinterface.h>

class ImageCacheCollectorMock : public QmlDesigner::ImageCacheCollectorInterface
{
public:
    MOCK_METHOD(void,
                start,
                (Utils::SmallStringView filePath,
                 Utils::SmallStringView state,
                 const QmlDesigner::ImageCache::AuxiliaryData &auxiliaryData,
                 ImageCacheCollectorInterface::CaptureCallback captureCallback,
                 ImageCacheCollectorInterface::AbortCallback abortCallback),
                (override));

    MOCK_METHOD(ImageTuple,
                createImage,
                (Utils::SmallStringView filePath,
                 Utils::SmallStringView state,
                 const QmlDesigner::ImageCache::AuxiliaryData &auxiliaryData),
                (override));

    MOCK_METHOD(QIcon,
                createIcon,
                (Utils::SmallStringView filePath,
                 Utils::SmallStringView state,
                 const QmlDesigner::ImageCache::AuxiliaryData &auxiliaryData),
                (override));
};
