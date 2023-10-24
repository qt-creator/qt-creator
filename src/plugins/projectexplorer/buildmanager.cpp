// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildmanager.h"

#include "buildprogress.h"
#include "buildsteplist.h"
#include "buildsystem.h"
#include "compileoutputwindow.h"
#include "deployconfiguration.h"
#include "devicesupport/devicemanager.h"
#include "kit.h"
#include "kitaspects.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "runcontrol.h"
#include "target.h"
#include "task.h"
#include "taskhub.h"
#include "taskwindow.h"
#include "waitforstopdialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <solutions/tasking/tasktree.h>

#include <utils/algorithm.h>
#include <utils/outputformatter.h>
#include <utils/stringutils.h>

#include <QApplication>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QHash>
#include <QList>
#include <QMessageBox>
#include <QPointer>
#include <QSet>
#include <QTime>
#include <QTimer>

using namespace Core;
using namespace Tasking;
using namespace Utils;

namespace ProjectExplorer {
using namespace Internal;

class ParserAwaiterTaskAdapter : public TaskAdapter<QSet<BuildSystem *>>
{
private:
    void start() final { checkParsing(); }
    void checkParsing() {
        const QSet<BuildSystem *> buildSystems = *task();
        for (BuildSystem *buildSystem : buildSystems) {
            if (!buildSystem || !buildSystem->isParsing())
                continue;
            connect(buildSystem, &BuildSystem::parsingFinished,
                    this, [this, buildSystem](bool success) {
                disconnect(buildSystem, &BuildSystem::parsingFinished, this, nullptr);
                if (!success) {
                    emit done(false);
                    return;
                }
                checkParsing();
            });
            return;
        }
        emit done(true);
    }
};

using ParserAwaiterTask = CustomTask<ParserAwaiterTaskAdapter>;

static QString msgProgress(int progress, int total)
{
    return Tr::tr("Finished %1 of %n steps", nullptr, total).arg(progress);
}

static const QList<Target *> targetsForSelection(const Project *project,
                                                 ConfigSelection targetSelection)
{
    if (targetSelection == ConfigSelection::All)
        return project->targets();
    if (project->activeTarget())
        return {project->activeTarget()};
    return {};
}

static const QList<BuildConfiguration *> buildConfigsForSelection(const Target *target,
                                                                  ConfigSelection configSelection)
{
    if (configSelection == ConfigSelection::All)
        return target->buildConfigurations();
    else if (target->activeBuildConfiguration())
        return {target->activeBuildConfiguration()};
    return {};
}

static int queue(const QList<Project *> &projects, const QList<Id> &stepIds,
                 ConfigSelection configSelection, const RunConfiguration *forRunConfig = nullptr,
                 RunControl *starter = nullptr)
{
    if (!ProjectExplorerPlugin::saveModifiedFiles())
        return -1;

    const ProjectExplorerSettings &settings = ProjectExplorerPlugin::projectExplorerSettings();
    if (settings.stopBeforeBuild != StopBeforeBuild::None
            && stepIds.contains(Constants::BUILDSTEPS_BUILD)) {
        StopBeforeBuild stopCondition = settings.stopBeforeBuild;
        if (stopCondition == StopBeforeBuild::SameApp && !forRunConfig)
            stopCondition = StopBeforeBuild::SameBuildDir;
        const auto isStoppableRc = [&projects, stopCondition, configSelection, forRunConfig,
                                    starter](RunControl *rc) {
            if (rc == starter)
                return false;
            if (!rc->isRunning())
                return false;

            switch (stopCondition) {
            case StopBeforeBuild::None:
                return false;
            case StopBeforeBuild::All:
                return true;
            case StopBeforeBuild::SameProject:
                return projects.contains(rc->project());
            case StopBeforeBuild::SameBuildDir:
                return Utils::contains(projects, [rc, configSelection](Project *p) {
                    const FilePath executable = rc->commandLine().executable();
                    IDevice::ConstPtr device = DeviceManager::deviceForPath(executable);
                    for (const Target * const t : targetsForSelection(p, configSelection)) {
                        if (device.isNull())
                            device = DeviceKitAspect::device(t->kit());
                        if (device.isNull() || device->type() != Constants::DESKTOP_DEVICE_TYPE)
                            continue;
                        for (const BuildConfiguration * const bc
                             : buildConfigsForSelection(t, configSelection)) {
                            if (executable.isChildOf(bc->buildDirectory()))
                                return true;
                        }
                    }
                    return false;
                });
            case StopBeforeBuild::SameApp:
                QTC_ASSERT(forRunConfig, return false);
                return forRunConfig->buildTargetInfo().targetFilePath
                        == rc->targetFilePath();
            }
            return false; // Can't get here!
        };
        const QList<RunControl *> toStop
                = Utils::filtered(ProjectExplorerPlugin::allRunControls(), isStoppableRc);

        if (!toStop.isEmpty()) {
            bool stopThem = true;
            if (settings.prompToStopRunControl) {
                QStringList names = Utils::transform(toStop, &RunControl::displayName);
                if (QMessageBox::question(ICore::dialogParent(),
                        Tr::tr("Stop Applications"),
                        Tr::tr("Stop these applications before building?")
                        + "\n\n" + names.join('\n'))
                        == QMessageBox::No) {
                    stopThem = false;
                }
            }

            if (stopThem) {
                for (RunControl *rc : toStop)
                    rc->initiateStop();

                WaitForStopDialog dialog(toStop);
                dialog.exec();

                if (dialog.canceled())
                    return -1;
            }
        }
    }

    QList<BuildStepList *> stepLists;
    QStringList preambleMessage;

    for (const Project *pro : projects) {
        if (pro && pro->needsConfiguration()) {
            preambleMessage.append(
                        Tr::tr("The project %1 is not configured, skipping it.")
                        .arg(pro->displayName()) + QLatin1Char('\n'));
        }
    }
    for (const Project *pro : projects) {
        for (const Target *const t : targetsForSelection(pro, configSelection)) {
            for (const BuildConfiguration *bc : buildConfigsForSelection(t, configSelection)) {
                const IDevice::Ptr device = BuildDeviceKitAspect::device(bc->kit())
                                                .constCast<IDevice>();

                if (device && !device->prepareForBuild(t)) {
                    preambleMessage.append(
                        Tr::tr("The build device failed to prepare for the build of %1 (%2).")
                            .arg(pro->displayName())
                            .arg(t->displayName())
                        + QLatin1Char('\n'));
                }
            }
        }
    }

    for (const Id id : stepIds) {
        const bool isBuild = id == Constants::BUILDSTEPS_BUILD;
        const bool isClean = id == Constants::BUILDSTEPS_CLEAN;
        const bool isDeploy = id == Constants::BUILDSTEPS_DEPLOY;
        for (const Project *pro : projects) {
            if (!pro || pro->needsConfiguration())
                continue;
            BuildStepList *bsl = nullptr;
            for (const Target * target : targetsForSelection(pro, configSelection)) {
                if (isBuild || isClean) {
                    for (const BuildConfiguration * const bc
                         : buildConfigsForSelection(target, configSelection)) {
                        bsl = isBuild ? bc->buildSteps() : bc->cleanSteps();
                        if (bsl && !bsl->isEmpty())
                            stepLists << bsl;
                    }
                    continue;
                }
                if (isDeploy && target->activeDeployConfiguration())
                    bsl = target->activeDeployConfiguration()->stepList();
                if (bsl && !bsl->isEmpty())
                    stepLists << bsl;
            }
        }
    }

    if (stepLists.isEmpty())
        return 0;

    if (!BuildManager::buildLists(stepLists, preambleMessage))
        return -1;
    return stepLists.count();
}

class BuildItem
{
public:
    BuildStep *buildStep = nullptr;
    bool enabled = true;
    QString name;
};

class BuildManagerPrivate
{
public:
    Internal::CompileOutputWindow *m_outputWindow = nullptr;
    Internal::TaskWindow *m_taskWindow = nullptr;

    QList<BuildItem> m_pendingQueue;
    QList<BuildItem> m_buildQueue;
    int m_progress = 0;
    int m_maxProgress = 0;
    bool m_poppedUpTaskWindow = false;
    bool m_isDeploying = false;
    // used to decide if we are building a project to decide when to emit buildStateChanged(Project *)
    QHash<Project *, int>  m_activeBuildSteps;
    QHash<Target *, int> m_activeBuildStepsPerTarget;
    QHash<ProjectConfiguration *, int> m_activeBuildStepsPerProjectConfiguration;

    // Progress reporting to the progress manager
    QFutureInterface<void> *m_progressFutureInterface = nullptr;
    QFutureWatcher<void> m_progressWatcher;
    QPointer<FutureProgress> m_futureProgress;

    std::unique_ptr<TaskTree> m_taskTree;
    QElapsedTimer m_elapsed;
};

static BuildManagerPrivate *d = nullptr;
static BuildManager *m_instance = nullptr;

BuildManager::BuildManager(QObject *parent, QAction *cancelBuildAction)
    : QObject(parent)
{
    QTC_CHECK(!m_instance);
    m_instance = this;
    d = new BuildManagerPrivate;

    connect(ProjectManager::instance(), &ProjectManager::aboutToRemoveProject,
            this, &BuildManager::aboutToRemoveProject);

    d->m_outputWindow = new Internal::CompileOutputWindow(cancelBuildAction);
    ExtensionSystem::PluginManager::addObject(d->m_outputWindow);

    d->m_taskWindow = new Internal::TaskWindow;
    ExtensionSystem::PluginManager::addObject(d->m_taskWindow);

    qRegisterMetaType<ProjectExplorer::BuildStep::OutputFormat>();
    qRegisterMetaType<ProjectExplorer::BuildStep::OutputNewlineSetting>();

    connect(d->m_taskWindow, &Internal::TaskWindow::tasksChanged,
            this, &BuildManager::updateTaskCount);

    connect(&d->m_progressWatcher, &QFutureWatcherBase::canceled,
            this, &BuildManager::cancel);
    connect(&d->m_progressWatcher, &QFutureWatcherBase::finished,
            this, &BuildManager::finish);
}

BuildManager *BuildManager::instance()
{
    return m_instance;
}

void BuildManager::extensionsInitialized()
{
    TaskHub::addCategory({Constants::TASK_CATEGORY_COMPILE,
                          Tr::tr("Compile", "Category for compiler issues listed under 'Issues'"),
                          Tr::tr("Issues parsed from the compile output."),
                          true,
                          100});
    TaskHub::addCategory(
        {Constants::TASK_CATEGORY_BUILDSYSTEM,
         Tr::tr("Build System", "Category for build system issues listed under 'Issues'"),
         Tr::tr("Issues from the build system, such as CMake or qmake."),
         true,
         100});
    TaskHub::addCategory(
        {Constants::TASK_CATEGORY_DEPLOYMENT,
         Tr::tr("Deployment", "Category for deployment issues listed under 'Issues'"),
         Tr::tr("Issues found when deploying applications to devices."),
         true,
         100});
    TaskHub::addCategory({Constants::TASK_CATEGORY_AUTOTEST,
                          Tr::tr("Autotests", "Category for autotest issues listed under 'Issues'"),
                          Tr::tr("Issues found when running tests."),
                          true,
                          100});
}

void BuildManager::buildProjectWithoutDependencies(Project *project)
{
    queue({project}, {Id(Constants::BUILDSTEPS_BUILD)}, ConfigSelection::Active);
}

void BuildManager::cleanProjectWithoutDependencies(Project *project)
{
    queue({project}, {Id(Constants::BUILDSTEPS_CLEAN)}, ConfigSelection::Active);
}

void BuildManager::rebuildProjectWithoutDependencies(Project *project)
{
    queue({project}, {Id(Constants::BUILDSTEPS_CLEAN), Id(Constants::BUILDSTEPS_BUILD)},
          ConfigSelection::Active);
}

void BuildManager::buildProjectWithDependencies(Project *project, ConfigSelection configSelection,
                                                RunControl *starter)
{
    queue(ProjectManager::projectOrder(project), {Id(Constants::BUILDSTEPS_BUILD)},
          configSelection, nullptr, starter);
}

void BuildManager::cleanProjectWithDependencies(Project *project, ConfigSelection configSelection)
{
    queue(ProjectManager::projectOrder(project), {Id(Constants::BUILDSTEPS_CLEAN)},
          configSelection);
}

void BuildManager::rebuildProjectWithDependencies(Project *project, ConfigSelection configSelection)
{
    queue(ProjectManager::projectOrder(project),
          {Id(Constants::BUILDSTEPS_CLEAN), Id(Constants::BUILDSTEPS_BUILD)},
          configSelection);
}

void BuildManager::buildProjects(const QList<Project *> &projects, ConfigSelection configSelection)
{
    queue(projects, {Id(Constants::BUILDSTEPS_BUILD)}, configSelection);
}

void BuildManager::cleanProjects(const QList<Project *> &projects, ConfigSelection configSelection)
{
    queue(projects, {Id(Constants::BUILDSTEPS_CLEAN)}, configSelection);
}

void BuildManager::rebuildProjects(const QList<Project *> &projects,
                                   ConfigSelection configSelection)
{
    queue(projects, {Id(Constants::BUILDSTEPS_CLEAN), Id(Constants::BUILDSTEPS_BUILD)},
          configSelection);
}

void BuildManager::deployProjects(const QList<Project *> &projects)
{
    QList<Id> steps;
    if (ProjectExplorerPlugin::projectExplorerSettings().buildBeforeDeploy != BuildBeforeRunMode::Off)
        steps << Id(Constants::BUILDSTEPS_BUILD);
    steps << Id(Constants::BUILDSTEPS_DEPLOY);
    queue(projects, steps, ConfigSelection::Active);
}

BuildForRunConfigStatus BuildManager::potentiallyBuildForRunConfig(RunConfiguration *rc)
{
    QList<Id> stepIds;
    const ProjectExplorerSettings &settings = ProjectExplorerPlugin::projectExplorerSettings();
    if (settings.deployBeforeRun) {
        if (!isBuilding()) {
            switch (settings.buildBeforeDeploy) {
            case BuildBeforeRunMode::AppOnly:
                if (rc->target()->activeBuildConfiguration())
                    rc->target()->activeBuildConfiguration()->restrictNextBuild(rc);
                Q_FALLTHROUGH();
            case BuildBeforeRunMode::WholeProject:
                stepIds << Id(Constants::BUILDSTEPS_BUILD);
                break;
            case BuildBeforeRunMode::Off:
                break;
            }
        }
        if (!isDeploying())
            stepIds << Id(Constants::BUILDSTEPS_DEPLOY);
    }

    Project * const pro = rc->target()->project();
    const int queueCount = queue(ProjectManager::projectOrder(pro), stepIds,
                                 ConfigSelection::Active, rc);
    if (rc->target()->activeBuildConfiguration())
        rc->target()->activeBuildConfiguration()->restrictNextBuild(nullptr);

    if (queueCount < 0)
        return BuildForRunConfigStatus::BuildFailed;
    if (queueCount > 0 || isBuilding(rc->project()))
        return BuildForRunConfigStatus::Building;
    return BuildForRunConfigStatus::NotBuilding;
}

BuildManager::~BuildManager()
{
    cancel();
    m_instance = nullptr;
    ExtensionSystem::PluginManager::removeObject(d->m_taskWindow);
    delete d->m_taskWindow;

    ExtensionSystem::PluginManager::removeObject(d->m_outputWindow);
    delete d->m_outputWindow;

    delete d;
    d = nullptr;
}

void BuildManager::aboutToRemoveProject(Project *p)
{
    QHash<Project *, int>::iterator it = d->m_activeBuildSteps.find(p);
    QHash<Project *, int>::iterator end = d->m_activeBuildSteps.end();
    if (it != end && *it > 0) {
        // We are building the project that's about to be removed.
        // We cancel the whole queue, which isn't the nicest thing to do
        // but a safe thing.
        cancel();
    }
}

bool BuildManager::isBuilding()
{
    // we are building even if we are not running yet
    return !d->m_pendingQueue.isEmpty() || !d->m_buildQueue.isEmpty();
}

bool BuildManager::isDeploying()
{
    return d->m_isDeploying;
}

int BuildManager::getErrorTaskCount()
{
    const int errors =
            d->m_taskWindow->errorTaskCount(Constants::TASK_CATEGORY_BUILDSYSTEM)
            + d->m_taskWindow->errorTaskCount(Constants::TASK_CATEGORY_COMPILE)
            + d->m_taskWindow->errorTaskCount(Constants::TASK_CATEGORY_DEPLOYMENT);
    return errors;
}

QString BuildManager::displayNameForStepId(Id stepId)
{
    if (stepId == Constants::BUILDSTEPS_CLEAN) {
        //: Displayed name for a "cleaning" build step
        return Tr::tr("Clean");
    }
    if (stepId == Constants::BUILDSTEPS_DEPLOY) {
        //: Displayed name for a deploy step
        return Tr::tr("Deploy");
    }
    //: Displayed name for a normal build step
    return Tr::tr("Build");
}

void BuildManager::cleanupBuild()
{
    const QList<BuildItem> buildQueue = d->m_buildQueue;
    d->m_buildQueue.clear();
    for (const BuildItem &item : buildQueue) {
        decrementActiveBuildSteps(item.buildStep);
        disconnect(item.buildStep, nullptr, m_instance, nullptr);
    }
    if (d->m_progressFutureInterface) {
        d->m_progressFutureInterface->reportFinished();
        d->m_progressWatcher.setFuture(QFuture<void>());
        delete d->m_progressFutureInterface;
        d->m_progressFutureInterface = nullptr;
    }
    d->m_progress = 0;
    d->m_maxProgress = 0;
    d->m_futureProgress = nullptr;
}

void BuildManager::cancel()
{
    if (!d->m_taskTree)
        return;

    d->m_taskTree.reset();

    const QList<BuildItem> pendingQueue = d->m_pendingQueue;
    d->m_pendingQueue.clear();
    for (const BuildItem &item : pendingQueue) {
        decrementActiveBuildSteps(item.buildStep);
        disconnect(item.buildStep, nullptr, m_instance, nullptr);
    }

    d->m_poppedUpTaskWindow = false;
    d->m_isDeploying = false;

    if (d->m_progressFutureInterface) {
        d->m_progressFutureInterface->setProgressValueAndText(100 * d->m_progress,
                                                              Tr::tr("Build/Deployment canceled"));
        d->m_progressFutureInterface->reportCanceled();
    }
    cleanupBuild();

    addToOutputWindow(Tr::tr("Canceled build/deployment."), BuildStep::OutputFormat::ErrorMessage);
    emit m_instance->buildQueueFinished(false);
}

void BuildManager::updateTaskCount()
{
    const int errors = getErrorTaskCount();
    ProgressManager::setApplicationLabel(errors > 0 ? QString::number(errors) : QString());
    if (isBuilding() && errors > 0 && !d->m_poppedUpTaskWindow) {
        showTaskWindow();
        d->m_poppedUpTaskWindow = true;
    }
}

void BuildManager::finish()
{
    const QString elapsedTime = Utils::formatElapsedTime(d->m_elapsed.elapsed());
    addToOutputWindow(elapsedTime, BuildStep::OutputFormat::NormalMessage);
    d->m_outputWindow->flush();

    QApplication::alert(ICore::dialogParent(), 3000);
}

void BuildManager::toggleOutputWindow()
{
    d->m_outputWindow->toggle(IOutputPane::ModeSwitch | IOutputPane::WithFocus);
}

void BuildManager::showTaskWindow()
{
    d->m_taskWindow->popup(IOutputPane::NoModeSwitch);
}

void BuildManager::toggleTaskWindow()
{
    d->m_taskWindow->toggle(IOutputPane::ModeSwitch | IOutputPane::WithFocus);
}

bool BuildManager::tasksAvailable()
{
    const int count =
            d->m_taskWindow->taskCount(Constants::TASK_CATEGORY_BUILDSYSTEM)
            + d->m_taskWindow->taskCount(Constants::TASK_CATEGORY_COMPILE)
            + d->m_taskWindow->taskCount(Constants::TASK_CATEGORY_DEPLOYMENT);
    return count > 0;
}

void BuildManager::startBuildQueue()
{
    if (compileOutputSettings().popUp())
        d->m_outputWindow->popup(IOutputPane::NoModeSwitch);

    const auto onAwaiterSetup = [](QSet<BuildSystem *> &buildSystems) {
        // Delay if any of the involved build systems are currently parsing.
        buildSystems = transform<QSet<BuildSystem *>>(
            d->m_buildQueue, [](const BuildItem &item) { return item.buildStep->buildSystem(); });
        if (d->m_futureProgress && !d->m_buildQueue.isEmpty())
            d->m_futureProgress.data()->setTitle(d->m_buildQueue.first().name);
    };

    const GroupItem abortPolicy
        = ProjectExplorerPlugin::projectExplorerSettings().abortBuildAllOnError
              ? stopOnError : continueOnError;

    QList<GroupItem> topLevel { abortPolicy, ParserAwaiterTask(onAwaiterSetup) };
    Project *lastProject = nullptr;
    Target *lastTarget = nullptr;
    QList<GroupItem> targetTasks;
    d->m_progress = 0;
    d->m_maxProgress = 0;

    for (const BuildItem &item : std::as_const(d->m_buildQueue)) {
        BuildStep *buildStep = item.buildStep;
        Target *target = buildStep->target();
        if (lastTarget != target) {
            if (!targetTasks.isEmpty())
                topLevel.append(Group(targetTasks));
            targetTasks.clear();
            lastTarget = target;
        }

        Project *project = buildStep->project();
        if (lastProject != project) {
            targetTasks.append(Sync([projectName = buildStep->project()->displayName()] {
                addToOutputWindow(Tr::tr("Running steps for project %1...")
                                      .arg(projectName), BuildStep::OutputFormat::NormalMessage);
            }));
            lastProject = project;
        }

        if (!item.enabled) {
            targetTasks.append(Sync([name = buildStep->displayName()] {
                addToOutputWindow(Tr::tr("Skipping disabled step %1.")
                                      .arg(name), BuildStep::OutputFormat::NormalMessage);
            }));
            continue;
        }
        ++d->m_maxProgress;

        const auto onRecipeSetup = [buildStep, name = item.name] {
            d->m_outputWindow->reset();
            buildStep->setupOutputFormatter(d->m_outputWindow->outputFormatter());
            connect(buildStep, &BuildStep::progress, instance(), &BuildManager::progressChanged);
            if (d->m_futureProgress)
                d->m_futureProgress.data()->setTitle(name);
        };
        const auto onRecipeDone = [buildStep] {
            disconnect(buildStep, &BuildStep::progress, instance(), nullptr);
            d->m_outputWindow->flush();
            ++d->m_progress;
            d->m_progressFutureInterface->setProgressValueAndText(
                100 * d->m_progress, msgProgress(d->m_progress, d->m_maxProgress));
        };
        const auto onRecipeError = [buildStep, target, onRecipeDone] {
            onRecipeDone();
            const QString projectName = buildStep->project()->displayName();
            const QString targetName = target->displayName();
            addToOutputWindow(Tr::tr("Error while building/deploying project %1 (kit: %2)")
                                  .arg(projectName, targetName), BuildStep::OutputFormat::Stderr);
            const Tasks kitTasks = target->kit()->validate();
            if (!kitTasks.isEmpty()) {
                addToOutputWindow(Tr::tr("The kit %1 has configuration issues which might "
                                         "be the root cause for this problem.")
                                      .arg(targetName), BuildStep::OutputFormat::Stderr);
            }
            addToOutputWindow(Tr::tr("When executing step \"%1\"")
                                  .arg(buildStep->displayName()), BuildStep::OutputFormat::Stderr);
        };
        const Group recipeGroup {
            onGroupSetup(onRecipeSetup),
            buildStep->runRecipe(),
            onGroupDone(onRecipeDone),
            onGroupError(onRecipeError),
        };
        targetTasks.append(recipeGroup);
    }
    if (!targetTasks.isEmpty())
        topLevel.append(Group(targetTasks));

    d->m_taskTree.reset(new TaskTree(Group{topLevel}));
    const auto endHandler = [](bool success) {
        d->m_taskTree.release()->deleteLater();

        if (!success && d->m_progressFutureInterface)
            d->m_progressFutureInterface->reportCanceled();

        cleanupBuild();

        if (d->m_pendingQueue.isEmpty()) {
            d->m_poppedUpTaskWindow = false;
            d->m_isDeploying = false;
        }

        emit m_instance->buildQueueFinished(success);

        if (!d->m_pendingQueue.isEmpty()) {
            d->m_buildQueue = d->m_pendingQueue;
            d->m_pendingQueue.clear();
            startBuildQueue();
        }
    };
    connect(d->m_taskTree.get(), &TaskTree::done, instance(), [endHandler] { endHandler(true); });
    connect(d->m_taskTree.get(), &TaskTree::errorOccurred, instance(),
            [endHandler] { endHandler(false); });

    // Progress Reporting
    d->m_progressFutureInterface = new QFutureInterface<void>;
    d->m_progressWatcher.setFuture(d->m_progressFutureInterface->future());
    ProgressManager::setApplicationLabel({});
    d->m_futureProgress = ProgressManager::addTask(d->m_progressFutureInterface->future(),
        {}, "ProjectExplorer.Task.Build",
        ProgressManager::KeepOnFinish | ProgressManager::ShowInApplicationIcon);
    connect(d->m_futureProgress.data(), &FutureProgress::clicked,
            m_instance, &BuildManager::showBuildResults);
    d->m_futureProgress.data()->setWidget(new BuildProgress(d->m_taskWindow));
    d->m_futureProgress.data()->setStatusBarWidget(new BuildProgress(d->m_taskWindow,
                                                                     Qt::Horizontal));
    d->m_progressFutureInterface->setProgressRange(0, d->m_maxProgress * 100);
    d->m_progressFutureInterface->reportStarted();

    d->m_elapsed.start();
    d->m_taskTree->start();
}

void BuildManager::showBuildResults()
{
    if (tasksAvailable())
        toggleTaskWindow();
    else
        toggleOutputWindow();
    //toggleTaskWindow();
}

void BuildManager::addToTaskWindow(const Task &task, int linkedOutputLines, int skipLines)
{
    // Distribute to all others
    d->m_outputWindow->registerPositionOf(task, linkedOutputLines, skipLines);
    TaskHub::addTask(task);
}

void BuildManager::addToOutputWindow(const QString &string, BuildStep::OutputFormat format,
                                     BuildStep::OutputNewlineSetting newlineSettings)
{
    QString stringToWrite;
    if (format == BuildStep::OutputFormat::NormalMessage || format == BuildStep::OutputFormat::ErrorMessage) {
        stringToWrite = QTime::currentTime().toString();
        stringToWrite += ": ";
    }
    stringToWrite += string;
    if (newlineSettings == BuildStep::DoAppendNewline)
        stringToWrite += '\n';
    d->m_outputWindow->appendText(stringToWrite, format);
}

void BuildManager::progressChanged(int percent, const QString &text)
{
    if (d->m_progressFutureInterface)
        d->m_progressFutureInterface->setProgressValueAndText(percent + 100 * d->m_progress, text);
}

bool BuildManager::buildQueueAppend(const QList<BuildItem> &items, const QStringList &preambleMessage)
{
    if (!d->m_taskTree) {
        d->m_outputWindow->clearContents();
        if (ProjectExplorerPlugin::projectExplorerSettings().clearIssuesOnRebuild) {
            TaskHub::clearTasks(Constants::TASK_CATEGORY_COMPILE);
            TaskHub::clearTasks(Constants::TASK_CATEGORY_BUILDSYSTEM);
            TaskHub::clearTasks(Constants::TASK_CATEGORY_DEPLOYMENT);
            TaskHub::clearTasks(Constants::TASK_CATEGORY_AUTOTEST);
        }
        for (const QString &str : preambleMessage)
            addToOutputWindow(str, BuildStep::OutputFormat::NormalMessage, BuildStep::DontAppendNewline);
    }

    QList<BuildStep *> connectedSteps;
    for (const BuildItem &item : items) {
        BuildStep *buildStep = item.buildStep;
        connect(buildStep, &BuildStep::addTask, m_instance, &BuildManager::addToTaskWindow);
        connect(buildStep, &BuildStep::addOutput, m_instance, &BuildManager::addToOutputWindow);
        connectedSteps.append(buildStep);
        if (!item.enabled)
            continue;
        if (!isBuilding(buildStep) && buildStep->init())
            continue;

        // init() failed, print something for the user...
        const QString projectName = buildStep->project()->displayName();
        const QString targetName = buildStep->target()->displayName();
        addToOutputWindow(Tr::tr("Error while building/deploying project %1 (kit: %2)")
                              .arg(projectName, targetName), BuildStep::OutputFormat::Stderr);
        addToOutputWindow(Tr::tr("When executing step \"%1\"")
                              .arg(buildStep->displayName()), BuildStep::OutputFormat::Stderr);
        for (BuildStep *buildStep : std::as_const(connectedSteps))
            disconnect(buildStep, nullptr, m_instance, nullptr);
        d->m_outputWindow->popup(IOutputPane::NoModeSwitch);
        return false;
    }

    if (d->m_taskTree)
        d->m_pendingQueue << items;
    else
        d->m_buildQueue = items;

    if (d->m_buildQueue.isEmpty() && d->m_pendingQueue.isEmpty()) {
        if (compileOutputSettings().popUp())
            d->m_outputWindow->popup(IOutputPane::NoModeSwitch);
        emit m_instance->buildQueueFinished(true);
        return true;
    }

    for (const BuildItem &item : items)
        incrementActiveBuildSteps(item.buildStep);

    if (!d->m_taskTree)
        startBuildQueue();
    return true;
}

bool BuildManager::buildList(BuildStepList *bsl)
{
    return buildLists({bsl});
}

bool BuildManager::buildLists(const QList<BuildStepList *> &bsls, const QStringList &preambleMessage)
{
    const bool wasDeploying = d->m_isDeploying;
    QList<BuildItem> buildItems;
    for (BuildStepList *list : bsls) {
        const QString name = displayNameForStepId(list->id());
        const QList<BuildStep *> steps = list->steps();
        for (BuildStep *step : steps)
            buildItems.append({step, step->enabled(), name});
        d->m_isDeploying = d->m_isDeploying || list->id() == Constants::BUILDSTEPS_DEPLOY;
    }

    if (buildQueueAppend(buildItems, preambleMessage))
        return true;

    d->m_isDeploying = wasDeploying;
    return false;
}

void BuildManager::appendStep(BuildStep *step, const QString &name)
{
    buildQueueAppend({{step, step->enabled(), name}});
}

template <class T>
int count(const QHash<T *, int> &hash, const T *key)
{
    typename QHash<T *, int>::const_iterator it = hash.find(const_cast<T *>(key));
    typename QHash<T *, int>::const_iterator end = hash.end();
    if (it != end)
        return *it;
    return 0;
}

bool BuildManager::isBuilding(const Project *pro)
{
    return count(d->m_activeBuildSteps, pro) > 0;
}

bool BuildManager::isBuilding(const Target *t)
{
    return count(d->m_activeBuildStepsPerTarget, t) > 0;
}

bool BuildManager::isBuilding(const ProjectConfiguration *p)
{
    return count(d->m_activeBuildStepsPerProjectConfiguration, p) > 0;
}

bool BuildManager::isBuilding(BuildStep *step)
{
    const auto checker = [step](const BuildItem &item) { return item.buildStep == step; };
    return Utils::anyOf(d->m_buildQueue, checker) || Utils::anyOf(d->m_pendingQueue, checker);
}

template <class T> bool increment(QHash<T *, int> &hash, T *key)
{
    typename QHash<T *, int>::iterator it = hash.find(key);
    typename QHash<T *, int>::iterator end = hash.end();
    if (it == end) {
        hash.insert(key, 1);
        return true;
    } else if (*it == 0) {
        ++*it;
        return true;
    } else {
        ++*it;
    }
    return false;
}

template <class T> bool decrement(QHash<T *, int> &hash, T *key)
{
    typename QHash<T *, int>::iterator it = hash.find(key);
    typename QHash<T *, int>::iterator end = hash.end();
    if (it == end) {
        // Can't happen
    } else if (*it == 1) {
        --*it;
        return true;
    } else {
        --*it;
    }
    return false;
}

void BuildManager::incrementActiveBuildSteps(BuildStep *bs)
{
    increment<ProjectConfiguration>(d->m_activeBuildStepsPerProjectConfiguration, bs->projectConfiguration());
    increment<Target>(d->m_activeBuildStepsPerTarget, bs->target());
    if (increment<Project>(d->m_activeBuildSteps, bs->project()))
        emit m_instance->buildStateChanged(bs->project());
}

void BuildManager::decrementActiveBuildSteps(BuildStep *bs)
{
    decrement<ProjectConfiguration>(d->m_activeBuildStepsPerProjectConfiguration, bs->projectConfiguration());
    decrement<Target>(d->m_activeBuildStepsPerTarget, bs->target());
    if (decrement<Project>(d->m_activeBuildSteps, bs->project()))
        emit m_instance->buildStateChanged(bs->project());
}

} // namespace ProjectExplorer
