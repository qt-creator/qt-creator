/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "remotelinuxapplicationrunner.h"

#include "linuxdeviceconfiguration.h"
#include "maemoglobal.h"
#include "remotelinuxrunconfiguration.h"
#include "maemousedportsgatherer.h"

#include <utils/qtcassert.h>
#include <utils/ssh/sshconnection.h>
#include <utils/ssh/sshconnectionmanager.h>
#include <utils/ssh/sshremoteprocess.h>

#include <limits>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(State, state, m_state)

using namespace Qt4ProjectManager;
using namespace Utils;

namespace RemoteLinux {
using namespace Internal;

RemoteLinuxApplicationRunner::RemoteLinuxApplicationRunner(QObject *parent,
        RemoteLinuxRunConfiguration *runConfig)
    : QObject(parent),
      m_portsGatherer(new MaemoUsedPortsGatherer(this)),
      m_devConfig(runConfig->deviceConfig()),
      m_remoteExecutable(runConfig->remoteExecutableFilePath()),
      m_appArguments(runConfig->arguments()),
      m_commandPrefix(runConfig->commandPrefix()),
      m_initialFreePorts(runConfig->freePorts()),
      m_stopRequested(false),
      m_state(Inactive)
{
    // Prevent pkill from matching our own pkill call.
    QString pkillArg = m_remoteExecutable;
    const int lastPos = pkillArg.count() - 1;
    pkillArg.replace(lastPos, 1, QLatin1Char('[') + pkillArg.at(lastPos) + QLatin1Char(']'));
    m_procsToKill << pkillArg;

    connect(m_portsGatherer, SIGNAL(error(QString)), SLOT(handlePortsGathererError(QString)));
    connect(m_portsGatherer, SIGNAL(portListReady()), SLOT(handleUsedPortsAvailable()));
}

RemoteLinuxApplicationRunner::~RemoteLinuxApplicationRunner() {}

SshConnection::Ptr RemoteLinuxApplicationRunner::connection() const
{
    return m_connection;
}

LinuxDeviceConfiguration::ConstPtr RemoteLinuxApplicationRunner::devConfig() const
{
    return m_devConfig;
}

void RemoteLinuxApplicationRunner::start()
{
    QTC_ASSERT(!m_stopRequested, return);
    ASSERT_STATE(Inactive);

    QString errorMsg;
    if (!canRun(errorMsg)) {
        emitError(tr("Cannot run: %1").arg(errorMsg), true);
        return;
    }

    m_connection = SshConnectionManager::instance().acquireConnection(m_devConfig->sshParameters());
    setState(Connecting);
    m_exitStatus = -1;
    m_freePorts = m_initialFreePorts;
    connect(m_connection.data(), SIGNAL(connected()), this,
        SLOT(handleConnected()));
    connect(m_connection.data(), SIGNAL(error(Utils::SshError)), this,
        SLOT(handleConnectionFailure()));
    if (isConnectionUsable()) {
        handleConnected();
    } else {
        emit reportProgress(tr("Connecting to device..."));
        if (m_connection->state() == Utils::SshConnection::Unconnected)
            m_connection->connectToHost();
    }
}

void RemoteLinuxApplicationRunner::stop()
{
    if (m_stopRequested)
        return;

    switch (m_state) {
    case Connecting:
        setState(Inactive);
        emit remoteProcessFinished(InvalidExitCode);
        break;
    case GatheringPorts:
        m_portsGatherer->stop();
        setState(Inactive);
        emit remoteProcessFinished(InvalidExitCode);
        break;
    case PreRunCleaning:
    case AdditionalPreRunCleaning:
    case AdditionalInitializing:
    case ProcessStarting:
    case PostRunCleaning:
    case AdditionalPostRunCleaning:
        m_stopRequested = true;
        break;
    case ReadyForExecution:
        m_stopRequested = true;
        setState(AdditionalPostRunCleaning);
        doAdditionalPostRunCleanup();
        break;
    case ProcessStarted:
        m_stopRequested = true;
        cleanup();
        break;
    case Inactive:
        break;
    }
}

void RemoteLinuxApplicationRunner::handleConnected()
{
    ASSERT_STATE(Connecting);
    if (m_stopRequested) {
        emit remoteProcessFinished(InvalidExitCode);
        setState(Inactive);
    } else {
        setState(PreRunCleaning);
        cleanup();
    }
}

void RemoteLinuxApplicationRunner::handleConnectionFailure()
{
    if (m_state == Inactive) {
        qWarning("Unexpected state %d in %s.", m_state, Q_FUNC_INFO);
        return;
    }

    if (m_state != Connecting || m_state != PreRunCleaning)
        doAdditionalConnectionErrorHandling();

    const QString errorMsg = m_state == Connecting
        ? MaemoGlobal::failedToConnectToServerMessage(m_connection, m_devConfig)
        : tr("Connection error: %1").arg(m_connection->errorString());
    emitError(errorMsg);
}

void RemoteLinuxApplicationRunner::cleanup()
{
    ASSERT_STATE(QList<State>() << PreRunCleaning << PostRunCleaning << ProcessStarted);

    emit reportProgress(tr("Killing remote process(es)..."));

    // Fremantle's busybox configuration is strange.
    const char *killTemplate;
    if (m_devConfig->osType() == LinuxDeviceConfiguration::Maemo5OsType)
        killTemplate = "pkill -f -%2 %1;";
    else
        killTemplate = "pkill -%2 -f %1;";

    QString niceKill;
    QString brutalKill;
    foreach (const QString &proc, m_procsToKill) {
        niceKill += QString::fromLocal8Bit(killTemplate).arg(proc).arg("SIGTERM");
        brutalKill += QString::fromLocal8Bit(killTemplate).arg(proc).arg("SIGKILL");
    }
    QString remoteCall = niceKill + QLatin1String("sleep 1; ") + brutalKill;
    remoteCall.remove(remoteCall.count() - 1, 1); // Get rid of trailing semicolon.

    m_cleaner = m_connection->createRemoteProcess(remoteCall.toUtf8());
    connect(m_cleaner.data(), SIGNAL(closed(int)), this,
        SLOT(handleCleanupFinished(int)));
    m_cleaner->start();
}

void RemoteLinuxApplicationRunner::handleCleanupFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    ASSERT_STATE(QList<State>() << PreRunCleaning << PostRunCleaning << ProcessStarted << Inactive);

    if (m_state == Inactive)
        return;
    if (m_stopRequested && m_state == PreRunCleaning) {
        setState(Inactive);
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }
    if (m_stopRequested || m_state == PostRunCleaning) {
        setState(AdditionalPostRunCleaning);
        doAdditionalPostRunCleanup();
        return;
    }

    if (exitStatus != SshRemoteProcess::ExitedNormally) {
        emitError(tr("Initial cleanup failed: %1").arg(m_cleaner->errorString()));
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }

    setState(AdditionalPreRunCleaning);
    doAdditionalInitialCleanup();
}

void RemoteLinuxApplicationRunner::startExecution(const QByteArray &remoteCall)
{
    ASSERT_STATE(ReadyForExecution);

    if (m_stopRequested)
        return;

    m_runner = m_connection->createRemoteProcess(remoteCall);
    connect(m_runner.data(), SIGNAL(started()), this,
        SLOT(handleRemoteProcessStarted()));
    connect(m_runner.data(), SIGNAL(closed(int)), this,
        SLOT(handleRemoteProcessFinished(int)));
    connect(m_runner.data(), SIGNAL(outputAvailable(QByteArray)), this,
        SIGNAL(remoteOutput(QByteArray)));
    connect(m_runner.data(), SIGNAL(errorOutputAvailable(QByteArray)), this,
        SIGNAL(remoteErrorOutput(QByteArray)));
    setState(ProcessStarting);
    m_runner->start();
}

void RemoteLinuxApplicationRunner::handleRemoteProcessStarted()
{
    ASSERT_STATE(ProcessStarting);

    setState(ProcessStarted);
    if (m_stopRequested) {
        cleanup();
        return;
    }

    emit reportProgress(tr("Remote process started."));
    emit remoteProcessStarted();
}

void RemoteLinuxApplicationRunner::handleRemoteProcessFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);
    ASSERT_STATE(QList<State>() << ProcessStarted << Inactive);

    m_exitStatus = exitStatus;
    if (!m_stopRequested && m_state != Inactive) {
        setState(PostRunCleaning);
        cleanup();
    }
}

bool RemoteLinuxApplicationRunner::isConnectionUsable() const
{
    return m_connection && m_connection->state() == SshConnection::Connected
        && m_connection->connectionParameters() == m_devConfig->sshParameters();
}

void RemoteLinuxApplicationRunner::setState(State newState)
{
    if (newState == Inactive) {
        m_portsGatherer->stop();
        if (m_connection) {
            disconnect(m_connection.data(), 0, this, 0);
            SshConnectionManager::instance().releaseConnection(m_connection);
            m_connection = SshConnection::Ptr();
        }
        if (m_cleaner)
            disconnect(m_cleaner.data(), 0, this, 0);
        m_stopRequested = false;
    }
    m_state = newState;
}

void RemoteLinuxApplicationRunner::emitError(const QString &errorMsg, bool force)
{
    if (m_state != Inactive) {
        setState(Inactive);
        emit error(errorMsg);
    } else if (force) {
        emit error(errorMsg);
    }
}

void RemoteLinuxApplicationRunner::handlePortsGathererError(const QString &errorMsg)
{
    if (m_state != Inactive)
        emitError(errorMsg);
}

void RemoteLinuxApplicationRunner::handleUsedPortsAvailable()
{
    ASSERT_STATE(GatheringPorts);

    if (m_stopRequested) {
        setState(Inactive);
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }

    setState(AdditionalInitializing);
    doAdditionalInitializations();
}

bool RemoteLinuxApplicationRunner::canRun(QString &whyNot) const
{
    if (m_remoteExecutable.isEmpty()) {
        whyNot = tr("No remote executable set.");
        return false;
    }

    if (!m_devConfig) {
        whyNot = tr("No device configuration set.");
        return false;
    }

    return true;
}

void RemoteLinuxApplicationRunner::doAdditionalInitialCleanup()
{
    handleInitialCleanupDone(true);
}

void RemoteLinuxApplicationRunner::doAdditionalInitializations()
{
    handleInitializationsDone(true);
}

void RemoteLinuxApplicationRunner::doAdditionalPostRunCleanup()
{
    handlePostRunCleanupDone();
}

void RemoteLinuxApplicationRunner::handleInitialCleanupDone(bool success)
{
    ASSERT_STATE(AdditionalPreRunCleaning);

    if (m_state != AdditionalPreRunCleaning)
        return;
    if (!success || m_stopRequested) {
        setState(Inactive);
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }

    setState(GatheringPorts);
    m_portsGatherer->start(m_connection, m_devConfig);
}

void RemoteLinuxApplicationRunner::handleInitializationsDone(bool success)
{
    ASSERT_STATE(AdditionalInitializing);

    if (m_state != AdditionalInitializing)
        return;
    if (!success) {
        setState(Inactive);
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }
    if (m_stopRequested) {
        setState(AdditionalPostRunCleaning);
        doAdditionalPostRunCleanup();
        return;
    }

    setState(ReadyForExecution);
    emit readyForExecution();
}

void RemoteLinuxApplicationRunner::handlePostRunCleanupDone()
{
    ASSERT_STATE(AdditionalPostRunCleaning);

    const bool wasStopRequested = m_stopRequested;
    setState(Inactive);
    if (wasStopRequested)
        emit remoteProcessFinished(InvalidExitCode);
    else if (m_exitStatus == SshRemoteProcess::ExitedNormally)
        emit remoteProcessFinished(m_runner->exitCode());
    else
        emit error(tr("Error running remote process: %1").arg(m_runner->errorString()));
}

const qint64 RemoteLinuxApplicationRunner::InvalidExitCode = std::numeric_limits<qint64>::min();

} // namespace RemoteLinux

