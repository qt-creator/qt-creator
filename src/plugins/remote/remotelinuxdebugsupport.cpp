// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxdebugsupport.h"

#include "remotelinux_constants.h"
#include "remotelinuxtr.h"
#include "windowsdevice.h"

#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>

#include <qmlprojectmanager/qmlprojectconstants.h>

#include <QtTaskTree/QBarrier>

#include <utils/qtcprocess.h>
#include <utils/url.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Remote::Internal {

static const QList<Id> supportedRunConfigs()
{
    return {
        Constants::RunConfigId,
        Constants::CustomRunConfigId,
        QmlProjectManager::Constants::QML_RUNCONFIG_ID
    };
}

class RemoteLinuxRunWorkerFactory final : public RunWorkerFactory
{
public:
    RemoteLinuxRunWorkerFactory()
    {
        setId("RemoteLinuxRunWorkerFactory");
        setRecipeProducer([](RunControl *runControl) {
            return runControl->processRecipe(runControl->processTask());
        });
        addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
        addSupportedDeviceType(Constants::GenericLinuxOsType);
        addSupportedDeviceType(Constants::GenericMacOsType);
        setSupportedRunConfigs(supportedRunConfigs());
        setExecutionType(Constants::ExecutionType);
    }
};

// The native Windows device runs the application on its interactive desktop session (so a GUI
// is visible), not the invisible SSH session; flag the process so WindowsProcessInterface takes
// that path. A dedicated factory keeps this off the Linux/Mac run path.
class WindowsRunWorkerFactory final : public RunWorkerFactory
{
public:
    WindowsRunWorkerFactory()
    {
        setId("WindowsRunWorkerFactory");
        setRecipeProducer([](RunControl *runControl) {
            return runControl->processRecipe([](Process &process) {
                process.setExtraData(Constants::RunInInteractiveSession, true);
            });
        });
        addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
        addSupportedDeviceType(Constants::GenericWindowsOsType);
        setSupportedRunConfigs(supportedRunConfigs());
        setExecutionType(Constants::ExecutionType);
    }
};

// Debugging on the native Windows device uses CDB running on the device (over SSH): cdb.exe
// launches the inferior itself (StartInternal), so no debug channel is needed. The CDB helper
// extension is taken from the device (see WindowsDevice::cdbExtensionDirectory).
class WindowsDebugWorkerFactory final : public RunWorkerFactory
{
public:
    WindowsDebugWorkerFactory()
    {
        setId("WindowsDebugWorkerFactory");
        setRecipeProducer([](RunControl *runControl) {
            DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
            rp.setStartMode(StartInternal);
            rp.setUseTerminal(false);
            if (const auto device
                    = std::dynamic_pointer_cast<const WindowsDevice>(runControl->device())) {
                const FilePath extDir = device->cdbExtensionDirectory();
                if (!extDir.isEmpty())
                    rp.setCdbExtensionPath(device->rootPath().withNewPath(extDir.path()));
            }
            return debuggerRecipe(runControl, rp);
        });
        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        addSupportedDeviceType(Constants::GenericWindowsOsType);
        setSupportedRunConfigs(supportedRunConfigs());
        setExecutionType(Constants::ExecutionType);
    }
};

class RemoteLinuxDebugWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    RemoteLinuxDebugWorkerFactory()
    {
        setId("RemoteLinuxDebugWorkerFactory");
        setRecipeProducer([](RunControl *runControl) {
            runControl->requestDebugChannel();

            DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
            rp.setupPortsGatherer(runControl);
            rp.setUseTerminal(false);
            rp.setAddQmlServerInferiorCmdArgIfNeeded(true);

            rp.setStartMode(AttachToRemoteServer);
            rp.setCloseMode(KillAndExitMonitorAtClose);
            rp.setUseExtendedRemote(true);

            if (runControl->device()->osType() == Utils::OsTypeMac)
                rp.setLldbPlatform("remote-macosx");
            else
                rp.setLldbPlatform("remote-linux");
            return debuggerRecipe(runControl, rp);
        });
        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        addSupportedDeviceType(Constants::GenericLinuxOsType);
        addSupportedDeviceType(Constants::GenericMacOsType);
        setSupportedRunConfigs(supportedRunConfigs());
        setExecutionType(Constants::ExecutionType);
    }
};

class RemoteLinuxQmlToolingWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    RemoteLinuxQmlToolingWorkerFactory()
    {
        setId("RemoteLinuxQmlToolingWorkerFactory");
        setRecipeProducer([](RunControl *runControl) {
            runControl->requestQmlChannel();

            const auto modifier = [runControl](Process &process) {
                QmlDebugServicesPreset services = servicesForRunMode(runControl->runMode());

                CommandLine cmd = runControl->commandLine();
                cmd.addArg(qmlDebugTcpArguments(services, runControl->qmlChannel()));
                process.setCommand(cmd);
            };
            const ProcessTask processTask(runControl->processTaskWithModifier(modifier));
            return Group {
                When (processTask, &Process::started, WorkflowPolicy::StopOnSuccessOrError) >> Do {
                    runControl->createRecipe(runnerIdForRunMode(runControl->runMode()))
                }
            };
        });
        addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
        addSupportedRunMode(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
        addSupportedDeviceType(Constants::GenericLinuxOsType);
        addSupportedDeviceType(Constants::GenericMacOsType);
        setSupportedRunConfigs(supportedRunConfigs());
        setExecutionType(Constants::ExecutionType);
    }
};

// QML profiling and previewing on the native Windows device. As for a normal run, the
// application starts on the device's interactive desktop session. Windows blocks an incoming
// connection to the application's QML debug port, so the application binds that port to the
// device's loopback interface and Creator reaches it through an SSH forward.
class WindowsQmlToolingWorkerFactory final : public RunWorkerFactory
{
public:
    WindowsQmlToolingWorkerFactory()
    {
        setId("WindowsQmlToolingWorkerFactory");
        setRecipeProducer([](RunControl *runControl) {
            runControl->requestQmlChannel();

            const Storage<int> devicePortStorage;
            const Storage<QUrl> localUrlStorage;

            // The device port exists only once the ports gatherer has run, i.e. inside the
            // recipe, and the channel below stops carrying it as soon as it is rewritten.
            // The local one is taken as late as possible: nothing holds it between here and
            // ssh binding it.
            const auto onPorts = [runControl, devicePortStorage, localUrlStorage] {
                *devicePortStorage = runControl->qmlChannel().port();
                if (*devicePortStorage <= 0) {
                    runControl->postMessage(Tr::tr("No free port found on the device."),
                                            ErrorMessageFormat);
                    return false;
                }
                *localUrlStorage = Utils::urlFromLocalHostAndFreePort();
                if (localUrlStorage->port() <= 0) {
                    runControl->postMessage(Tr::tr("No free port found on this host."),
                                            ErrorMessageFormat);
                    return false;
                }
                return true;
            };

            const auto modifier = [runControl, devicePortStorage, localUrlStorage](
                                      Process &process) {
                const QmlDebugServicesPreset services = servicesForRunMode(runControl->runMode());

                const QString deviceAddress = "127.0.0.1";
                const int devicePort = *devicePortStorage;
                const QUrl &localUrl = *localUrlStorage;

                // It has to be the literal address: Qt parses the host as an IP address and
                // falls back to every interface, with a warning, for a name it cannot parse.
                QUrl server;
                server.setHost(deviceAddress);
                server.setPort(devicePort);

                CommandLine cmd = runControl->commandLine();
                cmd.addArg(qmlDebugDesktopTcpArguments(services, server));
                process.setCommand(cmd);

                process.setExtraData(Constants::RunInInteractiveSession, true);
                process.setExtraData(Constants::SshForwardAddress, deviceAddress);
                process.setExtraData(Constants::SshForwardPort, devicePort);
                process.setExtraData(Constants::SshForwardLocalPort, localUrl.port());
                process.setExtraData(Constants::SshForwardLocalAddress, localUrl.host());

                QUrl channel = runControl->qmlChannel();
                channel.setHost(localUrl.host());
                channel.setPort(localUrl.port());
                runControl->setQmlChannel(channel);
            };
            const ProcessTask processTask(runControl->processTaskWithModifier(modifier));
            return Group {
                devicePortStorage,
                localUrlStorage,
                QSyncTask(onPorts),
                When (processTask, &Process::started, WorkflowPolicy::StopOnSuccessOrError) >> Do {
                    runControl->createRecipe(runnerIdForRunMode(runControl->runMode()))
                }
            };
        });
        addSupportedRunMode(ProjectExplorer::Constants::QML_PROFILER_RUN_MODE);
        addSupportedRunMode(ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE);
        addSupportedDeviceType(Constants::GenericWindowsOsType);
        setSupportedRunConfigs(supportedRunConfigs());
        setExecutionType(Constants::ExecutionType);
    }
};

void setupRemoteLinuxRunAndDebugSupport()
{
    static RemoteLinuxRunWorkerFactory runWorkerFactory;
    static WindowsRunWorkerFactory windowsRunWorkerFactory;
    static RemoteLinuxDebugWorkerFactory debugWorkerFactory;
    static RemoteLinuxQmlToolingWorkerFactory qmlToolingWorkerFactory;
    static WindowsDebugWorkerFactory windowsDebugWorkerFactory;
    static WindowsQmlToolingWorkerFactory windowsQmlToolingWorkerFactory;
}

} // Remote::Internal
