// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customexecutablerunconfiguration.h"

#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "runconfigurationaspects.h"
#include "target.h"

using namespace Utils;

namespace ProjectExplorer {

// CustomExecutableRunConfiguration

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(BuildConfiguration *bc)
    : CustomExecutableRunConfiguration(bc, Constants::CUSTOM_EXECUTABLE_RUNCONFIG_ID)
{}

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(BuildConfiguration *bc, Id id)
    : RunConfiguration(bc, id)
{
    environment.setSupportForBuildEnvironment(bc);

    executable.setDeviceSelector(kit(), ExecutableAspect::HostDevice);
    executable.setSettingsKey("ProjectExplorer.CustomExecutableRunConfiguration.Executable");
    executable.setReadOnly(false);
    executable.setHistoryCompleter("Qt.CustomExecutable.History");
    executable.setExpectedKind(PathChooser::ExistingCommand);
    executable.setEnvironment(environment.environment());

    workingDir.setEnvironment(&environment);

    connect(&environment, &EnvironmentAspect::environmentChanged, this, [this]  {
         executable.setEnvironment(environment.environment());
    });

    setDefaultDisplayName(defaultDisplayName());
    setUsesEmptyBuildKeys();
}

bool CustomExecutableRunConfiguration::isEnabled(Id) const
{
    return true;
}

QString CustomExecutableRunConfiguration::defaultDisplayName() const
{
    if (executable().isEmpty())
        return Tr::tr("Custom Executable");
    return Tr::tr("Run %1").arg(executable().toUserOutput());
}

Tasks CustomExecutableRunConfiguration::checkForIssues() const
{
    Tasks tasks;
    if (executable().isEmpty()) {
        tasks << createConfigurationIssue(Tr::tr("You need to set an executable in the custom run "
                                             "configuration."));
    }
    return tasks;
}

// Factories

CustomExecutableRunConfigurationFactory::CustomExecutableRunConfigurationFactory() :
    FixedRunConfigurationFactory(Tr::tr("Custom Executable"))
{
    registerRunConfiguration<CustomExecutableRunConfiguration>(
        Constants::CUSTOM_EXECUTABLE_RUNCONFIG_ID);
}

} // namespace ProjectExplorer
