// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerrangemodel_test.h"

#include "../qmlprofilermodelmanager.h"
#include "../qmlprofilerrangemodel.h"

#include <tracing/timelinemodelaggregator.h>

#include <QTest>

using namespace QmlDebug;

namespace Profiler::Internal {

static int appendBindingType(QmlProfilerModelManager &manager, int line)
{
    return manager.appendEventType(QmlEventType(UndefinedMessage, Binding, -1,
                                                QmlEventLocation("main.qml", line, 1),
                                                QString("b%1").arg(line)));
}

static void appendRange(QmlProfilerModelManager &manager, int type, qint64 start, qint64 end)
{
    QmlEvent event(start, type, {});
    event.setRangeStage(RangeStart);
    manager.appendEvent(QmlEvent(event));
    event.setRangeStage(RangeEnd);
    event.setTimestamp(end);
    manager.appendEvent(std::move(event));
}

// Expanded, each type has a row of its own, below the header row, in the order the types first
// appear. Collapsed, a range is in the row of its nesting level.
void QmlProfilerRangeModelTest::testRows()
{
    QmlProfilerModelManager manager;
    Timeline::TimelineModelAggregator aggregator;
    QmlProfilerRangeModel model(&manager, Binding, &aggregator);
    manager.initialize();
    const int a = appendBindingType(manager, 1);
    const int b = appendBindingType(manager, 2);
    const int c = appendBindingType(manager, 3);
    // The events of a range arrive in the order they happen.
    const auto stage = [&](int type, qint64 time, Message rangeStage) {
        QmlEvent e(time, type, {});
        e.setRangeStage(rangeStage);
        manager.appendEvent(std::move(e));
    };
    stage(b, 0, RangeStart);
    stage(a, 10, RangeStart);
    stage(b, 20, RangeStart);
    stage(b, 30, RangeEnd);
    stage(a, 50, RangeEnd);
    stage(b, 100, RangeEnd);
    appendRange(manager, c, 200, 210);
    appendRange(manager, a, 300, 310);
    manager.finalize();

    QCOMPARE(model.count(), 5);
    QCOMPARE(model.expandedRowCount(), 4);
    QCOMPARE(model.collapsedRowCount(), 3);
    const QList<int> types = {b, a, b, c, a};
    const QList<int> expandedRows = {1, 2, 1, 3, 2};
    const QList<int> collapsedRows = {0, 1, 2, 0, 0};
    for (int i = 0; i < model.count(); ++i) {
        QCOMPARE(model.typeId(i), types.at(i));
        QCOMPARE(model.expandedRow(i), expandedRows.at(i));
        QCOMPARE(model.collapsedRow(i), collapsedRows.at(i));
    }
}

// Nesting levels above what a byte holds keep their row.
void QmlProfilerRangeModelTest::testDeepNesting()
{
    const int depth = 600;
    QmlProfilerModelManager manager;
    Timeline::TimelineModelAggregator aggregator;
    QmlProfilerRangeModel model(&manager, Binding, &aggregator);
    manager.initialize();
    const int type = appendBindingType(manager, 1);
    for (int level = 0; level < depth; ++level) {
        QmlEvent event(level, type, {});
        event.setRangeStage(RangeStart);
        manager.appendEvent(std::move(event));
    }
    for (int level = depth - 1; level >= 0; --level) {
        QmlEvent event(2 * depth - level, type, {});
        event.setRangeStage(RangeEnd);
        manager.appendEvent(std::move(event));
    }
    appendRange(manager, type, 3 * depth, 3 * depth + 1);
    manager.finalize();

    QCOMPARE(model.count(), depth + 1);
    QCOMPARE(model.collapsedRowCount(), depth);
    for (int i = 0; i < depth; ++i)
        QCOMPARE(model.collapsedRow(i), i);
    QCOMPARE(model.collapsedRow(depth), 0);
}

// While events are still coming in, every range is in the first row of either view.
void QmlProfilerRangeModelTest::testRowsBeforeFinalize()
{
    QmlProfilerModelManager manager;
    Timeline::TimelineModelAggregator aggregator;
    QmlProfilerRangeModel model(&manager, Binding, &aggregator);
    manager.initialize();
    const int type = appendBindingType(manager, 1);
    appendRange(manager, type, 0, 100);
    appendRange(manager, type, 10, 20);

    QCOMPARE(model.count(), 2);
    for (int i = 0; i < model.count(); ++i) {
        QCOMPARE(model.expandedRow(i), 1);
        QCOMPARE(model.collapsedRow(i), Timeline::kCollapsedStartRow);
    }
}

} // namespace Profiler::Internal
