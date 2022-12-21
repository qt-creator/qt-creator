// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxdebugsupport.h"

#include <projectexplorer/runconfigurationaspects.h>

using namespace Debugger;
using namespace ProjectExplorer;

namespace RemoteLinux::Internal {

LinuxDeviceDebugSupport::LinuxDeviceDebugSupport(RunControl *runControl)
    : DebuggerRunTool(runControl, DebuggerRunTool::DoNotAllowTerminal)
{
    setId("LinuxDeviceDebugSupport");

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

} // Internal::Internal
