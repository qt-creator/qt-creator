// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Autotest::Internal {

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

class BoostTestSettings : public Core::PagedSettings
{
public:
    explicit BoostTestSettings(Utils::Id settingsId);

    static QString logLevelToOption(const LogLevel logLevel);
    static QString reportLevelToOption(const ReportLevel reportLevel);

    Utils::SelectionAspect logLevel{this};
    Utils::SelectionAspect reportLevel{this};
    Utils::IntegerAspect seed{this};
    Utils::BoolAspect randomize{this};
    Utils::BoolAspect systemErrors{this};
    Utils::BoolAspect fpExceptions{this};
    Utils::BoolAspect memLeaks{this};
};

} // Autotest::Internal

Q_DECLARE_METATYPE(Autotest::Internal::LogLevel)
Q_DECLARE_METATYPE(Autotest::Internal::ReportLevel)
