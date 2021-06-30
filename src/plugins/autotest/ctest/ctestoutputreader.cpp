/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "ctestoutputreader.h"

#include "ctesttreeitem.h"
#include "../testframeworkmanager.h"
#include "../testresult.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <utils/qtcassert.h>

#include <QRegularExpression>

namespace Autotest {
namespace Internal {

class CTestResult : public TestResult
{
public:
    CTestResult(const QString &id, const QString &project, const QString &testCase)
        : TestResult(id, project)
        , m_testCase(testCase)
    {}

    bool isDirectParentOf(const TestResult *other, bool *needsIntermediate) const override
    {
        if (!TestResult::isDirectParentOf(other, needsIntermediate))
            return false;
        return result() == ResultType::TestStart;
    }

    const ITestTreeItem *findTestTreeItem() const override
    {
        ITestTool *testTool = TestFrameworkManager::testToolForBuildSystemId(
                    CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
        QTC_ASSERT(testTool, return nullptr);
        const ITestTreeItem *rootNode = testTool->rootNode();
        if (!rootNode)
            return nullptr;

        return rootNode->findFirstLevelChild([this](const ITestTreeItem *item) {
            return item && item->name() == m_testCase;
        });
    }

private:
    QString m_testCase;
};

CTestOutputReader::CTestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                                     QProcess *testApplication,
                                     const Utils::FilePath &buildDirectory)
    : TestOutputReader(futureInterface, testApplication, buildDirectory)
{
}

void CTestOutputReader::processOutputLine(const QByteArray &outputLine)
{
    static const QRegularExpression testProject("^Test project (.*)$");
    static const QRegularExpression testCase("^(test \\d+)|(    Start\\s+\\d+: .*)$");
    static const QRegularExpression testResult("^\\s*\\d+/\\d+ Test\\s+#\\d+: (.*) (\\.+)\\s*"
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

    if (ExactMatch match = testProject.match(line)) {
        if (!m_testName.isEmpty()) // possible?
            sendCompleteInformation();
        m_project = match.captured(1);
        TestResultPtr testResult = createDefaultResult();
        testResult->setResult(ResultType::TestStart);
        testResult->setDescription(tr("Running tests for %1").arg(m_project));
        reportResult(testResult);
    } else if (ExactMatch match = testCase.match(line)) {
        if (!m_testName.isEmpty())
            sendCompleteInformation();
    } else if (ExactMatch match = testResult.match(line)) {
        m_description = match.captured();
        m_testName = match.captured(1);
        const QString resultType = match.captured(3);
        if (resultType == "Passed")
            m_result = ResultType::Pass;
        else if (resultType == "***Failed" || resultType == "***Not Run")
            m_result = ResultType::Fail;
        else
            m_result = ResultType::MessageFatal;
    } else if (ExactMatch match = summary.match(line)) {
        if (!m_testName.isEmpty())
            sendCompleteInformation();
        TestResultPtr testResult = createDefaultResult();
        testResult->setResult(ResultType::MessageInfo);
        testResult->setDescription(match.captured());
        reportResult(testResult);
        int failed = match.captured(1).toInt();
        int testCount = match.captured(2).toInt();
        m_summary.insert(ResultType::Fail, failed);
        m_summary.insert(ResultType::Pass, testCount - failed);
    } else if (ExactMatch match = summaryTime.match(line)) {
        if (!m_testName.isEmpty()) // possible?
            sendCompleteInformation();
        TestResultPtr testResult = createDefaultResult();
        testResult->setResult(ResultType::TestEnd);
        testResult->setDescription(match.captured());
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

TestResultPtr CTestOutputReader::createDefaultResult() const
{
    return TestResultPtr(new CTestResult(id(), m_project, m_testName));
}

void CTestOutputReader::sendCompleteInformation()
{
    TestResultPtr testResult = createDefaultResult();
    testResult->setResult(m_result);
    testResult->setDescription(m_description);
    reportResult(testResult);
    m_testName.clear();
    m_description.clear();
    m_result = ResultType::Invalid;
}

} // namespace Internal
} // namespace Autotest
