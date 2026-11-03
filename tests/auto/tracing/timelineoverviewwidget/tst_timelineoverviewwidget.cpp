// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelineoverviewwidget.h>
#include <tracing/timelinezoomcontrol.h>

#include <utils/theme/theme.h>

#include <QImage>
#include <QTest>

using namespace Timeline;

class DummyTheme : public Utils::Theme
{
public:
    DummyTheme() : Utils::Theme(QLatin1String("dummy")) {}
};

// Drawing the content of the overview asks for the color of every item, and
// nothing else in the overview does. The items are red, which the dummy theme
// paints nothing in.
class CountingModel : public TimelineModel
{
public:
    explicit CountingModel(TimelineModelAggregator *aggregator)
        : TimelineModel(aggregator)
    {
        insert(0, 100, 0);
        insert(500, 100, 0);
        setCollapsedRowCount(1);
        setExpandedRowCount(1);
    }

    QRgb color(int index) const override
    {
        Q_UNUSED(index)
        ++colorRequests;
        return qRgb(255, 0, 0);
    }

    int collapsedRow(int index) const override
    {
        Q_UNUSED(index)
        return 0;
    }

    int expandedRow(int index) const override
    {
        Q_UNUSED(index)
        return 0;
    }

    mutable int colorRequests = 0;
};

class tst_TimelineOverviewWidget : public QObject
{
    Q_OBJECT

public:
    // A high-DPI screen, where device pixels and logical pixels differ.
    static void initMain() { qputenv("QT_SCALE_FACTOR", "2"); }

private slots:
    void initTestCase();
    void cleanupTestCase();

    void repaintingReusesTheContent();
    void hidingAModelRedrawsTheContent();
};

void tst_TimelineOverviewWidget::initTestCase()
{
    Utils::setCreatorTheme(new DummyTheme);
}

void tst_TimelineOverviewWidget::cleanupTestCase()
{
    // setCreatorTheme() deletes the previous theme.
    Utils::setCreatorTheme(nullptr);
}

void tst_TimelineOverviewWidget::repaintingReusesTheContent()
{
    TimelineModelAggregator aggregator;
    TimelineZoomControl zoom;
    zoom.setTrace(0, 1000);
    zoom.setRange(0, 1000);
    auto model = new CountingModel(&aggregator);
    aggregator.addModel(model);

    TimelineOverviewWidget overview(&aggregator, &zoom);
    overview.setAttribute(Qt::WA_DontShowOnScreen);
    overview.resize(400, 50);
    overview.show();
    QCOMPARE(overview.devicePixelRatio(), 2.0);

    QImage image(overview.size(), QImage::Format_RGB32);
    overview.render(&image);
    const int requestsForTheContent = model->colorRequests;
    QVERIFY(requestsForTheContent > 0);

    overview.render(&image);
    QCOMPARE(model->colorRequests, requestsForTheContent);
}

void tst_TimelineOverviewWidget::hidingAModelRedrawsTheContent()
{
    TimelineModelAggregator aggregator;
    TimelineZoomControl zoom;
    zoom.setTrace(0, 1000);
    zoom.setRange(0, 1000);
    auto model = new CountingModel(&aggregator);
    aggregator.addModel(model);

    TimelineOverviewWidget overview(&aggregator, &zoom);
    overview.setAttribute(Qt::WA_DontShowOnScreen);
    overview.resize(400, 50);
    overview.show();

    // In the band of the model, on the item from 0 to 100 and between the items
    const QPoint onItem(20, 30);
    const QPoint offItems(100, 30);

    QImage image(overview.size(), QImage::Format_RGB32);
    overview.render(&image);
    QVERIFY(image.pixel(onItem) != image.pixel(offItems));

    model->setHidden(true);
    overview.render(&image);
    QCOMPARE(image.pixel(onItem), image.pixel(offItems));
}

QTEST_MAIN(tst_TimelineOverviewWidget)

#include "tst_timelineoverviewwidget.moc"
