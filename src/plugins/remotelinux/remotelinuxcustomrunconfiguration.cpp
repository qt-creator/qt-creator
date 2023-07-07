// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxcustomrunconfiguration.h"

#include "remotelinux_constants.h"
#include "remotelinuxtr.h"
#include "remotelinuxenvironmentaspect.h"

#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/hostosinfo.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux::Internal {

class RemoteLinuxCustomRunConfiguration : public RunConfiguration
{
public:
    RemoteLinuxCustomRunConfiguration(Target *target, Id id);

    QString runConfigDefaultDisplayName();

private:
    Tasks checkForIssues() const override;
};

RemoteLinuxCustomRunConfiguration::RemoteLinuxCustomRunConfiguration(Target *target, Id id)
    : RunConfiguration(target, id)
{
    auto envAspect = addAspect<RemoteLinuxEnvironmentAspect>();
    envAspect->setDeviceSelector(target, EnvironmentAspect::RunDevice);

    auto exeAspect = addAspect<ExecutableAspect>();
    exeAspect->setDeviceSelector(target, ExecutableAspect::RunDevice);
    exeAspect->setSettingsKey("RemoteLinux.CustomRunConfig.RemoteExecutable");
    exeAspect->setLabelText(Tr::tr("Remote executable:"));
    exeAspect->setReadOnly(false);
    exeAspect->setHistoryCompleter("RemoteLinux.CustomExecutable.History");
    exeAspect->setExpectedKind(PathChooser::Any);

    auto symbolsAspect = addAspect<FilePathAspect>();
    symbolsAspect->setSettingsKey("RemoteLinux.CustomRunConfig.LocalExecutable");
    symbolsAspect->setLabelText(Tr::tr("Local executable:"));

    auto argsAspect = addAspect<ArgumentsAspect>();
    argsAspect->setMacroExpander(macroExpander());

    auto workingDirAspect = addAspect<WorkingDirectoryAspect>();
    workingDirAspect->setMacroExpander(macroExpander());
    workingDirAspect->setEnvironment(envAspect);

    if (HostOsInfo::isAnyUnixHost())
        addAspect<TerminalAspect>();

    if (HostOsInfo::isAnyUnixHost()) {
        auto x11Forwarding = addAspect<X11ForwardingAspect>();
        x11Forwarding->setMacroExpander(macroExpander());
    }

    setDefaultDisplayName(runConfigDefaultDisplayName());
}

QString RemoteLinuxCustomRunConfiguration::runConfigDefaultDisplayName()
{
    QString remoteExecutable = aspect<ExecutableAspect>()->executable().toString();
    QString display = remoteExecutable.isEmpty()
            ? Tr::tr("Custom Executable") : Tr::tr("Run \"%1\"").arg(remoteExecutable);
    return  RunConfigurationFactory::decoratedTargetName(display, target());
}

Tasks RemoteLinuxCustomRunConfiguration::checkForIssues() const
{
    Tasks tasks;
    if (aspect<ExecutableAspect>()->executable().isEmpty()) {
        tasks << createConfigurationIssue(Tr::tr("The remote executable must be set in order to run "
                                                 "a custom remote run configuration."));
    }
    return tasks;
}

// RemoteLinuxCustomRunConfigurationFactory

RemoteLinuxCustomRunConfigurationFactory::RemoteLinuxCustomRunConfigurationFactory()
    : FixedRunConfigurationFactory(Tr::tr("Custom Executable"), true)
{
    registerRunConfiguration<RemoteLinuxCustomRunConfiguration>(Constants::CustomRunConfigId);
    addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
}

} // RemoteLinux::Internal
