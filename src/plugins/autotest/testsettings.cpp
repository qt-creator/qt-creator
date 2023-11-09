// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testsettings.h"

#include "autotestconstants.h"
#include "autotesttr.h"
#include "testframeworkmanager.h"

#include <utils/qtcsettings.h>

using namespace Utils;

namespace Autotest::Internal  {

static const char groupSuffix[]                 = ".group";

constexpr int defaultTimeout = 60000;

TestSettings &testSettings()
{
    static TestSettings theSettings;
    return theSettings;
}

TestSettings::TestSettings()
{
    setSettingsGroup(Constants::SETTINGSGROUP);

    scanThreadLimit.setSettingsKey("ScanThreadLimit");
    scanThreadLimit.setDefaultValue(0);
    scanThreadLimit.setRange(0, QThread::idealThreadCount());
    scanThreadLimit.setSpecialValueText("Automatic");
    scanThreadLimit.setToolTip(Tr::tr("Number of worker threads used when scanning for tests."));

    timeout.setSettingsKey("Timeout");
    timeout.setDefaultValue(defaultTimeout);
    timeout.setRange(5000, 36'000'000); // 36 Mio ms = 36'000 s = 10 h
    timeout.setSuffix(Tr::tr(" s")); // we show seconds, but store milliseconds
    timeout.setDisplayScaleFactor(1000);
    timeout.setToolTip(Tr::tr("Timeout used when executing test cases. This will apply "
                              "for each test case on its own, not the whole project."));

    omitInternalMsg.setSettingsKey("OmitInternal");
    omitInternalMsg.setDefaultValue(true);
    omitInternalMsg.setLabelText(Tr::tr("Omit internal messages"));
    omitInternalMsg.setToolTip(Tr::tr("Hides internal messages by default. "
        "You can still enable them by using the test results filter."));

    omitRunConfigWarn.setSettingsKey("OmitRCWarnings");
    omitRunConfigWarn.setLabelText(Tr::tr("Omit run configuration warnings"));
    omitRunConfigWarn.setToolTip(Tr::tr("Hides warnings related to a deduced run configuration."));

    limitResultOutput.setSettingsKey("LimitResultOutput");
    limitResultOutput.setDefaultValue(true);
    limitResultOutput.setLabelText(Tr::tr("Limit result output"));
    limitResultOutput.setToolTip(Tr::tr("Limits result output to 100000 characters."));

    limitResultDescription.setSettingsKey("LimitResultDescription");
    limitResultDescription.setLabelText(Tr::tr("Limit result description:"));
    limitResultDescription.setToolTip(
        Tr::tr("Limit number of lines shown in test result tooltip and description."));

    resultDescriptionMaxSize.setSettingsKey("ResultDescriptionMaxSize");
    resultDescriptionMaxSize.setDefaultValue(10);
    resultDescriptionMaxSize.setRange(1, 100000);

    autoScroll.setSettingsKey("AutoScrollResults");
    autoScroll.setDefaultValue(true);
    autoScroll.setLabelText(Tr::tr("Automatically scroll results"));
    autoScroll.setToolTip(Tr::tr("Automatically scrolls down when new items are added "
                                 "and scrollbar is at bottom."));

    processArgs.setSettingsKey("ProcessArgs");
    processArgs.setLabelText(Tr::tr("Process arguments"));
    processArgs.setToolTip(
        Tr::tr("Allow passing arguments specified on the respective run configuration.\n"
               "Warning: this is an experimental feature and might lead to failing to "
               "execute the test executable."));

    displayApplication.setSettingsKey("DisplayApp");
    displayApplication.setLabelText(Tr::tr("Group results by application"));

    popupOnStart.setSettingsKey("PopupOnStart");
    popupOnStart.setLabelText(Tr::tr("Open results when tests start"));
    popupOnStart.setToolTip(
        Tr::tr("Displays test results automatically when tests are started."));

    popupOnFinish.setSettingsKey("PopupOnFinish");
    popupOnFinish.setDefaultValue(true);
    popupOnFinish.setLabelText(Tr::tr("Open results when tests finish"));
    popupOnFinish.setToolTip(
        Tr::tr("Displays test results automatically when tests are finished."));

    popupOnFail.setSettingsKey("PopupOnFail");
    popupOnFail.setLabelText(Tr::tr("Only for unsuccessful test runs"));
    popupOnFail.setToolTip(Tr::tr("Displays test results only if the test run contains "
                                  "failed, fatal or unexpectedly passed tests."));

    runAfterBuild.setSettingsKey("RunAfterBuild");
    runAfterBuild.setDisplayStyle(Utils::SelectionAspect::DisplayStyle::ComboBox);
    runAfterBuild.setToolTip(Tr::tr("Runs chosen tests automatically if a build succeeded."));
    runAfterBuild.addOption(Tr::tr("None"));
    runAfterBuild.addOption(Tr::tr("All"));
    runAfterBuild.addOption(Tr::tr("Selected"));

    fromSettings();

    resultDescriptionMaxSize.setEnabler(&limitResultDescription);
    popupOnFail.setEnabler(&popupOnFinish);
}

void TestSettings::toSettings() const
{
    AspectContainer::writeSettings();

    QtcSettings *s = BaseAspect::qtcSettings();
    s->beginGroup(Constants::SETTINGSGROUP);

    // store frameworks and their current active and grouping state
    for (auto it = frameworks.cbegin(); it != frameworks.cend(); ++it) {
        const Utils::Id &id = it.key();
        s->setValue(id.toKey(), it.value());
        s->setValue(id.toKey() + groupSuffix, frameworksGrouping.value(id));
    }
    // ..and the testtools as well
    for (auto it = tools.cbegin(); it != tools.cend(); ++it)
        s->setValue(it.key().toKey(), it.value());
    s->endGroup();
}

void TestSettings::fromSettings()
{
    AspectContainer::readSettings();

    QtcSettings *s = BaseAspect::qtcSettings();
    s->beginGroup(Constants::SETTINGSGROUP);

    // try to get settings for registered frameworks
    const TestFrameworks &registered = TestFrameworkManager::registeredFrameworks();
    frameworks.clear();
    frameworksGrouping.clear();
    for (const ITestFramework *framework : registered) {
        // get their active state
        const Id id = framework->id();
        const Key key = id.toKey();
        frameworks.insert(id, s->value(key, framework->active()).toBool());
        // and whether grouping is enabled
        frameworksGrouping.insert(id, s->value(key + groupSuffix, framework->grouping()).toBool());
    }
    // ..and for test tools as well
    const TestTools &registeredTools = TestFrameworkManager::registeredTestTools();
    tools.clear();
    for (const ITestTool *testTool : registeredTools) {
        const Utils::Id id = testTool->id();
        tools.insert(id, s->value(id.toKey(), testTool->active()).toBool());
    }
    s->endGroup();
}

RunAfterBuildMode TestSettings::runAfterBuildMode() const
{
    return static_cast<RunAfterBuildMode>(runAfterBuild());
}

} // namespace Autotest::Internal
