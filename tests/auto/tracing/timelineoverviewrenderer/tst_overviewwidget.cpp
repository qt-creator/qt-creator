// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelineoverviewwidget.h>
#include <tracing/timelinezoomcontrol.h>

#include <QApplication>
#include <QPixmap>
#include <QTest>

using namespace Timeline;

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

QTEST_MAIN(tst_OverviewWidget)

#include "tst_overviewwidget.moc"
