// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerstatisticsmodel_test.h"

#include <profiler/qmlprofilermodelmanager.h>
#include <profiler/qmlprofilerstatisticsmodel.h>

#include <QTest>

#include <limits>

namespace Profiler::Internal {

using Stats = QmlProfilerStatisticsModel::QmlEventStats;

void QmlProfilerStatisticsModelTest::testFinalizeStats_data()
{
    QTest::addColumn<QList<qint64>>("durations");
    QTest::addColumn<qint64>("minimum");
    QTest::addColumn<qint64>("maximum");
    QTest::addColumn<qint64>("median");

    QTest::newRow("one") << QList<qint64>{7} << 7ll << 7ll << 7ll;
    QTest::newRow("odd") << QList<qint64>{5, 1, 9, 3, 7} << 1ll << 9ll << 5ll;
    QTest::newRow("even") << QList<qint64>{8, 2, 6, 4, 10, 1} << 1ll << 10ll << 5ll;
    QTest::newRow("even, duplicates") << QList<qint64>{3, 3, 1, 3} << 1ll << 3ll << 3ll;
    const qint64 max = std::numeric_limits<qint64>::max();
    QTest::newRow("even, near overflow") << QList<qint64>{max, max - 2} << max - 2 << max
                                         << max - 1;
}

void QmlProfilerStatisticsModelTest::testFinalizeStats()
{
    QFETCH(QList<qint64>, durations);
    QFETCH(qint64, minimum);
    QFETCH(qint64, maximum);
    QFETCH(qint64, median);

    Stats stats;
    stats.durations.assign(durations.cbegin(), durations.cend());
    stats.finalize();

    QCOMPARE(stats.calls, durations.size());
    QCOMPARE(stats.minimum, minimum);
    QCOMPARE(stats.maximum, maximum);
    QCOMPARE(stats.median, median);
}

void QmlProfilerStatisticsModelTest::testFinalizeWithoutCalls()
{
    Stats stats;
    stats.finalize();

    QCOMPARE(stats.calls, 0);
    QCOMPARE(stats.minimum, 0);
    QCOMPARE(stats.maximum, 0);
    QCOMPARE(stats.median, 0);
}

void QmlProfilerStatisticsModelTest::testFinalizeReleasesDurations()
{
    Stats stats;
    for (qint64 duration = 0; duration < 1000; ++duration)
        stats.durations.push_back(duration);
    QVERIFY(stats.durations.capacity() >= 1000);

    stats.finalize();

    QCOMPARE(stats.calls, 1000);
    QVERIFY(stats.durations.empty());
    QCOMPARE(stats.durations.capacity(), 0u);
}

// A range filter, or a trace that starts in the middle of a range, can hand the statistics an
// end without a start. It has to be ignored, also by the callers and callees.
void QmlProfilerStatisticsModelTest::testRangeEndWithoutStart()
{
    using namespace QmlDebug;
    QmlProfilerModelManager manager;
    QmlProfilerStatisticsModel statistics(&manager);
    QmlProfilerStatisticsRelativesModel callees(&manager, &statistics,
                                                QmlProfilerStatisticsCallees);
    QmlProfilerStatisticsRelativesModel callers(&manager, &statistics,
                                                QmlProfilerStatisticsCallers);
    manager.initialize();
    const int typeIndex = manager.appendEventType(
        QmlEventType(UndefinedMessage, Javascript, -1, QmlEventLocation("main.qml", 1, 1), "f"));

    QmlEvent event(5, typeIndex, {});
    event.setRangeStage(RangeEnd);
    manager.appendEvent(QmlEvent(event));
    event.setRangeStage(RangeStart);
    event.setTimestamp(10);
    manager.appendEvent(QmlEvent(event));
    event.setRangeStage(RangeEnd);
    event.setTimestamp(30);
    manager.appendEvent(std::move(event));
    manager.decreaseTraceStart(0);
    manager.increaseTraceEnd(100);
    manager.finalize();

    QCOMPARE(statistics.data(statistics.index(typeIndex, MainCallCount), Qt::DisplayRole).toInt(),
             1);
    QCOMPARE(statistics.data(statistics.index(typeIndex, MainTotalTime), SortRole).toLongLong(),
             20);
}

} // namespace Profiler::Internal
