// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/imagecachecollectormock.h"
#include "../mocks/mockimagecachestorage.h"
#include "../utils/notification.h"

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
    QImage midSizeImage1{5, 5, QImage::Format_ARGB32};
    QImage smallImage1{1, 1, QImage::Format_ARGB32};
    NiceMock<MockFunction<void(const QImage &, const QImage &, const QImage &)>> imageCallbackMock;
    NiceMock<MockFunction<void(QmlDesigner::ImageCache::AbortReason)>> abortCallbackMock;
    NiceMock<ImageCacheCollectorMock> collectorMock;
    NiceMock<MockImageCacheStorage> storageMock;
    QmlDesigner::ImageCacheGenerator generator{collectorMock, storageMock};
};

TEST_F(ImageCacheGenerator, calls_collector_with_capture_callback)
{
    EXPECT_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{midSizeImage1}, QImage{smallImage1});
        });
    EXPECT_CALL(imageCallbackMock, Call(_, _, _))
        .WillRepeatedly(
            [&](const QImage &, const QImage &, const QImage &) { notification.notify(); });

    generator.generateImage("name", {}, {}, imageCallbackMock.AsStdFunction(), {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, calls_collector_only_if_not_processing)
{
    EXPECT_CALL(collectorMock, start(AnyOf(Eq("name"), Eq("name2")), _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage("name", {}, {}, imageCallbackMock.AsStdFunction(), {}, {});
    generator.generateImage("name2", {}, {}, imageCallbackMock.AsStdFunction(), {}, {});
    notification.wait(2);
}

TEST_F(ImageCacheGenerator, process_task_after_first_finished)
{
    ON_CALL(imageCallbackMock, Call(_, _, _))
        .WillByDefault([&](const QImage &, const QImage &, const QImage &) { notification.notify(); });

    EXPECT_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillOnce([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{midSizeImage1}, QImage{smallImage1});
        });
    EXPECT_CALL(collectorMock, start(Eq("name2"), _, _, _, _))
        .WillOnce([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{midSizeImage1}, QImage{smallImage1});
        });

    generator.generateImage("name", {}, {}, imageCallbackMock.AsStdFunction(), {}, {});
    generator.generateImage("name2", {}, {}, imageCallbackMock.AsStdFunction(), {}, {});
    notification.wait(2);
}

TEST_F(ImageCacheGenerator, dont_crash_at_destructing_generator)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{midSizeImage1}, QImage{smallImage1});
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

TEST_F(ImageCacheGenerator, store_image)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{midSizeImage1}, QImage{smallImage1});
        });

    EXPECT_CALL(storageMock,
                storeImage(Eq("name"),
                           Eq(Sqlite::TimeStamp{11}),
                           Eq(image1),
                           Eq(midSizeImage1),
                           Eq(smallImage1)))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage("name", {}, {11}, imageCallbackMock.AsStdFunction(), {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, store_image_with_extra_id)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{midSizeImage1}, QImage{smallImage1});
        });

    EXPECT_CALL(storageMock,
                storeImage(Eq("name+extraId"),
                           Eq(Sqlite::TimeStamp{11}),
                           Eq(image1),
                           Eq(midSizeImage1),
                           Eq(smallImage1)))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage("name", "extraId", {11}, imageCallbackMock.AsStdFunction(), {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, store_null_image)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{}, QImage{}, QImage{});
        });

    EXPECT_CALL(
        storageMock,
        storeImage(Eq("name"), Eq(Sqlite::TimeStamp{11}), Eq(QImage{}), Eq(QImage{}), Eq(QImage{})))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage(
        "name", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, store_null_image_with_extra_id)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{}, QImage{}, QImage{});
        });

    EXPECT_CALL(storageMock,
                storeImage(Eq("name+extraId"),
                           Eq(Sqlite::TimeStamp{11}),
                           Eq(QImage{}),
                           Eq(QImage{}),
                           Eq(QImage{})))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage("name",
                            "extraId",
                            {11},
                            imageCallbackMock.AsStdFunction(),
                            abortCallbackMock.AsStdFunction(),
                            {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, abort_callback)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{midSizeImage1}, QImage{smallImage1});
        });
    ON_CALL(collectorMock, start(Eq("name2"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto abortCallback) {
            abortCallback(QmlDesigner::ImageCache::AbortReason::Failed);
        });

    EXPECT_CALL(imageCallbackMock, Call(_, _, _))
        .WillOnce([&](const QImage &, const QImage &, const QImage &) { notification.notify(); });
    EXPECT_CALL(abortCallbackMock, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)))
        .WillOnce([&](auto) { notification.notify(); });

    generator.generateImage(
        "name", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    generator.generateImage(
        "name2", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait(2);
}

TEST_F(ImageCacheGenerator, store_null_image_for_abort_callback_abort)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto abortCallback) {
            abortCallback(QmlDesigner::ImageCache::AbortReason::Abort);
        });
    ON_CALL(collectorMock, start(Eq("dummyNotify"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto) { notification.notify(); });

    EXPECT_CALL(storageMock, storeImage(Eq("name"), _, _, _, _)).Times(0);

    generator.generateImage(
        "name", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    generator.generateImage("dummyNotify", {}, {}, {}, {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, dont_store_null_image_for_abort_callback_failed)
{
    ON_CALL(collectorMock, start(_, _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto abortCallback) {
            abortCallback(QmlDesigner::ImageCache::AbortReason::Failed);
        });

    EXPECT_CALL(
        storageMock,
        storeImage(Eq("name"), Eq(Sqlite::TimeStamp{11}), Eq(QImage{}), Eq(QImage{}), Eq(QImage{})))
        .WillOnce([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage(
        "name", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, abort_for_null_image)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{}, QImage{}, QImage{});
        });

    EXPECT_CALL(abortCallbackMock, Call(Eq(QmlDesigner::ImageCache::AbortReason::Failed)))
        .WillOnce([&](auto) { notification.notify(); });

    generator.generateImage(
        "name", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, call_image_callback_if_small_image_is_not_null)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback({}, {}, smallImage1);
        });

    EXPECT_CALL(imageCallbackMock, Call(Eq(QImage()), Eq(QImage()), Eq(smallImage1)))
        .WillOnce([&](auto, auto, auto) { notification.notify(); });

    generator.generateImage(
        "name", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, store_image_if_small_image_is_not_null)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback({}, {}, smallImage1);
        });

    EXPECT_CALL(storageMock, storeImage(_, _, Eq(QImage()), Eq(QImage()), Eq(smallImage1)))
        .WillOnce([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage(
        "name", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, call_image_callback_if_mid_size_image_is_not_null)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback({}, midSizeImage1, {});
        });

    EXPECT_CALL(imageCallbackMock, Call(Eq(QImage()), Eq(midSizeImage1), Eq(QImage{})))
        .WillOnce([&](auto, auto, auto) { notification.notify(); });

    generator.generateImage(
        "name", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, store_image_if_mid_size_image_is_not_null)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback({}, midSizeImage1, {});
        });

    EXPECT_CALL(storageMock, storeImage(_, _, Eq(QImage()), Eq(midSizeImage1), Eq(QImage())))
        .WillOnce([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage(
        "name", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, call_image_callback_if_image_is_not_null)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault(
            [&](auto, auto, auto, auto captureCallback, auto) { captureCallback(image1, {}, {}); });

    EXPECT_CALL(imageCallbackMock, Call(Eq(image1), Eq(QImage{}), Eq(QImage{})))
        .WillOnce([&](auto, auto, auto) { notification.notify(); });

    generator.generateImage(
        "name", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, store_image_if_image_is_not_null)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault(
            [&](auto, auto, auto, auto captureCallback, auto) { captureCallback(image1, {}, {}); });

    EXPECT_CALL(storageMock, storeImage(_, _, Eq(image1), Eq(QImage{}), Eq(QImage{})))
        .WillOnce([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage(
        "name", {}, {}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, call_wal_checkpoint_full_if_queue_is_empty)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault(
            [&](auto, auto, auto, auto captureCallback, auto) { captureCallback({}, {}, {}); });

    EXPECT_CALL(storageMock, walCheckpointFull()).WillRepeatedly([&]() { notification.notify(); });

    generator.generateImage(
        "name", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    generator.generateImage(
        "name2", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, clean_is_calling_abort_callback)
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

TEST_F(ImageCacheGenerator, wait_for_finished)
{
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            waitInThread.wait();
            captureCallback(QImage{image1}, QImage{midSizeImage1}, QImage{smallImage1});
        });
    ON_CALL(collectorMock, start(Eq("name2"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{image1}, QImage{midSizeImage1}, QImage{smallImage1});
        });

    generator.generateImage(
        "name", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});
    generator.generateImage(
        "name2", {}, {11}, imageCallbackMock.AsStdFunction(), abortCallbackMock.AsStdFunction(), {});

    EXPECT_CALL(imageCallbackMock, Call(_, _, _)).Times(AtMost(2));

    waitInThread.notify();
    generator.waitForFinished();
}

TEST_F(ImageCacheGenerator, calls_collector_with_extra_id)
{
    EXPECT_CALL(collectorMock, start(Eq("name"), Eq("extraId1"), _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    generator.generateImage("name", "extraId1", {}, imageCallbackMock.AsStdFunction(), {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, calls_collector_with_auxiliary_data)
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
                            FontCollectorSizesAuxiliaryData{sizes, "color", "text"});
    notification.wait();
}

TEST_F(ImageCacheGenerator, merge_tasks)
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

TEST_F(ImageCacheGenerator, dont_merge_tasks_with_different_id)
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

TEST_F(ImageCacheGenerator, merge_tasks_with_same_extra_id)
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

TEST_F(ImageCacheGenerator, dont_merge_tasks_with_different_extra_id)
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

TEST_F(ImageCacheGenerator, use_last_time_stamp_if_tasks_are_merged)
{
    ON_CALL(collectorMock, start(Eq("waitDummy"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto) { waitInThread.wait(); });
    ON_CALL(collectorMock, start(Eq("notificationDummy"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto) { notification.notify(); });
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto abortCallback) {
            abortCallback(QmlDesigner::ImageCache::AbortReason::Failed);
        });

    EXPECT_CALL(storageMock, storeImage(Eq("name"), Eq(Sqlite::TimeStamp{4}), _, _, _));

    generator.generateImage("waitDummy", {}, {}, {}, {}, {});
    generator.generateImage("name", {}, {3}, {}, abortCallbackMock.AsStdFunction(), {});
    generator.generateImage("name", {}, {4}, {}, abortCallbackMock.AsStdFunction(), {});
    generator.generateImage("notificationDummy", {}, {}, {}, {}, {});
    waitInThread.notify();
    notification.wait();
}

TEST_F(ImageCacheGenerator, merge_capture_callback_if_tasks_are_merged)
{
    NiceMock<MockFunction<void(const QImage &, const QImage &, const QImage &)>> newerImageCallbackMock;
    ON_CALL(collectorMock, start(Eq("waitDummy"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto) { waitInThread.wait(); });
    ON_CALL(collectorMock, start(Eq("notificationDummy"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto, auto) { notification.notify(); });
    ON_CALL(collectorMock, start(Eq("name"), _, _, _, _))
        .WillByDefault([&](auto, auto, auto, auto imageCallback, auto) {
            imageCallback(QImage{image1}, QImage{midSizeImage1}, QImage{smallImage1});
        });

    EXPECT_CALL(imageCallbackMock, Call(_, _, _));
    EXPECT_CALL(newerImageCallbackMock, Call(_, _, _));

    generator.generateImage("waitDummy", {}, {}, {}, {}, {});
    generator.generateImage("name", {}, {}, imageCallbackMock.AsStdFunction(), {}, {});
    generator.generateImage("name", {}, {}, newerImageCallbackMock.AsStdFunction(), {}, {});
    generator.generateImage("notificationDummy", {}, {}, {}, {}, {});
    waitInThread.notify();
    notification.wait();
}

TEST_F(ImageCacheGenerator, merge_abort_callback_if_tasks_are_merged)
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

TEST_F(ImageCacheGenerator, dont_call_null_image_callback)
{
    EXPECT_CALL(collectorMock, start(_, _, _, _, _))
        .WillOnce([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(image1, midSizeImage1, smallImage1);
            notification.notify();
        });

    generator.generateImage("name", {}, {}, {}, {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, dont_call_null_abort_callback_for_null_image)
{
    EXPECT_CALL(collectorMock, start(_, _, _, _, _))
        .WillOnce([&](auto, auto, auto, auto captureCallback, auto) {
            captureCallback(QImage{}, QImage{}, QImage{});
            notification.notify();
        });

    generator.generateImage("name", {}, {}, {}, {}, {});
    notification.wait();
}

TEST_F(ImageCacheGenerator, dont_call_null_abort_callback)
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
