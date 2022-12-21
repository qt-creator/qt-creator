// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gtestoutputreader.h"

#include "gtestresult.h"
#include "../testtreeitem.h"
#include "../autotesttr.h"

#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>

#include <QRegularExpression>

namespace Autotest {
namespace Internal {

GTestOutputReader::GTestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                                     Utils::QtcProcess *testApplication,
                                     const Utils::FilePath &buildDirectory,
                                     const Utils::FilePath &projectFile)
    : TestOutputReader(futureInterface, testApplication, buildDirectory)
    , m_projectFile(projectFile)
{
    if (m_testApplication) {
        connect(m_testApplication, &Utils::QtcProcess::done, this, [this] {
            const int exitCode = m_testApplication->exitCode();
            if (exitCode == 1 && !m_description.isEmpty()) {
                createAndReportResult(Tr::tr("Running tests failed.\n %1\nExecutable: %2")
                                      .arg(m_description).arg(id()), ResultType::MessageFatal);
            }
            // on Windows abort() will result in normal termination, but exit code will be set to 3
            if (Utils::HostOsInfo::isWindowsHost() && exitCode == 3)
                reportCrash();
        });
    }
}

void GTestOutputReader::processOutputLine(const QByteArray &outputLine)
{
    static const QRegularExpression newTestStarts("^\\[-{10}\\] \\d+ tests? from (.*)$");
    static const QRegularExpression testEnds("^\\[-{10}\\] \\d+ tests? from (.*) \\((.*)\\)$");
    static const QRegularExpression newTestSetStarts("^\\[ RUN      \\] (.*)$");
    static const QRegularExpression testSetSuccess("^\\[       OK \\] (.*) \\((.*)\\)$");
    static const QRegularExpression testSetFail("^\\[  FAILED  \\] (.*) \\((\\d+ ms)\\)$");
    static const QRegularExpression testDeath("^\\[  DEATH   \\] (.*)$");
    static const QRegularExpression testSetSkipped("^\\[  SKIPPED \\] (.*) \\((\\d+ ms)\\)$");
    static const QRegularExpression disabledTests("^  YOU HAVE (\\d+) DISABLED TESTS?$");
    static const QRegularExpression iterations("^Repeating all tests "
                                               "\\(iteration (\\d+)\\) \\. \\. \\.$");
    static const QRegularExpression logging("^\\[( FATAL | ERROR |WARNING|  INFO )\\] "
                                            "(.*):(\\d+):: (.*)$");

    const QString line = removeCommandlineColors(QString::fromLatin1(outputLine));
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
                m_description.append(' ' + Tr::tr("(iteration %1)").arg(m_iteration));
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
        testResult->setDescription(Tr::tr("Test execution took %1").arg(match.captured(2)));
        reportResult(testResult);
        m_currentTestSuite.clear();
        m_currentTestCase.clear();
    } else if (ExactMatch match = newTestStarts.match(line)) {
        setCurrentTestSuite(match.captured(1));
        TestResultPtr testResult = createDefaultResult();
        testResult->setResult(ResultType::TestStart);
        if (m_iteration > 1) {
            testResult->setDescription(Tr::tr("Repeating test suite %1 (iteration %2)")
                                       .arg(m_currentTestSuite).arg(m_iteration));
        } else {
            testResult->setDescription(Tr::tr("Executing test suite %1").arg(m_currentTestSuite));
        }
        reportResult(testResult);
    } else if (ExactMatch match = newTestSetStarts.match(line)) {
        m_testSetStarted = true;
        setCurrentTestCase(match.captured(1));
        TestResultPtr testResult = TestResultPtr(new GTestResult(QString(), m_projectFile,
                                                                 QString()));
        testResult->setResult(ResultType::MessageCurrentTest);
        testResult->setDescription(Tr::tr("Entering test case %1").arg(m_currentTestCase));
        reportResult(testResult);
        m_description.clear();
    } else if (ExactMatch match = testSetSuccess.match(line)) {
        m_testSetStarted = false;
        TestResultPtr testResult = createDefaultResult();
        testResult->setResult(ResultType::Pass);
        testResult->setDescription(m_description);
        reportResult(testResult);
        m_description.clear();
        testResult = createDefaultResult();
        testResult->setResult(ResultType::MessageInternal);
        testResult->setDescription(Tr::tr("Execution took %1.").arg(match.captured(2)));
        reportResult(testResult);
        m_futureInterface.setProgressValue(m_futureInterface.progressValue() + 1);
    } else if (ExactMatch match = testSetFail.match(line)) {
        m_testSetStarted = false;
        TestResultPtr testResult = createDefaultResult();
        testResult->setResult(ResultType::Fail);
        m_description.chop(1);
        handleDescriptionAndReportResult(testResult);
        testResult = createDefaultResult();
        testResult->setResult(ResultType::MessageInternal);
        testResult->setDescription(Tr::tr("Execution took %1.").arg(match.captured(2)));
        reportResult(testResult);
        m_futureInterface.setProgressValue(m_futureInterface.progressValue() + 1);
    } else if (ExactMatch match = testSetSkipped.match(line)) {
        if (!m_testSetStarted)  // ignore SKIPPED at summary
            return;
        m_testSetStarted = false;
        TestResultPtr testResult = createDefaultResult();
        testResult->setResult(ResultType::Skip);
        m_description.chop(1);
        m_description.prepend(match.captured(1) + '\n');
        handleDescriptionAndReportResult(testResult);
        testResult = createDefaultResult();
        testResult->setResult(ResultType::MessageInternal);
        testResult->setDescription(Tr::tr("Execution took %1.").arg(match.captured(2)));
        reportResult(testResult);
    } else if (ExactMatch match = logging.match(line)) {
        const QString severity = match.captured(1).trimmed();
        ResultType type = ResultType::Invalid;
        switch (severity.at(0).toLatin1()) {
        case 'I': type = ResultType::MessageInfo; break;    // INFO
        case 'W': type = ResultType::MessageWarn; break;    // WARNING
        case 'E': type = ResultType::MessageError; break;   // ERROR
        case 'F': type = ResultType::MessageFatal; break;   // FATAL
        }
        TestResultPtr testResult = createDefaultResult();
        testResult->setResult(type);
        testResult->setLine(match.captured(3).toInt());
        const Utils::FilePath file = constructSourceFilePath(m_buildDir, match.captured(2));
        if (file.exists())
            testResult->setFileName(file);
        testResult->setDescription(match.captured(4));
        reportResult(testResult);
    } else if (ExactMatch match = testDeath.match(line)) {
        m_description.append(line);
        m_description.append('\n');
    }
}

void GTestOutputReader::processStdError(const QByteArray &outputLine)
{
    // we need to process the output, GTest may uses both out streams
    checkForSanitizerOutput(outputLine);
    processOutputLine(outputLine);
    emit newOutputLineAvailable(outputLine, OutputChannel::StdErr);
}

TestResultPtr GTestOutputReader::createDefaultResult() const
{
    GTestResult *result = new GTestResult(id(), m_projectFile, m_currentTestSuite);
    result->setTestCaseName(m_currentTestCase);
    result->setIteration(m_iteration);

    const ITestTreeItem *testItem = result->findTestTreeItem();

    if (testItem && testItem->line()) {
        result->setFileName(testItem->filePath());
        result->setLine(testItem->line());
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

void GTestOutputReader::handleDescriptionAndReportResult(TestResultPtr testResult)
{
    static const QRegularExpression failureLocation("^(.*):(\\d+): Failure$");
    static const QRegularExpression skipOrErrorLocation("^(.*)\\((\\d+)\\): (Skipped|error:.*)$");

    QStringList resultDescription;

    for (const QString &output : m_description.split('\n')) {
        QRegularExpressionMatch innerMatch = failureLocation.match(output);
        if (!innerMatch.hasMatch()) {
             innerMatch = skipOrErrorLocation.match(output);
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
        const Utils::FilePath file = constructSourceFilePath(m_buildDir, innerMatch.captured(1));
        if (file.exists())
            testResult->setFileName(file);
        resultDescription << output;
    }
    testResult->setDescription(resultDescription.join('\n'));
    reportResult(testResult);
    m_description.clear();
}

} // namespace Internal
} // namespace Autotest
