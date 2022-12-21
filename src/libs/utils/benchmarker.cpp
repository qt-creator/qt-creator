// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "benchmarker.h"
#include "environment.h"

#include <QCoreApplication>
#include <QLoggingCategory>
#include <QTimer>

static Q_LOGGING_CATEGORY(benchmarksLog, "qtc.benchmark", QtWarningMsg);

namespace Utils {

Benchmarker::Benchmarker(const QString &testsuite, const QString &testcase,
                         const QString &tagData) :
    Benchmarker(benchmarksLog(), testsuite, testcase, tagData)
{ }

Benchmarker::Benchmarker(const QLoggingCategory &cat,
                         const QString &testsuite, const QString &testcase,
                         const QString &tagData) :
    m_category(cat),
    m_tagData(tagData),
    m_testsuite(testsuite),
    m_testcase(testcase)
{
    m_timer.start();
}

Benchmarker::~Benchmarker()
{
    if (m_timer.isValid())
        report(m_timer.elapsed());
}

void Benchmarker::report(qint64 ms)
{
    m_timer.invalidate();
    report(m_category, m_testsuite, m_testcase, ms, m_tagData);
}

void Benchmarker::report(const QString &testsuite,
                         const QString &testcase, qint64 ms, const QString &tags)
{
    report(benchmarksLog(), testsuite, testcase, ms, tags);
}

void Benchmarker::report(const QLoggingCategory &cat, const QString &testsuite, const QString &testcase,
                         qint64 ms, const QString &tags)
{
    static const QByteArray quitAfter = qtcEnvironmentVariable("QTC_QUIT_AFTER_BENCHMARK").toLatin1();
    QString t = "unit=ms";
    if (!tags.isEmpty())
        t += "," + tags;

    const QByteArray testSuite = testsuite.toUtf8();
    const QByteArray testCase = testcase.toUtf8();
    qCDebug(cat, "%s::%s: %lld { %s }", testSuite.data(), testCase.data(), ms, t.toUtf8().data());
    if (!quitAfter.isEmpty() && quitAfter == testSuite + "::" + testCase)
        QTimer::singleShot(1000, qApp, &QCoreApplication::quit);
}


} // namespace Utils
