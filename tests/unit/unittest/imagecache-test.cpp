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

#include <imagecache.h>

namespace {

class ImageCache : public testing::Test
{
protected:
    Notification notification;
    Notification waitInThread;
    NiceMock<MockFunction<void()>> mockAbortCallback;
    NiceMock<MockFunction<void(const QImage &image)>> mockCaptureCallback;
    NiceMock<MockImageCacheStorage> mockStorage;
    NiceMock<MockImageCacheGenerator> mockGenerator;
    NiceMock<MockTimeStampProvider> mockTimeStampProvider;
    QmlDesigner::ImageCache cache{mockStorage, mockGenerator, mockTimeStampProvider};
    QImage image1{10, 10, QImage::Format_ARGB32};
};

TEST_F(ImageCache, RequestImageFetchesImageFromStorage)
{
    EXPECT_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), _))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::Entry{{}, false};
        });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, RequestImageFetchesImageFromStorageWithTimeStamp)
{
    EXPECT_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillRepeatedly(Return(Sqlite::TimeStamp{123}));
    EXPECT_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{123})))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::Entry{QImage{}, false};
        });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, RequestImageCallsCaptureCallbackWithImageFromStorage)
{
    ON_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::Entry{image1, true}));

    EXPECT_CALL(mockCaptureCallback, Call(Eq(image1))).WillRepeatedly([&](const QImage &) {
        notification.notify();
    });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, RequestImageCallsAbortCallbackWithoutImage)
{
    ON_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::Entry{QImage{}, true}));

    EXPECT_CALL(mockAbortCallback, Call()).WillRepeatedly([&] { notification.notify(); });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, RequestImageRequestImageFromGenerator)
{
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{123}));

    EXPECT_CALL(mockGenerator,
                generateImage(Eq("/path/to/Component.qml"), _, Eq(Sqlite::TimeStamp{123}), _, _))
        .WillRepeatedly([&](auto, auto, auto, auto &&callback, auto) { notification.notify(); });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, RequestImageCallsCaptureCallbackWithImageFromGenerator)
{
    ON_CALL(mockGenerator, generateImage(Eq("/path/to/Component.qml"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto &&callback, auto) {
            callback(QImage{image1});
            notification.notify();
        });

    EXPECT_CALL(mockCaptureCallback, Call(Eq(image1)));

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, RequestImageCallsAbortCallbackFromGenerator)
{
    ON_CALL(mockGenerator, generateImage(Eq("/path/to/Component.qml"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto &&, auto &&abortCallback) {
            abortCallback();
            notification.notify();
        });

    EXPECT_CALL(mockAbortCallback, Call());

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, RequestIconFetchesIconFromStorage)
{
    EXPECT_CALL(mockStorage, fetchIcon(Eq("/path/to/Component.qml"), _))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::Entry{{}, false};
        });

    cache.requestIcon("/path/to/Component.qml",
                      mockCaptureCallback.AsStdFunction(),
                      mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, RequestIconFetchesIconFromStorageWithTimeStamp)
{
    EXPECT_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillRepeatedly(Return(Sqlite::TimeStamp{123}));
    EXPECT_CALL(mockStorage, fetchIcon(Eq("/path/to/Component.qml"), Eq(Sqlite::TimeStamp{123})))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::Entry{QImage{}, false};
        });

    cache.requestIcon("/path/to/Component.qml",
                      mockCaptureCallback.AsStdFunction(),
                      mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, RequestIconCallsCaptureCallbackWithImageFromStorage)
{
    ON_CALL(mockStorage, fetchIcon(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::Entry{image1, true}));

    EXPECT_CALL(mockCaptureCallback, Call(Eq(image1))).WillRepeatedly([&](const QImage &) {
        notification.notify();
    });

    cache.requestIcon("/path/to/Component.qml",
                      mockCaptureCallback.AsStdFunction(),
                      mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, RequestIconCallsAbortCallbackWithoutIcon)
{
    ON_CALL(mockStorage, fetchIcon(Eq("/path/to/Component.qml"), _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::Entry{QImage{}, true}));

    EXPECT_CALL(mockAbortCallback, Call()).WillRepeatedly([&] { notification.notify(); });

    cache.requestIcon("/path/to/Component.qml",
                      mockCaptureCallback.AsStdFunction(),
                      mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, RequestIconRequestImageFromGenerator)
{
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{123}));

    EXPECT_CALL(mockGenerator,
                generateImage(Eq("/path/to/Component.qml"), _, Eq(Sqlite::TimeStamp{123}), _, _))
        .WillRepeatedly([&](auto, auto, auto, auto &&callback, auto) { notification.notify(); });

    cache.requestIcon("/path/to/Component.qml",
                      mockCaptureCallback.AsStdFunction(),
                      mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, RequestIconCallsCaptureCallbackWithImageFromGenerator)
{
    ON_CALL(mockGenerator, generateImage(Eq("/path/to/Component.qml"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto &&callback, auto) {
            callback(QImage{image1});
            notification.notify();
        });

    EXPECT_CALL(mockCaptureCallback, Call(Eq(image1)));

    cache.requestIcon("/path/to/Component.qml",
                      mockCaptureCallback.AsStdFunction(),
                      mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, RequestIconCallsAbortCallbackFromGenerator)
{
    ON_CALL(mockGenerator, generateImage(Eq("/path/to/Component.qml"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto &&, auto &&abortCallback) {
            abortCallback();
            notification.notify();
        });

    EXPECT_CALL(mockAbortCallback, Call());

    cache.requestIcon("/path/to/Component.qml",
                      mockCaptureCallback.AsStdFunction(),
                      mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, CleanRemovesEntries)
{
    EXPECT_CALL(mockGenerator, generateImage(_, _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto &&mockCaptureCallback, auto &&) {
            mockCaptureCallback(QImage{});
            waitInThread.wait();
        });
    cache.requestIcon("/path/to/Component1.qml",
                      mockCaptureCallback.AsStdFunction(),
                      mockAbortCallback.AsStdFunction());

    EXPECT_CALL(mockCaptureCallback, Call(_)).Times(AtMost(1));

    cache.requestIcon("/path/to/Component3.qml",
                      mockCaptureCallback.AsStdFunction(),
                      mockAbortCallback.AsStdFunction());
    cache.clean();
    waitInThread.notify();
}

TEST_F(ImageCache, CleanCallsAbort)
{
    ON_CALL(mockGenerator, generateImage(_, _, _, _, _))
        .WillByDefault(
            [&](auto, auto, auto, auto &&mockCaptureCallback, auto &&) { waitInThread.wait(); });
    cache.requestIcon("/path/to/Component1.qml",
                      mockCaptureCallback.AsStdFunction(),
                      mockAbortCallback.AsStdFunction());
    cache.requestIcon("/path/to/Component2.qml",
                      mockCaptureCallback.AsStdFunction(),
                      mockAbortCallback.AsStdFunction());

    EXPECT_CALL(mockAbortCallback, Call()).Times(AtLeast(2));

    cache.requestIcon("/path/to/Component3.qml",
                      mockCaptureCallback.AsStdFunction(),
                      mockAbortCallback.AsStdFunction());
    cache.clean();
    waitInThread.notify();
}

TEST_F(ImageCache, CleanCallsGeneratorClean)
{
    EXPECT_CALL(mockGenerator, clean()).Times(AtLeast(1));

    cache.clean();
}

TEST_F(ImageCache, AfterCleanNewJobsWorks)
{
    cache.clean();

    EXPECT_CALL(mockGenerator, generateImage(Eq("/path/to/Component.qml"), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto &&, auto &&) { notification.notify(); });

    cache.requestIcon("/path/to/Component.qml",
                      mockCaptureCallback.AsStdFunction(),
                      mockAbortCallback.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCache, WaitForFinished)
{
    ON_CALL(mockStorage, fetchImage(_, _))
        .WillByDefault(Return(QmlDesigner::ImageCacheStorageInterface::Entry{image1, true}));
    cache.requestImage("/path/to/Component1.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());
    cache.requestImage("/path/to/Component2.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction());

    EXPECT_CALL(mockCaptureCallback, Call(_)).Times(2);

    cache.waitForFinished();
}

TEST_F(ImageCache, WaitForFinishedInGenerator)
{
    EXPECT_CALL(mockGenerator, waitForFinished());

    cache.waitForFinished();
}

TEST_F(ImageCache, RequestImageWithStateFetchesImageFromStorage)
{
    EXPECT_CALL(mockStorage, fetchImage(Eq("/path/to/Component.qml+state1"), _))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::Entry{{}, false};
        });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction(),
                       "state1");
    notification.wait();
}

TEST_F(ImageCache, RequestIconWithStateFetchesImageFromStorage)
{
    EXPECT_CALL(mockStorage, fetchIcon(Eq("/path/to/Component.qml+state1"), _))
        .WillRepeatedly([&](Utils::SmallStringView, auto) {
            notification.notify();
            return QmlDesigner::ImageCacheStorageInterface::Entry{{}, false};
        });

    cache.requestIcon("/path/to/Component.qml",
                      mockCaptureCallback.AsStdFunction(),
                      mockAbortCallback.AsStdFunction(),
                      "state1");
    notification.wait();
}

TEST_F(ImageCache, RequestImageWithStateRequestImageFromGenerator)
{
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{123}));

    EXPECT_CALL(mockGenerator,
                generateImage(Eq("/path/to/Component.qml"), Eq("state1"), Eq(Sqlite::TimeStamp{123}), _, _))
        .WillRepeatedly([&](auto, auto, auto, auto &&callback, auto) { notification.notify(); });

    cache.requestImage("/path/to/Component.qml",
                       mockCaptureCallback.AsStdFunction(),
                       mockAbortCallback.AsStdFunction(),
                       "state1");
    notification.wait();
}

} // namespace
