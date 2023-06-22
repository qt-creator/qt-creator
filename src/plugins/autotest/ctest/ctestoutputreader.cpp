// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctestoutputreader.h"

#include "../autotesttr.h"
#include "../testframeworkmanager.h"
#include "../testtreeitem.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <utils/qtcassert.h>

#include <QRegularExpression>

using namespace Utils;

namespace Autotest {
namespace Internal {

static ResultHooks::FindTestItemHook findTestItemHook(const QString &testCaseName)
{
    return [=](const TestResult &result) -> ITestTreeItem * {
        Q_UNUSED(result)
        ITestTool *testTool = TestFrameworkManager::testToolForBuildSystemId(
                    CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
        QTC_ASSERT(testTool, return nullptr);
        const ITestTreeItem *rootNode = testTool->rootNode();
        if (!rootNode)
            return nullptr;

        return rootNode->findFirstLevelChild([&](const ITestTreeItem *item) {
            return item && item->name() == testCaseName;
        });
    };
}

static ResultHooks::DirectParentHook directParentHook()
{
    return [=](const TestResult &result, const TestResult &, bool *) -> bool {
        return result.result() == ResultType::TestStart;
    };
}

class CTestResult : public TestResult
{
public:
    CTestResult(const QString &id, const QString &project, const QString &testCaseName)
        : TestResult(id, project, {{}, {}, findTestItemHook(testCaseName), directParentHook()})
    {}
};

CTestOutputReader::CTestOutputReader(Process *testApplication,
                                     const FilePath &buildDirectory)
    : TestOutputReader(testApplication, buildDirectory)
{
}

void CTestOutputReader::processOutputLine(const QByteArray &outputLine)
{
    static const QRegularExpression verbose("^\\d+: .*$");
    static const QRegularExpression testProject("^Test project (.*)$");
    static const QRegularExpression testCase1("^test (?<current>\\d+)$");
    static const QRegularExpression testCase2("^    Start\\s+(?<current>\\d+): (?<name>.*)$");
    static const QRegularExpression testResult("^\\s*(?<first>\\d+/\\d+)? "
                                               "Test\\s+#(?<current>\\d+): (.*) (\\.+)\\s*"
                                               "(Passed|\\*\\*\\*Failed|\\*\\*\\*Not Run|"
                                               ".*\\*\\*\\*Exception:.*)\\s+(.*) sec$");
    static const QRegularExpression testCrash("^\\s*\\d+/\\d+ Test\\s+#\\d+: (.*) (\\.+)\\s*"
                                              "Exit code .*$");
    static const QRegularExpression summary("^\\d+% tests passed, (\\d+) tests failed "
                                            "out of (\\d+)");
    static const QRegularExpression summaryTime("^Total Test time .* =\\s+(.*) sec$");

    const QString line = removeCommandlineColors(QString::fromLatin1(outputLine));
    if (line.trimmed().isEmpty())
        return;

    struct ExactMatch : public QRegularExpressionMatch
    {
        ExactMatch(const QRegularExpressionMatch &other) : QRegularExpressionMatch(other) {}
        operator bool() const { return hasMatch(); }
    };

    if (ExactMatch match = verbose.match(line)) { // ignore verbose output for visual display
        return;
    } else if (ExactMatch match = testProject.match(line)) {
        if (!m_testName.isEmpty()) // possible?
            sendCompleteInformation();
        m_project = match.captured(1);
        TestResult testResult = createDefaultResult();
        testResult.setResult(ResultType::TestStart);
        testResult.setDescription(Tr::tr("Running tests for \"%1\".").arg(m_project));
        reportResult(testResult);
    } else if (ExactMatch match = testCase1.match(line)) {
        int current = match.captured("current").toInt();
        if (m_result != ResultType::Invalid && m_currentTestNo != -1 && current != m_currentTestNo)
            sendCompleteInformation();
        m_currentTestNo = -1;
    } else if (ExactMatch match = testCase2.match(line)) {
        int current = match.captured("current").toInt();
        if (m_result != ResultType::Invalid && m_currentTestNo != -1 && current != m_currentTestNo)
            sendCompleteInformation();
        m_currentTestNo = -1;
    } else if (ExactMatch match = testResult.match(line)) {
        int current = match.captured("current").toInt();
        if (m_result != ResultType::Invalid && m_currentTestNo != -1 && current != m_currentTestNo)
            sendCompleteInformation();
        if (!m_description.isEmpty() && match.captured("first").isEmpty())
            m_description.append('\n').append(match.captured());
        else
            m_description = match.captured();
        m_currentTestNo = current;
        m_testName = match.captured(3);
        const QString resultType = match.captured(5);
        if (resultType == "Passed")
            m_result = ResultType::Pass;
        else if (resultType == "***Failed" || resultType == "***Not Run")
            m_result = ResultType::Fail;
        else
            m_result = ResultType::MessageFatal;
    } else if (ExactMatch match = summary.match(line)) {
        if (!m_testName.isEmpty())
            sendCompleteInformation();
        TestResult testResult = createDefaultResult();
        testResult.setResult(ResultType::MessageInfo);
        testResult.setDescription(match.captured());
        reportResult(testResult);
        int failed = match.captured(1).toInt();
        int testCount = match.captured(2).toInt();
        m_summary.insert(ResultType::Fail, failed);
        m_summary.insert(ResultType::Pass, testCount - failed);
    } else if (ExactMatch match = summaryTime.match(line)) {
        if (!m_testName.isEmpty()) // possible?
            sendCompleteInformation();
        TestResult testResult = createDefaultResult();
        testResult.setResult(ResultType::TestEnd);
        testResult.setDescription(match.captured());
        reportResult(testResult);
    } else if (ExactMatch match = testCrash.match(line)) {
        m_description = match.captured();
        m_testName = match.captured(1);
        m_result = ResultType::Fail;
        m_expectExceptionFromCrash = true;
    } else {
        if (!m_description.isEmpty())
            m_description.append('\n');
        m_description.append(line);
        if (m_expectExceptionFromCrash) {
            if (QTC_GUARD(line.startsWith("***Exception:")))
                m_expectExceptionFromCrash = false;
        }
    }
}

TestResult CTestOutputReader::createDefaultResult() const
{
    return CTestResult(id(), m_project, m_testName);
}

void CTestOutputReader::sendCompleteInformation()
{
    // some verbose output we did not ignore
    if (m_result == ResultType::Invalid) {
        QTC_CHECK(m_currentTestNo == -1 && m_testName.isEmpty());
        return;
    }
    TestResult testResult = createDefaultResult();
    testResult.setResult(m_result);
    testResult.setDescription(m_description);
    reportResult(testResult);
    m_testName.clear();
    m_description.clear();
    m_currentTestNo = -1;
    m_result = ResultType::Invalid;
}

} // namespace Internal
} // namespace Autotest
