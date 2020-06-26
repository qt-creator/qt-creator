/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "buildmanager.h"

#include "buildprogress.h"
#include "buildsteplist.h"
#include "compileoutputwindow.h"
#include "deployconfiguration.h"
#include "kit.h"
#include "kitinformation.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "runcontrol.h"
#include "session.h"
#include "target.h"
#include "task.h"
#include "taskhub.h"
#include "taskwindow.h"
#include "waitforstopdialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/outputformatter.h>
#include <utils/runextensions.h>
#include <utils/stringutils.h>

#include <QApplication>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QHash>
#include <QList>
#include <QMessageBox>
#include <QPointer>
#include <QTime>
#include <QTimer>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {
using namespace Internal;

static QString msgProgress(int progress, int total)
{
    return BuildManager::tr("Finished %1 of %n steps", nullptr, total).arg(progress);
}

static const QList<Target *> targetsForSelection(const Project *project,
                                                 ConfigSelection targetSelection)
{
    if (targetSelection == ConfigSelection::All)
        return project->targets();
    return {project->activeTarget()};
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
                 ConfigSelection configSelection, const RunConfiguration *forRunConfig = nullptr)
{
    if (!ProjectExplorerPlugin::saveModifiedFiles())
        return -1;

    const ProjectExplorerSettings &settings = ProjectExplorerPlugin::projectExplorerSettings();
    if (settings.stopBeforeBuild != StopBeforeBuild::None
            && stepIds.contains(Constants::BUILDSTEPS_BUILD)) {
        StopBeforeBuild stopCondition = settings.stopBeforeBuild;
        if (stopCondition == StopBeforeBuild::SameApp && !forRunConfig)
            stopCondition = StopBeforeBuild::SameBuildDir;
        const auto isStoppableRc = [&projects, stopCondition, configSelection, forRunConfig](RunControl *rc) {
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
                    IDevice::ConstPtr device = rc->runnable().device;
                    for (const Target * const t : targetsForSelection(p, configSelection)) {
                        if (device.isNull())
                            device = DeviceKitAspect::device(t->kit());
                        if (device.isNull() || device->type() != Constants::DESKTOP_DEVICE_TYPE)
                            continue;
                        for (const BuildConfiguration * const bc
                             : buildConfigsForSelection(t, configSelection)) {
                            if (rc->runnable().executable.isChildOf(bc->buildDirectory()))
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
                        BuildManager::tr("Stop Applications"),
                        BuildManager::tr("Stop these applications before building?")
                        + "\n\n" + names.join('\n'))
                        == QMessageBox::No) {
                    stopThem = false;
                }
            }

            if (stopThem) {
                foreach (RunControl *rc, toStop)
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

    foreach (Project *pro, projects) {
        if (pro && pro->needsConfiguration()) {
            preambleMessage.append(
                        BuildManager::tr("The project %1 is not configured, skipping it.")
                        .arg(pro->displayName()) + QLatin1Char('\n'));
        }
    }
    foreach (Id id, stepIds) {
        const bool isBuild = id == Constants::BUILDSTEPS_BUILD;
        const bool isClean = id == Constants::BUILDSTEPS_CLEAN;
        const bool isDeploy = id == Constants::BUILDSTEPS_DEPLOY;
        foreach (Project *pro, projects) {
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


class BuildManagerPrivate
{
public:
    Internal::CompileOutputWindow *m_outputWindow = nullptr;
    Internal::TaskWindow *m_taskWindow = nullptr;

    QList<BuildStep *> m_buildQueue;
    QList<bool> m_enabledState;
    QStringList m_stepNames;
    int m_progress = 0;
    int m_maxProgress = 0;
    bool m_running = false;
    bool m_isDeploying = false;
    // is set to true while canceling, so that nextBuildStep knows that the BuildStep finished because of canceling
    bool m_skipDisabled = false;
    bool m_canceling = false;
    bool m_lastStepSucceeded = true;
    bool m_allStepsSucceeded = true;
    BuildStep *m_currentBuildStep = nullptr;
    QString m_currentConfiguration;
    // used to decide if we are building a project to decide when to emit buildStateChanged(Project *)
    QHash<Project *, int>  m_activeBuildSteps;
    QHash<Target *, int> m_activeBuildStepsPerTarget;
    QHash<ProjectConfiguration *, int> m_activeBuildStepsPerProjectConfiguration;
    Project *m_previousBuildStepProject = nullptr;

    // Progress reporting to the progress manager
    QFutureInterface<void> *m_progressFutureInterface = nullptr;
    QFutureWatcher<void> m_progressWatcher;
    QPointer<FutureProgress> m_futureProgress;

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

    connect(SessionManager::instance(), &SessionManager::aboutToRemoveProject,
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
    TaskHub::addCategory(Constants::TASK_CATEGORY_COMPILE,
                         tr("Compile", "Category for compiler issues listed under 'Issues'"));
    TaskHub::addCategory(Constants::TASK_CATEGORY_BUILDSYSTEM,
                         tr("Build System", "Category for build system issues listed under 'Issues'"));
    TaskHub::addCategory(Constants::TASK_CATEGORY_DEPLOYMENT,
                         tr("Deployment", "Category for deployment issues listed under 'Issues'"));
    TaskHub::addCategory(Constants::TASK_CATEGORY_AUTOTEST,
                         tr("Autotests", "Category for autotest issues listed under 'Issues'"));
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

void BuildManager::buildProjectWithDependencies(Project *project, ConfigSelection configSelection)
{
    queue(SessionManager::projectOrder(project), {Id(Constants::BUILDSTEPS_BUILD)},
          configSelection);
}

void BuildManager::cleanProjectWithDependencies(Project *project, ConfigSelection configSelection)
{
    queue(SessionManager::projectOrder(project), {Id(Constants::BUILDSTEPS_CLEAN)},
          configSelection);
}

void BuildManager::rebuildProjectWithDependencies(Project *project, ConfigSelection configSelection)
{
    queue(SessionManager::projectOrder(project),
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
    const int queueCount = queue(SessionManager::projectOrder(pro), stepIds,
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
    return !d->m_buildQueue.isEmpty() || d->m_running;
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

void BuildManager::setCompileOutputSettings(const Internal::CompileOutputSettings &settings)
{
    d->m_outputWindow->setSettings(settings);
}

const Internal::CompileOutputSettings &BuildManager::compileOutputSettings()
{
    return d->m_outputWindow->settings();
}

QString BuildManager::displayNameForStepId(Id stepId)
{
    if (stepId == Constants::BUILDSTEPS_CLEAN) {
        //: Displayed name for a "cleaning" build step
        return tr("Clean");
    }
    if (stepId == Constants::BUILDSTEPS_DEPLOY) {
        //: Displayed name for a deploy step
        return tr("Deploy");
    }
    //: Displayed name for a normal build step
    return tr("Build");
}

void BuildManager::cancel()
{
    if (d->m_running) {
        if (d->m_canceling)
            return;
        d->m_canceling = true;
        d->m_currentBuildStep->cancel();
    }
}

void BuildManager::updateTaskCount()
{
    const int errors = getErrorTaskCount();
    ProgressManager::setApplicationLabel(errors > 0 ? QString::number(errors) : QString());
}

void BuildManager::finish()
{
    const QString elapsedTime = Utils::formatElapsedTime(d->m_elapsed.elapsed());
    m_instance->addToOutputWindow(elapsedTime, BuildStep::OutputFormat::NormalMessage);

    QApplication::alert(ICore::dialogParent(), 3000);
}

void BuildManager::emitCancelMessage()
{
    m_instance->addToOutputWindow(tr("Canceled build/deployment."), BuildStep::OutputFormat::ErrorMessage);
}

void BuildManager::clearBuildQueue()
{
    for (BuildStep *bs : qAsConst(d->m_buildQueue)) {
        decrementActiveBuildSteps(bs);
        disconnectOutput(bs);
    }

    d->m_stepNames.clear();
    d->m_buildQueue.clear();
    d->m_enabledState.clear();
    d->m_running = false;
    d->m_isDeploying = false;
    d->m_previousBuildStepProject = nullptr;
    d->m_currentBuildStep = nullptr;

    d->m_progressFutureInterface->reportCanceled();
    d->m_progressFutureInterface->reportFinished();
    d->m_progressWatcher.setFuture(QFuture<void>());
    delete d->m_progressFutureInterface;
    d->m_progressFutureInterface = nullptr;
    d->m_futureProgress = nullptr;
    d->m_maxProgress = 0;

    emit m_instance->buildQueueFinished(false);
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
    if (d->m_buildQueue.isEmpty()) {
        emit m_instance->buildQueueFinished(true);
        return;
    }
    if (!d->m_running) {
        d->m_elapsed.start();
        // Progress Reporting
        d->m_progressFutureInterface = new QFutureInterface<void>;
        d->m_progressWatcher.setFuture(d->m_progressFutureInterface->future());
        ProgressManager::setApplicationLabel(QString());
        d->m_futureProgress = ProgressManager::addTask(d->m_progressFutureInterface->future(),
              QString(), "ProjectExplorer.Task.Build",
              ProgressManager::KeepOnFinish | ProgressManager::ShowInApplicationIcon);
        connect(d->m_futureProgress.data(), &FutureProgress::clicked,
                m_instance, &BuildManager::showBuildResults);
        d->m_futureProgress.data()->setWidget(new Internal::BuildProgress(d->m_taskWindow));
        d->m_futureProgress.data()->setStatusBarWidget(new Internal::BuildProgress(d->m_taskWindow,
                                                                                   Qt::Horizontal));
        d->m_progress = 0;
        d->m_progressFutureInterface->setProgressRange(0, d->m_maxProgress * 100);

        d->m_running = true;
        d->m_allStepsSucceeded = true;
        d->m_progressFutureInterface->reportStarted();
        nextStep();
    } else {
        // Already running
        d->m_progressFutureInterface->setProgressRange(0, d->m_maxProgress * 100);
        d->m_progressFutureInterface->setProgressValueAndText(d->m_progress*100, msgProgress(d->m_progress, d->m_maxProgress));
    }
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

void BuildManager::nextBuildQueue()
{
    d->m_outputWindow->flush();
    if (d->m_canceling) {
        d->m_canceling = false;
        QTimer::singleShot(0, m_instance, &BuildManager::emitCancelMessage);

        disconnectOutput(d->m_currentBuildStep);
        decrementActiveBuildSteps(d->m_currentBuildStep);

        //TODO NBS fix in qtconcurrent
        d->m_progressFutureInterface->setProgressValueAndText(d->m_progress*100,
                                                              tr("Build/Deployment canceled"));
        clearBuildQueue();
        return;
    }

    disconnectOutput(d->m_currentBuildStep);
    if (!d->m_skipDisabled)
        ++d->m_progress;
    d->m_progressFutureInterface->setProgressValueAndText(d->m_progress*100, msgProgress(d->m_progress, d->m_maxProgress));
    decrementActiveBuildSteps(d->m_currentBuildStep);

    const bool success = d->m_skipDisabled || d->m_lastStepSucceeded;
    if (success) {
        nextStep();
    } else {
        // Build Failure
        d->m_allStepsSucceeded = false;
        Target *t = d->m_currentBuildStep->target();
        const QString projectName = d->m_currentBuildStep->project()->displayName();
        const QString targetName = t->displayName();
        addToOutputWindow(tr("Error while building/deploying project %1 (kit: %2)").arg(projectName, targetName), BuildStep::OutputFormat::Stderr);
        const Tasks kitTasks = t->kit()->validate();
        if (!kitTasks.isEmpty()) {
            addToOutputWindow(tr("The kit %1 has configuration issues which might be the root cause for this problem.")
                              .arg(targetName), BuildStep::OutputFormat::Stderr);
        }
        addToOutputWindow(tr("When executing step \"%1\"").arg(d->m_currentBuildStep->displayName()), BuildStep::OutputFormat::Stderr);

        bool abort = ProjectExplorerPlugin::projectExplorerSettings().abortBuildAllOnError;
        if (!abort) {
            while (!d->m_buildQueue.isEmpty()
                   && d->m_buildQueue.front()->target() == t) {
                BuildStep * const nextStepForFailedTarget = d->m_buildQueue.takeFirst();
                disconnectOutput(nextStepForFailedTarget);
                decrementActiveBuildSteps(nextStepForFailedTarget);
            }
            if (d->m_buildQueue.isEmpty())
                abort = true;
        }

        if (abort) {
            // NBS TODO fix in qtconcurrent
            d->m_progressFutureInterface->setProgressValueAndText(d->m_progress * 100,
                    tr("Error while building/deploying project %1 (kit: %2)")
                        .arg(projectName, targetName));
            clearBuildQueue();
        } else {
            nextStep();
        }
    }
}

void BuildManager::progressChanged(int percent, const QString &text)
{
    if (d->m_progressFutureInterface)
        d->m_progressFutureInterface->setProgressValueAndText(percent + 100 * d->m_progress, text);
}

void BuildManager::nextStep()
{
    if (!d->m_buildQueue.empty()) {
        d->m_currentBuildStep = d->m_buildQueue.front();
        d->m_buildQueue.pop_front();
        QString name = d->m_stepNames.takeFirst();
        d->m_skipDisabled = !d->m_enabledState.takeFirst();
        if (d->m_futureProgress)
            d->m_futureProgress.data()->setTitle(name);

        if (d->m_currentBuildStep->project() != d->m_previousBuildStepProject) {
            const QString projectName = d->m_currentBuildStep->project()->displayName();
            addToOutputWindow(tr("Running steps for project %1...")
                              .arg(projectName), BuildStep::OutputFormat::NormalMessage);
            d->m_previousBuildStepProject = d->m_currentBuildStep->project();
        }

        if (d->m_skipDisabled) {
            addToOutputWindow(tr("Skipping disabled step %1.")
                              .arg(d->m_currentBuildStep->displayName()), BuildStep::OutputFormat::NormalMessage);
            nextBuildQueue();
            return;
        }

        static const auto finishedHandler = [](bool success)  {
            d->m_outputWindow->outputFormatter()->flush();
            d->m_lastStepSucceeded = success;
            disconnect(d->m_currentBuildStep, nullptr, instance(), nullptr);
            BuildManager::nextBuildQueue();
        };
        connect(d->m_currentBuildStep, &BuildStep::finished, instance(), finishedHandler);
        connect(d->m_currentBuildStep, &BuildStep::progress,
                instance(), &BuildManager::progressChanged);
        d->m_outputWindow->reset();
        d->m_currentBuildStep->setupOutputFormatter(d->m_outputWindow->outputFormatter());
        d->m_currentBuildStep->run();
    } else {
        d->m_running = false;
        d->m_isDeploying = false;
        d->m_previousBuildStepProject = nullptr;
        d->m_progressFutureInterface->reportFinished();
        d->m_progressWatcher.setFuture(QFuture<void>());
        d->m_currentBuildStep = nullptr;
        delete d->m_progressFutureInterface;
        d->m_progressFutureInterface = nullptr;
        d->m_maxProgress = 0;
        emit m_instance->buildQueueFinished(d->m_allStepsSucceeded);
    }
}

bool BuildManager::buildQueueAppend(const QList<BuildStep *> &steps, QStringList names, const QStringList &preambleMessage)
{
    if (!d->m_running) {
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

    int count = steps.size();
    bool init = true;
    int i = 0;
    for (; i < count; ++i) {
        BuildStep *bs = steps.at(i);
        connect(bs, &BuildStep::addTask, m_instance, &BuildManager::addToTaskWindow);
        connect(bs, &BuildStep::addOutput, m_instance, &BuildManager::addToOutputWindow);
        if (bs->enabled()) {
            init = bs->init();
            if (!init)
                break;
        }
    }
    if (!init) {
        BuildStep *bs = steps.at(i);

        // cleaning up
        // print something for the user
        const QString projectName = bs->project()->displayName();
        const QString targetName = bs->target()->displayName();
        addToOutputWindow(tr("Error while building/deploying project %1 (kit: %2)").arg(projectName, targetName), BuildStep::OutputFormat::Stderr);
        addToOutputWindow(tr("When executing step \"%1\"").arg(bs->displayName()), BuildStep::OutputFormat::Stderr);

        // disconnect the buildsteps again
        for (int j = 0; j <= i; ++j)
            disconnectOutput(steps.at(j));
        return false;
    }

    // Everthing init() well
    for (i = 0; i < count; ++i) {
        d->m_buildQueue.append(steps.at(i));
        d->m_stepNames.append(names.at(i));
        bool enabled = steps.at(i)->enabled();
        d->m_enabledState.append(enabled);
        if (enabled)
            ++d->m_maxProgress;
        incrementActiveBuildSteps(steps.at(i));
    }
    return true;
}

bool BuildManager::buildList(BuildStepList *bsl)
{
    return buildLists({bsl});
}

bool BuildManager::buildLists(const QList<BuildStepList *> bsls, const QStringList &preambelMessage)
{
    QList<BuildStep *> steps;
    QStringList stepListNames;
    for (BuildStepList *list : bsls) {
        steps.append(list->steps());
        stepListNames.append(displayNameForStepId(list->id()));
        d->m_isDeploying = d->m_isDeploying || list->id() == Constants::BUILDSTEPS_DEPLOY;
    }

    QStringList names;
    names.reserve(steps.size());
    for (int i = 0; i < bsls.size(); ++i) {
        for (int j = 0; j < bsls.at(i)->count(); ++j)
            names.append(stepListNames.at(i));
    }

    bool success = buildQueueAppend(steps, names, preambelMessage);
    if (!success) {
        d->m_outputWindow->popup(IOutputPane::NoModeSwitch);
        d->m_isDeploying = false;
        return false;
    }

    if (d->m_outputWindow->settings().popUp)
        d->m_outputWindow->popup(IOutputPane::NoModeSwitch);
    startBuildQueue();
    return true;
}

void BuildManager::appendStep(BuildStep *step, const QString &name)
{
    bool success = buildQueueAppend({step}, {name});
    if (!success) {
        d->m_outputWindow->popup(IOutputPane::NoModeSwitch);
        return;
    }
    if (d->m_outputWindow->settings().popUp)
        d->m_outputWindow->popup(IOutputPane::NoModeSwitch);
    startBuildQueue();
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
    return (d->m_currentBuildStep == step) || d->m_buildQueue.contains(step);
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

void BuildManager::disconnectOutput(BuildStep *bs)
{
    disconnect(bs, &BuildStep::addTask, m_instance, nullptr);
    disconnect(bs, &BuildStep::addOutput, m_instance, nullptr);
}

} // namespace ProjectExplorer
