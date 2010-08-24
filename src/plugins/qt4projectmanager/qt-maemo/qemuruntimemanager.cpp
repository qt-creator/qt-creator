/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "qemuruntimemanager.h"

#include "maemorunconfiguration.h"
#include "maemotoolchain.h"
#include "qtversionmanager.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QtCore/QDir>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QStringBuilder>
#include <QtCore/QTextStream>

#include <QtGui/QMessageBox>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

QemuRuntimeManager *QemuRuntimeManager::m_instance = 0;

const QSize iconSize = QSize(24, 20);
const QLatin1String binQmake("/bin/qmake" EXEC_SUFFIX);

QemuRuntimeManager::QemuRuntimeManager(QObject *parent)
    : QObject(parent)
    , m_qemuAction(0)
    , m_qemuProcess(new QProcess(this))
    , m_runningQtId(-1)
    , m_needsSetup(true)
    , m_userTerminated(false)
{
    Q_ASSERT(!m_instance);
    m_instance = this;

    m_qemuStarterIcon.addFile(":/qt-maemo/images/qemu-run.png", iconSize);
    m_qemuStarterIcon.addFile(":/qt-maemo/images/qemu-stop.png", iconSize,
        QIcon::Normal, QIcon::On);

    m_qemuAction = new QAction("Maemo Emulator", this);
    m_qemuAction->setEnabled(false);
    m_qemuAction->setVisible(false);
    m_qemuAction->setIcon(m_qemuStarterIcon.pixmap(iconSize));
    m_qemuAction->setToolTip(tr("Start Maemo Emulator"));
    connect(m_qemuAction, SIGNAL(triggered()), this, SLOT(startRuntime()));

    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *actionManager = core->actionManager();
    Core::Command *qemuCommand = actionManager->registerAction(m_qemuAction,
        "MaemoEmulator", QList<int>() << Core::Constants::C_GLOBAL_ID);
    qemuCommand->setAttribute(Core::Command::CA_UpdateText);
    qemuCommand->setAttribute(Core::Command::CA_UpdateIcon);

    Core::ModeManager *modeManager = core->modeManager();
    modeManager->addAction(qemuCommand, 1);

    // listen to qt version changes to update the start button
    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>)),
        this, SLOT(qtVersionsChanged(QList<int>)));

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
    connect(m_qemuProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this,
        SLOT(qemuProcessFinished()));
    connect(m_qemuProcess, SIGNAL(readyReadStandardOutput()), this,
        SLOT(qemuOutput()));
    connect(m_qemuProcess, SIGNAL(readyReadStandardError()), this,
        SLOT(qemuOutput()));
    connect(this, SIGNAL(qemuProcessStatus(QemuStatus, QString)),
        this, SLOT(qemuStatusChanged(QemuStatus, QString)));
}

QemuRuntimeManager::~QemuRuntimeManager()
{
    terminateRuntime();
}

QemuRuntimeManager &QemuRuntimeManager::instance()
{
    Q_ASSERT(m_instance);
    return *m_instance;
}

bool QemuRuntimeManager::runtimeForQtVersion(int uniqueId, Runtime *rt) const
{
    bool found = m_runtimes.contains(uniqueId);
    if (found)
        *rt = m_runtimes.value(uniqueId);
    return found;
}

void QemuRuntimeManager::qtVersionsChanged(const QList<int> &uniqueIds)
{
    if (m_needsSetup)
        setupRuntimes();

    QtVersionManager *manager = QtVersionManager::instance();
    foreach (int uniqueId, uniqueIds) {
        if (manager->isValidId(uniqueId)) {
            const QString &qmake = manager->version(uniqueId)->qmakeCommand();
            const QString &runtimeRoot = runtimeForQtVersion(qmake);
            if (runtimeRoot.isEmpty() || !QFile::exists(runtimeRoot)) {
                // no runtime available, or runtime needs to be installed
                m_runtimes.remove(uniqueId);
            } else {
                // valid maemo qt version, also has a runtime installed
                Runtime runtime(runtimeRoot);
                fillRuntimeInformation(&runtime);
                m_runtimes.insert(uniqueId, runtime);
            }
        } else {
            // this qt version has been removed from the settings
            m_runtimes.remove(uniqueId);
            if (uniqueId == m_runningQtId) {
                terminateRuntime();
                emit qemuProcessStatus(QemuUserReason, tr("Qemu has been shut "
                    "down, because you removed the corresponding Qt version."));
            }
        }
    }

    // make visible only if we have a runtime and a maemo target
    m_qemuAction->setVisible(!m_runtimes.isEmpty() && sessionHasMaemoTarget());
}

void QemuRuntimeManager::projectAdded(ProjectExplorer::Project *project)
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
    m_qemuAction->setVisible(!m_runtimes.isEmpty() && sessionHasMaemoTarget());
}

void QemuRuntimeManager::projectRemoved(ProjectExplorer::Project *project)
{
    disconnect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)), this,
        SLOT(targetAdded(ProjectExplorer::Target*)));
    disconnect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)), this,
        SLOT(targetRemoved(ProjectExplorer::Target*)));
    disconnect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
        this, SLOT(targetChanged(ProjectExplorer::Target*)));

    foreach (Target *target, project->targets())
        targetRemoved(target);
    m_qemuAction->setVisible(!m_runtimes.isEmpty() && sessionHasMaemoTarget());
}

void QemuRuntimeManager::projectChanged(ProjectExplorer::Project *project)
{
    if (project)
        toggleStarterButton(project->activeTarget());
}

bool targetIsMaemo(const QString &id)
{
    return id == QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID);
}

void QemuRuntimeManager::targetAdded(ProjectExplorer::Target *target)
{
    if (!target || !targetIsMaemo(target->id()))
        return;

    // handle all run configuration changes, add, remove, etc...
    connect(target, SIGNAL(addedRunConfiguration(ProjectExplorer::RunConfiguration*)),
        this, SLOT(runConfigurationAdded(ProjectExplorer::RunConfiguration*)));
    connect(target, SIGNAL(removedRunConfiguration(ProjectExplorer::RunConfiguration*)),
        this, SLOT(runConfigurationRemoved(ProjectExplorer::RunConfiguration*)));
    connect(target, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
        this, SLOT(runConfigurationChanged(ProjectExplorer::RunConfiguration*)));

    // handle all build configuration changes, add, remove, etc...
    connect(target, SIGNAL(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
        this, SLOT(buildConfigurationAdded(ProjectExplorer::BuildConfiguration*)));
    connect(target, SIGNAL(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
        this, SLOT(buildConfigurationRemoved(ProjectExplorer::BuildConfiguration*)));
    connect(target, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
        this, SLOT(buildConfigurationChanged(ProjectExplorer::BuildConfiguration*)));

    // handle the qt version changes the build configuration uses
    connect(target, SIGNAL(environmentChanged()), this, SLOT(environmentChanged()));

    foreach (RunConfiguration *rc, target->runConfigurations())
        toggleDeviceConnections(qobject_cast<MaemoRunConfiguration*> (rc), true);
    m_qemuAction->setVisible(!m_runtimes.isEmpty() && sessionHasMaemoTarget());
}

void QemuRuntimeManager::targetRemoved(ProjectExplorer::Target *target)
{
    if (!target || !targetIsMaemo(target->id()))
        return;

    disconnect(target, SIGNAL(addedRunConfiguration(ProjectExplorer::RunConfiguration*)),
        this, SLOT(runConfigurationAdded(ProjectExplorer::RunConfiguration*)));
    disconnect(target, SIGNAL(removedRunConfiguration(ProjectExplorer::RunConfiguration*)),
        this, SLOT(runConfigurationRemoved(ProjectExplorer::RunConfiguration*)));
    disconnect(target, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
        this, SLOT(runConfigurationChanged(ProjectExplorer::RunConfiguration*)));

    disconnect(target, SIGNAL(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
        this, SLOT(buildConfigurationAdded(ProjectExplorer::BuildConfiguration*)));
    disconnect(target, SIGNAL(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
        this, SLOT(buildConfigurationRemoved(ProjectExplorer::BuildConfiguration*)));
    disconnect(target, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
        this, SLOT(buildConfigurationChanged(ProjectExplorer::BuildConfiguration*)));

    disconnect(target, SIGNAL(environmentChanged()), this, SLOT(environmentChanged()));

    foreach (RunConfiguration *rc, target->runConfigurations())
        toggleDeviceConnections(qobject_cast<MaemoRunConfiguration*> (rc), false);
    m_qemuAction->setVisible(!m_runtimes.isEmpty() && sessionHasMaemoTarget());
}

void QemuRuntimeManager::targetChanged(ProjectExplorer::Target *target)
{
    if (target)
        toggleStarterButton(target);
}

void QemuRuntimeManager::runConfigurationAdded(ProjectExplorer::RunConfiguration *rc)
{
    if (!rc || !targetIsMaemo(rc->target()->id()))
        return;
    toggleDeviceConnections(qobject_cast<MaemoRunConfiguration*> (rc), true);
}

void QemuRuntimeManager::runConfigurationRemoved(ProjectExplorer::RunConfiguration *rc)
{
    if (!rc || rc->target()->id() != QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID))
        return;
    toggleDeviceConnections(qobject_cast<MaemoRunConfiguration*> (rc), false);
}

void QemuRuntimeManager::runConfigurationChanged(ProjectExplorer::RunConfiguration *rc)
{
    if (rc)
        m_qemuAction->setEnabled(targetUsesMatchingRuntimeConfig(rc->target()));
}

void QemuRuntimeManager::buildConfigurationAdded(ProjectExplorer::BuildConfiguration *bc)
{
    if (!bc || !targetIsMaemo(bc->target()->id()))
        return;

    connect(bc, SIGNAL(environmentChanged()), this, SLOT(environmentChanged()));
}

void QemuRuntimeManager::buildConfigurationRemoved(ProjectExplorer::BuildConfiguration *bc)
{
    if (!bc || !targetIsMaemo(bc->target()->id()))
        return;

    disconnect(bc, SIGNAL(environmentChanged()), this, SLOT(environmentChanged()));
}

void QemuRuntimeManager::buildConfigurationChanged(ProjectExplorer::BuildConfiguration *bc)
{
    if (bc)
        toggleStarterButton(bc->target());
}

void QemuRuntimeManager::environmentChanged()
{
    // likely to happen when the qt version changes the build config is using
    if (ProjectExplorerPlugin *explorer = ProjectExplorerPlugin::instance()) {
        if (Project *project = explorer->session()->startupProject())
            toggleStarterButton(project->activeTarget());
    }
}

void QemuRuntimeManager::deviceConfigurationChanged(ProjectExplorer::Target *target)
{
    m_qemuAction->setEnabled(targetUsesMatchingRuntimeConfig(target));
}

void QemuRuntimeManager::startRuntime()
{
    m_userTerminated = false;
    Project *p = ProjectExplorerPlugin::instance()->session()->startupProject();
    if (!p)
        return;
    QtVersion *version;
    if (!targetUsesMatchingRuntimeConfig(p->activeTarget(), &version)) {
        qWarning("Strange: Qemu button was enabled, but target does not match.");
        return;
    }

    m_runningQtId = version->uniqueId();
    const QString root
        = QDir::toNativeSeparators(maddeRoot(version->qmakeCommand())
            + QLatin1Char('/'));
    const Runtime rt = m_runtimes.value(version->uniqueId());
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
#ifdef Q_OS_WIN
    const QLatin1Char colon(';');
    const QLatin1String key("PATH");
    env.insert(key, env.value(key) % colon % root % QLatin1String("bin"));
    env.insert(key, env.value(key) % colon % root % QLatin1String("madlib"));
#elif defined(Q_OS_UNIX)
#  if defined(Q_OS_MAC)
    const QLatin1String key("DYLD_LIBRARY_PATH");
#  else
    const QLatin1String key("LD_LIBRARY_PATH");
#  endif // MAC
    env.insert(key, env.value(key) % QLatin1Char(':') % rt.m_libPath);
#endif // WIN/UNIX
    m_qemuProcess->setProcessEnvironment(env);
    m_qemuProcess->setWorkingDirectory(rt.m_root);

    // This is complex because of extreme MADDE weirdness.
    const bool pathIsRelative = QFileInfo(rt.m_bin).isRelative();
    const QString app =
#ifdef Q_OS_WIN
        root % (pathIsRelative
            ? QLatin1String("madlib/") % rt.m_bin // Fremantle.
            : rt.m_bin)                           // Haramattan.
            % QLatin1String(".exe");
#else
        pathIsRelative
            ? root % QLatin1String("madlib/") % rt.m_bin // Fremantle.
            : rt.m_bin;                                  // Haramattan.
#endif

    m_qemuProcess->start(app % QLatin1Char(' ') % rt.m_args,
        QIODevice::ReadWrite);
    if (!m_qemuProcess->waitForStarted())
        return;

    emit qemuProcessStatus(QemuStarting);
    connect(m_qemuAction, SIGNAL(triggered()), this, SLOT(terminateRuntime()));
    disconnect(m_qemuAction, SIGNAL(triggered()), this, SLOT(startRuntime()));
}

void QemuRuntimeManager::terminateRuntime()
{
    m_userTerminated = true;

    if (m_qemuProcess->state() != QProcess::NotRunning) {
        m_qemuProcess->terminate();
        m_qemuProcess->kill();
    }

    connect(m_qemuAction, SIGNAL(triggered()), this, SLOT(startRuntime()));
    disconnect(m_qemuAction, SIGNAL(triggered()), this, SLOT(terminateRuntime()));
}

void QemuRuntimeManager::qemuProcessFinished()
{
    m_runningQtId = -1;
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

void QemuRuntimeManager::qemuProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
        emit qemuProcessStatus(QemuFailedToStart, m_qemuProcess->errorString());
}

void QemuRuntimeManager::qemuStatusChanged(QemuStatus status, const QString &error)
{
    QString message;
    bool running = false;

    switch (status) {
        case QemuStarting:
            running = true;
            break;
        case QemuFailedToStart:
            message = tr("Qemu failed to start: %1").arg(error);
            break;
        case QemuCrashed:
            message = tr("Qemu crashed");
            break;
        case QemuFinished:
            message = error;
            break;
        case QemuUserReason:
            message = error;
            break;
        default:
            Q_ASSERT(!"Missing handling of Qemu status");
    }

    if (!message.isEmpty())
        QMessageBox::warning(0, tr("Qemu error"), message);
    updateStarterIcon(running);
}

void QemuRuntimeManager::qemuOutput()
{
    qDebug("%s", m_qemuProcess->readAllStandardOutput().data());
    qDebug("%s", m_qemuProcess->readAllStandardError().data());
}

// -- private

void QemuRuntimeManager::setupRuntimes()
{
    m_needsSetup = false;

    const QList<QtVersion*> &versions = QtVersionManager::instance()
        ->versionsForTargetId(Constants::MAEMO_DEVICE_TARGET_ID);

    QList<int> uniqueIds;
    foreach (QtVersion *version, versions)
        uniqueIds.append(version->uniqueId());
    qtVersionsChanged(uniqueIds);
}

void QemuRuntimeManager::updateStarterIcon(bool running)
{
    QIcon::State state;
    QString toolTip;
    if (running) {
        state = QIcon::On;
        toolTip = tr("Stop Maemo Emulator");
    } else {
        state = QIcon::Off;
        toolTip = tr("Start Maemo Emulator");
    }

    m_qemuAction->setToolTip(toolTip);
    m_qemuAction->setIcon(m_qemuStarterIcon.pixmap(iconSize, QIcon::Normal,
        state));
}

void QemuRuntimeManager::toggleStarterButton(Target *target)
{
    if (m_needsSetup)
        setupRuntimes();

    int uniqueId = -1;
    if (target) {
        if (Qt4Target *qt4Target = qobject_cast<Qt4Target*>(target)) {
            if (Qt4BuildConfiguration *bc = qt4Target->activeBuildConfiguration()) {
                if (QtVersion *version = bc->qtVersion())
                    uniqueId = version->uniqueId();
            }
        }
    }

    bool isRunning = m_qemuProcess->state() != QProcess::NotRunning;
    if (m_runningQtId == uniqueId)
        isRunning = false;

    m_qemuAction->setEnabled(m_runtimes.contains(uniqueId)
        && targetUsesMatchingRuntimeConfig(target) && !isRunning);
}

bool QemuRuntimeManager::sessionHasMaemoTarget() const
{
    bool result = false;
    ProjectExplorerPlugin *explorer = ProjectExplorerPlugin::instance();
    const QList<Project*> &projects = explorer->session()->projects();
    foreach (const Project *p, projects)
        result |= p->target(QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID)) != 0;
    return result;
}

bool QemuRuntimeManager::targetUsesMatchingRuntimeConfig(Target *target,
    QtVersion **qtVersion)
{
    if (!target)
        return false;

    MaemoRunConfiguration *mrc =
        qobject_cast<MaemoRunConfiguration *> (target->activeRunConfiguration());
    if (!mrc)
        return false;
    Qt4BuildConfiguration *bc
        = qobject_cast<Qt4BuildConfiguration *>(target->activeBuildConfiguration());
    if (!bc)
        return false;
    QtVersion *version = bc->qtVersion();
    if (!version || !m_runtimes.contains(version->uniqueId()))
        return false;

    if (qtVersion)
        *qtVersion = version;
    const MaemoDeviceConfig &config = mrc->deviceConfig();
    return config.isValid() && config.type == MaemoDeviceConfig::Simulator;
}

QString QemuRuntimeManager::maddeRoot(const QString &qmake) const
{
    QDir dir(QDir::cleanPath(qmake).remove(binQmake));
    dir.cdUp(); dir.cdUp();
    return dir.absolutePath();
}

QString QemuRuntimeManager::targetRoot(const QString &qmake) const
{
    const QString target = QDir::cleanPath(qmake).remove(binQmake);
    return target.mid(target.lastIndexOf(QLatin1Char('/')) + 1);
}

bool QemuRuntimeManager::fillRuntimeInformation(Runtime *runtime) const
{
    const QStringList files = QDir(runtime->m_root).entryList(QDir::Files
        | QDir::NoSymLinks | QDir::NoDotAndDotDot);

    const QLatin1String infoFile("information");
    // we need at least the information file and a second one, most likely
    // the image file qemu is going to load
    if (files.contains(infoFile) && files.count() > 1) {
        QFile file(runtime->m_root + QLatin1Char('/') + infoFile);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMap<QString, QString> map;
            QTextStream stream(&file);
            while (!stream.atEnd()) {
                const QString &line = stream.readLine().trimmed();
                const int index = line.indexOf(QLatin1Char('='));
                map.insert(line.mid(0, index).remove(QLatin1Char('\'')),
                    line.mid(index + 1).remove(QLatin1Char('\'')));
            }

            runtime->m_bin = map.value(QLatin1String("qemu"));
            runtime->m_args = map.value(QLatin1String("qemu_args"));
            const QString &libPathSpec = map.value(QLatin1String("libpath"));
            runtime->m_libPath =
                libPathSpec.mid(libPathSpec.indexOf(QLatin1Char('=')) + 1);
            runtime->m_sshPort = map.value(QLatin1String("sshport"));
            runtime->m_gdbServerPort = map.value(QLatin1String("redirport2"));
            return true;
        }
    }
    return false;
}

QString QemuRuntimeManager::runtimeForQtVersion(const QString &qmakeCommand) const
{
    const QString &target = targetRoot(qmakeCommand);
    const QString &madRoot = maddeRoot(qmakeCommand);

    QFile file(madRoot + QLatin1String("/cache/madde.conf"));
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            if (!line.startsWith(QLatin1String("target")))
                continue;

            const QStringList &list = line.split(QLatin1Char(' '));
            if (list.count() <= 1 || list.at(1) != target)
                continue;

            line = stream.readLine().trimmed();
            while (!stream.atEnd() && line != QLatin1String("end")) {
                if (line.startsWith(QLatin1String("runtime"))) {
                    const QStringList &list = line.split(QLatin1Char(' '));
                    if (list.count() > 1) {
                        return madRoot
                            + QLatin1String("/runtimes/") + list.at(1).trimmed();
                    }
                    break;
                }
                line = stream.readLine().trimmed();
            }
        }
    }
    return QString();
}

void QemuRuntimeManager::toggleDeviceConnections(MaemoRunConfiguration *mrc,
    bool _connect)
{
    if (!mrc)
        return;

    if (_connect) { // handle device configuration changes
        connect(mrc, SIGNAL(deviceConfigurationChanged(ProjectExplorer::Target*)),
            this, SLOT(deviceConfigurationChanged(ProjectExplorer::Target*)));
        connect(mrc, SIGNAL(deviceConfigurationsUpdated(ProjectExplorer::Target*)),
            this, SLOT(deviceConfigurationChanged(ProjectExplorer::Target*)));
    } else {
        disconnect(mrc, SIGNAL(deviceConfigurationChanged(ProjectExplorer::Target*)),
            this, SLOT(deviceConfigurationChanged(ProjectExplorer::Target*)));
        disconnect(mrc, SIGNAL(deviceConfigurationsUpdated(ProjectExplorer::Target*)),
            this, SLOT(deviceConfigurationChanged(ProjectExplorer::Target*)));
    }
}
