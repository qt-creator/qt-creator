// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinezoomcontrol.h>

#include <QTest>

using namespace Timeline;

class tst_ZoomControlState : public QObject
{
    Q_OBJECT

private slots:
    void startEndDuration();
    void windowAfterTrace();
    void rangeConstraints();
    void rangeClampedToTrace();
};

void tst_ZoomControlState::startEndDuration()
{
    TimelineZoomControl zoom;
    zoom.setTrace(1, 11);
    QCOMPARE(zoom.traceStart(), 1);
    QCOMPARE(zoom.traceEnd(), 11);
    QCOMPARE(zoom.traceDuration(), 10);
}

void tst_ZoomControlState::windowAfterTrace()
{
    TimelineZoomControl zoom;
    zoom.setTrace(1, 11);
    QVERIFY(zoom.windowStart() >= zoom.traceStart());
    QVERIFY(zoom.windowEnd() <= zoom.traceEnd());
}

void tst_ZoomControlState::rangeConstraints()
{
    TimelineZoomControl zoom;
    zoom.setTrace(1, 11);
    zoom.setRange(3, 7);
    QCOMPARE(zoom.rangeStart(), 3);
    QCOMPARE(zoom.rangeEnd(), 7);
}

void tst_ZoomControlState::rangeClampedToTrace()
{
    TimelineZoomControl zoom;
    zoom.setTrace(1, 11);
    zoom.setRange(-100, 200);
    QCOMPARE(zoom.rangeStart(), -100);
    QCOMPARE(zoom.rangeEnd(), 200);
}

QTEST_GUILESS_MAIN(tst_ZoomControlState)

#include "tst_zoomcontrolstate.moc"
