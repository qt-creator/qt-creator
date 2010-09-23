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

#include "maemoqemumanager.h"

#include "maemorunconfiguration.h"
#include "maemotoolchain.h"
#include "qtversionmanager.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/modemanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QStringBuilder>
#include <QtCore/QTextStream>

#include <QtGui/QAction>
#include <QtGui/QDesktopServices>
#include <QtGui/QMessageBox>

#include <QtXml/QXmlStreamReader>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

MaemoQemuManager *MaemoQemuManager::m_instance = 0;

const QSize iconSize = QSize(24, 20);
const QLatin1String binQmake("/bin/qmake" EXEC_SUFFIX);

MaemoQemuManager::MaemoQemuManager(QObject *parent)
    : QObject(parent)
    , m_qemuAction(0)
    , m_qemuProcess(new QProcess(this))
    , m_runningQtId(-1)
    , m_needsSetup(true)
    , m_userTerminated(false)
{
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
        "MaemoEmulator", Core::Context(Core::Constants::C_GLOBAL));
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

    m_runtimeRootWatcher = new QFileSystemWatcher(this);
    connect(m_runtimeRootWatcher, SIGNAL(directoryChanged(QString)), this,
        SLOT(runtimeRootChanged(QString)));
    m_runtimeFolderWatcher = new QFileSystemWatcher(this);
    connect(m_runtimeFolderWatcher, SIGNAL(directoryChanged(QString)), this,
        SLOT(runtimeFolderChanged(QString)));
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

bool MaemoQemuManager::runtimeForQtVersion(int uniqueId, Runtime *rt) const
{
    *rt = m_runtimes.value(uniqueId, Runtime());
    return rt->isValid();
}

void MaemoQemuManager::qtVersionsChanged(const QList<int> &uniqueIds)
{
    if (m_needsSetup)
        setupRuntimes();

    QtVersionManager *manager = QtVersionManager::instance();
    foreach (int uniqueId, uniqueIds) {
        if (manager->isValidId(uniqueId)) {
            QtVersion *version = manager->version(uniqueId);
            if (version->supportsTargetId(Constants::MAEMO_DEVICE_TARGET_ID)) {
                const QString &qmake = version->qmakeCommand();
                const QString &runtimeRoot = runtimeForQtVersion(qmake);
                if (!runtimeRoot.isEmpty()) {
                    Runtime runtime(runtimeRoot);
                    if (QFile::exists(runtimeRoot))
                        fillRuntimeInformation(&runtime);
                    runtime.m_watchPath =
                        runtimeRoot.left(runtimeRoot.lastIndexOf(QLatin1Char('/')));
                    m_runtimes.insert(uniqueId, runtime);
                    if (!m_runtimeRootWatcher->directories().contains(runtime.m_watchPath))
                        m_runtimeRootWatcher->addPath(runtime.m_watchPath);
                } else {
                    m_runtimes.remove(uniqueId);
                }
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
    m_qemuAction->setVisible(!m_runtimes.isEmpty() && sessionHasMaemoTarget());
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
    m_qemuAction->setVisible(!m_runtimes.isEmpty() && sessionHasMaemoTarget());
}

void MaemoQemuManager::projectChanged(ProjectExplorer::Project *project)
{
    if (project) {
        toggleStarterButton(project->activeTarget());
        deviceConfigurationChanged(project->activeTarget());
    }
}

bool targetIsMaemo(const QString &id)
{
    return id == QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID);
}

void MaemoQemuManager::targetAdded(ProjectExplorer::Target *target)
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

void MaemoQemuManager::targetRemoved(ProjectExplorer::Target *target)
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

void MaemoQemuManager::targetChanged(ProjectExplorer::Target *target)
{
    if (target) {
        toggleStarterButton(target);
        deviceConfigurationChanged(target);
    }
}

void MaemoQemuManager::runConfigurationAdded(ProjectExplorer::RunConfiguration *rc)
{
    if (!rc || !targetIsMaemo(rc->target()->id()))
        return;
    toggleDeviceConnections(qobject_cast<MaemoRunConfiguration*> (rc), true);
}

void MaemoQemuManager::runConfigurationRemoved(ProjectExplorer::RunConfiguration *rc)
{
    if (!rc || rc->target()->id() != QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID))
        return;
    toggleDeviceConnections(qobject_cast<MaemoRunConfiguration*> (rc), false);
}

void MaemoQemuManager::runConfigurationChanged(ProjectExplorer::RunConfiguration *rc)
{
    if (rc)
        m_qemuAction->setEnabled(targetUsesMatchingRuntimeConfig(rc->target()));
}

void MaemoQemuManager::buildConfigurationAdded(ProjectExplorer::BuildConfiguration *bc)
{
    if (!bc || !targetIsMaemo(bc->target()->id()))
        return;

    connect(bc, SIGNAL(environmentChanged()), this, SLOT(environmentChanged()));
}

void MaemoQemuManager::buildConfigurationRemoved(ProjectExplorer::BuildConfiguration *bc)
{
    if (!bc || !targetIsMaemo(bc->target()->id()))
        return;

    disconnect(bc, SIGNAL(environmentChanged()), this, SLOT(environmentChanged()));
}

void MaemoQemuManager::buildConfigurationChanged(ProjectExplorer::BuildConfiguration *bc)
{
    if (bc)
        toggleStarterButton(bc->target());
}

void MaemoQemuManager::environmentChanged()
{
    // likely to happen when the qt version changes the build config is using
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
    env.insert(key, root % QLatin1String("bin") % colon % env.value(key));
    env.insert(key, root % QLatin1String("madlib") % colon % env.value(key));
#endif
    for (QHash<QString, QString>::ConstIterator it = rt.m_environment.constBegin();
        it != rt.m_environment.constEnd(); ++it)
        env.insert(it.key(), it.value());
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

void MaemoQemuManager::qemuProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
        emit qemuProcessStatus(QemuFailedToStart, m_qemuProcess->errorString());
}

void MaemoQemuManager::qemuStatusChanged(QemuStatus status, const QString &error)
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

void MaemoQemuManager::qemuOutput()
{
    qDebug("%s", m_qemuProcess->readAllStandardOutput().data());
    qDebug("%s", m_qemuProcess->readAllStandardError().data());
}

void MaemoQemuManager::runtimeRootChanged(const QString &directory)
{
    QList<int> uniqueIds;
    QMap<int, Runtime>::const_iterator it;
    for (it = m_runtimes.constBegin(); it != m_runtimes.constEnd(); ++it) {
        if (QDir(it.value().m_watchPath) == QDir(directory))
            uniqueIds.append(it.key());
    }

    foreach (int uniqueId, uniqueIds) {
        Runtime runtime = m_runtimes.value(uniqueId, Runtime());
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
                    m_runtimeFolderWatcher->addPath(runtime.m_root);
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
        QMap<int, Runtime>::const_iterator it;
        for (it = m_runtimes.constBegin(); it != m_runtimes.constEnd(); ++it) {
            if (QDir(it.value().m_root) == QDir(directory))
                uniqueIds.append(it.key());
        }
        notify(uniqueIds);
        m_runtimeFolderWatcher->removePath(directory);
    }
}

// -- private

void MaemoQemuManager::setupRuntimes()
{
    m_needsSetup = false;

    const QList<QtVersion*> &versions = QtVersionManager::instance()
        ->versionsForTargetId(Constants::MAEMO_DEVICE_TARGET_ID);

    QList<int> uniqueIds;
    foreach (QtVersion *version, versions)
        uniqueIds.append(version->uniqueId());
    qtVersionsChanged(uniqueIds);
}

void MaemoQemuManager::updateStarterIcon(bool running)
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

void MaemoQemuManager::toggleStarterButton(Target *target)
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

    m_qemuAction->setEnabled(m_runtimes.value(uniqueId, Runtime()).isValid()
        && targetUsesMatchingRuntimeConfig(target) && !isRunning);
}

bool MaemoQemuManager::sessionHasMaemoTarget() const
{
    bool result = false;
    ProjectExplorerPlugin *explorer = ProjectExplorerPlugin::instance();
    const QList<Project*> &projects = explorer->session()->projects();
    foreach (const Project *p, projects)
        result |= p->target(QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID)) != 0;
    return result;
}

bool MaemoQemuManager::targetUsesMatchingRuntimeConfig(Target *target,
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
    if (!version || !m_runtimes.value(version->uniqueId(), Runtime()).isValid())
        return false;

    if (qtVersion)
        *qtVersion = version;
    const MaemoDeviceConfig &config = mrc->deviceConfig();
    return config.isValid() && config.type == MaemoDeviceConfig::Simulator;
}

QString MaemoQemuManager::maddeRoot(const QString &qmake) const
{
    QDir dir(QDir::cleanPath(qmake).remove(binQmake));
    dir.cdUp(); dir.cdUp();
    return dir.absolutePath();
}

QString MaemoQemuManager::targetRoot(const QString &qmake) const
{
    const QString target = QDir::cleanPath(qmake).remove(binQmake);
    return target.mid(target.lastIndexOf(QLatin1Char('/')) + 1);
}

bool MaemoQemuManager::fillRuntimeInformation(Runtime *runtime) const
{
    const QStringList files = QDir(runtime->m_root).entryList(QDir::Files
        | QDir::NoSymLinks | QDir::NoDotAndDotDot);

    // we need at least the information file
    const QLatin1String infoFile("information");
    if (files.contains(infoFile)) {
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
            setEnvironment(runtime, map.value(QLatin1String("libpath")));
            runtime->m_sshPort = map.value(QLatin1String("sshport"));
            runtime->m_freePorts = MaemoPortList();
            int i = 2;
            while (true) {
                const QString port = map.value(QLatin1String("redirport")
                    + QString::number(i++));
                if (port.isEmpty())
                    break;
                runtime->m_freePorts.addPort(port.toInt());
            }
            return true;
        }
    }
    return false;
}

void MaemoQemuManager::setEnvironment(Runtime *runTime,
    const QString &envSpec) const
{
    QString remainingEnvSpec = envSpec;
    QString currentKey;
    while (true) {
        const int nextEqualsSignPos
            = remainingEnvSpec.indexOf(QLatin1Char('='));
        if (nextEqualsSignPos == -1) {
            if (!currentKey.isEmpty())
                runTime->m_environment.insert(currentKey, remainingEnvSpec);
            break;
        }
        const int keyStartPos
            = remainingEnvSpec.lastIndexOf(QRegExp(QLatin1String("\\s")),
                nextEqualsSignPos) + 1;
        if (!currentKey.isEmpty()) {
            const int valueEndPos
                = remainingEnvSpec.lastIndexOf(QRegExp(QLatin1String("\\S")),
                    qMax(0, keyStartPos - 1)) + 1;
            runTime->m_environment.insert(currentKey,
                remainingEnvSpec.left(valueEndPos));
        }
        currentKey = remainingEnvSpec.mid(keyStartPos,
            nextEqualsSignPos - keyStartPos);
        remainingEnvSpec.remove(0, nextEqualsSignPos + 1);
    }
}

QString MaemoQemuManager::runtimeForQtVersion(const QString &qmakeCommand) const
{
    const QString &target = targetRoot(qmakeCommand);
    const QString &madRoot = maddeRoot(qmakeCommand);

    QString madCommand = madRoot + QLatin1String("/bin/mad");
    if (!QFileInfo(madCommand).exists())
        return QString();

    QProcess madProc;
    QStringList arguments(QLatin1String("info"));

#ifdef Q_OS_WIN
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("HOME",
        QDesktopServices::storageLocation(QDesktopServices::HomeLocation));
    env.insert(QLatin1String("PATH"),
        QDir::toNativeSeparators(madRoot % QLatin1String("/bin"))
        % QLatin1Char(';') % env.value(QLatin1String("PATH")));

    madProc.setProcessEnvironment(env);

    arguments.prepend(madCommand);
    madCommand = madRoot + QLatin1String("/bin/sh.exe");
#endif

    madProc.start(madCommand, arguments);
    if (!madProc.waitForStarted() || !madProc.waitForFinished())
        return QString();

    QStringList installedRuntimes;
    QString targetRuntime;
    QXmlStreamReader infoReader(madProc.readAllStandardOutput());
    while (!infoReader.atEnd() && !installedRuntimes.contains(targetRuntime)) {
        if (infoReader.readNext() == QXmlStreamReader::StartElement) {
            if (targetRuntime.isEmpty()
                && infoReader.name() == QLatin1String("target")) {
                const QXmlStreamAttributes &attrs = infoReader.attributes();
                if (attrs.value(QLatin1String("target_id")) == target)
                    targetRuntime = attrs.value("runtime_id").toString();
            } else if (infoReader.name() == QLatin1String("runtime")) {
                const QXmlStreamAttributes attrs = infoReader.attributes();
                while (!infoReader.atEnd()) {
                    if (infoReader.readNext() == QXmlStreamReader::EndElement
                         && infoReader.name() == QLatin1String("runtime"))
                        break;
                    if (infoReader.tokenType() == QXmlStreamReader::StartElement
                        && infoReader.name() == QLatin1String("installed")) {
                        if (infoReader.readNext() == QXmlStreamReader::Characters
                            && infoReader.text() == QLatin1String("true"))
                        installedRuntimes << attrs.value(QLatin1String("runtime_id")).toString();
                        break;
                    }
                }
            }
        }
    }
    if (installedRuntimes.contains(targetRuntime))
        return madRoot + QLatin1String("/runtimes/") + targetRuntime;
    return QString();
}

void MaemoQemuManager::notify(const QList<int> uniqueIds)
{
    qtVersionsChanged(uniqueIds);
    environmentChanged();   // to toggle the start button
}

void MaemoQemuManager::toggleDeviceConnections(MaemoRunConfiguration *mrc,
    bool _connect)
{
    if (!mrc)
        return;

    if (_connect) { // handle device configuration changes
        connect(mrc, SIGNAL(deviceConfigurationChanged(ProjectExplorer::Target*)),
            this, SLOT(deviceConfigurationChanged(ProjectExplorer::Target*)));
    } else {
        disconnect(mrc, SIGNAL(deviceConfigurationChanged(ProjectExplorer::Target*)),
            this, SLOT(deviceConfigurationChanged(ProjectExplorer::Target*)));
    }
}
