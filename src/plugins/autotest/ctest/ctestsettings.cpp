/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "ctestsettings.h"

#include "../autotestconstants.h"

#include <utils/layoutbuilder.h>

namespace Autotest {
namespace Internal {

CTestSettings::CTestSettings()
{
    setSettingsGroups("Autotest", "CTest");
    setAutoApply(false);

    registerAspect(&outputOnFail);
    outputOnFail.setSettingsKey("OutputOnFail");
    outputOnFail.setLabelText(tr("Output on failure"));
    outputOnFail.setDefaultValue(true);

    registerAspect(&outputMode);
    outputMode.setSettingsKey("OutputMode");
    outputMode.setLabelText(tr("Output mode"));
    outputMode.setDisplayStyle(Utils::SelectionAspect::DisplayStyle::ComboBox);
    outputMode.addOption({tr("Default"), {}, 0});
    outputMode.addOption({tr("Verbose"), {}, 1});
    outputMode.addOption({tr("Very Verbose"), {}, 2});

    registerAspect(&repetitionMode);
    repetitionMode.setSettingsKey("RepetitionMode");
    repetitionMode.setLabelText(tr("Repetition mode"));
    repetitionMode.setDisplayStyle(Utils::SelectionAspect::DisplayStyle::ComboBox);
    repetitionMode.addOption({tr("Until Fail"), {}, 0});
    repetitionMode.addOption({tr("Until Pass"), {}, 1});
    repetitionMode.addOption({tr("After Timeout"), {}, 2});

    registerAspect(&repetitionCount);
    repetitionCount.setSettingsKey("RepetitionCount");
    repetitionCount.setDefaultValue(1);
    repetitionCount.setLabelText(tr("Count"));
    repetitionCount.setToolTip(tr("Number of re-runs for the test."));
    repetitionCount.setRange(1, 10000);

    registerAspect(&repeat);
    repeat.setSettingsKey("Repeat");

    registerAspect(&scheduleRandom);
    scheduleRandom.setSettingsKey("ScheduleRandom");
    scheduleRandom.setLabelText(tr("Schedule random"));

    registerAspect(&stopOnFailure);
    stopOnFailure.setSettingsKey("StopOnFail");
    stopOnFailure.setLabelText(tr("Stop on failure"));

    registerAspect(&parallel);
    parallel.setSettingsKey("Parallel");
    parallel.setToolTip(tr("Run tests in parallel mode using given number of jobs."));

    registerAspect(&jobs);
    jobs.setSettingsKey("Jobs");
    jobs.setLabelText(tr("Jobs"));
    jobs.setDefaultValue(1);
    jobs.setRange(1, 128);

    registerAspect(&testLoad);
    testLoad.setSettingsKey("TestLoad");
    testLoad.setLabelText(tr("Test load"));
    testLoad.setToolTip(tr("Try not to start tests when they may cause CPU load to pass a "
                           "threshold."));

    registerAspect(&threshold);
    threshold.setSettingsKey("Threshold");
    threshold.setLabelText(tr("Threshold"));
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
    setDisplayName(tr("CTest"));

    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        CTestSettings &s = *settings;
        using namespace Utils::Layouting;
        const Break nl;

        Form form {
            Row {s.outputOnFail}, nl,
            Row {s.scheduleRandom}, nl,
            Row {s.stopOnFailure}, nl,
            Row {s.outputMode}, nl,
            Group {
                Title(tr("Repeat tests"), &s.repeat), nl,
                Row {s.repetitionMode, s.repetitionCount},
            }, nl,
            Group {
                Title(tr("Run in parallel"), &s.parallel), nl,
                Row {s.jobs}, nl,
                Row {s.testLoad, s.threshold}
            }
        };

        Column {Row {Column { form , Stretch() }, Stretch() } }.attachTo(widget);
    });
}

} // namespace Internal
} // namespace Autotest
