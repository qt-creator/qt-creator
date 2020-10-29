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

#include "imagecachecollectormock.h"
#include "mockimagecachestorage.h"
#include "notification.h"

#include <imagecachegenerator.h>

#include <mutex>

namespace {

class ImageCacheGenerator : public testing::Test
{
protected:
    template<typename Callable, typename... Arguments>
    static void executeAsync(Callable &&call, Arguments... arguments)
    {
        std::thread thread(
            [](Callable &&call, Arguments... arguments) {
                call(std::forward<Arguments>(arguments)...);
            },
            std::forward<Callable>(call),
            std::forward<Arguments>(arguments)...);
        thread.detach();
    }

protected:
    Notification waitInThread;
    Notification notification;
    QImage image1{10, 10, QImage::Format_ARGB32};
    NiceMock<MockFunction<void(const QImage &)>> imageCallbackMock;
    NiceMock<MockFunction<void()>> abortCallbackMock;
    NiceMock<ImageCacheCollectorMock> collectorMock;
    NiceMock<MockImageCacheStorage> storageMock;
    QmlDesigner::ImageCacheGenerator generator{collectorMock, storageMock};
};

TEST_F(ImageCacheGenerator, CallsCollectorWithCaptureCallback)
{
    EXPECT_CALL(collectorMock, start(Eq("name"), _, _))
        .WillRepeatedly([&](auto, auto captureCallback, auto) { captureCallback(QImage{image1}); });
    EXPECT_CALL(imageCallbackMock, Call(_)).WillRepeatedly([&](const QImage &) {
        notification.notify();
    });

    generator.generateImage("name", {}, imageCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, CallsCollectorOnlyIfNotProcessing)
{
    EXPECT_CALL(collectorMock, start(Eq("name"), _, _)).WillRepeatedly([&](auto, auto, auto) {
        notification.notify();
    });

    generator.generateImage("name", {}, imageCallbackMock.AsStdFunction(), {});
    generator.generateImage("name", {}, imageCallbackMock.AsStdFunction(), {});
    notification.wait(2);
}

TEST_F(ImageCacheGenerator, ProcessTaskAfterFirstFinished)
{
    ON_CALL(imageCallbackMock, Call(_)).WillByDefault([&](const QImage &) { notification.notify(); });

    EXPECT_CALL(collectorMock, start(Eq("name"), _, _)).WillOnce([&](auto, auto captureCallback, auto) {
        captureCallback(QImage{image1});
    });
    EXPECT_CALL(collectorMock, start(Eq("name2"), _, _)).WillOnce([&](auto, auto captureCallback, auto) {
        captureCallback(QImage{image1});
    });

    generator.generateImage("name", {}, imageCallbackMock.AsStdFunction(), {});
    generator.generateImage("name2", {}, imageCallbackMock.AsStdFunction(), {});
    notification.wait(2);
}

TEST_F(ImageCacheGenerator, DontCrashAtDestructingGenerator)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _)).WillByDefault([&](auto, auto captureCallback, auto) {
        captureCallback(QImage{image1});
    });

    generator.generateImage("name",
                            {},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction());
    generator.generateImage("name2",
                            {},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction());
    generator.generateImage("name3",
                            {},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction());
    generator.generateImage("name4",
                            {},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction());
}

TEST_F(ImageCacheGenerator, StoreImage)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _)).WillByDefault([&](auto, auto captureCallback, auto) {
        captureCallback(QImage{image1});
    });

    EXPECT_CALL(storageMock, storeImage(Eq("name"), Eq(Sqlite::TimeStamp{11}), Eq(image1)))
        .WillRepeatedly([&](auto, auto, auto) { notification.notify(); });

    generator.generateImage("name", {11}, imageCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, StoreNullImage)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _)).WillByDefault([&](auto, auto captureCallback, auto) {
        captureCallback(QImage{});
    });

    EXPECT_CALL(storageMock, storeImage(Eq("name"), Eq(Sqlite::TimeStamp{11}), Eq(QImage{})))
        .WillRepeatedly([&](auto, auto, auto) { notification.notify(); });

    generator.generateImage("name",
                            {11},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCacheGenerator, AbortCallback)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _)).WillByDefault([&](auto, auto captureCallback, auto) {
        captureCallback(QImage{image1});
    });
    ON_CALL(collectorMock, start(Eq("name2"), _, _)).WillByDefault([&](auto, auto, auto abortCallback) {
        abortCallback();
    });

    EXPECT_CALL(imageCallbackMock, Call(_)).WillOnce([&](const QImage &) { notification.notify(); });
    EXPECT_CALL(abortCallbackMock, Call()).WillOnce([&]() { notification.notify(); });

    generator.generateImage("name",
                            {},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction());
    generator.generateImage("name2",
                            {},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction());
    notification.wait(2);
}

TEST_F(ImageCacheGenerator, StoreNullImageForAbortCallback)
{
    ON_CALL(collectorMock, start(_, _, _)).WillByDefault([&](auto, auto, auto abortCallback) {
        abortCallback();
    });

    EXPECT_CALL(storageMock, storeImage(Eq("name"), Eq(Sqlite::TimeStamp{11}), Eq(QImage{})))
        .WillOnce([&](auto, auto, auto) { notification.notify(); });

    generator.generateImage("name",
                            {11},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCacheGenerator, AbortForEmptyImage)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _)).WillByDefault([&](auto, auto captureCallback, auto) {
        captureCallback(QImage{});
    });

    EXPECT_CALL(abortCallbackMock, Call()).WillOnce([&]() { notification.notify(); });

    generator.generateImage("name",
                            {},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCacheGenerator, CallWalCheckpointFullIfQueueIsEmpty)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _)).WillByDefault([&](auto, auto captureCallback, auto) {
        captureCallback({});
    });

    EXPECT_CALL(storageMock, walCheckpointFull()).WillRepeatedly([&]() { notification.notify(); });

    generator.generateImage("name",
                            {11},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction());
    generator.generateImage("name2",
                            {11},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction());
    notification.wait();
}

TEST_F(ImageCacheGenerator, CleanIsCallingAbortCallback)
{
    ON_CALL(collectorMock, start(_, _, _)).WillByDefault([&](auto, auto captureCallback, auto) {
        captureCallback({});
        waitInThread.wait();
    });
    generator.generateImage("name",
                            {11},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction());
    generator.generateImage("name2",
                            {11},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction());

    EXPECT_CALL(abortCallbackMock, Call()).Times(AtLeast(1));

    generator.clean();
    waitInThread.notify();
}

} // namespace
