// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonrunconfiguration.h"

#include "mesonpluginconstants.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/desktoprunconfiguration.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>

using namespace ProjectExplorer;

namespace MesonProjectManager {
namespace Internal {

MesonRunConfiguration::MesonRunConfiguration(Target *target, Utils::Id id)
    : RunConfiguration{target, id}
{
    auto envAspect = addAspect<LocalEnvironmentAspect>(target);

    addAspect<ExecutableAspect>(target, ExecutableAspect::RunDevice);
    addAspect<ArgumentsAspect>(macroExpander());
    addAspect<WorkingDirectoryAspect>(macroExpander(), envAspect);
    addAspect<TerminalAspect>();

    auto libAspect = addAspect<UseLibraryPathsAspect>();
    connect(libAspect, &UseLibraryPathsAspect::changed,
            envAspect, &EnvironmentAspect::environmentChanged);

    if (Utils::HostOsInfo::isMacHost()) {
        auto dyldAspect = addAspect<UseDyldSuffixAspect>();
        connect(dyldAspect, &UseLibraryPathsAspect::changed,
                envAspect, &EnvironmentAspect::environmentChanged);
        envAspect->addModifier([dyldAspect](Utils::Environment &env) {
            if (dyldAspect->value())
                env.set(QLatin1String("DYLD_IMAGE_SUFFIX"), QLatin1String("_debug"));
        });
    }

    envAspect->addModifier([this, libAspect](Utils::Environment &env) {
        BuildTargetInfo bti = buildTargetInfo();
        if (bti.runEnvModifier)
            bti.runEnvModifier(env, libAspect->value());
    });

    setUpdater([this] { updateTargetInformation(); });

    connect(target, &Target::buildSystemUpdated, this, &RunConfiguration::update);
}

void MesonRunConfiguration::updateTargetInformation()
{
    if (!activeBuildSystem())
        return;

    BuildTargetInfo bti = buildTargetInfo();
    aspect<TerminalAspect>()->setUseTerminalHint(bti.usesTerminal);
    aspect<ExecutableAspect>()->setExecutable(bti.targetFilePath);
    aspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(bti.workingDirectory);
    emit aspect<LocalEnvironmentAspect>()->environmentChanged();
}

MesonRunConfigurationFactory::MesonRunConfigurationFactory()
{
    registerRunConfiguration<MesonRunConfiguration>("MesonProjectManager.MesonRunConfiguration");
    addSupportedProjectType(Constants::Project::ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

} // namespace Internal
} // namespace MesonProjectManager
