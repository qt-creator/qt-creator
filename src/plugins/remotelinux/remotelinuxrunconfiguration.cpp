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
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/hostosinfo.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux::Internal {

class RemoteLinuxRunConfiguration final : public RunConfiguration
{
public:
    RemoteLinuxRunConfiguration(Target *target, Id id);
};

RemoteLinuxRunConfiguration::RemoteLinuxRunConfiguration(Target *target, Id id)
    : RunConfiguration(target, id)
{
    auto envAspect = addAspect<RemoteLinuxEnvironmentAspect>(target);

    auto exeAspect = addAspect<ExecutableAspect>(target, ExecutableAspect::RunDevice);
    exeAspect->setLabelText(Tr::tr("Executable on device:"));
    exeAspect->setPlaceHolderText(Tr::tr("Remote path not set"));
    exeAspect->makeOverridable("RemoteLinux.RunConfig.AlternateRemoteExecutable",
                               "RemoteLinux.RunConfig.UseAlternateRemoteExecutable");
    exeAspect->setHistoryCompleter("RemoteLinux.AlternateExecutable.History");

    auto symbolsAspect = addAspect<SymbolFileAspect>();
    symbolsAspect->setLabelText(Tr::tr("Executable on host:"));
    symbolsAspect->setDisplayStyle(SymbolFileAspect::LabelDisplay);

    addAspect<ArgumentsAspect>(macroExpander());
    addAspect<WorkingDirectoryAspect>(macroExpander(), envAspect);
    if (HostOsInfo::isAnyUnixHost())
        addAspect<TerminalAspect>();
    if (HostOsInfo::isAnyUnixHost())
        addAspect<X11ForwardingAspect>(macroExpander());

    auto libAspect = addAspect<UseLibraryPathsAspect>();
    libAspect->setValue(false);
    connect(libAspect, &UseLibraryPathsAspect::changed,
            envAspect, &EnvironmentAspect::environmentChanged);

    setUpdater([this, target, exeAspect, symbolsAspect, libAspect] {
        const IDeviceConstPtr buildDevice = BuildDeviceKitAspect::device(target->kit());
        const IDeviceConstPtr runDevice = DeviceKitAspect::device(target->kit());
        QTC_ASSERT(buildDevice, return);
        QTC_ASSERT(runDevice, return);
        const BuildTargetInfo bti = buildTargetInfo();
        const FilePath localExecutable = bti.targetFilePath;
        const DeploymentData deploymentData = target->deploymentData();
        const DeployableFile depFile = deploymentData.deployableForLocalFile(localExecutable);

        exeAspect->setExecutable(runDevice->filePath(depFile.remoteFilePath()));
        symbolsAspect->setFilePath(localExecutable);
        libAspect->setEnabled(buildDevice == runDevice);
    });

    setRunnableModifier([this](Runnable &r) {
        if (const auto * const forwardingAspect = aspect<X11ForwardingAspect>())
            r.extraData.insert("Ssh.X11ForwardToDisplay", forwardingAspect->display());
    });

    envAspect->addModifier([this, libAspect](Environment &env) {
        BuildTargetInfo bti = buildTargetInfo();
        if (bti.runEnvModifier)
            bti.runEnvModifier(env, libAspect->value());
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
