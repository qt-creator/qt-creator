// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"
#include "../utils/notification.h"

#include "../mocks/mockimagecachegenerator.h"
#include "../mocks/mockimagecachestorage.h"
#include "../mocks/mocktimestampprovider.h"

#include <asynchronousimagecache.h>

namespace {

class AsynchronousImageCache : public testing::Test
{
protected:
    Notification notification;
    Notification waitInThread;
    NiceMock<MockFunction<void(QmlDesigner::ImageCache::AbortReason)>> mockAbortCallback;
    NiceMock<MockFunction<void(const QImage &image)>> mockCaptureCallback;
    NiceMock<MockImageCacheStorage> mockStorage;
    NiceMock<MockImageCacheGenerator> mockGenerator;
    NiceMock<MockTimeStampProvider> mockTimeStampProvider;
    QmlDesigner::AsynchronousImageCache cache{mockStorage, mockGenerator, mockTimeStampProvider};
    QImage image1{10, 10, QImage::Format_ARGB32};
    QImage midSizeImage1{5, 5, QImage::Format_ARGB32};
    QImage smallImage1{1, 1, QImage::Format_ARGB32};
};

TEST_F(AsynchronousImageCache, request_image_fetches_image_from_storage)
{
    EXPECT_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), _))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{};
        });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_image_fetches_image_from_storage_with_time_stamp)
{
    EXPECT_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillRepeatedly(Return(Sqlite::TimeStamp{123}));
    EXPECT_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{123})))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{};
        });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_image_calls_capture_callback_with_image_from_storage)
{
    ON_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{image1}));

    EXPECT_CALL(mockCaptureCallback, Call(Eq(image1))).WillRepeatedly([&](const QImage &) {
        notification.notify();
    });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_image_calls_abort_callback_without_image)
{
    ON_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{QImage{}}));

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)))
        .WillRepeatedly([&](auto) { notification.notify(); });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_image_request_image_from_generator)
{
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{123}));

    EXPECT_CALL(mockGenerator,
                generateImage(Eq("/path/to/Component.qml"), _, Eq(Sqlite::TimeStamp{123}), _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto, auto) { notification.notify(); });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_image_calls_capture_callback_with_image_from_generator)
{
    ON_CALL(mockGenerator, generateImage(Eq("/path/to/Component.qml"), _, _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto &&callback, auto, auto) {
            callback(QImage{image1}, QImage{midSizeImage1}, QImage{smallImage1});
            notification.notify();
        });

    EXPECT_CALL(mockCaptureCallback, Call(Eq(image1)));

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_image_calls_abort_callback_from_generator)
{
    ON_CALL(mockGenerator, generateImage(Eq("/path/to/Component.qml"), _, _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto &&, auto &&abortCallback, auto) {
            abortCallback(QmlDesigner::ImageCache::AbortReason::Failed);
            notification.notify();
        });

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)));

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_mid_size_image_fetches_mid_size_image_from_storage)
{
    EXPECT_CALL(mockStorage, fetchMidSizeImage(Eq("/path/to/Component.qml"), _))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{};
        });

    cache.requestMidSizeImage("/path/to/Component.qml",
                              mockCaptureCallback.AsStdFunction(),
                              mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_mid_size_image_fetches_mid_size_image_from_storage_with_time_stamp)
{
    EXPECT_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillRepeatedly(Return(Sqlite::TimeStamp{123}));
    EXPECT_CALL(mockStorage,
                fetchMidSizeImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{123})))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{};
        });

    cache.requestMidSizeImage("/path/to/Component.qml",
                              mockCaptureCallback.AsStdFunction(),
                              mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_mid_size_image_calls_capture_callback_with_image_from_storage)
{
    ON_CALL(mockStorage, fetchMidSizeImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{smallImage1}));

    EXPECT_CALL(mockCaptureCallback, Call(Eq(smallImage1))).WillRepeatedly([&](const QImage &) {
        notification.notify();
    });

    cache.requestMidSizeImage("/path/to/Component.qml",
                              mockCaptureCallback.AsStdFunction(),
                              mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_mid_size_image_calls_abort_callback_without_mid_size_image)
{
    ON_CALL(mockStorage, fetchMidSizeImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{QImage{}}));

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)))
        .WillRepeatedly([&](auto) { notification.notify(); });

    cache.requestMidSizeImage("/path/to/Component.qml",
                              mockCaptureCallback.AsStdFunction(),
                              mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_mid_size_image_request_image_from_generator)
{
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{123}));

    EXPECT_CALL(mockGenerator,
                generateImage(Eq("/path/to/Component.qml"), _, Eq(Sqlite::TimeStamp{123}), _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto, auto) { notification.notify(); });

    cache.requestMidSizeImage("/path/to/Component.qml",
                              mockCaptureCallback.AsStdFunction(),
                              mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_mid_size_image_calls_capture_callback_with_image_from_generator)
{
    ON_CALL(mockGenerator, generateImage(Eq("/path/to/Component.qml"), _, _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto &&callback, auto, auto) {
            callback(QImage{image1}, QImage{midSizeImage1}, QImage{smallImage1});
            notification.notify();
        });

    EXPECT_CALL(mockCaptureCallback, Call(Eq(midSizeImage1)));

    cache.requestMidSizeImage("/path/to/Component.qml",
                              mockCaptureCallback.AsStdFunction(),
                              mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_mid_size_image_calls_abort_callback_from_generator)
{
    ON_CALL(mockGenerator, generateImage(Eq("/path/to/Component.qml"), _, _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto &&, auto &&abortCallback, auto) {
            abortCallback(QmlDesigner::ImageCache::AbortReason::Failed);
            notification.notify();
        });

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)));

    cache.requestMidSizeImage("/path/to/Component.qml",
                              mockCaptureCallback.AsStdFunction(),
                              mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_small_image_fetches_small_image_from_storage)
{
    EXPECT_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml"), _))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{};
        });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_small_image_fetches_small_image_from_storage_with_time_stamp)
{
    EXPECT_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillRepeatedly(Return(Sqlite::TimeStamp{123}));
    EXPECT_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{123})))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{};
        });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_small_image_calls_capture_callback_with_image_from_storage)
{
    ON_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{smallImage1}));

    EXPECT_CALL(mockCaptureCallback, Call(Eq(smallImage1))).WillRepeatedly([&](const QImage &) {
        notification.notify();
    });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_small_image_calls_abort_callback_without_small_image)
{
    ON_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{QImage{}}));

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)))
        .WillRepeatedly([&](auto) { notification.notify(); });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_small_image_request_image_from_generator)
{
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{123}));

    EXPECT_CALL(mockGenerator,
                generateImage(Eq("/path/to/Component.qml"), _, Eq(Sqlite::TimeStamp{123}), _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto, auto) { notification.notify(); });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_small_image_calls_capture_callback_with_image_from_generator)
{
    ON_CALL(mockGenerator, generateImage(Eq("/path/to/Component.qml"), _, _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto &&callback, auto, auto) {
            callback(QImage{image1}, QImage{midSizeImage1}, QImage{smallImage1});
            notification.notify();
        });

    EXPECT_CALL(mockCaptureCallback, Call(Eq(smallImage1)));

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_small_image_calls_abort_callback_from_generator)
{
    ON_CALL(mockGenerator, generateImage(Eq("/path/to/Component.qml"), _, _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto &&, auto &&abortCallback, auto) {
            abortCallback(QmlDesigner::ImageCache::AbortReason::Failed);
            notification.notify();
        });

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)));

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, clean_removes_entries)
{
    EXPECT_CALL(mockGenerator, generateImage(_, _, _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto &&captureCallback, auto &&, auto) {
            captureCallback(QImage{}, QImage{}, QImage{});
            waitInThread.wait();
        });
    cache.requestSmallImage("/path/to/Component1.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());

    EXPECT_CALL(mockCaptureCallback, Call(_)).Times(AtMost(1));

    cache.requestSmallImage("/path/to/Component3.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    cache.clean();
    waitInThread.notify();
}

TEST_F(AsynchronousImageCache, clean_calls_abort)
{
    ON_CALL(mockGenerator, generateImage(_, _, _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto &&, auto) { waitInThread.wait(); });
    cache.requestSmallImage("/path/to/Component1.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    cache.requestSmallImage("/path/to/Component2.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::Abort)))
        .Times(AtLeast(2));

    cache.requestSmallImage("/path/to/Component3.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    cache.clean();
    waitInThread.notify();
}

TEST_F(AsynchronousImageCache, clean_calls_generator_clean)
{
    EXPECT_CALL(mockGenerator, clean()).Times(AtLeast(1));

    cache.clean();
}

TEST_F(AsynchronousImageCache, after_clean_new_jobs_works)
{
    cache.clean();

    EXPECT_CALL(mockGenerator, generateImage(Eq("/path/to/Component.qml"), _, _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto &&, auto &&, auto) { notification.notify(); });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_image_with_extra_id_fetches_image_from_storage)
{
    EXPECT_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml+extraId1"), _))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{};
        });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction(),
                       "extraId1");
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_mid_size_image_with_extra_id_fetches_image_from_storage)
{
    EXPECT_CALL(mockStorage, fetchMidSizeImage(Eq("/path/to/Component.qml+extraId1"), _))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{};
        });

    cache.requestMidSizeImage("/path/to/Component.qml",
                              mockCaptureCallback.AsStdFunction(),
                              mockAbortCallback.AsStdFunction(),
                              "extraId1");
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_small_image_with_extra_id_fetches_image_from_storage)
{
    EXPECT_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml+extraId1"), _))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{};
        });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction(),
                            "extraId1");
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_image_with_extra_id_request_image_from_generator)
{
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{123}));

    EXPECT_CALL(mockGenerator,
                generateImage(
                    Eq("/path/to/Component.qml"), Eq("extraId1"), Eq(Sqlite::TimeStamp{123}), _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto &&, auto, auto) { notification.notify(); });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction(),
                       "extraId1");
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_mid_size_image_with_extra_id_request_image_from_generator)
{
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{123}));

    EXPECT_CALL(mockGenerator,
                generateImage(
                    Eq("/path/to/Component.qml"), Eq("extraId1"), Eq(Sqlite::TimeStamp{123}), _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto &&, auto, auto) { notification.notify(); });

    cache.requestMidSizeImage("/path/to/Component.qml",
                              mockCaptureCallback.AsStdFunction(),
                              mockAbortCallback.AsStdFunction(),
                              "extraId1");
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_small_image_with_extra_id_request_image_from_generator)
{
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{123}));

    EXPECT_CALL(mockGenerator,
                generateImage(
                    Eq("/path/to/Component.qml"), Eq("extraId1"), Eq(Sqlite::TimeStamp{123}), _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto &&, auto, auto) { notification.notify(); });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction(),
                            "extraId1");
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_image_with_auxiliary_data_request_image_from_generator)
{
    using QmlDesigner::ImageCache::FontCollectorSizesAuxiliaryData;
    std::vector<QSize> sizes{{20, 11}};
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{123}));

    EXPECT_CALL(mockGenerator,
                generateImage(Eq("/path/to/Component.qml"),
                              Eq("extraId1"),
                              Eq(Sqlite::TimeStamp{123}),
                              _,
                              _,
                              VariantWith<FontCollectorSizesAuxiliaryData>(
                                  AllOf(Field(&FontCollectorSizesAuxiliaryData::sizes,
                                              ElementsAre(QSize{20, 11})),
                                        Field(&FontCollectorSizesAuxiliaryData::colorName,
                                              Eq(u"color"))))))
        .WillRepeatedly([&](auto, auto, auto, auto &&, auto, auto) { notification.notify(); });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction(),
                       "extraId1",
                       FontCollectorSizesAuxiliaryData{sizes, "color", "text"});
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_mid_size_image_with_auxiliary_data_request_image_from_generator)
{
    using QmlDesigner::ImageCache::FontCollectorSizesAuxiliaryData;
    std::vector<QSize> sizes{{20, 11}};
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{123}));

    EXPECT_CALL(mockGenerator,
                generateImage(Eq("/path/to/Component.qml"),
                              Eq("extraId1"),
                              Eq(Sqlite::TimeStamp{123}),
                              _,
                              _,
                              VariantWith<FontCollectorSizesAuxiliaryData>(
                                  AllOf(Field(&FontCollectorSizesAuxiliaryData::sizes,
                                              ElementsAre(QSize{20, 11})),
                                        Field(&FontCollectorSizesAuxiliaryData::colorName,
                                              Eq(u"color"))))))
        .WillRepeatedly([&](auto, auto, auto, auto &&, auto, auto) { notification.notify(); });

    cache.requestMidSizeImage("/path/to/Component.qml",
                              mockCaptureCallback.AsStdFunction(),
                              mockAbortCallback.AsStdFunction(),
                              "extraId1",
                              FontCollectorSizesAuxiliaryData{sizes, "color", "text"});
    notification.wait();
}

TEST_F(AsynchronousImageCache, request_small_image_with_auxiliary_data_request_image_from_generator)
{
    using QmlDesigner::ImageCache::FontCollectorSizesAuxiliaryData;
    std::vector<QSize> sizes{{20, 11}};
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{123}));

    EXPECT_CALL(mockGenerator,
                generateImage(Eq("/path/to/Component.qml"),
                              Eq("extraId1"),
                              Eq(Sqlite::TimeStamp{123}),
                              _,
                              _,
                              VariantWith<FontCollectorSizesAuxiliaryData>(
                                  AllOf(Field(&FontCollectorSizesAuxiliaryData::sizes,
                                              ElementsAre(QSize{20, 11})),
                                        Field(&FontCollectorSizesAuxiliaryData::colorName,
                                              Eq(u"color"))))))
        .WillRepeatedly([&](auto, auto, auto, auto &&, auto, auto) { notification.notify(); });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction(),
                            "extraId1",
                            FontCollectorSizesAuxiliaryData{sizes, "color", "text"});
    notification.wait();
}

} // namespace
