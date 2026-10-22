// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertraceview_test.h"

#include <QSignalSpy>
#include <QTest>

namespace Profiler::Internal {

QmlProfilerTraceViewTest::QmlProfilerTraceViewTest()
    : traceView(&modelManager)
{}

void QmlProfilerTraceViewTest::testStateChanges()
{
    // Standard acquire-process-clear work flow
    modelManager.initialize();
    QVERIFY(traceView.isSuspended());
    modelManager.finalize();
    QVERIFY(!traceView.isSuspended());
    modelManager.clearAll();
    QVERIFY(!traceView.isSuspended());

    // Restrict to range
    modelManager.initialize();
    QVERIFY(traceView.isSuspended());
    modelManager.finalize();
    QVERIFY(!traceView.isSuspended());
    // The events are replayed in another thread, and the view waits for them.
    QSignalSpy restricted(&modelManager, &QmlProfilerModelManager::restrictionFinished);
    modelManager.restrictToRange(10, 14);
    QVERIFY(traceView.isSuspended());
    QTRY_COMPARE(restricted.count(), 1);
    QVERIFY(!traceView.isSuspended());
    modelManager.restrictToRange(-1, -1);
    QTRY_COMPARE(restricted.count(), 2);
    QVERIFY(!traceView.isSuspended());
    modelManager.clearAll();
    QVERIFY(!traceView.isSuspended());

    // Abort Acquiring
    modelManager.initialize();
    QVERIFY(traceView.isSuspended());
    modelManager.clearAll();
    QVERIFY(!traceView.isSuspended());
}

} // namespace Profiler::Internal
