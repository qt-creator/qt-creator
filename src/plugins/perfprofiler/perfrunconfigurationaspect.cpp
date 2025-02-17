// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilerconstants.h"
#include "perfprofilertr.h"
#include "perfrunconfigurationaspect.h"
#include "perfsettings.h"

#include <projectexplorer/runconfiguration.h>

using namespace ProjectExplorer;

namespace PerfProfiler::Internal {

PerfRunConfigurationAspect::PerfRunConfigurationAspect(Target *target)
{
    setProjectSettings(new PerfSettings(target));
    setGlobalSettings(&PerfProfiler::globalSettings());
    setId(Constants::PerfSettingsId);
    setDisplayName(Tr::tr("Performance Analyzer Settings"));
    setUsingGlobalSettings(true);
    resetProjectToGlobalSettings();
    setConfigWidgetCreator([this] { return createRunConfigAspectWidget(this); });
}

} // PerfProfiler::Internal
