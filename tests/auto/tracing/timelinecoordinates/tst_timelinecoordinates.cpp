// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <tracing/timelinecoordinates.h>

using namespace Timeline;

class tst_TimelineCoordinates : public QObject
{
    Q_OBJECT
private slots:
    void timeToProportion();
    void timeToPixel();
    void pixelToTime();
    void roundTrip();
    void rowAt_data();
    void rowAt();
    void rowTop_data();
    void rowTop();
};

void tst_TimelineCoordinates::timeToProportion()
{
    // Basic proportions within range
    QCOMPARE(Timeline::timeToProportion(0, 0, 100), 0.0);
    QCOMPARE(Timeline::timeToProportion(50, 0, 100), 0.5);
    QCOMPARE(Timeline::timeToProportion(100, 0, 100), 1.0);
    QCOMPARE(Timeline::timeToProportion(25, 0, 100), 0.25);

    // Non-zero start
    QCOMPARE(Timeline::timeToProportion(200, 100, 300), 0.5);
    QCOMPARE(Timeline::timeToProportion(100, 100, 300), 0.0);
    QCOMPARE(Timeline::timeToProportion(300, 100, 300), 1.0);

    // Time outside range — no clamping
    QCOMPARE(Timeline::timeToProportion(-50, 0, 100), -0.5);
    QCOMPARE(Timeline::timeToProportion(150, 0, 100), 1.5);

    // Degenerate zero-width range
    QCOMPARE(Timeline::timeToProportion(42, 100, 100), 0.0);
}

void tst_TimelineCoordinates::timeToPixel()
{
    // Start maps to 0, end maps to width
    QCOMPARE(Timeline::timeToPixel(0, 0, 1000, 500.0), 0.0);
    QCOMPARE(Timeline::timeToPixel(1000, 0, 1000, 500.0), 500.0);
    QCOMPARE(Timeline::timeToPixel(500, 0, 1000, 500.0), 250.0);

    // Proportional
    QCOMPARE(Timeline::timeToPixel(250, 0, 1000, 400.0), 100.0);

    // Zero-width range
    QCOMPARE(Timeline::timeToPixel(42, 5, 5, 800.0), 0.0);

    // Outside range — unclamped
    QVERIFY(Timeline::timeToPixel(-100, 0, 1000, 500.0) < 0.0);
    QVERIFY(Timeline::timeToPixel(1100, 0, 1000, 500.0) > 500.0);
}

void tst_TimelineCoordinates::pixelToTime()
{
    // x=0 maps to rangeStart, x=width maps to rangeEnd
    QCOMPARE(Timeline::pixelToTime(0.0, 500.0, 0, 1000), qint64(0));
    QCOMPARE(Timeline::pixelToTime(500.0, 500.0, 0, 1000), qint64(1000));
    QCOMPARE(Timeline::pixelToTime(250.0, 500.0, 0, 1000), qint64(500));

    // Non-zero start
    QCOMPARE(Timeline::pixelToTime(0.0, 400.0, 100, 500), qint64(100));
    QCOMPARE(Timeline::pixelToTime(400.0, 400.0, 100, 500), qint64(500));
    QCOMPARE(Timeline::pixelToTime(200.0, 400.0, 100, 500), qint64(300));

    // Zero/negative width — returns rangeStart
    QCOMPARE(Timeline::pixelToTime(100.0, 0.0, 50, 200), qint64(50));
    QCOMPARE(Timeline::pixelToTime(100.0, -1.0, 50, 200), qint64(50));
}

void tst_TimelineCoordinates::roundTrip()
{
    const qint64 rangeStart = 1000;
    const qint64 rangeEnd = 9000;
    const double width = 640.0;

    for (qint64 t : {rangeStart, rangeEnd, qint64(3000), qint64(5000), qint64(7500)}) {
        double px = Timeline::timeToPixel(t, rangeStart, rangeEnd, width);
        qint64 back = Timeline::pixelToTime(px, width, rangeStart, rangeEnd);
        // Integer truncation: allow at most 1ns of error
        QVERIFY2(qAbs(back - t) <= 1,
                 qPrintable(QString("round-trip failed for t=%1: got %2").arg(t).arg(back)));
    }
}

void tst_TimelineCoordinates::rowAt_data()
{
    QTest::addColumn<int>("y");
    QTest::addColumn<QList<int>>("heights");
    QTest::addColumn<int>("expectedRow");
    QTest::addColumn<int>("expectedY");

    QList<int> heights = {20, 30, 10};

    QTest::newRow("first row start")   << 0  << heights << 0 << 0;
    QTest::newRow("first row mid")     << 10 << heights << 0 << 10;
    QTest::newRow("first row end")     << 19 << heights << 0 << 19;
    QTest::newRow("second row start")  << 20 << heights << 1 << 0;
    QTest::newRow("second row mid")    << 35 << heights << 1 << 15;
    QTest::newRow("third row start")   << 50 << heights << 2 << 0;
    QTest::newRow("third row end")     << 59 << heights << 2 << 9;
    QTest::newRow("beyond all rows")   << 60 << heights << -1 << 0;
    QTest::newRow("negative y")        << -1 << heights << -1 << 0;
    QTest::newRow("empty heights")     << 0  << QList<int>{} << -1 << 0;
}

void tst_TimelineCoordinates::rowAt()
{
    QFETCH(int, y);
    QFETCH(QList<int>, heights);
    QFETCH(int, expectedRow);
    QFETCH(int, expectedY);

    RowHit hit = Timeline::rowAt(y, heights);
    QCOMPARE(hit.row, expectedRow);
    QCOMPARE(hit.yWithinRow, expectedY);
}

void tst_TimelineCoordinates::rowTop_data()
{
    QTest::addColumn<int>("index");
    QTest::addColumn<QList<int>>("heights");
    QTest::addColumn<int>("expected");

    QList<int> heights = {20, 30, 10};

    QTest::newRow("row 0")           << 0 << heights << 0;
    QTest::newRow("row 1")           << 1 << heights << 20;
    QTest::newRow("row 2")           << 2 << heights << 50;
    QTest::newRow("past end")        << 3 << heights << 60;
    QTest::newRow("past end by 2")   << 5 << heights << 60;
    QTest::newRow("empty heights 0") << 0 << QList<int>{} << 0;
    QTest::newRow("empty heights 1") << 1 << QList<int>{} << 0;
}

void tst_TimelineCoordinates::rowTop()
{
    QFETCH(int, index);
    QFETCH(QList<int>, heights);
    QFETCH(int, expected);

    QCOMPARE(Timeline::rowTop(index, heights), expected);
}

QTEST_GUILESS_MAIN(tst_TimelineCoordinates)

#include "tst_timelinecoordinates.moc"
