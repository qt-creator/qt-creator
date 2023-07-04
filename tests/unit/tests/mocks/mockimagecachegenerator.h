// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <imagecachegeneratorinterface.h>

class MockImageCacheGenerator : public QmlDesigner::ImageCacheGeneratorInterface
{
public:
    MOCK_METHOD(void,
                generateImage,
                (Utils::SmallStringView name,
                 Utils::SmallStringView state,
                 Sqlite::TimeStamp timeStamp,
                 QmlDesigner::ImageCache::CaptureImageWithScaledImagesCallback &&captureCallback,
                 QmlDesigner::ImageCache::AbortCallback &&abortCallback,
                 QmlDesigner::ImageCache::AuxiliaryData &&auxiliaryData),
                (override));
    MOCK_METHOD(void, clean, (), (override));
    MOCK_METHOD(void, waitForFinished, (), (override));
};
