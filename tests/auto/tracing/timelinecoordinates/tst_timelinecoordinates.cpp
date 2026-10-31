// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <tracing/timelinecoordinates.h>

using namespace std::chrono;
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
    void rulerBlockDuration();
    void forEachRulerTick();
};

namespace {

struct Ticks
{
    QList<nanoseconds> blockTimes;
    QList<double> blockX;
    QList<double> minor;
    QList<double> major;
    double pixelsPerBlock = 0.0;
};

Ticks collectTicks(nanoseconds rangeStart, nanoseconds rangeEnd, double widthPx)
{
    Ticks ticks;
    Timeline::forEachRulerTick(
        rangeStart, rangeEnd, widthPx,
        [&](nanoseconds t, double x, double pixelsPerBlock) {
            ticks.blockTimes.append(t);
            ticks.blockX.append(x);
            ticks.pixelsPerBlock = pixelsPerBlock;
        },
        [&](double x, bool isMajor) { (isMajor ? ticks.major : ticks.minor).append(x); });
    return ticks;
}

} // namespace

void tst_TimelineCoordinates::timeToProportion()
{
    // Basic proportions within range
    QCOMPARE(Timeline::timeToProportion(0ns, 0ns, 100ns), 0.0);
    QCOMPARE(Timeline::timeToProportion(50ns, 0ns, 100ns), 0.5);
    QCOMPARE(Timeline::timeToProportion(100ns, 0ns, 100ns), 1.0);
    QCOMPARE(Timeline::timeToProportion(25ns, 0ns, 100ns), 0.25);

    // Non-zero start
    QCOMPARE(Timeline::timeToProportion(200ns, 100ns, 300ns), 0.5);
    QCOMPARE(Timeline::timeToProportion(100ns, 100ns, 300ns), 0.0);
    QCOMPARE(Timeline::timeToProportion(300ns, 100ns, 300ns), 1.0);

    // Time outside range — no clamping
    QCOMPARE(Timeline::timeToProportion(-50ns, 0ns, 100ns), -0.5);
    QCOMPARE(Timeline::timeToProportion(150ns, 0ns, 100ns), 1.5);

    // Degenerate zero-width range
    QCOMPARE(Timeline::timeToProportion(42ns, 100ns, 100ns), 0.0);
}

void tst_TimelineCoordinates::timeToPixel()
{
    // Start maps to 0, end maps to width
    QCOMPARE(Timeline::timeToPixel(0ns, 0ns, 1000ns, 500.0), 0.0);
    QCOMPARE(Timeline::timeToPixel(1000ns, 0ns, 1000ns, 500.0), 500.0);
    QCOMPARE(Timeline::timeToPixel(500ns, 0ns, 1000ns, 500.0), 250.0);

    // Proportional
    QCOMPARE(Timeline::timeToPixel(250ns, 0ns, 1000ns, 400.0), 100.0);

    // Zero-width range
    QCOMPARE(Timeline::timeToPixel(42ns, 5ns, 5ns, 800.0), 0.0);

    // Outside range — unclamped
    QVERIFY(Timeline::timeToPixel(-100ns, 0ns, 1000ns, 500.0) < 0.0);
    QVERIFY(Timeline::timeToPixel(1100ns, 0ns, 1000ns, 500.0) > 500.0);
}

void tst_TimelineCoordinates::pixelToTime()
{
    // x=0 maps to rangeStart, x=width maps to rangeEnd
    QCOMPARE(Timeline::pixelToTime(0.0, 500.0, 0ns, 1000ns), 0ns);
    QCOMPARE(Timeline::pixelToTime(500.0, 500.0, 0ns, 1000ns), 1000ns);
    QCOMPARE(Timeline::pixelToTime(250.0, 500.0, 0ns, 1000ns), 500ns);

    // Non-zero start
    QCOMPARE(Timeline::pixelToTime(0.0, 400.0, 100ns, 500ns), 100ns);
    QCOMPARE(Timeline::pixelToTime(400.0, 400.0, 100ns, 500ns), 500ns);
    QCOMPARE(Timeline::pixelToTime(200.0, 400.0, 100ns, 500ns), 300ns);

    // Zero/negative width — returns rangeStart
    QCOMPARE(Timeline::pixelToTime(100.0, 0.0, 50ns, 200ns), 50ns);
    QCOMPARE(Timeline::pixelToTime(100.0, -1.0, 50ns, 200ns), 50ns);
}

void tst_TimelineCoordinates::roundTrip()
{
    const nanoseconds rangeStart = 1000ns;
    const nanoseconds rangeEnd = 9000ns;
    const double width = 640.0;

    for (nanoseconds t : {rangeStart, rangeEnd, 3000ns, 5000ns, 7500ns}) {
        double px = Timeline::timeToPixel(t, rangeStart, rangeEnd, width);
        nanoseconds back = Timeline::pixelToTime(px, width, rangeStart, rangeEnd);
        // Integer truncation: allow at most 1ns of error
        QVERIFY2(abs(back - t) <= 1ns,
                 qPrintable(QString("round-trip failed for t=%1: got %2")
                                .arg(t.count())
                                .arg(back.count())));
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

void tst_TimelineCoordinates::rulerBlockDuration()
{
    // 1000 ns over 1000 px: the 120 px hint asks for 120 ns, snapped down to 64.
    QCOMPARE(Timeline::rulerBlockDuration(1000ns, 1000.0), 64ns);

    // Snapping is to the power of two at or below the ideal duration.
    QCOMPARE(Timeline::rulerBlockDuration(1280ns, 1280.0), 64ns);
    QCOMPARE(Timeline::rulerBlockDuration(2560ns, 1280.0), 128ns);

    // A smaller hint asks for shorter blocks.
    QCOMPARE(Timeline::rulerBlockDuration(1000ns, 1000.0, 80), 64ns);
    QCOMPARE(Timeline::rulerBlockDuration(1000ns, 1000.0, 30), 16ns);

    // Never shorter than 1 ns, and degenerate input is safe.
    QCOMPARE(Timeline::rulerBlockDuration(1ns, 100000.0), 1ns);
    QCOMPARE(Timeline::rulerBlockDuration(0ns, 100.0), 1ns);
    QCOMPARE(Timeline::rulerBlockDuration(100ns, 0.0), 1ns);
    QCOMPARE(Timeline::rulerBlockDuration(100ns, -1.0), 1ns);
}

void tst_TimelineCoordinates::forEachRulerTick()
{
    // 1024 ns over 1024 px: 64 ns blocks, 64 px wide, 12.8 px per section.
    const Ticks aligned = collectTicks(0ns, 1024ns, 1024.0);
    QCOMPARE(aligned.pixelsPerBlock, 64.0);

    // Blocks start at multiples of the block duration. The block starting on the
    // right edge has no visible label area and is not reported.
    QCOMPARE(aligned.blockTimes.size(), 16);
    QCOMPARE(aligned.blockTimes.first(), 0ns);
    QCOMPARE(aligned.blockTimes.last(), 960ns);
    QCOMPARE(aligned.blockX.first(), 0.0);
    QCOMPARE(aligned.blockX.last(), 960.0);

    // A major tick sits on every block edge, the right edge of the view included.
    QCOMPARE(aligned.major.size(), 16);
    QCOMPARE(aligned.major.first(), 64.0);
    QCOMPARE(aligned.major.last(), 1024.0);

    // Four minor ticks per block, none of them beyond the last major tick.
    QCOMPARE(aligned.minor.size(), 64);
    QCOMPARE(aligned.minor.first(), 12.8);
    QCOMPARE(aligned.minor.last(), 1011.2);

    // Ticks arrive left to right and stay within the view.
    for (const QList<double> &ticks : {aligned.minor, aligned.major}) {
        for (int i = 0; i < ticks.size(); ++i) {
            QVERIFY(ticks.at(i) >= 0.0 && ticks.at(i) <= 1024.0);
            if (i > 0)
                QVERIFY(ticks.at(i) > ticks.at(i - 1));
        }
    }

    // A range that does not start on a block boundary: the first block starts
    // left of the view and is reported, because its label area reaches into it.
    const Ticks unaligned = collectTicks(100ns, 1100ns, 1000.0);
    QCOMPARE(unaligned.blockTimes.first(), 64ns);
    QCOMPARE(unaligned.blockX.first(), -36.0);
    QCOMPARE(unaligned.minor.first(), 2.4);
    QCOMPARE(unaligned.major.first(), 28.0);

    // Degenerate ranges report nothing at all.
    for (const Ticks &ticks : {collectTicks(0ns, 0ns, 1000.0),
                               collectTicks(1000ns, 0ns, 1000.0),
                               collectTicks(0ns, 1000ns, 0.0),
                               collectTicks(0ns, 1000ns, -1.0)}) {
        QVERIFY(ticks.blockTimes.isEmpty());
        QVERIFY(ticks.minor.isEmpty());
        QVERIFY(ticks.major.isEmpty());
    }
}

QTEST_GUILESS_MAIN(tst_TimelineCoordinates)

#include "tst_timelinecoordinates.moc"
