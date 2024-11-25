// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxdebugsupport.h"

#include "remotelinux_constants.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <debugger/debuggerruncontrol.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux::Internal {

class RemoteLinuxDebugWorker final : public DebuggerRunTool
{
public:
    explicit RemoteLinuxDebugWorker(RunControl *runControl)
        : DebuggerRunTool(runControl, DoNotAllowTerminal)
    {
        setId("RemoteLinuxDebugWorker");

        setUsePortsGatherer(isCppDebugging(), isQmlDebugging());
        addQmlServerInferiorCommandLineArgumentIfNeeded();
        setUseDebugServer({}, true, true);

        setStartMode(AttachToRemoteServer);
        setCloseMode(KillAndExitMonitorAtClose);
        setUseExtendedRemote(true);

        if (runControl->device()->osType() == Utils::OsTypeMac)
            setLldbPlatform("remote-macosx");
        else
            setLldbPlatform("remote-linux");
    }
};

class RemoteLinuxQmlToolingSupport final : public SimpleTargetRunner
{
public:
    explicit RemoteLinuxQmlToolingSupport(RunControl *runControl)
        : SimpleTargetRunner(runControl)
    {
        setId("RemoteLinuxQmlToolingSupport");

        runControl->requestQmlChannel();

        auto runworker = runControl->createWorker(QmlDebug::runnerIdForRunMode(runControl->runMode()));
        runworker->addStartDependency(this);
        addStopDependency(runworker);

        setStartModifier([this, runControl] {
            QmlDebug::QmlDebugServicesPreset services = QmlDebug::servicesForRunMode(runControl->runMode());

            CommandLine cmd = commandLine();
            cmd.addArg(QmlDebug::qmlDebugTcpArguments(services, qmlChannel()));
            setCommandLine(cmd);
        });
    }
};

// Factories

static const QList<Id> supportedRunConfigs()
{
    return {
        Constants::RunConfigId,
        Constants::CustomRunConfigId,
        "QmlProjectManager.QmlRunConfiguration"
    };
}

class RemoteLinuxRunWorkerFactory final : public RunWorkerFactory
{
public:
    RemoteLinuxRunWorkerFactory()
    {
        setProduct<SimpleTargetRunner>();
        addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
        addSupportedDeviceType(Constants::GenericLinuxOsType);
        setSupportedRunConfigs(supportedRunConfigs());
    }
};

class RemoteLinuxDebugWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    RemoteLinuxDebugWorkerFactory()
    {
        setProduct<RemoteLinuxDebugWorker>();
        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        addSupportedDeviceType(Constants::GenericLinuxOsType);
        setSupportedRunConfigs(supportedRunConfigs());
    }
};

class RemoteLinuxQmlToolingWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    RemoteLinuxQmlToolingWorkerFactory()
    {
        setProduct<RemoteLinuxQmlToolingSupport>();
        addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
        addSupportedRunMode(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
        addSupportedDeviceType(Constants::GenericLinuxOsType);
        setSupportedRunConfigs(supportedRunConfigs());
    }
};

void setupRemoteLinuxRunAndDebugSupport()
{
    static RemoteLinuxRunWorkerFactory runWorkerFactory;
    static RemoteLinuxDebugWorkerFactory debugWorkerFactory;
    static RemoteLinuxQmlToolingWorkerFactory qmlToolingWorkerFactory;
}

} // RemoteLinux::Internal
