// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldebugcommandlinearguments.h"

#include "projectexplorerconstants.h"

namespace ProjectExplorer {

Utils::Id runnerIdForRunMode(Utils::Id runMode)
{
    if (runMode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE)
        return ProjectExplorer::Constants::QML_PROFILER_RUNNER;
    if (runMode == ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE)
        return ProjectExplorer::Constants::QML_PREVIEW_RUNNER;
    return {};
}

QmlDebugServicesPreset servicesForRunMode(Utils::Id runMode)
{
    if (runMode == ProjectExplorer::Constants::QML_PROFILER_RUN_MODE)
        return QmlProfilerServices;
    if (runMode == ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE)
        return QmlPreviewServices;
    if (runMode == ProjectExplorer::Constants::DEBUG_RUN_MODE)
        return QmlDebuggerServices;
    return {};
}

} // namespace ProjectExplorer
