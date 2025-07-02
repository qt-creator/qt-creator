// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <coreplugin/coreconstants.h>

#include <utils/aspects.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <QUuid>

namespace ProjectExplorer {

enum class TerminalMode { On, Off, Smart };
enum class BuildBeforeRunMode { Off, WholeProject, AppOnly };
enum class StopBeforeBuild { None, SameProject, All, SameBuildDir, SameApp };

class ProjectExplorerSettings : public Utils::AspectContainer
{
public:
    ProjectExplorerSettings();

    Utils::TypedSelectionAspect<BuildBeforeRunMode> buildBeforeDeploy{this};
    Utils::IntegerAspect reaperTimeoutInSeconds{this};
    Utils::BoolAspect deployBeforeRun{this};
    Utils::BoolAspect saveBeforeBuild{this};
    Utils::BoolAspect useJom{this};
    Utils::BoolAspect promptToStopRunControl{this};
    Utils::BoolAspect automaticallyCreateRunConfigurations{this};
    Utils::BoolAspect addLibraryPathsToRunEnv{this};
    Utils::BoolAspect closeSourceFilesWithProject{this};
    Utils::BoolAspect clearIssuesOnRebuild{this};
    Utils::BoolAspect abortBuildAllOnError{this};
    Utils::BoolAspect lowBuildPriority{this};
    Utils::BoolAspect warnAgainstNonAsciiBuildDir{this};
    Utils::BoolAspect showAllKits{this};
    Utils::TypedSelectionAspect<StopBeforeBuild> stopBeforeBuild{this};
    Utils::TypedSelectionAspect<TerminalMode> terminalMode{this};
    Utils::TypedAspect<Utils::EnvironmentItems> appEnvChanges{this};

    // Add a UUid which is used to identify the development environment.
    // This is used to warn the user when he is trying to open a .user file that was created
    // somewhere else (which might lead to unexpected results).
    Utils::TypedAspect<QUuid> environmentId;
};

PROJECTEXPLORER_EXPORT ProjectExplorerSettings &projectExplorerSettings();

namespace Internal {

void setPromptToStopSettings(bool promptToStop); // FIXME: Remove.
void setSaveBeforeBuildSettings(bool saveBeforeBuild); // FIXME: Remove.

void setupProjectExplorerSettings();

} // namespace Internal
} // namespace ProjectExplorer
