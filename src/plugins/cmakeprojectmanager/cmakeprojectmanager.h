// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace ProjectExplorer {
enum class BuildAction;
class BuildSystem;
class ProjectNode;
}

namespace CMakeProjectManager::Internal {

void setupCMakeManager();
void setupOnlineHelpManager();

void runCMake(ProjectExplorer::BuildSystem *buildSystem);
void runCMakeWithProfiling(ProjectExplorer::BuildSystem *buildSystem);

void runSubprojectOperation(
    ProjectExplorer::BuildSystem *bs,
    const ProjectExplorer::ProjectNode *node,
    ProjectExplorer::BuildAction action);

} // CMakeProjectManager::Internal
