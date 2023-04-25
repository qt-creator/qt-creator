// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctestsettings.h"

#include "../autotestconstants.h"
#include "../autotesttr.h"

#include <utils/layoutbuilder.h>

namespace Autotest {
namespace Internal {

CTestSettings::CTestSettings()
{
    setSettingsGroups("Autotest", "CTest");
    setAutoApply(false);

    registerAspect(&outputOnFail);
    outputOnFail.setSettingsKey("OutputOnFail");
    outputOnFail.setLabelText(Tr::tr("Output on failure"));
    outputOnFail.setDefaultValue(true);

    registerAspect(&outputMode);
    outputMode.setSettingsKey("OutputMode");
    outputMode.setLabelText(Tr::tr("Output mode"));
    outputMode.setDisplayStyle(Utils::SelectionAspect::DisplayStyle::ComboBox);
    outputMode.addOption({Tr::tr("Default"), {}, 0});
    outputMode.addOption({Tr::tr("Verbose"), {}, 1});
    outputMode.addOption({Tr::tr("Very Verbose"), {}, 2});

    registerAspect(&repetitionMode);
    repetitionMode.setSettingsKey("RepetitionMode");
    repetitionMode.setLabelText(Tr::tr("Repetition mode"));
    repetitionMode.setDisplayStyle(Utils::SelectionAspect::DisplayStyle::ComboBox);
    repetitionMode.addOption({Tr::tr("Until Fail"), {}, 0});
    repetitionMode.addOption({Tr::tr("Until Pass"), {}, 1});
    repetitionMode.addOption({Tr::tr("After Timeout"), {}, 2});

    registerAspect(&repetitionCount);
    repetitionCount.setSettingsKey("RepetitionCount");
    repetitionCount.setDefaultValue(1);
    repetitionCount.setLabelText(Tr::tr("Count"));
    repetitionCount.setToolTip(Tr::tr("Number of re-runs for the test."));
    repetitionCount.setRange(1, 10000);

    registerAspect(&repeat);
    repeat.setSettingsKey("Repeat");

    registerAspect(&scheduleRandom);
    scheduleRandom.setSettingsKey("ScheduleRandom");
    scheduleRandom.setLabelText(Tr::tr("Schedule random"));

    registerAspect(&stopOnFailure);
    stopOnFailure.setSettingsKey("StopOnFail");
    stopOnFailure.setLabelText(Tr::tr("Stop on failure"));

    registerAspect(&parallel);
    parallel.setSettingsKey("Parallel");
    parallel.setToolTip(Tr::tr("Run tests in parallel mode using given number of jobs."));

    registerAspect(&jobs);
    jobs.setSettingsKey("Jobs");
    jobs.setLabelText(Tr::tr("Jobs"));
    jobs.setDefaultValue(1);
    jobs.setRange(1, 128);

    registerAspect(&testLoad);
    testLoad.setSettingsKey("TestLoad");
    testLoad.setLabelText(Tr::tr("Test load"));
    testLoad.setToolTip(Tr::tr("Try not to start tests when they may cause CPU load to pass a "
                           "threshold."));

    registerAspect(&threshold);
    threshold.setSettingsKey("Threshold");
    threshold.setLabelText(Tr::tr("Threshold"));
    threshold.setDefaultValue(1);
    threshold.setRange(1, 128);
    threshold.setEnabler(&testLoad);
}

QStringList CTestSettings::activeSettingsAsOptions() const
{
    QStringList options;
    if (outputOnFail.value())
        options << "--output-on-failure";
    switch (outputMode.value()) {
    case 1: options << "-V"; break;
    case 2: options << "-VV"; break;
    default: break;
    }
    if (repeat.value()) {
        QString repeatOption;
        switch (repetitionMode.value()) {
        case 0: repeatOption = "until-fail"; break;
        case 1: repeatOption = "until-pass"; break;
        case 2: repeatOption = "after-timeout"; break;
        default: break;
        }
        if (!repeatOption.isEmpty()) {
            repeatOption.append(':');
            repeatOption.append(QString::number(repetitionCount.value()));
            options << "--repeat" << repeatOption;
        }
    }
    if (scheduleRandom.value())
        options << "--schedule-random";
    if (stopOnFailure.value())
        options << "--stop-on-failure";
    if (parallel.value()) {
        options << "-j" << QString::number(jobs.value());
        if (testLoad.value())
            options << "--test-load" << QString::number(threshold.value());
    }
    return options;
}

CTestSettingsPage::CTestSettingsPage(CTestSettings *settings, Utils::Id settingsId)
{
    setId(settingsId);
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayName(Tr::tr("CTest"));

    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        CTestSettings &s = *settings;
        using namespace Layouting;

        Form form {
            Row {s.outputOnFail}, br,
            Row {s.scheduleRandom}, br,
            Row {s.stopOnFailure}, br,
            Row {s.outputMode}, br,
            Group {
                title(Tr::tr("Repeat tests")),
                s.repeat.groupChecker(),
                Row {s.repetitionMode, s.repetitionCount},
            }, br,
            Group {
                title(Tr::tr("Run in parallel")),
                s.parallel.groupChecker(),
                Column {
                    Row {s.jobs}, br,
                    Row {s.testLoad, s.threshold}
                }
            }
        };

        Column { Row { Column { form , st }, st } }.attachTo(widget);
    });
}

} // namespace Internal
} // namespace Autotest
