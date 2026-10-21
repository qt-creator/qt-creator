// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerstatisticsmodel_test.h"

#include <profiler/qmlprofilermodelmanager.h>
#include <profiler/qmlprofilerstatisticsmodel.h>

#include <QTest>

namespace Profiler::Internal {

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
