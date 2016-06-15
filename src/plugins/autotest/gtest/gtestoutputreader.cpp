/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "gtestoutputreader.h"
#include "gtestresult.h"

#include <QDir>
#include <QFileInfo>

namespace Autotest {
namespace Internal {

static QString constructSourceFilePath(const QString &path, const QString &filePath)
{
    return QFileInfo(path, filePath).canonicalFilePath();
}

GTestOutputReader::GTestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                                     QProcess *testApplication, const QString &buildDirectory)
    : TestOutputReader(futureInterface, testApplication, buildDirectory)
{
}

void GTestOutputReader::processOutput()
{
    if (!m_testApplication || m_testApplication->state() != QProcess::Running)
        return;

    static QRegExp newTestStarts(QStringLiteral("^\\[-{10}\\] \\d+ tests? from (.*)$"));
    static QRegExp testEnds(QStringLiteral("^\\[-{10}\\] \\d+ tests? from (.*) \\((.*)\\)$"));
    static QRegExp newTestSetStarts(QStringLiteral("^\\[ RUN      \\] (.*)$"));
    static QRegExp testSetSuccess(QStringLiteral("^\\[       OK \\] (.*) \\((.*)\\)$"));
    static QRegExp testSetFail(QStringLiteral("^\\[  FAILED  \\] (.*) \\((.*)\\)$"));
    static QRegExp disabledTests(QStringLiteral("^  YOU HAVE (\\d+) DISABLED TESTS?$"));
    static QRegExp failureLocation(QStringLiteral("^(.*):(\\d+): Failure$"));
    static QRegExp errorLocation(QStringLiteral("^(.*)\\((\\d+)\\): error:.*$"));
    static QRegExp iterations(QStringLiteral("^Repeating all tests \\(iteration (\\d+)\\) . . .$"));

    while (m_testApplication->canReadLine()) {
        if (m_futureInterface.isCanceled())
            return;
        QByteArray read = m_testApplication->readLine();
        if (!m_unprocessed.isEmpty()) {
            read = m_unprocessed + read;
            m_unprocessed.clear();
        }
        if (!read.endsWith('\n')) {
            m_unprocessed = read;
            continue;
        }
        read.chop(1); // remove the newline from the output
        if (read.endsWith('\r'))
            read.chop(1);

        const QString line = QString::fromLatin1(read);
        if (line.trimmed().isEmpty())
            continue;

        if (!line.startsWith(QLatin1Char('['))) {
            m_description.append(line).append(QLatin1Char('\n'));
            if (iterations.exactMatch(line)) {
                m_iteration = iterations.cap(1).toInt();
                m_description.clear();
            } else if (line.startsWith(QStringLiteral("Note:"))) {
                TestResultPtr testResult = TestResultPtr(new GTestResult());
                testResult->setResult(Result::MessageInternal);
                testResult->setDescription(line);
                m_futureInterface.reportResult(testResult);
                m_description.clear();
            } else if (disabledTests.exactMatch(line)) {
                TestResultPtr testResult = TestResultPtr(new GTestResult());
                testResult->setResult(Result::MessageDisabledTests);
                int disabled = disabledTests.cap(1).toInt();
                testResult->setDescription(tr("You have %n disabled test(s).", 0, disabled));
                testResult->setLine(disabled); // misuse line property to hold number of disabled
                m_futureInterface.reportResult(testResult);
                m_description.clear();
            }
            continue;
        }

        if (testEnds.exactMatch(line)) {
            GTestResult *testResult = new GTestResult(m_currentTestName);
            testResult->setTestSetName(m_currentTestSet);
            testResult->setResult(Result::MessageTestCaseEnd);
            testResult->setDescription(tr("Test execution took %1").arg(testEnds.cap(2)));
            m_futureInterface.reportResult(TestResultPtr(testResult));
            m_currentTestName.clear();
            m_currentTestSet.clear();
        } else if (newTestStarts.exactMatch(line)) {
            m_currentTestName = newTestStarts.cap(1);
            TestResultPtr testResult = TestResultPtr(new GTestResult(m_currentTestName));
            if (m_iteration > 1) {
                testResult->setResult(Result::MessageTestCaseRepetition);
                testResult->setDescription(tr("Repeating test case %1 (iteration %2)")
                                           .arg(m_currentTestName).arg(m_iteration));
            } else {
                testResult->setResult(Result::MessageTestCaseStart);
                testResult->setDescription(tr("Executing test case %1").arg(m_currentTestName));
            }
            m_futureInterface.reportResult(testResult);
        } else if (newTestSetStarts.exactMatch(line)) {
            m_currentTestSet = newTestSetStarts.cap(1);
            TestResultPtr testResult = TestResultPtr(new GTestResult());
            testResult->setResult(Result::MessageCurrentTest);
            testResult->setDescription(tr("Entering test set %1").arg(m_currentTestSet));
            m_futureInterface.reportResult(testResult);
            m_description.clear();
        } else if (testSetSuccess.exactMatch(line)) {
            GTestResult *testResult = new GTestResult(m_currentTestName);
            testResult->setTestSetName(m_currentTestSet);
            testResult->setResult(Result::Pass);
            testResult->setDescription(m_description);
            m_futureInterface.reportResult(TestResultPtr(testResult));
            m_description.clear();
            testResult = new GTestResult(m_currentTestName);
            testResult->setTestSetName(m_currentTestSet);
            testResult->setResult(Result::MessageInternal);
            testResult->setDescription(tr("Execution took %1.").arg(testSetSuccess.cap(2)));
            m_futureInterface.reportResult(TestResultPtr(testResult));
            m_futureInterface.setProgressValue(m_futureInterface.progressValue() + 1);
        } else if (testSetFail.exactMatch(line)) {
            GTestResult *testResult = new GTestResult(m_currentTestName);
            testResult->setTestSetName(m_currentTestSet);
            testResult->setResult(Result::Fail);
            m_description.chop(1);
            testResult->setDescription(m_description);

            foreach (const QString &output, m_description.split(QLatin1Char('\n'))) {
                QRegExp *match = 0;
                if (failureLocation.exactMatch(output))
                    match = &failureLocation;
                else if (errorLocation.exactMatch(output))
                    match = &errorLocation;

                if (match) {
                    QString file = constructSourceFilePath(m_buildDir, match->cap(1));
                    if (!file.isEmpty()) {
                        testResult->setFileName(file);
                        testResult->setLine(match->cap(2).toInt());
                        break;
                    }
                }
            }
            m_futureInterface.reportResult(TestResultPtr(testResult));
            m_description.clear();
            testResult = new GTestResult(m_currentTestName);
            testResult->setTestSetName(m_currentTestSet);
            testResult->setResult(Result::MessageInternal);
            testResult->setDescription(tr("Execution took %1.").arg(testSetFail.cap(2)));
            m_futureInterface.reportResult(TestResultPtr(testResult));
            m_futureInterface.setProgressValue(m_futureInterface.progressValue() + 1);
        }
    }
}

} // namespace Internal
} // namespace Autotest
