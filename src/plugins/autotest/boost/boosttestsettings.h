/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#pragma once

#include "../iframeworksettings.h"

namespace Autotest {
namespace Internal {

enum class LogLevel
{
    All,
    Success,
    TestSuite,
    UnitScope,
    Message,
    Warning,
    Error,
    CppException,
    SystemError,
    FatalError,
    Nothing
};

enum class ReportLevel
{
    Confirm,
    Short,
    Detailed,
    No
};

class BoostTestSettings : public IFrameworkSettings
{
public:
    BoostTestSettings() = default;
    QString name() const override;
    static QString logLevelToOption(const LogLevel logLevel);
    static QString reportLevelToOption(const ReportLevel reportLevel);

    LogLevel logLevel = LogLevel::Warning;
    ReportLevel reportLevel = ReportLevel::Confirm;
    int seed = 0;
    bool randomize = false;
    bool systemErrors = false;
    bool fpExceptions = false;
    bool memLeaks = true;

protected:
    void fromFrameworkSettings(const QSettings *s) override;
    void toFrameworkSettings(QSettings *s) const override;
};

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::LogLevel)
Q_DECLARE_METATYPE(Autotest::Internal::ReportLevel)
