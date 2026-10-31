// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QColor>
#include <QSignalSpy>
#include <QTest>
#include <tracing/timelinezoomcontrol.h>

using namespace std::chrono;
using namespace Timeline;

class tst_TimelineZoomControl : public QObject
{
    Q_OBJECT
private:
    void verifyWindow(const TimelineZoomControl &zoomControl);

private slots:
    void trace();
    void window();
    void range();
    void selection();
};

void tst_TimelineZoomControl::verifyWindow(const TimelineZoomControl &zoomControl)
{
    QVERIFY(zoomControl.windowStart() <= zoomControl.rangeStart());
    QVERIFY(zoomControl.windowEnd() >= zoomControl.rangeEnd());
    QVERIFY(zoomControl.traceStart() <= zoomControl.windowStart());
    QVERIFY(zoomControl.traceEnd() >= zoomControl.windowEnd());
}

void tst_TimelineZoomControl::trace()
{
    TimelineZoomControl zoomControl;
    QSignalSpy spy(&zoomControl, &TimelineZoomControl::traceChanged);
    QCOMPARE(zoomControl.traceStart(), -1ns);
    QCOMPARE(zoomControl.traceEnd(), -1ns);
    QCOMPARE(zoomControl.traceDuration(), 0ns);

    zoomControl.setTrace(100ns, 200ns);
    QCOMPARE(zoomControl.traceStart(), 100ns);
    QCOMPARE(zoomControl.traceEnd(), 200ns);
    QCOMPARE(zoomControl.traceDuration(), 100ns);
    QCOMPARE(spy.count(), 1);

    zoomControl.clear();
    QCOMPARE(zoomControl.traceStart(), -1ns);
    QCOMPARE(zoomControl.traceEnd(), -1ns);
    QCOMPARE(zoomControl.traceDuration(), 0ns);
    QCOMPARE(spy.count(), 2);
}

void tst_TimelineZoomControl::window()
{
    TimelineZoomControl zoomControl;

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, [&zoomControl] {
        QVERIFY(zoomControl.windowLocked());
        zoomControl.setWindowLocked(false);
    });

    int numWindowChanges = 0;

    connect(&zoomControl, &TimelineZoomControl::windowChanged, this, [&](nanoseconds, nanoseconds) {
        verifyWindow(zoomControl);

        QVERIFY(!timer.isActive());
        if (++numWindowChanges == 9) { // delay a bit during a move
            zoomControl.setWindowLocked(true);
            timer.start(300);
        }
    });

    QCOMPARE(zoomControl.windowStart(), -1ns);
    QCOMPARE(zoomControl.windowEnd(), -1ns);
    QCOMPARE(zoomControl.windowDuration(), 0ns);

    zoomControl.setTrace(100ns, 200ns);
    QCOMPARE(zoomControl.windowStart(), 100ns);
    QCOMPARE(zoomControl.windowEnd(), 200ns);
    QCOMPARE(zoomControl.windowDuration(), 100ns);

    zoomControl.clear();
    QCOMPARE(zoomControl.windowStart(), -1ns);
    QCOMPARE(zoomControl.windowEnd(), -1ns);
    QCOMPARE(zoomControl.windowDuration(), 0ns);

    zoomControl.setTrace(100000ns, 200000ns);
    QCOMPARE(zoomControl.windowStart(), 100000ns);
    QVERIFY(zoomControl.windowEnd() < 110000ns);

    zoomControl.setRange(199995ns, 200000ns); // jump to end
    verifyWindow(zoomControl);
    zoomControl.setRange(100000ns, 100005ns); // jump to start
    verifyWindow(zoomControl);

    zoomControl.setRange(150000ns, 150005ns);
    zoomControl.setRange(152000ns, 152005ns); // move right

    QMetaObject::Connection connection = connect(
                &zoomControl, &TimelineZoomControl::windowMovingChanged, this, [&](bool moving) {
        if (moving)
            return;

        verifyWindow(zoomControl);
        if (zoomControl.windowEnd() == zoomControl.traceEnd()) {
            zoomControl.setRange(104005ns, 104010ns); // jump left
            verifyWindow(zoomControl);
            zoomControl.setRange(102005ns, 102010ns); // make sure it doesn't overrun trace start
        } else if (zoomControl.windowStart() == zoomControl.traceStart()) {
            QCoreApplication::exit();
        } else {
            QVERIFY(zoomControl.rangeStart() - zoomControl.windowStart() ==
                    zoomControl.windowEnd() - zoomControl.rangeEnd());
            if (zoomControl.rangeStart() == 152000ns) {
                zoomControl.setRange(150000ns, 150005ns); // move left
            } else if (zoomControl.rangeStart() == 150000ns) {
                zoomControl.setRange(196990ns, 196995ns); // jump right
                verifyWindow(zoomControl);
                zoomControl.setRange(198990ns, 198995ns); // make sure it doesn't overrun trace end
            }
        }
        verifyWindow(zoomControl);
    });

    QGuiApplication::exec();

    disconnect(connection);

    bool stopDetected = false;
    connect(&zoomControl, &TimelineZoomControl::windowMovingChanged, [&](bool moving) {
        if (!moving) {
            QCOMPARE(stopDetected, false);
            stopDetected = true;
        } else {
            zoomControl.clear();
            QCOMPARE(zoomControl.windowStart(), -1ns);
            QCOMPARE(zoomControl.windowEnd(), -1ns);
            QCOMPARE(zoomControl.rangeStart(), -1ns);
            QCOMPARE(zoomControl.rangeEnd(), -1ns);
            zoomControl.clear(); // no second windowMovingChanged(false), please
        }
    });

    zoomControl.setRange(180010ns, 180015ns);
}

void tst_TimelineZoomControl::range()
{
    TimelineZoomControl zoomControl;
    QSignalSpy spy(&zoomControl, &TimelineZoomControl::rangeChanged);
    QCOMPARE(zoomControl.rangeStart(), -1ns);
    QCOMPARE(zoomControl.rangeEnd(), -1ns);
    QCOMPARE(zoomControl.rangeDuration(), 0ns);

    zoomControl.setTrace(10ns, 500ns);
    QCOMPARE(zoomControl.rangeStart(), 10ns);
    QCOMPARE(zoomControl.rangeEnd(), 10ns);
    QCOMPARE(zoomControl.rangeDuration(), 0ns);
    QCOMPARE(spy.count(), 1);

    zoomControl.setRange(100ns, 200ns);
    QCOMPARE(zoomControl.rangeStart(), 100ns);
    QCOMPARE(zoomControl.rangeEnd(), 200ns);
    QCOMPARE(zoomControl.rangeDuration(), 100ns);
    QCOMPARE(spy.count(), 2);

    zoomControl.clear();
    QCOMPARE(zoomControl.rangeStart(), -1ns);
    QCOMPARE(zoomControl.rangeEnd(), -1ns);
    QCOMPARE(zoomControl.rangeDuration(), 0ns);
    QCOMPARE(spy.count(), 3);
}

void tst_TimelineZoomControl::selection()
{
    TimelineZoomControl zoomControl;
    QSignalSpy spy(&zoomControl, &TimelineZoomControl::selectionChanged);
    QCOMPARE(zoomControl.selectionStart(), -1ns);
    QCOMPARE(zoomControl.selectionEnd(), -1ns);
    QCOMPARE(zoomControl.selectionDuration(), 0ns);

    zoomControl.setTrace(10ns, 500ns);
    zoomControl.setSelection(100ns, 200ns);
    QCOMPARE(zoomControl.selectionStart(), 100ns);
    QCOMPARE(zoomControl.selectionEnd(), 200ns);
    QCOMPARE(zoomControl.selectionDuration(), 100ns);
    QCOMPARE(spy.count(), 1);

    zoomControl.clear();
    QCOMPARE(zoomControl.selectionStart(), -1ns);
    QCOMPARE(zoomControl.selectionEnd(), -1ns);
    QCOMPARE(zoomControl.selectionDuration(), 0ns);
    QCOMPARE(spy.count(), 2);
}

QTEST_GUILESS_MAIN(tst_TimelineZoomControl)

#include "tst_timelinezoomcontrol.moc"
