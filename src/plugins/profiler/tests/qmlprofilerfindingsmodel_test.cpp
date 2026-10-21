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

void QmlProfilerFindingsModelTest::testClear()
{
    model.clear();
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(model.findings().isEmpty());
}

} // namespace Profiler::Internal
