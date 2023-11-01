// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "boosttestoutputreader.h"

#include "boosttestframework.h"
#include "boosttestresult.h"

#include "../autotesttr.h"
#include "../testtreeitem.h"

#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>
#include <QRegularExpression>

using namespace Utils;

namespace Autotest {
namespace Internal {

static Q_LOGGING_CATEGORY(orLog, "qtc.autotest.boost.outputreader", QtWarningMsg)

BoostTestOutputReader::BoostTestOutputReader(Process *testApplication,
                                             const FilePath &buildDirectory,
                                             const FilePath &projectFile,
                                             LogLevel log, ReportLevel report)
    : TestOutputReader(testApplication, buildDirectory)
    , m_projectFile(projectFile)
    , m_logLevel(log)
    , m_reportLevel(report)
{
}

// content of "error:..." / "info:..." / ... messages
static QString caseFromContent(const QString &content)
{
    const int length = content.length();
    if (content.startsWith("last checkpoint:")) {
        int index = content.indexOf('"');
        if (index != 17 || length <= 18) {
            qCDebug(orLog) << "double quote position" << index << " or content length" << length
                           << "wrong on content" << content;
            return {};
        }
        index = content.indexOf('"', 18);
        if (index == -1) {
            qCDebug(orLog) << "no closing double quote" << content;
            return {};
        }
        return content.mid(18, index - 1);
    }

    int index = content.indexOf(": in ");
    if (index == -1) // "info: check true has passed"
        return {};

    if (index <= 4 || length < index + 4) {
        qCDebug(orLog) << "unexpected position" << index << "for info" << content;
        return {};
    }

    QString result = content.mid(index + 5);
    static const QRegularExpression functionName("\"(.+)\":.*");
    const QRegularExpressionMatch matcher = functionName.match(result);
    if (!matcher.hasMatch()) {
        qCDebug(orLog) << "got no match";
        return {};
    }
    return matcher.captured(1);
}

void BoostTestOutputReader::sendCompleteInformation()
{
    QTC_ASSERT(m_result != ResultType::Invalid, return);
    BoostTestResult result(id(), m_currentModule, m_projectFile, m_currentTest, m_currentSuite);
    if (m_lineNumber) {
        result.setLine(m_lineNumber);
        result.setFileName(m_fileName);
    } else if (const ITestTreeItem *it = result.findTestTreeItem()) {
        result.setLine(it->line());
        result.setFileName(it->filePath());
    }

    result.setDescription(m_description);
    result.setResult(m_result);
    reportResult(result);
    m_result = ResultType::Invalid;
}

void BoostTestOutputReader::handleMessageMatch(const QRegularExpressionMatch &match)
{
    m_fileName = constructSourceFilePath(m_buildDir, match.captured(1));
    m_lineNumber = match.captured(2).toInt();

    const QString &content = match.captured(3);
    if (content.startsWith("info:")) {
        if (m_currentTest.isEmpty() || m_logLevel > LogLevel::UnitScope) {
            QString tmp = caseFromContent(content);
            if (!tmp.isEmpty())
                m_currentTest = tmp;
        }
        m_result = ResultType::Pass;
        m_description = content;
    } else if (content.startsWith("error:")) {
        if (m_currentTest.isEmpty() || m_logLevel > LogLevel::UnitScope)
            m_currentTest = caseFromContent(content);
        m_result = ResultType::Fail;
        if (m_reportLevel == ReportLevel::No)
            ++m_summary[ResultType::Fail];
        m_description = content;
    } else if (content.startsWith("fatal error:")) {
        if (m_currentTest.isEmpty() || m_logLevel > LogLevel::UnitScope)
            m_currentTest = caseFromContent(content);
        m_result = ResultType::MessageFatal;
        ++m_summary[ResultType::MessageFatal];
        m_description = content;
    } else if (content.startsWith("last checkpoint:")) {
        if (m_currentTest.isEmpty() || m_logLevel > LogLevel::UnitScope)
            m_currentTest = caseFromContent(content);
        m_result = ResultType::MessageInfo;
        m_description = content;
    } else if (content.startsWith("Entering")) {
        m_result = ResultType::TestStart;
        const QString type = match.captured(8);
        if (type == "case") {
            m_currentTest = match.captured(9);
            m_description = Tr::tr("Executing test case %1").arg(m_currentTest);
        } else if (type == "suite") {
            if (m_currentSuite.isEmpty())
                m_currentSuite = match.captured(9);
            else
                m_currentSuite.append("/").append(match.captured(9));
            m_currentTest.clear();
            m_description = Tr::tr("Executing test suite %1").arg(m_currentSuite);
        }
    } else if (content.startsWith("Leaving")) {
        const QString type = match.captured(10);
        if (type == "case") {
            if (m_currentTest != match.captured(11) && m_currentTest.isEmpty())
                m_currentTest = match.captured(11);
            m_result = ResultType::TestEnd;
            m_description = Tr::tr("Test execution took %1.").arg(match.captured(12));
        } else if (type == "suite") {
            if (!m_currentSuite.isEmpty()) {
                int index = m_currentSuite.lastIndexOf('/');
                if (QTC_GUARD(m_currentSuite.mid(index + 1) == match.captured(11))) {
                    if (index == -1)
                        m_currentSuite.clear();
                    else
                        m_currentSuite = m_currentSuite.left(index);
                }
            } else if (match.capturedLength(11)) { // FIXME remove this branch?
                QTC_CHECK(false);
                m_currentSuite = match.captured(11);
            }
            m_currentTest.clear();
            m_result = ResultType::TestEnd;
            m_description = Tr::tr("Test suite execution took %1.").arg(match.captured(12));
        }
    } else if (content.startsWith("Test case ")) {
        m_currentTest = match.captured(4);
        m_result = ResultType::Skip;
        if (m_reportLevel == ReportLevel::Confirm || m_reportLevel == ReportLevel::No)
            ++m_summary[ResultType::Skip];
        m_description = content;
    }

    if (m_result != ResultType::Invalid) // we got a new result
        sendCompleteInformation();
}

void BoostTestOutputReader::processOutputLine(const QByteArray &outputLine)
{
    static const QRegularExpression newTestStart("^Running (\\d+) test cases?\\.\\.\\.$");
    static const QRegularExpression dependency("^Including test case (.+) as a dependency of "
                                               "test case (.+)$");
    static const QRegularExpression messages("^(.+)\\((\\d+)\\): (info: (.+)|error: (.+)|"
                                             "fatal error: (.+)|last checkpoint: (.+)"
                                             "|Entering test (case|suite) \"(.+)\""
                                             "|Leaving test (case|suite) \"(.+)\"; testing time: (\\d+.+)"
                                             "|Test case \"(.+)\" is skipped because .+$)$");
    static const QRegularExpression moduleMssg("^(Entering test module \"(.+)\"|"
                                               "Leaving test module \"(.+)\"; testing time: (\\d+.+))$");
    static const QRegularExpression noAssertion("^Test case (.*) did not check any assertions$");

    static const QRegularExpression summaryPreamble("^\\s*Test (module|suite|case) \"(.*)\" has "
                                                    "(failed|passed)( with:)?$");
    static const QRegularExpression summarySkip("^\\s+Test case \"(.*)\" was skipped$");
    static const QRegularExpression summaryDetail("^\\s+(\\d+) test cases? out of (\\d+) "
                                                  "(failed|passed|skipped)$");
    static const QRegularExpression summaryAssertion("^\\s+(\\d+) assertions? out of (\\d+) "
                                                     "(failed|passed)$");

    static const QRegularExpression finish("^\\*{3} (\\d+) failure(s are| is) detected in the "
                                           "test module \"(.*)\"$");
    const QString noErrors("*** No errors detected");

    const QString line = removeCommandlineColors(QString::fromUtf8(outputLine));
    if (line.trimmed().isEmpty())
        return;

    QRegularExpressionMatch match = messages.match(line);
    if (match.hasMatch()) {
        handleMessageMatch(match);
        return;
    }

    match = dependency.match(line);
    if (match.hasMatch()) {
        if (m_result != ResultType::Invalid)
            sendCompleteInformation();
        BoostTestResult result(id(), m_currentModule, m_projectFile);
        result.setDescription(match.captured(0));
        result.setResult(ResultType::MessageInfo);
        reportResult(result);
        return;
    }

    match = newTestStart.match(line);
    if (match.hasMatch()) {
        if (m_result != ResultType::Invalid)
            sendCompleteInformation();
        m_testCaseCount = match.captured(1).toInt();
        m_description.clear();
        return;
    }

    match = moduleMssg.match(line);
    if (match.hasMatch()) {
        if (m_result != ResultType::Invalid)
            sendCompleteInformation();
        if (match.captured(1).startsWith("Entering")) {
            m_currentModule = match.captured(2);
            BoostTestResult result(id(), m_currentModule, m_projectFile);
            result.setDescription(Tr::tr("Executing test module %1").arg(m_currentModule));
            result.setResult(ResultType::TestStart);
            reportResult(result);
            m_description.clear();
        } else {
            QTC_CHECK(m_currentModule == match.captured(3));
            BoostTestResult result(id(), m_currentModule, m_projectFile);
            result.setDescription(Tr::tr("Test module execution took %1.").arg(match.captured(4)));
            result.setResult(ResultType::TestEnd);
            reportResult(result);

            m_currentTest.clear();
            m_currentSuite.clear();
            m_currentModule.clear();
            m_description.clear();
        }
        return;
    }

    match = noAssertion.match(line);
    if (match.hasMatch()) {
        if (m_result != ResultType::Invalid)
            sendCompleteInformation();
        const QString caseWithOptionalSuite = match.captured(1);
        int index = caseWithOptionalSuite.lastIndexOf('/');
        if (index == -1) {
            QTC_CHECK(caseWithOptionalSuite == m_currentTest);
        } else {
            QTC_CHECK(caseWithOptionalSuite.mid(index + 1) == m_currentTest);
            int sIndex = caseWithOptionalSuite.lastIndexOf('/', index - 1);
            if (sIndex == -1) {
                QTC_CHECK(caseWithOptionalSuite.left(index) == m_currentSuite);
                m_currentSuite = caseWithOptionalSuite.left(index); // FIXME should not be necessary - but we currently do not care for the whole suite path
            } else {
                QTC_CHECK(caseWithOptionalSuite.mid(sIndex + 1, index - sIndex - 1) == m_currentSuite);
            }
        }
        createAndReportResult(match.captured(0), ResultType::MessageWarn);
        return;
    }

    match = summaryPreamble.match(line);
    if (match.hasMatch()) {
        createAndReportResult(match.captured(0), ResultType::MessageInfo);
        if (m_reportLevel == ReportLevel::Detailed || match.captured(4).isEmpty()) {
            if (match.captured(1) == "case") {
                if (match.captured(3) == "passed")
                    ++m_summary[ResultType::Pass];
                else
                    ++m_summary[ResultType::Fail];
            }
        }
        return;
    }

    match = summaryDetail.match(line);
    if (match.hasMatch()) {
        createAndReportResult(match.captured(0), ResultType::MessageInfo);
        int report = match.captured(1).toInt();
        QString type = match.captured(3);
        if (m_reportLevel != ReportLevel::Detailed) {
            if (type == "passed")
                m_summary[ResultType::Pass] += report;
            else if (type == "failed")
                m_summary[ResultType::Fail] += report;
            else if (type == "skipped")
                m_summary[ResultType::Skip] += report;
        }

        return;
    }

    match = summaryAssertion.match(line);
    if (match.hasMatch()) {
        createAndReportResult(match.captured(0), ResultType::MessageInfo);
        return;
    }

    match = summarySkip.match(line);
    if (match.hasMatch()) {
        createAndReportResult(match.captured(0), ResultType::MessageInfo);
        if (m_reportLevel == ReportLevel::Detailed)
            ++m_summary[ResultType::Skip];
        return;
    }

    match = finish.match(line);
    if (match.hasMatch()) {
        if (m_result != ResultType::Invalid)
            sendCompleteInformation();
        BoostTestResult result(id(), {}, m_projectFile);
        const int failed = match.captured(1).toInt();
        const int fatals = m_summary.value(ResultType::MessageFatal);
        QString txt
            = Tr::tr("%n failure(s) detected in %1.", nullptr, failed).arg(match.captured(3));
        const int passed = qMax(0, m_testCaseCount - failed);
        if (m_testCaseCount != -1)
            txt.append(' ').append(Tr::tr("%1 tests passed.").arg(passed));
        result.setDescription(txt);
        result.setResult(ResultType::MessageInfo);
        reportResult(result);
        if (m_reportLevel == ReportLevel::Confirm) { // for the final summary
            m_summary[ResultType::Pass] += passed;
            m_summary[ResultType::Fail] += failed - fatals;
        }
        m_testCaseCount = -1;
        return;
    }

    if (line == noErrors) {
        if (m_result != ResultType::Invalid)
            sendCompleteInformation();
        BoostTestResult result(id(), {}, m_projectFile);
        QString txt = Tr::tr("No errors detected.");
        if (m_testCaseCount != -1)
            txt.append(' ').append(Tr::tr("%1 tests passed.").arg(m_testCaseCount));
        result.setDescription(txt);
        result.setResult(ResultType::MessageInfo);
        reportResult(result);
        if (m_reportLevel == ReportLevel::Confirm) // for the final summary
            m_summary.insert(ResultType::Pass, m_testCaseCount);
        return;
    }

    // some plain output...
    if (!m_description.isEmpty())
        m_description.append('\n');
    m_description.append(line);
}

void BoostTestOutputReader::processStdError(const QByteArray &outputLine)
{
    // we need to process the output, Boost UTF uses both out streams
    checkForSanitizerOutput(outputLine);
    processOutputLine(outputLine);
    emit newOutputLineAvailable(outputLine, OutputChannel::StdErr);
}

TestResult BoostTestOutputReader::createDefaultResult() const
{
    return BoostTestResult(id(), m_currentModule, m_projectFile, m_currentTest, m_currentSuite);
}

void BoostTestOutputReader::onDone(int exitCode)
{
    if (m_reportLevel == ReportLevel::No && m_testCaseCount != -1) {
        const int reportedFailsAndSkips = m_summary[ResultType::Fail] + m_summary[ResultType::Skip];
        m_summary.insert(ResultType::Pass, m_testCaseCount - reportedFailsAndSkips);
    }
    // boost::exit_success (0), boost::exit_test_failure (201)
    // or boost::exit_exception_failure (200)
    // be graceful and do not add a fatal for exit_test_failure
    if (m_logLevel == LogLevel::Nothing && m_reportLevel == ReportLevel::No) {
        switch (exitCode) {
        case 0:
            reportNoOutputFinish(Tr::tr("Running tests exited with %1.").arg("boost::exit_success"),
                                 ResultType::Pass);
            break;
        case 200:
            reportNoOutputFinish(
                        Tr::tr("Running tests exited with %1.").arg("boost::exit_test_exception"),
                        ResultType::MessageFatal);
            break;
        case 201:
            reportNoOutputFinish(Tr::tr("Running tests exited with %1.")
                                 .arg("boost::exit_test_failure"), ResultType::Fail);
            break;
        }
    } else if (exitCode != 0 && exitCode != 201 && !m_description.isEmpty()) {
        if (m_description.startsWith("Test setup error:")) {
            createAndReportResult(m_description + '\n' + Tr::tr("Executable: %1")
                                  .arg(id()), ResultType::MessageWarn);
        } else {
            createAndReportResult(Tr::tr("Running tests failed.\n%1\nExecutable: %2")
                                  .arg(m_description).arg(id()), ResultType::MessageFatal);
        }
    }
}

void BoostTestOutputReader::reportNoOutputFinish(const QString &description, ResultType type)
{
    BoostTestResult result(id(), m_currentModule, m_projectFile,
                           Tr::tr("Running tests without output."));
    result.setDescription(description);
    result.setResult(type);
    reportResult(result);
}

} // namespace Internal
} // namespace Autotest
