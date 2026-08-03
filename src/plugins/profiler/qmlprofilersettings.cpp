// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilersettings.h"

#include "profilertr.h"
#include "qmlprofilerconstants.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Profiler::Internal {

QmlProfilerSettings &globalSettings()
{
    static QmlProfilerSettings theSettings;
    return theSettings;
}

QmlProfilerSettings::QmlProfilerSettings()
{
    setAutoApply(false);
    setSettingsGroup(Constants::ANALYZER);

    flushEnabled.setSettingsKey("Analyzer.QmlProfiler.FlushEnabled");
    flushEnabled.setLabelPlacement(BoolAspect::LabelPlacement::InExtraLabel);
    flushEnabled.setLabelText(Tr::tr("Flush data while profiling:"));
    flushEnabled.setToolTip(Tr::tr(
        "Periodically flush pending data to the profiler. This reduces the delay when loading the\n"
        "data and the memory usage in the application. It distorts the profile as the flushing\n"
        "itself takes time."));

    flushInterval.setSettingsKey("Analyzer.QmlProfiler.FlushInterval");
    flushInterval.setRange(1, 10000000);
    flushInterval.setDefaultValue(1000);
    flushInterval.setLabelText(Tr::tr("Flush interval (ms):"));

    lastTraceFile.setSettingsKey("Analyzer.QmlProfiler.LastTraceFile");

    aggregateTraces.setSettingsKey("Analyzer.QmlProfiler.AggregateTraces");
    aggregateTraces.setLabelPlacement(BoolAspect::LabelPlacement::InExtraLabel);
    aggregateTraces.setLabelText(Tr::tr("Process data only when process ends:"));
    aggregateTraces.setToolTip(Tr::tr(
        "Only process data when the process being profiled ends, not when the current recording\n"
        "session ends. This way multiple recording sessions can be aggregated in a single trace,\n"
        "for example if multiple QML engines start and stop sequentially during a single run of\n"
        "the program."));

    findingsCompileThresholdMs.setSettingsKey("Analyzer.QmlProfiler.Findings.CompileThresholdMs");
    findingsCompileThresholdMs.setRange(1, 100000);
    findingsCompileThresholdMs.setDefaultValue(50);
    findingsCompileThresholdMs.setLabelText(Tr::tr("Report compilation above (ms):"));

    findingsSyncLoadThresholdMs.setSettingsKey("Analyzer.QmlProfiler.Findings.SyncLoadThresholdMs");
    findingsSyncLoadThresholdMs.setRange(1, 100000);
    findingsSyncLoadThresholdMs.setDefaultValue(20);
    findingsSyncLoadThresholdMs.setLabelText(Tr::tr("Report items built in a handler above (ms):"));

    findingsPeriodicMinCount.setSettingsKey("Analyzer.QmlProfiler.Findings.PeriodicMinCount");
    findingsPeriodicMinCount.setRange(2, 1000000);
    findingsPeriodicMinCount.setDefaultValue(50);
    findingsPeriodicMinCount.setLabelText(Tr::tr("Report handlers running at least:"));
    findingsPeriodicMinCount.setToolTip(Tr::tr(
        "How often a signal handler has to run before the regularity of its interval is\n"
        "reported. Raise this on applications with many legitimate timers."));

    findingsPeriodicDeviationPercent.setSettingsKey(
        "Analyzer.QmlProfiler.Findings.PeriodicDeviationPercent");
    findingsPeriodicDeviationPercent.setRange(1, 100);
    findingsPeriodicDeviationPercent.setDefaultValue(25);
    findingsPeriodicDeviationPercent.setLabelText(Tr::tr("Interval counts as regular within (%):"));

    findingsPixmapMegapixels.setSettingsKey("Analyzer.QmlProfiler.Findings.PixmapMegapixels");
    findingsPixmapMegapixels.setRange(0.1, 1000.0);
    findingsPixmapMegapixels.setDefaultValue(2.0);
    findingsPixmapMegapixels.setLabelText(Tr::tr("Report images above (megapixels):"));

    findingsPerFrameBudgetUs.setSettingsKey("Analyzer.QmlProfiler.Findings.PerFrameBudgetUs");
    findingsPerFrameBudgetUs.setRange(1, 1000000);
    findingsPerFrameBudgetUs.setDefaultValue(500);
    findingsPerFrameBudgetUs.setLabelText(Tr::tr("Report per-frame cost above (us):"));

    setLayouter([this] {
        using namespace Layouting;
        // The findings thresholds sit in the same form as the rest: an aspect placed in a
        // Group needs its label spelled out again in a Row, and these already carry one.
        return Form {
            flushEnabled, br,
            flushInterval, br,
            aggregateTraces, br,
            findingsCompileThresholdMs, br,
            findingsSyncLoadThresholdMs, br,
            findingsPeriodicMinCount, br,
            findingsPeriodicDeviationPercent, br,
            findingsPixmapMegapixels, br,
            findingsPerFrameBudgetUs, br,
        };
    });

    readSettings();

    flushInterval.setEnabler(&flushEnabled);
}

// QmlProfilerSettingsPage

class QmlProfilerSettingsPage final : public Core::IOptionsPage
{
public:
    QmlProfilerSettingsPage()
    {
        setId(Constants::SETTINGS);
        setDisplayName(Tr::tr("QML Profiler"));
        setCategory("T.Analyzer");
        setSettingsProvider([] { return &globalSettings(); });
    }
};

const QmlProfilerSettingsPage settingsPage;

} // namespace Profiler::Internal
