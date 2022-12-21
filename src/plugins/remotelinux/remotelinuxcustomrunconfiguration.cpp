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
    auto envAspect = addAspect<RemoteLinuxEnvironmentAspect>(target);

    auto exeAspect = addAspect<ExecutableAspect>(target, ExecutableAspect::RunDevice);
    exeAspect->setSettingsKey("RemoteLinux.CustomRunConfig.RemoteExecutable");
    exeAspect->setLabelText(Tr::tr("Remote executable:"));
    exeAspect->setDisplayStyle(StringAspect::LineEditDisplay);
    exeAspect->setHistoryCompleter("RemoteLinux.CustomExecutable.History");
    exeAspect->setExpectedKind(PathChooser::Any);

    auto symbolsAspect = addAspect<SymbolFileAspect>();
    symbolsAspect->setSettingsKey("RemoteLinux.CustomRunConfig.LocalExecutable");
    symbolsAspect->setLabelText(Tr::tr("Local executable:"));
    symbolsAspect->setDisplayStyle(SymbolFileAspect::PathChooserDisplay);

    addAspect<ArgumentsAspect>(macroExpander());
    addAspect<WorkingDirectoryAspect>(macroExpander(), envAspect);
    if (HostOsInfo::isAnyUnixHost())
        addAspect<TerminalAspect>();
    if (HostOsInfo::isAnyUnixHost())
        addAspect<X11ForwardingAspect>(macroExpander());

    setRunnableModifier([this](Runnable &r) {
        if (const auto * const forwardingAspect = aspect<X11ForwardingAspect>())
            r.extraData.insert("Ssh.X11ForwardToDisplay", forwardingAspect->display());
    });

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
    registerRunConfiguration<RemoteLinuxCustomRunConfiguration>("RemoteLinux.CustomRunConfig");
    addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
}

} // RemoteLinux::Internal
