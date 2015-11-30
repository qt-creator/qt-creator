/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "buildmanager.h"

#include "buildprogress.h"
#include "buildsteplist.h"
#include "compileoutputwindow.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "target.h"
#include "taskwindow.h"
#include "taskhub.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <projectexplorer/session.h>
#include <extensionsystem/pluginmanager.h>

#include <QPointer>
#include <QTime>
#include <QTimer>
#include <QList>
#include <QHash>
#include <QFutureWatcher>
#include <QElapsedTimer>

#include <utils/QtConcurrentTools>

#include <QApplication>

using namespace Core;

namespace ProjectExplorer {

static QString msgProgress(int progress, int total)
{
    return BuildManager::tr("Finished %1 of %n steps", 0, total).arg(progress);
}

struct BuildManagerPrivate
{
    BuildManagerPrivate();

    Internal::CompileOutputWindow *m_outputWindow;
    TaskHub *m_taskHub;
    Internal::TaskWindow *m_taskWindow;

    QList<BuildStep *> m_buildQueue;
    QList<bool> m_enabledState;
    QStringList m_stepNames;
    bool m_running;
    QFutureWatcher<bool> m_watcher;
    QFutureInterface<bool> m_futureInterfaceForAysnc;
    BuildStep *m_currentBuildStep;
    QString m_currentConfiguration;
    // used to decide if we are building a project to decide when to emit buildStateChanged(Project *)
    QHash<Project *, int>  m_activeBuildSteps;
    QHash<Target *, int> m_activeBuildStepsPerTarget;
    QHash<ProjectConfiguration *, int> m_activeBuildStepsPerProjectConfiguration;
    Project *m_previousBuildStepProject;
    // is set to true while canceling, so that nextBuildStep knows that the BuildStep finished because of canceling
    bool m_skipDisabled;
    bool m_canceling;

    // Progress reporting to the progress manager
    int m_progress;
    int m_maxProgress;
    QFutureInterface<void> *m_progressFutureInterface;
    QFutureWatcher<void> m_progressWatcher;
    QPointer<FutureProgress> m_futureProgress;

    QElapsedTimer m_elapsed;
};

BuildManagerPrivate::BuildManagerPrivate() :
    m_running(false)
  , m_previousBuildStepProject(0)
  , m_skipDisabled(false)
  , m_canceling(false)
  , m_maxProgress(0)
  , m_progressFutureInterface(0)
{
}

static BuildManagerPrivate *d = 0;
static BuildManager *m_instance = 0;

BuildManager::BuildManager(QObject *parent, QAction *cancelBuildAction)
    : QObject(parent)
{
    m_instance = this;
    d = new BuildManagerPrivate;

    connect(&d->m_watcher, SIGNAL(finished()),
            this, SLOT(nextBuildQueue()), Qt::QueuedConnection);

    connect(&d->m_watcher, SIGNAL(progressValueChanged(int)),
            this, SLOT(progressChanged()));
    connect(&d->m_watcher, SIGNAL(progressTextChanged(QString)),
            this, SLOT(progressTextChanged()));
    connect(&d->m_watcher, SIGNAL(progressRangeChanged(int,int)),
            this, SLOT(progressChanged()));

    connect(SessionManager::instance(), SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)),
            this, SLOT(aboutToRemoveProject(ProjectExplorer::Project*)));

    d->m_outputWindow = new Internal::CompileOutputWindow(cancelBuildAction);
    ExtensionSystem::PluginManager::addObject(d->m_outputWindow);

    d->m_taskWindow = new Internal::TaskWindow;
    ExtensionSystem::PluginManager::addObject(d->m_taskWindow);

    qRegisterMetaType<ProjectExplorer::BuildStep::OutputFormat>();
    qRegisterMetaType<ProjectExplorer::BuildStep::OutputNewlineSetting>();

    connect(d->m_taskWindow, SIGNAL(tasksChanged()),
            this, SLOT(updateTaskCount()));

    connect(d->m_taskWindow, SIGNAL(tasksCleared()),
            this,SIGNAL(tasksCleared()));

    connect(&d->m_progressWatcher, SIGNAL(canceled()),
            this, SLOT(cancel()));
    connect(&d->m_progressWatcher, SIGNAL(finished()),
            this, SLOT(finish()));
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
}

BuildManager::~BuildManager()
{
    cancel();
    m_instance = 0;
    ExtensionSystem::PluginManager::removeObject(d->m_taskWindow);
    delete d->m_taskWindow;

    ExtensionSystem::PluginManager::removeObject(d->m_outputWindow);
    delete d->m_outputWindow;

    delete d;
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

int BuildManager::getErrorTaskCount()
{
    const int errors =
            d->m_taskWindow->errorTaskCount(Constants::TASK_CATEGORY_BUILDSYSTEM)
            + d->m_taskWindow->errorTaskCount(Constants::TASK_CATEGORY_COMPILE)
            + d->m_taskWindow->errorTaskCount(Constants::TASK_CATEGORY_DEPLOYMENT);
    return errors;
}

void BuildManager::cancel()
{
    if (d->m_running) {
        if (d->m_canceling)
            return;
        d->m_canceling = true;
        d->m_watcher.cancel();
        if (d->m_currentBuildStep->runInGuiThread()) {
            d->m_currentBuildStep->cancel();
            while (d->m_canceling)
                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        } else {
            d->m_watcher.waitForFinished();
        }
    }
}

void BuildManager::updateTaskCount()
{
    const int errors = getErrorTaskCount();
    ProgressManager::setApplicationLabel(errors > 0 ? QString::number(errors) : QString());
    emit m_instance->tasksChanged();
}

void BuildManager::finish()
{
    const QTime format = QTime(0, 0, 0, 0).addMSecs(d->m_elapsed.elapsed() + 500);
    QString time = format.toString(QLatin1String("h:mm:ss"));
    if (time.startsWith(QLatin1String("0:")))
        time.remove(0, 2); // Don't display zero hours
    addToOutputWindow(tr("Elapsed time: %1.") .arg(time), BuildStep::MessageOutput);

    QApplication::alert(ICore::mainWindow(), 3000);
}

void BuildManager::emitCancelMessage()
{
    addToOutputWindow(tr("Canceled build/deployment."), BuildStep::ErrorMessageOutput);
}

void BuildManager::clearBuildQueue()
{
    foreach (BuildStep *bs, d->m_buildQueue) {
        decrementActiveBuildSteps(bs);
        disconnectOutput(bs);
    }

    d->m_stepNames.clear();
    d->m_buildQueue.clear();
    d->m_enabledState.clear();
    d->m_running = false;
    d->m_previousBuildStepProject = 0;
    d->m_currentBuildStep = 0;

    d->m_progressFutureInterface->reportCanceled();
    d->m_progressFutureInterface->reportFinished();
    d->m_progressWatcher.setFuture(QFuture<void>());
    delete d->m_progressFutureInterface;
    d->m_progressFutureInterface = 0;
    d->m_futureProgress = 0;
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
        connect(d->m_futureProgress.data(), SIGNAL(clicked()), m_instance, SLOT(showBuildResults()));
        d->m_futureProgress.data()->setWidget(new Internal::BuildProgress(d->m_taskWindow));
        d->m_futureProgress.data()->setStatusBarWidget(new Internal::BuildProgress(d->m_taskWindow,
                                                                                   Qt::Horizontal));
        d->m_progress = 0;
        d->m_progressFutureInterface->setProgressRange(0, d->m_maxProgress * 100);

        d->m_running = true;
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
    BuildStep::OutputNewlineSetting newLineSetting)
{
    QString stringToWrite;
    if (format == BuildStep::MessageOutput || format == BuildStep::ErrorMessageOutput) {
        stringToWrite = QTime::currentTime().toString();
        stringToWrite += QLatin1String(": ");
    }
    stringToWrite += string;
    if (newLineSetting == BuildStep::DoAppendNewline)
        stringToWrite += QLatin1Char('\n');
    d->m_outputWindow->appendText(stringToWrite, format);
}

void BuildManager::buildStepFinishedAsync()
{
    disconnect(d->m_currentBuildStep, SIGNAL(finished()),
               m_instance, SLOT(buildStepFinishedAsync()));
    d->m_futureInterfaceForAysnc = QFutureInterface<bool>();
    nextBuildQueue();
}

void BuildManager::nextBuildQueue()
{
    d->m_outputWindow->flush();
    if (d->m_canceling) {
        d->m_canceling = false;
        QTimer::singleShot(0, m_instance, SLOT(emitCancelMessage()));

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

    bool result = d->m_skipDisabled || d->m_watcher.result();
    if (!result) {
        // Build Failure
        const QString projectName = d->m_currentBuildStep->project()->displayName();
        const QString targetName = d->m_currentBuildStep->target()->displayName();
        addToOutputWindow(tr("Error while building/deploying project %1 (kit: %2)").arg(projectName, targetName), BuildStep::ErrorOutput);
        addToOutputWindow(tr("When executing step \"%1\"").arg(d->m_currentBuildStep->displayName()), BuildStep::ErrorOutput);
        // NBS TODO fix in qtconcurrent
        d->m_progressFutureInterface->setProgressValueAndText(d->m_progress*100, tr("Error while building/deploying project %1 (kit: %2)").arg(projectName, targetName));
    }

    if (result)
        nextStep();
    else
        clearBuildQueue();
}

void BuildManager::progressChanged()
{
    if (!d->m_progressFutureInterface)
        return;
    int range = d->m_watcher.progressMaximum() - d->m_watcher.progressMinimum();
    if (range != 0) {
        int percent = (d->m_watcher.progressValue() - d->m_watcher.progressMinimum()) * 100 / range;
        d->m_progressFutureInterface->setProgressValueAndText(d->m_progress * 100 + percent, msgProgress(d->m_progress, d->m_maxProgress)
                                                              + QLatin1Char('\n') + d->m_watcher.progressText());
    }
}

void BuildManager::progressTextChanged()
{
    if (!d->m_progressFutureInterface)
        return;
    int range = d->m_watcher.progressMaximum() - d->m_watcher.progressMinimum();
    int percent = 0;
    if (range != 0)
        percent = (d->m_watcher.progressValue() - d->m_watcher.progressMinimum()) * 100 / range;
    d->m_progressFutureInterface->setProgressValueAndText(d->m_progress*100 + percent, msgProgress(d->m_progress, d->m_maxProgress) +
                                                          QLatin1Char('\n') + d->m_watcher.progressText());
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
                              .arg(projectName), BuildStep::MessageOutput);
            d->m_previousBuildStepProject = d->m_currentBuildStep->project();
        }

        if (d->m_skipDisabled) {
            addToOutputWindow(tr("Skipping disabled step %1.")
                              .arg(d->m_currentBuildStep->displayName()), BuildStep::MessageOutput);
            nextBuildQueue();
            return;
        }

        if (d->m_currentBuildStep->runInGuiThread()) {
            connect (d->m_currentBuildStep, SIGNAL(finished()),
                     m_instance, SLOT(buildStepFinishedAsync()));
            d->m_watcher.setFuture(d->m_futureInterfaceForAysnc.future());
            d->m_currentBuildStep->run(d->m_futureInterfaceForAysnc);
        } else {
            d->m_watcher.setFuture(QtConcurrent::run(&BuildStep::run, d->m_currentBuildStep));
        }
    } else {
        d->m_running = false;
        d->m_previousBuildStepProject = 0;
        d->m_progressFutureInterface->reportFinished();
        d->m_progressWatcher.setFuture(QFuture<void>());
        d->m_currentBuildStep = 0;
        delete d->m_progressFutureInterface;
        d->m_progressFutureInterface = 0;
        d->m_maxProgress = 0;
        emit m_instance->buildQueueFinished(true);
    }
}

bool BuildManager::buildQueueAppend(QList<BuildStep *> steps, QStringList names, const QStringList &preambleMessage)
{
    if (!d->m_running) {
        d->m_outputWindow->clearContents();
        TaskHub::clearTasks(Constants::TASK_CATEGORY_COMPILE);
        TaskHub::clearTasks(Constants::TASK_CATEGORY_BUILDSYSTEM);
        TaskHub::clearTasks(Constants::TASK_CATEGORY_DEPLOYMENT);

        foreach (const QString &str, preambleMessage)
            addToOutputWindow(str, BuildStep::MessageOutput, BuildStep::DontAppendNewline);
    }

    QList<const BuildStep *> earlierSteps;
    int count = steps.size();
    bool init = true;
    int i = 0;
    for (; i < count; ++i) {
        BuildStep *bs = steps.at(i);
        connect(bs, SIGNAL(addTask(ProjectExplorer::Task, int, int)),
                m_instance, SLOT(addToTaskWindow(ProjectExplorer::Task, int, int)));
        connect(bs, SIGNAL(addOutput(QString,ProjectExplorer::BuildStep::OutputFormat,ProjectExplorer::BuildStep::OutputNewlineSetting)),
                m_instance, SLOT(addToOutputWindow(QString,ProjectExplorer::BuildStep::OutputFormat,ProjectExplorer::BuildStep::OutputNewlineSetting)));
        if (bs->enabled()) {
            init = bs->init(earlierSteps);
            if (!init)
                break;
            earlierSteps.append(bs);
        }
    }
    if (!init) {
        BuildStep *bs = steps.at(i);

        // cleaning up
        // print something for the user
        const QString projectName = bs->project()->displayName();
        const QString targetName = bs->target()->displayName();
        addToOutputWindow(tr("Error while building/deploying project %1 (kit: %2)").arg(projectName, targetName), BuildStep::ErrorOutput);
        addToOutputWindow(tr("When executing step \"%1\"").arg(bs->displayName()), BuildStep::ErrorOutput);

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

bool BuildManager::buildList(BuildStepList *bsl, const QString &stepListName)
{
    return buildLists(QList<BuildStepList *>() << bsl, QStringList() << stepListName);
}

bool BuildManager::buildLists(QList<BuildStepList *> bsls, const QStringList &stepListNames, const QStringList &preambelMessage)
{
    QList<BuildStep *> steps;
    foreach (BuildStepList *list, bsls)
        steps.append(list->steps());

    QStringList names;
    names.reserve(steps.size());
    for (int i = 0; i < bsls.size(); ++i) {
        for (int j = 0; j < bsls.at(i)->steps().size(); ++j) {
            names.append(stepListNames.at(i));
        }
    }

    bool success = buildQueueAppend(steps, names, preambelMessage);
    if (!success) {
        d->m_outputWindow->popup(IOutputPane::NoModeSwitch);
        return false;
    }

    if (ProjectExplorerPlugin::projectExplorerSettings().showCompilerOutput)
        d->m_outputWindow->popup(IOutputPane::NoModeSwitch);
    startBuildQueue();
    return true;
}

void BuildManager::appendStep(BuildStep *step, const QString &name)
{
    bool success = buildQueueAppend(QList<BuildStep *>() << step, QStringList() << name);
    if (!success) {
        d->m_outputWindow->popup(IOutputPane::NoModeSwitch);
        return;
    }
    if (ProjectExplorerPlugin::projectExplorerSettings().showCompilerOutput)
        d->m_outputWindow->popup(IOutputPane::NoModeSwitch);
    startBuildQueue();
}

template <class T>
int count(const QHash<T *, int> &hash, T *key)
{
    typename QHash<T *, int>::const_iterator it = hash.find(key);
    typename QHash<T *, int>::const_iterator end = hash.end();
    if (it != end)
        return *it;
    return 0;
}

bool BuildManager::isBuilding(Project *pro)
{
    return count(d->m_activeBuildSteps, pro) > 0;
}

bool BuildManager::isBuilding(Target *t)
{
    return count(d->m_activeBuildStepsPerTarget, t) > 0;
}

bool BuildManager::isBuilding(ProjectConfiguration *p)
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
    disconnect(bs, SIGNAL(addTask(ProjectExplorer::Task, int, int)),
               m_instance, SLOT(addToTaskWindow(ProjectExplorer::Task, int, int)));
    disconnect(bs, SIGNAL(addOutput(QString, ProjectExplorer::BuildStep::OutputFormat,
        ProjectExplorer::BuildStep::OutputNewlineSetting)),
        m_instance, SLOT(addToOutputWindow(QString, ProjectExplorer::BuildStep::OutputFormat,
            ProjectExplorer::BuildStep::OutputNewlineSetting)));
}

} // namespace ProjectExplorer
