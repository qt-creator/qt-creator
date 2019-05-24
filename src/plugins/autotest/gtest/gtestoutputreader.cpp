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
#include "../testtreemodel.h"
#include "../testtreeitem.h"
#include <utils/hostosinfo.h>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace Autotest {
namespace Internal {

static QString constructSourceFilePath(const QString &path, const QString &filePath)
{
    return QFileInfo(path, filePath).canonicalFilePath();
}

GTestOutputReader::GTestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                                     QProcess *testApplication, const QString &buildDirectory,
                                     const QString &projectFile)
    : TestOutputReader(futureInterface, testApplication, buildDirectory)
    , m_projectFile(projectFile)
{
    if (m_testApplication) {
        connect(m_testApplication, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this] (int exitCode, QProcess::ExitStatus /*exitStatus*/) {
            if (exitCode == 1 && !m_description.isEmpty()) {
                createAndReportResult(tr("Running tests failed.\n %1\nExecutable: %2")
                        .arg(m_description).arg(id()), ResultType::MessageFatal);
            }
            // on Windows abort() will result in normal termination, but exit code will be set to 3
            if (Utils::HostOsInfo::isWindowsHost() && exitCode == 3)
                reportCrash();
        });
    }
}

void GTestOutputReader::processOutputLine(const QByteArray &outputLineWithNewLine)
{
    static const QRegularExpression newTestStarts("^\\[-{10}\\] \\d+ tests? from (.*)$");
    static const QRegularExpression testEnds("^\\[-{10}\\] \\d+ tests? from (.*) \\((.*)\\)$");
    static const QRegularExpression newTestSetStarts("^\\[ RUN      \\] (.*)$");
    static const QRegularExpression testSetSuccess("^\\[       OK \\] (.*) \\((.*)\\)$");
    static const QRegularExpression testSetFail("^\\[  FAILED  \\] (.*) \\((\\d+ ms)\\)$");
    static const QRegularExpression disabledTests("^  YOU HAVE (\\d+) DISABLED TESTS?$");
    static const QRegularExpression failureLocation("^(.*):(\\d+): Failure$");
    static const QRegularExpression errorLocation("^(.*)\\((\\d+)\\): error:.*$");
    static const QRegularExpression iterations("^Repeating all tests "
                                               "\\(iteration (\\d+)\\) \\. \\. \\.$");

    const QString line = QString::fromLatin1(chopLineBreak(outputLineWithNewLine));
    if (line.trimmed().isEmpty())
        return;

    struct ExactMatch : public QRegularExpressionMatch
    {
        ExactMatch(const QRegularExpressionMatch &other) : QRegularExpressionMatch(other) {}
        operator bool() const { return hasMatch(); }
    };

    if (!line.startsWith('[')) {
        m_description.append(line).append('\n');
        if (ExactMatch match = iterations.match(line)) {
            m_iteration = match.captured(1).toInt();
            m_description.clear();
        } else if (line.startsWith(QStringLiteral("Note:"))) {
            m_description = line;
            if (m_iteration > 1)
                m_description.append(' ' + tr("(iteration %1)").arg(m_iteration));
            TestResultPtr testResult = TestResultPtr(new GTestResult(id(), m_projectFile, QString()));
            testResult->setResult(ResultType::MessageInternal);
            testResult->setDescription(m_description);
            reportResult(testResult);
            m_description.clear();
        } else if (ExactMatch match = disabledTests.match(line)) {
            m_disabled = match.captured(1).toInt();
            m_description.clear();
        }
        return;
    }

    if (ExactMatch match = testEnds.match(line)) {
        TestResultPtr testResult = createDefaultResult();
        testResult->setResult(ResultType::TestEnd);
        testResult->setDescription(tr("Test execution took %1").arg(match.captured(2)));
        reportResult(testResult);
        m_currentTestSuite.clear();
        m_currentTestCase.clear();
    } else if (ExactMatch match = newTestStarts.match(line)) {
        setCurrentTestSuite(match.captured(1));
        TestResultPtr testResult = createDefaultResult();
        testResult->setResult(ResultType::TestStart);
        if (m_iteration > 1) {
            testResult->setDescription(tr("Repeating test suite %1 (iteration %2)")
                                       .arg(m_currentTestSuite).arg(m_iteration));
        } else {
            testResult->setDescription(tr("Executing test suite %1").arg(m_currentTestSuite));
        }
        reportResult(testResult);
    } else if (ExactMatch match = newTestSetStarts.match(line)) {
        setCurrentTestCase(match.captured(1));
        TestResultPtr testResult = TestResultPtr(new GTestResult(QString(), m_projectFile,
                                                                 QString()));
        testResult->setResult(ResultType::MessageCurrentTest);
        testResult->setDescription(tr("Entering test case %1").arg(m_currentTestCase));
        reportResult(testResult);
        m_description.clear();
    } else if (ExactMatch match = testSetSuccess.match(line)) {
        TestResultPtr testResult = createDefaultResult();
        testResult->setResult(ResultType::Pass);
        testResult->setDescription(m_description);
        reportResult(testResult);
        m_description.clear();
        testResult = createDefaultResult();
        testResult->setResult(ResultType::MessageInternal);
        testResult->setDescription(tr("Execution took %1.").arg(match.captured(2)));
        reportResult(testResult);
        m_futureInterface.setProgressValue(m_futureInterface.progressValue() + 1);
    } else if (ExactMatch match = testSetFail.match(line)) {
        TestResultPtr testResult = createDefaultResult();
        testResult->setResult(ResultType::Fail);
        m_description.chop(1);
        QStringList resultDescription;

        for (const QString &output : m_description.split('\n')) {
            QRegularExpressionMatch innerMatch = failureLocation.match(output);
            if (!innerMatch.hasMatch()) {
                 innerMatch = errorLocation.match(output);
                 if (!innerMatch.hasMatch()) {
                    resultDescription << output;
                    continue;
                 }
            }
            testResult->setDescription(resultDescription.join('\n'));
            reportResult(testResult);
            resultDescription.clear();

            testResult = createDefaultResult();
            testResult->setResult(ResultType::MessageLocation);
            testResult->setLine(innerMatch.captured(2).toInt());
            QString file = constructSourceFilePath(m_buildDir, innerMatch.captured(1));
            if (!file.isEmpty())
                testResult->setFileName(file);
            resultDescription << output;
        }
        testResult->setDescription(resultDescription.join('\n'));
        reportResult(testResult);
        m_description.clear();
        testResult = createDefaultResult();
        testResult->setResult(ResultType::MessageInternal);
        testResult->setDescription(tr("Execution took %1.").arg(match.captured(2)));
        reportResult(testResult);
        m_futureInterface.setProgressValue(m_futureInterface.progressValue() + 1);
    }
}

TestResultPtr GTestOutputReader::createDefaultResult() const
{
    GTestResult *result = new GTestResult(id(), m_projectFile, m_currentTestSuite);
    result->setTestCaseName(m_currentTestCase);
    result->setIteration(m_iteration);

    const TestTreeItem *testItem = result->findTestTreeItem();

    if (testItem && testItem->line()) {
        result->setFileName(testItem->filePath());
        result->setLine(static_cast<int>(testItem->line()));
    }

    return TestResultPtr(result);
}

void GTestOutputReader::setCurrentTestCase(const QString &testCase)
{
    m_currentTestCase = testCase;
}

void GTestOutputReader::setCurrentTestSuite(const QString &testSuite)
{
    m_currentTestSuite = testSuite;
}

} // namespace Internal
} // namespace Autotest
