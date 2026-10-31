// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sampletrace.h"

#include <utils/result.h>

#include <QDateTime>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

using namespace Profiler::Internal;
using namespace Utils;
using namespace Qt::StringLiterals;
using namespace std::chrono;
using namespace std::chrono_literals;

static SampleTraceData makeTestData()
{
    SampleTraceData data;
    data.pid = 1234;
    data.labels = {{"start", "/src/start.cpp", 1, "app", 0x10},
                   {"main", "/src/main.cpp", 42},
                   {"qt_frame", QString(), 0, "QtCore", 0x2bc},
                   "idle()"};
    data.threadNames = {{10, "main"}, {11, "worker"}};
    data.samples = {
        {0us, 10, true, {0, 1, 2}},
        {0us, 11, false, {0, 3}},
        {200us, 10, true, {0, 1, 2}},
        {200us, 11, true, {0, 1}},
        {400us, 10, false, {0}},
    };
    return data;
}

class tst_SampleTrace : public QObject
{
    Q_OBJECT

private slots:
    void roundTrip()
    {
        const SampleTraceData data = makeTestData();

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath dirPath = FilePath::fromString(dir.path());

        QVERIFY_RESULT(writeSampleTrace(data, dirPath));
        QVERIFY(isSamplerTrace(dirPath));

        const Result<SampleTraceData> read = readSampleTrace(dirPath);
        QVERIFY_RESULT(read);
        QCOMPARE(read->pid, data.pid);
        QCOMPARE(read->labels, data.labels);
        QCOMPARE(read->threadNames, data.threadNames);
        QCOMPARE(read->samples, data.samples);
    }

    void pausedRangesRoundTrip()
    {
        SampleTraceData data = makeTestData();
        data.pausedRanges = {{100us, 150us}, {300us, 350us}};

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath dirPath = FilePath::fromString(dir.path());

        QVERIFY_RESULT(writeSampleTrace(data, dirPath));
        const Result<SampleTraceData> read = readSampleTrace(dirPath);
        QVERIFY_RESULT(read);
        QCOMPARE(read->pausedRanges, data.pausedRanges);
        QCOMPARE(read->samples, data.samples);
    }

    // Losses and throttling are written among the samples, in time order, and
    // come back as they went in -- including those before the first sample
    // and after the last one.
    void gapsRoundTrip()
    {
        SampleTraceData data = makeTestData();
        // Between the samples at 200 and 400, a throttle comes before a loss.
        data.lostSamples = {{0us, 3}, {300us, 7}, {900us, 1}};
        data.throttledTimestamps = {100us, 200us, 250us, 1000us};

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath dirPath = FilePath::fromString(dir.path());

        QVERIFY_RESULT(writeSampleTrace(data, dirPath));
        const Result<SampleTraceData> read = readSampleTrace(dirPath);
        QVERIFY_RESULT(read);
        QCOMPARE(read->samples, data.samples);
    QCOMPARE(read->lostSamples, data.lostSamples);
    QCOMPARE(read->throttledTimestamps, data.throttledTimestamps);
    }

    void noPausedRangesWritesNoFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath dirPath = FilePath::fromString(dir.path());

        QVERIFY_RESULT(writeSampleTrace(makeTestData(), dirPath));
        QVERIFY(!dirPath.pathAppended("paused-ranges").exists());
        const Result<SampleTraceData> read = readSampleTrace(dirPath);
        QVERIFY_RESULT(read);
        QVERIFY(read->pausedRanges.isEmpty());
    }

    void malformedPausedRangesAreSkipped()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath dirPath = FilePath::fromString(dir.path());
        QVERIFY_RESULT(writeSampleTrace(makeTestData(), dirPath));

        QFile file(dirPath.pathAppended("paused-ranges").toFSPathString());
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("100 150\n"
                   "not numbers\n"
                   "300\n"
                   "400 350\n"   // Ends before it starts.
                   "1 2 3\n"
                   "\n"
                   "500 600\n");
        file.close();

        const Result<SampleTraceData> read = readSampleTrace(dirPath);
        QVERIFY_RESULT(read);
        const QList<std::pair<microseconds, microseconds>> expected{{100us, 150us},
                                                                     {500us, 600us}};
        QCOMPARE(read->pausedRanges, expected);
    }

    void pausedRangesAreOnTheTraceTimeline()
    {
        // First sample at 1 s on the steady clock, last one 5 ms into the trace.
        const nanoseconds first = 1s;
        const std::vector<std::pair<nanoseconds, nanoseconds>> intervals{
            {first - 500us, first + 100us}, // Began before the first sample.
            {first + 1ms, first + 2ms},
            {first + 3ms, first + 9ms}, // Past the last sample.
            {first + 4500us, -1ns},     // Still going on.
        };
        const QList<std::pair<microseconds, microseconds>> expected{{1000us, 2000us},
                                                                     {3000us, 5000us}};
        QCOMPARE(pausedRanges(intervals, first, 5000us), expected);
    }

    void pausesAfterTheLastSampleLeaveNoRange()
    {
        const nanoseconds first = 1ms;
        const std::vector<std::pair<nanoseconds, nanoseconds>> intervals{
            {first + 6ms, first + 7ms}};
        QVERIFY(pausedRanges(intervals, first, 5000us).isEmpty());
    }

    void incompleteTraceIsReported()
    {
        SampleTraceData data = makeTestData();
        QVERIFY(incompleteTraceWarning(data).isEmpty());

        data.lostSamples = {{0us, 3}, {300us, 7}};
        const QString lost = incompleteTraceWarning(data);
        QVERIFY2(lost.contains("10 sample"_L1), qPrintable(lost));
        QVERIFY2(!lost.contains("throttled"_L1), qPrintable(lost));

        data.lostSamples.clear();
        data.throttledTimestamps = {100us, 200us};
        const QString throttled = incompleteTraceWarning(data);
        QVERIFY2(throttled.contains("throttled"_L1), qPrintable(throttled));
        QVERIFY2(throttled.contains("2 time"_L1), qPrintable(throttled));
    }

    void emptyDirIsNotASamplerTrace()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath dirPath = FilePath::fromString(dir.path());
        QVERIFY(!isSamplerTrace(dirPath));
        QVERIFY(!readSampleTrace(dirPath).has_value());
    }

    void emptyDataRoundTrips()
    {
        SampleTraceData data;
        data.pid = 1;

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath dirPath = FilePath::fromString(dir.path());

        QVERIFY_RESULT(writeSampleTrace(data, dirPath));
        QVERIFY(isSamplerTrace(dirPath));
        const Result<SampleTraceData> read = readSampleTrace(dirPath);
        QVERIFY_RESULT(read);
        QVERIFY(read->samples.isEmpty());
        QVERIFY(read->labels.isEmpty());
    }

    void tracePathCarriesReadableDate()
    {
        const FilePath path = uniqueTracePath("tst-sampletrace"_L1, ".qtd"_L1);
        QVERIFY(!path.exists());
        QCOMPARE(path.parentDir(),
                 FilePath::fromString(
                     QStandardPaths::writableLocation(QStandardPaths::TempLocation)));

        static const QRegularExpression re(
            uR"(^tst-sampletrace-(\d{4}-\d{2}-\d{2}-\d{2}-\d{2}-\d{2})\.qtd$)"_s);
        const QRegularExpressionMatch match = re.match(path.fileName());
        QVERIFY2(match.hasMatch(), qPrintable(path.fileName()));

        const QDateTime stamp =
            QDateTime::fromString(match.captured(1), u"yyyy-MM-dd-hh-mm-ss"_s);
        QVERIFY(stamp.isValid());
        QCOMPARE(stamp.date(), QDate::currentDate());
    }

    void tracePathAvoidsCollisions()
    {
        // A fixed point in time, so the two calls below are guaranteed to
        // collide on the same second instead of racing the real clock.
        const QDateTime now = QDateTime::currentDateTime();

        const FilePath first = uniqueTracePathAt(now, "tst-sampletrace"_L1);
        QVERIFY(first.createDir());
        const QScopeGuard cleanup([&first] { first.removeRecursively(); });

        // Same second, so the plain timestamp is taken and a counter must appear.
        const FilePath second = uniqueTracePathAt(now, "tst-sampletrace"_L1);
        QVERIFY(second != first);
        QVERIFY(!second.exists());
        QCOMPARE(second.fileName(), first.fileName() + "-2"_L1);
    }
};

QTEST_GUILESS_MAIN(tst_SampleTrace)

#include "tst_sampletrace.moc"
