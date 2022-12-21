// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilertraceview_test.h"
#include <QtTest>

namespace QmlProfiler {
namespace Internal {

QmlProfilerTraceViewTest::QmlProfilerTraceViewTest(QObject *parent) :
    QObject(parent), traceView(nullptr, nullptr, &modelManager)
{
}

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
    modelManager.restrictToRange(10, 14);
    QVERIFY(!traceView.isSuspended());
    modelManager.restrictToRange(-1, -1);
    QVERIFY(!traceView.isSuspended());
    modelManager.clearAll();
    QVERIFY(!traceView.isSuspended());

    // Abort Acquiring
    modelManager.initialize();
    QVERIFY(traceView.isSuspended());
    modelManager.clearAll();
    QVERIFY(!traceView.isSuspended());
}

} // namespace Internal
} // namespace QmlProfiler
