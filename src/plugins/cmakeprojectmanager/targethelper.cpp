// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "targethelper.h"

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <utils/algorithm.h>

#include "cmakebuildstep.h"
#include "cmakebuildsystem.h"
#include "cmakeproject.h"

namespace CMakeProjectManager {

void buildTarget(const Utils::FilePath &projectPath, const QString &targetName)
{
    // Get the project containing the target selected
    const auto cmakeProject = qobject_cast<CMakeProject *>(Utils::findOrDefault(
        ProjectExplorer::ProjectManager::projects(), [projectPath](ProjectExplorer::Project *p) {
            return p->projectFilePath() == projectPath;
        }));
    if (!cmakeProject || !cmakeProject->activeTarget()
        || !cmakeProject->activeTarget()->activeBuildConfiguration())
        return;

    if (ProjectExplorer::BuildManager::isBuilding(cmakeProject))
        ProjectExplorer::BuildManager::cancel();

    // Find the make step
    const ProjectExplorer::BuildStepList *buildStepList
        = cmakeProject->activeTarget()->activeBuildConfiguration()->buildSteps();
    const auto buildStep = buildStepList->firstOfType<Internal::CMakeBuildStep>();
    if (!buildStep)
        return;

    // Change the make step to build only the given target
    const QStringList oldTargets = buildStep->buildTargets();
    buildStep->setBuildTargets({targetName});

    // Build
    ProjectExplorer::BuildManager::buildProjectWithDependencies(cmakeProject);
    buildStep->setBuildTargets(oldTargets);
}

} // namespace CMakeProjectManager
