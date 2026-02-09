// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"
#include "project.h"
#include "runconfiguration.h"

#include <utils/aspects.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QUuid>

namespace ProjectExplorer {

enum class TerminalMode { On, Off, Smart };
enum class BuildBeforeRunMode { Off, WholeProject, AppOnly };
enum class StopBeforeBuild { None, SameProject, All, SameBuildDir, SameApp };
enum class SyncRunConfigs { Off, SameKit, All };
enum class KitFilter { ShowAll, ShowOnlyMatching, ShowOnlyActive /* Implies Matching */};

class PROJECTEXPLORER_EXPORT ProjectExplorerSettings : public Utils::AspectContainer
{
public:
    ProjectExplorerSettings(bool global);

    static ProjectExplorerSettings &get(QObject *context);
    static ProjectExplorerSettings &get(const QObject *context);

    template<typename Aspect> static void registerCallback(
        QObject *context,
        Aspect ProjectExplorerSettings::*aspect,
        const Utils::BaseAspect::Callback &callback);
    template<typename Aspect> static void registerCallback(
        const QObject *context,
        Aspect ProjectExplorerSettings::*aspect,
        const Utils::BaseAspect::Callback &callback);

    Utils::BoolAspect useCurrentDirectory{this};
    Utils::BoolAspect useProjectDirectory{this};
    Utils::FilePathAspect projectsDirectory{this};

    Utils::TypedSelectionAspect<BuildBeforeRunMode> buildBeforeDeploy{this};
    Utils::IntegerAspect reaperTimeoutInSeconds{this};
    Utils::BoolAspect deployBeforeRun{this};
    Utils::BoolAspect saveBeforeBuild{this};
    Utils::BoolAspect useJom{this};
    Utils::BoolAspect promptToStopRunControl{this};
    Utils::BoolAspect automaticallyCreateRunConfigurations{this};
    Utils::TypedSelectionAspect<SyncRunConfigs> syncRunConfigurations{this};
    Utils::BoolAspect addLibraryPathsToRunEnv{this};
    Utils::BoolAspect closeSourceFilesWithProject{this};
    Utils::BoolAspect clearIssuesOnRebuild{this};
    Utils::BoolAspect abortBuildAllOnError{this};
    Utils::BoolAspect lowBuildPriority{this};
    Utils::BoolAspect warnAgainstNonAsciiBuildDir{this};
    Utils::TypedSelectionAspect<KitFilter> kitFilter{this};
    Utils::TypedSelectionAspect<StopBeforeBuild> stopBeforeBuild{this};
    Utils::TypedSelectionAspect<TerminalMode> terminalMode{this};
    Utils::TypedAspect<Utils::EnvironmentItems> appEnvChanges{this};

    // Add a UUid which is used to identify the development environment.
    // This is used to warn the user when he is trying to open a .user file that was created
    // somewhere else (which might lead to unexpected results).
    Utils::TypedAspect<QByteArray> environmentId{this};

private:
    static Project *projectForContext(QObject *context);
};

PROJECTEXPLORER_EXPORT ProjectExplorerSettings &globalProjectExplorerSettings();

class PROJECTEXPLORER_EXPORT PerProjectProjectExplorerSettings : public GlobalOrProjectAspect
{
public:
    PerProjectProjectExplorerSettings(Project *project);

    ProjectExplorerSettings &custom();
    ProjectExplorerSettings &current();

    void restore();

private:
    Project * const m_project;
};

template<typename Aspect>
inline void ProjectExplorerSettings::registerCallback(
    QObject *context, Aspect ProjectExplorerSettings::*aspect, const Callback &callback)
{
    Project * const project = projectForContext(context);

    // We are not in a project context, so just forward changes from the global
    // instance directly to the provided callback.
    if (!project) {
        (globalProjectExplorerSettings().*aspect).addOnChanged(context, callback);
        return;
    }

    // We are in a project context. Here we connect to both instances, but
    // forward only when appropriate according to the current setting.
    // (We could also connect or disconnect accordingly, but that seems a lot
    // of hassle for little gain.)
    const auto handler =
        [c = QPointer(context), p = QPointer(project), callback](bool runForGlobalChange) {
            if (!c)
                return;
            QTC_ASSERT(p, return);
            if (p->projectExplorerSettings().isUsingGlobalSettings() == runForGlobalChange)
                callback();
        };
    (globalProjectExplorerSettings().*aspect).addOnChanged(context, [handler] { handler(true); });
    (project->projectExplorerSettings().custom().*aspect).addOnChanged(context, [handler] {
        handler(false);
    });

    // If the "global or project" property is toggled, we always run the callback.
    connect(&project->projectExplorerSettings(), &GlobalOrProjectAspect::currentSettingsChanged,
            context, callback);

    // The same goes for the "reset to global" action.
    connect(&project->projectExplorerSettings(), &GlobalOrProjectAspect::wasResetToGlobalValues,
            context, callback);
}

template<typename Aspect>
inline void ProjectExplorerSettings::registerCallback(
    const QObject *context, Aspect ProjectExplorerSettings::*aspect, const Callback &callback)
{
    return registerCallback(const_cast<QObject *>(context), aspect, callback);
}

namespace Internal {

void setPromptToStopSettings(bool promptToStop); // FIXME: Remove.
void setSaveBeforeBuildSettings(bool saveBeforeBuild); // FIXME: Remove.

void setupProjectExplorerSettings();
void setupProjectExplorerSettingsProjectPanel();

} // namespace Internal
} // namespace ProjectExplorer
