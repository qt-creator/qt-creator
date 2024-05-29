// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testresult.h"

#include <utils/theme/theme.h>

namespace Squish {
namespace Internal {

TestResult::TestResult(Result::Type type, const QString &text, const QString &timeStamp)
    : m_type(type)
    , m_text(text)
    , m_timeStamp(timeStamp)
    , m_line(-1)
{}

QString TestResult::typeToString(Result::Type type)
{
    switch (type) {
    case Result::Log:
        return "Log";
    case Result::Pass:
        return "Pass";
    case Result::Fail:
        return "Fail";
    case Result::ExpectedFail:
        return "Expected Fail";
    case Result::UnexpectedPass:
        return "Unexpected Pass";
    case Result::Warn:
        return "Warning";
    case Result::Error:
        return "Error";
    case Result::Fatal:
        return "Fatal";
    case Result::Detail:
        return "Detail";
    case Result::Start:
        return "Test Start";
    case Result::End:
        return "Test Finished";
    }
    return "UNKNOWN";
}

QColor TestResult::colorForType(Result::Type type)
{
    switch (type) {
    case Result::Start:
    case Result::Log:
    case Result::Detail:
    case Result::End:
        return creatorColor(Utils::Theme::OutputPanes_StdOutTextColor);
    case Result::Pass:
        return creatorColor(Utils::Theme::OutputPanes_TestPassTextColor);
    case Result::Fail:
    case Result::Error:
        return creatorColor(Utils::Theme::OutputPanes_TestFailTextColor);
    case Result::ExpectedFail:
        return creatorColor(Utils::Theme::OutputPanes_TestXFailTextColor);
    case Result::UnexpectedPass:
        return creatorColor(Utils::Theme::OutputPanes_TestXPassTextColor);
    case Result::Warn:
        return creatorColor(Utils::Theme::OutputPanes_TestWarnTextColor);
    case Result::Fatal:
        return creatorColor(Utils::Theme::OutputPanes_TestFatalTextColor);
    }
    return creatorColor(Utils::Theme::OutputPanes_StdOutTextColor);
}

} // namespace Internal
} // namespace Squish
