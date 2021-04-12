/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "catchtestsettings.h"

#include "../autotestconstants.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Autotest {
namespace Internal {

CatchTestSettings::CatchTestSettings()
{
    setSettingsGroups("Autotest", "Catch2");
    setAutoApply(false);

    registerAspect(&abortAfter);
    abortAfter.setSettingsKey("AbortAfter");
    abortAfter.setRange(1, 9999);
    abortAfter.setEnabler(&abortAfterChecked);

    registerAspect(&benchmarkSamples);
    benchmarkSamples.setSettingsKey("BenchSamples");
    benchmarkSamples.setRange(1, 999999);
    benchmarkSamples.setDefaultValue(100);
    benchmarkSamples.setEnabler(&samplesChecked);

    registerAspect(&benchmarkResamples);
    benchmarkResamples.setSettingsKey("BenchResamples");
    benchmarkResamples.setRange(1, 9999999);
    benchmarkResamples.setDefaultValue(100000);
    benchmarkResamples.setToolTip(tr("Number of resamples for bootstrapping."));
    benchmarkResamples.setEnabler(&resamplesChecked);

    registerAspect(&confidenceInterval);
    confidenceInterval.setSettingsKey("BenchConfInt");
    confidenceInterval.setRange(0., 1.);
    confidenceInterval.setSingleStep(0.05);
    confidenceInterval.setDefaultValue(0.95);
    confidenceInterval.setEnabler(&confidenceIntervalChecked);

    registerAspect(&benchmarkWarmupTime);
    benchmarkWarmupTime.setSettingsKey("BenchWarmup");
    benchmarkWarmupTime.setSuffix(tr(" ms"));
    benchmarkWarmupTime.setRange(0, 10000);
    benchmarkWarmupTime.setEnabler(&warmupChecked);

    registerAspect(&abortAfterChecked);
    abortAfterChecked.setSettingsKey("AbortChecked");
    abortAfterChecked.setLabelText(tr("Abort after"));
    abortAfterChecked.setToolTip(tr("Aborts after the specified number of failures."));

    registerAspect(&samplesChecked);
    samplesChecked.setSettingsKey("SamplesChecked");
    samplesChecked.setLabelText(tr("Benchmark samples"));
    samplesChecked.setToolTip(tr("Number of samples to collect while running benchmarks."));

    registerAspect(&resamplesChecked);
    resamplesChecked.setSettingsKey("ResamplesChecked");
    resamplesChecked.setLabelText(tr("Benchmark resamples"));
    resamplesChecked.setToolTip(tr("Number of resamples used for statistical bootstrapping."));

    registerAspect(&confidenceIntervalChecked);
    confidenceIntervalChecked.setSettingsKey("ConfIntChecked");
    confidenceIntervalChecked.setToolTip(tr("Confidence interval used for statistical bootstrapping."));
    confidenceIntervalChecked.setLabelText(tr("Benchmark confidence interval"));

    registerAspect(&warmupChecked);
    warmupChecked.setSettingsKey("WarmupChecked");
    warmupChecked.setLabelText(tr("Benchmark warmup time"));
    warmupChecked.setToolTip(tr("Warmup time for each test."));

    registerAspect(&noAnalysis);
    noAnalysis.setSettingsKey("NoAnalysis");
    noAnalysis.setLabelText(tr("Disable analysis"));
    noAnalysis.setToolTip(tr("Disables statistical analysis and bootstrapping."));

    registerAspect(&showSuccess);
    showSuccess.setSettingsKey("ShowSuccess");
    showSuccess.setLabelText(tr("Show success"));
    showSuccess.setToolTip(tr("Show success for tests."));

    registerAspect(&breakOnFailure);
    breakOnFailure.setSettingsKey("BreakOnFailure");
    breakOnFailure.setDefaultValue(true);
    breakOnFailure.setLabelText(tr("Break on failure while debugging"));
    breakOnFailure.setToolTip(tr("Turns failures into debugger breakpoints."));

    registerAspect(&noThrow);
    noThrow.setSettingsKey("NoThrow");
    noThrow.setLabelText(tr("Skip throwing assertions"));
    noThrow.setToolTip(tr("Skips all assertions that test for thrown exceptions."));

    registerAspect(&visibleWhitespace);
    visibleWhitespace.setSettingsKey("VisibleWS");
    visibleWhitespace.setLabelText(tr("Visualize whitespace"));
    visibleWhitespace.setToolTip(tr("Makes whitespace visible."));

    registerAspect(&warnOnEmpty);
    warnOnEmpty.setSettingsKey("WarnEmpty");
    warnOnEmpty.setLabelText(tr("Warn on empty tests"));
    warnOnEmpty.setToolTip(tr("Warns if a test section does not check any assertion."));

    forEachAspect([](BaseAspect *aspect) {
        // FIXME: Make the positioning part of the LayoutBuilder later
        if (auto boolAspect = dynamic_cast<BoolAspect *>(aspect))
            boolAspect->setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    });
}

CatchTestSettingsPage::CatchTestSettingsPage(CatchTestSettings *settings, Utils::Id settingsId)
{
    setId(settingsId);
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayName(QCoreApplication::translate("CatchTestFramework", "Catch Test"));
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        CatchTestSettings &s = *settings;
        using namespace Layouting;
        const Break nl;

        Grid col {
            s.showSuccess, nl,
            s.breakOnFailure, nl,
            s.noThrow, nl,
            s.visibleWhitespace, nl,
            s.abortAfterChecked, s.abortAfter, nl,
            s.samplesChecked, s.benchmarkSamples, nl,
            s.resamplesChecked, s.benchmarkResamples, nl,
            s.confidenceIntervalChecked, s.confidenceInterval, nl,
            s.warmupChecked, s.benchmarkWarmupTime, nl,
            s.noAnalysis
        };

        Column { Row { col, Stretch() }, Stretch() }.attachTo(widget);
    });
}

} // namespace Internal
} // namespace Autotest
