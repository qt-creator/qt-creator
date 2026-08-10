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
    void rulerBlockDuration();
    void forEachRulerTick();
};

namespace {

struct Ticks
{
    QList<qint64> blockTimes;
    QList<double> blockX;
    QList<double> minor;
    QList<double> major;
    double pixelsPerBlock = 0.0;
};

Ticks collectTicks(qint64 rangeStart, qint64 rangeEnd, double widthPx)
{
    Ticks ticks;
    Timeline::forEachRulerTick(
        rangeStart, rangeEnd, widthPx,
        [&](qint64 t, double x, double pixelsPerBlock) {
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

void tst_TimelineCoordinates::rulerBlockDuration()
{
    // 1000 ns over 1000 px: the 120 px hint asks for 120 ns, snapped down to 64.
    QCOMPARE(Timeline::rulerBlockDuration(1000, 1000.0), qint64(64));

    // Snapping is to the power of two at or below the ideal duration.
    QCOMPARE(Timeline::rulerBlockDuration(1280, 1280.0), qint64(64));
    QCOMPARE(Timeline::rulerBlockDuration(2560, 1280.0), qint64(128));

    // A smaller hint asks for shorter blocks.
    QCOMPARE(Timeline::rulerBlockDuration(1000, 1000.0, 80), qint64(64));
    QCOMPARE(Timeline::rulerBlockDuration(1000, 1000.0, 30), qint64(16));

    // Never shorter than 1 ns, and degenerate input is safe.
    QCOMPARE(Timeline::rulerBlockDuration(1, 100000.0), qint64(1));
    QCOMPARE(Timeline::rulerBlockDuration(0, 100.0), qint64(1));
    QCOMPARE(Timeline::rulerBlockDuration(100, 0.0), qint64(1));
    QCOMPARE(Timeline::rulerBlockDuration(100, -1.0), qint64(1));
}

void tst_TimelineCoordinates::forEachRulerTick()
{
    // 1024 ns over 1024 px: 64 ns blocks, 64 px wide, 12.8 px per section.
    const Ticks aligned = collectTicks(0, 1024, 1024.0);
    QCOMPARE(aligned.pixelsPerBlock, 64.0);

    // Blocks start at multiples of the block duration. The block starting on the
    // right edge has no visible label area and is not reported.
    QCOMPARE(aligned.blockTimes.size(), 16);
    QCOMPARE(aligned.blockTimes.first(), qint64(0));
    QCOMPARE(aligned.blockTimes.last(), qint64(960));
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
    const Ticks unaligned = collectTicks(100, 1100, 1000.0);
    QCOMPARE(unaligned.blockTimes.first(), qint64(64));
    QCOMPARE(unaligned.blockX.first(), -36.0);
    QCOMPARE(unaligned.minor.first(), 2.4);
    QCOMPARE(unaligned.major.first(), 28.0);

    // Degenerate ranges report nothing at all.
    for (const Ticks &ticks : {collectTicks(0, 0, 1000.0),
                               collectTicks(1000, 0, 1000.0),
                               collectTicks(0, 1000, 0.0),
                               collectTicks(0, 1000, -1.0)}) {
        QVERIFY(ticks.blockTimes.isEmpty());
        QVERIFY(ticks.minor.isEmpty());
        QVERIFY(ticks.major.isEmpty());
    }
}

QTEST_GUILESS_MAIN(tst_TimelineCoordinates)

#include "tst_timelinecoordinates.moc"
