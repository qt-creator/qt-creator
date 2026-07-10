// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "samplemerge.h"

#include <QtTest>

using namespace QmlProfiler::Internal;

using Label = SampleTraceData::Label;
using ThreadSample = SampleTraceData::ThreadSample;

// Resolves a sample's frame ids back to their label names, so a merged stack can
// be asserted by symbol rather than by (interned) label id.
static QStringList names(const SampleTraceData &data, int sampleIndex)
{
    QStringList result;
    for (int id : data.samples[sampleIndex].frames)
        result << data.labels[id].name;
    return result;
}

class tst_SampleMerge : public QObject
{
    Q_OBJECT

private slots:
    void engineFrameDetection()
    {
        QVERIFY(isEngineFrame(Label("QV4::Function::call")));
        QVERIFY(isEngineFrame(Label("QQmlBinding::evaluate")));
        QVERIFY(isEngineFrame(Label("JITCode:QtQml")));
        QVERIFY(isEngineFrame(Label("foo", QString(), 0, "libQt6Qml.so")));
        QVERIFY(!isEngineFrame(Label("main")));
        QVERIFY(!isEngineFrame(Label("MyModel::data", "/m.cpp", 5, "app")));
    }

    void replacesEngineRunWithJsStack()
    {
        // main > QV4::call > interpret > MyModel::data  (engine run in the middle)
        SampleTraceData native;
        native.pid = 1;
        native.labels = {"main", "QV4::call", "QV4::interpret", "MyModel::data"};
        native.samples = {{500, 1, true, {0, 1, 2, 3}}};

        // At t=500 the JS stack is onClicked() > compute() (root-first).
        const QList<QmlRange> ranges = {
            {100, 1000, "onClicked", "/main.qml", 10, -1},
            {400, 900, "compute", "/main.qml", 20, 0},
        };

        const SampleTraceData merged = mergeQmlIntoSamples(native, ranges);

        // Engine frames replaced; native above (main) and below (MyModel::data) kept.
        QCOMPARE(names(merged, 0),
                 (QStringList{"main", "onClicked", "compute", "MyModel::data"}));
        // The spliced JS labels carry their source location.
        const Label &onClicked = merged.labels[merged.samples[0].frames[1]];
        QCOMPARE(onClicked.file, QString("/main.qml"));
        QCOMPARE(onClicked.line, 10);
        QVERIFY(onClicked.module.isEmpty()); // empty module marks a JS frame
    }

    void interleaveKeepsEngineFrames()
    {
        SampleTraceData native;
        native.labels = {"main", "QV4::call", "MyModel::data"};
        native.samples = {{500, 1, true, {0, 1, 2}}};
        const QList<QmlRange> ranges = {{0, 1000, "onClicked", "/main.qml", 10, -1}};

        MergeOptions opts;
        opts.revealEngineFrames = true;
        const SampleTraceData merged = mergeQmlIntoSamples(native, ranges, opts);

        QCOMPARE(names(merged, 0),
                 (QStringList{"main", "QV4::call", "onClicked", "MyModel::data"}));
    }

    void sampleWithoutEngineRunIsUnchanged()
    {
        SampleTraceData native;
        native.labels = {"main", "compute", "work"};
        native.samples = {{500, 1, true, {0, 1, 2}}};
        const QList<QmlRange> ranges = {{0, 1000, "onClicked", "/main.qml", 10, -1}};

        const SampleTraceData merged = mergeQmlIntoSamples(native, ranges);
        QCOMPARE(names(merged, 0), (QStringList{"main", "compute", "work"}));
        QCOMPARE(merged.labels.size(), native.labels.size()); // nothing interned
    }

    void engineRunWithoutActiveJsIsKept()
    {
        // A sample whose time falls outside every range: the engine frames must
        // survive rather than be dropped, so engine housekeeping stays visible.
        SampleTraceData native;
        native.labels = {"main", "QV4::gc"};
        native.samples = {{5000, 1, true, {0, 1}}};
        const QList<QmlRange> ranges = {{0, 1000, "onClicked", "/main.qml", 10, -1}};

        const SampleTraceData merged = mergeQmlIntoSamples(native, ranges);
        QCOMPARE(names(merged, 0), (QStringList{"main", "QV4::gc"}));
    }

    void jsLabelsAreInternedOnce()
    {
        // The same JS activation seen in two samples yields one shared label.
        SampleTraceData native;
        native.labels = {"main", "QV4::call"};
        native.samples = {
            {300, 1, true, {0, 1}},
            {700, 1, true, {0, 1}},
        };
        const QList<QmlRange> ranges = {{0, 1000, "onClicked", "/main.qml", 10, -1}};

        const SampleTraceData merged = mergeQmlIntoSamples(native, ranges);
        QCOMPARE(merged.labels.size(), 3); // main, QV4::call, onClicked
        QCOMPARE(merged.samples[0].frames.last(), merged.samples[1].frames.last());
    }

    void clockOffsetShiftsSampleLookup()
    {
        // The sample sits at tsUs=50, but the QML range only covers [1000,2000).
        // A +1200us offset moves the lookup into the range; without it, nothing
        // is active and the engine frame is kept.
        SampleTraceData native;
        native.labels = {"QV4::call"};
        native.samples = {{50, 1, true, {0}}};
        const QList<QmlRange> ranges = {{1000, 2000, "onClicked", "/main.qml", 10, -1}};

        QCOMPARE(names(mergeQmlIntoSamples(native, ranges), 0), (QStringList{"QV4::call"}));

        MergeOptions opts;
        opts.sampleTimeOffsetUs = 1200; // 50 + 1200 = 1250, inside [1000,2000)
        QCOMPARE(names(mergeQmlIntoSamples(native, ranges, opts), 0), (QStringList{"onClicked"}));
    }

    void negativeClockOffsetIsClampedAtZero()
    {
        SampleTraceData native;
        native.labels = {"QV4::call"};
        native.samples = {{100, 1, true, {0}}};
        const QList<QmlRange> ranges = {{0, 50, "onClicked", "/main.qml", 10, -1}};

        MergeOptions opts;
        opts.sampleTimeOffsetUs = -1000; // 100 - 1000 clamps to 0, inside [0,50)
        QCOMPARE(names(mergeQmlIntoSamples(native, ranges, opts), 0), (QStringList{"onClicked"}));
    }

    void picksInnermostRangeAtSampleTime()
    {
        SampleTraceData native;
        native.labels = {"QV4::call"};
        native.samples = {
            {150, 1, true, {0}}, // only onClicked active
            {450, 1, true, {0}}, // onClicked > compute active
        };
        const QList<QmlRange> ranges = {
            {100, 1000, "onClicked", "/main.qml", 10, -1},
            {400, 900, "compute", "/main.qml", 20, 0},
        };

        const SampleTraceData merged = mergeQmlIntoSamples(native, ranges);
        QCOMPARE(names(merged, 0), (QStringList{"onClicked"}));
        QCOMPARE(names(merged, 1), (QStringList{"onClicked", "compute"}));
    }
};

QTEST_GUILESS_MAIN(tst_SampleMerge)

#include "tst_samplemerge.moc"
