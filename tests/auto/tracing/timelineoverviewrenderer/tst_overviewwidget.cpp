// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelineoverviewwidget.h>
#include <tracing/timelinezoomcontrol.h>

#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <QApplication>
#include <QPixmap>
#include <QScopeGuard>
#include <QTest>

using namespace Timeline;

// Gives the paused bands colours to be told from the background by.
class BandTheme : public Utils::Theme
{
public:
    BandTheme() : Utils::Theme(QLatin1String("band"))
    {
        d->colors[Token_Background_Muted].first = QColor(Qt::blue);
        d->colors[Token_Stroke_Subtle].first = QColor(Qt::green);
    }
};

class DummyModel : public TimelineModel
{
public:
    DummyModel(TimelineModelAggregator *parent) : TimelineModel(parent) {}

    void loadData()
    {
        setCollapsedRowCount(3);
        setExpandedRowCount(3);
        for (int i = 0; i < 10; ++i)
            insert(i, i, i);
        emit contentChanged();
    }
};

// A model in the state every timeline model passes through while a trace loads:
// per-event rows are already assigned, the row count is only published from
// finalize(), so the model reports rows its row count does not cover.
class UnfinalizedRowModel : public TimelineModel
{
public:
    UnfinalizedRowModel(TimelineModelAggregator *parent) : TimelineModel(parent)
    {
        insert(0, 10, 1);
        insert(20, 10, 1);
    }

    int expandedRow(int) const override { return 2; }
    int collapsedRow(int) const override { return 2; }
    QRgb color(int) const override { return qRgb(255, 0, 0); }

    void finalize()
    {
        setCollapsedRowCount(3);
        setExpandedRowCount(3);
        emit contentChanged();
    }
};

static QImage rendered(QWidget &widget)
{
    QPixmap target(widget.size());
    widget.render(&target);
    return target.toImage();
}

// The pixel columns in which `a` and `b` differ.
static QList<int> differingColumns(const QImage &a, const QImage &b)
{
    QList<int> columns;
    for (int x = 0; x < a.width(); ++x) {
        for (int y = 0; y < a.height(); ++y) {
            if (a.pixel(x, y) != b.pixel(x, y)) {
                columns.append(x);
                break;
            }
        }
    }
    return columns;
}

static int redPixels(QWidget &widget)
{
    QPixmap target(widget.size());
    widget.render(&target);
    const QImage image = target.toImage();
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y).red() > 200)
                ++count;
        }
    }
    return count;
}

class tst_OverviewWidget : public QObject
{
    Q_OBJECT

private slots:
    void noCrashEmpty();
    void noCrashWithData();
    void rowsBeyondRowCount();
    void pausedRangesAreDrawnWhereTheyLie();
};

void tst_OverviewWidget::noCrashEmpty()
{
    TimelineModelAggregator aggregator;
    TimelineZoomControl zoom;
    TimelineOverviewWidget widget(&aggregator, &zoom);
    widget.resize(400, 50);
    widget.update();
    QApplication::processEvents();
}

void tst_OverviewWidget::noCrashWithData()
{
    TimelineModelAggregator aggregator;
    TimelineZoomControl zoom;
    auto model = new DummyModel(&aggregator);
    aggregator.addModel(model);
    model->loadData();
    zoom.setTrace(0, 10);
    TimelineOverviewWidget widget(&aggregator, &zoom);
    widget.resize(400, 50);
    widget.update();
    QApplication::processEvents();
}

// Painting a model whose rows are ahead of its row count must not index the
// per-row arrays out of bounds, and the events must show up once the row count
// covers them - the cached content pixmap has to be rebuilt for that.
void tst_OverviewWidget::rowsBeyondRowCount()
{
    TimelineModelAggregator aggregator;
    UnfinalizedRowModel model(&aggregator);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.row(0), 2);

    TimelineZoomControl zoom;
    zoom.setTrace(0, 30);
    aggregator.setModels({&model});
    TimelineOverviewWidget widget(&aggregator, &zoom);
    widget.resize(200, 50);
    QCOMPARE(redPixels(widget), 0);

    model.finalize();
    QVERIFY(redPixels(widget) > 0);
}

// 200 pixels over a trace from 100 to 1100: five time units a pixel.
void tst_OverviewWidget::pausedRangesAreDrawnWhereTheyLie()
{
    Utils::setCreatorTheme(new BandTheme);
    const QScopeGuard resetTheme([] { Utils::setCreatorTheme(nullptr); });

    TimelineModelAggregator aggregator;
    TimelineZoomControl zoom;
    zoom.setTrace(100, 1100);
    zoom.setRange(100, 1100);
    TimelineOverviewWidget widget(&aggregator, &zoom);
    widget.resize(200, 50);
    const QImage before = rendered(widget);

    aggregator.setPausedRanges({
        {0, 300},     // Straddles the start: pixels 0 to 40.
        {600, 700},   // Pixels 100 to 120.
        {900, 901},   // Shorter than a pixel, and still one wide at 160.
        {2000, 3000}, // Past the end.
    });
    const QList<int> columns = differingColumns(before, rendered(widget));

    QVERIFY(!columns.isEmpty());
    for (int x : columns) {
        const bool inBand = (x >= 0 && x <= 40) || (x >= 100 && x <= 120) || (x >= 159 && x <= 161);
        QVERIFY2(inBand, qPrintable(QString("column %1 changed").arg(x)));
    }
    QVERIFY(columns.contains(0));
    QVERIFY(columns.contains(39));
    QVERIFY(!columns.contains(41));
    QVERIFY(columns.contains(110));
    QVERIFY(std::any_of(columns.cbegin(), columns.cend(), [](int x) { return x >= 159 && x <= 161; }));

    aggregator.setPausedRanges({});
    QVERIFY(differingColumns(before, rendered(widget)).isEmpty());
}

QTEST_MAIN(tst_OverviewWidget)

#include "tst_overviewwidget.moc"
