// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerfindingsmodel_test.h"

#include "../profilertr.h"
#include "../qmlprofilersettings.h"

#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopeGuard>
#include <QTest>

#include <algorithm>
#include <iterator>

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

    const auto addType = [&](RangeType rangeType, const char *file, int line, int column,
                             const char *data = "") {
        const int typeId = manager.numEventTypes();
        manager.appendEventType(QmlEventType(UndefinedMessage, rangeType, -1,
                                             QmlEventLocation(QString::fromLatin1(file), line,
                                                              column),
                                             QString::fromLatin1(data)));
        return typeId;
    };

    // A ListView with a delegate written inline. The delegate's Text has an expensive
    // binding, and a cheap one deciding whether it is visible.
    viewTypeId = addType(Creating, "View.qml", 10, 5, "QtQuick/ListView");
    viewComponentTypeId = addType(Creating, "View.qml", 14, 19, "<component>");
    viewDelegateTypeId = addType(Creating, "View.qml", 14, 19, "QtQuick/Rectangle");
    viewTextTypeId = addType(Creating, "View.qml", 16, 9, "QtQuick/Text");
    viewTextBindingTypeId = addType(Binding, "View.qml", 17, 19);
    viewTextJsTypeId = addType(Javascript, "View.qml", 17, 19, "expression for text");
    viewVisibleBindingTypeId = addType(Binding, "View.qml", 18, 22);
    viewVisibleJsTypeId = addType(Javascript, "View.qml", 18, 22, "expression for visible");
    outsideBindingTypeId = addType(Binding, "Outside.qml", 5, 9);

    // A Repeater whose delegate is a component of its own file.
    repeaterTypeId = addType(Creating, "List.qml", 20, 5, "QtQuick/Repeater");
    repeaterComponentTypeId = addType(Creating, "List.qml", 22, 19, "<component>");
    wrapperTypeId = addType(Creating, "List.qml", 22, 19, "Delegate");
    fileRootTypeId = addType(Creating, "Delegate.qml", 3, 1, "QtQuick/Rectangle");
    fileBindingTypeId = addType(Binding, "Delegate.qml", 6, 12);

    // Blocks created from JavaScript, the way SameGame does it.
    gameHandlerTypeId = addType(HandlingSignal, "Main.qml", 114, 21);
    gameHandlerJsTypeId = addType(Javascript, "Main.qml", 114, 21, "expression for onTriggered");
    createBlockTypeId = addType(Javascript, "game.js", 321, 1, "createBlock");
    blockTypeId = addType(Creating, "Block.qml", 7, 1, "QtQuick/Item");
    blockImageTypeId = addType(Creating, "Block.qml", 22, 5, "QtQuick/Image");
    blockStateTypeId = addType(Creating, "Block.qml", 65, 9, "QtQuick/State");
    blockAnimationTypeId = addType(Creating, "Block.qml", 19, 13, "QtQuick/SpringAnimation");
    blockChangesTypeId = addType(Creating, "Block.qml", 67, 13, "QtQuick/PropertyChanges");
    blockSourceBindingTypeId = addType(Binding, "Block.qml", 24, 17);
    blockSourceJsTypeId = addType(Javascript, "Block.qml", 24, 17, "expression for source");
    blockCompletedTypeId = addType(HandlingSignal, "Block.qml", 30, 5);
    blockCompletedJsTypeId = addType(Javascript, "Block.qml", 30, 5, "expression for onCompleted");
    blockAfterBindingTypeId = addType(Binding, "Block.qml", 66, 15);
    blockAfterJsTypeId = addType(Javascript, "Block.qml", 66, 15, "expression for when");

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

    // Nested ranges, written in trace order. Only closing a range moves time on, so the
    // duration of a range is the sum of what is closed inside it.
    const auto begin = [&](int typeId) {
        QmlEvent event;
        event.setTypeIndex(typeId);
        event.setRangeStage(RangeStart);
        event.setTimestamp(timestamp);
        manager.appendEvent(std::move(event));
    };
    const auto end = [&](int typeId, qint64 durationNs) {
        QmlEvent event;
        event.setTypeIndex(typeId);
        event.setRangeStage(RangeEnd);
        event.setTimestamp(timestamp += durationNs);
        manager.appendEvent(std::move(event));
    };

    // The engine reports each object in two ranges of the same type: one when it is
    // created, nested in its parent's, and one when it is completed, after the bindings of
    // the whole instance have run. An object without QQmlParserStatus is not completed.

    // 78 us per delegate. The Outside.qml binding runs while the delegate is completed,
    // but it is not the delegate's: it costs time, but is no part to make cheaper.
    const auto addViewDelegate = [&] {
        begin(viewDelegateTypeId);
        begin(viewTextTypeId);
        end(viewTextTypeId, 2000);
        end(viewDelegateTypeId, 3000);

        begin(viewTextBindingTypeId);
        begin(viewTextJsTypeId);
        end(viewTextJsTypeId, 50000);
        end(viewTextBindingTypeId, 10000);
        begin(viewVisibleBindingTypeId);
        begin(viewVisibleJsTypeId);
        end(viewVisibleJsTypeId, 1000);
        end(viewVisibleBindingTypeId, 0);

        begin(viewTextTypeId);
        end(viewTextTypeId, 4000);
        begin(viewDelegateTypeId);
        begin(outsideBindingTypeId);
        end(outsideBindingTypeId, 7000);
        end(viewDelegateTypeId, 1000);
    };

    // The view creates its delegate component, fills itself while it is completed, and
    // builds the rest of its delegates later, when nothing in the trace says who asked.
    begin(viewTypeId);
    begin(viewComponentTypeId);
    end(viewComponentTypeId, 1000);
    end(viewTypeId, 5000);
    begin(viewTypeId);
    for (int i = 0; i < 10; ++i)
        addViewDelegate();
    end(viewTypeId, 1000);
    for (int i = 0; i < 240; ++i)
        addViewDelegate();

    // 7 us per delegate. The composite type wrapping Delegate.qml's root is created, but
    // only that root is completed.
    begin(repeaterTypeId);
    begin(repeaterComponentTypeId);
    end(repeaterComponentTypeId, 1000);
    end(repeaterTypeId, 2000);
    begin(repeaterTypeId);
    for (int i = 0; i < 220; ++i) {
        begin(wrapperTypeId);
        begin(fileRootTypeId);
        end(fileRootTypeId, 3000);
        end(wrapperTypeId, 1000);
        begin(fileBindingTypeId);
        end(fileBindingTypeId, 2000);
        begin(fileRootTypeId);
        end(fileRootTypeId, 1000);
    }
    end(repeaterTypeId, 1000);

    // Three games of 70 blocks, 100.7 us each, created by a JavaScript function called from
    // a timer's handler. The State is not completed. Component.onCompleted runs after the
    // last completion and is part of building the block; the binding that runs once the
    // function sets the block's position is not, but the objects it creates for the block
    // are.
    for (int game = 0; game < 3; ++game) {
        begin(gameHandlerTypeId);
        begin(gameHandlerJsTypeId);
        for (int i = 0; i < 70; ++i) {
            begin(createBlockTypeId);

            begin(blockTypeId);
            begin(blockStateTypeId);
            end(blockStateTypeId, 500);
            begin(blockImageTypeId);
            end(blockImageTypeId, 1500);
            end(blockTypeId, 2000);

            begin(blockSourceBindingTypeId);
            begin(blockSourceJsTypeId);
            end(blockSourceJsTypeId, 30000);
            end(blockSourceBindingTypeId, 5000);

            begin(blockImageTypeId);
            end(blockImageTypeId, 50000);
            begin(blockTypeId);
            end(blockTypeId, 1000);

            // A Behavior's animation, created and completed once the block is.
            begin(blockAnimationTypeId);
            end(blockAnimationTypeId, 400);
            begin(blockAnimationTypeId);
            end(blockAnimationTypeId, 100);

            begin(blockCompletedTypeId);
            begin(blockCompletedJsTypeId);
            end(blockCompletedJsTypeId, 8000);
            end(blockCompletedTypeId, 2000);

            // The binding applies a State, which creates its changes only now.
            begin(blockAfterBindingTypeId);
            begin(blockAfterJsTypeId);
            end(blockAfterJsTypeId, 3000);
            begin(blockChangesTypeId);
            end(blockChangesTypeId, 200);
            end(blockAfterBindingTypeId, 0);

            end(createBlockTypeId, 1000);
        }
        end(gameHandlerJsTypeId, 0);
        end(gameHandlerTypeId, 0);
    }

    // A delegate built once per item of a long list, 100 us each, by C++: nothing else is
    // running when it is.
    for (int i = 0; i < 300; ++i) {
        begin(delegateTypeId);
        end(delegateTypeId, 60000);
        begin(delegateTypeId);
        end(delegateTypeId, 40000);
    }
    ++timestamp;

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

static const Finding *findingAt(const QList<Finding> &findings, const char *ruleId,
                               const QString &filename)
{
    for (const Finding &finding : findings) {
        if (finding.ruleId == QLatin1String(ruleId) && finding.location.filename() == filename)
            return &finding;
    }
    return nullptr;
}

static int countFindings(const QList<Finding> &findings, const char *ruleId,
                         const QString &filename)
{
    return std::count_if(findings.begin(), findings.end(), [&](const Finding &finding) {
        return finding.ruleId == QLatin1String(ruleId) && finding.location.filename() == filename;
    });
}

void QmlProfilerFindingsModelTest::testSyncViewLoadAttributedToHandler()
{
    const Finding *finding = findingAt(model.findings(), "sync-view-load", "Panel.qml");
    QVERIFY(finding);
    // Reported where the fix belongs: the handler that waited, not the item it built.
    QCOMPARE(finding->location.filename(), QString("Panel.qml"));
    QCOMPARE(finding->location.line(), 42);
    QCOMPARE(finding->costNs, 30000000);
    QCOMPARE(finding->occurrences, 1);
}

void QmlProfilerFindingsModelTest::testPeriodicHandlerReported()
{
    const Finding *finding = findingAt(model.findings(), "periodic-handler", "Overlay.qml");
    QVERIFY(finding);
    QCOMPARE(finding->occurrences, 60);
    QVERIFY(finding->what.contains(QString("200")));
}

void QmlProfilerFindingsModelTest::testOversizedPixmapReported()
{
    const Finding *finding = findingAt(model.findings(), "oversized-pixmap", "qrc:/huge.png");
    QVERIFY(finding);
    QCOMPARE(finding->location.filename(), QString("qrc:/huge.png"));
    QVERIFY(finding->what.contains(QString("4000")));
}

void QmlProfilerFindingsModelTest::testPerFrameCostReported()
{
    const Finding *finding = findingAt(model.findings(), "per-frame-cost", "Gauge.qml");
    QVERIFY(finding);
    QCOMPARE(finding->location.filename(), QString("Gauge.qml"));
    // The frames of one thread, although both of them reported every one of the first 100.
    QCOMPARE(finding->occurrences, 105); // frames the cost was spread over
    QCOMPARE(finding->costNs, 100000000);
}

void QmlProfilerFindingsModelTest::testBlockingCallReported()
{
    const Finding *finding = findingAt(model.findings(), "blocking-call", "Import.qml");
    QVERIFY(finding);
    QCOMPARE(finding->location.filename(), QString("Import.qml"));
    QCOMPARE(finding->costNs, 200000000);
    QCOMPARE(finding->occurrences, 1);
}

void QmlProfilerFindingsModelTest::testFrameJankAttributedToHandler()
{
    const Finding *finding = findingAt(model.findings(), "frame-jank", "Chart.qml");
    QVERIFY(finding);
    // The handler that ran in the late frames, not the binding that runs in every frame.
    QCOMPARE(finding->location.filename(), QString("Chart.qml"));
    QCOMPARE(finding->occurrences, 5);
    QCOMPARE(finding->costNs, 200000000);
}

void QmlProfilerFindingsModelTest::testMemoryChurnAttributedToCaller()
{
    const Finding *finding = findingAt(model.findings(), "memory-churn", "Report.qml");
    QVERIFY(finding);
    // Reported where the memory was taken, not at the allocation event, which has no
    // location of its own.
    QCOMPARE(finding->location.filename(), QString("Report.qml"));
    QCOMPARE(finding->location.line(), 55);
    QCOMPARE(finding->occurrences, 4);
}

void QmlProfilerFindingsModelTest::testRepeatedCreationByView()
{
    const QList<Finding> findings = model.findings();
    // The delegate is reported once, at its root. Its Text is part of it, not a component
    // of its own.
    QCOMPARE(countFindings(findings, "repeated-creation", "View.qml"), 1);
    const Finding *finding = findingAt(findings, "repeated-creation", "View.qml");
    QVERIFY(finding);
    QCOMPARE(finding->location.line(), 14);
    QCOMPARE(finding->typeIndex, viewDelegateTypeId);
    // Each delegate is created and completed: two ranges, one instance.
    QCOMPARE(finding->occurrences, 250);
    QCOMPARE(finding->costNs, 250 * 78000);

    // The view is named, both for the delegates it built while it was completed and for
    // those built later, when only the component tells whose they are.
    QVERIFY2(finding->what.contains("ListView at View.qml:10"), qPrintable(finding->what));
    QVERIFY2(finding->suggestion.contains("reuseItems"), qPrintable(finding->suggestion));
    QVERIFY(!finding->details.isEmpty());
    QCOMPARE(finding->details.at(0).typeIndex, viewTypeId);
    QCOMPARE(finding->details.at(0).location.line(), 10);
    QCOMPARE(finding->details.at(0).occurrences, 250);

    // The costliest part is the binding, named by its property, with what it cost.
    QVERIFY2(finding->suggestion.contains("text binding at View.qml:17"),
             qPrintable(finding->suggestion));
    QCOMPARE(finding->details.at(1).typeIndex, viewTextBindingTypeId);
    QCOMPARE(finding->details.at(1).location.line(), 17);
    QCOMPARE(finding->details.at(1).costNs, 250 * 60000);
    QCOMPARE(finding->details.at(1).occurrences, 250);
    QCOMPARE(finding->details.at(2).typeIndex, viewTextTypeId);
    QCOMPARE(finding->details.at(2).costNs, 250 * (2000 + 4000));

    // A binding elsewhere that ran while the delegate was built is not one of its parts.
    for (const FindingDetail &detail : finding->details)
        QVERIFY(detail.location.filename() != QString("Outside.qml"));
    QVERIFY(!finding->suggestion.contains("Outside.qml"));

    // What the trace can say about parts that are not always shown: that their
    // visibility is a binding.
    QVERIFY2(finding->suggestion.contains("visible at View.qml:18"),
             qPrintable(finding->suggestion));
}

void QmlProfilerFindingsModelTest::testRepeatedCreationByRepeaterOfFileDelegate()
{
    const QList<Finding> findings = model.findings();
    QCOMPARE(countFindings(findings, "repeated-creation", "Delegate.qml"), 0);
    const Finding *finding = findingAt(findings, "repeated-creation", "List.qml");
    QVERIFY(finding);
    QCOMPARE(finding->typeIndex, wrapperTypeId);
    // The wrapper of a file's root is never completed, so a new wrapper is a new instance.
    QCOMPARE(finding->occurrences, 220);
    QCOMPARE(finding->costNs, 220 * 7000);
    QVERIFY2(finding->what.contains("Repeater at List.qml:20"), qPrintable(finding->what));
    // A Repeater has no pool to reuse delegates from.
    QVERIFY2(!finding->suggestion.contains("reuseItems: true"), qPrintable(finding->suggestion));
    QVERIFY2(finding->suggestion.contains("ListModel"), qPrintable(finding->suggestion));
}

void QmlProfilerFindingsModelTest::testRepeatedCreationFromScript()
{
    const QList<Finding> findings = model.findings();
    // Objects of the block that are created only when first needed are parts of it, not
    // components of their own.
    QCOMPARE(countFindings(findings, "repeated-creation", "Block.qml"), 1);
    const Finding *finding = findingAt(findings, "repeated-creation", "Block.qml");
    QVERIFY(finding);
    QCOMPARE(finding->location.line(), 7);
    QCOMPARE(finding->occurrences, 210);
    // Building a block includes its bindings, its onCompleted handler and the objects
    // created for it later, but not the binding that runs once createBlock() moves the
    // finished block.
    QCOMPARE(finding->costNs, 210 * (100000 + 500 + 200));

    QVERIFY2(finding->what.contains("createBlock() at game.js:321"), qPrintable(finding->what));
    // No view is involved, so there is no view to ask for reuse.
    QVERIFY2(!finding->why.contains("reuseItems") && !finding->suggestion.contains("reuseItems"),
             qPrintable(finding->suggestion));
    QVERIFY2(finding->suggestion.contains("incubateObject()"), qPrintable(finding->suggestion));

    QVERIFY(!finding->details.isEmpty());
    QCOMPARE(finding->details.at(0).typeIndex, createBlockTypeId);
    QCOMPARE(finding->details.at(0).location.filename(), QString("game.js"));
    QCOMPARE(finding->details.at(0).location.line(), 321);

    QVERIFY2(finding->suggestion.contains("Image at Block.qml:22"),
             qPrintable(finding->suggestion));
    QVERIFY2(finding->suggestion.contains("source binding at Block.qml:24"),
             qPrintable(finding->suggestion));
    QVERIFY2(finding->suggestion.contains("onCompleted handler at Block.qml:30"),
             qPrintable(finding->suggestion));
    QVERIFY(!finding->suggestion.contains("Block.qml:66"));

    const auto source = std::find_if(finding->details.begin(), finding->details.end(),
                                     [this](const FindingDetail &detail) {
        return detail.typeIndex == blockSourceBindingTypeId;
    });
    QVERIFY(source != finding->details.end());
    QCOMPARE(source->costNs, 210 * 35000);
    QCOMPARE(source->occurrences, 210);
}

void QmlProfilerFindingsModelTest::testRepeatedCreationWithoutContext()
{
    const Finding *finding = findingAt(model.findings(), "repeated-creation", "Row.qml");
    QVERIFY(finding);
    QCOMPARE(finding->occurrences, 300);
    QCOMPARE(finding->costNs, 30000000);
    // Nothing in the trace says what built these, so nothing is claimed about it.
    QVERIFY2(finding->why.contains("C++"), qPrintable(finding->why));
    QVERIFY(!finding->suggestion.contains("reuseItems"));
    QVERIFY(!finding->suggestion.contains("incubateObject"));
    for (const FindingDetail &detail : finding->details)
        QCOMPARE(detail.location.filename(), QString("Row.qml"));
}

void QmlProfilerFindingsModelTest::testSyncViewLoadNamesWhatItBuilds()
{
    const Finding *finding = findingAt(model.findings(), "sync-view-load", "Main.qml");
    QVERIFY(finding);
    // The handler the interface waited for, not the function that did the creating, and
    // the calls of it rather than the ranges inside them.
    QCOMPARE(finding->typeIndex, gameHandlerTypeId);
    QCOMPARE(finding->occurrences, 3);
    QCOMPARE(finding->costNs, 210 * (100000 + 500 + 200));
    QVERIFY2(finding->suggestion.contains("incubateObject()"), qPrintable(finding->suggestion));

    QCOMPARE(finding->details.size(), 2);
    QCOMPARE(finding->details.at(0).typeIndex, blockTypeId);
    QCOMPARE(finding->details.at(0).occurrences, 210);
    QCOMPARE(finding->details.at(1).typeIndex, createBlockTypeId);
}

void QmlProfilerFindingsModelTest::testDetailsAreChildRows()
{
    int row = 0;
    for (; row < model.rowCount(); ++row) {
        const QModelIndex index = model.index(row, QmlProfilerFindingsModel::ColumnFinding);
        if (index.data(QmlProfilerFindingsModel::RuleIdRole).toString() == "repeated-creation"
            && index.data(QmlProfilerFindingsModel::FilenameRole).toString() == "Block.qml") {
            break;
        }
    }
    QVERIFY(row < model.rowCount());

    const QModelIndex parent = model.index(row, 0);
    const Finding &finding = model.findings().at(row);
    QVERIFY(!finding.details.isEmpty());
    QCOMPARE(model.rowCount(parent), finding.details.size());
    for (int i = 0; i < finding.details.size(); ++i) {
        const QModelIndex child = model.index(i, QmlProfilerFindingsModel::ColumnLocation, parent);
        QVERIFY(child.isValid());
        QCOMPARE(model.parent(child), parent);
        QCOMPARE(model.rowCount(child), 0);
        // Each detail can be jumped to on its own.
        QCOMPARE(child.data(QmlProfilerFindingsModel::FilenameRole).toString(),
                 finding.details.at(i).location.filename());
        QCOMPARE(child.data(QmlProfilerFindingsModel::LineRole).toInt(),
                 finding.details.at(i).location.line());
        QCOMPARE(child.data(QmlProfilerFindingsModel::TypeIdRole).toInt(),
                 finding.details.at(i).typeIndex);
        QVERIFY(findingIsInSource(child));
    }
}

void QmlProfilerFindingsModelTest::testBindingChurnReported()
{
    const Finding *finding = findingAt(model.findings(), "binding-churn", "Ticker.qml");
    QVERIFY(finding);
    // The binding is reported for how often it runs, although it costs next to nothing.
    QCOMPARE(finding->location.filename(), QString("Ticker.qml"));
    QCOMPARE(finding->occurrences, 1200);
}

void QmlProfilerFindingsModelTest::testPixmapReloadReported()
{
    const Finding *finding = findingAt(model.findings(), "pixmap-reload", "qrc:/tile.png");
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
    bool sawDetails = false;
    for (const QJsonValue &value : findings) {
        const QJsonObject object = value.toObject();
        QVERIFY(!object.value("ruleId").toString().isEmpty());
        QVERIFY(!object.value("what").toString().isEmpty());
        QVERIFY(!object.value("suggestion").toString().isEmpty());
        QVERIFY(!object.value("file").toString().isEmpty());

        const QString severity = object.value("severity").toString();
        QVERIFY(severity == QLatin1String("critical") || severity == QLatin1String("warning")
                || severity == QLatin1String("info"));

        if (object.value("ruleId").toString() == QLatin1String("repeated-creation")
            && object.value("file").toString() == QLatin1String("Block.qml")) {
            const QJsonArray details = object.value("details").toArray();
            QVERIFY(!details.isEmpty());
            const QJsonObject creator = details.first().toObject();
            QCOMPARE(creator.value("file").toString(), QString("game.js"));
            QCOMPARE(creator.value("line").toInt(), 321);
            QVERIFY(!creator.value("text").toString().isEmpty());
            sawDetails = true;
        }

        // A position is reported only where the trace has one.
        if (object.value("ruleId").toString() == QLatin1String("sync-view-load")
            && object.value("file").toString() == QLatin1String("Panel.qml")) {
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
    QVERIFY(sawDetails);
}

// As the dashboard and the options page do it: through apply(), which is what announces it.
static bool hasFinding(const QList<Finding> &findings, const char *ruleId)
{
    return std::any_of(findings.cbegin(), findings.cend(), [ruleId](const Finding &finding) {
        return finding.ruleId == QLatin1String(ruleId);
    });
}

static void applyTargetFrameRate(int framesPerSecond)
{
    QmlProfilerSettings &settings = globalSettings();
    settings.targetFrameRate.setVolatileValue(framesPerSecond);
    settings.apply();
}

void QmlProfilerFindingsModelTest::testTargetFrameRateRerates()
{
    const int originalTarget = globalSettings().targetFrameRate();
    const auto restore = qScopeGuard([originalTarget] {
        applyTargetFrameRate(originalTarget);
    });
    const int eventsBefore = manager.numEvents();

    // At 30 FPS a frame has 33 ms: the 50 ms frames are late, but not by two frames, and a
    // binding taking 0.95 ms of every frame stays below 3% of it.
    applyTargetFrameRate(30);
    QVERIFY(!hasFinding(model.findings(), "frame-jank"));
    QVERIFY(!hasFinding(model.findings(), "per-frame-cost"));
    // Rules that do not depend on frames are not lost on the way.
    QVERIFY(findingAt(model.findings(), "blocking-call", "Import.qml"));

    // At 240 FPS a frame has 4.2 ms: the same frames stutter, and the same binding eats
    // more than 3% of each.
    applyTargetFrameRate(240);
    const Finding *jank = findingAt(model.findings(), "frame-jank", "Chart.qml");
    QVERIFY(jank);
    QCOMPARE(jank->occurrences, 5);
    QVERIFY2(jank->what.contains(QString("than 8 ms")), qPrintable(jank->what));
    QVERIFY(findingAt(model.findings(), "per-frame-cost", "Gauge.qml"));
    QCOMPARE(model.rowCount(), model.findings().count());

    // Re-rated from the trace already loaded, not from a new one.
    QCOMPARE(manager.numEvents(), eventsBefore);
}

// Forwards to a rule and counts the events it is shown.
class CountingRule final : public FindingRule
{
public:
    CountingRule(std::unique_ptr<FindingRule> rule, int *events)
        : m_rule(std::move(rule))
        , m_events(events)
    {}

    quint64 features() const override { return m_rule->features(); }
    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        ++*m_events;
        m_rule->loadEvent(event, type);
    }
    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        return m_rule->finalize(manager);
    }
    void clear() override { m_rule->clear(); }
    bool setTargetFrameRate(int framesPerSecond) override
    {
        return m_rule->setTargetFrameRate(framesPerSecond);
    }

private:
    std::unique_ptr<FindingRule> m_rule;
    int *m_events;
};

// The frame-rate dependent rules as they were before they learned to re-rate: they decide
// with a fixed budget while the events go by. An independent account of what the findings
// at a target must be.
class FixedFrameThread
{
public:
    bool accepts(const QmlEvent &event)
    {
        const int thread = event.number<qint32>(2);
        if (m_thread == -1)
            m_thread = thread;
        return thread == m_thread;
    }

private:
    int m_thread = -1;
};

class FixedBudgetPerFrameCostRule final : public FindingRule
{
public:
    FixedBudgetPerFrameCostRule(quint64 features, qint64 budgetNs)
        : m_features(features)
        , m_budgetNs(budgetNs)
    {}

    quint64 features() const override { return m_features; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.message() == Event && type.detailType() == AnimationFrame) {
            if (m_frameThread.accepts(event))
                ++m_frames;
            return;
        }
        if (type.rangeType() != Binding && type.rangeType() != Javascript)
            return;
        QList<qint64> &starts = m_starts[event.typeIndex()];
        if (event.rangeStage() == RangeStart) {
            starts.append(event.timestamp());
        } else if (event.rangeStage() == RangeEnd && !starts.isEmpty()) {
            m_totals[event.typeIndex()] += event.timestamp() - starts.takeLast();
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        if (m_frames == 0)
            return findings;
        for (auto it = m_totals.constBegin(), end = m_totals.constEnd(); it != end; ++it) {
            const qint64 perFrame = it.value() / m_frames;
            if (perFrame < m_budgetNs)
                continue;
            Finding finding;
            finding.ruleId = "per-frame-cost";
            finding.location = manager->eventType(it.key()).location();
            finding.typeIndex = it.key();
            finding.costNs = it.value();
            finding.occurrences = m_frames;
            finding.what = Tr::tr("Costs %1 ms per rendered frame (%2 ms over %n frame(s)).",
                                  nullptr, m_frames)
                               .arg(perFrame / 1000000.0, 0, 'f', 2)
                               .arg(it.value() / 1000000);
            finding.why = Tr::tr("Work of this size, repeated for every frame, leaves less of the "
                                 "frame for everything else that has to happen in it.");
            finding.suggestion = Tr::tr("Look for a value that is recomputed although it did not "
                                        "change, and keep the result instead of deriving it again "
                                        "each frame.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override {}

private:
    const quint64 m_features;
    const qint64 m_budgetNs;
    FixedFrameThread m_frameThread;
    int m_frames = 0;
    QMap<int, QList<qint64>> m_starts;
    QMap<int, qint64> m_totals; // by type index, as the rule reports them
};

class FixedBudgetFrameJankRule final : public FindingRule
{
public:
    FixedBudgetFrameJankRule(quint64 features, qint64 budgetNs)
        : m_features(features)
        , m_budgetNs(budgetNs)
    {}

    quint64 features() const override { return m_features; }

    void loadEvent(const QmlEvent &event, const QmlEventType &type) override
    {
        if (type.message() == Event && type.detailType() == AnimationFrame) {
            if (!m_frameThread.accepts(event))
                return;
            if (m_lastFrameEnd >= 0 && event.timestamp() - m_lastFrameEnd > m_budgetNs) {
                ++m_jankyFrames;
                for (auto it = m_pending.constBegin(), end = m_pending.constEnd(); it != end;
                     ++it) {
                    m_cost[it.key()] += it.value();
                    ++m_frames[it.key()];
                }
            }
            m_lastFrameEnd = event.timestamp();
            m_pending.clear();
            return;
        }
        const RangeType rangeType = type.rangeType();
        if (rangeType != Binding && rangeType != Javascript && rangeType != Creating
            && rangeType != HandlingSignal) {
            return;
        }
        if (event.rangeStage() == RangeStart) {
            m_open.append({event.typeIndex(), event.timestamp()});
        } else if (event.rangeStage() == RangeEnd) {
            for (int i = m_open.size() - 1; i >= 0; --i) {
                if (m_open.at(i).first != event.typeIndex())
                    continue;
                const qint64 start = m_open.at(i).second;
                m_open.resize(i);
                if (m_open.isEmpty())
                    m_pending[event.typeIndex()] += event.timestamp() - start;
                break;
            }
        }
    }

    QList<Finding> finalize(const QmlProfilerModelManager *manager) override
    {
        QList<Finding> findings;
        for (auto it = m_cost.constBegin(), end = m_cost.constEnd(); it != end; ++it) {
            if (it.value() < m_budgetNs)
                continue;
            Finding finding;
            finding.ruleId = "frame-jank";
            finding.location = manager->eventType(it.key()).location();
            finding.typeIndex = it.key();
            finding.costNs = it.value();
            finding.occurrences = m_frames.value(it.key());
            finding.what = Tr::tr("Runs for %1 ms inside %2 of the %n frame(s) that took longer "
                                  "than %3 ms.", nullptr, m_jankyFrames)
                               .arg(it.value() / 1000000).arg(m_frames.value(it.key()))
                               .arg(m_budgetNs / 1000000);
            finding.why = Tr::tr("The frame could not be finished while this was running, so it "
                                 "stayed on screen for as long as the work took. Work that lands "
                                 "in a few frames is seen as a jump, not as a slowdown.");
            finding.suggestion = Tr::tr("Move the work out of the frame it happens in: do it "
                                        "before it is needed, spread it over several frames, or "
                                        "let it run asynchronously and show the result when it "
                                        "arrives.");
            findings.append(finding);
        }
        return findings;
    }

    void clear() override {}

private:
    const quint64 m_features;
    const qint64 m_budgetNs;
    FixedFrameThread m_frameThread;
    QList<std::pair<int, qint64>> m_open;
    QMap<int, qint64> m_pending;
    QMap<int, qint64> m_cost; // by type index, as the rule reports them
    QMap<int, int> m_frames;
    qint64 m_lastFrameEnd = -1;
    int m_jankyFrames = 0;
};

// defaultFindingRules(), with the two rules that depend on the target replaced by the ones
// above. They are the only ones that ask for animation frames.
static FindingRules fixedBudgetRules(int targetFrameRate)
{
    const qint64 budgetNs = 1000000000ll / targetFrameRate;
    FindingRules rules = defaultFindingRules();
    int replaced = 0;
    for (std::unique_ptr<FindingRule> &rule : rules) {
        const quint64 features = rule->features();
        if (!(features & (1ULL << ProfileAnimations)))
            continue;
        ++replaced;
        if (features & (1ULL << ProfileHandlingSignal)) {
            rule = std::make_unique<FixedBudgetFrameJankRule>(features,
                                                              budgetNs * kStutterFrameBudgets);
        } else {
            rule = std::make_unique<FixedBudgetPerFrameCostRule>(
                features, budgetNs * globalSettings().findingsPerFrameCostPercent() / 100);
        }
    }
    QTC_CHECK(replaced == 2);
    return rules;
}

// Everything a finding says, in order, for comparing two lists of them.
static QString describe(const QList<Finding> &findings)
{
    QJsonObject json = findingsToJson(findings, 0, 0);
    QJsonArray typeIndices;
    for (const Finding &finding : findings)
        typeIndices.append(finding.typeIndex);
    json.insert("typeIndices", typeIndices);
    return QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Indented));
}

static QList<Finding> findingsOf(const QList<Finding> &findings, const char *ruleId)
{
    QList<Finding> result;
    std::copy_if(findings.begin(), findings.end(), std::back_inserter(result),
                 [ruleId](const Finding &finding) {
        return finding.ruleId == QLatin1String(ruleId);
    });
    return result;
}

// Frames of every length from 1 ms to 120 ms, one of 2.5 s and some exactly as long as a
// target allows and a nanosecond either side, with handlers, bindings and functions inside
// them, some running across the end of a frame, and a render thread that reports the same
// frames again.
static void loadMixedFrames(QmlProfilerModelManager &manager)
{
    manager.initialize();
    const int frameType = manager.appendEventType(
        QmlEventType(Event, UndefinedRangeType, AnimationFrame));
    QList<int> rangeTypes;
    const auto addTypes = [&](RangeType rangeType, const char *file, int count) {
        for (int i = 0; i < count; ++i) {
            rangeTypes.append(manager.appendEventType(
                QmlEventType(UndefinedMessage, rangeType, -1,
                             QmlEventLocation(QString::fromLatin1(file), i + 1, 1))));
        }
    };
    addTypes(HandlingSignal, "Handlers.qml", 12);
    addTypes(Binding, "Bindings.qml", 24);
    addTypes(Javascript, "Script.js", 12);
    addTypes(Creating, "Delegate.qml", 6);

    quint32 seed = 12345;
    const auto random = [&seed](quint32 bound) {
        seed = seed * 1664525u + 1013904223u;
        return (seed >> 8) % bound;
    };
    const auto range = [&](int typeIndex, qint64 start, qint64 end) {
        QmlEvent event;
        event.setTypeIndex(typeIndex);
        event.setRangeStage(RangeStart);
        event.setTimestamp(start);
        manager.appendEvent(QmlEvent(event));
        event.setRangeStage(RangeEnd);
        event.setTimestamp(end);
        manager.appendEvent(std::move(event));
    };
    const auto frame = [&](qint64 timestamp, int thread) {
        QmlEvent event;
        event.setTypeIndex(frameType);
        event.setTimestamp(timestamp);
        event.setNumbers<qint32>({60, 1, thread});
        manager.appendEvent(std::move(event));
    };

    qint64 time = 1000000;
    frame(time, GuiThread);
    for (int i = 0; i < 3000; ++i) {
        const qint64 gap = i == 1500 ? 2500000000ll : 1000000 + qint64(random(120)) * 1000000
                                                          + random(1000000);
        qint64 cursor = time + 1000;
        const int ranges = int(random(6));
        for (int r = 0; r < ranges && cursor < time + gap - 200000; ++r) {
            const int typeIndex = rangeTypes.at(int(random(quint32(rangeTypes.size()))));
            const qint64 room = time + gap - 100000 - cursor;
            const qint64 duration = 1000 + qint64(random(quint32(qMin<qint64>(room, 40000000))));
            // Sometimes a binding runs inside the outer range.
            if (random(3) == 0 && duration > 10000) {
                QmlEvent outer;
                outer.setTypeIndex(typeIndex);
                outer.setRangeStage(RangeStart);
                outer.setTimestamp(cursor);
                manager.appendEvent(QmlEvent(outer));
                range(rangeTypes.at(12 + int(random(24))), cursor + 1000, cursor + duration / 2);
                outer.setRangeStage(RangeEnd);
                outer.setTimestamp(cursor + duration);
                manager.appendEvent(std::move(outer));
            } else {
                range(typeIndex, cursor, cursor + duration);
            }
            cursor += duration + 1000;
        }
        time += gap;
        // Now and then a handler runs across the end of the frame, and is charged to the
        // next one.
        if (random(10) == 0) {
            QmlEvent across;
            across.setTypeIndex(rangeTypes.at(int(random(12))));
            across.setRangeStage(RangeStart);
            across.setTimestamp(time - 50000);
            manager.appendEvent(QmlEvent(across));
            frame(time, GuiThread);
            frame(time + 1, RenderThread);
            across.setRangeStage(RangeEnd);
            across.setTimestamp(time + 3000000);
            manager.appendEvent(std::move(across));
            time += 3000000;
        } else {
            frame(time, GuiThread);
            frame(time + 1, RenderThread);
        }
    }
    for (const int target : {30, 60, 75, 240}) {
        for (const qint64 offset : {-1, 0, 1}) {
            const qint64 gap = frameBudgetNs(target) * kStutterFrameBudgets + offset;
            range(rangeTypes.at(int(random(12))), time + 1000, time + gap - 1000);
            time += gap;
            frame(time, GuiThread);
        }
    }
    manager.finalize();
}

void QmlProfilerFindingsModelTest::testRerateMatchesReplay()
{
    const int originalTarget = globalSettings().targetFrameRate();
    const auto restore = qScopeGuard([originalTarget] {
        applyTargetFrameRate(originalTarget);
    });
    applyTargetFrameRate(60);

    QmlProfilerModelManager traceManager;
    // Follows the settings, as the dashboard's and the findings view's models do.
    QmlProfilerFindingsModel followed(&traceManager);
    int eventsSeen = 0;
    FindingRules countingRules;
    for (std::unique_ptr<FindingRule> &rule : defaultFindingRules())
        countingRules.push_back(std::make_unique<CountingRule>(std::move(rule), &eventsSeen));
    QmlProfilerFindingsModel counted(&traceManager, std::move(countingRules));
    loadMixedFrames(traceManager);
    QVERIFY(eventsSeen > 0);
    const int eventsAfterLoad = eventsSeen;

    // Rates the trace from scratch, by replaying it to rules that decide as they go.
    QmlProfilerFindingsModel replayed(&traceManager, FindingRules());

    QMap<int, QList<Finding>> byTarget;
    for (const int target : {30, 240, 75, 60}) {
        applyTargetFrameRate(target);
        counted.setTargetFrameRate(target);
        replayed.setRules(fixedBudgetRules(target));

        const QString expected = describe(replayed.findings());
        QCOMPARE(describe(followed.findings()), expected);
        QCOMPARE(describe(counted.findings()), expected);
        QCOMPARE(followed.rowCount(), replayed.rowCount());
        byTarget.insert(target, followed.findings());
    }
    // Re-rated from what the rules collected while loading.
    QCOMPARE(eventsSeen, eventsAfterLoad);

    // The targets differ in their verdicts, or the comparison would show nothing.
    QVERIFY(findingsOf(byTarget.value(30), "per-frame-cost").size()
            < findingsOf(byTarget.value(240), "per-frame-cost").size());
    for (const char *ruleId : {"frame-jank", "per-frame-cost"}) {
        for (const auto &[low, high] : {std::pair(30, 240), std::pair(60, 75)}) {
            QVERIFY2(describe(findingsOf(byTarget.value(low), ruleId))
                         != describe(findingsOf(byTarget.value(high), ruleId)),
                     qPrintable(QString("%1 at %2 and %3").arg(ruleId).arg(low).arg(high)));
        }
    }
}

void QmlProfilerFindingsModelTest::testClear()
{
    model.clear();
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(model.findings().isEmpty());
}

} // namespace Profiler::Internal
