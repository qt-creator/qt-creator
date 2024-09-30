// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"
#include "debuggerconstants.h"
#include "debuggerengine.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>

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

    void startRunControl();

    void start() override;
    void stop() override;

    void setSolibSearchPath(const Utils::FilePaths &list);

    static void setBreakOnMainNextTime();

    void setInferior(const Utils::ProcessRunData &runnable);
    void setInferiorExecutable(const Utils::FilePath &executable);
    void setInferiorEnvironment(const Utils::Environment &env); // Used by GammaRay plugin
    void setRunControlName(const QString &name);
    void setStartMessage(const QString &msg);
    void setCrashParameter(const QString &event);

    void addExpectedSignal(const QString &signal);

    void setStartMode(DebuggerStartMode startMode);
    void setCloseMode(DebuggerCloseMode closeMode);

    void setAttachPid(Utils::ProcessHandle pid);
    void setAttachPid(qint64 pid);

    void setSysRoot(const Utils::FilePath &sysRoot);
    void setSymbolFile(const Utils::FilePath &symbolFile);
    void setRemoteChannel(const QString &channel);
    void setRemoteChannel(const QString &host, int port);
    QString remoteChannel() const;

    void setUseExtendedRemote(bool on);
    void setUseContinueInsteadOfRun(bool on);
    void setContinueAfterAttach(bool on);
    void setBreakOnMain(bool on);
    void setUseTerminal(bool on);
    void setUseDebugServer(Utils::ProcessHandle attachPid, bool essential, bool useMulti);

    void setCommandsAfterConnect(const QString &commands);
    void setCommandsForReset(const QString &commands);

    void setDebugInfoLocation(const Utils::FilePath &debugInfoLocation);

    void setQmlServer(const QUrl &qmlServer);
    QUrl qmlServer() const; // Used in GammaRay integration.

    void setCoreFilePath(const Utils::FilePath &core, bool isSnapshot = false);

    void setTestCase(int testCase);
    void setOverrideStartScript(const Utils::FilePath &script);

    void kickoffTerminalProcess();
    void interruptTerminal();

    Internal::DebuggerRunParameters &runParameters() { return m_runParameters; }

    QUrl debugChannel() const;
    SubChannelProvider *debugChannelProvider() const;

    QUrl qmlChannel() const;
    SubChannelProvider *qmlChannelProvider() const;

protected:
    bool isCppDebugging() const;
    bool isQmlDebugging() const;

    void setUsePortsGatherer(bool useCpp, bool useQml);

    void addSolibSearchDir(const QString &str);
    void addQmlServerInferiorCommandLineArgumentIfNeeded();
    void modifyDebuggerEnvironment(const Utils::EnvironmentItems &item);
    void addSearchDirectory(const Utils::FilePath &dir);

    void setLldbPlatform(const QString &platform);
    void setRemoteChannel(const QUrl &url);
    void setUseTargetAsync(bool on);
    void setSkipExecutableValidation(bool on);
    void setUseCtrlCStub(bool on);

    void setIosPlatform(const QString &platform);
    void setDeviceSymbolsRoot(const QString &deviceSymbolsRoot);
    void setAbi(const ProjectExplorer::Abi &abi);

    DebuggerEngineType cppEngineType() const;

private:
    void showMessage(const QString &msg, int channel = LogDebug, int timeout = -1);

    bool fixupParameters();
    void handleEngineStarted(Internal::DebuggerEngine *engine);
    void handleEngineFinished(Internal::DebuggerEngine *engine);

    void startTerminalIfNeededAndContinueStartup();
    void continueAfterTerminalStart();

    void startDebugServerIfNeededAndContinueStartup();
    void continueAfterDebugServerStart();

    Internal::DebuggerRunToolPrivate *d;
    QList<QPointer<Internal::DebuggerEngine>> m_engines;
    Internal::DebuggerRunParameters m_runParameters;
};

class DEBUGGER_EXPORT SubChannelProvider : public ProjectExplorer::RunWorker
{
public:
    explicit SubChannelProvider(ProjectExplorer::RunControl *runControl);

    void start() final;

    QUrl channel() const { return m_channel; }

private:
    QUrl m_channel;
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
