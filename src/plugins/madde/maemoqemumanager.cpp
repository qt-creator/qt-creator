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

#include "maemoqemumanager.h"

#include "maemoglobal.h"
#include "maemoqemuruntimeparser.h"
#include "maemosettingspages.h"
#include "maemoqtversion.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/modemanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <remotelinux/remotelinuxrunconfiguration.h>
#include <utils/filesystemwatcher.h>

#include <QDebug>
#include <QDir>
#include <QList>
#include <QSet>
#include <QStringBuilder>

#include <QAction>
#include <QDesktopServices>
#include <QMessageBox>

#include <limits.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {

MaemoQemuManager *MaemoQemuManager::m_instance = 0;

const QSize iconSize = QSize(24, 20);

MaemoQemuManager::MaemoQemuManager(QObject *parent)
    : QObject(parent)
    , m_qemuAction(0)
    , m_qemuProcess(new QProcess(this))
    , m_runningQtId(INT_MIN)
    , m_userTerminated(false)
    , m_runtimeRootWatcher(0)
    , m_runtimeFolderWatcher(0)
{
    m_qemuStarterIcon.addFile(QLatin1String(":/qt-maemo/images/qemu-run.png"), iconSize);
    m_qemuStarterIcon.addFile(QLatin1String(":/qt-maemo/images/qemu-stop.png"), iconSize,
        QIcon::Normal, QIcon::On);

    m_qemuAction = new QAction(tr("MeeGo Emulator"), this);
    m_qemuAction->setIcon(m_qemuStarterIcon.pixmap(iconSize));
    m_qemuAction->setToolTip(tr("Start MeeGo Emulator"));
    connect(m_qemuAction, SIGNAL(triggered()), this, SLOT(startRuntime()));

    Core::Command *qemuCommand = Core::ActionManager::registerAction(m_qemuAction,
        "MaemoEmulator", Core::Context(Core::Constants::C_GLOBAL));
    qemuCommand->setAttribute(Core::Command::CA_UpdateText);
    qemuCommand->setAttribute(Core::Command::CA_UpdateIcon);

    Core::ModeManager::addAction(qemuCommand->action(), 1);
    m_qemuAction->setEnabled(false);
    m_qemuAction->setVisible(false);

    // listen to Qt version changes to update the start button
    connect(QtSupport::QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
        this, SLOT(qtVersionsChanged(QList<int>,QList<int>,QList<int>)));

    // listen to project add, remove and startup changes to udate start button
    SessionManager *session = ProjectExplorerPlugin::instance()->session();
    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)), this,
        SLOT(projectAdded(ProjectExplorer::Project*)));
    connect(session, SIGNAL(projectRemoved(ProjectExplorer::Project*)), this,
        SLOT(projectRemoved(ProjectExplorer::Project*)));
    connect(session, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
        this, SLOT(projectChanged(ProjectExplorer::Project*)));

    connect(m_qemuProcess, SIGNAL(error(QProcess::ProcessError)), this,
        SLOT(qemuProcessError(QProcess::ProcessError)));
    connect(m_qemuProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this,
        SLOT(qemuProcessFinished()));
    connect(m_qemuProcess, SIGNAL(readyReadStandardOutput()), this,
        SLOT(qemuOutput()));
    connect(m_qemuProcess, SIGNAL(readyReadStandardError()), this,
        SLOT(qemuOutput()));
    connect(this, SIGNAL(qemuProcessStatus(QemuStatus,QString)),
        this, SLOT(qemuStatusChanged(QemuStatus,QString)));
}

Utils::FileSystemWatcher *MaemoQemuManager::runtimeRootWatcher()
{
    if (!m_runtimeRootWatcher) {
        m_runtimeRootWatcher = new Utils::FileSystemWatcher(this);
        m_runtimeRootWatcher->setObjectName(QLatin1String("MaemoQemuRuntimeRootWatcher"));
        connect(m_runtimeRootWatcher, SIGNAL(directoryChanged(QString)), this,
            SLOT(runtimeRootChanged(QString)));
    }
    return m_runtimeRootWatcher;
}

Utils::FileSystemWatcher *MaemoQemuManager::runtimeFolderWatcher()
{
    if (!m_runtimeFolderWatcher) {
        m_runtimeFolderWatcher = new Utils::FileSystemWatcher(this);
        m_runtimeFolderWatcher->setObjectName(QLatin1String("MaemoQemuRuntimeFolderWatcher"));
        connect(m_runtimeFolderWatcher, SIGNAL(directoryChanged(QString)), this,
            SLOT(runtimeFolderChanged(QString)));
    }
    return m_runtimeFolderWatcher;
}

MaemoQemuManager::~MaemoQemuManager()
{
    terminateRuntime();
    m_instance = 0;
}

MaemoQemuManager &MaemoQemuManager::instance(QObject *parent)
{
    if (m_instance == 0)
        m_instance = new MaemoQemuManager(parent);
    return *m_instance;
}

bool MaemoQemuManager::runtimeForQtVersion(int uniqueId, MaemoQemuRuntime *rt) const
{
    *rt = m_runtimes.value(uniqueId, MaemoQemuRuntime());
    return rt->isValid();
}

bool MaemoQemuManager::qemuIsRunning() const
{
    return m_runningQtId != INT_MIN;
}

void MaemoQemuManager::qtVersionsChanged(const QList<int> &added, const QList<int> &removed, const QList<int> &changed)
{
    QList<int> uniqueIds;
    uniqueIds << added << removed << changed;
    QtSupport::QtVersionManager *manager = QtSupport::QtVersionManager::instance();
    foreach (int uniqueId, uniqueIds) {
        if (manager->isValidId(uniqueId)) {
            MaemoQtVersion *version = dynamic_cast<MaemoQtVersion *>(manager->version(uniqueId));

            if (version) {
                MaemoQemuRuntime runtime
                    = MaemoQemuRuntimeParser::parseRuntime(version);
                if (runtime.isValid()) {
                    m_runtimes.insert(uniqueId, runtime);
                    if (!runtimeRootWatcher()->watchesDirectory(runtime.m_watchPath))
                        runtimeRootWatcher()->addDirectory(runtime.m_watchPath,
                                                           Utils::FileSystemWatcher::WatchAllChanges);
                } else {
                    m_runtimes.remove(uniqueId);
                }
            }
        } else {
            // this Qt version has been removed from the settings
            m_runtimes.remove(uniqueId);
            if (uniqueId == m_runningQtId) {
                terminateRuntime();
                emit qemuProcessStatus(QemuUserReason, tr("Qemu has been shut "
                    "down, because you removed the corresponding Qt version."));
            }
        }
    }

    showOrHideQemuButton();
}

void MaemoQemuManager::projectAdded(ProjectExplorer::Project *project)
{
    // handle all target related changes, add, remove, etc...
    connect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)), this,
        SLOT(targetAdded(ProjectExplorer::Target*)));
    connect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)), this,
        SLOT(targetRemoved(ProjectExplorer::Target*)));
    connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
        this, SLOT(targetChanged(ProjectExplorer::Target*)));

    foreach (Target *target, project->targets())
        targetAdded(target);
}

void MaemoQemuManager::projectRemoved(ProjectExplorer::Project *project)
{
    disconnect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)), this,
        SLOT(targetAdded(ProjectExplorer::Target*)));
    disconnect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)), this,
        SLOT(targetRemoved(ProjectExplorer::Target*)));
    disconnect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
        this, SLOT(targetChanged(ProjectExplorer::Target*)));

    foreach (Target *target, project->targets())
        targetRemoved(target);
    showOrHideQemuButton();
}

void MaemoQemuManager::projectChanged(ProjectExplorer::Project *project)
{
    if (project) {
        toggleStarterButton(project->activeTarget());
        deviceConfigurationChanged(project->activeTarget());
    }
}

void MaemoQemuManager::targetAdded(ProjectExplorer::Target *target)
{
    if (!target || !MaemoGlobal::hasMaemoDevice(target->kit()))
        return;

    // handle the Qt version changes the build configuration uses
    connect(target, SIGNAL(environmentChanged()), this, SLOT(environmentChanged()));
    connect(target, SIGNAL(kitChanged()), this, SLOT(systemChanged()));

    toggleStarterButton(target);
}

void MaemoQemuManager::targetRemoved(ProjectExplorer::Target *target)
{
    if (!target || !MaemoGlobal::hasMaemoDevice(target->kit()))
        return;

    disconnect(target, SIGNAL(environmentChanged()), this, SLOT(environmentChanged()));
    disconnect(target, SIGNAL(kitChanged()), this, SLOT(systemChanged()));

    showOrHideQemuButton();
}

void MaemoQemuManager::targetChanged(ProjectExplorer::Target *target)
{
    if (target) {
        toggleStarterButton(target);
        deviceConfigurationChanged(target);
    }
}

void MaemoQemuManager::systemChanged()
{
    Target *t = qobject_cast<Target *>(sender());
    targetChanged(t);
}

void MaemoQemuManager::environmentChanged()
{
    // likely to happen when the Qt version changes the build config is using
    if (ProjectExplorerPlugin *explorer = ProjectExplorerPlugin::instance()) {
        if (Project *project = explorer->session()->startupProject())
            toggleStarterButton(project->activeTarget());
    }
}

void MaemoQemuManager::deviceConfigurationChanged(ProjectExplorer::Target *target)
{
    m_qemuAction->setEnabled(targetUsesMatchingRuntimeConfig(target));
}

void MaemoQemuManager::startRuntime()
{
    m_userTerminated = false;
    Project *p = ProjectExplorerPlugin::instance()->session()->startupProject();
    if (!p)
        return;
    QtSupport::BaseQtVersion *version;
    if (!targetUsesMatchingRuntimeConfig(p->activeTarget(), &version)) {
        qWarning("Strange: Qemu button was enabled, but target does not match.");
        return;
    }

    m_runningQtId = version->uniqueId();
    const MaemoQemuRuntime rt = m_runtimes.value(version->uniqueId());
    m_qemuProcess->setProcessEnvironment(rt.environment());
    m_qemuProcess->setWorkingDirectory(rt.m_root);
    m_qemuProcess->start(rt.m_bin % QLatin1Char(' ') % rt.m_args);
    if (!m_qemuProcess->waitForStarted())
        return;

    emit qemuProcessStatus(QemuStarting);
    connect(m_qemuAction, SIGNAL(triggered()), this, SLOT(terminateRuntime()));
    disconnect(m_qemuAction, SIGNAL(triggered()), this, SLOT(startRuntime()));
}

void MaemoQemuManager::terminateRuntime()
{
    m_userTerminated = true;

    if (m_qemuProcess->state() != QProcess::NotRunning) {
        m_qemuProcess->terminate();
        m_qemuProcess->kill();
    }

    connect(m_qemuAction, SIGNAL(triggered()), this, SLOT(startRuntime()));
    disconnect(m_qemuAction, SIGNAL(triggered()), this, SLOT(terminateRuntime()));
}

void MaemoQemuManager::qemuProcessFinished()
{
    m_runningQtId = INT_MIN;
    QemuStatus status = QemuFinished;
    QString error;

    if (!m_userTerminated) {
        if (m_qemuProcess->exitStatus() == QProcess::CrashExit) {
            status = QemuCrashed;
            error = m_qemuProcess->errorString();
        } else if (m_qemuProcess->exitCode() != 0) {
            error = tr("Qemu finished with error: Exit code was %1.")
                .arg(m_qemuProcess->exitCode());
        }
    }

    m_userTerminated = false;
    emit qemuProcessStatus(status, error);
}

void MaemoQemuManager::qemuProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
        emit qemuProcessStatus(QemuFailedToStart, m_qemuProcess->errorString());
}

void MaemoQemuManager::qemuStatusChanged(QemuStatus status, const QString &error)
{
    bool running = false;
    switch (status) {
        case QemuStarting:
            running = true;
            break;
        case QemuFailedToStart:
            QMessageBox::warning(0, tr("Qemu error"),
                tr("Qemu failed to start: %1"));
            break;
        case QemuCrashed:
            MaemoQemuSettingsPage::showQemuCrashDialog();
            break;
        case QemuFinished:
        case QemuUserReason:
            if (!error.isEmpty())
                QMessageBox::warning(0, tr("Qemu error"), error);
            break;
        default:
            Q_ASSERT(!"Missing handling of Qemu status");
    }

    updateStarterIcon(running);
}

void MaemoQemuManager::qemuOutput()
{
    qDebug("%s", m_qemuProcess->readAllStandardOutput().data());
    qDebug("%s", m_qemuProcess->readAllStandardError().data());
}

void MaemoQemuManager::runtimeRootChanged(const QString &directory)
{
    QList<int> uniqueIds;
    QMap<int, MaemoQemuRuntime>::const_iterator it;
    for (it = m_runtimes.constBegin(); it != m_runtimes.constEnd(); ++it) {
        if (QDir(it.value().m_watchPath) == QDir(directory))
            uniqueIds.append(it.key());
    }

    foreach (int uniqueId, uniqueIds) {
        MaemoQemuRuntime runtime = m_runtimes.value(uniqueId, MaemoQemuRuntime());
        if (runtime.isValid()) {
            if (QFile::exists(runtime.m_root)) {
                // nothing changed, so we can remove it
                uniqueIds.removeAll(uniqueId);
            }
        } else {
            if (QFile::exists(runtime.m_root)) {
                if (!QFile::exists(runtime.m_root + QLatin1String("/information"))) {
                    // install might be still in progress
                    uniqueIds.removeAll(uniqueId);
                    runtimeFolderWatcher()->addDirectory(runtime.m_root,
                                                         Utils::FileSystemWatcher::WatchAllChanges);
                }
            }
        }
    }
    notify(uniqueIds);
}

void MaemoQemuManager::runtimeFolderChanged(const QString &directory)
{
    if (QFile::exists(directory + QLatin1String("/information"))) {
        QList<int> uniqueIds;
        QMap<int, MaemoQemuRuntime>::const_iterator it;
        for (it = m_runtimes.constBegin(); it != m_runtimes.constEnd(); ++it) {
            if (QDir(it.value().m_root) == QDir(directory))
                uniqueIds.append(it.key());
        }
        notify(uniqueIds);
        if (m_runtimeFolderWatcher)
            m_runtimeFolderWatcher->removeDirectory(directory);
    }
}

// -- private

void MaemoQemuManager::updateStarterIcon(bool running)
{
    QIcon::State state;
    QString toolTip;
    if (running) {
        state = QIcon::On;
        toolTip = tr("Stop MeeGo Emulator");
    } else {
        state = QIcon::Off;
        toolTip = tr("Start MeeGo Emulator");
    }

    m_qemuAction->setToolTip(toolTip);
    m_qemuAction->setIcon(m_qemuStarterIcon.pixmap(iconSize, QIcon::Normal,
        state));
}

void MaemoQemuManager::toggleStarterButton(Target *target)
{
    int uniqueId = -1;
    if (target) {
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target->kit());
        if (version)
            uniqueId = version->uniqueId();
    }

    if (uniqueId >= 0 && (m_runtimes.isEmpty() || !m_runtimes.contains(uniqueId)))
        qtVersionsChanged(QList<int>(), QList<int>(), QList<int>() << uniqueId);

    bool isRunning = m_qemuProcess->state() != QProcess::NotRunning;
    if (m_runningQtId == uniqueId)
        isRunning = false;

    const Project * const p
        = ProjectExplorerPlugin::instance()->session()->startupProject();
    const bool qemuButtonEnabled
        = p && p->activeTarget() && MaemoGlobal::hasMaemoDevice(target->kit())
            && m_runtimes.value(uniqueId, MaemoQemuRuntime()).isValid()
            && targetUsesMatchingRuntimeConfig(target) && !isRunning;
    m_qemuAction->setEnabled(qemuButtonEnabled);
    showOrHideQemuButton();
}

bool MaemoQemuManager::sessionHasMaemoTarget() const
{
    ProjectExplorerPlugin *explorer = ProjectExplorerPlugin::instance();
    const QList<Project*> &projects = explorer->session()->projects();
    foreach (const Project *p, projects) {
        foreach (const Target * const target, p->targets()) {
            if (MaemoGlobal::hasMaemoDevice(target->kit()))
                return true;
        }
    }
    return false;
}

bool MaemoQemuManager::targetUsesMatchingRuntimeConfig(Target *target,
    QtSupport::BaseQtVersion **qtVersion)
{
    if (!target)
        return false;
    if (target != target->project()->activeTarget())
        return false;

    RemoteLinuxRunConfiguration *mrc =
        qobject_cast<RemoteLinuxRunConfiguration *> (target->activeRunConfiguration());
    if (!mrc)
        return false;

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(target->kit());
    if (!version || !m_runtimes.value(version->uniqueId(), MaemoQemuRuntime()).isValid())
        return false;

    if (qtVersion)
        *qtVersion = version;
    const IDevice::ConstPtr config = DeviceKitInformation::device(target->kit());
    return !config.isNull() && config->machineType() == IDevice::Emulator;
}

void MaemoQemuManager::notify(const QList<int> uniqueIds)
{
    qtVersionsChanged(QList<int>(), QList<int>(), uniqueIds);
    environmentChanged();   // to toggle the start button
}

void MaemoQemuManager::showOrHideQemuButton()
{
    const bool showButton = !m_runtimes.isEmpty() && sessionHasMaemoTarget();
    if (!showButton)
        terminateRuntime();
    m_qemuAction->setVisible(showButton);
}

}   // namespace Internal
}   // namespace Madde
