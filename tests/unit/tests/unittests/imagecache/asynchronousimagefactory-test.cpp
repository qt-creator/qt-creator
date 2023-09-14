// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"
#include "../utils/notification.h"

#include "../mocks/imagecachecollectormock.h"
#include "../mocks/mockimagecachegenerator.h"
#include "../mocks/mockimagecachestorage.h"
#include "../mocks/mocktimestampprovider.h"

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
    QImage midSizeImage1{5, 5, QImage::Format_ARGB32};
    QImage smallImage1{1, 1, QImage::Format_ARGB32};
};

TEST_F(AsynchronousImageFactory, request_image_request_image_from_collector)
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

TEST_F(AsynchronousImageFactory, request_image_with_extra_id_request_image_from_collector)
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

TEST_F(AsynchronousImageFactory, request_image_with_auxiliary_data_request_image_from_collector)
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

TEST_F(AsynchronousImageFactory, dont_request_image_request_image_from_collector_if_file_was_updated_recently)
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

TEST_F(AsynchronousImageFactory, request_image_request_image_from_collector_if_file_was_not_updated_recently)
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

TEST_F(AsynchronousImageFactory, clean_removes_entries)
{
    EXPECT_CALL(collectorMock, start(Eq("/path/to/Component1.qml"), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) {
            notification.notify();
            waitInThread.wait();
        });
    factory.generate("/path/to/Component1.qml");
    notification.wait();

    EXPECT_CALL(collectorMock, start(Eq("/path/to/Component3.qml"), _, _, _, _)).Times(0);

    factory.generate("/path/to/Component3.qml");
    factory.clean();
    waitInThread.notify();
}

TEST_F(AsynchronousImageFactory, after_clean_new_jobs_works)
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

TEST_F(AsynchronousImageFactory, capture_image_callback_stores_image)
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
        .WillByDefault([&](auto, auto, auto, auto capture, auto) {
            capture(image1, midSizeImage1, smallImage1);
        });

    EXPECT_CALL(storageMock,
                storeImage(Eq("/path/to/Component.qml+id"),
                           Sqlite::TimeStamp{125},
                           Eq(image1),
                           Eq(midSizeImage1),
                           Eq(smallImage1)))
        .WillOnce([&](auto, auto, auto, auto, auto) { notification.notify(); });

    factory.generate("/path/to/Component.qml", "id");
    notification.wait();
}

} // namespace
