// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxrunconfiguration.h"

#include "remotelinux_constants.h"
#include "remotelinuxenvironmentaspect.h"
#include "remotelinuxtr.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <utils/hostosinfo.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Remote::Internal {

class RemoteLinuxRunConfiguration final : public RunConfiguration
{
public:
    RemoteLinuxRunConfiguration(BuildConfiguration *bc, Id id);

    RemoteLinuxEnvironmentAspect environment{this};
    ExecutableAspect executable{this};
    SymbolFileAspect symbolFile{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    RunAsAspect runAs{this};
    TerminalAspect terminal{this};
    X11ForwardingAspect x11Forwarding{this};
    UseVncDisplayAspect useVncDisplay{this};
    UseLibraryPathsAspect useLibraryPath{this};
};

RemoteLinuxRunConfiguration::RemoteLinuxRunConfiguration(BuildConfiguration *bc, Id id)
    : RunConfiguration(bc, id)
{
    environment.setDeviceSelector(kit(), EnvironmentAspect::RunDevice);

    executable.setDeviceSelector(kit(), ExecutableAspect::RunDevice);
    executable.setLabelText(Tr::tr("Executable on device:"));
    executable.setPlaceHolderText(Tr::tr("Remote path not set"));
    executable.makeOverridable("RemoteLinux.RunConfig.AlternateRemoteExecutable",
                               "RemoteLinux.RunConfig.UseAlternateRemoteExecutable");
    executable.setHistoryCompleter("RemoteLinux.AlternateExecutable.History");

    symbolFile.setLabelText(Tr::tr("Executable on host:"));

    workingDir.setEnvironment(&environment);

    connect(&useLibraryPath, &BaseAspect::changed,
            &environment, &EnvironmentAspect::environmentChanged);
    connect(&useVncDisplay, &BaseAspect::changed,
            &environment, &EnvironmentAspect::environmentChanged);

    setUpdater([this] {
        const IDeviceConstPtr buildDevice = BuildDeviceKitAspect::device(kit());
        const IDeviceConstPtr runDevice = RunDeviceKitAspect::device(kit());
        QTC_ASSERT(buildDevice, return);
        QTC_ASSERT(runDevice, return);
        const BuildTargetInfo bti = buildTargetInfo();
        const FilePath localExecutable = bti.targetFilePath;
        const DeploymentData deploymentData = buildSystem()->deploymentData();
        const DeployableFile depFile = deploymentData.deployableForLocalFile(localExecutable);

        executable.setExecutable(runDevice->filePath(depFile.remoteFilePath()));
        if (executable().isEmpty() && buildDevice == runDevice)
            executable.setExecutable(localExecutable);
        symbolFile.setValue(localExecutable);

        useLibraryPath.setEnabled(buildDevice == runDevice);

        // The run device can be a native Windows machine; its runtime library search path is
        // PATH (handled OS-aware by useLibraryPath), but the X11/VNC/run-as options and a remote
        // terminal do not apply, so hide them for a Windows run device.
        const bool isWindowsDevice = runDevice->osType() == OsTypeWindows;
        x11Forwarding.setVisible(!isWindowsDevice);
        useVncDisplay.setVisible(!isWindowsDevice);
        runAs.setVisible(!isWindowsDevice);
        terminal.setVisible(!isWindowsDevice && HostOsInfo::isAnyUnixHost());
    });

    environment.addModifier([this](Environment &env) {
        BuildTargetInfo bti = buildTargetInfo();
        if (bti.runEnvModifier)
            bti.runEnvModifier(env, useLibraryPath());
        if (useVncDisplay())
            env.set("QT_QPA_PLATFORM", "vnc");
    });
}

// RemoteLinuxRunConfigurationFactory

class RemoteLinuxRunConfigurationFactory final : public ProjectExplorer::RunConfigurationFactory
{
public:
    RemoteLinuxRunConfigurationFactory()
    {
        registerRunConfiguration<RemoteLinuxRunConfiguration>(Constants::RunConfigId);
        setDecorateDisplayNames(true);
        addSupportedTargetDeviceType(Constants::GenericLinuxOsType);
        addSupportedTargetDeviceType(Constants::GenericMacOsType);
        addSupportedTargetDeviceType(Constants::GenericWindowsOsType);
    }
};

void setupRemoteLinuxRunConfiguration()
{
    static RemoteLinuxRunConfigurationFactory theRemoteLinuxRunConfigurationFactory;
}

} // Remote::Internal
