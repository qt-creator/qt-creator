/****************************************************************************
**
** Copyright (C) 2015 Petar Perisin <petar.perisin@gmail.com>
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

#include "unstartedappwatcherdialog.h"

#include "debuggeritem.h"
#include "debuggerdialogs.h"
#include "debuggerkitinformation.h"

#include <utils/pathchooser.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projecttree.h>
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

using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

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

UnstartedAppWatcherDialog::UnstartedAppWatcherDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Attach to Process Not Yet Started"));

    m_kitChooser = new DebuggerKitChooser(DebuggerKitChooser::LocalDebugging, this);
    m_kitChooser->populate();
    m_kitChooser->setVisible(true);

    Project *project = ProjectTree::currentProject();
    if (project && project->activeTarget() && project->activeTarget()->kit())
        m_kitChooser->setCurrentKitId(project->activeTarget()->kit()->id());
    else if (KitManager::defaultKit())
        m_kitChooser->setCurrentKitId(KitManager::defaultKit()->id());

    m_pathChooser = new Utils::PathChooser(this);
    m_pathChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_pathChooser->setHistoryCompleter(QLatin1String("LocalExecutable"));

    if (project && project->activeTarget() && project->activeTarget()->activeRunConfiguration()) {
        LocalApplicationRunConfiguration *localAppRC =
                qobject_cast<LocalApplicationRunConfiguration *>
                             (project->activeTarget()->activeRunConfiguration());

        if (localAppRC)
            m_pathChooser->setPath(localAppRC->executable());
    }

    m_hideOnAttachCheckBox = new QCheckBox(tr("Reopen dialog when application finishes"), this);
    m_hideOnAttachCheckBox->setToolTip(tr("Reopens this dialog when application finishes."));

    m_hideOnAttachCheckBox->setChecked(false);
    m_hideOnAttachCheckBox->setVisible(true);

    m_continueOnAttachCheckBox = new QCheckBox(tr("Continue on attach"), this);
    m_continueOnAttachCheckBox->setToolTip(tr("Debugger does not stop the"
                                              " application after attach."));

    m_continueOnAttachCheckBox->setChecked(true);
    m_continueOnAttachCheckBox->setVisible(true);

    m_waitingLabel = new QLabel(QString(), this);
    m_waitingLabel->setAlignment(Qt::AlignCenter);

    m_watchingPushButton = new QPushButton(this);
    m_watchingPushButton->setCheckable(true);
    m_watchingPushButton->setChecked(false);
    m_watchingPushButton->setEnabled(false);
    m_watchingPushButton->setText(tr("Start Watching"));

    m_closePushButton = new QPushButton(this);
    m_closePushButton->setText(tr("Close"));

    QHBoxLayout *buttonsLine = new QHBoxLayout();
    buttonsLine->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    buttonsLine->addWidget(m_watchingPushButton);
    buttonsLine->addWidget(m_closePushButton);

    QFormLayout *mainLayout = new QFormLayout(this);
    mainLayout->addRow(new QLabel(tr("Kit: "), this), m_kitChooser);
    mainLayout->addRow(new QLabel(tr("Executable: "), this), m_pathChooser);
    mainLayout->addRow(m_hideOnAttachCheckBox);
    mainLayout->addRow(m_continueOnAttachCheckBox);
    mainLayout->addRow(m_waitingLabel);
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    mainLayout->addRow(buttonsLine);
    setLayout(mainLayout);

    connect(m_pathChooser, &Utils::PathChooser::beforeBrowsing,
            this, &UnstartedAppWatcherDialog::selectExecutable);
    connect(m_watchingPushButton, &QAbstractButton::toggled,
            this, &UnstartedAppWatcherDialog::startStopWatching);
    connect(m_pathChooser, &Utils::PathChooser::pathChanged, this,
            &UnstartedAppWatcherDialog::stopAndCheckExecutable);
    connect(m_closePushButton, &QAbstractButton::clicked,
            this, &QDialog::reject);
    connect(&m_timer, &QTimer::timeout,
            this, &UnstartedAppWatcherDialog::findProcess);
    connect(m_kitChooser, &KitChooser::currentIndexChanged,
            this, &UnstartedAppWatcherDialog::kitChanged);
    kitChanged();

    setWaitingState(checkExecutableString() ? NotWatchingState : InvalidWacherState);
}

void UnstartedAppWatcherDialog::selectExecutable()
{
    QString path;

    Project *project = ProjectTree::currentProject();

    if (project && project->activeTarget() && project->activeTarget()->activeRunConfiguration()) {

        LocalApplicationRunConfiguration *localAppRC =
                qobject_cast<LocalApplicationRunConfiguration *>
                             (project->activeTarget()->activeRunConfiguration());
        if (localAppRC)
            path = QFileInfo(localAppRC->executable()).path();
    }

    if (path.isEmpty()) {
        if (project && project->activeTarget() &&
                project->activeTarget()->activeBuildConfiguration()) {
            path = project->activeTarget()->activeBuildConfiguration()->buildDirectory().toString();
        } else if (project) {
            path = project->projectDirectory().toString();
        }
    }
    m_pathChooser->setInitialBrowsePathBackup(path);
}

void UnstartedAppWatcherDialog::startWatching()
{
    show();
    if (checkExecutableString()) {
        setWaitingState(WatchingState);
        startStopTimer(true);
    } else {
        setWaitingState(InvalidWacherState);
    }
}

void UnstartedAppWatcherDialog::pidFound(const DeviceProcessItem &p)
{
    setWaitingState(FoundState);
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
    setWaitingState(start ? WatchingState : NotWatchingState);
    m_watchingPushButton->setText(start ? QLatin1String("Stop Watching")
                                        : QLatin1String("Start Watching"));
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
    const QString &appName = Utils::FileUtils::normalizePathName(m_pathChooser->path());
    DeviceProcessItem fallback;
    foreach (const DeviceProcessItem &p, DeviceProcessList::localProcesses()) {
        if (Utils::FileUtils::normalizePathName(p.exe) == appName) {
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
    setWaitingState(checkExecutableString() ? NotWatchingState : InvalidWacherState);
}

void UnstartedAppWatcherDialog::kitChanged()
{
    const DebuggerItem *debugger = DebuggerKitInformation::debugger(m_kitChooser->currentKit());
    if (!debugger)
        return;
    if (debugger->engineType() == Debugger::CdbEngineType) {
        m_continueOnAttachCheckBox->setEnabled(false);
        m_continueOnAttachCheckBox->setChecked(true);
    } else {
        m_continueOnAttachCheckBox->setEnabled(true);
    }
}

bool UnstartedAppWatcherDialog::checkExecutableString() const
{
    if (!m_pathChooser->path().isEmpty()) {
        QFileInfo fileInfo(m_pathChooser->path());
        return (fileInfo.exists() && fileInfo.isFile());
    }
    return false;
}

Kit *UnstartedAppWatcherDialog::currentKit() const
{
    return m_kitChooser->currentKit();
}

DeviceProcessItem UnstartedAppWatcherDialog::currentProcess() const
{
    return m_process;
}

bool UnstartedAppWatcherDialog::hideOnAttach() const
{
    return m_hideOnAttachCheckBox->isChecked();
}

bool UnstartedAppWatcherDialog::continueOnAttach() const
{
    return m_continueOnAttachCheckBox->isEnabled() && m_continueOnAttachCheckBox->isChecked();
}

void UnstartedAppWatcherDialog::setWaitingState(UnstartedAppWacherState state)
{
    switch (state) {
    case InvalidWacherState:
        m_waitingLabel->setText(tr("Select valid executable."));
        m_watchingPushButton->setEnabled(false);
        m_watchingPushButton->setChecked(false);
        m_pathChooser->setEnabled(true);
        m_kitChooser->setEnabled(true);
        break;

    case NotWatchingState:
        m_waitingLabel->setText(tr("Not watching."));
        m_watchingPushButton->setEnabled(true);
        m_watchingPushButton->setChecked(false);
        m_pathChooser->setEnabled(true);
        m_kitChooser->setEnabled(true);
        break;

    case WatchingState:
        m_waitingLabel->setText(tr("Waiting for process to start..."));
        m_watchingPushButton->setEnabled(true);
        m_watchingPushButton->setChecked(true);
        m_pathChooser->setEnabled(false);
        m_kitChooser->setEnabled(false);
        break;

    case FoundState:
        m_waitingLabel->setText(tr("Attach"));
        m_watchingPushButton->setEnabled(false);
        m_watchingPushButton->setChecked(true);
        m_pathChooser->setEnabled(false);
        m_kitChooser->setEnabled(true);
        break;
    }
}

} // namespace Internal
} // namespace Debugger
