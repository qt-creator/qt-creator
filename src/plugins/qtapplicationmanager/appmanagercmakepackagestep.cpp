// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagercmakepackagestep.h"

#include "appmanagerconstants.h"
#include "appmanagertargetinformation.h"
#include "appmanagertr.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager::Internal {

class AppManagerCMakePackageStepFactory final : public BuildStepFactory
{
public:
    AppManagerCMakePackageStepFactory()
    {
        cloneStepCreator(CMakeProjectManager::Constants::CMAKE_BUILD_STEP_ID, Constants::CMAKE_PACKAGE_STEP_ID);
        setExtraInit([] (BuildStep *step) {
            // We update the build targets when the active run configuration changes
            const auto updaterSlot = [step] {
                const TargetInformation targetInformation(step->buildConfiguration());
                step->setBuildTargets({targetInformation.cmakeBuildTarget});
                step->setStepEnabled(!targetInformation.isBuiltin);
            };
            QObject::connect(
                step->buildConfiguration(),
                &BuildConfiguration::activeRunConfigurationChanged,
                step,
                updaterSlot);
            QObject::connect(
                step->buildConfiguration(),
                &BuildConfiguration::activeDeployConfigurationChanged,
                step,
                updaterSlot);
            QObject::connect(step->buildSystem(), &BuildSystem::parsingFinished, step, updaterSlot);
            QObject::connect(
                step->buildConfiguration(),
                &BuildConfiguration::runConfigurationsUpdated,
                step,
                updaterSlot);
            QObject::connect(step->project(), &Project::displayNameChanged, step, updaterSlot);
        });

        setDisplayName(Tr::tr("Create Application Manager package with CMake"));
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
        setSupportedProjectType(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
    }
};

void setupAppManagerCMakePackageStep()
{
    static AppManagerCMakePackageStepFactory theAppManagerCMakePackageStepFactory;
}

} // AppManager::Internal
