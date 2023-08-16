// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxcustomrunconfiguration.h"

#include "remotelinux_constants.h"
#include "remotelinuxtr.h"
#include "remotelinuxenvironmentaspect.h"

#include <projectexplorer/runconfigurationaspects.h>
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

    RemoteLinuxEnvironmentAspect environment{this};
    ExecutableAspect executable{this};
    SymbolFileAspect symbolFile{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    TerminalAspect terminal{this};
    X11ForwardingAspect x11Forwarding{this};
};

RemoteLinuxCustomRunConfiguration::RemoteLinuxCustomRunConfiguration(Target *target, Id id)
    : RunConfiguration(target, id)
{
    environment.setDeviceSelector(target, EnvironmentAspect::RunDevice);

    executable.setDeviceSelector(target, ExecutableAspect::RunDevice);
    executable.setSettingsKey("RemoteLinux.CustomRunConfig.RemoteExecutable");
    executable.setLabelText(Tr::tr("Remote executable:"));
    executable.setReadOnly(false);
    executable.setHistoryCompleter("RemoteLinux.CustomExecutable.History");
    executable.setExpectedKind(PathChooser::Any);

    symbolFile.setSettingsKey("RemoteLinux.CustomRunConfig.LocalExecutable");
    symbolFile.setLabelText(Tr::tr("Local executable:"));

    arguments.setMacroExpander(macroExpander());

    workingDir.setMacroExpander(macroExpander());
    workingDir.setEnvironment(&environment);

    terminal.setVisible(HostOsInfo::isAnyUnixHost());

    x11Forwarding.setMacroExpander(macroExpander());

    setDefaultDisplayName(runConfigDefaultDisplayName());
}

QString RemoteLinuxCustomRunConfiguration::runConfigDefaultDisplayName()
{
    FilePath remoteExecutable = executable();
    QString display = remoteExecutable.isEmpty()
            ? Tr::tr("Custom Executable")
            : Tr::tr("Run \"%1\"").arg(remoteExecutable.toUserOutput());
    return RunConfigurationFactory::decoratedTargetName(display, target());
}

Tasks RemoteLinuxCustomRunConfiguration::checkForIssues() const
{
    Tasks tasks;
    if (executable().isEmpty()) {
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
