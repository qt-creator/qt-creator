// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilerconstants.h"
#include "perfprofilerplugin.h"
#include "perfprofilertr.h"
#include "perfrunconfigurationaspect.h"
#include "perfsettings.h"

#include <debugger/analyzer/analyzerrunconfigwidget.h>

namespace PerfProfiler {

PerfRunConfigurationAspect::PerfRunConfigurationAspect(ProjectExplorer::Target *target)
{
    setProjectSettings(new PerfSettings(target));
    setGlobalSettings(&PerfProfiler::globalSettings());
    setId(Constants::PerfSettingsId);
    setDisplayName(Tr::tr("Performance Analyzer Settings"));
    setUsingGlobalSettings(true);
    resetProjectToGlobalSettings();
    setConfigWidgetCreator([this] { return new Debugger::AnalyzerRunConfigWidget(this); });
}

} // namespace PerfProfiler
