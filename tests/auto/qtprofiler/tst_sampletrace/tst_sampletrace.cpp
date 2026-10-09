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
        {0, 10, true, {0, 1, 2}},
        {0, 11, false, {0, 3}},
        {200, 10, true, {0, 1, 2}},
        {200, 11, true, {0, 1}},
        {400, 10, false, {0}},
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
        data.pausedRangesUs = {{100, 150}, {300, 350}};

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath dirPath = FilePath::fromString(dir.path());

        QVERIFY_RESULT(writeSampleTrace(data, dirPath));
        const Result<SampleTraceData> read = readSampleTrace(dirPath);
        QVERIFY_RESULT(read);
        QCOMPARE(read->pausedRangesUs, data.pausedRangesUs);
        QCOMPARE(read->samples, data.samples);
    }

    // Losses and throttling are written among the samples, in time order, and
    // come back as they went in -- including those before the first sample
    // and after the last one.
    void gapsRoundTrip()
    {
        SampleTraceData data = makeTestData();
        // Between the samples at 200 and 400, a throttle comes before a loss.
        data.lostSamples = {{0, 3}, {300, 7}, {900, 1}};
        data.throttledTsUs = {100, 200, 250, 1000};

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath dirPath = FilePath::fromString(dir.path());

        QVERIFY_RESULT(writeSampleTrace(data, dirPath));
        const Result<SampleTraceData> read = readSampleTrace(dirPath);
        QVERIFY_RESULT(read);
        QCOMPARE(read->samples, data.samples);
    QCOMPARE(read->lostSamples, data.lostSamples);
    QCOMPARE(read->throttledTsUs, data.throttledTsUs);
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
        QVERIFY(read->pausedRangesUs.isEmpty());
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
        const QList<std::pair<quint64, quint64>> expected{{100, 150}, {500, 600}};
        QCOMPARE(read->pausedRangesUs, expected);
    }

    void pausedRangesAreOnTheTraceTimeline()
    {
        // First sample at 1 s on the steady clock, last one 5 ms into the trace.
        const qint64 first = 1'000'000'000;
        const std::vector<std::pair<qint64, qint64>> intervals{
            {first - 500'000, first + 100'000},   // Began before the first sample.
            {first + 1'000'000, first + 2'000'000},
            {first + 3'000'000, first + 9'000'000}, // Past the last sample.
            {first + 4'500'000, -1},                // Still going on.
        };
        const QList<std::pair<quint64, quint64>> expected{{1000, 2000}, {3000, 5000}};
        QCOMPARE(pausedRangesUs(intervals, first, 5000), expected);
    }

    void pausesAfterTheLastSampleLeaveNoRange()
    {
        const qint64 first = 1'000'000;
        const std::vector<std::pair<qint64, qint64>> intervals{{first + 6'000'000,
                                                                 first + 7'000'000}};
        QVERIFY(pausedRangesUs(intervals, first, 5000).isEmpty());
    }

    void incompleteTraceIsReported()
    {
        SampleTraceData data = makeTestData();
        QVERIFY(incompleteTraceWarning(data).isEmpty());

        data.lostSamples = {{0, 3}, {300, 7}};
        const QString lost = incompleteTraceWarning(data);
        QVERIFY2(lost.contains("10 sample"_L1), qPrintable(lost));
        QVERIFY2(!lost.contains("throttled"_L1), qPrintable(lost));

        data.lostSamples.clear();
        data.throttledTsUs = {100, 200};
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
