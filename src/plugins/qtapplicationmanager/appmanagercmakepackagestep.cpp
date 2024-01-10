// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagercmakepackagestep.h"

#include "appmanagerconstants.h"
#include "appmanagertargetinformation.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h>

using namespace ProjectExplorer;
using namespace CMakeProjectManager;
using namespace Utils;

namespace AppManager {
namespace Internal {

AppManagerCMakePackageStepFactory::AppManagerCMakePackageStepFactory()
{
    cloneStepCreator(CMakeProjectManager::Constants::CMAKE_BUILD_STEP_ID, Constants::CMAKE_PACKAGE_STEP_ID);
    setExtraInit([] (BuildStep *step) {
        // We update the build targets when the active run configuration changes
        const auto updaterSlot = [step] {
            const auto targetInformation = TargetInformation(step->target());
            step->setBuildTargets({targetInformation.cmakeBuildTarget});
            step->setEnabled(!targetInformation.isBuiltin);
        };
        QObject::connect(step->target(), &Target::activeRunConfigurationChanged, step, updaterSlot);
        QObject::connect(step->target(), &Target::activeDeployConfigurationChanged, step, updaterSlot);
        QObject::connect(step->target(), &Target::parsingFinished, step, updaterSlot);
        QObject::connect(step->target(), &Target::runConfigurationsUpdated, step, updaterSlot);
        QObject::connect(step->project(), &Project::displayNameChanged, step, updaterSlot);
    });

    setDisplayName(tr("Create Appman package with CMake"));
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
}

} // namespace Internal
} // namespace AppManager
