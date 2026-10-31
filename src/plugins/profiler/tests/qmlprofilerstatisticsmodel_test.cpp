// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerstatisticsmodel_test.h"

#include <profiler/qmlprofilermodelmanager.h>
#include <profiler/qmlprofilerstatisticsmodel.h>

#include <QTest>

#include <limits>

using namespace std::chrono;

namespace Profiler::Internal {

using Stats = QmlProfilerStatisticsModel::QmlEventStats;

void QmlProfilerStatisticsModelTest::testFinalizeStats_data()
{
    using Durations = QList<nanoseconds>;
    QTest::addColumn<Durations>("durations");
    QTest::addColumn<nanoseconds>("minimum");
    QTest::addColumn<nanoseconds>("maximum");
    QTest::addColumn<nanoseconds>("median");

    QTest::newRow("one") << Durations{7ns} << 7ns << 7ns << 7ns;
    QTest::newRow("odd") << Durations{5ns, 1ns, 9ns, 3ns, 7ns} << 1ns << 9ns << 5ns;
    QTest::newRow("even") << Durations{8ns, 2ns, 6ns, 4ns, 10ns, 1ns} << 1ns << 10ns << 5ns;
    QTest::newRow("even, duplicates") << Durations{3ns, 3ns, 1ns, 3ns} << 1ns << 3ns << 3ns;
    const nanoseconds max = nanoseconds::max();
    QTest::newRow("even, near overflow") << Durations{max, max - 2ns} << max - 2ns << max
                                         << max - 1ns;
}

void QmlProfilerStatisticsModelTest::testFinalizeStats()
{
    using Durations = QList<nanoseconds>;
    QFETCH(Durations, durations);
    QFETCH(nanoseconds, minimum);
    QFETCH(nanoseconds, maximum);
    QFETCH(nanoseconds, median);

    Stats stats;
    for (const nanoseconds duration : std::as_const(durations))
        stats.durations.push_back(duration);
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
    QCOMPARE(stats.minimum, 0ns);
    QCOMPARE(stats.maximum, 0ns);
    QCOMPARE(stats.median, 0ns);
}

void QmlProfilerStatisticsModelTest::testFinalizeReleasesDurations()
{
    Stats stats;
    for (nanoseconds duration = 0ns; duration < 1000ns; ++duration)
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

    QmlEvent event(5ns, typeIndex, {});
    event.setRangeStage(RangeEnd);
    manager.appendEvent(QmlEvent(event));
    event.setRangeStage(RangeStart);
    event.setTimestamp(10ns);
    manager.appendEvent(QmlEvent(event));
    event.setRangeStage(RangeEnd);
    event.setTimestamp(30ns);
    manager.appendEvent(std::move(event));
    manager.decreaseTraceStart(0ns);
    manager.increaseTraceEnd(100ns);
    manager.finalize();

    QCOMPARE(statistics.data(statistics.index(typeIndex, MainCallCount), Qt::DisplayRole).toInt(),
             1);
    QCOMPARE(statistics.data(statistics.index(typeIndex, MainTotalTime), SortRole).toLongLong(),
             20);
}

} // namespace Profiler::Internal
