// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxdebugsupport.h"

#include "remotelinux_constants.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <debugger/debuggerruncontrol.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux::Internal {

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
        setProducer([](RunControl *rc) {
            auto debugger = new DebuggerRunTool(rc, DebuggerRunTool::DoNotAllowTerminal);
            debugger->setId("RemoteLinuxDebugWorker");

            debugger->setupPortsGatherer();
            debugger->addQmlServerInferiorCommandLineArgumentIfNeeded();
            debugger->setUseDebugServer({}, true, true);

            debugger->setStartMode(AttachToRemoteServer);
            debugger->setCloseMode(KillAndExitMonitorAtClose);
            debugger->setUseExtendedRemote(true);

            if (rc->device()->osType() == Utils::OsTypeMac)
                debugger->setLldbPlatform("remote-macosx");
            else
                debugger->setLldbPlatform("remote-linux");
            return debugger;
        });
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
        setProducer([](RunControl *runControl) {
            runControl->requestQmlChannel();

            auto worker = new SimpleTargetRunner(runControl);
            worker->setId("RemoteLinuxQmlToolingSupport");

            auto runworker = runControl->createWorker(runnerIdForRunMode(runControl->runMode()));
            runworker->addStartDependency(worker);
            worker->addStopDependency(runworker);

            worker->setStartModifier([worker, runControl] {
                QmlDebugServicesPreset services = servicesForRunMode(runControl->runMode());

                CommandLine cmd = worker->commandLine();
                cmd.addArg(qmlDebugTcpArguments(services, worker->qmlChannel()));
                worker->setCommandLine(cmd);
            });
            return worker;
        });
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
