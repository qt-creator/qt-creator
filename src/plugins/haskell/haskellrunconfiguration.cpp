// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "haskellrunconfiguration.h"

#include "haskellconstants.h"
#include "haskelltr.h"
#include "haskellsettings.h"

#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/processinterface.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace Haskell::Internal {

class HaskellRunConfiguration final : public RunConfiguration
{
public:
    HaskellRunConfiguration(Target *target, Id id)
        : RunConfiguration(target, id)
    {
        environment.setSupportForBuildEnvironment(target);

        executable.setSettingsKey("Haskell.Executable");
        executable.setLabelText(Tr::tr("Executable"));

        workingDir.setEnvironment(&environment);
        workingDir.setDefaultWorkingDirectory(project()->projectDirectory());
        workingDir.setVisible(false);

        setUpdater([this] { executable.setValue(buildTargetInfo().buildKey); });

        connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
        update();
    }

private:
    ProcessRunData runnable() const final
    {
        const FilePath projectDirectory = project()->projectDirectory();
        ProcessRunData r;
        QStringList args;
        if (BuildConfiguration *buildConfiguration = target()->activeBuildConfiguration()) {
            args << "--work-dir"
                 << QDir(projectDirectory.toString()).relativeFilePath(
                        buildConfiguration->buildDirectory().toString());
        }
        args << "exec" << executable();
        if (!arguments.arguments().isEmpty())
            args << "--" << arguments.arguments();

        r.workingDirectory = projectDirectory;
        r.environment = environment.environment();
        r.command = {r.environment.searchInPath(settings().stackPath().path()), args};
        return r;
    }

    EnvironmentAspect environment{this};
    StringAspect executable{this};
    ArgumentsAspect arguments{this};
    WorkingDirectoryAspect workingDir{this};
    TerminalAspect terminal{this};
};

// Factory

class HaskellRunConfigurationFactory final : public ProjectExplorer::RunConfigurationFactory
{
public:
    HaskellRunConfigurationFactory()
    {
        registerRunConfiguration<HaskellRunConfiguration>(Constants::C_HASKELL_RUNCONFIG_ID);
        addSupportedProjectType(Constants::C_HASKELL_PROJECT_ID);
        addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
    }
};

void setupHaskellRunSupport()
{
    static HaskellRunConfigurationFactory runConfigFactory;
    static ProcessRunnerFactory runWorkerFactory{{Constants::C_HASKELL_RUNCONFIG_ID}};
    static SimpleDebugRunnerFactory debugWorkerFactory{{Constants::C_HASKELL_RUNCONFIG_ID}};
}

} // Haskell::Internal
