// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/mockimagecachestorage.h"
#include "../utils/notification.h"

#include <asynchronousexplicitimagecache.h>

namespace {

class AsynchronousExplicitImageCache : public testing::Test
{
protected:
    Notification notification;
    Notification waitInThread;
    NiceMock<MockImageCacheStorage> mockStorage;
    NiceMock<MockFunction<void(QmlDesigner::ImageCache::AbortReason)>> mockAbortCallback;
    NiceMock<MockFunction<void(QmlDesigner::ImageCache::AbortReason)>> mockAbortCallback2;
    NiceMock<MockFunction<void(const QImage &image)>> mockCaptureCallback;
    NiceMock<MockFunction<void(const QImage &image)>> mockCaptureCallback2;
    QmlDesigner::AsynchronousExplicitImageCache cache{mockStorage};
    QImage image1{10, 10, QImage::Format_ARGB32};
    QImage midSizeImage1{5, 5, QImage::Format_ARGB32};
    QImage smallImage1{1, 1, QImage::Format_ARGB32};
};

TEST_F(AsynchronousExplicitImageCache, request_image_fetches_image_from_storage)
{
    EXPECT_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{})))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{};
        });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, request_image_fetches_image_from_storage_with_time_stamp)
{
    EXPECT_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{})))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{};
        });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, request_image_calls_capture_callback_with_image_from_storage)
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

TEST_F(AsynchronousExplicitImageCache, request_image_calls_abort_callback_without_entry)
{
    ON_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{}));

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::NoEntry)))
        .WillRepeatedly([&](auto) { notification.notify(); });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, request_image_calls_abort_callback_without_image)
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

TEST_F(AsynchronousExplicitImageCache, request_mid_size_image_fetches_mid_size_image_from_storage)
{
    EXPECT_CALL(mockStorage, fetchMidSizeImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{})))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{};
        });

    cache.requestMidSizeImage("/path/to/Component.qml",
                              mockCaptureCallback.AsStdFunction(),
                              mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache,
       request_mid_size_image_calls_capture_callback_with_image_from_storage)
{
    ON_CALL(mockStorage, fetchMidSizeImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{midSizeImage1}));

    EXPECT_CALL(mockCaptureCallback, Call(Eq(midSizeImage1))).WillRepeatedly([&](const QImage &) {
        notification.notify();
    });

    cache.requestMidSizeImage("/path/to/Component.qml",
                              mockCaptureCallback.AsStdFunction(),
                              mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, request_mid_size_image_calls_abort_callback_without_entry)
{
    ON_CALL(mockStorage, fetchMidSizeImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{}));

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::NoEntry)))
        .WillRepeatedly([&](auto) { notification.notify(); });

    cache.requestMidSizeImage("/path/to/Component.qml",
                              mockCaptureCallback.AsStdFunction(),
                              mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache,
       request_mid_size_image_calls_abort_callback_without_mid_size_image)
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

TEST_F(AsynchronousExplicitImageCache, request_small_image_fetches_small_image_from_storage)
{
    EXPECT_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{})))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{};
        });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache,
       request_small_image_calls_capture_callback_with_image_from_storage)
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

TEST_F(AsynchronousExplicitImageCache, request_small_image_calls_abort_callback_without_entry)
{
    ON_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{}));

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::NoEntry)))
        .WillRepeatedly([&](auto) { notification.notify(); });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, request_small_image_calls_abort_callback_without_small_image)
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

TEST_F(AsynchronousExplicitImageCache, DISABLED_clean_removes_entries)
{
    ON_CALL(mockStorage, fetchSmallImage(_, _)).WillByDefault([&](Utils::SmallStringView, auto) {
        return QmlDesigner::ImageCacheStorageInterface::ImageEntry{smallImage1};
    });
    ON_CALL(mockCaptureCallback2, Call(_)).WillByDefault([&](auto) { waitInThread.wait(); });
    cache.requestSmallImage("/path/to/Component1.qml",
                            mockCaptureCallback2.AsStdFunction(),
                            mockAbortCallback2.AsStdFunction());

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::Abort)))
        .WillOnce([&](auto) { notification.notify(); });
    EXPECT_CALL(mockCaptureCallback, Call(_)).Times(0);

    cache.requestSmallImage("/path/to/Component3.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    cache.clean();
    waitInThread.notify();
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, clean_calls_abort)
{
    QmlDesigner::AsynchronousExplicitImageCache cache{mockStorage};
    ON_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component1.qml"), _))
        .WillByDefault([&](Utils::SmallStringView, auto) {
            notification.notify();
            waitInThread.wait();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{smallImage1};
        });
    cache.requestSmallImage("/path/to/Component1.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback2.AsStdFunction());
    notification.wait();
    cache.requestSmallImage("/path/to/Component2.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::Abort)))
        .WillOnce([&](auto) { notification.notify(); });

    cache.clean();
    waitInThread.notify();
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, after_clean_new_jobs_works)
{
    cache.clean();

    ON_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{})))
        .WillByDefault([&](Utils::SmallStringView, auto) {
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{smallImage1};
        });
    ON_CALL(mockCaptureCallback, Call(_)).WillByDefault([&](auto) { notification.notify(); });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, request_image_with_extra_id_fetches_image_from_storage)
{
    ON_CALL(mockAbortCallback, Call(_)).WillByDefault([&](auto) { notification.notify(); });

    EXPECT_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml+extraId1"), _))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{};
        });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction(),
                       "extraId1");
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, request_mid_size_image_with_extra_id_fetches_image_from_storage)
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

TEST_F(AsynchronousExplicitImageCache, request_small_image_with_extra_id_fetches_image_from_storage)
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

} // namespace
