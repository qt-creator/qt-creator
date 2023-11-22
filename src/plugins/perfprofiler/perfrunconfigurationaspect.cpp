// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfrunconfigurationaspect.h"

#include "perfprofiler_global.h"
#include "perfprofilerconstants.h"
#include "perfprofilertr.h"
#include "perfsettings.h"

#include <projectexplorer/runconfiguration.h>

#include <debugger/analyzer/analyzerrunconfigwidget.h>

using namespace ProjectExplorer;

namespace PerfProfiler {

class PerfRunConfigurationAspect
    : public GlobalOrProjectAspect
{
    Q_OBJECT

public:
    PerfRunConfigurationAspect(Target *target)
    {
        setProjectSettings(new PerfSettings(target));
        setGlobalSettings(&PerfProfiler::globalSettings());
        setId(Constants::PerfSettingsId);
        setDisplayName(Tr::tr("Performance Analyzer Settings"));
        setUsingGlobalSettings(true);
        resetProjectToGlobalSettings();
        setConfigWidgetCreator([this] { return new Debugger::AnalyzerRunConfigWidget(this); });
    }
};

void setupPerfRunConfigurationAspect()
{
    RunConfiguration::registerAspect<PerfRunConfigurationAspect>();
}

} // namespace PerfProfiler

#include "perfrunconfigurationaspect.moc"
