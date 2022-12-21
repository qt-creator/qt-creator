// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customexecutablerunconfiguration.h"

#include "localenvironmentaspect.h"
#include "projectexplorerconstants.h"
#include "runconfigurationaspects.h"
#include "target.h"

using namespace Utils;

namespace ProjectExplorer {

const char CUSTOM_EXECUTABLE_RUNCONFIG_ID[] = "ProjectExplorer.CustomExecutableRunConfiguration";

// CustomExecutableRunConfiguration

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(Target *target)
    : CustomExecutableRunConfiguration(target, CUSTOM_EXECUTABLE_RUNCONFIG_ID)
{}

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(Target *target, Id id)
    : RunConfiguration(target, id)
{
    auto envAspect = addAspect<LocalEnvironmentAspect>(target);

    auto exeAspect = addAspect<ExecutableAspect>(target, ExecutableAspect::HostDevice);
    exeAspect->setSettingsKey("ProjectExplorer.CustomExecutableRunConfiguration.Executable");
    exeAspect->setDisplayStyle(StringAspect::PathChooserDisplay);
    exeAspect->setHistoryCompleter("Qt.CustomExecutable.History");
    exeAspect->setExpectedKind(PathChooser::ExistingCommand);
    exeAspect->setEnvironmentChange(EnvironmentChange::fromFixedEnvironment(envAspect->environment()));

    addAspect<ArgumentsAspect>(macroExpander());
    addAspect<WorkingDirectoryAspect>(macroExpander(), envAspect);
    addAspect<TerminalAspect>();

    connect(envAspect, &EnvironmentAspect::environmentChanged, this, [exeAspect, envAspect]  {
         exeAspect->setEnvironmentChange(EnvironmentChange::fromFixedEnvironment(envAspect->environment()));
    });

    setDefaultDisplayName(defaultDisplayName());
}

FilePath CustomExecutableRunConfiguration::executable() const
{
    return aspect<ExecutableAspect>()->executable();
}

bool CustomExecutableRunConfiguration::isEnabled() const
{
    return true;
}

Runnable CustomExecutableRunConfiguration::runnable() const
{
    Runnable r;
    r.command = commandLine();
    r.environment = aspect<EnvironmentAspect>()->environment();
    r.workingDirectory = aspect<WorkingDirectoryAspect>()->workingDirectory();

    if (!r.command.isEmpty()) {
        const FilePath expanded = macroExpander()->expand(r.command.executable());
        r.command.setExecutable(expanded);
    }

    return r;
}

QString CustomExecutableRunConfiguration::defaultDisplayName() const
{
    if (executable().isEmpty())
        return tr("Custom Executable");
    return tr("Run %1").arg(executable().toUserOutput());
}

Tasks CustomExecutableRunConfiguration::checkForIssues() const
{
    Tasks tasks;
    if (executable().isEmpty()) {
        tasks << createConfigurationIssue(tr("You need to set an executable in the custom run "
                                             "configuration."));
    }
    return tasks;
}

// Factories

CustomExecutableRunConfigurationFactory::CustomExecutableRunConfigurationFactory() :
    FixedRunConfigurationFactory(CustomExecutableRunConfiguration::tr("Custom Executable"))
{
    registerRunConfiguration<CustomExecutableRunConfiguration>(CUSTOM_EXECUTABLE_RUNCONFIG_ID);
}

CustomExecutableRunWorkerFactory::CustomExecutableRunWorkerFactory()
{
    setProduct<SimpleTargetRunner>();
    addSupportedRunMode(Constants::NORMAL_RUN_MODE);
    addSupportedRunConfig(CUSTOM_EXECUTABLE_RUNCONFIG_ID);
}

} // namespace ProjectExplorer
