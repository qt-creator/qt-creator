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
    for (int i = 0; i < 3; ++i) {
        QmlEvent event;
        event.setTypeIndex(pixmapErrorTypeId);
        event.setTimestamp(++timestamp);
        manager.appendEvent(std::move(event));
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
    QCOMPARE(finding->occurrences, 100); // frames the cost was spread over
    QCOMPARE(finding->costNs, 100000000);
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
