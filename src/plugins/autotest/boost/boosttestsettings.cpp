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

#include "boosttestsettings.h"

#include "boosttestconstants.h"

#include "../autotestconstants.h"

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Autotest {
namespace Internal {

BoostTestSettings::BoostTestSettings()
{
    setSettingsGroups("Autotest", "BoostTest");
    setAutoApply(false);

    registerAspect(&logLevel);
    logLevel.setSettingsKey("LogLevel");
    logLevel.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    logLevel.addOption("All");
    logLevel.addOption("Success");
    logLevel.addOption("Test Suite");
    logLevel.addOption("Unit Scope");
    logLevel.addOption("Message");
    logLevel.addOption("Warning");
    logLevel.addOption("Error");
    logLevel.addOption("C++ Exception");
    logLevel.addOption("System Error");
    logLevel.addOption("Fatal Error");
    logLevel.addOption("Nothing");
    logLevel.setDefaultValue(int(LogLevel::Warning));
    logLevel.setLabelText(tr("Log format:"));

    registerAspect(&reportLevel);
    reportLevel.setSettingsKey("ReportLevel");
    reportLevel.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    reportLevel.addOption("Confirm");
    reportLevel.addOption("Short");
    reportLevel.addOption("Detailed");
    reportLevel.addOption("No");
    reportLevel.setDefaultValue(int(ReportLevel::Confirm));
    reportLevel.setLabelText(tr("Report level:"));

    registerAspect(&seed);
    seed.setSettingsKey("Seed");
    seed.setEnabled(false);
    seed.setLabelText(tr("Seed:"));
    seed.setToolTip(tr("A seed of 0 means no randomization. A value of 1 uses the current "
        "time, any other value is used as random seed generator."));
    seed.setEnabler(&randomize);

    registerAspect(&randomize);
    randomize.setSettingsKey("Randomize");
    randomize.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    randomize.setLabelText(tr("Randomize"));
    randomize.setToolTip(tr("Randomize execution order."));

    registerAspect(&systemErrors);
    systemErrors.setSettingsKey("SystemErrors");
    systemErrors.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    systemErrors.setLabelText(tr("Catch system errors"));
    systemErrors.setToolTip(tr("Catch or ignore system errors."));

    registerAspect(&fpExceptions);
    fpExceptions.setSettingsKey("FPExceptions");
    fpExceptions.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    fpExceptions.setLabelText(tr("Floating point exceptions"));
    fpExceptions.setToolTip(tr("Enable floating point exception traps."));

    registerAspect(&memLeaks);
    memLeaks.setSettingsKey("MemoryLeaks");
    memLeaks.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    memLeaks.setDefaultValue(true);
    memLeaks.setLabelText(tr("Detect memory leaks"));
    memLeaks.setToolTip(tr("Enable memory leak detection."));
}

BoostTestSettingsPage::BoostTestSettingsPage(BoostTestSettings *settings, Utils::Id settingsId)
{
    setId(settingsId);
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayName(QCoreApplication::translate("BoostTestFramework",
                                               BoostTest::Constants::FRAMEWORK_SETTINGS_CATEGORY));
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        BoostTestSettings &s = *settings;
        using namespace Layouting;
        const Break nl;

        Grid grid {
            s.logLevel, nl,
            s.reportLevel, nl,
            s.randomize, Row { s.seed }, nl,
            s.systemErrors, nl,
            s.fpExceptions, nl,
            s.memLeaks,
        };

        Column { Row { Column { grid, Stretch() }, Stretch() } }.attachTo(widget);
    });
}

QString BoostTestSettings::logLevelToOption(const LogLevel logLevel)
{
    switch (logLevel) {
    case LogLevel::All: return QString("all");
    case LogLevel::Success: return QString("success");
    case LogLevel::TestSuite: return QString("test_suite");
    case LogLevel::UnitScope: return QString("unit_scope");
    case LogLevel::Message: return QString("message");
    case LogLevel::Error: return QString("error");
    case LogLevel::CppException: return QString("cpp_exception");
    case LogLevel::SystemError: return QString("system_error");
    case LogLevel::FatalError: return QString("fatal_error");
    case LogLevel::Nothing: return QString("nothing");
    case LogLevel::Warning: return QString("warning");
    }
    return QString();
}

QString BoostTestSettings::reportLevelToOption(const ReportLevel reportLevel)
{
    switch (reportLevel) {
    case ReportLevel::Confirm: return QString("confirm");
    case ReportLevel::Short: return QString("short");
    case ReportLevel::Detailed: return QString("detailed");
    case ReportLevel::No: return QString("no");
    }
    return QString();
}

} // namespace Internal
} // namespace Autotest
