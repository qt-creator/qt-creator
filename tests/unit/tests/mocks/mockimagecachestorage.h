// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../utils/googletest.h"

#include <imagecachestorageinterface.h>

class MockImageCacheStorage : public QmlDesigner::ImageCacheStorageInterface
{
public:
    MOCK_METHOD(QmlDesigner::ImageCacheStorageInterface::ImageEntry,
                fetchImage,
                (Utils::SmallStringView name, Sqlite::TimeStamp minimumTimeStamp),
                (const, override));

    MOCK_METHOD(QmlDesigner::ImageCacheStorageInterface::ImageEntry,
                fetchMidSizeImage,
                (Utils::SmallStringView name, Sqlite::TimeStamp minimumTimeStamp),
                (const, override));

    MOCK_METHOD(QmlDesigner::ImageCacheStorageInterface::ImageEntry,
                fetchSmallImage,
                (Utils::SmallStringView name, Sqlite::TimeStamp minimumTimeStamp),
                (const, override));

    MOCK_METHOD(QmlDesigner::ImageCacheStorageInterface::IconEntry,
                fetchIcon,
                (Utils::SmallStringView name, Sqlite::TimeStamp minimumTimeStamp),
                (const, override));

    MOCK_METHOD(void,
                storeImage,
                (Utils::SmallStringView name,
                 Sqlite::TimeStamp newTimeStamp,
                 const QImage &image,
                 const QImage &midSizeImage,
                 const QImage &smallImage),
                (override));

    MOCK_METHOD(void,
                storeIcon,
                (Utils::SmallStringView name, Sqlite::TimeStamp newTimeStamp, const QIcon &icon),
                (override));

    MOCK_METHOD(void, walCheckpointFull, (), (override));
    MOCK_METHOD(Sqlite::TimeStamp,
                fetchModifiedImageTime,
                (Utils::SmallStringView name),
                (const, override));
    MOCK_METHOD(bool, fetchHasImage, (Utils::SmallStringView name), (const, override));
};
