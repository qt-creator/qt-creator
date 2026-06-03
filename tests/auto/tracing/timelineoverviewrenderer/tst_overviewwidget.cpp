// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinemodel.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelineoverviewwidget.h>
#include <tracing/timelinezoomcontrol.h>

#include <QApplication>
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

class tst_OverviewWidget : public QObject
{
    Q_OBJECT

private slots:
    void noCrashEmpty();
    void noCrashWithData();
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

QTEST_MAIN(tst_OverviewWidget)

#include "tst_overviewwidget.moc"
