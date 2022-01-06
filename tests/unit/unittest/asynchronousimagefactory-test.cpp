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
        ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
            .WillByDefault(Return(Sqlite::TimeStamp{123}));
    }

protected:
    Notification notification;
    Notification waitInThread;
    NiceMock<MockImageCacheStorage> mockStorage;
    NiceMock<MockImageCacheGenerator> mockGenerator;
    NiceMock<MockTimeStampProvider> mockTimeStampProvider;
    QmlDesigner::AsynchronousImageFactory factory{mockStorage, mockGenerator, mockTimeStampProvider};
    QImage image1{10, 10, QImage::Format_ARGB32};
    QImage smallImage1{1, 1, QImage::Format_ARGB32};
};

TEST_F(AsynchronousImageFactory, RequestImageRequestImageFromGenerator)
{
    EXPECT_CALL(mockGenerator,
                generateImage(Eq("/path/to/Component.qml"),
                              IsEmpty(),
                              Eq(Sqlite::TimeStamp{123}),
                              _,
                              _,
                              VariantWith<Utils::monostate>(Utils::monostate{})))
        .WillRepeatedly([&](auto, auto, auto, auto, auto, auto) { notification.notify(); });

    factory.generate("/path/to/Component.qml");
    notification.wait();
}

TEST_F(AsynchronousImageFactory, RequestImageWithExtraIdRequestImageFromGenerator)
{
    EXPECT_CALL(mockGenerator,
                generateImage(Eq("/path/to/Component.qml"),
                              Eq("foo"),
                              Eq(Sqlite::TimeStamp{123}),
                              _,
                              _,
                              VariantWith<Utils::monostate>(Utils::monostate{})))
        .WillRepeatedly([&](auto, auto, auto, auto, auto, auto) { notification.notify(); });

    factory.generate("/path/to/Component.qml", "foo");
    notification.wait();
}

TEST_F(AsynchronousImageFactory, RequestImageWithAuxiliaryDataRequestImageFromGenerator)
{
    std::vector<QSize> sizes{{20, 11}};

    EXPECT_CALL(mockGenerator,
                generateImage(Eq("/path/to/Component.qml"),
                              Eq("foo"),
                              Eq(Sqlite::TimeStamp{123}),
                              _,
                              _,
                              VariantWith<FontCollectorSizesAuxiliaryData>(AllOf(
                                  Field(&FontCollectorSizesAuxiliaryData::sizes,
                                        ElementsAre(QSize{20, 11})),
                                  Field(&FontCollectorSizesAuxiliaryData::colorName, Eq(u"color")),
                                  Field(&FontCollectorSizesAuxiliaryData::text, Eq(u"some text"))))))
        .WillRepeatedly([&](auto, auto, auto, auto, auto, auto) { notification.notify(); });

    factory.generate("/path/to/Component.qml",
                     "foo",
                     QmlDesigner::ImageCache::FontCollectorSizesAuxiliaryData{sizes,
                                                                              "color",
                                                                              "some text"});
    notification.wait();
}

TEST_F(AsynchronousImageFactory, DontRequestImageRequestImageFromGeneratorIfFileWasNotUpdated)
{
    ON_CALL(mockStorage, fetchHasImage(Eq("/path/to/Component.qml"))).WillByDefault([&](auto) {
        notification.notify();
        return true;
    });
    ON_CALL(mockStorage, fetchModifiedImageTime(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{124}));
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{124}));

    EXPECT_CALL(mockGenerator, generateImage(_, _, _, _, _, _)).Times(0);

    factory.generate("/path/to/Component.qml");
    notification.wait();
}

TEST_F(AsynchronousImageFactory,
       RequestImageRequestImageFromGeneratorIfFileWasNotUpdatedButTheImageIsNull)
{
    ON_CALL(mockStorage, fetchHasImage(Eq("/path/to/Component.qml"))).WillByDefault(Return(false));
    ON_CALL(mockStorage, fetchModifiedImageTime(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{124}));
    ON_CALL(mockTimeStampProvider, timeStamp(Eq("/path/to/Component.qml")))
        .WillByDefault(Return(Sqlite::TimeStamp{124}));

    EXPECT_CALL(mockGenerator,
                generateImage(Eq("/path/to/Component.qml"),
                              IsEmpty(),
                              Eq(Sqlite::TimeStamp{124}),
                              _,
                              _,
                              VariantWith<Utils::monostate>(Utils::monostate{})))
        .WillRepeatedly([&](auto, auto, auto, auto, auto, auto) { notification.notify(); });

    factory.generate("/path/to/Component.qml");
    notification.wait();
}

TEST_F(AsynchronousImageFactory, CleanRemovesEntries)
{
    EXPECT_CALL(mockGenerator, generateImage(Eq("/path/to/Component1.qml"), _, _, _, _, _))
        .WillRepeatedly([&](auto, auto, auto, auto, auto, auto) { waitInThread.wait(); });
    factory.generate("/path/to/Component1.qml");

    EXPECT_CALL(mockGenerator, generateImage(Eq("/path/to/Component3.qml"), _, _, _, _, _)).Times(0);

    factory.generate("/path/to/Component3.qml");
    factory.clean();
    waitInThread.notify();
}

TEST_F(AsynchronousImageFactory, CleanCallsGeneratorClean)
{
    EXPECT_CALL(mockGenerator, clean()).Times(AtLeast(1));

    factory.clean();
}

TEST_F(AsynchronousImageFactory, AfterCleanNewJobsWorks)
{
    factory.clean();

    EXPECT_CALL(mockGenerator,
                generateImage(Eq("/path/to/Component.qml"),
                              IsEmpty(),
                              Eq(Sqlite::TimeStamp{123}),
                              _,
                              _,
                              VariantWith<Utils::monostate>(Utils::monostate{})))
        .WillRepeatedly([&](auto, auto, auto, auto, auto, auto) { notification.notify(); });

    factory.generate("/path/to/Component.qml");
    notification.wait();
}

} // namespace
