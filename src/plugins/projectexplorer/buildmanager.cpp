/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "buildmanager.h"

#include "buildprogress.h"
#include "buildstep.h"
#include "compileoutputwindow.h"
#include "projectexplorerconstants.h"
#include "projectexplorer.h"
#include "taskwindow.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtGui/QHeaderView>
#include <QtGui/QIcon>
#include <QtGui/QLabel>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

BuildManager::BuildManager(ProjectExplorerPlugin *parent)
    : QObject(parent)
    , m_running(false)
    , m_previousBuildStepProject(0)
    , m_canceling(false)
    , m_maxProgress(0)
    , m_progressFutureInterface(0)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    m_projectExplorerPlugin = parent;

    connect(&m_watcher, SIGNAL(finished()),
            this, SLOT(nextBuildQueue()));

    m_outputWindow = new CompileOutputWindow(this);
    pm->addObject(m_outputWindow);

    m_taskWindow = new TaskWindow;
    pm->addObject(m_taskWindow);

    connect(m_taskWindow, SIGNAL(tasksChanged()),
            this, SIGNAL(tasksChanged()));

    connect(&m_progressWatcher, SIGNAL(canceled()),
            this, SLOT(cancel()));
}

BuildManager::~BuildManager()
{
    cancel();
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();

    pm->removeObject(m_taskWindow);
    delete m_taskWindow;

    pm->removeObject(m_outputWindow);
    delete m_outputWindow;
}

bool BuildManager::isBuilding() const
{
    // we are building even if we are not running yet
    return !m_buildQueue.isEmpty() || m_running;
}

void BuildManager::cancel()
{
    if (m_running) {
        m_canceling = true;
        m_watcher.cancel();
        m_watcher.waitForFinished();

        // The cancel message is added to the output window via a single shot timer
        // since the canceling is likely to have generated new addToOutputWindow signals
        // which are waiting in the event queue to be processed
        // (And we want those to be before the cancel message.)
        QTimer::singleShot(0, this, SLOT(emitCancelMessage()));

        disconnect(m_currentBuildStep, SIGNAL(addToTaskWindow(QString, int, int, QString)),
                   this, SLOT(addToTaskWindow(QString, int, int, QString)));
        disconnect(m_currentBuildStep, SIGNAL(addToOutputWindow(QString)),
                   this, SLOT(addToOutputWindow(QString)));
        decrementActiveBuildSteps(m_currentBuildStep->project());

        m_progressFutureInterface->setProgressValueAndText(m_progress, "Build canceled"); //TODO NBS fix in qtconcurrent
        clearBuildQueue();
    }
    return;
}

void BuildManager::emitCancelMessage()
{
    emit addToOutputWindow(tr("<font color=\"#ff0000\">Canceled build.</font>"));
}

void BuildManager::clearBuildQueue()
{
    foreach (BuildStep * bs, m_buildQueue)
        decrementActiveBuildSteps(bs->project());

    m_buildQueue.clear();
    m_configurations.clear();
    m_running = false;
    m_previousBuildStepProject = 0;

    m_progressFutureInterface->reportCanceled();
    m_progressFutureInterface->reportFinished();
    delete m_progressFutureInterface;
    m_progressFutureInterface = 0;
    m_maxProgress = 0;

    emit buildQueueFinished(false);
}


void BuildManager::toggleOutputWindow()
{
    m_outputWindow->toggle(false);
}

void BuildManager::showTaskWindow()
{
    m_taskWindow->popup(false);
}

void BuildManager::toggleTaskWindow()
{
    m_taskWindow->toggle(false);
}

bool BuildManager::tasksAvailable() const
{
    return m_taskWindow->numberOfTasks() > 0;
}

void BuildManager::gotoTaskWindow()
{
    m_taskWindow->popup(true);
}

void BuildManager::startBuildQueue()
{
    if (!m_running) {
        // Progress Reporting
        Core::ProgressManager *progressManager = Core::ICore::instance()->progressManager();
        m_progressFutureInterface = new QFutureInterface<void>;
        m_progressWatcher.setFuture(m_progressFutureInterface->future());
        Core::FutureProgress *progress = progressManager->addTask(m_progressFutureInterface->future(),
                                                                  tr("Build"),
                                                                  Constants::TASK_BUILD);
        connect(progress, SIGNAL(clicked()), this, SLOT(showBuildResults()));
        progress->setWidget(new BuildProgress(m_taskWindow));
        m_progress = 0;
        m_progressFutureInterface->setProgressRange(0, m_maxProgress);

        m_running = true;
        m_canceling = false;
        m_progressFutureInterface->reportStarted();
        m_outputWindow->clearContents();
        m_taskWindow->clearContents();
        nextStep();
    } else {
        // Already running
        m_progressFutureInterface->setProgressRange(0, m_maxProgress);
        const QString &progressText = tr("Finished %1 of %2 build steps").arg(m_progress).arg(m_maxProgress);
        m_progressFutureInterface->setProgressValueAndText(m_progress, progressText);
    }
}

void BuildManager::showBuildResults()
{
    if (m_taskWindow->numberOfTasks() != 0)
        toggleTaskWindow();
    else
        toggleOutputWindow();
    //toggleTaskWindow();
}

void BuildManager::addToTaskWindow(const QString &file, int type, int line, const QString &description)
{
    m_taskWindow->addItem(BuildParserInterface::PatternType(type), description, file, line);
}

void BuildManager::addToOutputWindow(const QString &string)
{
    m_outputWindow->appendText(string);
}

void BuildManager::nextBuildQueue()
{
    if (m_canceling)
        return;

    disconnect(m_currentBuildStep, SIGNAL(addToTaskWindow(QString, int, int, QString)),
                this, SLOT(addToTaskWindow(QString, int, int, QString)));
    disconnect(m_currentBuildStep, SIGNAL(addToOutputWindow(QString)),
               this, SLOT(addToOutputWindow(QString)));

    ++m_progress;
    const QString &progressText = tr("Finished %1 of %2 build steps").arg(m_progress).arg(m_maxProgress);
    m_progressFutureInterface->setProgressValueAndText(m_progress, progressText);

    bool result = m_watcher.result();
    if (!result) {
        // Build Failure
        addToOutputWindow(tr("<font color=\"#ff0000\">Error while building project %1</font>").arg(m_currentBuildStep->project()->name()));
        addToOutputWindow(tr("<font color=\"#ff0000\">When executing build step '%1'</font>").arg(m_currentBuildStep->displayName()));
        // NBS TODO fix in qtconcurrent
        m_progressFutureInterface->setProgressValueAndText(m_progress, tr("Error while building project %1").arg(m_currentBuildStep->project()->name()));
    }

    decrementActiveBuildSteps(m_currentBuildStep->project());
    if (result)
        nextStep();
    else
        clearBuildQueue();
}

void BuildManager::nextStep()
{
    if (!m_buildQueue.empty()) {
        m_currentBuildStep = m_buildQueue.front();
        m_currentConfiguration = m_configurations.front();
        m_buildQueue.pop_front();
        m_configurations.pop_front();

        connect(m_currentBuildStep, SIGNAL(addToTaskWindow(QString, int, int, QString)),
                this, SLOT(addToTaskWindow(QString, int, int, QString)));
        connect(m_currentBuildStep, SIGNAL(addToOutputWindow(QString)),
                this, SLOT(addToOutputWindow(QString)));

        bool init = m_currentBuildStep->init(m_currentConfiguration);
        if (!init) {
            addToOutputWindow(tr("<font color=\"#ff0000\">Error while building project %1</font>").arg(m_currentBuildStep->project()->name()));
            addToOutputWindow(tr("<font color=\"#ff0000\">When executing build step '%1'</font>").arg(m_currentBuildStep->displayName()));
            cancel();
            return;
        }

        if (m_currentBuildStep->project() != m_previousBuildStepProject) {
            const QString projectName = m_currentBuildStep->project()->name();
            addToOutputWindow(tr("<b>Running build steps for project %2...</b>")
                              .arg(projectName));
            m_previousBuildStepProject = m_currentBuildStep->project();
        }
        m_watcher.setFuture(QtConcurrent::run(&BuildStep::run, m_currentBuildStep));
    } else {
        m_running = false;
        m_previousBuildStepProject = 0;
        m_progressFutureInterface->reportFinished();
        delete m_progressFutureInterface;
        m_progressFutureInterface = 0;
        m_maxProgress = 0;
        emit buildQueueFinished(true);
    }
}

void BuildManager::buildQueueAppend(BuildStep * bs, const QString &configuration)
{
    m_buildQueue.append(bs);
    ++m_maxProgress;
    incrementActiveBuildSteps(bs->project());
    m_configurations.append(configuration);
}

void BuildManager::buildProjects(const QList<Project *> &projects, const QList<QString> &configurations)
{
    Q_ASSERT(projects.count() == configurations.count());
    QList<QString>::const_iterator cit = configurations.constBegin();
    QList<Project *>::const_iterator it, end;
    end = projects.constEnd();

    for (it = projects.constBegin(); it != end; ++it, ++cit) {
        QList<BuildStep *> buildSteps = (*it)->buildSteps();
        foreach (BuildStep *bs, buildSteps) {
            buildQueueAppend(bs, *cit);
        }
    }
    startBuildQueue();
}

void BuildManager::cleanProjects(const QList<Project *> &projects, const QList<QString> &configurations)
{
    Q_ASSERT(projects.count() == configurations.count());
    QList<QString>::const_iterator cit = configurations.constBegin();
    QList<Project *>::const_iterator it, end;
    end = projects.constEnd();

    for (it = projects.constBegin(); it != end; ++it, ++cit) {
        QList<BuildStep *> cleanSteps = (*it)->cleanSteps();
        foreach (BuildStep *bs, cleanSteps) {
            buildQueueAppend(bs, *cit);
        }
    }
    startBuildQueue();
}

void BuildManager::buildProject(Project *p, const QString &configuration)
{
    buildProjects(QList<Project *>() << p, QList<QString>() << configuration);
}

void BuildManager::cleanProject(Project *p, const QString &configuration)
{
    cleanProjects(QList<Project *>() << p, QList<QString>() << configuration);
}

void BuildManager::appendStep(BuildStep *step, const QString &configuration)
{
    buildQueueAppend(step, configuration);
    startBuildQueue();
}

bool BuildManager::isBuilding(Project *pro)
{
    QHash<Project *, int>::iterator it = m_activeBuildSteps.find(pro);
    QHash<Project *, int>::iterator end = m_activeBuildSteps.end();
    if (it == end || *it == 0)
        return false;
    else
        return true;
}

void BuildManager::incrementActiveBuildSteps(Project *pro)
{
    QHash<Project *, int>::iterator it = m_activeBuildSteps.find(pro);
    QHash<Project *, int>::iterator end = m_activeBuildSteps.end();
    if (it == end) {
        m_activeBuildSteps.insert(pro, 1);
        emit buildStateChanged(pro);
    } else if (*it == 0) {
        ++*it;
        emit buildStateChanged(pro);
    } else {
        ++*it;
    }
}

void BuildManager::decrementActiveBuildSteps(Project *pro)
{
    QHash<Project *, int>::iterator it = m_activeBuildSteps.find(pro);
    QHash<Project *, int>::iterator end = m_activeBuildSteps.end();
    if (it == end) {
        Q_ASSERT(false && "BuildManager m_activeBuildSteps says project is not building, but apparently a build step was still in the queue.");
    } else if (*it == 1) {
        --*it;
        emit buildStateChanged(pro);
    } else {
        --*it;
    }
}
