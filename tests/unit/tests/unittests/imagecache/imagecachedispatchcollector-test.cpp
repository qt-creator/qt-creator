// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/imagecachecollectormock.h"

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

MATCHER_P3(IsImage,
           image,
           midSizeImage,
           smallImage,
           std::string(negation ? "aren't " : "are ") + PrintToString(image) + ", "
               + PrintToString(midSizeImage) + " and " + PrintToString(smallImage))
{
    const std::tuple<QImage, QImage, QImage> &other = arg;
    return std::get<0>(other) == image && std::get<1>(other) == midSizeImage
           && std::get<2>(other) == smallImage;
}

class ImageCacheDispatchCollector : public ::testing::Test
{
protected:
    ImageCacheDispatchCollector()
    {
        ON_CALL(collectorMock1, createIcon(_, _, _)).WillByDefault(Return(icon1));
        ON_CALL(collectorMock2, createIcon(_, _, _)).WillByDefault(Return(icon2));

        ON_CALL(collectorMock1, createImage(_, _, _))
            .WillByDefault(Return(std::tuple{image1, midSizeImage1, smallImage1}));
        ON_CALL(collectorMock2, createImage(_, _, _))
            .WillByDefault(Return(std::tuple{image2, midSizeImage2, smallImage2}));
    }

protected:
    std::vector<QSize> sizes{{20, 11}};
    NiceMock<MockFunction<void(const QImage &, const QImage &, const QImage &)>> captureCallbackMock;
    NiceMock<MockFunction<void(QmlDesigner::ImageCache::AbortReason)>> abortCallbackMock;
    NiceMock<ImageCacheCollectorMock> collectorMock1;
    NiceMock<ImageCacheCollectorMock> collectorMock2;
    QImage image1{1, 1, QImage::Format_ARGB32};
    QImage image2{1, 2, QImage::Format_ARGB32};
    QImage midSizeImage1{2, 1, QImage::Format_ARGB32};
    QImage midSizeImage2{2, 2, QImage::Format_ARGB32};
    QImage smallImage1{3, 1, QImage::Format_ARGB32};
    QImage smallImage2{3, 2, QImage::Format_ARGB32};
    QIcon icon1{QPixmap::fromImage(image1)};
    QIcon icon2{QPixmap::fromImage(image2)};
};

TEST_F(ImageCacheDispatchCollector, call_qml_collector_start)
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

    EXPECT_CALL(captureCallbackMock, Call(_, _, _));
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
            captureCallback({}, {}, {});
            abortCallback(QmlDesigner::ImageCache::AbortReason::Abort);
        });
    EXPECT_CALL(collectorMock1, start(_, _, _, _, _)).Times(0);

    collector.start("foo.qml",
                    "state",
                    FontCollectorSizesAuxiliaryData{sizes, "color", "text"},
                    captureCallbackMock.AsStdFunction(),
                    abortCallbackMock.AsStdFunction());
}

TEST_F(ImageCacheDispatchCollector, call_ui_file_collector_start)
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

    EXPECT_CALL(captureCallbackMock, Call(_, _, _));
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
            captureCallback({}, {}, {});
            abortCallback(QmlDesigner::ImageCache::AbortReason::Abort);
        });
    EXPECT_CALL(collectorMock2, start(_, _, _, _, _)).Times(0);

    collector.start("foo.ui.qml",
                    "state",
                    FontCollectorSizesAuxiliaryData{sizes, "color", "text"},
                    captureCallbackMock.AsStdFunction(),
                    abortCallbackMock.AsStdFunction());
}

TEST_F(ImageCacheDispatchCollector, dont_call_collector_start_for_unknown_file)
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

TEST_F(ImageCacheDispatchCollector, call_first_collector_create_icon)
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

TEST_F(ImageCacheDispatchCollector, first_collector_create_icon_calls)
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

TEST_F(ImageCacheDispatchCollector, call_second_collector_create_icon)
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

TEST_F(ImageCacheDispatchCollector, second_collector_create_icon_calls)
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

TEST_F(ImageCacheDispatchCollector, dont_call_collector_create_icon_for_unknown_file)
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

TEST_F(ImageCacheDispatchCollector, call_first_collector_create_image)
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

    ASSERT_THAT(image, IsImage(image1, midSizeImage1, smallImage1));
}

TEST_F(ImageCacheDispatchCollector, first_collector_create_image_calls)
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

TEST_F(ImageCacheDispatchCollector, call_second_collector_create_image)
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

    ASSERT_THAT(image, IsImage(image2, midSizeImage2, smallImage2));
}

TEST_F(ImageCacheDispatchCollector, second_collector_create_image_calls)
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

TEST_F(ImageCacheDispatchCollector, dont_call_collector_create_image_for_unknown_file)
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

    ASSERT_TRUE(std::get<0>(image).isNull() && std::get<1>(image).isNull()
                && std::get<2>(image).isNull());
}

} // namespace
