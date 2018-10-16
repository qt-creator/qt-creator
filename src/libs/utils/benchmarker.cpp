/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "benchmarker.h"

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
    static const QByteArray quitAfter = qgetenv("QTC_QUIT_AFTER_BENCHMARK");
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
