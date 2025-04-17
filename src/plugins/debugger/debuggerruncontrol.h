// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"
#include "debuggerengine.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

namespace Debugger {

DEBUGGER_EXPORT Tasking::Group debuggerRecipe(
    ProjectExplorer::RunControl *runControl,
    const DebuggerRunParameters &initialParameters,
    const std::function<void(DebuggerRunParameters &)> &parametersModifier = {});

DEBUGGER_EXPORT ProjectExplorer::RunWorker *createDebuggerWorker(
    ProjectExplorer::RunControl *runControl,
    const DebuggerRunParameters &initialParameters,
    const std::function<void(DebuggerRunParameters &)> &parametersModifier = {});

void setupDebuggerRunWorker();

class SimpleDebugRunnerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    explicit SimpleDebugRunnerFactory(const QList<Utils::Id> &runConfigs, const QList<Utils::Id> &extraRunModes = {})
    {
        cloneProduct(Constants::DEBUGGER_RUN_FACTORY);
        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        for (const Utils::Id &id : extraRunModes)
            addSupportedRunMode(id);
        setSupportedRunConfigs(runConfigs);
    }
};

} // Debugger
