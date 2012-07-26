/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "remotelinuxapplicationrunner.h"

#include "linuxdeviceconfiguration.h"
#include "remotelinuxrunconfiguration.h"

#include <projectexplorer/target.h>
#include <projectexplorer/profileinformation.h>
#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>

#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <ssh/sshconnection.h>
#include <ssh/sshconnectionmanager.h>
#include <ssh/sshremoteprocess.h>

#include <limits>

using namespace ProjectExplorer;
using namespace QSsh;
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
        : devConfig(DeviceProfileInformation::device(runConfig->target()->profile())),
          remoteExecutable(runConfig->remoteExecutableFilePath()),
          appArguments(runConfig->arguments()),
          commandPrefix(runConfig->commandPrefix()),
          initialFreePorts(devConfig->freePorts()),
          connection(0),
          stopRequested(false),
          state(Inactive)
    { }

    DeviceUsedPortsGatherer portsGatherer;
    IDevice::ConstPtr devConfig;
    const QString remoteExecutable;
    const QString appArguments;
    const QString commandPrefix;
    const PortList initialFreePorts;

    QSsh::SshConnection *connection;
    QSsh::SshRemoteProcess::Ptr runner;
    QSsh::SshRemoteProcess::Ptr cleaner;

    PortList freePorts;
    int exitStatus;
    bool stopRequested;
    State state;

};
} // namespace Internal


using namespace Internal;

AbstractRemoteLinuxApplicationRunner::AbstractRemoteLinuxApplicationRunner(RemoteLinuxRunConfiguration *runConfig,
        QObject *parent)
    : QObject(parent), d(new AbstractRemoteLinuxApplicationRunnerPrivate(runConfig))
{
    connect(&d->portsGatherer, SIGNAL(error(QString)), SLOT(handlePortsGathererError(QString)));
    connect(&d->portsGatherer, SIGNAL(portListReady()), SLOT(handleUsedPortsAvailable()));
}

AbstractRemoteLinuxApplicationRunner::~AbstractRemoteLinuxApplicationRunner()
{
    delete d;
}

SshConnection *AbstractRemoteLinuxApplicationRunner::connection() const
{
    return d->connection;
}

IDevice::ConstPtr AbstractRemoteLinuxApplicationRunner::devConfig() const
{
    return d->devConfig;
}

DeviceUsedPortsGatherer *AbstractRemoteLinuxApplicationRunner::usedPortsGatherer() const
{
    return &d->portsGatherer;
}

PortList *AbstractRemoteLinuxApplicationRunner::freePorts()
{
    return &d->freePorts;
}

QString AbstractRemoteLinuxApplicationRunner::remoteExecutable() const
{
    return d->remoteExecutable;
}

QString AbstractRemoteLinuxApplicationRunner::arguments() const
{
    return d->appArguments;
}

QString AbstractRemoteLinuxApplicationRunner::commandPrefix() const
{
    return d->commandPrefix;
}

void AbstractRemoteLinuxApplicationRunner::start()
{
    QTC_ASSERT(!d->stopRequested && d->state == Inactive, return);

    QString errorMsg;
    if (!canRun(errorMsg)) {
        emitError(tr("Cannot run: %1").arg(errorMsg), true);
        return;
    }

    d->state = SettingUpDevice;
    doDeviceSetup();
}

void AbstractRemoteLinuxApplicationRunner::stop()
{
    if (d->stopRequested)
        return;

    switch (d->state) {
    case Connecting:
        setInactive();
        emit remoteProcessFinished(InvalidExitCode);
        break;
    case GatheringPorts:
        d->portsGatherer.stop();
        setInactive();
        emit remoteProcessFinished(InvalidExitCode);
        break;
    case SettingUpDevice:
    case PreRunCleaning:
    case AdditionalPreRunCleaning:
    case AdditionalInitializing:
    case ProcessStarting:
    case PostRunCleaning:
        d->stopRequested = true; // TODO: We might need stopPreRunCleaning() etc. for the subclasses
        break;
    case ReadyForExecution:
        d->stopRequested = true;
        d->state = PostRunCleaning;
        doPostRunCleanup();
        break;
    case ProcessStarted:
        d->stopRequested = true;
        cleanup();
        break;
    case Inactive:
        break;
    }
}

void AbstractRemoteLinuxApplicationRunner::handleConnected()
{
    QTC_ASSERT(d->state == Connecting, return);

    if (d->stopRequested) {
        emit remoteProcessFinished(InvalidExitCode);
        setInactive();
    } else {
        d->state = PreRunCleaning;
        cleanup();
    }
}

void AbstractRemoteLinuxApplicationRunner::handleConnectionFailure()
{
    QTC_ASSERT(d->state != Inactive, return);

    if (d->state != Connecting || d->state != PreRunCleaning)
        doAdditionalConnectionErrorHandling();

    const QString errorMsg = d->state == Connecting
        ? tr("Could not connect to host: %1") : tr("Connection error: %1");
    emitError(errorMsg.arg(d->connection->errorString()));
}

void AbstractRemoteLinuxApplicationRunner::cleanup()
{
    QTC_ASSERT(d->state == PreRunCleaning
        || (d->state == ProcessStarted && d->stopRequested), return);

    emit reportProgress(tr("Killing remote process(es)..."));
    d->cleaner = d->connection->createRemoteProcess(killApplicationCommandLine().toUtf8());
    connect(d->cleaner.data(), SIGNAL(closed(int)), SLOT(handleCleanupFinished(int)));
    d->cleaner->start();
}

void AbstractRemoteLinuxApplicationRunner::handleCleanupFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::CrashExit
        || exitStatus == SshRemoteProcess::NormalExit);

    QTC_ASSERT(d->state == PreRunCleaning
        || (d->state == ProcessStarted && d->stopRequested) || d->state == Inactive, return);

    if (d->state == Inactive)
        return;
    if (d->stopRequested && d->state == PreRunCleaning) {
        setInactive();
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }
    if (d->stopRequested) {
        d->state = PostRunCleaning;
        doPostRunCleanup();
        return;
    }

    if (exitStatus != SshRemoteProcess::NormalExit) {
        emitError(tr("Initial cleanup failed: %1").arg(d->cleaner->errorString()));
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }

    d->state = AdditionalPreRunCleaning;
    doAdditionalInitialCleanup();
}

void AbstractRemoteLinuxApplicationRunner::startExecution(const QByteArray &remoteCall)
{
    QTC_ASSERT(d->state == ReadyForExecution, return);

    if (d->stopRequested)
        return;

    d->runner = d->connection->createRemoteProcess(remoteCall);
    connect(d->runner.data(), SIGNAL(started()), SLOT(handleRemoteProcessStarted()));
    connect(d->runner.data(), SIGNAL(closed(int)), SLOT(handleRemoteProcessFinished(int)));
    connect(d->runner.data(), SIGNAL(readyReadStandardOutput()), SLOT(handleRemoteStdout()));
    connect(d->runner.data(), SIGNAL(readyReadStandardError()), SLOT(handleRemoteStderr()));
    d->state = ProcessStarting;
    d->runner->start();
}

void AbstractRemoteLinuxApplicationRunner::handleRemoteProcessStarted()
{
    QTC_ASSERT(d->state == ProcessStarting, return);

    d->state = ProcessStarted;
    if (d->stopRequested) {
        cleanup();
        return;
    }

    emit reportProgress(tr("Remote process started."));
    emit remoteProcessStarted();
}

void AbstractRemoteLinuxApplicationRunner::handleRemoteProcessFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::CrashExit
        || exitStatus == SshRemoteProcess::NormalExit);
    QTC_ASSERT(d->state == ProcessStarted || d->state == Inactive, return);

    d->exitStatus = exitStatus;
    if (!d->stopRequested && d->state != Inactive) {
        d->state = PostRunCleaning;
        doPostRunCleanup();
    }
}

void AbstractRemoteLinuxApplicationRunner::setInactive()
{
    d->portsGatherer.stop();
    if (d->connection) {
        disconnect(d->connection, 0, this, 0);
        SshConnectionManager::instance().releaseConnection(d->connection);
        d->connection = 0;
    }
    if (d->cleaner)
        disconnect(d->cleaner.data(), 0, this, 0);
    d->stopRequested = false;
    d->state = Inactive;
}

void AbstractRemoteLinuxApplicationRunner::emitError(const QString &errorMsg, bool force)
{
    if (d->state != Inactive) {
        setInactive();
        emit error(errorMsg);
    } else if (force) {
        emit error(errorMsg);
    }
}

void AbstractRemoteLinuxApplicationRunner::handlePortsGathererError(const QString &errorMsg)
{
    if (d->state != Inactive) {
        if (connection()->errorState() != SshNoError) {
            emitError(errorMsg);
        } else {
            emit reportProgress(tr("Gathering ports failed: %1\nContinuing anyway.").arg(errorMsg));
            handleUsedPortsAvailable();
        }
    }
}

void AbstractRemoteLinuxApplicationRunner::handleUsedPortsAvailable()
{
    QTC_ASSERT(d->state == GatheringPorts, return);

    if (d->stopRequested) {
        setInactive();
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }

    d->state = AdditionalInitializing;
    doAdditionalInitializations();
}

void AbstractRemoteLinuxApplicationRunner::handleRemoteStdout()
{
    emit remoteOutput(d->runner->readAllStandardOutput());
}

void AbstractRemoteLinuxApplicationRunner::handleRemoteStderr()
{
    emit remoteErrorOutput(d->runner->readAllStandardError());
}

bool AbstractRemoteLinuxApplicationRunner::canRun(QString &whyNot) const
{
    if (d->remoteExecutable.isEmpty()) {
        whyNot = tr("No remote executable set.");
        return false;
    }

    if (!d->devConfig) {
        whyNot = tr("No device configuration set.");
        return false;
    }

    return true;
}

void AbstractRemoteLinuxApplicationRunner::handleDeviceSetupDone(bool success)
{
    QTC_ASSERT(d->state == SettingUpDevice, return);

    if (!success || d->stopRequested) {
        setInactive();
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }

    d->connection = SshConnectionManager::instance().acquireConnection(d->devConfig->sshParameters());
    d->state = Connecting;
    d->exitStatus = -1;
    d->freePorts = d->initialFreePorts;
    connect(d->connection, SIGNAL(connected()), SLOT(handleConnected()));
    connect(d->connection, SIGNAL(error(QSsh::SshError)),
        SLOT(handleConnectionFailure()));
    if (d->connection->state() == SshConnection::Connected) {
        handleConnected();
    } else {
        emit reportProgress(tr("Connecting to device..."));
        if (d->connection->state() == QSsh::SshConnection::Unconnected)
            d->connection->connectToHost();
    }
}

void AbstractRemoteLinuxApplicationRunner::handleInitialCleanupDone(bool success)
{
    QTC_ASSERT(d->state == AdditionalPreRunCleaning, return);

    if (!success || d->stopRequested) {
        setInactive();
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }

    d->state = GatheringPorts;
    d->portsGatherer.start(d->devConfig);
}

void AbstractRemoteLinuxApplicationRunner::handleInitializationsDone(bool success)
{
    QTC_ASSERT(d->state == AdditionalInitializing, return);

    if (!success) {
        setInactive();
        emit remoteProcessFinished(InvalidExitCode);
        return;
    }
    if (d->stopRequested) {
        d->state = PostRunCleaning;
        doPostRunCleanup();
        return;
    }

    d->state = ReadyForExecution;
    emit readyForExecution();
}

void AbstractRemoteLinuxApplicationRunner::handlePostRunCleanupDone()
{
    QTC_ASSERT(d->state == PostRunCleaning, return);

    const bool wasStopRequested = d->stopRequested;
    setInactive();
    if (wasStopRequested)
        emit remoteProcessFinished(InvalidExitCode);
    else if (d->exitStatus == SshRemoteProcess::NormalExit)
        emit remoteProcessFinished(d->runner->exitCode());
    else
        emit error(tr("Error running remote process: %1").arg(d->runner->errorString()));
}

QString AbstractRemoteLinuxApplicationRunner::killApplicationCommandLine() const
{
    return QString::fromLatin1("cd /proc; for pid in `ls -d [0123456789]*`; "
        "do "
            "if [ \"`readlink /proc/$pid/exe`\" = \"%1\" ]; then "
            "    kill $pid; sleep 1; kill -9 $pid; "
            "fi; "
        "done").arg(remoteExecutable());
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

} // namespace RemoteLinux

