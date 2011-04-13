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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "maemosshrunner.h"

#include "maemodeploystep.h"
#include "maemoglobal.h"
#include "maemoqemumanager.h"
#include "maemoremotemounter.h"
#include "maemoremotemountsmodel.h"
#include "maemorunconfiguration.h"
#include "maemousedportsgatherer.h"

#include <utils/ssh/sshconnection.h>
#include <utils/ssh/sshremoteprocess.h>

#include <QtCore/QFileInfo>

#include <limits>

#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(State, state, m_state)

using namespace Utils;

namespace Qt4ProjectManager {
namespace Internal {

MaemoSshRunner::MaemoSshRunner(QObject *parent,
    MaemoRunConfiguration *runConfig, bool debugging)
    : QObject(parent),
      m_mounter(new MaemoRemoteMounter(this)),
      m_portsGatherer(new MaemoUsedPortsGatherer(this)),
      m_devConfig(runConfig->deviceConfig()),
      m_remoteExecutable(runConfig->remoteExecutableFilePath()),
      m_appArguments(runConfig->arguments()),
      m_userEnvChanges(runConfig->userEnvironmentChanges()),
      m_initialFreePorts(runConfig->freePorts()),
      m_mountSpecs(runConfig->remoteMounts()->mountSpecs()),
      m_state(Inactive)
{
    m_connection = runConfig->deployStep()->sshConnection();
    m_mounter->setBuildConfiguration(runConfig->activeQt4BuildConfiguration());
    if (debugging && runConfig->useRemoteGdb()) {
        m_mountSpecs << MaemoMountSpecification(runConfig->localDirToMountForRemoteGdb(),
            runConfig->remoteProjectSourcesMountPoint());
    }

    m_procsToKill << QFileInfo(m_remoteExecutable).fileName();
    connect(m_mounter, SIGNAL(mounted()), this, SLOT(handleMounted()));
    connect(m_mounter, SIGNAL(unmounted()), this, SLOT(handleUnmounted()));
    connect(m_mounter, SIGNAL(error(QString)), this,
        SLOT(handleMounterError(QString)));
    connect(m_mounter, SIGNAL(reportProgress(QString)), this,
        SIGNAL(reportProgress(QString)));
    connect(m_mounter, SIGNAL(debugOutput(QString)), this,
        SIGNAL(mountDebugOutput(QString)));
    connect(m_portsGatherer, SIGNAL(error(QString)), this,
        SLOT(handlePortsGathererError(QString)));
    connect(m_portsGatherer, SIGNAL(portListReady()), this,
        SLOT(handleUsedPortsAvailable()));
}

MaemoSshRunner::~MaemoSshRunner() {}

void MaemoSshRunner::start()
{
    ASSERT_STATE(QList<State>() << Inactive << StopRequested);

    if (m_remoteExecutable.isEmpty()) {
        emitError(tr("Cannot run: No remote executable set."), true);
        return;
    }
    if (!m_devConfig) {
        emitError(tr("Cannot run: No device configuration set."), true);
        return;
    }

    if (m_devConfig->type() == MaemoDeviceConfig::Emulator
            && !MaemoQemuManager::instance().qemuIsRunning()) {
        MaemoQemuManager::instance().startRuntime();
        emitError(tr("Cannot run: Qemu was not running. "
            "It has now been started up for you, but it will take "
            "a bit of time until it is ready."), true);
        return;
    }

    setState(Connecting);
    m_exitStatus = -1;
    m_freePorts = m_initialFreePorts;
    if (m_connection)
        disconnect(m_connection.data(), 0, this, 0);
    const bool reUse = isConnectionUsable();
    if (!reUse)
        m_connection = SshConnection::create();
    connect(m_connection.data(), SIGNAL(connected()), this,
        SLOT(handleConnected()));
    connect(m_connection.data(), SIGNAL(error(Utils::SshError)), this,
        SLOT(handleConnectionFailure()));
    if (reUse) {
        handleConnected();
    } else {
        emit reportProgress(tr("Connecting to device..."));
        m_connection->connectToHost(m_devConfig->sshParameters());
    }
}

void MaemoSshRunner::stop()
{
    if (m_state == PostRunCleaning || m_state == StopRequested
        || m_state == Inactive)
        return;
    if (m_state == Connecting) {
        setState(Inactive);
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }

    setState(StopRequested);
    cleanup();
}

void MaemoSshRunner::handleConnected()
{
    ASSERT_STATE(QList<State>() << Connecting << StopRequested);
    if (m_state == StopRequested) {
        setState(Inactive);
    } else {
        setState(PreRunCleaning);
        cleanup();
    }
}

void MaemoSshRunner::handleConnectionFailure()
{
    if (m_state == Inactive)
        qWarning("Unexpected state %d in %s.", m_state, Q_FUNC_INFO);

    const QString errorMsg = m_state == Connecting
        ? MaemoGlobal::failedToConnectToServerMessage(m_connection, m_devConfig)
        : tr("Connection error: %1").arg(m_connection->errorString());
    emitError(errorMsg);
}

void MaemoSshRunner::cleanup()
{
    ASSERT_STATE(QList<State>() << PreRunCleaning << PostRunCleaning
        << StopRequested);

    emit reportProgress(tr("Killing remote process(es)..."));

    // pkill behaves differently on Fremantle and Harmattan.
    const char *const killTemplate = "pkill -%2 '^%1$'; pkill -%2 '/%1$';";
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

void MaemoSshRunner::handleCleanupFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    ASSERT_STATE(QList<State>() << PreRunCleaning << PostRunCleaning
        << StopRequested << Inactive);

    if (m_state == Inactive)
        return;
    if (m_state == StopRequested || m_state == PostRunCleaning) {
        unmount();
        return;
    }

    if (exitStatus != SshRemoteProcess::ExitedNormally) {
        emitError(tr("Initial cleanup failed: %1")
            .arg(m_cleaner->errorString()));
    } else {
        m_mounter->setConnection(m_connection);
        unmount();
    }
}

void MaemoSshRunner::handleUnmounted()
{
    ASSERT_STATE(QList<State>() << PreRunCleaning << PreMountUnmounting
        << PostRunCleaning << StopRequested);

    switch (m_state) {
    case PreRunCleaning: {
        for (int i = 0; i < m_mountSpecs.count(); ++i)
            m_mounter->addMountSpecification(m_mountSpecs.at(i), false);
        setState(PreMountUnmounting);
        unmount();
        break;
    }
    case PreMountUnmounting:
        setState(GatheringPorts);
        m_portsGatherer->start(m_connection, m_freePorts);
        break;
    case PostRunCleaning:
    case StopRequested: {
        m_mounter->resetMountSpecifications();
        const bool stopRequested = m_state == StopRequested;
        setState(Inactive);
        if (stopRequested) {
            emit remoteProcessFinished(InvalidExitCode);
        } else if (m_exitStatus == SshRemoteProcess::ExitedNormally) {
            emit remoteProcessFinished(m_runner->exitCode());
        } else {
            emit error(tr("Error running remote process: %1")
                .arg(m_runner->errorString()));
        }
        break;
    }
    default: ;
    }
}

void MaemoSshRunner::handleMounted()
{
    ASSERT_STATE(QList<State>() << Mounting << StopRequested);

    if (m_state == Mounting) {
        setState(ReadyForExecution);
        emit readyForExecution();
    }
}

void MaemoSshRunner::handleMounterError(const QString &errorMsg)
{
    ASSERT_STATE(QList<State>() << PreRunCleaning << PostRunCleaning
        << PreMountUnmounting << Mounting << StopRequested << Inactive);

    emitError(errorMsg);
}

void MaemoSshRunner::startExecution(const QByteArray &remoteCall)
{
    ASSERT_STATE(ReadyForExecution);

    m_runner = m_connection->createRemoteProcess(remoteCall);
    connect(m_runner.data(), SIGNAL(started()), this,
        SIGNAL(remoteProcessStarted()));
    connect(m_runner.data(), SIGNAL(closed(int)), this,
        SLOT(handleRemoteProcessFinished(int)));
    connect(m_runner.data(), SIGNAL(outputAvailable(QByteArray)), this,
        SIGNAL(remoteOutput(QByteArray)));
    connect(m_runner.data(), SIGNAL(errorOutputAvailable(QByteArray)), this,
        SIGNAL(remoteErrorOutput(QByteArray)));
    setState(ProcessStarting);
    m_runner->start();
}

void MaemoSshRunner::handleRemoteProcessFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);
    ASSERT_STATE(QList<State>() << ProcessStarting << StopRequested << Inactive);

    m_exitStatus = exitStatus;
    if (m_state != StopRequested && m_state != Inactive) {
        setState(PostRunCleaning);
        cleanup();
    }
}

bool MaemoSshRunner::isConnectionUsable() const
{
    return m_connection && m_connection->state() == SshConnection::Connected
        && m_connection->connectionParameters() == m_devConfig->sshParameters();
}

void MaemoSshRunner::setState(State newState)
{
    if (newState == Inactive) {
        m_mounter->setConnection(SshConnection::Ptr());
        m_portsGatherer->stop();
        if (m_connection) {
            disconnect(m_connection.data(), 0, this, 0);
            m_connection = SshConnection::Ptr();
        }
        if (m_cleaner)
            disconnect(m_cleaner.data(), 0, this, 0);
    }
    m_state = newState;
}

void MaemoSshRunner::emitError(const QString &errorMsg, bool force)
{
    if (m_state != Inactive) {
        setState(Inactive);
        emit error(errorMsg);
    } else if (force) {
        emit error(errorMsg);
    }
}

void MaemoSshRunner::mount()
{
    setState(Mounting);
    if (m_mounter->hasValidMountSpecifications()) {
        emit reportProgress(tr("Mounting host directories..."));
        m_mounter->mount(freePorts(), m_portsGatherer);
    } else {
        handleMounted();
    }
}

void MaemoSshRunner::unmount()
{
    ASSERT_STATE(QList<State>() << PreRunCleaning << PreMountUnmounting
        << PostRunCleaning << StopRequested);
    if (m_mounter->hasValidMountSpecifications()) {
        QString message;
        switch (m_state) {
        case PreRunCleaning:
            message = tr("Unmounting left-over host directory mounts...");
            break;
        case PreMountUnmounting:
            message = tr("Potentially unmounting left-over host directory mounts...");
        case StopRequested: case PostRunCleaning:
            message = tr("Unmounting host directories...");
            break;
        default:
            break;
        }
        emit reportProgress(message);
        m_mounter->unmount();
    } else {
        handleUnmounted();
    }
}

void MaemoSshRunner::handlePortsGathererError(const QString &errorMsg)
{
    emitError(errorMsg);
}

void MaemoSshRunner::handleUsedPortsAvailable()
{
    ASSERT_STATE(QList<State>() << GatheringPorts << StopRequested);

    if (m_state == StopRequested) {
        setState(Inactive);
    } else {
        mount();
    }
}

const qint64 MaemoSshRunner::InvalidExitCode
    = std::numeric_limits<qint64>::min();

} // namespace Internal
} // namespace Qt4ProjectManager

