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

#include "testresult.h"

#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

namespace Autotest {
namespace Internal {

FaultyTestResult::FaultyTestResult(Result::Type result, const QString &description)
{
    setResult(result);
    setDescription(description);
}

TestResult::TestResult()
    : TestResult(QString())
{
}

TestResult::TestResult(const QString &name)
    : m_name(name)
{
}

const QString TestResult::outputString(bool selected) const
{
    return selected ? m_description : m_description.split('\n').first();
}

Result::Type TestResult::resultFromString(const QString &resultString)
{
    if (resultString == "pass")
        return Result::Pass;
    if (resultString == "fail")
        return Result::Fail;
    if (resultString == "xfail")
        return Result::ExpectedFail;
    if (resultString == "xpass")
        return Result::UnexpectedPass;
    if (resultString == "skip")
        return Result::Skip;
    if (resultString == "qdebug")
        return Result::MessageDebug;
    if (resultString == "qinfo")
        return Result::MessageInfo;
    if (resultString == "warn" || resultString == "qwarn")
        return Result::MessageWarn;
    if (resultString == "qfatal")
        return Result::MessageFatal;
    if (resultString == "system")
        return Result::MessageSystem;
    if (resultString == "bpass")
        return Result::BlacklistedPass;
    if (resultString == "bfail")
        return Result::BlacklistedFail;
    qDebug("Unexpected test result: %s", qPrintable(resultString));
    return Result::Invalid;
}

Result::Type TestResult::toResultType(int rt)
{
    if (rt < Result::FIRST_TYPE || rt > Result::LAST_TYPE)
        return Result::Invalid;

    return (Result::Type)rt;
}

QString TestResult::resultToString(const Result::Type type)
{
    switch (type) {
    case Result::Pass:
    case Result::MessageTestCaseSuccess:
        return QString("PASS");
    case Result::Fail:
    case Result::MessageTestCaseFail:
        return QString("FAIL");
    case Result::ExpectedFail:
        return QString("XFAIL");
    case Result::UnexpectedPass:
        return QString("XPASS");
    case Result::Skip:
        return QString("SKIP");
    case Result::Benchmark:
        return QString("BENCH");
    case Result::MessageDebug:
        return QString("DEBUG");
    case Result::MessageInfo:
        return QString("INFO");
    case Result::MessageWarn:
    case Result::MessageTestCaseWarn:
        return QString("WARN");
    case Result::MessageFatal:
        return QString("FATAL");
    case Result::MessageSystem:
        return QString("SYSTEM");
    case Result::BlacklistedPass:
        return QString("BPASS");
    case Result::BlacklistedFail:
        return QString("BFAIL");
    default:
        if (type >= Result::INTERNAL_MESSAGES_BEGIN && type <= Result::INTERNAL_MESSAGES_END)
            return QString();
        return QString("UNKNOWN");
    }
}

QColor TestResult::colorForType(const Result::Type type)
{
    if (type >= Result::INTERNAL_MESSAGES_BEGIN && type <= Result::INTERNAL_MESSAGES_END)
        return QColor("transparent");

    Utils::Theme *creatorTheme = Utils::creatorTheme();
    switch (type) {
    case Result::Pass:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestPassTextColor);
    case Result::Fail:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestFailTextColor);
    case Result::ExpectedFail:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestXFailTextColor);
    case Result::UnexpectedPass:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestXPassTextColor);
    case Result::Skip:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestSkipTextColor);
    case Result::MessageDebug:
    case Result::MessageInfo:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestDebugTextColor);
    case Result::MessageWarn:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestWarnTextColor);
    case Result::MessageFatal:
    case Result::MessageSystem:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestFatalTextColor);
    case Result::BlacklistedPass:
    case Result::BlacklistedFail:
    default:
        return creatorTheme->color(Utils::Theme::OutputPanes_StdOutTextColor);
    }
}

bool TestResult::isMessageCaseStart(const Result::Type type)
{
    return type == Result::MessageTestCaseStart || type == Result::MessageTestCaseSuccess
            || type == Result::MessageTestCaseFail || type == Result::MessageTestCaseWarn
            || type == Result::MessageIntermediate;
}

bool TestResult::isDirectParentOf(const TestResult *other, bool * /*needsIntermediate*/) const
{
    QTC_ASSERT(other, return false);
    return m_name == other->m_name;
}

bool TestResult::isIntermediateFor(const TestResult *other) const
{
    QTC_ASSERT(other, return false);
    return m_name == other->m_name;
}

TestResult *TestResult::createIntermediateResultFor(const TestResult *other)
{
    QTC_ASSERT(other, return nullptr);
    TestResult *intermediate = new TestResult(other->m_name);
    return intermediate;
}

} // namespace Internal
} // namespace Autotest
