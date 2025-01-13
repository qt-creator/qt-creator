// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"
#include "debuggerengine.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>

#include <utils/environmentfwd.h>

namespace Debugger {

namespace Internal { class DebuggerRunToolPrivate; }

class SubChannelProvider;

class DEBUGGER_EXPORT DebuggerRunTool : public ProjectExplorer::RunWorker
{
public:
    enum AllowTerminal { DoAllowTerminal, DoNotAllowTerminal };
    explicit DebuggerRunTool(ProjectExplorer::RunControl *runControl,
                             AllowTerminal allowTerminal = DoAllowTerminal);
    ~DebuggerRunTool() override;

    void start() override;
    void stop() override;

    void setUseDebugServer(Utils::ProcessHandle attachPid, bool essential, bool useMulti);

    void setTestCase(int testCase);

    void kickoffTerminalProcess();
    void interruptTerminal();

    void addQmlServerInferiorCommandLineArgumentIfNeeded();
    void setupPortsGatherer();

    void modifyDebuggerEnvironment(const Utils::EnvironmentItems &item);

    DebuggerRunParameters &runParameters() { return m_runParameters; }

private:
    void showMessage(const QString &msg, int channel = LogDebug, int timeout = -1);

    void handleEngineStarted(Internal::DebuggerEngine *engine);
    void handleEngineFinished(Internal::DebuggerEngine *engine);

    void startCoreFileSetupIfNeededAndContinueStartup();
    void continueAfterCoreFileSetup();

    void startTerminalIfNeededAndContinueStartup();
    void continueAfterTerminalStart();

    void startDebugServerIfNeededAndContinueStartup();
    void continueAfterDebugServerStart();

    Internal::DebuggerRunToolPrivate *d;
    QList<QPointer<Internal::DebuggerEngine>> m_engines;
    DebuggerRunParameters m_runParameters;
};

class DebuggerRunWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    DebuggerRunWorkerFactory();
};

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

extern DEBUGGER_EXPORT const char DebugServerRunnerWorkerId[];
extern DEBUGGER_EXPORT const char GdbServerPortGathererWorkerId[];

} // Debugger
