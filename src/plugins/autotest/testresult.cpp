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

TestResult::TestResult(const QString &id, const QString &name)
    : m_id(id)
    , m_name(name)
{
}

const QString TestResult::outputString(bool selected) const
{
    if (m_result == ResultType::Application)
        return m_id;
    return selected ? m_description : m_description.split('\n').first();
}

const TestTreeItem *TestResult::findTestTreeItem() const
{
    return nullptr;
}

ResultType TestResult::resultFromString(const QString &resultString)
{
    if (resultString == "pass")
        return ResultType::Pass;
    if (resultString == "fail" || resultString == "fail!")
        return ResultType::Fail;
    if (resultString == "xfail")
        return ResultType::ExpectedFail;
    if (resultString == "xpass")
        return ResultType::UnexpectedPass;
    if (resultString == "skip")
        return ResultType::Skip;
    if (resultString == "result")
        return ResultType::Benchmark;
    if (resultString == "qdebug")
        return ResultType::MessageDebug;
    if (resultString == "qinfo" || resultString == "info")
        return ResultType::MessageInfo;
    if (resultString == "warn" || resultString == "qwarn" || resultString == "warning")
        return ResultType::MessageWarn;
    if (resultString == "qfatal")
        return ResultType::MessageFatal;
    if ((resultString == "system") || (resultString == "qsystem"))
        return ResultType::MessageSystem;
    if (resultString == "bpass")
        return ResultType::BlacklistedPass;
    if (resultString == "bfail")
        return ResultType::BlacklistedFail;
    if (resultString == "bxpass")
        return ResultType::BlacklistedXPass;
    if (resultString == "bxfail")
        return ResultType::BlacklistedXFail;
    qDebug("Unexpected test result: %s", qPrintable(resultString));
    return ResultType::Invalid;
}

ResultType TestResult::toResultType(int rt)
{
    if (rt < int(ResultType::FIRST_TYPE) || rt > int(ResultType::LAST_TYPE))
        return ResultType::Invalid;

    return ResultType(rt);
}

QString TestResult::resultToString(const ResultType type)
{
    switch (type) {
    case ResultType::Pass:
        return QString("PASS");
    case ResultType::Fail:
        return QString("FAIL");
    case ResultType::ExpectedFail:
        return QString("XFAIL");
    case ResultType::UnexpectedPass:
        return QString("XPASS");
    case ResultType::Skip:
        return QString("SKIP");
    case ResultType::Benchmark:
        return QString("BENCH");
    case ResultType::MessageDebug:
        return QString("DEBUG");
    case ResultType::MessageInfo:
        return QString("INFO");
    case ResultType::MessageWarn:
        return QString("WARN");
    case ResultType::MessageFatal:
        return QString("FATAL");
    case ResultType::MessageSystem:
        return QString("SYSTEM");
    case ResultType::MessageError:
        return QString("ERROR");
    case ResultType::BlacklistedPass:
        return QString("BPASS");
    case ResultType::BlacklistedFail:
        return QString("BFAIL");
    case ResultType::BlacklistedXPass:
        return QString("BXPASS");
    case ResultType::BlacklistedXFail:
        return QString("BXFAIL");
    case ResultType::MessageLocation:
    case ResultType::Application:
        return QString();
    default:
        if (type >= ResultType::INTERNAL_MESSAGES_BEGIN && type <= ResultType::INTERNAL_MESSAGES_END)
            return QString();
        return QString("UNKNOWN");
    }
}

QColor TestResult::colorForType(const ResultType type)
{
    if (type >= ResultType::INTERNAL_MESSAGES_BEGIN && type <= ResultType::INTERNAL_MESSAGES_END)
        return QColor("transparent");

    Utils::Theme *creatorTheme = Utils::creatorTheme();
    switch (type) {
    case ResultType::Pass:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestPassTextColor);
    case ResultType::Fail:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestFailTextColor);
    case ResultType::ExpectedFail:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestXFailTextColor);
    case ResultType::UnexpectedPass:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestXPassTextColor);
    case ResultType::Skip:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestSkipTextColor);
    case ResultType::MessageDebug:
    case ResultType::MessageInfo:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestDebugTextColor);
    case ResultType::MessageWarn:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestWarnTextColor);
    case ResultType::MessageFatal:
    case ResultType::MessageSystem:
    case ResultType::MessageError:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestFatalTextColor);
    case ResultType::BlacklistedPass:
    case ResultType::BlacklistedFail:
    case ResultType::BlacklistedXPass:
    case ResultType::BlacklistedXFail:
    default:
        return creatorTheme->color(Utils::Theme::OutputPanes_StdOutTextColor);
    }
}

bool TestResult::isDirectParentOf(const TestResult *other, bool * /*needsIntermediate*/) const
{
    QTC_ASSERT(other, return false);
    return !m_id.isEmpty() && m_id == other->m_id && m_name == other->m_name;
}

bool TestResult::isIntermediateFor(const TestResult *other) const
{
    QTC_ASSERT(other, return false);
    return !m_id.isEmpty() && m_id == other->m_id && m_name == other->m_name;
}

TestResult *TestResult::createIntermediateResultFor(const TestResult *other)
{
    QTC_ASSERT(other, return nullptr);
    TestResult *intermediate = new TestResult(other->m_id, other->m_name);
    return intermediate;
}

} // namespace Autotest
