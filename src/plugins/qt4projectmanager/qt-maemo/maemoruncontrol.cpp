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
#include "maemodeploystep.h"
#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "maemorunconfiguration.h"

#include <coreplugin/icore.h>
#include <coreplugin/ssh/sftpchannel.h>
#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>
#include <debugger/debuggerengine.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>
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
    } else {
        stopInternal();
    }
}

QString AbstractMaemoRunControl::executableFilePathOnTarget() const
{
    const MaemoDeployables * const deployables
        = m_runConfig->packageStep()->deployables();
    return deployables->remoteExecutableFilePath(m_runConfig->executable());
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
    return m_runner && m_runner->isRunning();
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
        startExecutionIfPossible();
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
    return MaemoGlobal::homeDirOnDevice(m_devConfig.server.uname);
}

const QString AbstractMaemoRunControl::targetCmdLinePrefix() const
{
    return QString::fromLocal8Bit("%1 chmod a+x %2 && source /etc/profile && DISPLAY=:0.0 ")
        .arg(MaemoGlobal::remoteSudo()).arg(executableFilePathOnTarget());
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
    , m_uploadJob(SftpInvalidJob)
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
    const QString &dumperLib = m_runConfig->dumperLib();
    if (!dumperLib.isEmpty()
        && m_runConfig->deployStep()->currentlyNeedsDeployment(m_devConfig.server.host,
            MaemoDeployable(dumperLib, uploadDir()))) {
        m_uploader = m_connection->createSftpChannel();
        connect(m_uploader.data(), SIGNAL(initialized()), this,
            SLOT(handleSftpChannelInitialized()));
        connect(m_uploader.data(), SIGNAL(initializationFailed(QString)), this,
            SLOT(handleSftpChannelInitializationFailed(QString)));
        connect(m_uploader.data(), SIGNAL(finished(Core::SftpJobId, QString)),
            this, SLOT(handleSftpJobFinished(Core::SftpJobId, QString)));
        m_uploader->initialize();
    } else {
        startDebugging();
    }
}

void MaemoDebugRunControl::handleSftpChannelInitialized()
{
    if (m_stopped)
        return;

    const QString dumperLib = m_runConfig->dumperLib();
    const QString fileName = QFileInfo(dumperLib).fileName();
    const QString remoteFilePath = uploadDir() + '/' + fileName;
    m_uploadJob = m_uploader->uploadFile(dumperLib, remoteFilePath,
        SftpOverwriteExisting);
    if (m_uploadJob == SftpInvalidJob) {
        handleError(tr("Upload failed: Could not open file '%1'")
            .arg(dumperLib));
    } else {
        emit appendMessage(this,
            tr("Started uploading debugging helpers ('%1').").arg(dumperLib),
            false);
    }
}

void MaemoDebugRunControl::handleSftpChannelInitializationFailed(const QString &error)
{
    handleError(error);
}

void MaemoDebugRunControl::handleSftpJobFinished(Core::SftpJobId job,
    const QString &error)
{
    if (m_stopped)
        return;

    if (job != m_uploadJob) {
        qWarning("Warning: Unknown debugging helpers upload job %d finished.", job);
        return;
    }

    if (!error.isEmpty()) {
        handleError(tr("Error: Could not upload debugging helpers."));
    } else {
        m_runConfig->deployStep()->setDeployed(m_devConfig.server.host,
            MaemoDeployable(m_runConfig->dumperLib(), uploadDir()));
        emit appendMessage(this,
            tr("Finished uploading debugging helpers."), false);
        startDebugging();
    }
}

void MaemoDebugRunControl::startDebugging()
{
#ifdef USE_GDBSERVER
    AbstractMaemoRunControl::startExecution();
#else
    DebuggerPlugin::startDebugger(m_debuggerRunControl);
#endif
}

bool MaemoDebugRunControl::isDeploying() const
{
    return m_uploader && m_uploadJob != SftpInvalidJob;
}

void MaemoDebugRunControl::stopInternal()
{
    if (isDeploying()) {
        disconnect(m_uploader.data(), 0, this, 0);
        m_uploader->closeChannel();
        m_uploadJob = SftpInvalidJob;
        emit finished();
    } else if (m_debuggerRunControl && m_debuggerRunControl->engine()) {
        m_debuggerRunControl->engine()->quitDebugger();
    } else {
        emit finished();
    }
}

bool MaemoDebugRunControl::isRunning() const
{
    return isDeploying() || AbstractMaemoRunControl::isRunning()
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
    DebuggerPlugin::startDebugger(m_debuggerRunControl);
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

} // namespace Internal
} // namespace Qt4ProjectManager
