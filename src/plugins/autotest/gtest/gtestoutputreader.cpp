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

void GTestOutputReader::processOutput(const QByteArray &outputLine)
{
    static QRegExp newTestStarts("^\\[-{10}\\] \\d+ tests? from (.*)$");
    static QRegExp testEnds("^\\[-{10}\\] \\d+ tests? from (.*) \\((.*)\\)$");
    static QRegExp newTestSetStarts("^\\[ RUN      \\] (.*)$");
    static QRegExp testSetSuccess("^\\[       OK \\] (.*) \\((.*)\\)$");
    static QRegExp testSetFail("^\\[  FAILED  \\] (.*) \\((.*)\\)$");
    static QRegExp disabledTests("^  YOU HAVE (\\d+) DISABLED TESTS?$");
    static QRegExp failureLocation("^(.*):(\\d+): Failure$");
    static QRegExp errorLocation("^(.*)\\((\\d+)\\): error:.*$");
    static QRegExp iterations("^Repeating all tests \\(iteration (\\d+)\\) \\. \\. \\.$");

    QByteArray read = outputLine;
    if (!m_unprocessed.isEmpty()) {
        read = m_unprocessed + read;
        m_unprocessed.clear();
    }
    if (!read.endsWith('\n')) {
        m_unprocessed = read;
        return;
    }
    read.chop(1); // remove the newline from the output
    if (read.endsWith('\r'))
        read.chop(1);

    const QString line = QString::fromLatin1(read);
    if (line.trimmed().isEmpty())
        return;

    if (!line.startsWith('[')) {
        m_description.append(line).append('\n');
        if (iterations.exactMatch(line)) {
            m_iteration = iterations.cap(1).toInt();
            m_description.clear();
        } else if (line.startsWith(QStringLiteral("Note:"))) {
            m_description = line;
            if (m_iteration > 1)
                m_description.append(' ' + tr("(iteration %1)").arg(m_iteration));
            TestResultPtr testResult = TestResultPtr(new GTestResult);
            testResult->setResult(Result::MessageInternal);
            testResult->setDescription(m_description);
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
        return;
    }

    if (testEnds.exactMatch(line)) {
        GTestResult *testResult = createDefaultResult();
        testResult->setResult(Result::MessageTestCaseEnd);
        testResult->setDescription(tr("Test execution took %1").arg(testEnds.cap(2)));
        m_futureInterface.reportResult(TestResultPtr(testResult));
        m_currentTestName.clear();
        m_currentTestSet.clear();
    } else if (newTestStarts.exactMatch(line)) {
        m_currentTestName = newTestStarts.cap(1);
        TestResultPtr testResult = TestResultPtr(createDefaultResult());
        testResult->setResult(Result::MessageTestCaseStart);
        if (m_iteration > 1) {
            testResult->setDescription(tr("Repeating test case %1 (iteration %2)")
                                       .arg(m_currentTestName).arg(m_iteration));
        } else {
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
        GTestResult *testResult = createDefaultResult();
        testResult->setResult(Result::Pass);
        testResult->setDescription(m_description);
        m_futureInterface.reportResult(TestResultPtr(testResult));
        m_description.clear();
        testResult = createDefaultResult();
        testResult->setResult(Result::MessageInternal);
        testResult->setDescription(tr("Execution took %1.").arg(testSetSuccess.cap(2)));
        m_futureInterface.reportResult(TestResultPtr(testResult));
        m_futureInterface.setProgressValue(m_futureInterface.progressValue() + 1);
    } else if (testSetFail.exactMatch(line)) {
        GTestResult *testResult = createDefaultResult();
        testResult->setResult(Result::Fail);
        m_description.chop(1);
        testResult->setDescription(m_description);

        for (const QString &output : m_description.split('\n')) {
            QRegExp *match = nullptr;
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
        testResult = createDefaultResult();
        testResult->setResult(Result::MessageInternal);
        testResult->setDescription(tr("Execution took %1.").arg(testSetFail.cap(2)));
        m_futureInterface.reportResult(TestResultPtr(testResult));
        m_futureInterface.setProgressValue(m_futureInterface.progressValue() + 1);
    }
}

GTestResult *GTestOutputReader::createDefaultResult() const
{
    GTestResult *result = new GTestResult(m_currentTestName);
    result->setTestSetName(m_currentTestSet);
    result->setIteration(m_iteration);
    return result;
}

} // namespace Internal
} // namespace Autotest
