// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorerconstants.h"

#include <coreplugin/coreconstants.h>
#include <utils/hostosinfo.h>

#include <QUuid>

namespace ProjectExplorer {
namespace Internal {

enum class TerminalMode { On, Off, Smart };
enum class AppOutputPaneMode { FlashOnOutput, PopupOnOutput, PopupOnFirstOutput };
enum class BuildBeforeRunMode { Off, WholeProject, AppOnly };
enum class StopBeforeBuild { None, SameProject, All, SameBuildDir, SameApp };

class ProjectExplorerSettings
{
public:
    friend bool operator==(const ProjectExplorerSettings &p1, const ProjectExplorerSettings &p2)
    {
        return p1.buildBeforeDeploy == p2.buildBeforeDeploy
                && p1.deployBeforeRun == p2.deployBeforeRun
                && p1.saveBeforeBuild == p2.saveBeforeBuild
                && p1.useJom == p2.useJom
                && p1.autorestoreLastSession == p2.autorestoreLastSession
                && p1.prompToStopRunControl == p2.prompToStopRunControl
                && p1.automaticallyCreateRunConfigurations == p2.automaticallyCreateRunConfigurations
                && p1.addLibraryPathsToRunEnv == p2.addLibraryPathsToRunEnv
                && p1.environmentId == p2.environmentId
                && p1.stopBeforeBuild == p2.stopBeforeBuild
                && p1.terminalMode == p2.terminalMode
                && p1.closeSourceFilesWithProject == p2.closeSourceFilesWithProject
                && p1.clearIssuesOnRebuild == p2.clearIssuesOnRebuild
                && p1.abortBuildAllOnError == p2.abortBuildAllOnError
                && p1.lowBuildPriority == p2.lowBuildPriority;
    }

    BuildBeforeRunMode buildBeforeDeploy = BuildBeforeRunMode::WholeProject;
    bool deployBeforeRun = true;
    bool saveBeforeBuild = false;
    bool useJom = true;
    bool autorestoreLastSession = false; // This option is set in the Session Manager!
    bool prompToStopRunControl = false;
    bool automaticallyCreateRunConfigurations = true;
    bool addLibraryPathsToRunEnv = true;
    bool closeSourceFilesWithProject = true;
    bool clearIssuesOnRebuild = true;
    bool abortBuildAllOnError = true;
    bool lowBuildPriority = false;
    StopBeforeBuild stopBeforeBuild = Utils::HostOsInfo::isWindowsHost()
                                          ? StopBeforeBuild::SameProject
                                          : StopBeforeBuild::None;
    TerminalMode terminalMode = TerminalMode::Off;

    // Add a UUid which is used to identify the development environment.
    // This is used to warn the user when he is trying to open a .user file that was created
    // somewhere else (which might lead to unexpected results).
    QUuid environmentId;
};

class AppOutputSettings
{
public:
    AppOutputPaneMode runOutputMode = AppOutputPaneMode::PopupOnFirstOutput;
    AppOutputPaneMode debugOutputMode = AppOutputPaneMode::FlashOnOutput;
    bool cleanOldOutput = false;
    bool mergeChannels = false;
    bool wrapOutput = false;
    int maxCharCount = Core::Constants::DEFAULT_MAX_CHAR_COUNT;
};

class CompileOutputSettings
{
public:
    bool popUp = false;
    bool wrapOutput = false;
    int maxCharCount = Core::Constants::DEFAULT_MAX_CHAR_COUNT;
};

} // namespace ProjectExplorer
} // namespace Internal
