// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildstep.h"
#include "projectexplorer_export.h"

#include <QObject>
#include <QStringList>

namespace ProjectExplorer {

class BuildItem;
class Project;
class RunConfiguration;
class RunControl;
class Task;

enum class BuildForRunConfigStatus { Building, NotBuilding, BuildFailed };
enum class ConfigSelection { All, Active };

class PROJECTEXPLORER_EXPORT BuildManager : public QObject
{
    Q_OBJECT

public:
    explicit BuildManager(QObject *parent, QAction *cancelBuildAction);
    ~BuildManager() override;
    static BuildManager *instance();

    static void extensionsInitialized();

    static int buildProjectWithoutDependencies(Project *project);
    static int cleanProjectWithoutDependencies(Project *project);
    static int rebuildProjectWithoutDependencies(Project *project);
    static int buildProjectWithDependencies(
        Project *project,
        ConfigSelection configSelection = ConfigSelection::Active,
        RunControl *starter = nullptr);
    static int cleanProjectWithDependencies(Project *project, ConfigSelection configSelection);
    static int rebuildProjectWithDependencies(Project *project, ConfigSelection configSelection);
    static int buildProjects(const QList<Project *> &projects, ConfigSelection configSelection);
    static int cleanProjects(const QList<Project *> &projects, ConfigSelection configSelection);
    static int rebuildProjects(const QList<Project *> &projects, ConfigSelection configSelection);

    static int deployProjects(const QList<Project *> &projects);

    static BuildForRunConfigStatus potentiallyBuildForRunConfig(RunConfiguration *rc);

    static bool isBuilding();
    static bool isDeploying();
    static bool tasksAvailable();

    static bool buildLists(const QList<BuildStepList *> &bsls,
                           const QStringList &preambelMessage = {});
    static bool buildList(BuildStepList *bsl);

    static bool isBuilding(const Project *p);
    static bool isBuilding(const Target *t);
    static bool isBuilding(const ProjectConfiguration *p);
    static bool isBuilding(BuildStep *step);

    // Append any build step to the list of build steps (currently only used to add the QMakeStep)
    static void appendStep(BuildStep *step, const QString &name);

    static int getErrorTaskCount();

    static QString displayNameForStepId(Utils::Id stepId);

    static std::optional<QPair<int, QString>> currentProgress();

public slots:
    static void cancel();
    // Shows without focus
    static void showTaskWindow();
    static void toggleTaskWindow();
    static void toggleOutputWindow();
    static void aboutToRemoveProject(ProjectExplorer::Project *p);

signals:
    void buildStateChanged(ProjectExplorer::Project *pro);
    void buildQueueFinished(bool success);

private:
    static void cleanupBuild();
    static void addToTaskWindow(const ProjectExplorer::Task &task, int linkedOutputLines, int skipLines);
    static void addToOutputWindow(const QString &string, BuildStep::OutputFormat format,
                           BuildStep::OutputNewlineSetting newlineSettings = BuildStep::DoAppendNewline);

    static void progressChanged(int percent, const QString &text);
    static void showBuildResults();
    static void updateTaskCount();
    static void finish();

    static void startBuildQueue();
    static bool buildQueueAppend(const QList<BuildItem> &items,
                                 const QStringList &preambleMessage = {});
    static void incrementActiveBuildSteps(BuildStep *bs);
    static void decrementActiveBuildSteps(BuildStep *bs);
};

} // namespace ProjectExplorer
