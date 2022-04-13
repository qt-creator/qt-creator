/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "customexecutablerunconfiguration.h"

#include "devicesupport/devicemanager.h"
#include "localenvironmentaspect.h"
#include "target.h"

using namespace Utils;

namespace ProjectExplorer {

const char CUSTOM_EXECUTABLE_RUNCONFIG_ID[] = "ProjectExplorer.CustomExecutableRunConfiguration";

// CustomExecutableRunConfiguration

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(Target *target)
    : CustomExecutableRunConfiguration(target, CUSTOM_EXECUTABLE_RUNCONFIG_ID)
{}

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(Target *target, Utils::Id id)
    : RunConfiguration(target, id)
{
    auto envAspect = addAspect<LocalEnvironmentAspect>(target);

    auto exeAspect = addAspect<ExecutableAspect>();
    exeAspect->setSettingsKey("ProjectExplorer.CustomExecutableRunConfiguration.Executable");
    exeAspect->setDisplayStyle(StringAspect::PathChooserDisplay);
    exeAspect->setHistoryCompleter("Qt.CustomExecutable.History");
    exeAspect->setExpectedKind(PathChooser::ExistingCommand);
    exeAspect->setEnvironmentChange(EnvironmentChange::fromFixedEnvironment(envAspect->environment()));

    addAspect<ArgumentsAspect>();
    addAspect<WorkingDirectoryAspect>(envAspect);
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
    const FilePath workingDirectory = aspect<WorkingDirectoryAspect>()->workingDirectory();

    Runnable r;
    r.command = commandLine();
    r.environment = aspect<EnvironmentAspect>()->environment();
    r.workingDirectory = workingDirectory;
    r.device = DeviceManager::defaultDesktopDevice();

    if (!r.command.isEmpty()) {
        const FilePath expanded = macroExpander()->expand(r.command.executable());
        r.command.setExecutable(r.environment.searchInPath(expanded.toString(), {workingDirectory}));
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
