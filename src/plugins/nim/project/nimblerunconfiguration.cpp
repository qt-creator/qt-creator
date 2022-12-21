// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimblerunconfiguration.h"

#include "nimbuildsystem.h"
#include "nimconstants.h"
#include "nimtr.h"

#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/environment.h>

using namespace ProjectExplorer;

namespace Nim {

// NimbleRunConfiguration

class NimbleRunConfiguration : public RunConfiguration
{
public:
    NimbleRunConfiguration(Target *target, Utils::Id id)
        : RunConfiguration(target, id)
    {
        auto envAspect = addAspect<LocalEnvironmentAspect>(target);
        addAspect<ExecutableAspect>(target, ExecutableAspect::RunDevice);
        addAspect<ArgumentsAspect>(macroExpander());
        addAspect<WorkingDirectoryAspect>(macroExpander(), envAspect);
        addAspect<TerminalAspect>();

        setUpdater([this] {
            BuildTargetInfo bti = buildTargetInfo();
            setDisplayName(bti.displayName);
            setDefaultDisplayName(bti.displayName);
            aspect<ExecutableAspect>()->setExecutable(bti.targetFilePath);
            aspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(bti.workingDirectory);
        });

        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
        update();
    }
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
    NimbleTestConfiguration(ProjectExplorer::Target *target, Utils::Id id)
        : RunConfiguration(target, id)
    {
        addAspect<ExecutableAspect>(target, ExecutableAspect::BuildDevice)
                ->setExecutable(Nim::nimblePathFromKit(target->kit()));
        addAspect<ArgumentsAspect>(macroExpander())->setArguments("test");
        addAspect<WorkingDirectoryAspect>(macroExpander(), nullptr)
                ->setDefaultWorkingDirectory(project()->projectDirectory());
        addAspect<TerminalAspect>();

        setDisplayName(Tr::tr("Nimble Test"));
        setDefaultDisplayName(Tr::tr("Nimble Test"));
    }
};

NimbleTestConfigurationFactory::NimbleTestConfigurationFactory()
    : FixedRunConfigurationFactory(QString())
{
    registerRunConfiguration<NimbleTestConfiguration>("Nim.NimbleTestConfiguration");
    addSupportedProjectType(Constants::C_NIMBLEPROJECT_ID);
}

} // Nim
