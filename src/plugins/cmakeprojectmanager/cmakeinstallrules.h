// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/deploymentdata.h>

#include <QFuture>

#include <vector>

namespace Utils { class FilePath; }

// fileapiparser.h takes CMakeFileInfo from fileapidataextractor.h, which is where this
// header is used: including it would be a circle.
namespace CMakeProjectManager::Internal::FileApiDetails {
class DirectoryDetails;
class TargetDetails;
} // namespace CMakeProjectManager::Internal::FileApiDetails

namespace CMakeProjectManager::Internal {

// What an install would put on the target, and how well the install rules describe it.
// Knowledge stays Bad, and data empty, wherever a rule names files that cannot be looked
// up: a deployment missing some of them is worse than none at all, because the deploy
// step that installs the project to get a complete one is added only while knowledge is
// Bad.
class InstallRuleDeployment
{
public:
    ProjectExplorer::DeploymentData data;
    ProjectExplorer::DeploymentKnowledge knowledge = ProjectExplorer::DeploymentKnowledge::Bad;
};

// The install() rules of the project as CMake resolved them for the configuration that
// was read: a destination holds no generator expression any more and the rules of the
// subdirectories are all in here. A destination that is not absolute is taken against
// installPrefix, and the drive letter drops off the front of the result, which is what an
// install with DESTDIR set does to a path of the host.
//
// An install(PROGRAMS) is a plain install(FILES) in the rules, so its file does not become
// executable on the target. Empty for a CMake before 3.21, which describes no install rule
// at all.
InstallRuleDeployment deploymentFromInstallRules(
    const QFuture<void> &cancelFuture,
    const std::vector<FileApiDetails::DirectoryDetails> &directories,
    const std::vector<FileApiDetails::TargetDetails> &targets,
    const Utils::FilePath &sourceDirectory,
    const Utils::FilePath &buildDirectory,
    const QString &installPrefix);

} // CMakeProjectManager::Internal
