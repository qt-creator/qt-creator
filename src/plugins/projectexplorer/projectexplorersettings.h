// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <coreplugin/coreconstants.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QUuid>

namespace ProjectExplorer {

enum class TerminalMode { On, Off, Smart };
enum class BuildBeforeRunMode { Off, WholeProject, AppOnly };
enum class StopBeforeBuild { None, SameProject, All, SameBuildDir, SameApp };

class ProjectExplorerSettings
{
public:
    BuildBeforeRunMode buildBeforeDeploy = BuildBeforeRunMode::WholeProject;
    int reaperTimeoutInSeconds = 1;
    bool deployBeforeRun = true;
    bool saveBeforeBuild = false;
    bool useJom = true;
    bool prompToStopRunControl = false;
    bool automaticallyCreateRunConfigurations = true;
    bool addLibraryPathsToRunEnv = true;
    bool closeSourceFilesWithProject = true;
    bool clearIssuesOnRebuild = true;
    bool abortBuildAllOnError = true;
    bool lowBuildPriority = false;
    bool warnAgainstNonAsciiBuildDir = true;
    bool showAllKits = true;
    StopBeforeBuild stopBeforeBuild = Utils::HostOsInfo::isWindowsHost()
                                          ? StopBeforeBuild::SameProject
                                          : StopBeforeBuild::None;
    TerminalMode terminalMode = TerminalMode::Off;
    Utils::EnvironmentItems appEnvChanges;

    // Add a UUid which is used to identify the development environment.
    // This is used to warn the user when he is trying to open a .user file that was created
    // somewhere else (which might lead to unexpected results).
    QUuid environmentId;
};

PROJECTEXPLORER_EXPORT const ProjectExplorerSettings &projectExplorerSettings();
ProjectExplorerSettings &mutableProjectExplorerSettings();

namespace Internal {

void setPromptToStopSettings(bool promptToStop); // FIXME: Remove.
void setSaveBeforeBuildSettings(bool saveBeforeBuild); // FIXME: Remove.

void setupProjectExplorerSettings();

} // namespace Internal
} // namespace ProjectExplorer
