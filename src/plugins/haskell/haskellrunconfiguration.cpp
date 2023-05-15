// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "haskellrunconfiguration.h"

#include "haskellconstants.h"
#include "haskellproject.h"
#include "haskelltr.h"
#include "haskellsettings.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace Haskell {
namespace Internal {

HaskellRunConfigurationFactory::HaskellRunConfigurationFactory()
{
    registerRunConfiguration<HaskellRunConfiguration>(Constants::C_HASKELL_RUNCONFIG_ID);
    addSupportedProjectType(Constants::C_HASKELL_PROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

HaskellExecutableAspect::HaskellExecutableAspect()
{
    setSettingsKey("Haskell.Executable");
    setLabelText(Tr::tr("Executable"));
}

HaskellRunConfiguration::HaskellRunConfiguration(Target *target, Utils::Id id)
    : RunConfiguration(target, id)
{
    auto envAspect = addAspect<LocalEnvironmentAspect>(target);

    addAspect<HaskellExecutableAspect>();
    addAspect<ArgumentsAspect>(macroExpander());

    auto workingDirAspect = addAspect<WorkingDirectoryAspect>(macroExpander(), envAspect);
    workingDirAspect->setDefaultWorkingDirectory(target->project()->projectDirectory());
    workingDirAspect->setVisible(false);

    addAspect<TerminalAspect>();

    setUpdater([this] { aspect<HaskellExecutableAspect>()->setValue(buildTargetInfo().buildKey); });
    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
    update();
}

Runnable HaskellRunConfiguration::runnable() const
{
    const Utils::FilePath projectDirectory = target()->project()->projectDirectory();
    Runnable r;
    QStringList args;
    if (BuildConfiguration *buildConfiguration = target()->activeBuildConfiguration()) {
        args << "--work-dir"
             << QDir(projectDirectory.toString()).relativeFilePath(
                    buildConfiguration->buildDirectory().toString());
    }
    args << "exec" << aspect<HaskellExecutableAspect>()->value();
    const QString arguments = aspect<ArgumentsAspect>()->arguments();
    if (!arguments.isEmpty())
        args << "--" << arguments;

    r.workingDirectory = projectDirectory;
    r.environment = aspect<LocalEnvironmentAspect>()->environment();
    r.command = {r.environment.searchInPath(settings().stackPath()), args};
    return r;
}

} // namespace Internal
} // namespace Haskell
