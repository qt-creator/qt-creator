// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxrunconfiguration.h"

#include "remotelinux_constants.h"
#include "remotelinuxenvironmentaspect.h"
#include "remotelinuxtr.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspects.h>
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
    RemoteLinuxRunConfiguration(Target *target, Id id);

    RemoteLinuxEnvironmentAspect environment{this};
    ExecutableAspect executable{this};
    SymbolFileAspect symbolFile{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    TerminalAspect terminal{this};
    X11ForwardingAspect x11Forwarding{this};
    UseLibraryPathsAspect useLibraryPath{this};
};

RemoteLinuxRunConfiguration::RemoteLinuxRunConfiguration(Target *target, Id id)
    : RunConfiguration(target, id)
{
    environment.setDeviceSelector(target, EnvironmentAspect::RunDevice);

    executable.setDeviceSelector(target, ExecutableAspect::RunDevice);
    executable.setLabelText(Tr::tr("Executable on device:"));
    executable.setPlaceHolderText(Tr::tr("Remote path not set"));
    executable.makeOverridable("RemoteLinux.RunConfig.AlternateRemoteExecutable",
                               "RemoteLinux.RunConfig.UseAlternateRemoteExecutable");
    executable.setHistoryCompleter("RemoteLinux.AlternateExecutable.History");

    symbolFile.setLabelText(Tr::tr("Executable on host:"));

    arguments.setMacroExpander(macroExpander());

    workingDir.setMacroExpander(macroExpander());
    workingDir.setEnvironment(&environment);

    terminal.setVisible(HostOsInfo::isAnyUnixHost());

    x11Forwarding.setMacroExpander(macroExpander());

    connect(&useLibraryPath, &BaseAspect::changed,
            &environment, &EnvironmentAspect::environmentChanged);

    setUpdater([this, target] {
        const IDeviceConstPtr buildDevice = BuildDeviceKitAspect::device(target->kit());
        const IDeviceConstPtr runDevice = DeviceKitAspect::device(target->kit());
        QTC_ASSERT(buildDevice, return);
        QTC_ASSERT(runDevice, return);
        const BuildTargetInfo bti = buildTargetInfo();
        const FilePath localExecutable = bti.targetFilePath;
        const DeploymentData deploymentData = target->deploymentData();
        const DeployableFile depFile = deploymentData.deployableForLocalFile(localExecutable);

        executable.setExecutable(runDevice->filePath(depFile.remoteFilePath()));
        symbolFile.setValue(localExecutable);
        useLibraryPath.setEnabled(buildDevice == runDevice);
    });

    environment.addModifier([this](Environment &env) {
        BuildTargetInfo bti = buildTargetInfo();
        if (bti.runEnvModifier)
            bti.runEnvModifier(env, useLibraryPath());
    });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    connect(target, &Target::deploymentDataChanged, this, &RunConfiguration::update);
    connect(target, &Target::kitChanged, this, &RunConfiguration::update);
}

// RemoteLinuxRunConfigurationFactory

RemoteLinuxRunConfigurationFactory::RemoteLinuxRunConfigurationFactory()
{
    registerRunConfiguration<RemoteLinuxRunConfiguration>(Constants::RunConfigId);
    setDecorateDisplayNames(true);
    addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
}

} // RemoteLinux::Internal
