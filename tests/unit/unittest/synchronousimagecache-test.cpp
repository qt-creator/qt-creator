// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include "imagecachecollectormock.h"
#include "mockimagecachestorage.h"
#include "mocktimestampprovider.h"

#include <synchronousimagecache.h>

namespace {

MATCHER_P(IsIcon, icon, std::string(negation ? "isn't " : "is ") + PrintToString(icon))
{
    const QIcon &other = arg;
    return icon.availableSizes() == other.availableSizes();
}

class SynchronousImageCache : public testing::Test
{
protected:
    SynchronousImageCache()
    {
        ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
            .WillByDefault(Return(Sqlite::TimeStamp{123}));
        ON_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{123})))
            .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{image1}));
        ON_CALL(mockStorage,
                fetchImage(Eq("/path/to/Component.qml+extraId1"), Eq(Sqlite::TimeStamp{123})))
            .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{image2}));
        ON_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{123})))
            .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{smallImage1}));
        ON_CALL(mockStorage,
                fetchSmallImage(Eq("/path/to/Component.qml+extraId1"), Eq(Sqlite::TimeStamp{123})))
            .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{smallImage2}));
        ON_CALL(mockStorage, fetchIcon(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{123})))
            .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::IconEntry{icon1}));
        ON_CALL(mockStorage,
                fetchIcon(Eq("/path/to/Component.qml+extraId1"), Eq(Sqlite::TimeStamp{123})))
            .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::IconEntry{icon2}));
        ON_CALL(mockCollector, createImage(Eq("/path/to/Component.qml"), Eq("extraId1"), _))
            .WillByDefault(Return(std::make_pair(image3, smallImage3)));
        ON_CALL(mockCollector, createIcon(Eq("/path/to/Component.qml"), Eq("extraId1"), _))
            .WillByDefault(Return(icon3));
    }

protected:
    NiceMock<MockFunction<void(const QImage &image)>> mockCaptureCallback;
    NiceMock<MockImageCacheStorage> mockStorage;
    NiceMock<ImageCacheCollectorMock> mockCollector;
    NiceMock<MockTimeStampProvider> mockTimeStampProvider;
    QmlDesigner::SynchronousImageCache cache{mockStorage, mockTimeStampProvider, mockCollector};
    QImage image1{1, 1, QImage::Format_ARGB32};
    QImage image2{2, 2, QImage::Format_ARGB32};
    QImage image3{3, 3, QImage::Format_ARGB32};
    QImage smallImage1{1, 1, QImage::Format_ARGB32};
    QImage smallImage2{2, 1, QImage::Format_ARGB32};
    QImage smallImage3{3, 1, QImage::Format_ARGB32};
    QIcon icon1{QPixmap::fromImage(image1)};
    QIcon icon2{QPixmap::fromImage(image2)};
    QIcon icon3{QPixmap::fromImage(image3)};
};

TEST_F(SynchronousImageCache, GetImageFromStorage)
{
    auto image = cache.image("/path/to/Component.qml");

    ASSERT_THAT(image, image1);
}

TEST_F(SynchronousImageCache, GetImageWithExtraIdFromStorage)
{
    auto image = cache.image("/path/to/Component.qml", "extraId1");

    ASSERT_THAT(image, image2);
}

TEST_F(SynchronousImageCache, GetImageWithOutdatedTimeStampFromCollector)
{
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{124}));

    auto image = cache.image("/path/to/Component.qml", "extraId1");

    ASSERT_THAT(image, image3);
}

TEST_F(SynchronousImageCache, GetImageWithOutdatedTimeStampStored)
{
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{124}));

    EXPECT_CALL(mockStorage,
                storeImage(Eq("/path/to/Component.qml+extraId1"),
                           Eq(Sqlite::TimeStamp{124}),
                           Eq(image3),
                           Eq(smallImage3)));

    auto image = cache.image("/path/to/Component.qml", "extraId1");
}

TEST_F(SynchronousImageCache, GetSmallImageFromStorage)
{
    auto image = cache.smallImage("/path/to/Component.qml");

    ASSERT_THAT(image, smallImage1);
}

TEST_F(SynchronousImageCache, GetSmallImageWithExtraIdFromStorage)
{
    auto image = cache.smallImage("/path/to/Component.qml", "extraId1");

    ASSERT_THAT(image, smallImage2);
}

TEST_F(SynchronousImageCache, GetSmallImageWithOutdatedTimeStampFromCollector)
{
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{124}));

    auto image = cache.smallImage("/path/to/Component.qml", "extraId1");

    ASSERT_THAT(image, smallImage3);
}

TEST_F(SynchronousImageCache, GetSmallImageWithOutdatedTimeStampStored)
{
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{124}));

    EXPECT_CALL(mockStorage,
                storeImage(Eq("/path/to/Component.qml+extraId1"),
                           Eq(Sqlite::TimeStamp{124}),
                           Eq(image3),
                           Eq(smallImage3)));

    auto image = cache.smallImage("/path/to/Component.qml", "extraId1");
}

TEST_F(SynchronousImageCache, GetIconFromStorage)
{
    auto icon = cache.icon("/path/to/Component.qml");

    ASSERT_THAT(icon, IsIcon(icon1));
}

TEST_F(SynchronousImageCache, GetIconWithExtraIdFromStorage)
{
    auto icon = cache.icon("/path/to/Component.qml", "extraId1");

    ASSERT_THAT(icon, IsIcon(icon2));
}

TEST_F(SynchronousImageCache, GetIconWithOutdatedTimeStampFromCollector)
{
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{124}));

    auto icon = cache.icon("/path/to/Component.qml", "extraId1");

    ASSERT_THAT(icon, IsIcon(icon3));
}

TEST_F(SynchronousImageCache, GetIconWithOutdatedTimeStampStored)
{
    using QmlDesigner::ImageCache::FontCollectorSizesAuxiliaryData;
    std::vector<QSize> sizes{{20, 11}};
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{124}));

    EXPECT_CALL(mockStorage,
                storeIcon(Eq("/path/to/Component.qml+extraId1"),
                          Eq(Sqlite::TimeStamp{124}),
                          IsIcon(icon3)));

    auto icon = cache.icon("/path/to/Component.qml",
                           "extraId1",
                           FontCollectorSizesAuxiliaryData{sizes, "color", "text"});
}

TEST_F(SynchronousImageCache, IconCallsCollectorWithAuxiliaryData)
{
    using QmlDesigner::ImageCache::FontCollectorSizesAuxiliaryData;
    std::vector<QSize> sizes{{20, 11}};
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{124}));

    EXPECT_CALL(mockCollector,
                createIcon(Eq("/path/to/Component.qml"),
                           Eq("extraId1"),
                           VariantWith<FontCollectorSizesAuxiliaryData>(
                               AllOf(Field(&FontCollectorSizesAuxiliaryData::sizes,
                                           ElementsAre(QSize{20, 11})),
                                     Field(&FontCollectorSizesAuxiliaryData::colorName,
                                           Eq(u"color"))))));

    auto icon = cache.icon("/path/to/Component.qml",
                           "extraId1",
                           FontCollectorSizesAuxiliaryData{sizes, "color", "text"});
}

TEST_F(SynchronousImageCache, ImageCallsCollectorWithAuxiliaryData)
{
    using QmlDesigner::ImageCache::FontCollectorSizesAuxiliaryData;
    std::vector<QSize> sizes{{20, 11}};
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{124}));

    EXPECT_CALL(mockCollector,
                createImage(Eq("/path/to/Component.qml"),
                            Eq("extraId1"),
                            VariantWith<FontCollectorSizesAuxiliaryData>(
                                AllOf(Field(&FontCollectorSizesAuxiliaryData::sizes,
                                            ElementsAre(QSize{20, 11})),
                                      Field(&FontCollectorSizesAuxiliaryData::colorName,
                                            Eq(u"color"))))));

    auto icon = cache.image("/path/to/Component.qml",
                            "extraId1",
                            FontCollectorSizesAuxiliaryData{sizes, "color", "text"});
}

TEST_F(SynchronousImageCache, SmallImageCallsCollectorWithAuxiliaryData)
{
    using QmlDesigner::ImageCache::FontCollectorSizesAuxiliaryData;
    std::vector<QSize> sizes{{20, 11}};
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{124}));

    EXPECT_CALL(mockCollector,
                createImage(Eq("/path/to/Component.qml"),
                            Eq("extraId1"),
                            VariantWith<FontCollectorSizesAuxiliaryData>(
                                AllOf(Field(&FontCollectorSizesAuxiliaryData::sizes,
                                            ElementsAre(QSize{20, 11})),
                                      Field(&FontCollectorSizesAuxiliaryData::colorName,
                                            Eq(u"color"))))));

    auto icon = cache.smallImage("/path/to/Component.qml",
                                 "extraId1",
                                 FontCollectorSizesAuxiliaryData{sizes, "color", "text"});
}

} // namespace
