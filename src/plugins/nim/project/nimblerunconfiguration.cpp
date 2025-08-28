// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimblerunconfiguration.h"

#include "nimconstants.h"
#include "nimproject.h"
#include "nimtr.h"

#include <projectexplorer/project.h>
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
    NimbleRunConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {
        environment.setSupportForBuildEnvironment(bc);

        executable.setDeviceSelector(kit(), ExecutableAspect::RunDevice);

        setUpdater([this] {
            BuildTargetInfo bti = buildTargetInfo();
            setDisplayName(bti.displayName);
            setDefaultDisplayName(bti.displayName);
            executable.setExecutable(bti.targetFilePath);
            workingDir.setDefaultWorkingDirectory(bti.workingDirectory);
        });
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
    NimbleTestConfiguration(BuildConfiguration *bc, Id id)
        : RunConfiguration(bc, id)
    {
        setDisplayName(Tr::tr("Nimble Test"));
        setDefaultDisplayName(Tr::tr("Nimble Test"));

        executable.setDeviceSelector(kit(), ExecutableAspect::BuildDevice);
        executable.setExecutable(Nim::nimblePathFromKit(kit()));

        arguments.setArguments("test");

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
