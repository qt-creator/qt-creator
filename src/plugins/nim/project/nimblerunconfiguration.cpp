// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimblerunconfiguration.h"

#include "nimbuildsystem.h"
#include "nimconstants.h"
#include "nimtr.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

// NimbleRunConfiguration

class NimbleRunConfiguration : public RunConfiguration
{
public:
    NimbleRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        environment.setSupportForBuildEnvironment(target);

        executable.setDeviceSelector(target, ExecutableAspect::RunDevice);

        arguments.setMacroExpander(macroExpander());

        workingDir.setMacroExpander(macroExpander());

        setUpdater([this] {
            BuildTargetInfo bti = buildTargetInfo();
            setDisplayName(bti.displayName);
            setDefaultDisplayName(bti.displayName);
            executable.setExecutable(bti.targetFilePath);
            workingDir.setDefaultWorkingDirectory(bti.workingDirectory);
        });

        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
        update();
    }

    EnvironmentAspect environment{this};
    ExecutableAspect executable{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    TerminalAspect terminal{this};
};

NimbleRunConfigurationFactory::NimbleRunConfigurationFactory()
    : RunConfigurationFactory()
{
    registerRunConfiguration<NimbleRunConfiguration>("Nim.NimbleRunConfiguration");
    addSupportedProjectType(Constants::C_NIMBLEPROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}


// NimbleTestConfiguration

class NimbleTestConfiguration : public RunConfiguration
{
public:
    NimbleTestConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        setDisplayName(Tr::tr("Nimble Test"));
        setDefaultDisplayName(Tr::tr("Nimble Test"));

        executable.setDeviceSelector(target, ExecutableAspect::BuildDevice);
        executable.setExecutable(Nim::nimblePathFromKit(kit()));

        arguments.setMacroExpander(macroExpander());
        arguments.setArguments("test");

        workingDir.setMacroExpander(macroExpander());
        workingDir.setDefaultWorkingDirectory(project()->projectDirectory());
    }

    ExecutableAspect executable{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    TerminalAspect terminal{this};
};

NimbleTestConfigurationFactory::NimbleTestConfigurationFactory()
    : FixedRunConfigurationFactory(QString())
{
    registerRunConfiguration<NimbleTestConfiguration>("Nim.NimbleTestConfiguration");
    addSupportedProjectType(Constants::C_NIMBLEPROJECT_ID);
}

} // Nim
