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

static const char abortAfterKey[]                 = "AbortAfter";
static const char abortAfterCheckedKey[]          = "AbortChecked";
static const char benchmarkSamplesKey[]           = "BenchSamples";
static const char samplesCheckedKey[]             = "SamplesChecked";
static const char benchmarkResamplesKey[]         = "BenchResamples";
static const char resamplesCheckedKey[]           = "ResamplesChecked";
static const char benchmarConfidenceIntervalKey[] = "BenchConfInt";
static const char confIntCheckedKey[]             = "ConfIntChecked";
static const char benchmarWarmupTimeKey[]         = "BenchWarmup";
static const char warmupCheckedKey[]              = "WarmupChecked";
static const char noAnalysisKey[]                 = "NoAnalysis";
static const char showSuccessKey[]                = "ShowSuccess";
static const char breakOnFailureKey[]             = "BreakOnFailure";
static const char noThrowKey[]                    = "NoThrow";
static const char visibleWhitespaceKey[]          = "VisibleWS";
static const char warnOnEmptyKey[]                = "WarnEmpty";

QString CatchTestSettings::name() const
{
    return QString("Catch2");
}

void CatchTestSettings::toFrameworkSettings(QSettings *s) const
{
    s->setValue(abortAfterCheckedKey, abortAfterChecked);
    s->setValue(abortAfterKey, abortAfter);
    s->setValue(samplesCheckedKey, samplesChecked);
    s->setValue(benchmarkSamplesKey, benchmarkSamples);
    s->setValue(resamplesCheckedKey, resamplesChecked);
    s->setValue(benchmarkResamplesKey, benchmarkResamples);
    s->setValue(confIntCheckedKey, confidenceIntervalChecked);
    s->setValue(benchmarConfidenceIntervalKey, confidenceInterval);
    s->setValue(warmupCheckedKey, warmupChecked);
    s->setValue(benchmarWarmupTimeKey, benchmarkWarmupTime);
    s->setValue(noAnalysisKey, noAnalysis);
    s->setValue(showSuccessKey, showSuccess);
    s->setValue(breakOnFailureKey, breakOnFailure);
    s->setValue(noThrowKey, noThrow);
    s->setValue(visibleWhitespaceKey, visibleWhitespace);
    s->setValue(warnOnEmptyKey, warnOnEmpty);
}

void CatchTestSettings::fromFrameworkSettings(const QSettings *s)
{
    abortAfterChecked = s->value(abortAfterCheckedKey, false).toBool();
    abortAfter = s->value(abortAfterKey, 0).toInt();
    samplesChecked = s->value(samplesCheckedKey, false).toBool();
    benchmarkSamples = s->value(benchmarkSamplesKey, 100).toInt();
    resamplesChecked = s->value(resamplesCheckedKey, false).toBool();
    benchmarkResamples = s->value(benchmarkResamplesKey, 100000).toInt();
    confidenceIntervalChecked = s->value(confIntCheckedKey, false).toBool();
    confidenceInterval = s->value(benchmarConfidenceIntervalKey, 0.95).toDouble();
    warmupChecked = s->value(warmupCheckedKey, false).toBool();
    benchmarkWarmupTime = s->value(benchmarWarmupTimeKey, 0).toInt();
    noAnalysis = s->value(noAnalysisKey, false).toBool();
    showSuccess = s->value(showSuccessKey, false).toBool();
    breakOnFailure = s->value(breakOnFailureKey, true).toBool();
    noThrow = s->value(noThrowKey, false).toBool();
    visibleWhitespace = s->value(visibleWhitespaceKey, false).toBool();
    warnOnEmpty = s->value(warnOnEmptyKey, false).toBool();
}

} // namespace Internal
} // namespace Autotest
