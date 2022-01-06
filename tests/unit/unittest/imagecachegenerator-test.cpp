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
    QImage smallImage1{1, 1, QImage::Format_ARGB32};
    NiceMock<MockFunction<void(const QImage &, const QImage &)>> imageCallbackMock;
    NiceMock<MockFunction<void(QmlDesigner::ImageCache::AbortReason)>> abortCallbackMock;
    NiceMock<ImageCacheCollectorMock> collectorMock;
    NiceMock<MockImageCacheStorage> storageMock;
    QmlDesigner::ImageCacheGenerator generator{collectorMock, storageMock};
};

TEST_F(ImageCacheGenerator, CallsCollectorWithCaptureCallback)
{
    EXPECT_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{smallImage1});
        });
    EXPECT_CALL(imageCallbackMock, Call(_, _)).WillRepeatedly([&](const QImage &, const QImage &) {
        notification.notify();
    });

    generator.generateImage("name", {}, {}, imageCallbackMock.AsStdFunction(), {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, CallsCollectorOnlyIfNotProcessing)
{
    EXPECT_CALL(collectorMock, start(AnyOf(Eq("name"), Eq("name2")), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage("name", {}, {}, imageCallbackMock.AsStdFunction(), {}, {});
    generator.generateImage("name2", {}, {}, imageCallbackMock.AsStdFunction(), {}, {});
    notification.wait(2);
}

TEST_F(ImageCacheGenerator, ProcessTaskAfterFirstFinished)
{
    ON_CALL(imageCallbackMock, Call(_, _)).WillByDefault([&](const QImage &, const QImage &) {
        notification.notify();
    });

    EXPECT_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillOnce([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{smallImage1});
        });
    EXPECT_CALL(collectorMock, start(Eq("name2"), _, _, _, _))
        .WillOnce([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{smallImage1});
        });

    generator.generateImage("name", {}, {}, imageCallbackMock.AsStdFunction(), {}, {});
    generator.generateImage("name2", {}, {}, imageCallbackMock.AsStdFunction(), {}, {});
    notification.wait(2);
}

TEST_F(ImageCacheGenerator, DontCrashAtDestructingGenerator)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{smallImage1});
        });

    generator.generateImage(
        "name", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    generator.generateImage(
        "name2", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    generator.generateImage(
        "name3", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    generator.generateImage(
        "name4", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
}

TEST_F(ImageCacheGenerator, StoreImage)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{smallImage1});
        });

    EXPECT_CALL(storageMock,
                storeImage(Eq("name"), Eq(Sqlite::TimeStamp{11}), Eq(image1), Eq(smallImage1)))
        .WillRepeatedly([&](auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage("name", {}, {11}, imageCallbackMock.AsStdFunction(), {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, StoreImageWithExtraId)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{smallImage1});
        });

    EXPECT_CALL(storageMock,
                storeImage(Eq("name+extraId"), Eq(Sqlite::TimeStamp{11}), Eq(image1), Eq(smallImage1)))
        .WillRepeatedly([&](auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage("name", "extraId", {11}, imageCallbackMock.AsStdFunction(), {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, StoreNullImage)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{}, QImage{});
        });

    EXPECT_CALL(storageMock,
                storeImage(Eq("name"), Eq(Sqlite::TimeStamp{11}), Eq(QImage{}), Eq(QImage{})))
        .WillRepeatedly([&](auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage(
        "name", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, StoreNullImageWithExtraId)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{}, QImage{});
        });

    EXPECT_CALL(storageMock,
                storeImage(Eq("name+extraId"), Eq(Sqlite::TimeStamp{11}), Eq(QImage{}), Eq(QImage{})))
        .WillRepeatedly([&](auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage("name",
                            "extraId",
                            {11},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction(),
                            {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, AbortCallback)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{smallImage1});
        });
    ON_CALL(collectorMock, start(Eq("name2"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto abortCallback) {
            abortCallback(QmlDesigner::ImageCache::AbortReason::Failed);
        });

    EXPECT_CALL(imageCallbackMock, Call(_, _)).WillOnce([&](const QImage &, const QImage &) {
        notification.notify();
    });
    EXPECT_CALL(abortCallbackMock, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)))
        .WillOnce([&](auto) { notification.notify(); });

    generator.generateImage(
        "name", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    generator.generateImage(
        "name2", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait(2);
}

TEST_F(ImageCacheGenerator, StoreNullImageForAbortCallbackAbort)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto abortCallback) {
            abortCallback(QmlDesigner::ImageCache::AbortReason::Abort);
        });
    ON_CALL(collectorMock, start(Eq("dummyNotify"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto) { notification.notify(); });

    EXPECT_CALL(storageMock, storeImage(Eq("name"), _, _, _)).Times(0);

    generator.generateImage(
        "name", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    generator.generateImage("dummyNotify", {}, {}, {}, {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, DontStoreNullImageForAbortCallbackFailed)
{
    ON_CALL(collectorMock, start(_, _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto abortCallback) {
            abortCallback(QmlDesigner::ImageCache::AbortReason::Failed);
        });

    EXPECT_CALL(storageMock,
                storeImage(Eq("name"), Eq(Sqlite::TimeStamp{11}), Eq(QImage{}), Eq(QImage{})))
        .WillOnce([&](auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage(
        "name", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, AbortForEmptyImage)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{}, QImage{});
        });

    EXPECT_CALL(abortCallbackMock, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)))
        .WillOnce([&](auto) { notification.notify(); });

    generator.generateImage(
        "name", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, CallWalCheckpointFullIfQueueIsEmpty)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) { captureCallback({}, {}); });

    EXPECT_CALL(storageMock, walCheckpointFull()).WillRepeatedly([&]() { notification.notify(); });

    generator.generateImage(
        "name", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    generator.generateImage(
        "name2", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, CleanIsCallingAbortCallback)
{
    ON_CALL(collectorMock, start(_, _, _, _, _)).WillByDefault([&](auto, auto, auto, auto, auto) {
        notification.wait();
    });
    generator.generateImage(
        "name", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    generator.generateImage(
        "name2", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});

    EXPECT_CALL(abortCallbackMock, Call(Eq(QmlDesigner::ImageCache::AbortReason::Abort)))
        .Times(AtLeast(1))
        .WillRepeatedly([&](auto) { waitInThread.notify(); });

    generator.clean();
    notification.notify();
    waitInThread.wait();
}

TEST_F(ImageCacheGenerator, WaitForFinished)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            waitInThread.wait();
            captureCallback(QImage{image1}, QImage{smallImage1});
        });
    ON_CALL(collectorMock, start(Eq("name2"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{smallImage1});
        });

    generator.generateImage(
        "name", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    generator.generateImage(
        "name2", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});

    EXPECT_CALL(imageCallbackMock, Call(_, _)).Times(2);

    waitInThread.notify();
    generator.waitForFinished();
}

TEST_F(ImageCacheGenerator, CallsCollectorWithExtraId)
{
    EXPECT_CALL(collectorMock, start(Eq("name"), Eq("extraId1"), _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage("name", "extraId1", {}, imageCallbackMock.AsStdFunction(), {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, CallsCollectorWithAuxiliaryData)
{
    using QmlDesigner::ImageCache::FontCollectorSizesAuxiliaryData;
    std::vector<QSize> sizes{{20, 11}};

    EXPECT_CALL(collectorMock,
                start(Eq("name"),
                      _,
                      VariantWith<FontCollectorSizesAuxiliaryData>(
                          AllOf(Field(&FontCollectorSizesAuxiliaryData::sizes,
                                      ElementsAre(QSize{20, 11})),
                                Field(&FontCollectorSizesAuxiliaryData::colorName, Eq(u"color")))),
                      _,
                      _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage("name",
                            {},
                            {},
                            imageCallbackMock.AsStdFunction(),
                            {},
                            FontCollectorSizesAuxiliaryData{sizes, "color"});
    notification.wait();
}

TEST_F(ImageCacheGenerator, MergeTasks)
{
    EXPECT_CALL(collectorMock, start(Eq("waitDummy"), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { waitInThread.wait(); });
    EXPECT_CALL(collectorMock, start(Eq("notificationDummy"), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    EXPECT_CALL(collectorMock, start(Eq("name"), _, _, _, _));

    generator.generateImage("waitDummy", {}, {}, {}, {}, {});
    generator.generateImage("name", {}, {}, {}, {}, {});
    generator.generateImage("name", {}, {}, {}, {}, {});
    generator.generateImage("notificationDummy", {}, {}, {}, {}, {});
    waitInThread.notify();
    notification.wait();
}

TEST_F(ImageCacheGenerator, DontMergeTasksWithDifferentId)
{
    EXPECT_CALL(collectorMock, start(Eq("waitDummy"), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { waitInThread.wait(); });

    EXPECT_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });
    EXPECT_CALL(collectorMock, start(Eq("name2"), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage("waitDummy", {}, {}, {}, {}, {});
    generator.generateImage("name", {}, {}, {}, {}, {});
    generator.generateImage("name2", {}, {}, {}, {}, {});
    waitInThread.notify();
    notification.wait(2);
}

TEST_F(ImageCacheGenerator, MergeTasksWithSameExtraId)
{
    EXPECT_CALL(collectorMock, start(Eq("waitDummy"), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { waitInThread.wait(); });
    EXPECT_CALL(collectorMock, start(Eq("notificationDummy"), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    EXPECT_CALL(collectorMock, start(Eq("name"), _, _, _, _));

    generator.generateImage("waitDummy", {}, {}, {}, {}, {});
    generator.generateImage("name", "id1", {}, {}, {}, {});
    generator.generateImage("name", "id1", {}, {}, {}, {});
    waitInThread.notify();
    generator.generateImage("notificationDummy", {}, {}, {}, {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, DontMergeTasksWithDifferentExtraId)
{
    EXPECT_CALL(collectorMock, start(Eq("waitDummy"), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { waitInThread.wait(); });

    EXPECT_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .Times(2)
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage("waitDummy", {}, {}, {}, {}, {});
    generator.generateImage("name", "id1", {}, {}, {}, {});
    generator.generateImage("name", "id2", {}, {}, {}, {});
    waitInThread.notify();
    notification.wait(2);
}

TEST_F(ImageCacheGenerator, UseLastTimeStampIfTasksAreMerged)
{
    ON_CALL(collectorMock, start(Eq("waitDummy"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto) { waitInThread.wait(); });
    ON_CALL(collectorMock, start(Eq("notificationDummy"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto) { notification.notify(); });
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto abortCallback) {
            abortCallback(QmlDesigner::ImageCache::AbortReason::Failed);
        });

    EXPECT_CALL(storageMock, storeImage(Eq("name"), Eq(Sqlite::TimeStamp{4}), _, _));

    generator.generateImage("waitDummy", {}, {}, {}, {}, {});
    generator.generateImage("name", {}, {3}, {}, abortCallbackMock.AsStdFunction(), {});
    generator.generateImage("name", {}, {4}, {}, abortCallbackMock.AsStdFunction(), {});
    generator.generateImage("notificationDummy", {}, {}, {}, {}, {});
    waitInThread.notify();
    notification.wait();
}

TEST_F(ImageCacheGenerator, MergeCaptureCallbackIfTasksAreMerged)
{
    NiceMock<MockFunction<void(const QImage &, const QImage &)>> newerImageCallbackMock;
    ON_CALL(collectorMock, start(Eq("waitDummy"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto) { waitInThread.wait(); });
    ON_CALL(collectorMock, start(Eq("notificationDummy"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto) { notification.notify(); });
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto imageCallback, auto) {
            imageCallback(QImage{image1}, QImage{smallImage1});
        });

    EXPECT_CALL(imageCallbackMock, Call(_, _));
    EXPECT_CALL(newerImageCallbackMock, Call(_, _));

    generator.generateImage("waitDummy", {}, {}, {}, {}, {});
    generator.generateImage("name", {}, {}, imageCallbackMock.AsStdFunction(), {}, {});
    generator.generateImage("name", {}, {}, newerImageCallbackMock.AsStdFunction(), {}, {});
    generator.generateImage("notificationDummy", {}, {}, {}, {}, {});
    waitInThread.notify();
    notification.wait();
}

TEST_F(ImageCacheGenerator, MergeAbortCallbackIfTasksAreMerged)
{
    NiceMock<MockFunction<void(QmlDesigner::ImageCache::AbortReason)>> newerAbortCallbackMock;
    ON_CALL(collectorMock, start(Eq("waitDummy"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto) { waitInThread.wait(); });
    ON_CALL(collectorMock, start(Eq("notificationDummy"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto) { notification.notify(); });
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto abortCallback) {
            abortCallback(QmlDesigner::ImageCache::AbortReason::Failed);
        });

    EXPECT_CALL(abortCallbackMock, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)));
    EXPECT_CALL(newerAbortCallbackMock, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)));

    generator.generateImage("waitDummy", {}, {}, {}, {}, {});
    generator.generateImage("name", {}, {}, {}, abortCallbackMock.AsStdFunction(), {});
    generator.generateImage("name", {}, {}, {}, newerAbortCallbackMock.AsStdFunction(), {});
    generator.generateImage("notificationDummy", {}, {}, {}, {}, {});
    waitInThread.notify();
    notification.wait();
}

TEST_F(ImageCacheGenerator, DontCallNullImageCallback)
{
    EXPECT_CALL(collectorMock, start(_, _, _, _, _))
        .WillOnce([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(image1, smallImage1);
            notification.notify();
        });

    generator.generateImage("name", {}, {}, {}, {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, DontCallNullAbortCallbackForNullImage)
{
    EXPECT_CALL(collectorMock, start(_, _, _, _, _))
        .WillOnce([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{}, QImage{});
            notification.notify();
        });

    generator.generateImage("name", {}, {}, {}, {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, DontCallNullAbortCallback)
{
    EXPECT_CALL(collectorMock, start(_, _, _, _, _))
        .WillOnce([&](auto, auto, auto, auto, auto abortCallback) {
            abortCallback(QmlDesigner::ImageCache::AbortReason::Failed);
            notification.notify();
        });

    generator.generateImage("name", {}, {}, {}, {}, {});
    notification.wait();
}

} // namespace
