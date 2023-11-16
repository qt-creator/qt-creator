// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testresult.h"

#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

using namespace Utils;

namespace Autotest {

TestResult::TestResult(const QString &id, const QString &name, const ResultHooks &hooks)
    : m_id(id)
    , m_name(name)
    , m_hooks(hooks)
{
}

bool TestResult::isValid() const
{
    return m_id.has_value();
}

const QString TestResult::outputString(bool selected) const
{
    if (m_hooks.outputString)
        return m_hooks.outputString(*this, selected);

    if (m_result == ResultType::Application)
        return id();
    return selected ? m_description : m_description.split('\n').first();
}

const ITestTreeItem *TestResult::findTestTreeItem() const
{
    if (m_hooks.findTestItem)
        return m_hooks.findTestItem(*this);
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
    if (resultString == "error" || resultString == "qcritical")
        return ResultType::MessageError;
    if (resultString == "system" || resultString == "qsystem")
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
        return {};
    default:
        if (type >= ResultType::INTERNAL_MESSAGES_BEGIN && type <= ResultType::INTERNAL_MESSAGES_END)
            return {};
        return QString("UNKNOWN");
    }
}

QColor TestResult::colorForType(const ResultType type)
{
    if (type >= ResultType::INTERNAL_MESSAGES_BEGIN && type <= ResultType::INTERNAL_MESSAGES_END)
        return QColor("transparent");

    const Theme *theme = creatorTheme();
    switch (type) {
    case ResultType::Pass:
        return theme->color(Theme::OutputPanes_TestPassTextColor);
    case ResultType::Fail:
        return theme->color(Theme::OutputPanes_TestFailTextColor);
    case ResultType::ExpectedFail:
        return theme->color(Theme::OutputPanes_TestXFailTextColor);
    case ResultType::UnexpectedPass:
        return theme->color(Theme::OutputPanes_TestXPassTextColor);
    case ResultType::Skip:
        return theme->color(Theme::OutputPanes_TestSkipTextColor);
    case ResultType::MessageDebug:
    case ResultType::MessageInfo:
        return theme->color(Theme::OutputPanes_TestDebugTextColor);
    case ResultType::MessageWarn:
        return theme->color(Theme::OutputPanes_TestWarnTextColor);
    case ResultType::MessageFatal:
    case ResultType::MessageSystem:
    case ResultType::MessageError:
        return theme->color(Theme::OutputPanes_TestFatalTextColor);
    case ResultType::BlacklistedPass:
    case ResultType::BlacklistedFail:
    case ResultType::BlacklistedXPass:
    case ResultType::BlacklistedXFail:
    default:
        return theme->color(Theme::OutputPanes_StdOutTextColor);
    }
}

bool TestResult::isDirectParentOf(const TestResult &other, bool *needsIntermediate) const
{
    QTC_ASSERT(other.isValid(), return false);
    const bool ret = m_id && m_id == other.m_id && m_name == other.m_name;
    if (!ret)
        return false;
    if (m_hooks.directParent)
        return m_hooks.directParent(*this, other, needsIntermediate);
    return true;
}

bool TestResult::isIntermediateFor(const TestResult &other) const
{
    QTC_ASSERT(other.isValid(), return false);
    if (m_hooks.intermediate)
        return m_hooks.intermediate(*this, other);
    return m_id && m_id == other.m_id && m_name == other.m_name;
}

TestResult TestResult::intermediateResult() const
{
    if (m_hooks.createResult)
        return m_hooks.createResult(*this);
    return {id(), m_name};
}

} // namespace Autotest
