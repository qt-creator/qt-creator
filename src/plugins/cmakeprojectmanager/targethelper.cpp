// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "targethelper.h"

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildsteplist.h>

#include "cmakebuildstep.h"

namespace CMakeProjectManager::Internal {

void buildTarget(const ProjectExplorer::BuildSystem *buildSystem, const QString &targetName)
{
    if (ProjectExplorer::BuildManager::isBuilding(buildSystem->project()))
        ProjectExplorer::BuildManager::cancel();

    // Find the make step
    const ProjectExplorer::BuildStepList *buildStepList
        = buildSystem->buildConfiguration()->buildSteps();
    const auto buildStep = buildStepList->firstOfType<Internal::CMakeBuildStep>();
    if (!buildStep)
        return;

    // Change the make step to build only the given target
    const QStringList oldTargets = buildStep->buildTargets();
    buildStep->setBuildTargets({targetName});

    // Build
    ProjectExplorer::BuildManager::buildProjectWithDependencies(buildSystem->project());
    buildStep->setBuildTargets(oldTargets);
}

} // namespace CMakeProjectManager::Internal
