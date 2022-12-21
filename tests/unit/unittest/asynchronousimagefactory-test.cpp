// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include "imagecachecollectormock.h"
#include "mockimagecachegenerator.h"
#include "mockimagecachestorage.h"
#include "mocktimestampprovider.h"
#include "notification.h"

#include <asynchronousimagefactory.h>

namespace {

using QmlDesigner::ImageCache::FontCollectorSizesAuxiliaryData;

class AsynchronousImageFactory : public testing::Test
{
protected:
    AsynchronousImageFactory()
    {
        ON_CALL(timeStampProviderMock, timeStamp(Eq("/path/to/Component.qml")))
            .WillByDefault(Return(Sqlite::TimeStamp{123}));
    }

protected:
    Notification notification;
    Notification waitInThread;
    NiceMock<MockImageCacheStorage> storageMock;
    NiceMock<ImageCacheCollectorMock> collectorMock;
    NiceMock<MockTimeStampProvider> timeStampProviderMock;
    QmlDesigner::AsynchronousImageFactory factory{storageMock, timeStampProviderMock, collectorMock};
    QImage image1{10, 10, QImage::Format_ARGB32};
    QImage smallImage1{1, 1, QImage::Format_ARGB32};
};

TEST_F(AsynchronousImageFactory, RequestImageRequestImageFromCollector)
{
    EXPECT_CALL(collectorMock,
                start(Eq("/path/to/Component.qml"),
                      IsEmpty(),
                      VariantWith<std::monostate>(std::monostate{}),
                      _,
                      _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    factory.generate("/path/to/Component.qml");
    notification.wait();
}

TEST_F(AsynchronousImageFactory, RequestImageWithExtraIdRequestImageFromCollector)
{
    EXPECT_CALL(collectorMock,
                start(Eq("/path/to/Component.qml"),
                      Eq("foo"),
                      VariantWith<std::monostate>(std::monostate{}),
                      _,
                      _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    factory.generate("/path/to/Component.qml", "foo");
    notification.wait();
}

TEST_F(AsynchronousImageFactory, RequestImageWithAuxiliaryDataRequestImageFromCollector)
{
    std::vector<QSize> sizes{{20, 11}};

    EXPECT_CALL(collectorMock,
                start(Eq("/path/to/Component.qml"),
                      Eq("foo"),
                      VariantWith<FontCollectorSizesAuxiliaryData>(
                          AllOf(Field(&FontCollectorSizesAuxiliaryData::sizes,
                                      ElementsAre(QSize{20, 11})),
                                Field(&FontCollectorSizesAuxiliaryData::colorName, Eq(u"color")),
                                Field(&FontCollectorSizesAuxiliaryData::text, Eq(u"some text")))),
                      _,
                      _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    factory.generate("/path/to/Component.qml",
                     "foo",
                     QmlDesigner::ImageCache::FontCollectorSizesAuxiliaryData{sizes,
                                                                              "color",
                                                                              "some text"});
    notification.wait();
}

TEST_F(AsynchronousImageFactory, DontRequestImageRequestImageFromCollectorIfFileWasUpdatedRecently)
{
    ON_CALL(storageMock, fetchModifiedImageTime(Eq("/path/to/Component.qml"))).WillByDefault([&](auto) {
        notification.notify();
        return Sqlite::TimeStamp{124};
    });
    ON_CALL(timeStampProviderMock, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{125}));
    ON_CALL(timeStampProviderMock, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{1}));

    EXPECT_CALL(collectorMock, start(_, _, _, _, _)).Times(0);

    factory.generate("/path/to/Component.qml");
    notification.wait();
}

TEST_F(AsynchronousImageFactory, RequestImageRequestImageFromCollectorIfFileWasNotUpdatedRecently)
{
    ON_CALL(storageMock, fetchModifiedImageTime(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{123}));
    ON_CALL(timeStampProviderMock, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{125}));
    ON_CALL(timeStampProviderMock, pause()).WillByDefault(Return(Sqlite::TimeStamp{1}));

    EXPECT_CALL(collectorMock, start(_, _, _, _, _)).WillOnce([&](auto, auto, auto, auto, auto) {
        notification.notify();
    });

    factory.generate("/path/to/Component.qml");
    notification.wait();
}

TEST_F(AsynchronousImageFactory, CleanRemovesEntries)
{
    EXPECT_CALL(collectorMock, start(Eq("/path/to/Component1.qml"), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { waitInThread.wait(); });
    factory.generate("/path/to/Component1.qml");

    EXPECT_CALL(collectorMock, start(Eq("/path/to/Component3.qml"), _, _, _, _)).Times(0);

    factory.generate("/path/to/Component3.qml");
    factory.clean();
    waitInThread.notify();
}

TEST_F(AsynchronousImageFactory, AfterCleanNewJobsWorks)
{
    factory.clean();

    EXPECT_CALL(collectorMock,
                start(Eq("/path/to/Component.qml"),
                      IsEmpty(),
                      VariantWith<std::monostate>(std::monostate{}),
                      _,
                      _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    factory.generate("/path/to/Component.qml");
    notification.wait();
}

TEST_F(AsynchronousImageFactory, CaptureImageCallbackStoresImage)
{
    ON_CALL(storageMock, fetchModifiedImageTime(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{123}));
    ON_CALL(timeStampProviderMock, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{125}));
    ON_CALL(timeStampProviderMock, pause()).WillByDefault(Return(Sqlite::TimeStamp{1}));
    ON_CALL(collectorMock,
            start(Eq("/path/to/Component.qml"),
                  Eq("id"),
                  VariantWith<std::monostate>(std::monostate{}),
                  _,
                  _))
        .WillByDefault([&](auto, auto, auto, auto capture, auto) { capture(image1, smallImage1); });

    EXPECT_CALL(storageMock,
                storeImage(Eq("/path/to/Component.qml+id"),
                           Sqlite::TimeStamp{125},
                           Eq(image1),
                           Eq(smallImage1)))
        .WillOnce([&](auto, auto, auto, auto) { notification.notify(); });

    factory.generate("/path/to/Component.qml", "id");
    notification.wait();
}

} // namespace
