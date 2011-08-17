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
#include "portlist.h"
#include "remotelinuxrunconfiguration.h"
#include "remotelinuxusedportsgatherer.h"

#include <utils/qtcassert.h>
#include <utils/ssh/sshconnection.h>
#include <utils/ssh/sshconnectionmanager.h>
#include <utils/ssh/sshremoteprocess.h>

#include <limits>

using namespace Qt4ProjectManager;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {
namespace {

enum State {
    Inactive, SettingUpDevice, Connecting, PreRunCleaning, AdditionalPreRunCleaning,
    GatheringPorts, AdditionalInitializing, ReadyForExecution, ProcessStarting, ProcessStarted,
    PostRunCleaning
};

} // anonymous namespace

class AbstractRemoteLinuxApplicationRunnerPrivate
{
public:
    AbstractRemoteLinuxApplicationRunnerPrivate(const RemoteLinuxRunConfiguration *runConfig)
        : devConfig(runConfig->deviceConfig()),
          remoteExecutable(runConfig->remoteExecutableFilePath()),
          appArguments(runConfig->arguments()),
          commandPrefix(runConfig->commandPrefix()),
          initialFreePorts(runConfig->freePorts()),
          stopRequested(false),
          state(Inactive)
    {
    }

    RemoteLinuxUsedPortsGatherer portsGatherer;
    LinuxDeviceConfiguration::ConstPtr devConfig;
    const QString remoteExecutable;
    const QString appArguments;
    const QString commandPrefix;
    const PortList initialFreePorts;

    Utils::SshConnection::Ptr connection;
    Utils::SshRemoteProcess::Ptr runner;
    Utils::SshRemoteProcess::Ptr cleaner;

    PortList freePorts;
    int exitStatus;
    bool stopRequested;
    State state;

};
} // namespace Internal


using namespace Internal;

AbstractRemoteLinuxApplicationRunner::AbstractRemoteLinuxApplicationRunner(RemoteLinuxRunConfiguration *runConfig,
        QObject *parent)
    : QObject(parent), m_d(new AbstractRemoteLinuxApplicationRunnerPrivate(runConfig))
{
    connect(&m_d->portsGatherer, SIGNAL(error(QString)), SLOT(handlePortsGathererError(QString)));
    connect(&m_d->portsGatherer, SIGNAL(portListReady()), SLOT(handleUsedPortsAvailable()));
}

AbstractRemoteLinuxApplicationRunner::~AbstractRemoteLinuxApplicationRunner()
{
    delete m_d;
}

SshConnection::Ptr AbstractRemoteLinuxApplicationRunner::connection() const
{
    return m_d->connection;
}

LinuxDeviceConfiguration::ConstPtr AbstractRemoteLinuxApplicationRunner::devConfig() const
{
    return m_d->devConfig;
}

const RemoteLinuxUsedPortsGatherer *AbstractRemoteLinuxApplicationRunner::usedPortsGatherer() const
{
    return &m_d->portsGatherer;
}

PortList *AbstractRemoteLinuxApplicationRunner::freePorts()
{
    return &m_d->freePorts;
}

QString AbstractRemoteLinuxApplicationRunner::remoteExecutable() const
{
    return m_d->remoteExecutable;
}

QString AbstractRemoteLinuxApplicationRunner::arguments() const
{
    return m_d->appArguments;
}

QString AbstractRemoteLinuxApplicationRunner::commandPrefix() const
{
    return m_d->commandPrefix;
}

void AbstractRemoteLinuxApplicationRunner::start()
{
    QTC_ASSERT(!m_d->stopRequested && m_d->state == Inactive, return);

    QString errorMsg;
    if (!canRun(errorMsg)) {
        emitError(tr("Cannot run: %1").arg(errorMsg), true);
        return;
    }

    m_d->state = SettingUpDevice;
    doDeviceSetup();
}

void AbstractRemoteLinuxApplicationRunner::stop()
{
    if (m_d->stopRequested)
        return;

    switch (m_d->state) {
    case Connecting:
        setInactive();
        emit remoteProcessFinished(InvalidExitCode);
        break;
    case GatheringPorts:
        m_d->portsGatherer.stop();
        setInactive();
        emit remoteProcessFinished(InvalidExitCode);
        break;
    case SettingUpDevice:
    case PreRunCleaning:
    case AdditionalPreRunCleaning:
    case AdditionalInitializing:
    case ProcessStarting:
    case PostRunCleaning:
        m_d->stopRequested = true; // TODO: We might need stopPreRunCleaning() etc. for the subclasses
        break;
    case ReadyForExecution:
        m_d->stopRequested = true;
        m_d->state = PostRunCleaning;
        doPostRunCleanup();
        break;
    case ProcessStarted:
        m_d->stopRequested = true;
        cleanup();
        break;
    case Inactive:
        break;
    }
}

void AbstractRemoteLinuxApplicationRunner::handleConnected()
{
    QTC_ASSERT(m_d->state == Connecting, return);

    if (m_d->stopRequested) {
        emit remoteProcessFinished(InvalidExitCode);
        setInactive();
    } else {
        m_d->state = PreRunCleaning;
        cleanup();
    }
}

void AbstractRemoteLinuxApplicationRunner::handleConnectionFailure()
{
    QTC_ASSERT(m_d->state != Inactive, return);

    if (m_d->state != Connecting || m_d->state != PreRunCleaning)
        doAdditionalConnectionErrorHandling();

    const QString errorMsg = m_d->state == Connecting
        ? tr("Could not connect to host: %1") : tr("Connection error: %1");
    emitError(errorMsg.arg(m_d->connection->errorString()));
}

void AbstractRemoteLinuxApplicationRunner::cleanup()
{
    QTC_ASSERT(m_d->state == PreRunCleaning
        || (m_d->state == ProcessStarted && m_d->stopRequested), return);

    emit reportProgress(tr("Killing remote process(es)..."));
    m_d->cleaner = m_d->connection->createRemoteProcess(killApplicationCommandLine().toUtf8());
    connect(m_d->cleaner.data(), SIGNAL(closed(int)), SLOT(handleCleanupFinished(int)));
    m_d->cleaner->start();
}

void AbstractRemoteLinuxApplicationRunner::handleCleanupFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    QTC_ASSERT(m_d->state == PreRunCleaning
        || (m_d->state == ProcessStarted && m_d->stopRequested) || m_d->state == Inactive, return);

    if (m_d->state == Inactive)
        return;
    if (m_d->stopRequested && m_d->state == PreRunCleaning) {
        setInactive();
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }
    if (m_d->stopRequested) {
        m_d->state = PostRunCleaning;
        doPostRunCleanup();
        return;
    }

    if (exitStatus != SshRemoteProcess::ExitedNormally) {
        emitError(tr("Initial cleanup failed: %1").arg(m_d->cleaner->errorString()));
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }

    m_d->state = AdditionalPreRunCleaning;
    doAdditionalInitialCleanup();
}

void AbstractRemoteLinuxApplicationRunner::startExecution(const QByteArray &remoteCall)
{
    QTC_ASSERT(m_d->state == ReadyForExecution, return);

    if (m_d->stopRequested)
        return;

    m_d->runner = m_d->connection->createRemoteProcess(remoteCall);
    connect(m_d->runner.data(), SIGNAL(started()), SLOT(handleRemoteProcessStarted()));
    connect(m_d->runner.data(), SIGNAL(closed(int)), SLOT(handleRemoteProcessFinished(int)));
    connect(m_d->runner.data(), SIGNAL(outputAvailable(QByteArray)),
        SIGNAL(remoteOutput(QByteArray)));
    connect(m_d->runner.data(), SIGNAL(errorOutputAvailable(QByteArray)),
        SIGNAL(remoteErrorOutput(QByteArray)));
    m_d->state = ProcessStarting;
    m_d->runner->start();
}

void AbstractRemoteLinuxApplicationRunner::handleRemoteProcessStarted()
{
    QTC_ASSERT(m_d->state == ProcessStarting, return);

    m_d->state = ProcessStarted;
    if (m_d->stopRequested) {
        cleanup();
        return;
    }

    emit reportProgress(tr("Remote process started."));
    emit remoteProcessStarted();
}

void AbstractRemoteLinuxApplicationRunner::handleRemoteProcessFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);
    QTC_ASSERT(m_d->state == ProcessStarted || m_d->state == Inactive, return);

    m_d->exitStatus = exitStatus;
    if (!m_d->stopRequested && m_d->state != Inactive) {
        m_d->state = PostRunCleaning;
        doPostRunCleanup();
    }
}

void AbstractRemoteLinuxApplicationRunner::setInactive()
{
    m_d->portsGatherer.stop();
    if (m_d->connection) {
        disconnect(m_d->connection.data(), 0, this, 0);
        SshConnectionManager::instance().releaseConnection(m_d->connection);
        m_d->connection = SshConnection::Ptr();
    }
    if (m_d->cleaner)
        disconnect(m_d->cleaner.data(), 0, this, 0);
    m_d->stopRequested = false;
    m_d->state = Inactive;
}

void AbstractRemoteLinuxApplicationRunner::emitError(const QString &errorMsg, bool force)
{
    if (m_d->state != Inactive) {
        setInactive();
        emit error(errorMsg);
    } else if (force) {
        emit error(errorMsg);
    }
}

void AbstractRemoteLinuxApplicationRunner::handlePortsGathererError(const QString &errorMsg)
{
    if (m_d->state != Inactive)
        emitError(errorMsg);
}

void AbstractRemoteLinuxApplicationRunner::handleUsedPortsAvailable()
{
    QTC_ASSERT(m_d->state == GatheringPorts, return);

    if (m_d->stopRequested) {
        setInactive();
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }

    m_d->state = AdditionalInitializing;
    doAdditionalInitializations();
}

bool AbstractRemoteLinuxApplicationRunner::canRun(QString &whyNot) const
{
    if (m_d->remoteExecutable.isEmpty()) {
        whyNot = tr("No remote executable set.");
        return false;
    }

    if (!m_d->devConfig) {
        whyNot = tr("No device configuration set.");
        return false;
    }

    return true;
}

void AbstractRemoteLinuxApplicationRunner::setDeviceConfiguration(const LinuxDeviceConfiguration::ConstPtr &deviceConfig)
{
    m_d->devConfig = deviceConfig;
}

void AbstractRemoteLinuxApplicationRunner::handleDeviceSetupDone(bool success)
{
    QTC_ASSERT(m_d->state == SettingUpDevice, return);

    if (!success || m_d->stopRequested) {
        setInactive();
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }

    m_d->connection = SshConnectionManager::instance().acquireConnection(m_d->devConfig->sshParameters());
    m_d->state = Connecting;
    m_d->exitStatus = -1;
    m_d->freePorts = m_d->initialFreePorts;
    connect(m_d->connection.data(), SIGNAL(connected()), SLOT(handleConnected()));
    connect(m_d->connection.data(), SIGNAL(error(Utils::SshError)),
        SLOT(handleConnectionFailure()));
    if (m_d->connection->state() == SshConnection::Connected) {
        handleConnected();
    } else {
        emit reportProgress(tr("Connecting to device..."));
        if (m_d->connection->state() == Utils::SshConnection::Unconnected)
            m_d->connection->connectToHost();
    }
}

void AbstractRemoteLinuxApplicationRunner::handleInitialCleanupDone(bool success)
{
    QTC_ASSERT(m_d->state == AdditionalPreRunCleaning, return);

    if (!success || m_d->stopRequested) {
        setInactive();
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }

    m_d->state = GatheringPorts;
    m_d->portsGatherer.start(m_d->connection, m_d->devConfig);
}

void AbstractRemoteLinuxApplicationRunner::handleInitializationsDone(bool success)
{
    QTC_ASSERT(m_d->state == AdditionalInitializing, return);

    if (!success) {
        setInactive();
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }
    if (m_d->stopRequested) {
        m_d->state = PostRunCleaning;
        doPostRunCleanup();
        return;
    }

    m_d->state = ReadyForExecution;
    emit readyForExecution();
}

void AbstractRemoteLinuxApplicationRunner::handlePostRunCleanupDone()
{
    QTC_ASSERT(m_d->state == PostRunCleaning, return);

    const bool wasStopRequested = m_d->stopRequested;
    setInactive();
    if (wasStopRequested)
        emit remoteProcessFinished(InvalidExitCode);
    else if (m_d->exitStatus == SshRemoteProcess::ExitedNormally)
        emit remoteProcessFinished(m_d->runner->exitCode());
    else
        emit error(tr("Error running remote process: %1").arg(m_d->runner->errorString()));
}

const qint64 AbstractRemoteLinuxApplicationRunner::InvalidExitCode = std::numeric_limits<qint64>::min();


GenericRemoteLinuxApplicationRunner::GenericRemoteLinuxApplicationRunner(RemoteLinuxRunConfiguration *runConfig,
        QObject *parent)
    : AbstractRemoteLinuxApplicationRunner(runConfig, parent)
{
}

GenericRemoteLinuxApplicationRunner::~GenericRemoteLinuxApplicationRunner()
{
}


void GenericRemoteLinuxApplicationRunner::doDeviceSetup()
{
    handleDeviceSetupDone(true);
}

void GenericRemoteLinuxApplicationRunner::doAdditionalInitialCleanup()
{
    handleInitialCleanupDone(true);
}

void GenericRemoteLinuxApplicationRunner::doAdditionalInitializations()
{
    handleInitializationsDone(true);
}

void GenericRemoteLinuxApplicationRunner::doPostRunCleanup()
{
    handlePostRunCleanupDone();
}

void GenericRemoteLinuxApplicationRunner::doAdditionalConnectionErrorHandling()
{
}

QString GenericRemoteLinuxApplicationRunner::killApplicationCommandLine() const
{
    // Prevent pkill from matching our own pkill call.
    QString pkillArg = remoteExecutable();
    const int lastPos = pkillArg.count() - 1;
    pkillArg.replace(lastPos, 1, QLatin1Char('[') + pkillArg.at(lastPos) + QLatin1Char(']'));

    const char * const killTemplate = "pkill -%2 -f %1";
    const QString niceKill = QString::fromLocal8Bit(killTemplate).arg(pkillArg).arg("SIGTERM");
    const QString brutalKill = QString::fromLocal8Bit(killTemplate).arg(pkillArg).arg("SIGKILL");
    return niceKill + QLatin1String("; sleep 1; ") + brutalKill;
}

} // namespace RemoteLinux

