/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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
    Utils::Theme *creatorTheme = Utils::creatorTheme();

    switch (type) {
    case Result::Start:
    case Result::Log:
    case Result::Detail:
    case Result::End:
        return creatorTheme->color(Utils::Theme::OutputPanes_StdOutTextColor);
    case Result::Pass:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestPassTextColor);
    case Result::Fail:
    case Result::Error:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestFailTextColor);
    case Result::ExpectedFail:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestXFailTextColor);
    case Result::UnexpectedPass:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestXPassTextColor);
    case Result::Warn:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestWarnTextColor);
    case Result::Fatal:
        return creatorTheme->color(Utils::Theme::OutputPanes_TestFatalTextColor);
    }
    return creatorTheme->color(Utils::Theme::OutputPanes_StdOutTextColor);
}

} // namespace Internal
} // namespace Squish
