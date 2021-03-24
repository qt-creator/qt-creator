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

namespace Autotest {
namespace Internal {

CatchTestSettings::CatchTestSettings()
{
    setSettingsGroups("Autotest", "Catch2");
    setAutoApply(false);

    registerAspect(&abortAfter);
    abortAfter.setSettingsKey("AbortAfter");

    registerAspect(&benchmarkSamples);
    benchmarkSamples.setSettingsKey("BenchSamples");
    benchmarkSamples.setDefaultValue(100);

    registerAspect(&benchmarkResamples);
    benchmarkResamples.setSettingsKey("BenchResamples");
    benchmarkResamples.setDefaultValue(100000);

    registerAspect(&confidenceInterval);
    confidenceInterval.setSettingsKey("BenchConfInt");
    confidenceInterval.setDefaultValue(0.95);

    registerAspect(&benchmarkWarmupTime);
    benchmarkWarmupTime.setSettingsKey("BenchWarmup");

    registerAspect(&abortAfterChecked);
    abortAfterChecked.setSettingsKey("AbortChecked");

    registerAspect(&samplesChecked);
    samplesChecked.setSettingsKey("SamplesChecked");

    registerAspect(&resamplesChecked);
    resamplesChecked.setSettingsKey("ResamplesChecked");

    registerAspect(&confidenceIntervalChecked);
    confidenceIntervalChecked.setSettingsKey("ConfIntChecked");

    registerAspect(&warmupChecked);
    warmupChecked.setSettingsKey("WarmupChecked");

    registerAspect(&noAnalysis);
    noAnalysis.setSettingsKey("NoAnalysis");

    registerAspect(&showSuccess);
    showSuccess.setSettingsKey("ShowSuccess");

    registerAspect(&breakOnFailure);
    breakOnFailure.setSettingsKey("BreakOnFailure");
    breakOnFailure.setDefaultValue(true);

    registerAspect(&noThrow);
    noThrow.setSettingsKey("NoThrow");

    registerAspect(&visibleWhitespace);
    visibleWhitespace.setSettingsKey("VisibleWS");

    registerAspect(&warnOnEmpty);
    warnOnEmpty.setSettingsKey("WarnEmpty");
}

} // namespace Internal
} // namespace Autotest
