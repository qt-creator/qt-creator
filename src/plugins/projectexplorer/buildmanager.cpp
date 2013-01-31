/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "buildmanager.h"

#include "buildprogress.h"
#include "buildsteplist.h"
#include "compileoutputwindow.h"
#include "projectexplorerconstants.h"
#include "projectexplorer.h"
#include "project.h"
#include "projectexplorersettings.h"
#include "target.h"
#include "taskwindow.h"
#include "taskhub.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <projectexplorer/session.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QPointer>
#include <QTime>
#include <QTimer>
#include <QMetaType>
#include <QList>
#include <QHash>
#include <QFutureWatcher>
#include <QElapsedTimer>

#include <utils/QtConcurrentTools>

#include <QApplication>

static inline QString msgProgress(int progress, int total)
{
    return ProjectExplorer::BuildManager::tr("Finished %1 of %n steps", 0, total).arg(progress);
}

namespace ProjectExplorer {
struct BuildManagerPrivate {
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
    bool m_doNotEnterEventLoop;
    QEventLoop *m_eventLoop;

    // Progress reporting to the progress manager
    int m_progress;
    int m_maxProgress;
    QFutureInterface<void> *m_progressFutureInterface;
    QFutureWatcher<void> m_progressWatcher;
    QPointer<Core::FutureProgress> m_futureProgress;

    QElapsedTimer m_elapsed;
};

BuildManagerPrivate::BuildManagerPrivate() :
    m_running(false)
  , m_previousBuildStepProject(0)
  , m_skipDisabled(false)
  , m_canceling(false)
  , m_doNotEnterEventLoop(false)
  , m_eventLoop(0)
  , m_maxProgress(0)
  , m_progressFutureInterface(0)
{
}

BuildManager::BuildManager(ProjectExplorerPlugin *parent, QAction *cancelBuildAction)
    : QObject(parent), d(new BuildManagerPrivate)
{
    connect(&d->m_watcher, SIGNAL(finished()),
            this, SLOT(nextBuildQueue()));

    connect(&d->m_watcher, SIGNAL(progressValueChanged(int)),
            this, SLOT(progressChanged()));
    connect(&d->m_watcher, SIGNAL(progressTextChanged(QString)),
            this, SLOT(progressTextChanged()));
    connect(&d->m_watcher, SIGNAL(progressRangeChanged(int,int)),
            this, SLOT(progressChanged()));

    connect(parent->session(), SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)),
            this, SLOT(aboutToRemoveProject(ProjectExplorer::Project*)));

    d->m_outputWindow = new Internal::CompileOutputWindow(this, cancelBuildAction);
    ExtensionSystem::PluginManager::addObject(d->m_outputWindow);

    d->m_taskHub = ProjectExplorerPlugin::instance()->taskHub();
    d->m_taskWindow = new Internal::TaskWindow(d->m_taskHub);
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

void BuildManager::extensionsInitialized()
{
    d->m_taskHub->addCategory(Core::Id(Constants::TASK_CATEGORY_COMPILE),
        tr("Compile", "Category for compiler issues listed under 'Issues'"));
    d->m_taskHub->addCategory(Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM),
        tr("Build System", "Category for build system issues listed under 'Issues'"));
}

BuildManager::~BuildManager()
{
    cancel();
    ExtensionSystem::PluginManager::removeObject(d->m_taskWindow);
    delete d->m_taskWindow;

    ExtensionSystem::PluginManager::removeObject(d->m_outputWindow);
    delete d->m_outputWindow;

    delete d;
}

void BuildManager::aboutToRemoveProject(ProjectExplorer::Project *p)
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

bool BuildManager::isBuilding() const
{
    // we are building even if we are not running yet
    return !d->m_buildQueue.isEmpty() || d->m_running;
}

int BuildManager::getErrorTaskCount() const
{
    const int errors =
            d->m_taskWindow->errorTaskCount(Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM))
            + d->m_taskWindow->errorTaskCount(Core::Id(Constants::TASK_CATEGORY_COMPILE));
    return errors;
}

void BuildManager::cancel()
{
    if (d->m_running) {
        d->m_canceling = true;
        d->m_watcher.cancel();
        if (d->m_currentBuildStep->runInGuiThread()) {
            // This is evil. A nested event loop.
            d->m_currentBuildStep->cancel();
            if (d->m_doNotEnterEventLoop) {
                d->m_doNotEnterEventLoop = false;
            } else {
                d->m_eventLoop = new QEventLoop;
                d->m_eventLoop->exec();
                delete d->m_eventLoop;
                d->m_eventLoop = 0;
            }
        } else {
            d->m_watcher.waitForFinished();
        }

        // The cancel message is added to the output window via a single shot timer
        // since the canceling is likely to have generated new addToOutputWindow signals
        // which are waiting in the event queue to be processed
        // (And we want those to be before the cancel message.)
        QTimer::singleShot(0, this, SLOT(emitCancelMessage()));

        disconnectOutput(d->m_currentBuildStep);
        decrementActiveBuildSteps(d->m_currentBuildStep);

        d->m_progressFutureInterface->setProgressValueAndText(d->m_progress*100, tr("Build/Deployment canceled")); //TODO NBS fix in qtconcurrent
        clearBuildQueue();
    }
    return;
}

void BuildManager::updateTaskCount()
{
    Core::ProgressManager *progressManager = Core::ICore::progressManager();
    const int errors = getErrorTaskCount();
    if (errors > 0)
        progressManager->setApplicationLabel(QString::number(errors));
    else
        progressManager->setApplicationLabel(QString());
    emit tasksChanged();
}

void BuildManager::finish()
{
    const int seconds = d->m_elapsed.elapsed() / 1000;
    const int minutes = seconds / 60;
    const int hours = minutes / 60;
    QString maybeHours;
    if (hours) {
        maybeHours.setNum(hours);
        maybeHours.append(QLatin1Char(':'));
    }
    addToOutputWindow(tr("Elapsed time: %1%2:%3.") .arg(maybeHours)
                      .arg(minutes % 60, 2, 10, QLatin1Char('0'))
                      .arg(seconds % 60, 2, 10, QLatin1Char('0')), BuildStep::MessageOutput);

    QApplication::alert(Core::ICore::mainWindow(), 3000);
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

    emit buildQueueFinished(false);
}


void BuildManager::toggleOutputWindow()
{
    d->m_outputWindow->toggle(Core::IOutputPane::ModeSwitch);
}

void BuildManager::showTaskWindow()
{
    d->m_taskWindow->popup(Core::IOutputPane::NoModeSwitch);
}

void BuildManager::toggleTaskWindow()
{
    d->m_taskWindow->toggle(Core::IOutputPane::ModeSwitch);
}

bool BuildManager::tasksAvailable() const
{
    const int count =
            d->m_taskWindow->taskCount(Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM))
            + d->m_taskWindow->taskCount(Core::Id(Constants::TASK_CATEGORY_COMPILE));
    return count > 0;
}

void BuildManager::startBuildQueue(const QStringList &preambleMessage)
{
    if (d->m_buildQueue.isEmpty()) {
        emit buildQueueFinished(true);
        return;
    }
    if (!d->m_running) {
        d->m_elapsed.start();
        // Progress Reporting
        Core::ProgressManager *progressManager = Core::ICore::progressManager();
        d->m_progressFutureInterface = new QFutureInterface<void>;
        d->m_progressWatcher.setFuture(d->m_progressFutureInterface->future());
        d->m_outputWindow->clearContents();
        foreach (const QString &str, preambleMessage)
            addToOutputWindow(str, BuildStep::MessageOutput, BuildStep::DontAppendNewline);
        d->m_taskHub->clearTasks(Core::Id(Constants::TASK_CATEGORY_COMPILE));
        d->m_taskHub->clearTasks(Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
        progressManager->setApplicationLabel(QString());
        d->m_futureProgress = progressManager->addTask(d->m_progressFutureInterface->future(),
              QString(),
              QLatin1String(Constants::TASK_BUILD),
              Core::ProgressManager::KeepOnFinish | Core::ProgressManager::ShowInApplicationIcon);
        connect(d->m_futureProgress.data(), SIGNAL(clicked()), this, SLOT(showBuildResults()));
        d->m_futureProgress.data()->setWidget(new Internal::BuildProgress(d->m_taskWindow));
        d->m_progress = 0;
        d->m_progressFutureInterface->setProgressRange(0, d->m_maxProgress * 100);

        d->m_running = true;
        d->m_canceling = false;
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

void BuildManager::addToTaskWindow(const ProjectExplorer::Task &task)
{
    d->m_outputWindow->registerPositionOf(task);
    // Distribute to all others
    d->m_taskHub->addTask(task);
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
               this, SLOT(buildStepFinishedAsync()));
    d->m_futureInterfaceForAysnc = QFutureInterface<bool>();
    if (d->m_canceling) {
        if (d->m_eventLoop)
            d->m_eventLoop->exit();
        else
            d->m_doNotEnterEventLoop = true;
    } else {
        nextBuildQueue();
    }
}

void BuildManager::nextBuildQueue()
{
    if (d->m_canceling)
        return;

    disconnectOutput(d->m_currentBuildStep);
    ++d->m_progress;
    d->m_progressFutureInterface->setProgressValueAndText(d->m_progress*100, msgProgress(d->m_progress, d->m_maxProgress));
    decrementActiveBuildSteps(d->m_currentBuildStep);

    bool result = d->m_skipDisabled || d->m_watcher.result();
    if (!result) {
        // Build Failure
        const QString projectName = d->m_currentBuildStep->project()->displayName();
        const QString targetName = d->m_currentBuildStep->target()->displayName();
        addToOutputWindow(tr("Error while building/deploying project %1 (kit: %2)").arg(projectName, targetName), BuildStep::ErrorOutput);
        addToOutputWindow(tr("When executing step '%1'").arg(d->m_currentBuildStep->displayName()), BuildStep::ErrorOutput);
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
                     this, SLOT(buildStepFinishedAsync()));
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
        emit buildQueueFinished(true);
    }
}

bool BuildManager::buildQueueAppend(QList<BuildStep *> steps, QStringList names)
{
    int count = steps.size();
    bool init = true;
    int i = 0;
    for (; i < count; ++i) {
        BuildStep *bs = steps.at(i);
        connect(bs, SIGNAL(addTask(ProjectExplorer::Task)),
                this, SLOT(addToTaskWindow(ProjectExplorer::Task)));
        connect(bs, SIGNAL(addOutput(QString,ProjectExplorer::BuildStep::OutputFormat,ProjectExplorer::BuildStep::OutputNewlineSetting)),
                this, SLOT(addToOutputWindow(QString,ProjectExplorer::BuildStep::OutputFormat,ProjectExplorer::BuildStep::OutputNewlineSetting)));
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
        addToOutputWindow(tr("Error while building/deploying project %1 (kit: %2)").arg(projectName, targetName), BuildStep::ErrorOutput);
        addToOutputWindow(tr("When executing step '%1'").arg(bs->displayName()), BuildStep::ErrorOutput);

        // disconnect the buildsteps again
        for (int j = 0; j <= i; ++j)
            disconnectOutput(steps.at(j));
        return false;
    }

    // Everthing init() well
    for (i = 0; i < count; ++i) {
        ++d->m_maxProgress;
        d->m_buildQueue.append(steps.at(i));
        d->m_stepNames.append(names.at(i));
        d->m_enabledState.append(steps.at(i)->enabled());
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

    bool success = buildQueueAppend(steps, names);
    if (!success) {
        d->m_outputWindow->popup(Core::IOutputPane::NoModeSwitch);
        return false;
    }

    if (ProjectExplorerPlugin::instance()->projectExplorerSettings().showCompilerOutput)
        d->m_outputWindow->popup(Core::IOutputPane::NoModeSwitch);
    startBuildQueue(preambelMessage);
    return true;
}

void BuildManager::appendStep(BuildStep *step, const QString &name)
{
    bool success = buildQueueAppend(QList<BuildStep *>() << step, QStringList() << name);
    if (!success) {
        d->m_outputWindow->popup(Core::IOutputPane::NoModeSwitch);
        return;
    }
    if (ProjectExplorerPlugin::instance()->projectExplorerSettings().showCompilerOutput)
        d->m_outputWindow->popup(Core::IOutputPane::NoModeSwitch);
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
        emit buildStateChanged(bs->project());
}

void BuildManager::decrementActiveBuildSteps(BuildStep *bs)
{
    decrement<ProjectConfiguration>(d->m_activeBuildStepsPerProjectConfiguration, bs->projectConfiguration());
    decrement<Target>(d->m_activeBuildStepsPerTarget, bs->target());
    if (decrement<Project>(d->m_activeBuildSteps, bs->project()))
        emit buildStateChanged(bs->project());
}

void BuildManager::disconnectOutput(BuildStep *bs)
{
    disconnect(bs, SIGNAL(addTask(ProjectExplorer::Task)),
               this, SLOT(addToTaskWindow(ProjectExplorer::Task)));
    disconnect(bs, SIGNAL(addOutput(QString, ProjectExplorer::BuildStep::OutputFormat,
        ProjectExplorer::BuildStep::OutputNewlineSetting)),
        this, SLOT(addToOutputWindow(QString, ProjectExplorer::BuildStep::OutputFormat,
            ProjectExplorer::BuildStep::OutputNewlineSetting)));
}

} // namespace ProjectExplorer
