/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include "mesonrunconfiguration.h"
#include <mesonpluginconstants.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/desktoprunconfiguration.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <QLatin1String>
namespace MesonProjectManager {
namespace Internal {
MesonRunConfiguration::MesonRunConfiguration(ProjectExplorer::Target *target, Utils::Id id)
    : ProjectExplorer::RunConfiguration{target, id}
{
    auto envAspect = addAspect<ProjectExplorer::LocalEnvironmentAspect>(target);

    addAspect<ProjectExplorer::ExecutableAspect>();
    addAspect<ProjectExplorer::ArgumentsAspect>();
    addAspect<ProjectExplorer::WorkingDirectoryAspect>();
    addAspect<ProjectExplorer::TerminalAspect>();

    auto libAspect = addAspect<ProjectExplorer::UseLibraryPathsAspect>();
    connect(libAspect,
            &ProjectExplorer::UseLibraryPathsAspect::changed,
            envAspect,
            &ProjectExplorer::EnvironmentAspect::environmentChanged);

    if (Utils::HostOsInfo::isMacHost()) {
        auto dyldAspect = addAspect<ProjectExplorer::UseDyldSuffixAspect>();
        connect(dyldAspect,
                &ProjectExplorer::UseLibraryPathsAspect::changed,
                envAspect,
                &ProjectExplorer::EnvironmentAspect::environmentChanged);
        envAspect->addModifier([dyldAspect](Utils::Environment &env) {
            if (dyldAspect->value())
                env.set(QLatin1String("DYLD_IMAGE_SUFFIX"), QLatin1String("_debug"));
        });
    }

    envAspect->addModifier([this, libAspect](Utils::Environment &env) {
        ProjectExplorer::BuildTargetInfo bti = buildTargetInfo();
        if (bti.runEnvModifier)
            bti.runEnvModifier(env, libAspect->value());
    });

    setUpdater([this] { updateTargetInformation(); });

    connect(target,
            &ProjectExplorer::Target::buildSystemUpdated,
            this,
            &ProjectExplorer::RunConfiguration::update);
}

void MesonRunConfiguration::updateTargetInformation()
{
    if (!activeBuildSystem())
        return;

    ProjectExplorer::BuildTargetInfo bti = buildTargetInfo();
    auto terminalAspect = aspect<ProjectExplorer::TerminalAspect>();
    terminalAspect->setUseTerminalHint(bti.usesTerminal);
    aspect<ProjectExplorer::ExecutableAspect>()->setExecutable(bti.targetFilePath);
    aspect<ProjectExplorer::WorkingDirectoryAspect>()->setDefaultWorkingDirectory(
        bti.workingDirectory);
    aspect<ProjectExplorer::LocalEnvironmentAspect>()->environmentChanged();
}

MesonRunConfigurationFactory::MesonRunConfigurationFactory()
{
    registerRunConfiguration<MesonRunConfiguration>("MesonProjectManager.MesonRunConfiguration");
    addSupportedProjectType(Constants::Project::ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

} // namespace Internal
} // namespace MesonProjectManager
