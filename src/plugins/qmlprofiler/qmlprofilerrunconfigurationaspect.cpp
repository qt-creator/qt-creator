// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerrunconfigurationaspect.h"

#include "qmlprofilerconstants.h"
#include "qmlprofilersettings.h"
#include "qmlprofilertr.h"

#include <debugger/analyzer/analyzerrunconfigwidget.h>

namespace QmlProfiler::Internal {

QmlProfilerRunConfigurationAspect::QmlProfilerRunConfigurationAspect(ProjectExplorer::Target *)
{
    setProjectSettings(new QmlProfilerSettings);
    setGlobalSettings(&Internal::globalSettings());
    setId(Constants::SETTINGS);
    setDisplayName(Tr::tr("QML Profiler Settings"));
    setUsingGlobalSettings(true);
    resetProjectToGlobalSettings();
    setConfigWidgetCreator([this] { return new Debugger::AnalyzerRunConfigWidget(this); });
}

} // QmlProfiler::Internal
