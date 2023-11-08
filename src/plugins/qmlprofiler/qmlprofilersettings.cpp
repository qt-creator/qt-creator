// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilersettings.h"

#include "qmlprofilerconstants.h"
#include "qmlprofilertr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <debugger/analyzer/analyzericons.h>
#include <debugger/debuggertr.h>

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace QmlProfiler::Internal {

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

    setLayouter([this] {
        using namespace Layouting;
        return Form {
            flushEnabled, br,
            flushInterval, br,
            aggregateTraces, br,
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
        setDisplayCategory(::Debugger::Tr::tr("Analyzer"));
        setCategoryIconPath(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);
        setSettingsProvider([] { return &globalSettings(); });
    }
};

const QmlProfilerSettingsPage settingsPage;

} // QmlProfiler::Internal
