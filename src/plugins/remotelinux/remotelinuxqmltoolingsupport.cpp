// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxqmltoolingsupport.h"

#include "remotelinux_constants.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux::Internal {

class RemoteLinuxQmlToolingSupport : public SimpleTargetRunner
{
public:
    explicit RemoteLinuxQmlToolingSupport(RunControl *runControl)
        : SimpleTargetRunner(runControl)
    {
        setId("RemoteLinuxQmlToolingSupport");

        auto portsGatherer = new PortsGatherer(runControl);
        addStartDependency(portsGatherer);

        // The ports gatherer can safely be stopped once the process is running, even though it has to
        // be started before.
        addStopDependency(portsGatherer);

        auto runworker = runControl->createWorker(QmlDebug::runnerIdForRunMode(runControl->runMode()));
        runworker->addStartDependency(this);
        addStopDependency(runworker);

        setStartModifier([this, runControl, portsGatherer, runworker] {
            const QUrl serverUrl = portsGatherer->findEndPoint();
            runworker->recordData("QmlServerUrl", serverUrl);

            QmlDebug::QmlDebugServicesPreset services = QmlDebug::servicesForRunMode(runControl->runMode());

            CommandLine cmd = commandLine();
            cmd.addArg(QmlDebug::qmlDebugTcpArguments(services, serverUrl));
            setCommandLine(cmd);
        });
    }
};

RemoteLinuxQmlToolingWorkerFactory::RemoteLinuxQmlToolingWorkerFactory(const QList<Utils::Id> &runConfigs)
{
    setProduct<RemoteLinuxQmlToolingSupport>();
    addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
    addSupportedRunMode(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
    setSupportedRunConfigs(runConfigs);
    addSupportedDeviceType(Constants::GenericLinuxOsType);
}

} // RemoteLinux::Internal
