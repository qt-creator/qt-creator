/****************************************************************************
**
** Copyright (C) 2016 Petar Perisin <petar.perisin@gmail.com>
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
#include <projectexplorer/runnables.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLabel>
#include <QFormLayout>
#include <QLineEdit>
#include <QFileDialog>

using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

static bool isLocal(RunConfiguration *runConfiguration)
{
    Target *target = runConfiguration ? runConfiguration->target() : 0;
    Kit *kit = target ? target->kit() : 0;
    return DeviceTypeKitInformation::deviceTypeId(kit) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
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

UnstartedAppWatcherDialog::UnstartedAppWatcherDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Attach to Process Not Yet Started"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_kitChooser = new DebuggerKitChooser(DebuggerKitChooser::LocalDebugging, this);
    m_kitChooser->populate();
    m_kitChooser->setVisible(true);

    Project *project = ProjectTree::currentProject();
    Target *activeTarget = project ? project->activeTarget() : 0;
    Kit *kit = activeTarget ? activeTarget->kit() : 0;

    if (kit)
        m_kitChooser->setCurrentKitId(kit->id());
    else if (KitManager::defaultKit())
        m_kitChooser->setCurrentKitId(KitManager::defaultKit()->id());

    auto pathLayout = new QHBoxLayout;
    m_pathChooser = new Utils::PathChooser(this);
    m_pathChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_pathChooser->setHistoryCompleter(QLatin1String("LocalExecutable"), true);
    m_pathChooser->setMinimumWidth(400);

    auto resetExecutable = new QPushButton(tr("Reset"));
    resetExecutable->setEnabled(false);
    pathLayout->addWidget(m_pathChooser);
    pathLayout->addWidget(resetExecutable);
    if (activeTarget) {
        if (RunConfiguration *runConfig = activeTarget->activeRunConfiguration()) {
            const Runnable runnable = runConfig->runnable();
            if (runnable.is<StandardRunnable>() && isLocal(runConfig)) {
                resetExecutable->setEnabled(true);
                connect(resetExecutable, &QPushButton::clicked,
                        this, [this, runnable]() {
                    m_pathChooser->setPath(runnable.as<StandardRunnable>().executable);
                });
            }
        }
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

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    m_watchingPushButton = buttonBox->addButton(tr("Start Watching"), QDialogButtonBox::ActionRole);
    m_watchingPushButton->setCheckable(true);
    m_watchingPushButton->setChecked(false);
    m_watchingPushButton->setEnabled(false);
    m_watchingPushButton->setDefault(true);

    QFormLayout *mainLayout = new QFormLayout(this);
    mainLayout->addRow(new QLabel(tr("Kit: "), this), m_kitChooser);
    mainLayout->addRow(new QLabel(tr("Executable: "), this), pathLayout);
    mainLayout->addRow(m_hideOnAttachCheckBox);
    mainLayout->addRow(m_continueOnAttachCheckBox);
    mainLayout->addRow(m_waitingLabel);
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    mainLayout->addRow(buttonBox);
    setLayout(mainLayout);

    connect(m_pathChooser, &Utils::PathChooser::beforeBrowsing,
            this, &UnstartedAppWatcherDialog::selectExecutable);
    connect(m_watchingPushButton, &QAbstractButton::toggled,
            this, &UnstartedAppWatcherDialog::startStopWatching);
    connect(m_pathChooser, &Utils::PathChooser::pathChanged, this,
            &UnstartedAppWatcherDialog::stopAndCheckExecutable);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(&m_timer, &QTimer::timeout,
            this, &UnstartedAppWatcherDialog::findProcess);
    connect(m_kitChooser, &KitChooser::currentIndexChanged,
            this, &UnstartedAppWatcherDialog::kitChanged);
    kitChanged();
    m_pathChooser->setFocus();

    setWaitingState(checkExecutableString() ? NotWatchingState : InvalidWacherState);
}

bool UnstartedAppWatcherDialog::event(QEvent *e)
{
    if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            ke->accept();
            return true;
        }
    }
    return QDialog::event(e);
}

void UnstartedAppWatcherDialog::selectExecutable()
{
    QString path;

    Project *project = ProjectTree::currentProject();
    Target *activeTarget = project ? project->activeTarget() : 0;

    if (activeTarget) {
        if (RunConfiguration *runConfig = activeTarget->activeRunConfiguration()) {
            const Runnable runnable = runConfig->runnable();
            if (runnable.is<StandardRunnable>() && isLocal(runConfig))
                path = QFileInfo(runnable.as<StandardRunnable>().executable).path();
        }
    }

    if (path.isEmpty()) {
        if (activeTarget && activeTarget->activeBuildConfiguration()) {
            path = activeTarget->activeBuildConfiguration()->buildDirectory().toString();
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
