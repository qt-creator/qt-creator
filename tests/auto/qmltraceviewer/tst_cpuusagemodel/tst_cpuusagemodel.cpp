// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpuusagemodel.h"

#include <tracing/timelinemodelaggregator.h>

#include <QtTest>

using namespace QmlProfiler::Internal;

// Three ticks at t=0, 100, 200 µs with two threads:
//   t=0:   both running (usage 2)
//   t=100: only thread 10 running (usage 1)
//   t=200: none running (usage 0)
static SampleTraceData makeTestData()
{
    SampleTraceData data;
    data.pid = 1;
    data.labels = {"start"};
    data.threadNames = {{10, "main"}, {11, QString()}};
    data.samples = {
        {0, 10, true, {0}},
        {0, 11, true, {0}},
        {100, 10, true, {0}},
        {100, 11, false, {0}},
        {200, 10, false, {0}},
        {200, 11, false, {0}},
    };
    return data;
}

class tst_CpuUsageModel : public QObject
{
    Q_OBJECT

private slots:
    void rowsAndCounts()
    {
        Timeline::TimelineModelAggregator aggregator;
        CpuUsageModel model(&aggregator);
        SampleTraceData data = makeTestData();
        model.setTraceData(&data);

        // 3 total items + 3 running-thread items (2 at t=0, 1 at t=100).
        QCOMPARE(model.count(), 6);
        // Row 0 is the timeline's category header row.
        QCOMPARE(model.expandedRowCount(), 4); // header + total + 2 threads
        QCOMPARE(model.collapsedRowCount(), 2); // header + total
        // Total row scaled to its peak (2 concurrent threads), labeled per-core: 200%.
        QCOMPARE(model.rowMaxValue(1), qint64(200));
        QCOMPARE(model.traceEndNs(), qint64(200) * 1000);
    }

    void heightsAndRows()
    {
        Timeline::TimelineModelAggregator aggregator;
        CpuUsageModel model(&aggregator);
        SampleTraceData data = makeTestData();
        model.setTraceData(&data);
        model.setExpanded(true);

        int totalItems = 0;
        int threadItems = 0;
        for (int i = 0; i < model.count(); ++i) {
            if (model.expandedRow(i) == 1) {
                ++totalItems;
                // Total height = running / peak(2): t0 -> 2/2, t100 -> 1/2, t200 -> 0.
                if (model.startTime(i) == 0)
                    QCOMPARE(model.relativeHeight(i), 1.0f);
                else if (model.startTime(i) == 100000) // 100 µs in ns
                    QCOMPARE(model.relativeHeight(i), 0.5f);
                else
                    QCOMPARE(model.relativeHeight(i), 0.0f);
                QCOMPARE(model.collapsedRow(i), 1);
            } else {
                ++threadItems;
                QVERIFY(model.expandedRow(i) >= 2);
                QCOMPARE(model.relativeHeight(i), 1.0f); // expanded: full height
            }
        }
        QCOMPARE(totalItems, 3);
        QCOMPARE(threadItems, 3);

        // Collapsed: thread items share the total row and repeat its fraction
        // (running / peak) instead of drawing full-height bars over it.
        model.setExpanded(false);
        for (int i = 0; i < model.count(); ++i) {
            if (model.expandedRow(i) >= 2) {
                QCOMPARE(model.relativeHeight(i),
                         float(model.startTime(i) == 0 ? 1.0f : 0.5f));
            }
        }
    }

    void labelsNameThreads()
    {
        Timeline::TimelineModelAggregator aggregator;
        CpuUsageModel model(&aggregator);
        SampleTraceData data = makeTestData();
        model.setTraceData(&data);

        const Timeline::RowLabels labels = model.labels();
        QCOMPARE(labels.size(), 3); // Total + 2 threads
        QCOMPARE(labels[0].description, QString("Total"));
        QVERIFY2(labels[1].description.contains("main"), qPrintable(labels[1].description));
        QVERIFY2(labels[2].description.contains("11"), qPrintable(labels[2].description));
    }

    // The total row must read as a bar graph: a constant color (heights carry
    // the information) and a row tall enough for the track painter to draw its
    // value-scale axis (>= 60 px).
    void totalRowRendersAsGraph()
    {
        Timeline::TimelineModelAggregator aggregator;
        CpuUsageModel model(&aggregator);
        SampleTraceData data = makeTestData();
        model.setTraceData(&data);

        QRgb totalColor = 0;
        bool seenTotal = false;
        for (int i = 0; i < model.count(); ++i) {
            if (model.expandedRow(i) != 1)
                continue;
            if (!seenTotal) {
                totalColor = model.color(i);
                seenTotal = true;
            } else {
                QCOMPARE(model.color(i), totalColor); // same hue for every load level
            }
        }
        QVERIFY(seenTotal);
        QCOMPARE(model.expandedRowHeight(1), 3 * Timeline::TimelineModel::defaultRowHeight());
    }

    void densityZoomedOutAverages()
    {
        // 5 ticks at 0,100,200,300,400 us. thread 10 on for 3 of them,
        // thread 11 on for 2; peak concurrent running = 2.
        SampleTraceData data;
        data.pid = 1;
        data.labels = {"f"};
        data.threadNames = {{10, "a"}, {11, "b"}};
        data.samples = {
            {0, 10, true, {0}},   {0, 11, true, {0}},    // running 2
            {100, 10, true, {0}}, {100, 11, false, {0}}, // running 1
            {200, 10, true, {0}}, {200, 11, false, {0}}, // running 1
            {300, 10, false, {0}},{300, 11, true, {0}},  // running 1
            {400, 10, false, {0}},{400, 11, false, {0}}, // running 0
        };
        Timeline::TimelineModelAggregator aggregator;
        CpuUsageModel model(&aggregator);
        model.setTraceData(&data);
        QVERIFY(model.rendersAsDensity());

        // One column covering the whole trace [0, 400us] in ns.
        QList<float> col(1, 0.0f);
        // thread 10 row (row 2): on 3 of 5 ticks -> 0.6
        QVERIFY(model.fillDensityColumns(2, 0, qint64(400) * 1000, col));
        QCOMPARE(col[0], 0.6f);
        // thread 11 row (row 3): on 2 of 5 ticks -> 0.4
        col.assign(1, 0.0f);
        QVERIFY(model.fillDensityColumns(3, 0, qint64(400) * 1000, col));
        QCOMPARE(col[0], 0.4f);
        // total row (row 1): avg running (2+1+1+1+0)/5 = 1.0, / maxRunning(2) = 0.5
        col.assign(1, 0.0f);
        QVERIFY(model.fillDensityColumns(1, 0, qint64(400) * 1000, col));
        QCOMPARE(col[0], 0.5f);
    }

    void densityTwoColumnsSplit()
    {
        // thread on for the first half, off for the second.
        SampleTraceData data;
        data.pid = 1;
        data.labels = {"f"};
        data.threadNames = {{10, "a"}};
        data.samples = {
            {0, 10, true, {0}}, {100, 10, true, {0}},
            {200, 10, false, {0}}, {300, 10, false, {0}},
        };
        Timeline::TimelineModelAggregator aggregator;
        CpuUsageModel model(&aggregator);
        model.setTraceData(&data);

        // Two columns over [0, 400us): ticks 0,100 in col 0; 200,300 in col 1.
        QList<float> col(2, 0.0f);
        QVERIFY(model.fillDensityColumns(2, 0, qint64(400) * 1000, col));
        QCOMPARE(col[0], 1.0f); // both first-half ticks running
        QCOMPARE(col[1], 0.0f); // both second-half ticks idle
    }

    void densityZoomedInInheritsTick()
    {
        // More columns than ticks: empty columns inherit the covering tick.
        SampleTraceData data;
        data.pid = 1;
        data.labels = {"f"};
        data.threadNames = {{10, "a"}};
        data.samples = {{0, 10, true, {0}}, {100, 10, false, {0}}};
        Timeline::TimelineModelAggregator aggregator;
        CpuUsageModel model(&aggregator);
        model.setTraceData(&data);

        // 8 columns over [0,100us): early columns map to tick@0 (running),
        // later columns to tick@100 (idle); none left at the uninitialised -1.
        QList<float> col(8, -1.0f);
        QVERIFY(model.fillDensityColumns(2, 0, qint64(100) * 1000, col));
        QCOMPARE(col.first(), 1.0f); // covered by the running tick@0
        QCOMPARE(col.last(), 0.0f);  // covered by the idle tick@100
        for (float v : col)
            QVERIFY(v == 0.0f || v == 1.0f); // never the uninitialised -1
    }

    void clearEmptiesModel()
    {
        Timeline::TimelineModelAggregator aggregator;
        CpuUsageModel model(&aggregator);
        SampleTraceData data = makeTestData();
        model.setTraceData(&data);
        model.setTraceData(nullptr);
        QCOMPARE(model.count(), 0);
        QCOMPARE(model.traceEndNs(), qint64(0));
    }
};

QTEST_GUILESS_MAIN(tst_CpuUsageModel)

#include "tst_cpuusagemodel.moc"
