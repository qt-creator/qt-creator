// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callstacklanemodel.h"

#include <tracing/timelinemodelaggregator.h>

#include <QtTest>

using namespace Profiler::Internal;
using namespace std::chrono;
using namespace std::chrono_literals;

constexpr quint64 tid = 10;
constexpr quint64 otherTid = 11;

// An item as the lane shows it: which function, where, and at which depth.
struct Item
{
    QString function;
    nanoseconds start;
    nanoseconds duration;
    int depth;

    friend bool operator==(const Item &, const Item &) = default;
};

static char *toString(const Item &item)
{
    return QTest::toString(QString("%1 [%2 ns, %3 ns) depth %4")
                               .arg(item.function)
                               .arg(item.start.count())
                               .arg((item.start + item.duration).count())
                               .arg(item.depth));
}

// Rows count from the thread's header row, so depth 0 is row 1.
static QList<Item> items(const CallStackLaneModel &model)
{
    QList<Item> result;
    for (int i = 0; i < model.count(); ++i) {
        result.append({model.details(i).value("displayName"), model.startTime(i),
                       model.duration(i), model.expandedRow(i) - 1});
    }
    return result;
}

// The model's items in time order, which is also the order its rows are drawn
// and searched in: by start, and with equal starts the longer item first.
static bool isInTimeOrder(const CallStackLaneModel &model)
{
    for (int i = 1; i < model.count(); ++i) {
        if (model.startTime(i) < model.startTime(i - 1))
            return false;
        if (model.startTime(i) == model.startTime(i - 1)
            && model.duration(i) > model.duration(i - 1)) {
            return false;
        }
    }
    return true;
}

static SampleTraceData trace(const QList<SampleTraceData::ThreadSample> &samples)
{
    SampleTraceData data;
    data.pid = 1;
    data.labels = {"main", "work", "other", "recurse"};
    data.threadNames = {{tid, "worker"}};
    data.samples = samples;
    return data;
}

enum Label { Main, Work, Other, Recurse };

class tst_CallStackLaneModel : public QObject
{
    Q_OBJECT

private slots:
    void aFrameOverSeveralSamplesIsOneItem()
    {
        Timeline::TimelineModelAggregator aggregator;
        CallStackLaneModel model(&aggregator, tid);
        SampleTraceData data = trace({{0us, tid, true, {Main, Work}},
                                      {100us, tid, true, {Main, Work}},
                                      {200us, tid, true, {Main, Work}}});
        model.setTraceData(&data);

        // Each ends one sample interval after its last sample.
        QCOMPARE(items(model), QList<Item>({{"main", 0us, 300us, 0}, {"work", 0us, 300us, 1}}));
        QCOMPARE(model.expandedRowCount(), 3); // the header, then a row per depth
        QCOMPARE(model.labels().size(), 2);
    }

    void aFrameEndsAtTheFirstSampleWithoutIt()
    {
        Timeline::TimelineModelAggregator aggregator;
        CallStackLaneModel model(&aggregator, tid);
        SampleTraceData data = trace({{0us, tid, true, {Main, Work}},
                                      {100us, tid, true, {Main, Work}},
                                      {200us, tid, true, {Main, Other}}});
        model.setTraceData(&data);

        QCOMPARE(items(model), QList<Item>({{"main", 0us, 300us, 0},
                                            {"work", 0us, 200us, 1},
                                            {"other", 200us, 100us, 1}}));
    }

    void recursionGivesAnItemPerDepth()
    {
        Timeline::TimelineModelAggregator aggregator;
        CallStackLaneModel model(&aggregator, tid);
        SampleTraceData data = trace({{0us, tid, true, {Main, Recurse, Recurse}},
                                      {100us, tid, true, {Main, Recurse, Recurse}}});
        model.setTraceData(&data);

        QCOMPARE(items(model), QList<Item>({{"main", 0us, 200us, 0},
                                            {"recurse", 0us, 200us, 1},
                                            {"recurse", 0us, 200us, 2}}));
    }

    // The child closes, and is inserted, before the parent that started with
    // it: the items still have to line up with their depths.
    void framesStartingTogetherKeepTheirDepths()
    {
        Timeline::TimelineModelAggregator aggregator;
        CallStackLaneModel model(&aggregator, tid);
        SampleTraceData data = trace({{0us, tid, true, {Main, Work, Other}},
                                      {100us, tid, true, {Main, Work}},
                                      {200us, tid, true, {Main}}});
        model.setTraceData(&data);

        QCOMPARE(items(model), QList<Item>({{"main", 0us, 300us, 0},
                                            {"work", 0us, 200us, 1},
                                            {"other", 0us, 100us, 2}}));
        QVERIFY(isInTimeOrder(model));
    }

    void anEmptyStackClosesEverything()
    {
        Timeline::TimelineModelAggregator aggregator;
        CallStackLaneModel model(&aggregator, tid);
        SampleTraceData data = trace({{0us, tid, true, {Main, Work}},
                                      {100us, tid, false, {}},
                                      {200us, tid, true, {Main, Work}}});
        model.setTraceData(&data);

        QCOMPARE(items(model), QList<Item>({{"main", 0us, 100us, 0},
                                            {"work", 0us, 100us, 1},
                                            {"main", 200us, 100us, 0},
                                            {"work", 200us, 100us, 1}}));
    }

    void aSingleSampleLastsOneMicrosecond()
    {
        Timeline::TimelineModelAggregator aggregator;
        CallStackLaneModel model(&aggregator, tid);
        SampleTraceData data = trace({{500us, tid, true, {Main}}});
        model.setTraceData(&data);

        QCOMPARE(items(model), QList<Item>({{"main", 500us, 1us, 0}}));
    }

    void samplesOfOtherThreadsAreLeftOut()
    {
        Timeline::TimelineModelAggregator aggregator;
        CallStackLaneModel model(&aggregator, tid);
        SampleTraceData data = trace({{0us, tid, true, {Main, Work}},
                                      {0us, otherTid, true, {Other}},
                                      {100us, otherTid, true, {}},
                                      {100us, tid, true, {Main, Work}},
                                      {200us, otherTid, true, {Main, Other}},
                                      {200us, tid, true, {Main, Work}}});
        model.setTraceData(&data);

        QCOMPARE(items(model), QList<Item>({{"main", 0us, 300us, 0}, {"work", 0us, 300us, 1}}));
        QCOMPARE(model.displayName(), QString("Stacks of worker (10)"));
    }

    // perf and Windows only sample a thread while it is on the CPU, so the
    // gap between its last two samples says nothing about the trace's interval.
    void theLastFramesLastOneIntervalOfTheTrace()
    {
        Timeline::TimelineModelAggregator aggregator;
        CallStackLaneModel model(&aggregator, tid);
        QList<SampleTraceData::ThreadSample> samples;
        for (microseconds time = 0us; time <= 1000us; time += 100us) {
            if (time == 0us || time == 900us)
                samples.append({time, tid, true, {Main, Work}});
            samples.append({time, otherTid, true, {Other}});
        }
        SampleTraceData data = trace(samples);
        model.setTraceData(&data);

        QCOMPARE(items(model), QList<Item>({{"main", 0us, 1000us, 0}, {"work", 0us, 1000us, 1}}));
    }

    void onlyTheGivenSamplesAreRead()
    {
        Timeline::TimelineModelAggregator aggregator;
        CallStackLaneModel model(&aggregator, tid);
        SampleTraceData data = trace({{0us, tid, true, {Main, Work}},
                                      {100us, tid, true, {Main, Other}},
                                      {200us, tid, true, {Main, Work}}});
        model.setTraceData(&data, {0, 2});

        QCOMPARE(items(model), QList<Item>({{"main", 0us, 300us, 0}, {"work", 0us, 300us, 1}}));
    }

    void clearingRemovesTheItems()
    {
        Timeline::TimelineModelAggregator aggregator;
        CallStackLaneModel model(&aggregator, tid);
        SampleTraceData data = trace({{0us, tid, true, {Main, Work}}});
        model.setTraceData(&data);
        QCOMPARE(model.count(), 2);

        model.setTraceData(nullptr);
        QCOMPARE(model.count(), 0);
        QVERIFY(model.labels().isEmpty());
    }
};

QTEST_GUILESS_MAIN(tst_CallStackLaneModel)

#include "tst_callstacklanemodel.moc"
