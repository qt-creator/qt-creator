// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonrunconfiguration.h"

#include "mesonpluginconstants.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/hostosinfo.h>

#include <debugger/debuggerruncontrol.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace MesonProjectManager::Internal {

class MesonRunConfiguration final : public RunConfiguration
{
public:
    MesonRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        environment.setSupportForBuildEnvironment(target);

        executable.setDeviceSelector(target, ExecutableAspect::RunDevice);

        workingDir.setEnvironment(&environment);

        connect(&useLibraryPaths, &BaseAspect::changed,
                &environment, &EnvironmentAspect::environmentChanged);

        if (HostOsInfo::isMacHost()) {
            connect(&useDyldSuffix, &BaseAspect::changed,
                    &environment, &EnvironmentAspect::environmentChanged);
            environment.addModifier([this](Environment &env) {
                if (useDyldSuffix())
                    env.set(QLatin1String("DYLD_IMAGE_SUFFIX"), QLatin1String("_debug"));
            });
        } else {
            useDyldSuffix.setVisible(false);
        }

        environment.addModifier([this](Environment &env) {
            BuildTargetInfo bti = buildTargetInfo();
            if (bti.runEnvModifier)
                bti.runEnvModifier(env, useLibraryPaths());
        });

        setUpdater([this] {
            if (!activeBuildSystem())
                return;

            BuildTargetInfo bti = buildTargetInfo();
            terminal.setUseTerminalHint(bti.usesTerminal);
            executable.setExecutable(bti.targetFilePath);
            workingDir.setDefaultWorkingDirectory(bti.workingDirectory);
            emit environment.environmentChanged();
        });

        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    }

    EnvironmentAspect environment{this};
    ExecutableAspect executable{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    TerminalAspect terminal{this};
    UseLibraryPathsAspect useLibraryPaths{this};
    UseDyldSuffixAspect useDyldSuffix{this};
};

// MesonRunConfigurationFactory

class MesonRunConfigurationFactory final : public RunConfigurationFactory
{
public:
    MesonRunConfigurationFactory()
    {
        registerRunConfiguration<MesonRunConfiguration>(Constants::MESON_RUNCONFIG_ID);
        addSupportedProjectType(Constants::Project::ID);
        addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
    }
};

void setupMesonRunConfiguration()
{
    static MesonRunConfigurationFactory theMesonRunConfigurationFactory;
}

void setupMesonRunAndDebugWorkers()
{
    using namespace Debugger;
    static ProcessRunnerFactory theMesonRunWorkerFactory({Constants::MESON_RUNCONFIG_ID});
    static SimpleDebugRunnerFactory theMesonDebugRunWorkerFactory({Constants::MESON_RUNCONFIG_ID});
}

} // MesonProjectManager::Internal
