/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemoruncontrol.h"

#include "maemodeployables.h"
#include "maemopackagecreationstep.h"
#include "maemorunconfiguration.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/ssh/sftpchannel.h>
#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>
#include <debugger/debuggerengine.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/toolchain.h>
#include <utils/qtcassert.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFuture>
#include <QtCore/QProcess>
#include <QtCore/QStringBuilder>

#include <QtGui/QMessageBox>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

#define USE_GDBSERVER

using ProjectExplorer::RunConfiguration;
using ProjectExplorer::ToolChain;
using namespace Debugger;


AbstractMaemoRunControl::AbstractMaemoRunControl(RunConfiguration *rc, QString mode)
    : RunControl(rc, mode)
    , m_runConfig(qobject_cast<MaemoRunConfiguration *>(rc))
    , m_devConfig(m_runConfig ? m_runConfig->deviceConfig() : MaemoDeviceConfig())
{
}

AbstractMaemoRunControl::~AbstractMaemoRunControl()
{
}

    // TODO: We can cache the connection. We'd have to check if the connection
    // is active and its parameters are the same as m_devConfig. If yes,
    //     skip the connection step and jump right to handleConnected()
void AbstractMaemoRunControl::start()
{
    if (!m_devConfig.isValid()) {
        handleError(tr("No device configuration set for run configuration."));
    } else {
        m_stopped = false;
        emit started();
        m_connection = SshConnection::create();
        connect(m_connection.data(), SIGNAL(connected()), this,
            SLOT(handleConnected()));
        connect(m_connection.data(), SIGNAL(error(SshError)), this,
            SLOT(handleConnectionFailure()));
        m_connection->connectToHost(m_devConfig.server);
    }
}

void AbstractMaemoRunControl::handleConnected()
{
    if (m_stopped)
        return;

    emit appendMessage(this, tr("Cleaning up remote leftovers first ..."), false);
    const QStringList appsToKill = QStringList() << executableFileName()
#ifdef USE_GDBSERVER
        << QLatin1String("gdbserver");
#else
        << QLatin1String("gdb");
#endif
    killRemoteProcesses(appsToKill, true);
}

void AbstractMaemoRunControl::handleConnectionFailure()
{
    if (m_stopped)
        return;

    handleError(tr("Could not connect to host: %1")
        .arg(m_connection->errorString()));
    emit finished();
}

void AbstractMaemoRunControl::stop()
{
    m_stopped = true;
    if (m_connection && m_connection->state() == SshConnection::Connecting) {
        disconnect(m_connection.data(), 0, this, 0);
        m_connection->disconnectFromHost();
        emit finished();
    } else if (m_initialCleaner && m_initialCleaner->isRunning()) {
        disconnect(m_initialCleaner.data(), 0, this, 0);
        emit finished();
    } else if (m_installer && m_installer->isRunning()) {
        disconnect(m_installer.data(), 0, this, 0);
        emit finished();
    } else if (isDeploying()) {
        m_uploadsInProgress.clear();
        m_linksInProgress.clear();
        disconnect(m_uploader.data(), 0, this, 0);
        m_progress.reportCanceled();
        m_progress.reportFinished();
        emit finished();
    } else {
        stopInternal();
    }
}

void AbstractMaemoRunControl::startDeployment()
{
    QTC_ASSERT(m_runConfig, return);

    m_uploader = m_connection->createSftpChannel();
    connect(m_uploader.data(), SIGNAL(initialized()), this,
        SLOT(handleSftpChannelInitialized()));
    connect(m_uploader.data(), SIGNAL(initializationFailed(QString)), this,
        SLOT(handleSftpChannelInitializationFailed(QString)));
    connect(m_uploader.data(), SIGNAL(finished(Core::SftpJobId, QString)),
        this, SLOT(handleSftpJobFinished(Core::SftpJobId, QString)));
    m_uploader->initialize();
}

void AbstractMaemoRunControl::handleSftpChannelInitialized()
{
    if (m_stopped)
        return;

    m_uploadsInProgress.clear();
    m_linksInProgress.clear();
    m_needsInstall = false;
    const QList<MaemoDeployable> &deployables = filesToDeploy();
    foreach (const MaemoDeployable &d, deployables) {
        const QString fileName = QFileInfo(d.localFilePath).fileName();
        const QString remoteFilePath = d.remoteDir + '/' + fileName;
        const QString uploadFilePath = uploadDir() + '/' + fileName + '.'
            + QCryptographicHash::hash(remoteFilePath.toUtf8(),
                  QCryptographicHash::Md5).toHex();
        const SftpJobId job = m_uploader->uploadFile(d.localFilePath,
            uploadFilePath, SftpOverwriteExisting);
        if (job == SftpInvalidJob) {
            handleError(tr("Upload failed: Could not open file '%1'")
                .arg(d.localFilePath));
            return;
        }
        emit appendMessage(this, tr("Started uploading file '%1'.")
            .arg(d.localFilePath), false);
        m_uploadsInProgress.insert(job, DeployInfo(d, uploadFilePath));
    }

    Core::ICore::instance()->progressManager()
        ->addTask(m_progress.future(), tr("Deploying"),
                  QLatin1String("Maemo.Deploy"));
    if (!m_uploadsInProgress.isEmpty()) {
        m_progress.setProgressRange(0, m_uploadsInProgress.count());
        m_progress.setProgressValue(0);
        m_progress.reportStarted();
    } else {
        m_progress.reportFinished();
        startExecutionIfPossible();
    }
}

void AbstractMaemoRunControl::handleSftpChannelInitializationFailed(const QString &error)
{
    if (m_stopped)
        return;
    handleError(tr("Could not set up SFTP connection: %1").arg(error));
}

void AbstractMaemoRunControl::handleSftpJobFinished(Core::SftpJobId job,
    const QString &error)
{
    if (m_stopped)
        return;

    QMap<SftpJobId, DeployInfo>::Iterator it = m_uploadsInProgress.find(job);
    if (it == m_uploadsInProgress.end()) {
        qWarning("%s: Job %u not found in map.", Q_FUNC_INFO, job);
        return;
    }

    const DeployInfo &deployInfo = it.value();
    if (!error.isEmpty()) {
        handleError(tr("Failed to upload file %1: %2")
            .arg(deployInfo.first.localFilePath, error));
        return;
    }

    m_progress.setProgressValue(m_progress.progressValue() + 1);
    appendMessage(this, tr("Successfully uploaded file '%1'.")
        .arg(deployInfo.first.localFilePath), false);

    const QString remoteFilePath = deployInfo.first.remoteDir + '/'
        + QFileInfo(deployInfo.first.localFilePath).fileName();
    QByteArray linkCommand = remoteSudo().toUtf8() + " ln -sf "
        + deployInfo.second.toUtf8() + ' ' + remoteFilePath.toUtf8();
    SshRemoteProcess::Ptr linkProcess
        = m_connection->createRemoteProcess(linkCommand);
    connect(linkProcess.data(), SIGNAL(closed(int)), this,
        SLOT(handleLinkProcessFinished(int)));
    m_linksInProgress.insert(linkProcess, deployInfo.first);
    linkProcess->start();
    m_uploadsInProgress.erase(it);
}

void AbstractMaemoRunControl::handleLinkProcessFinished(int exitStatus)
{
    if (m_stopped)
        return;

    SshRemoteProcess * const proc = static_cast<SshRemoteProcess *>(sender());

    // TODO: List instead of map? We can't use it for lookup anyway.
    QMap<SshRemoteProcess::Ptr, MaemoDeployable>::Iterator it;
    for (it = m_linksInProgress.begin(); it != m_linksInProgress.end(); ++it) {
        if (it.key().data() == proc)
            break;
    }
    if (it == m_linksInProgress.end()) {
        qWarning("%s: Remote process %p not found in process list.",
            Q_FUNC_INFO, proc);
        return;
    }

    const MaemoDeployable &deployable = it.value();
    if (exitStatus != SshRemoteProcess::ExitedNormally
        || proc->exitCode() != 0) {
        handleError(tr("Deployment failed for file '%1': "
            "Could not create link '%2' on remote system.")
            .arg(deployable.localFilePath, deployable.remoteDir + '/'
                + QFileInfo(deployable.localFilePath).fileName()));
        return;
    }

    m_runConfig->setDeployed(m_devConfig.server.host, it.value());
    m_linksInProgress.erase(it);
    if (m_linksInProgress.isEmpty() && m_uploadsInProgress.isEmpty()) {
        if (m_needsInstall) {
            emit appendMessage(this, tr("Installing package ..."), false);
            const QByteArray cmd = remoteSudo().toUtf8() + " dpkg -i "
                + packageFileName().toUtf8();
            m_installer = m_connection->createRemoteProcess(cmd);
            connect(m_installer.data(), SIGNAL(closed(int)), this,
                SLOT(handleInstallationFinished(int)));
            connect(m_installer.data(), SIGNAL(outputAvailable(QByteArray)),
                this, SLOT(handleRemoteOutput(QByteArray)));
            connect(m_installer.data(),
                SIGNAL(errorOutputAvailable(QByteArray)), this,
                SLOT(handleRemoteErrorOutput(QByteArray)));
            m_installer->start();
        } else {
            handleDeploymentFinished();
        }
    }
}

void AbstractMaemoRunControl::handleInstallationFinished(int exitStatus)
{
    if (m_stopped)
        return;

    if (exitStatus != SshRemoteProcess::ExitedNormally
        || m_installer->exitCode() != 0) {
        handleError(tr("Installing package failed."));
    } else {
        emit appendMessage(this, tr("Package installation finished."), false);
        handleDeploymentFinished();
    }
}

void AbstractMaemoRunControl::handleDeploymentFinished()
{
    emit appendMessage(this, tr("Deployment finished."), false);
    m_progress.reportFinished();
    startExecutionIfPossible();
}

QList<MaemoDeployable> AbstractMaemoRunControl::filesToDeploy()
{
    QList<MaemoDeployable> deployableList;
    if (m_runConfig->packageStep()->isPackagingEnabled()) {
        const MaemoDeployable d(packageFilePath(), uploadDir());
        m_needsInstall = addDeployableIfNeeded(deployableList, d);
    } else {
        const MaemoDeployables * const deployables
            = m_runConfig->packageStep()->deployables();
        const int deployableCount = deployables->deployableCount();
        for (int i = 0; i < deployableCount; ++i) {
            const MaemoDeployable &d = deployables->deployableAt(i);
            addDeployableIfNeeded(deployableList, d);
        }
        m_needsInstall = false;
    }
    return deployableList;
}

bool AbstractMaemoRunControl::addDeployableIfNeeded(QList<MaemoDeployable> &deployables,
    const MaemoDeployable &deployable)
{
    if (m_runConfig->currentlyNeedsDeployment(m_devConfig.server.host,
        deployable)) {
        deployables << deployable;
        return true;
    } else {
        return false;
    }
}

bool AbstractMaemoRunControl::isDeploying() const
{
    return !m_uploadsInProgress.isEmpty() || !m_linksInProgress.isEmpty();
}

QString AbstractMaemoRunControl::packageFileName() const
{
    return QFileInfo(packageFilePath()).fileName();
}

QString AbstractMaemoRunControl::packageFilePath() const
{
    return m_runConfig->packageStep()->packageFilePath();
}

QString AbstractMaemoRunControl::executableFilePathOnTarget() const
{
    const MaemoDeployables * const deployables
        = m_runConfig->packageStep()->deployables();
    return deployables->remoteExecutableFilePath(m_runConfig->executable());
}

bool AbstractMaemoRunControl::isCleaning() const
{
    return m_initialCleaner && m_initialCleaner->isRunning();
}

void AbstractMaemoRunControl::startExecutionIfPossible()
{
    if (executableFilePathOnTarget().isEmpty()) {
        handleError(tr("Cannot run: No remote executable set."));
        emit finished();
    } else {
        startExecution();
    }
}

void AbstractMaemoRunControl::startExecution()
{
    m_runner = m_connection->createRemoteProcess(remoteCall().toUtf8());
    connect(m_runner.data(), SIGNAL(started()), this,
        SLOT(handleRemoteProcessStarted()));
    connect(m_runner.data(), SIGNAL(closed(int)), this,
        SLOT(handleRemoteProcessFinished(int)));
    connect(m_runner.data(), SIGNAL(outputAvailable(QByteArray)), this,
        SLOT(handleRemoteOutput(QByteArray)));
    connect(m_runner.data(), SIGNAL(errorOutputAvailable(QByteArray)), this,
        SLOT(handleRemoteErrorOutput(QByteArray)));
    emit appendMessage(this, tr("Starting remote application."), false);
    m_runner->start();
}

void AbstractMaemoRunControl::handleRemoteProcessFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    if (m_stopped)
        return;

    if (exitStatus == SshRemoteProcess::ExitedNormally) {
        emit appendMessage(this,
            tr("Finished running remote process. Exit code was %1.")
            .arg(m_runner->exitCode()), false);
        emit finished();
    } else {
        handleError(tr("Error running remote process: %1")
            .arg(m_runner->errorString()));
    }
}

void AbstractMaemoRunControl::handleRemoteOutput(const QByteArray &output)
{
    emit addToOutputWindowInline(this, QString::fromUtf8(output), false);
}

void AbstractMaemoRunControl::handleRemoteErrorOutput(const QByteArray &output)
{
    emit addToOutputWindowInline(this, QString::fromUtf8(output), true);
}

bool AbstractMaemoRunControl::isRunning() const
{
    return isDeploying() || (m_runner && m_runner->isRunning());
}

void AbstractMaemoRunControl::stopRunning(bool forDebugging)
{
    if (m_runner && m_runner->isRunning()) {
        disconnect(m_runner.data(), 0, this, 0);
        QStringList apps(executableFileName());
        if (forDebugging)
            apps << QLatin1String("gdbserver");
        killRemoteProcesses(apps, false);
        emit finished();
    }
}

void AbstractMaemoRunControl::killRemoteProcesses(const QStringList &apps,
    bool initialCleanup)
{
    QString niceKill;
    QString brutalKill;
    foreach (const QString &app, apps) {
        niceKill += QString::fromLocal8Bit("pkill -x %1;").arg(app);
        brutalKill += QString::fromLocal8Bit("pkill -x -9 %1;").arg(app);
    }
    QString remoteCall = niceKill + QLatin1String("sleep 1; ") + brutalKill;
    remoteCall.remove(remoteCall.count() - 1, 1); // Get rid of trailing semicolon.
    SshRemoteProcess::Ptr proc
        = m_connection->createRemoteProcess(remoteCall.toUtf8());
    if (initialCleanup) {
        m_initialCleaner = proc;
        connect(m_initialCleaner.data(), SIGNAL(closed(int)), this,
            SLOT(handleInitialCleanupFinished(int)));
    } else {
        m_stopper = proc;
    }
    proc->start();
}

void AbstractMaemoRunControl::handleInitialCleanupFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    if (m_stopped)
        return;

    if (exitStatus != SshRemoteProcess::ExitedNormally) {
        handleError(tr("Initial cleanup failed: %1")
            .arg(m_initialCleaner->errorString()));
    } else {
        emit appendMessage(this, tr("Initial cleanup done."), false);
        startDeployment();
    }
}

const QString AbstractMaemoRunControl::executableOnHost() const
{
    return m_runConfig->executable();
}

const QString AbstractMaemoRunControl::executableFileName() const
{
    return QFileInfo(executableOnHost()).fileName();
}

const QString AbstractMaemoRunControl::uploadDir() const
{
    return homeDirOnDevice(m_devConfig.server.uname);
}

QString AbstractMaemoRunControl::remoteSudo() const
{
    return QLatin1String("/usr/lib/mad-developer/devrootsh");
}

const QString AbstractMaemoRunControl::targetCmdLinePrefix() const
{
    return QString::fromLocal8Bit("%1 chmod a+x %2 && source /etc/profile && DISPLAY=:0.0 ")
        .arg(remoteSudo()).arg(executableFilePathOnTarget());
}

QString AbstractMaemoRunControl::targetCmdLineSuffix() const
{
    return m_runConfig->arguments().join(" ");
}

void AbstractMaemoRunControl::handleError(const QString &errString)
{
    QMessageBox::critical(0, tr("Remote Execution Failure"), errString);
    emit appendMessage(this, errString, true);
    stop();
}

template<typename SshChannel> void AbstractMaemoRunControl::closeSshChannel(SshChannel &channel)
{
    if (channel) {
        disconnect(channel.data(), 0, this, 0);
        // channel->closeChannel();
    }
}


MaemoRunControl::MaemoRunControl(RunConfiguration *runConfiguration)
    : AbstractMaemoRunControl(runConfiguration, ProjectExplorer::Constants::RUNMODE)
{
}

MaemoRunControl::~MaemoRunControl()
{
    stop();
}

QString MaemoRunControl::remoteCall() const
{
    return QString::fromLocal8Bit("%1 %2 %3").arg(targetCmdLinePrefix())
        .arg(executableFilePathOnTarget()).arg(targetCmdLineSuffix());
}

void MaemoRunControl::stopInternal()
{
    AbstractMaemoRunControl::stopRunning(false);
}


MaemoDebugRunControl::MaemoDebugRunControl(RunConfiguration *runConfiguration)
    : AbstractMaemoRunControl(runConfiguration, ProjectExplorer::Constants::DEBUGMODE)
    , m_debuggerRunControl(0)
    , m_startParams(new DebuggerStartParameters)
{
#ifdef USE_GDBSERVER
    m_startParams->startMode = AttachToRemote;
    m_startParams->executable = executableOnHost();
    m_startParams->debuggerCommand = m_runConfig->gdbCmd();
    m_startParams->remoteChannel
        = m_devConfig.server.host % QLatin1Char(':') % gdbServerPort();
    m_startParams->remoteArchitecture = QLatin1String("arm");
#else
    m_startParams->startMode = StartRemoteGdb;
    m_startParams->executable = executableFilePathOnTarget();
    m_startParams->debuggerCommand = targetCmdLinePrefix()
        + QLatin1String(" /usr/bin/gdb");
    m_startParams->connParams = m_devConfig.server;
#endif
    m_startParams->processArgs = m_runConfig->arguments();
    m_startParams->sysRoot = m_runConfig->sysRoot();
    m_startParams->toolChainType = ToolChain::GCC_MAEMO;
    m_startParams->dumperLibrary = m_runConfig->dumperLib();
    m_startParams->remoteDumperLib = uploadDir().toUtf8() + '/'
        + QFileInfo(m_runConfig->dumperLib()).fileName().toUtf8();

    m_debuggerRunControl = DebuggerPlugin::createDebugger(*m_startParams.data());
    connect(m_debuggerRunControl, SIGNAL(finished()), this,
        SLOT(debuggingFinished()), Qt::QueuedConnection);
}

MaemoDebugRunControl::~MaemoDebugRunControl()
{
    disconnect(SIGNAL(addToOutputWindow(ProjectExplorer::RunControl*,QString, bool)));
    disconnect(SIGNAL(addToOutputWindowInline(ProjectExplorer::RunControl*,QString, bool)));
    stop();
}

QString MaemoDebugRunControl::remoteCall() const
{
    return QString::fromLocal8Bit("%1 gdbserver :%2 %3 %4")
        .arg(targetCmdLinePrefix()).arg(gdbServerPort())
        .arg(executableFilePathOnTarget()).arg(targetCmdLineSuffix());
}

void MaemoDebugRunControl::startExecution()
{
#ifdef USE_GDBSERVER
    AbstractMaemoRunControl::startExecution();
#else
    startDebugging();
#endif
}

void MaemoDebugRunControl::startDebugging()
{
    DebuggerPlugin::startDebugger(m_debuggerRunControl);
}

void MaemoDebugRunControl::stopInternal()
{
    m_debuggerRunControl->engine()->quitDebugger();
}

bool MaemoDebugRunControl::isRunning() const
{
    return AbstractMaemoRunControl::isRunning()
        || m_debuggerRunControl->state() != DebuggerNotReady;
}

void MaemoDebugRunControl::debuggingFinished()
{
#ifdef USE_GDBSERVER
    AbstractMaemoRunControl::stopRunning(true);
#else
    emit finished();
#endif
}

void MaemoDebugRunControl::handleRemoteProcessStarted()
{
    startDebugging();
}

void MaemoDebugRunControl::debuggerOutput(const QString &output)
{
    emit appendMessage(this, QLatin1String("[gdb says:] ") + output, true);
}

QString MaemoDebugRunControl::gdbServerPort() const
{
    return m_devConfig.type == MaemoDeviceConfig::Physical
        ? QString::number(m_devConfig.gdbServerPort)
        : m_runConfig->runtimeGdbServerPort();  // During configuration we don't
        // know which port to use, so we display something in the config dialog,
        // but we will make sure we use the right port from the information file.
}

QList<MaemoDeployable> MaemoDebugRunControl::filesToDeploy()
{
    QList<MaemoDeployable> deployables
        = AbstractMaemoRunControl::filesToDeploy();
    const MaemoDeployable d(m_runConfig->dumperLib(), uploadDir());
    addDeployableIfNeeded(deployables, d);
    return deployables;
}

} // namespace Internal
} // namespace Qt4ProjectManager
