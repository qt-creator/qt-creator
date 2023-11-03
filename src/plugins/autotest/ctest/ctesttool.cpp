// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctesttool.h"

#include "ctesttreeitem.h"
#include "../autotestconstants.h"
#include "../autotesttr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h>

#include <projectexplorer/buildsystem.h>

#include <utils/layoutbuilder.h>

using namespace Layouting;
using namespace Utils;

namespace Autotest::Internal {

CTestTool &theCTestTool()
{
    static CTestTool tool;
    return tool;
}

CTestTool::CTestTool()
{
    setActive(false);
    setSettingsGroups("Autotest", "CTest");
    setAutoApply(false);
    setId("AutoTest.Framework.CTest");
    setDisplayName(Tr::tr("CTest"));

    setLayouter([this] {
        return Row { Form {
            outputOnFail, br,
            scheduleRandom, br,
            stopOnFailure, br,
            outputMode, br,
            Group {
                title(Tr::tr("Repeat tests")),
                repeat.groupChecker(),
                Row { repetitionMode, repetitionCount},
            }, br,
            Group {
                title(Tr::tr("Run in parallel")),
                parallel.groupChecker(),
                Column {
                    Row { jobs }, br,
                    Row { testLoad, threshold}
                }
            }
        }, st };
    });

    outputOnFail.setSettingsKey("OutputOnFail");
    outputOnFail.setLabelText(Tr::tr("Output on failure"));
    outputOnFail.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    outputOnFail.setDefaultValue(true);

    outputMode.setSettingsKey("OutputMode");
    outputMode.setLabelText(Tr::tr("Output mode"));
    outputMode.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    outputMode.addOption({Tr::tr("Default"), {}, 0});
    outputMode.addOption({Tr::tr("Verbose"), {}, 1});
    outputMode.addOption({Tr::tr("Very Verbose"), {}, 2});

    repetitionMode.setSettingsKey("RepetitionMode");
    repetitionMode.setLabelText(Tr::tr("Repetition mode"));
    repetitionMode.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    repetitionMode.addOption({Tr::tr("Until Fail"), {}, 0});
    repetitionMode.addOption({Tr::tr("Until Pass"), {}, 1});
    repetitionMode.addOption({Tr::tr("After Timeout"), {}, 2});

    repetitionCount.setSettingsKey("RepetitionCount");
    repetitionCount.setDefaultValue(1);
    repetitionCount.setLabelText(Tr::tr("Count"));
    repetitionCount.setToolTip(Tr::tr("Number of re-runs for the test."));
    repetitionCount.setRange(1, 10000);

    repeat.setSettingsKey("Repeat");

    scheduleRandom.setSettingsKey("ScheduleRandom");
    scheduleRandom.setLabelText(Tr::tr("Schedule random"));
    scheduleRandom.setLabelPlacement(BoolAspect::LabelPlacement::Compact);

    stopOnFailure.setSettingsKey("StopOnFail");
    stopOnFailure.setLabelText(Tr::tr("Stop on failure"));
    stopOnFailure.setLabelPlacement(BoolAspect::LabelPlacement::Compact);

    parallel.setSettingsKey("Parallel");
    parallel.setToolTip(Tr::tr("Run tests in parallel mode using given number of jobs."));

    jobs.setSettingsKey("Jobs");
    jobs.setLabelText(Tr::tr("Jobs"));
    jobs.setDefaultValue(1);
    jobs.setRange(1, 128);

    testLoad.setSettingsKey("TestLoad");
    testLoad.setLabelText(Tr::tr("Test load"));
    testLoad.setToolTip(Tr::tr("Try not to start tests when they may cause CPU load to pass a "
                               "threshold."));

    threshold.setSettingsKey("Threshold");
    threshold.setLabelText(Tr::tr("Threshold"));
    threshold.setDefaultValue(1);
    threshold.setRange(1, 128);

    readSettings();

    threshold.setEnabler(&testLoad);
}

QStringList CTestTool::activeSettingsAsOptions() const
{
    QStringList options;
    if (outputOnFail())
        options << "--output-on-failure";
    switch (outputMode()) {
    case 1: options << "-V"; break;
    case 2: options << "-VV"; break;
    default: break;
    }
    if (repeat()) {
        QString repeatOption;
        switch (repetitionMode()) {
        case 0: repeatOption = "until-fail"; break;
        case 1: repeatOption = "until-pass"; break;
        case 2: repeatOption = "after-timeout"; break;
        default: break;
        }
        if (!repeatOption.isEmpty()) {
            repeatOption.append(':');
            repeatOption.append(QString::number(repetitionCount()));
            options << "--repeat" << repeatOption;
        }
    }
    if (scheduleRandom())
        options << "--schedule-random";
    if (stopOnFailure())
        options << "--stop-on-failure";
    if (parallel()) {
        options << "-j" << QString::number(jobs());
        if (testLoad())
            options << "--test-load" << QString::number(threshold());
    }
    return options;
}

Id CTestTool::buildSystemId() const
{
    return Id(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
}

ITestTreeItem *CTestTool::createItemFromTestCaseInfo(const ProjectExplorer::TestCaseInfo &tci)
{
    CTestTreeItem *item = new CTestTreeItem(this, tci.name, tci.path, TestTreeItem::TestCase);
    item->setLine(tci.line);
    return item;
}

ITestTreeItem *CTestTool::createRootNode()
{
    return new CTestTreeItem(this, displayName(), {}, ITestTreeItem::Root);
}

// CTestToolSettingsPage

class CTestToolSettingsPage final : public Core::IOptionsPage
{
public:
    CTestToolSettingsPage()
    {
        setId(Id(Constants::SETTINGSPAGE_PREFIX).withSuffix("255.CTest"));
        setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
        setDisplayName(Tr::tr("CTest"));
        setSettingsProvider([] { return &theCTestTool(); });
    }
};

const CTestToolSettingsPage settingsPage;

} // Autotest::Internal
