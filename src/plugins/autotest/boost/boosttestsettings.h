// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

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

class BoostTestSettings : public Utils::AspectContainer
{
public:
    BoostTestSettings();

    static QString logLevelToOption(const LogLevel logLevel);
    static QString reportLevelToOption(const ReportLevel reportLevel);

    Utils::SelectionAspect logLevel;
    Utils::SelectionAspect reportLevel;
    Utils::IntegerAspect seed;
    Utils::BoolAspect randomize;
    Utils::BoolAspect systemErrors;
    Utils::BoolAspect fpExceptions;
    Utils::BoolAspect memLeaks;
};


class BoostTestSettingsPage final : public Core::IOptionsPage
{
public:
    BoostTestSettingsPage(BoostTestSettings *settings, Utils::Id settingsId);
};

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::LogLevel)
Q_DECLARE_METATYPE(Autotest::Internal::ReportLevel)
