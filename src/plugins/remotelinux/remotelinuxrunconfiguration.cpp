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

namespace RemoteLinux::Internal {

class RemoteLinuxRunConfiguration final : public RunConfiguration
{
public:
    RemoteLinuxRunConfiguration(BuildConfiguration *bc, Id id);

    RemoteLinuxEnvironmentAspect environment{this};
    ExecutableAspect executable{this};
    SymbolFileAspect symbolFile{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    TerminalAspect terminal{this};
    X11ForwardingAspect x11Forwarding{this};
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

    terminal.setVisible(HostOsInfo::isAnyUnixHost());

    connect(&useLibraryPath, &BaseAspect::changed,
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
        symbolFile.setValue(localExecutable);

        // Hack for remote build == run: deploymentData is empty when the deploy step is disabled.
        if (executable().isEmpty() && buildDevice == runDevice)
            executable.setExecutable(localExecutable);

        useLibraryPath.setEnabled(buildDevice == runDevice);
    });

    environment.addModifier([this](Environment &env) {
        BuildTargetInfo bti = buildTargetInfo();
        if (bti.runEnvModifier)
            bti.runEnvModifier(env, useLibraryPath());
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
        addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
    }
};

void setupRemoteLinuxRunConfiguration()
{
    static RemoteLinuxRunConfigurationFactory theRemoteLinuxRunConfigurationFactory;
}

} // RemoteLinux::Internal
