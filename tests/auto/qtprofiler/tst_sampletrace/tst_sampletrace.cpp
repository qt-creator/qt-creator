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

using namespace QmlProfiler::Internal;
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
        const FilePath first = uniqueTracePath("tst-sampletrace"_L1);
        QVERIFY(first.createDir());
        const QScopeGuard cleanup([&first] { first.removeRecursively(); });

        // Same second, so the plain timestamp is taken and a counter must appear.
        const FilePath second = uniqueTracePath("tst-sampletrace"_L1);
        QVERIFY(second != first);
        QVERIFY(!second.exists());
        QCOMPARE(second.fileName(), first.fileName() + "-2"_L1);
    }
};

QTEST_GUILESS_MAIN(tst_SampleTrace)

#include "tst_sampletrace.moc"
