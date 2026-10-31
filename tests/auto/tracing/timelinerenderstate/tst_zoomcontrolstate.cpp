// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <tracing/timelinezoomcontrol.h>

#include <QTest>

using namespace std::chrono;
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
    zoom.setTrace(1ns, 11ns);
    QCOMPARE(zoom.traceStart(), 1ns);
    QCOMPARE(zoom.traceEnd(), 11ns);
    QCOMPARE(zoom.traceDuration(), 10ns);
}

void tst_ZoomControlState::windowAfterTrace()
{
    TimelineZoomControl zoom;
    zoom.setTrace(1ns, 11ns);
    QVERIFY(zoom.windowStart() >= zoom.traceStart());
    QVERIFY(zoom.windowEnd() <= zoom.traceEnd());
}

void tst_ZoomControlState::rangeConstraints()
{
    TimelineZoomControl zoom;
    zoom.setTrace(1ns, 11ns);
    zoom.setRange(3ns, 7ns);
    QCOMPARE(zoom.rangeStart(), 3ns);
    QCOMPARE(zoom.rangeEnd(), 7ns);
}

void tst_ZoomControlState::rangeClampedToTrace()
{
    TimelineZoomControl zoom;
    zoom.setTrace(1ns, 11ns);
    zoom.setRange(-100ns, 200ns);
    QCOMPARE(zoom.rangeStart(), -100ns);
    QCOMPARE(zoom.rangeEnd(), 200ns);
}

QTEST_GUILESS_MAIN(tst_ZoomControlState)

#include "tst_zoomcontrolstate.moc"
