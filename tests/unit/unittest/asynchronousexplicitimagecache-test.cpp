/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "googletest.h"

#include "mockimagecachegenerator.h"
#include "mockimagecachestorage.h"
#include "mocktimestampprovider.h"
#include "notification.h"

#include <asynchronousexplicitimagecache.h>

namespace {

class AsynchronousExplicitImageCache : public testing::Test
{
protected:
    Notification notification;
    Notification waitInThread;
    NiceMock<MockFunction<void(QmlDesigner::ImageCache::AbortReason)>> mockAbortCallback;
    NiceMock<MockFunction<void(QmlDesigner::ImageCache::AbortReason)>> mockAbortCallback2;
    NiceMock<MockFunction<void(const QImage &image)>> mockCaptureCallback;
    NiceMock<MockFunction<void(const QImage &image)>> mockCaptureCallback2;
    NiceMock<MockImageCacheStorage> mockStorage;
    QmlDesigner::AsynchronousExplicitImageCache cache{mockStorage};
    QImage image1{10, 10, QImage::Format_ARGB32};
    QImage smallImage1{1, 1, QImage::Format_ARGB32};
};

TEST_F(AsynchronousExplicitImageCache, RequestImageFetchesImageFromStorage)
{
    EXPECT_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{})))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{{}, false};
        });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, RequestImageFetchesImageFromStorageWithTimeStamp)
{
    EXPECT_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{})))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{QImage{}, false};
        });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, RequestImageCallsCaptureCallbackWithImageFromStorage)
{
    ON_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{image1, true}));

    EXPECT_CALL(mockCaptureCallback, Call(Eq(image1))).WillRepeatedly([&](const QImage &) {
        notification.notify();
    });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, RequestImageCallsAbortCallbackWithoutEntry)
{
    ON_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{image1, false}));

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)))
        .WillRepeatedly([&](auto) { notification.notify(); });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, RequestImageCallsAbortCallbackWithoutImage)
{
    ON_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{QImage{}, true}));

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)))
        .WillRepeatedly([&](auto) { notification.notify(); });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, RequestSmallImageFetchesSmallImageFromStorage)
{
    EXPECT_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{})))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{{}, false};
        });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, RequestSmallImageCallsCaptureCallbackWithImageFromStorage)
{
    ON_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{smallImage1, true}));

    EXPECT_CALL(mockCaptureCallback, Call(Eq(smallImage1))).WillRepeatedly([&](const QImage &) {
        notification.notify();
    });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, RequestSmallImageCallsAbortCallbackWithoutEntry)
{
    ON_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{smallImage1, false}));

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)))
        .WillRepeatedly([&](auto) { notification.notify(); });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, RequestSmallImageCallsAbortCallbackWithoutSmallImage)
{
    ON_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::ImageEntry{QImage{}, true}));

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)))
        .WillRepeatedly([&](auto) { notification.notify(); });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, CleanRemovesEntries)
{
    ON_CALL(mockStorage, fetchSmallImage(_, _)).WillByDefault([&](Utils::SmallStringView, auto) {
        return QmlDesigner::ImageCacheStorageInterface::ImageEntry{smallImage1, true};
    });
    ON_CALL(mockCaptureCallback2, Call(_)).WillByDefault([&](auto) { waitInThread.wait(); });
    cache.requestSmallImage("/path/to/Component1.qml",
                            mockCaptureCallback2.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());

    EXPECT_CALL(mockCaptureCallback, Call(_)).Times(0);

    cache.requestSmallImage("/path/to/Component3.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    cache.clean();
    waitInThread.notify();
}

TEST_F(AsynchronousExplicitImageCache, CleanCallsAbort)
{
    ON_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component1.qml"), _))
        .WillByDefault([&](Utils::SmallStringView, auto) {
            waitInThread.wait();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{smallImage1, true};
        });
    cache.requestSmallImage("/path/to/Component1.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback2.AsStdFunction());
    cache.requestSmallImage("/path/to/Component2.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());

    EXPECT_CALL(mockAbortCallback, Call(Eq(QmlDesigner::ImageCache::AbortReason::Abort)));

    cache.clean();
    waitInThread.notify();
}

TEST_F(AsynchronousExplicitImageCache, AfterCleanNewJobsWorks)
{
    cache.clean();

    ON_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{})))
        .WillByDefault([&](Utils::SmallStringView, auto) {
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{smallImage1, true};
        });
    ON_CALL(mockCaptureCallback, Call(_)).WillByDefault([&](auto) { notification.notify(); });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, RequestImageWithExtraIdFetchesImageFromStorage)
{
    EXPECT_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml+extraId1"), _))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{{}, false};
        });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction(),
                       "extraId1");
    notification.wait();
}

TEST_F(AsynchronousExplicitImageCache, RequestSmallImageWithExtraIdFetchesImageFromStorage)
{
    EXPECT_CALL(mockStorage, fetchSmallImage(Eq("/path/to/Component.qml+extraId1"), _))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::ImageEntry{{}, false};
        });

    cache.requestSmallImage("/path/to/Component.qml",
                            mockCaptureCallback.AsStdFunction(),
                            mockAbortCallback.AsStdFunction(),
                            "extraId1");
    notification.wait();
}

} // namespace
