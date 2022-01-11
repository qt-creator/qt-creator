/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
                      VariantWith<Utils::monostate>(Utils::monostate{}),
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
                      VariantWith<Utils::monostate>(Utils::monostate{}),
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

TEST_F(AsynchronousImageFactory, DontRequestImageRequestImageFromCollectorIfFileWasNotUpdated)
{
    ON_CALL(storageMock, fetchModifiedImageTime(Eq("/path/to/Component.qml"))).WillByDefault([&](auto) {
        notification.notify();
        return Sqlite::TimeStamp{124};
    });
    ON_CALL(timeStampProviderMock, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{124}));

    EXPECT_CALL(collectorMock, start(_, _, _, _, _)).Times(0);

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
                      VariantWith<Utils::monostate>(Utils::monostate{}),
                      _,
                      _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto) { notification.notify(); });

    factory.generate("/path/to/Component.qml");
    notification.wait();
}

} // namespace
