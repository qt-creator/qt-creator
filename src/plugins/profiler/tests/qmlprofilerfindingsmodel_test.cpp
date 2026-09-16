// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerfindingsmodel_test.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

using namespace QmlDebug;

namespace Profiler::Internal {

// 60 ms is above the rule's 50 ms threshold, 10 ms is below it.
static const qint64 slowCompileNs = 60000000;
static const qint64 fastCompileNs = 10000000;

QmlProfilerFindingsModelTest::QmlProfilerFindingsModelTest()
    : model(&manager)
{}

void QmlProfilerFindingsModelTest::initTestCase()
{
    manager.initialize();
    qint64 timestamp = 0;

    slowCompileTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(UndefinedMessage, Compiling, -1,
                                         QmlEventLocation(QString("Main.qml"), 0, 0)));
    fastCompileTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(UndefinedMessage, Compiling, -1,
                                         QmlEventLocation(QString("Small.qml"), 0, 0)));
    pixmapErrorTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(PixmapCacheEvent, UndefinedRangeType, PixmapLoadingError,
                                         QmlEventLocation(QString("image://provider/missing.svg"),
                                                          0, 0)));
    pixmapErrorStartTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(PixmapCacheEvent, UndefinedRangeType,
                                         PixmapLoadingStarted,
                                         QmlEventLocation(QString("image://provider/missing.svg"),
                                                          0, 0)));
    pixmapSizeTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(PixmapCacheEvent, UndefinedRangeType, PixmapSizeKnown,
                                         QmlEventLocation(QString("qrc:/huge.png"), 0, 0)));
    handlerTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(UndefinedMessage, HandlingSignal, -1,
                                         QmlEventLocation(QString("Panel.qml"), 42, 5)));
    createdTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(UndefinedMessage, Creating, -1,
                                         QmlEventLocation(QString("Page.qml"), 1, 1)));
    timerHandlerTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(UndefinedMessage, HandlingSignal, -1,
                                         QmlEventLocation(QString("Overlay.qml"), 36, 5)));
    bindingTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(UndefinedMessage, Binding, -1,
                                         QmlEventLocation(QString("Gauge.qml"), 12, 9)));
    animationTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(Event, UndefinedRangeType, AnimationFrame));
    jankHandlerTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(UndefinedMessage, HandlingSignal, -1,
                                         QmlEventLocation(QString("Chart.qml"), 88, 5)));
    blockingHandlerTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(UndefinedMessage, HandlingSignal, -1,
                                         QmlEventLocation(QString("Import.qml"), 15, 5)));
    delegateTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(UndefinedMessage, Creating, -1,
                                         QmlEventLocation(QString("Row.qml"), 3, 1)));
    churnBindingTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(UndefinedMessage, Binding, -1,
                                         QmlEventLocation(QString("Ticker.qml"), 7, 13)));
    allocatingJsTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(UndefinedMessage, Javascript, -1,
                                         QmlEventLocation(QString("Report.qml"), 55, 9)));
    memoryTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(MemoryAllocation, UndefinedRangeType, SmallItem));
    reloadStartTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(PixmapCacheEvent, UndefinedRangeType,
                                         PixmapLoadingStarted,
                                         QmlEventLocation(QString("qrc:/tile.png"), 0, 0)));
    reloadFinishTypeId = manager.numEventTypes();
    manager.appendEventType(QmlEventType(PixmapCacheEvent, UndefinedRangeType,
                                         PixmapLoadingFinished,
                                         QmlEventLocation(QString("qrc:/tile.png"), 0, 0)));

    const auto addCompileRange = [&](int typeId, qint64 durationNs) {
        QmlEvent event;
        event.setTypeIndex(typeId);
        event.setRangeStage(RangeStart);
        event.setTimestamp(timestamp);
        manager.appendEvent(QmlEvent(event));

        timestamp += durationNs;
        event.setRangeStage(RangeEnd);
        event.setTimestamp(timestamp);
        manager.appendEvent(std::move(event));
        ++timestamp;
    };

    addCompileRange(slowCompileTypeId, slowCompileNs);
    addCompileRange(fastCompileTypeId, fastCompileNs);

    // The same image fails three times: the engine retries rather than caching the failure.
    // Each attempt starts a load, so the retries look exactly like reloads until the error
    // is taken into account.
    for (int i = 0; i < 3; ++i) {
        QmlEvent started;
        started.setTypeIndex(pixmapErrorStartTypeId);
        started.setTimestamp(++timestamp);
        manager.appendEvent(std::move(started));

        QmlEvent failed;
        failed.setTypeIndex(pixmapErrorTypeId);
        failed.setTimestamp(++timestamp);
        manager.appendEvent(std::move(failed));
    }

    // A handler that builds a view while it runs: the Creating range nests inside it.
    {
        QmlEvent handler;
        handler.setTypeIndex(handlerTypeId);
        handler.setRangeStage(RangeStart);
        handler.setTimestamp(++timestamp);
        manager.appendEvent(QmlEvent(handler));

        QmlEvent creating;
        creating.setTypeIndex(createdTypeId);
        creating.setRangeStage(RangeStart);
        creating.setTimestamp(++timestamp);
        manager.appendEvent(QmlEvent(creating));

        timestamp += 30000000; // 30 ms of building, above the 20 ms threshold
        creating.setRangeStage(RangeEnd);
        creating.setTimestamp(timestamp);
        manager.appendEvent(std::move(creating));

        handler.setRangeStage(RangeEnd);
        handler.setTimestamp(++timestamp);
        manager.appendEvent(std::move(handler));
    }

    // A timer-driven handler: 60 runs, every 200 ms exactly.
    for (int i = 0; i < 60; ++i) {
        QmlEvent event;
        event.setTypeIndex(timerHandlerTypeId);
        event.setRangeStage(RangeStart);
        event.setTimestamp(timestamp += 200000000);
        manager.appendEvent(QmlEvent(event));

        event.setRangeStage(RangeEnd);
        event.setTimestamp(++timestamp);
        manager.appendEvent(std::move(event));
    }

    // An image decoded far larger than any screen needs it.
    {
        QmlEvent event;
        event.setTypeIndex(pixmapSizeTypeId);
        event.setTimestamp(++timestamp);
        event.setNumbers({4000, 3000, 0});
        manager.appendEvent(std::move(event));
    }

    // 100 rendered frames, and a binding costing 1 ms of each of them.
    for (int i = 0; i < 100; ++i) {
        QmlEvent frame;
        frame.setTypeIndex(animationTypeId);
        frame.setTimestamp(++timestamp);
        frame.setNumbers({60, 1, 0});
        manager.appendEvent(std::move(frame));

        // The render thread reports the same rendering as a frame of its own. The two
        // together are one frame, not two.
        QmlEvent renderFrame;
        renderFrame.setTypeIndex(animationTypeId);
        renderFrame.setTimestamp(++timestamp);
        renderFrame.setNumbers({60, 1, 1});
        manager.appendEvent(std::move(renderFrame));

        QmlEvent binding;
        binding.setTypeIndex(bindingTypeId);
        binding.setRangeStage(RangeStart);
        binding.setTimestamp(++timestamp);
        manager.appendEvent(QmlEvent(binding));

        timestamp += 1000000; // 1 ms
        binding.setRangeStage(RangeEnd);
        binding.setTimestamp(timestamp);
        manager.appendEvent(std::move(binding));
    }

    // Five frames that take about 50 ms, each held up by a handler running 40 ms in it.
    // Averaged over the whole trace this work disappears; it is what ruins these frames.
    for (int i = 0; i < 5; ++i) {
        QmlEvent handler;
        handler.setTypeIndex(jankHandlerTypeId);
        handler.setRangeStage(RangeStart);
        handler.setTimestamp(++timestamp);
        manager.appendEvent(QmlEvent(handler));

        timestamp += 40000000;
        handler.setRangeStage(RangeEnd);
        handler.setTimestamp(timestamp);
        manager.appendEvent(std::move(handler));

        QmlEvent frame;
        frame.setTypeIndex(animationTypeId);
        frame.setTimestamp(timestamp += 10000000);
        frame.setNumbers({20, 1, 0});
        manager.appendEvent(std::move(frame));
    }

    // Everything below happens after the last frame, so it is no part of any frame.

    // One call that holds the thread for a fifth of a second.
    {
        QmlEvent event;
        event.setTypeIndex(blockingHandlerTypeId);
        event.setRangeStage(RangeStart);
        event.setTimestamp(++timestamp);
        manager.appendEvent(QmlEvent(event));

        timestamp += 200000000;
        event.setRangeStage(RangeEnd);
        event.setTimestamp(timestamp);
        manager.appendEvent(std::move(event));
    }

    // A delegate built once per item of a long list.
    for (int i = 0; i < 300; ++i) {
        QmlEvent event;
        event.setTypeIndex(delegateTypeId);
        event.setRangeStage(RangeStart);
        event.setTimestamp(++timestamp);
        manager.appendEvent(QmlEvent(event));

        timestamp += 100000;
        event.setRangeStage(RangeEnd);
        event.setTimestamp(timestamp);
        manager.appendEvent(std::move(event));
    }

    // A binding too cheap to show up in any cost, re-evaluated without end.
    for (int i = 0; i < 1200; ++i) {
        QmlEvent event;
        event.setTypeIndex(churnBindingTypeId);
        event.setRangeStage(RangeStart);
        event.setTimestamp(++timestamp);
        manager.appendEvent(QmlEvent(event));

        event.setRangeStage(RangeEnd);
        event.setTimestamp(++timestamp);
        manager.appendEvent(std::move(event));
    }

    // A function taking two megabytes from the JavaScript heap while it runs.
    {
        QmlEvent js;
        js.setTypeIndex(allocatingJsTypeId);
        js.setRangeStage(RangeStart);
        js.setTimestamp(++timestamp);
        manager.appendEvent(QmlEvent(js));

        for (int i = 0; i < 4; ++i) {
            QmlEvent allocation;
            allocation.setTypeIndex(memoryTypeId);
            allocation.setTimestamp(++timestamp);
            allocation.setNumbers({qint64(512 * 1024)});
            manager.appendEvent(std::move(allocation));
        }

        js.setRangeStage(RangeEnd);
        js.setTimestamp(++timestamp);
        manager.appendEvent(std::move(js));
    }

    // The same image loaded four times: nothing holds it between uses.
    for (int i = 0; i < 4; ++i) {
        QmlEvent started;
        started.setTypeIndex(reloadStartTypeId);
        started.setTimestamp(++timestamp);
        manager.appendEvent(std::move(started));

        QmlEvent finished;
        finished.setTypeIndex(reloadFinishTypeId);
        finished.setTimestamp(timestamp += 2000000);
        manager.appendEvent(std::move(finished));
    }

    manager.finalize();
}

void QmlProfilerFindingsModelTest::testSlowCompileReported()
{
    const QList<Finding> findings = model.findings();
    const auto it = std::find_if(findings.begin(), findings.end(), [](const Finding &finding) {
        return finding.ruleId == QLatin1String("first-use-compile");
    });
    QVERIFY(it != findings.end());
    QCOMPARE(it->location.filename(), QString("Main.qml"));
    QCOMPARE(it->costNs, slowCompileNs);
    QCOMPARE(it->occurrences, 1);
    QCOMPARE(it->typeIndex, slowCompileTypeId);
    QVERIFY(!it->why.isEmpty());
    QVERIFY(!it->suggestion.isEmpty());
}

void QmlProfilerFindingsModelTest::testFastCompileIgnored()
{
    for (const Finding &finding : model.findings())
        QVERIFY(finding.location.filename() != QString("Small.qml"));
}

void QmlProfilerFindingsModelTest::testPixmapLoadErrorsAggregated()
{
    const QList<Finding> findings = model.findings();
    const auto it = std::find_if(findings.begin(), findings.end(), [](const Finding &finding) {
        return finding.ruleId == QLatin1String("pixmap-load-error");
    });
    QVERIFY(it != findings.end());
    QCOMPARE(it->severity, Finding::Critical);
    QCOMPARE(it->occurrences, 3);
    QCOMPARE(it->location.filename(), QString("image://provider/missing.svg"));
}

void QmlProfilerFindingsModelTest::testLocationWithoutLine()
{
    // Pixmap events and Compiling ranges have no line; the trace reports 0 for both. The
    // location column must not append that as if it were a real line number.
    for (int row = 0, rowCount = model.rowCount(); row < rowCount; ++row) {
        const QString location
            = model.data(model.index(row, QmlProfilerFindingsModel::ColumnLocation)).toString();
        QVERIFY(!location.isEmpty());
        QVERIFY(!location.endsWith(QLatin1String(":0")));
    }
}

static const Finding *findingFor(const QList<Finding> &findings, const char *ruleId)
{
    for (const Finding &finding : findings) {
        if (finding.ruleId == QLatin1String(ruleId))
            return &finding;
    }
    return nullptr;
}

void QmlProfilerFindingsModelTest::testSyncViewLoadAttributedToHandler()
{
    const Finding *finding = findingFor(model.findings(), "sync-view-load");
    QVERIFY(finding);
    // Reported where the fix belongs: the handler that waited, not the item it built.
    QCOMPARE(finding->location.filename(), QString("Panel.qml"));
    QCOMPARE(finding->location.line(), 42);
    QCOMPARE(finding->costNs, 30000000);
    QCOMPARE(finding->occurrences, 1);
}

void QmlProfilerFindingsModelTest::testPeriodicHandlerReported()
{
    const Finding *finding = findingFor(model.findings(), "periodic-handler");
    QVERIFY(finding);
    QCOMPARE(finding->location.filename(), QString("Overlay.qml"));
    QCOMPARE(finding->occurrences, 60);
    QVERIFY(finding->what.contains(QString("200")));
}

void QmlProfilerFindingsModelTest::testOversizedPixmapReported()
{
    const Finding *finding = findingFor(model.findings(), "oversized-pixmap");
    QVERIFY(finding);
    QCOMPARE(finding->location.filename(), QString("qrc:/huge.png"));
    QVERIFY(finding->what.contains(QString("4000")));
}

void QmlProfilerFindingsModelTest::testPerFrameCostReported()
{
    const Finding *finding = findingFor(model.findings(), "per-frame-cost");
    QVERIFY(finding);
    QCOMPARE(finding->location.filename(), QString("Gauge.qml"));
    // The frames of one thread, although both of them reported every one of the first 100.
    QCOMPARE(finding->occurrences, 105); // frames the cost was spread over
    QCOMPARE(finding->costNs, 100000000);
}

void QmlProfilerFindingsModelTest::testBlockingCallReported()
{
    const Finding *finding = findingFor(model.findings(), "blocking-call");
    QVERIFY(finding);
    QCOMPARE(finding->location.filename(), QString("Import.qml"));
    QCOMPARE(finding->costNs, 200000000);
    QCOMPARE(finding->occurrences, 1);
}

void QmlProfilerFindingsModelTest::testFrameJankAttributedToHandler()
{
    const Finding *finding = findingFor(model.findings(), "frame-jank");
    QVERIFY(finding);
    // The handler that ran in the late frames, not the binding that runs in every frame.
    QCOMPARE(finding->location.filename(), QString("Chart.qml"));
    QCOMPARE(finding->occurrences, 5);
    QCOMPARE(finding->costNs, 200000000);
}

void QmlProfilerFindingsModelTest::testMemoryChurnAttributedToCaller()
{
    const Finding *finding = findingFor(model.findings(), "memory-churn");
    QVERIFY(finding);
    // Reported where the memory was taken, not at the allocation event, which has no
    // location of its own.
    QCOMPARE(finding->location.filename(), QString("Report.qml"));
    QCOMPARE(finding->location.line(), 55);
    QCOMPARE(finding->occurrences, 4);
}

void QmlProfilerFindingsModelTest::testRepeatedCreationReported()
{
    const Finding *finding = findingFor(model.findings(), "repeated-creation");
    QVERIFY(finding);
    QCOMPARE(finding->location.filename(), QString("Row.qml"));
    QCOMPARE(finding->occurrences, 300);
    QCOMPARE(finding->costNs, 30000000);
}

void QmlProfilerFindingsModelTest::testBindingChurnReported()
{
    const Finding *finding = findingFor(model.findings(), "binding-churn");
    QVERIFY(finding);
    // The binding is reported for how often it runs, although it costs next to nothing.
    QCOMPARE(finding->location.filename(), QString("Ticker.qml"));
    QCOMPARE(finding->occurrences, 1200);
}

void QmlProfilerFindingsModelTest::testPixmapReloadReported()
{
    const Finding *finding = findingFor(model.findings(), "pixmap-reload");
    QVERIFY(finding);
    QCOMPARE(finding->location.filename(), QString("qrc:/tile.png"));
    QCOMPARE(finding->occurrences, 4);
    QCOMPARE(finding->costNs, 8000000);

    // The image that never loads started as many loads as the threshold asks for, but it
    // is reported as an error, not as a reload.
    for (const Finding &other : model.findings()) {
        if (other.ruleId == QLatin1String("pixmap-reload"))
            QVERIFY(other.location.filename() != QString("image://provider/missing.svg"));
    }
}

void QmlProfilerFindingsModelTest::testSeverityOrdering()
{
    // Critical findings sort before the rest, whatever else was found.
    QCOMPARE(model.rowCount(), model.findings().count());
    QVERIFY(model.rowCount() >= 6);
    QCOMPARE(model.data(model.index(0, QmlProfilerFindingsModel::ColumnOccurrences),
                        QmlProfilerFindingsModel::RuleIdRole).toString(),
             QString("pixmap-load-error"));
}

void QmlProfilerFindingsModelTest::testJsonExport()
{
    const QJsonObject json = findingsToJson(model.findings(), 100, 900);
    QCOMPARE(json.value("version").toInt(), 1);
    QCOMPARE(json.value("traceStartNs").toInteger(), 100);
    QCOMPARE(json.value("traceEndNs").toInteger(), 900);

    const QJsonArray findings = json.value("findings").toArray();
    QCOMPARE(findings.count(), model.findings().count());

    bool sawPositioned = false;
    bool sawUnpositioned = false;
    for (const QJsonValue &value : findings) {
        const QJsonObject object = value.toObject();
        QVERIFY(!object.value("ruleId").toString().isEmpty());
        QVERIFY(!object.value("what").toString().isEmpty());
        QVERIFY(!object.value("suggestion").toString().isEmpty());
        QVERIFY(!object.value("file").toString().isEmpty());

        const QString severity = object.value("severity").toString();
        QVERIFY(severity == QLatin1String("critical") || severity == QLatin1String("warning")
                || severity == QLatin1String("info"));

        // A position is reported only where the trace has one.
        if (object.value("ruleId").toString() == QLatin1String("sync-view-load")) {
            QCOMPARE(object.value("line").toInt(), 42);
            sawPositioned = true;
        } else if (object.value("ruleId").toString() == QLatin1String("pixmap-load-error")) {
            QVERIFY(!object.contains("line"));
            QVERIFY(!object.contains("column"));
            sawUnpositioned = true;
        }
    }
    QVERIFY(sawPositioned);
    QVERIFY(sawUnpositioned);
}

void QmlProfilerFindingsModelTest::testClear()
{
    model.clear();
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(model.findings().isEmpty());
}

} // namespace Profiler::Internal
