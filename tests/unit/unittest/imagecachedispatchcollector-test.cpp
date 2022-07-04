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

#include <imagecache/imagecachedispatchcollector.h>
#include <imagecacheauxiliarydata.h>

#include <tuple>

namespace {
using QmlDesigner::ImageCache::FontCollectorSizesAuxiliaryData;

MATCHER_P(IsIcon, icon, std::string(negation ? "isn't " : "is ") + PrintToString(icon))
{
    const QIcon &other = arg;
    return icon.availableSizes() == other.availableSizes();
}

MATCHER_P2(IsImage,
           image,
           smallImage,
           std::string(negation ? "aren't " : "are ") + PrintToString(image) + " and "
               + PrintToString(smallImage))
{
    const std::pair<QImage, QImage> &other = arg;
    return other.first == image && other.second == smallImage;
}

class ImageCacheDispatchCollector : public ::testing::Test
{
protected:
    ImageCacheDispatchCollector()
    {
        ON_CALL(collectorMock1, createIcon(_, _, _)).WillByDefault(Return(icon1));
        ON_CALL(collectorMock2, createIcon(_, _, _)).WillByDefault(Return(icon2));

        ON_CALL(collectorMock1, createImage(_, _, _))
            .WillByDefault(Return(std::pair{image1, smallImage1}));
        ON_CALL(collectorMock2, createImage(_, _, _))
            .WillByDefault(Return(std::pair{image2, smallImage2}));
    }

protected:
    std::vector<QSize> sizes{{20, 11}};
    NiceMock<MockFunction<void(const QImage &, const QImage &)>> captureCallbackMock;
    NiceMock<MockFunction<void(QmlDesigner::ImageCache::AbortReason)>> abortCallbackMock;
    NiceMock<ImageCacheCollectorMock> collectorMock1;
    NiceMock<ImageCacheCollectorMock> collectorMock2;
    QImage image1{1, 1, QImage::Format_ARGB32};
    QImage image2{2, 2, QImage::Format_ARGB32};
    QImage smallImage1{1, 1, QImage::Format_ARGB32};
    QImage smallImage2{2, 1, QImage::Format_ARGB32};
    QIcon icon1{QPixmap::fromImage(image1)};
    QIcon icon2{QPixmap::fromImage(image2)};
};

TEST_F(ImageCacheDispatchCollector, CallQmlCollectorStart)
{
    QmlDesigner::ImageCacheDispatchCollector collector{std::make_tuple(
        std::make_pair(
            [](Utils::SmallStringView filePath,
               [[maybe_unused]] Utils::SmallStringView state,
               [[maybe_unused]] const QmlDesigner::ImageCache::AuxiliaryData &auxiliaryData) {
                Utils::SmallStringView ending = ".ui.qml";
                return filePath.size() > ending.size()
                       && std::equal(ending.rbegin(), ending.rend(), filePath.rbegin());
            },
            &collectorMock1),
        std::make_pair(
            [](Utils::SmallStringView filePath,
               [[maybe_unused]] Utils::SmallStringView state,
               [[maybe_unused]] const QmlDesigner::ImageCache::AuxiliaryData &auxiliaryData) {
                Utils::SmallStringView ending = ".qml";
                return filePath.size() > ending.size()
                       && std::equal(ending.rbegin(), ending.rend(), filePath.rbegin());
            },
            &collectorMock2))};

    EXPECT_CALL(captureCallbackMock, Call(_, _));
    EXPECT_CALL(abortCallbackMock, Call(_));
    EXPECT_CALL(collectorMock2,
                start(Eq("foo.qml"),
                      Eq("state"),
                      VariantWith<FontCollectorSizesAuxiliaryData>(
                          AllOf(Field(&FontCollectorSizesAuxiliaryData::sizes,
                                      ElementsAre(QSize{20, 11})),
                                Field(&FontCollectorSizesAuxiliaryData::colorName, Eq(u"color")))),
                      _,
                      _))
        .WillRepeatedly([&](auto, auto, auto, auto captureCallback, auto abortCallback) {
            captureCallback(QImage{}, QImage{});
            abortCallback(QmlDesigner::ImageCache::AbortReason::Abort);
        });
    EXPECT_CALL(collectorMock1, start(_, _, _, _, _)).Times(0);

    collector.start("foo.qml",
                    "state",
                    FontCollectorSizesAuxiliaryData{sizes, "color", "text"},
                    captureCallbackMock.AsStdFunction(),
                    abortCallbackMock.AsStdFunction());
}

TEST_F(ImageCacheDispatchCollector, CallUiFileCollectorStart)
{
    QmlDesigner::ImageCacheDispatchCollector collector{std::make_tuple(
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return true; },
                       &collectorMock1),
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return true; },
                       &collectorMock2))};

    EXPECT_CALL(captureCallbackMock, Call(_, _));
    EXPECT_CALL(abortCallbackMock, Call(_));
    EXPECT_CALL(collectorMock1,
                start(Eq("foo.ui.qml"),
                      Eq("state"),
                      VariantWith<FontCollectorSizesAuxiliaryData>(
                          AllOf(Field(&FontCollectorSizesAuxiliaryData::sizes,
                                      ElementsAre(QSize{20, 11})),
                                Field(&FontCollectorSizesAuxiliaryData::colorName, Eq(u"color")))),
                      _,
                      _))
        .WillRepeatedly([&](auto, auto, auto, auto captureCallback, auto abortCallback) {
            captureCallback(QImage{}, QImage{});
            abortCallback(QmlDesigner::ImageCache::AbortReason::Abort);
        });
    EXPECT_CALL(collectorMock2, start(_, _, _, _, _)).Times(0);

    collector.start("foo.ui.qml",
                    "state",
                    FontCollectorSizesAuxiliaryData{sizes, "color", "text"},
                    captureCallbackMock.AsStdFunction(),
                    abortCallbackMock.AsStdFunction());
}

TEST_F(ImageCacheDispatchCollector, DontCallCollectorStartForUnknownFile)
{
    QmlDesigner::ImageCacheDispatchCollector collector{std::make_tuple(
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return false; },
                       &collectorMock1),
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return false; },
                       &collectorMock2))};

    EXPECT_CALL(collectorMock2, start(_, _, _, _, _)).Times(0);
    EXPECT_CALL(collectorMock1, start(_, _, _, _, _)).Times(0);

    collector.start("foo.mesh",
                    "state",
                    FontCollectorSizesAuxiliaryData{sizes, "color", "text"},
                    captureCallbackMock.AsStdFunction(),
                    abortCallbackMock.AsStdFunction());
}

TEST_F(ImageCacheDispatchCollector, CallFirstCollectorCreateIcon)
{
    QmlDesigner::ImageCacheDispatchCollector collector{std::make_tuple(
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return true; },
                       &collectorMock1),
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return true; },
                       &collectorMock2))};

    auto icon = collector.createIcon("foo.ui.qml",
                                     "state",
                                     FontCollectorSizesAuxiliaryData{sizes, "color", "text"});

    ASSERT_THAT(icon, IsIcon(icon1));
}

TEST_F(ImageCacheDispatchCollector, FirstCollectorCreateIconCalls)
{
    QmlDesigner::ImageCacheDispatchCollector collector{std::make_tuple(
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return true; },
                       &collectorMock1),
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return true; },
                       &collectorMock2))};

    EXPECT_CALL(collectorMock1,
                createIcon(Eq("foo.ui.qml"),
                           Eq("state"),
                           VariantWith<FontCollectorSizesAuxiliaryData>(
                               AllOf(Field(&FontCollectorSizesAuxiliaryData::sizes,
                                           ElementsAre(QSize{20, 11})),
                                     Field(&FontCollectorSizesAuxiliaryData::colorName,
                                           Eq(u"color"))))));
    EXPECT_CALL(collectorMock2, createIcon(_, _, _)).Times(0);

    auto icon = collector.createIcon("foo.ui.qml",
                                     "state",
                                     FontCollectorSizesAuxiliaryData{sizes, "color", "text"});
}

TEST_F(ImageCacheDispatchCollector, CallSecondCollectorCreateIcon)
{
    QmlDesigner::ImageCacheDispatchCollector collector{std::make_tuple(
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return false; },
                       &collectorMock1),
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return true; },
                       &collectorMock2))};

    auto icon = collector.createIcon("foo.qml",
                                     "state",
                                     FontCollectorSizesAuxiliaryData{sizes, "color", "text"});

    ASSERT_THAT(icon, IsIcon(icon2));
}

TEST_F(ImageCacheDispatchCollector, SecondCollectorCreateIconCalls)
{
    QmlDesigner::ImageCacheDispatchCollector collector{std::make_tuple(
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return false; },
                       &collectorMock1),
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return true; },
                       &collectorMock2))};

    EXPECT_CALL(collectorMock2,
                createIcon(Eq("foo.qml"),
                           Eq("state"),
                           VariantWith<FontCollectorSizesAuxiliaryData>(
                               AllOf(Field(&FontCollectorSizesAuxiliaryData::sizes,
                                           ElementsAre(QSize{20, 11})),
                                     Field(&FontCollectorSizesAuxiliaryData::colorName,
                                           Eq(u"color"))))));
    EXPECT_CALL(collectorMock1, createIcon(_, _, _)).Times(0);

    auto icon = collector.createIcon("foo.qml",
                                     "state",
                                     FontCollectorSizesAuxiliaryData{sizes, "color", "text"});
}

TEST_F(ImageCacheDispatchCollector, DontCallCollectorCreateIconForUnknownFile)
{
    QmlDesigner::ImageCacheDispatchCollector collector{std::make_tuple(
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return false; },
                       &collectorMock1),
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return false; },
                       &collectorMock2))};

    auto icon = collector.createIcon("foo.mesh",
                                     "state",
                                     FontCollectorSizesAuxiliaryData{sizes, "color", "text"});

    ASSERT_TRUE(icon.isNull());
}

TEST_F(ImageCacheDispatchCollector, CallFirstCollectorCreateImage)
{
    QmlDesigner::ImageCacheDispatchCollector collector{std::make_tuple(
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return true; },
                       &collectorMock1),
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return true; },
                       &collectorMock2))};

    auto image = collector.createImage("foo.qml",
                                       "state",
                                       FontCollectorSizesAuxiliaryData{sizes, "color", "text"});

    ASSERT_THAT(image, IsImage(image1, smallImage1));
}

TEST_F(ImageCacheDispatchCollector, FirstCollectorCreateImageCalls)
{
    QmlDesigner::ImageCacheDispatchCollector collector{std::make_tuple(
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return true; },
                       &collectorMock1),
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return true; },
                       &collectorMock2))};

    EXPECT_CALL(collectorMock1,
                createImage(Eq("foo.ui.qml"),
                            Eq("state"),
                            VariantWith<FontCollectorSizesAuxiliaryData>(
                                AllOf(Field(&FontCollectorSizesAuxiliaryData::sizes,
                                            ElementsAre(QSize{20, 11})),
                                      Field(&FontCollectorSizesAuxiliaryData::colorName,
                                            Eq(u"color"))))));
    EXPECT_CALL(collectorMock2, createImage(_, _, _)).Times(0);

    auto icon = collector.createImage("foo.ui.qml",
                                      "state",
                                      FontCollectorSizesAuxiliaryData{sizes, "color", "text"});
}

TEST_F(ImageCacheDispatchCollector, CallSecondCollectorCreateImage)
{
    QmlDesigner::ImageCacheDispatchCollector collector{std::make_tuple(
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return false; },
                       &collectorMock1),
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return true; },
                       &collectorMock2))};

    auto image = collector.createImage("foo.ui.qml",
                                       "state",
                                       FontCollectorSizesAuxiliaryData{sizes, "color", "text"});

    ASSERT_THAT(image, IsImage(image2, smallImage2));
}

TEST_F(ImageCacheDispatchCollector, SecondCollectorCreateImageCalls)
{
    QmlDesigner::ImageCacheDispatchCollector collector{std::make_tuple(
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return false; },
                       &collectorMock1),
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return true; },
                       &collectorMock2))};

    EXPECT_CALL(collectorMock2,
                createImage(Eq("foo.qml"),
                            Eq("state"),
                            VariantWith<FontCollectorSizesAuxiliaryData>(
                                AllOf(Field(&FontCollectorSizesAuxiliaryData::sizes,
                                            ElementsAre(QSize{20, 11})),
                                      Field(&FontCollectorSizesAuxiliaryData::colorName,
                                            Eq(u"color"))))));
    EXPECT_CALL(collectorMock1, createImage(_, _, _)).Times(0);

    auto icon = collector.createImage("foo.qml",
                                      "state",
                                      FontCollectorSizesAuxiliaryData{sizes, "color", "text"});
}

TEST_F(ImageCacheDispatchCollector, DontCallCollectorCreateImageForUnknownFile)
{
    QmlDesigner::ImageCacheDispatchCollector collector{std::make_tuple(
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return false; },
                       &collectorMock1),
        std::make_pair([](Utils::SmallStringView,
                          Utils::SmallStringView,
                          const QmlDesigner::ImageCache::AuxiliaryData &) { return false; },
                       &collectorMock2))};

    auto image = collector.createImage("foo.mesh",
                                       "state",
                                       FontCollectorSizesAuxiliaryData{sizes, "color", "text"});

    ASSERT_TRUE(image.first.isNull() && image.second.isNull());
}

} // namespace
