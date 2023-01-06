// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxdebugsupport.h"

#include "remotelinux_constants.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <debugger/debuggerruncontrol.h>

using namespace Debugger;
using namespace ProjectExplorer;

namespace RemoteLinux::Internal {

class RemoteLinuxDebugWorker final : public DebuggerRunTool
{
public:
    RemoteLinuxDebugWorker(RunControl *runControl)
        : DebuggerRunTool(runControl, DoNotAllowTerminal)
    {
        setId("RemoteLinuxDebugWorker");

        setUsePortsGatherer(isCppDebugging(), isQmlDebugging());
        addQmlServerInferiorCommandLineArgumentIfNeeded();

        auto debugServer = new DebugServerRunner(runControl, portsGatherer());
        debugServer->setEssential(true);

        addStartDependency(debugServer);

        setStartMode(AttachToRemoteServer);
        setCloseMode(KillAndExitMonitorAtClose);
        setUseExtendedRemote(true);
        setLldbPlatform("remote-linux");
    }
};

// Factories

RemoteLinuxRunWorkerFactory::RemoteLinuxRunWorkerFactory(const QList<Utils::Id> &runConfigs)
{
    setProduct<SimpleTargetRunner>();
    addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    setSupportedRunConfigs(runConfigs);
    addSupportedDeviceType(Constants::GenericLinuxOsType);
}

RemoteLinuxDebugWorkerFactory::RemoteLinuxDebugWorkerFactory(const QList<Utils::Id> &runConfigs)
{
    setProduct<RemoteLinuxDebugWorker>();
    addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    setSupportedRunConfigs(runConfigs);
    addSupportedDeviceType(Constants::GenericLinuxOsType);
}

} // Internal::Internal
