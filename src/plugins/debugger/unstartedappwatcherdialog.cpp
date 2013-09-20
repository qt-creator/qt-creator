/****************************************************************************
**
** Copyright (C) 2014 Petar Perisin <petar.perisin@gmail.com>
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

#include "unstartedappwatcherdialog.h"

#include "debuggerdialogs.h"

#include <utils/pathchooser.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/localapplicationrunconfiguration.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QFormLayout>
#include <QLineEdit>
#include <QFileDialog>

namespace Debugger {
namespace Internal {

UnstartedAppWatcherDialogPrivate::UnstartedAppWatcherDialogPrivate(QWidget *dialog)
{
    dialog->setWindowTitle(UnstartedAppWatcherDialog::tr("Attach to process not yet started"));
    QVBoxLayout* mainLayout = new QVBoxLayout(dialog);

    kitChooser = new DebuggerKitChooser(DebuggerKitChooser::LocalDebugging, dialog);
    kitChooser->populate();
    kitChooser->setVisible(true);

    ProjectExplorer::Project *project = ProjectExplorer::ProjectExplorerPlugin::currentProject();
    if (project && project->activeTarget() && project->activeTarget()->kit())
        kitChooser->setCurrentKitId(project->activeTarget()->kit()->id());
    else if (ProjectExplorer::KitManager::defaultKit())
        kitChooser->setCurrentKitId(ProjectExplorer::KitManager::defaultKit()->id());

    pathChooser = new Utils::PathChooser(dialog);
    pathChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(new QLabel(UnstartedAppWatcherDialog::tr("Kit: "), dialog), kitChooser);
    formLayout->addRow(new QLabel(UnstartedAppWatcherDialog::tr("Executable: "), dialog), pathChooser);

    if (project && project->activeTarget() && project->activeTarget()->activeRunConfiguration()) {
        ProjectExplorer::LocalApplicationRunConfiguration *localAppRC =
                qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>
                             (project->activeTarget()->activeRunConfiguration());

        if (localAppRC)
            pathChooser->setPath(localAppRC->executable());
    }

    mainLayout->addItem(formLayout);
    hideOnAttachCheckBox = new QCheckBox(UnstartedAppWatcherDialog::tr("Hide after Attach"), dialog);
    hideOnAttachCheckBox->setToolTip(UnstartedAppWatcherDialog::tr("If checked, dialog will hide "
            "after attach, and reappear after application ends.\nAlternatively, dialog will close."));

    hideOnAttachCheckBox->setChecked(false);
    hideOnAttachCheckBox->setVisible(true);

    mainLayout->addWidget(hideOnAttachCheckBox);

    continueOnAttachCheckBox = new QCheckBox(UnstartedAppWatcherDialog::tr("Continue on Attach"), dialog);
    continueOnAttachCheckBox->setToolTip(UnstartedAppWatcherDialog::tr("If checked, debugger will "
                                         "not halt application after attach."));

    continueOnAttachCheckBox->setChecked(true);
    continueOnAttachCheckBox->setVisible(true);

    mainLayout->addWidget(continueOnAttachCheckBox);

    waitingLabel = new QLabel(QString(), dialog);
    waitingLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(waitingLabel);

    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    QHBoxLayout *buttonsLine = new QHBoxLayout();
    buttonsLine->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    watchingPushButton = new QPushButton(dialog);
    watchingPushButton->setCheckable(true);
    watchingPushButton->setChecked(false);
    watchingPushButton->setEnabled(false);
    watchingPushButton->setText(UnstartedAppWatcherDialog::tr("Start Watching"));

    buttonsLine->addWidget(watchingPushButton);

    closePushButton = new QPushButton(dialog);
    closePushButton->setText(UnstartedAppWatcherDialog::tr("Close"));

    buttonsLine->addWidget(closePushButton);

    mainLayout->addItem(buttonsLine);
    dialog->setLayout(mainLayout);
}

void UnstartedAppWatcherDialogPrivate::setWaitingState(UnstartedAppWacherState state)
{
    switch (state) {
    case InvalidWacherState:
        waitingLabel->setText(UnstartedAppWatcherDialog::tr("Please select valid executable."));
        watchingPushButton->setEnabled(false);
        watchingPushButton->setChecked(false);
        pathChooser->setEnabled(true);
        kitChooser->setEnabled(true);
        break;

    case NotWatchingState:
        waitingLabel->setText(UnstartedAppWatcherDialog::tr("Not Watching."));
        watchingPushButton->setEnabled(true);
        watchingPushButton->setChecked(false);
        pathChooser->setEnabled(true);
        kitChooser->setEnabled(true);
        break;

    case WatchingState:
        waitingLabel->setText(UnstartedAppWatcherDialog::tr("Waiting for process to start..."));
        watchingPushButton->setEnabled(true);
        watchingPushButton->setChecked(true);
        pathChooser->setEnabled(false);
        kitChooser->setEnabled(false);
        break;

    case FoundState:
        waitingLabel->setText(UnstartedAppWatcherDialog::tr("Attach"));
        watchingPushButton->setEnabled(false);
        watchingPushButton->setChecked(true);
        pathChooser->setEnabled(false);
        kitChooser->setEnabled(true);
        break;
    }
}

/*!
    \class Debugger::Internal::UnstartedAppWatcherDialog

    \brief The UnstartedAppWatcherDialog class provides ability to wait for a certain application
           to be started, after what it will attach to it.

    This dialog can be useful in cases where automated scripts are used in order to execute some
    tests on application. In those cases application will be started from a script. This dialog
    allows user to attach to application in those cases in very short time after they are started.

    In order to attach, user needs to provide appropriate kit (for local debugging) and
    application path.

    After selecting start, dialog will check if selected application is started every
    10 miliseconds. As soon as application is started, QtCreator will attach to it.

    After user attaches, it is possible to keep dialog active and as soon as debugging
    session ends, it will start watching again. This is because sometimes automated test
    scripts can restart application several times during tests.
*/

UnstartedAppWatcherDialog::UnstartedAppWatcherDialog(QWidget *parent) :
    QDialog(parent),
    d(new UnstartedAppWatcherDialogPrivate(this))
{
    connect(d->pathChooser, SIGNAL(beforeBrowsing()), this, SLOT(selectExecutable()));
    connect(d->watchingPushButton, SIGNAL(toggled(bool)), this, SLOT(startStopWatching(bool)));
    connect(d->pathChooser, SIGNAL(pathChanged(QString)), this, SLOT(stopAndCheckExecutable()));
    connect(d->closePushButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(findProcess()));
    d->setWaitingState(checkExecutableString() ? NotWatchingState : InvalidWacherState);
}

UnstartedAppWatcherDialog::~UnstartedAppWatcherDialog()
{
    delete d;
}

void UnstartedAppWatcherDialog::selectExecutable()
{
    QString path;

    ProjectExplorer::Project *project = ProjectExplorer::ProjectExplorerPlugin::currentProject();

    if (project && project->activeTarget() && project->activeTarget()->activeRunConfiguration()) {

        ProjectExplorer::LocalApplicationRunConfiguration *localAppRC =
                qobject_cast<ProjectExplorer::LocalApplicationRunConfiguration *>
                             (project->activeTarget()->activeRunConfiguration());
        if (localAppRC)
            path = QFileInfo(localAppRC->executable()).path();
    }

    if (path.isEmpty()) {
        if (project && project->activeTarget() &&
                project->activeTarget()->activeBuildConfiguration()) {
            path = project->activeTarget()->activeBuildConfiguration()->buildDirectory().toString();
        } else if (project) {
            path = project->projectDirectory();
        }
    }
    d->pathChooser->setInitialBrowsePathBackup(path);
}

void UnstartedAppWatcherDialog::startWatching()
{
    show();
    if (checkExecutableString()) {
        d->setWaitingState(WatchingState);
        startStopTimer(true);
    } else {
        d->setWaitingState(InvalidWacherState);
    }
}

void UnstartedAppWatcherDialog::pidFound(const ProjectExplorer::DeviceProcessItem &p)
{
    d->setWaitingState(FoundState);
    startStopTimer(false);
    m_process = p;

    if (hideOnAttach())
        hide();
    else
        accept();

    emit processFound();
}

void UnstartedAppWatcherDialog::startStopWatching(bool start)
{
    d->setWaitingState(start ? WatchingState : NotWatchingState);
    startStopTimer(start);
}

void UnstartedAppWatcherDialog::startStopTimer(bool start)
{
    if (start)
        m_timer.start(10);
    else
        m_timer.stop();
}

void UnstartedAppWatcherDialog::findProcess()
{
    QString appName = d->pathChooser->path();
    ProjectExplorer::DeviceProcessItem fallback;
    foreach (const ProjectExplorer::DeviceProcessItem &p,
             ProjectExplorer::DeviceProcessList::localProcesses()) {
        if (p.exe == appName) {
            pidFound(p);
            return;
        }
        if (p.cmdLine.startsWith(appName))
            fallback = p;
    }
    if (fallback.pid != 0)
        pidFound(fallback);
}

void UnstartedAppWatcherDialog::stopAndCheckExecutable()
{
    startStopTimer(false);
    d->setWaitingState(checkExecutableString() ? NotWatchingState : InvalidWacherState);
}

bool UnstartedAppWatcherDialog::checkExecutableString() const
{
    if (!d->pathChooser->path().isEmpty()) {
        QFileInfo fileInfo(d->pathChooser->path());
        return (fileInfo.exists() && fileInfo.isFile());
    }
    return false;
}

ProjectExplorer::Kit *UnstartedAppWatcherDialog::currentKit() const
{
    return d->kitChooser->currentKit();
}

ProjectExplorer::DeviceProcessItem UnstartedAppWatcherDialog::currentProcess() const
{
    return m_process;
}

bool UnstartedAppWatcherDialog::hideOnAttach() const
{
    return d->hideOnAttachCheckBox->isChecked();
}

bool UnstartedAppWatcherDialog::continueOnAttach() const
{
    return d->continueOnAttachCheckBox->isChecked();
}

} // namespace Internal
} // namespace Debugger
