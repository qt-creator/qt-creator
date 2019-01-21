/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "linuxdevicetester.h"

#include "remotelinux_constants.h"
#include "rsyncdeploystep.h"

#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <utils/port.h>
#include <utils/qtcassert.h>
#include <ssh/sftptransfer.h>
#include <ssh/sshremoteprocess.h>
#include <ssh/sshconnection.h>
#include <ssh/sshconnectionmanager.h>

using namespace ProjectExplorer;
using namespace QSsh;

namespace RemoteLinux {
namespace Internal {
namespace {

enum State { Inactive, Connecting, RunningUname, TestingPorts, TestingSftp, TestingRsync };

} // anonymous namespace

class GenericLinuxDeviceTesterPrivate
{
public:
    IDevice::Ptr deviceConfiguration;
    SshConnection *connection = nullptr;
    SshRemoteProcessPtr process;
    DeviceUsedPortsGatherer portsGatherer;
    SftpTransferPtr sftpTransfer;
    SshProcess rsyncProcess;
    State state = Inactive;
    bool sftpWorks = false;
};

} // namespace Internal

using namespace Internal;

GenericLinuxDeviceTester::GenericLinuxDeviceTester(QObject *parent)
    : DeviceTester(parent), d(new GenericLinuxDeviceTesterPrivate)
{
}

GenericLinuxDeviceTester::~GenericLinuxDeviceTester()
{
    if (d->connection)
        releaseConnection(d->connection);
    delete d;
}

void GenericLinuxDeviceTester::testDevice(const IDevice::Ptr &deviceConfiguration)
{
    QTC_ASSERT(d->state == Inactive, return);

    d->deviceConfiguration = deviceConfiguration;
    forceNewConnection(deviceConfiguration->sshParameters());
    d->connection = acquireConnection(deviceConfiguration->sshParameters());
    connect(d->connection, &SshConnection::connected,
            this, &GenericLinuxDeviceTester::handleConnected);
    connect(d->connection, &SshConnection::errorOccurred,
            this, &GenericLinuxDeviceTester::handleConnectionFailure);

    emit progressMessage(tr("Connecting to host..."));
    d->state = Connecting;
    d->connection->connectToHost();
}

void GenericLinuxDeviceTester::stopTest()
{
    QTC_ASSERT(d->state != Inactive, return);

    switch (d->state) {
    case Connecting:
        d->connection->disconnectFromHost();
        break;
    case TestingPorts:
        d->portsGatherer.stop();
        break;
    case RunningUname:
        d->process->close();
        break;
    case TestingSftp:
        d->sftpTransfer->stop();
        break;
    case TestingRsync:
        d->rsyncProcess.disconnect();
        d->rsyncProcess.kill();
    case Inactive:
        break;
    }

    setFinished(TestFailure);
}

void GenericLinuxDeviceTester::handleConnected()
{
    QTC_ASSERT(d->state == Connecting, return);

    d->process = d->connection->createRemoteProcess("uname -rsm");
    connect(d->process.get(), &SshRemoteProcess::done,
            this, &GenericLinuxDeviceTester::handleProcessFinished);

    emit progressMessage(tr("Checking kernel version..."));
    d->state = RunningUname;
    d->process->start();
}

void GenericLinuxDeviceTester::handleConnectionFailure()
{
    QTC_ASSERT(d->state != Inactive, return);

    emit errorMessage(d->connection->errorString() + QLatin1Char('\n'));

    setFinished(TestFailure);
}

void GenericLinuxDeviceTester::handleProcessFinished(const QString &error)
{
    QTC_ASSERT(d->state == RunningUname, return);

    if (!error.isEmpty() || d->process->exitCode() != 0) {
        const QByteArray stderrOutput = d->process->readAllStandardError();
        if (!stderrOutput.isEmpty())
            emit errorMessage(tr("uname failed: %1").arg(QString::fromUtf8(stderrOutput)) + QLatin1Char('\n'));
        else
            emit errorMessage(tr("uname failed.") + QLatin1Char('\n'));
    } else {
        emit progressMessage(QString::fromUtf8(d->process->readAllStandardOutput()));
    }

    connect(&d->portsGatherer, &DeviceUsedPortsGatherer::error,
            this, &GenericLinuxDeviceTester::handlePortsGatheringError);
    connect(&d->portsGatherer, &DeviceUsedPortsGatherer::portListReady,
            this, &GenericLinuxDeviceTester::handlePortListReady);

    emit progressMessage(tr("Checking if specified ports are available..."));
    d->state = TestingPorts;
    d->portsGatherer.start(d->deviceConfiguration);
}

void GenericLinuxDeviceTester::handlePortsGatheringError(const QString &message)
{
    QTC_ASSERT(d->state == TestingPorts, return);

    emit errorMessage(tr("Error gathering ports: %1").arg(message) + QLatin1Char('\n'));
    setFinished(TestFailure);
}

void GenericLinuxDeviceTester::handlePortListReady()
{
    QTC_ASSERT(d->state == TestingPorts, return);

    if (d->portsGatherer.usedPorts().isEmpty()) {
        emit progressMessage(tr("All specified ports are available.") + QLatin1Char('\n'));
    } else {
        QString portList;
        foreach (const Utils::Port port, d->portsGatherer.usedPorts())
            portList += QString::number(port.number()) + QLatin1String(", ");
        portList.remove(portList.count() - 2, 2);
        emit errorMessage(tr("The following specified ports are currently in use: %1")
            .arg(portList) + QLatin1Char('\n'));
    }

    emit progressMessage(tr("Checking whether an SFTP connection can be set up..."));
    d->sftpTransfer = d->connection->createDownload(FilesToTransfer(),
                                                    FileTransferErrorHandling::Abort);
    connect(d->sftpTransfer.get(), &SftpTransfer::done,
            this, &GenericLinuxDeviceTester::handleSftpFinished);
    d->state = TestingSftp;
    d->sftpTransfer->start();
}

void GenericLinuxDeviceTester::handleSftpStarted()
{
    QTC_ASSERT(d->state == TestingSftp, return);
}

void GenericLinuxDeviceTester::handleSftpFinished(const QString &error)
{
    QTC_ASSERT(d->state == TestingSftp, return);
    if (error.isEmpty()) {
        d->sftpWorks = true;
        emit progressMessage(tr("SFTP service available.\n"));
    } else {
        d->sftpWorks = false;
        emit errorMessage(tr("Error setting up SFTP connection: %1\n").arg(error));
    }
    disconnect(d->sftpTransfer.get(), nullptr, this, nullptr);
    testRsync();
}

void GenericLinuxDeviceTester::testRsync()
{
    emit progressMessage(tr("Checking whether rsync works..."));
    connect(&d->rsyncProcess, &QProcess::errorOccurred, [this] {
        if (d->rsyncProcess.error() == QProcess::FailedToStart)
            handleRsyncFinished();
    });
    connect(&d->rsyncProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this] {
        handleRsyncFinished();
    });
    const RsyncCommandLine cmdLine = RsyncDeployStep::rsyncCommand(*d->connection,
                                                                   RsyncDeployStep::defaultFlags());
    const QStringList args = QStringList(cmdLine.options)
            << "-n" << "--exclude=*" << (cmdLine.remoteHostSpec + ":/tmp");
    d->rsyncProcess.start("rsync", args);
}

void GenericLinuxDeviceTester::handleRsyncFinished()
{
    QString error;
    if (d->rsyncProcess.error() == QProcess::FailedToStart) {
        error = tr("Failed to start rsync: %1\n").arg(d->rsyncProcess.errorString());
    } else if (d->rsyncProcess.exitStatus() == QProcess::CrashExit) {
        error = tr("rsync crashed.\n");
    } else if (d->rsyncProcess.exitCode() != 0) {
        error = tr("rsync failed with exit code %1: %2\n")
                .arg(d->rsyncProcess.exitCode())
                .arg(QString::fromLocal8Bit(d->rsyncProcess.readAllStandardError()));
    }
    TestResult result = TestSuccess;
    if (!error.isEmpty()) {
        emit errorMessage(error);
        if (d->sftpWorks) {
            emit progressMessage(tr("SFTP will be used for deployment, because rsync "
                                    "is not available.\n"));
        } else {
            emit errorMessage(tr("Deployment to this device will not work out of the box.\n"));
            result = TestFailure;
        }
    } else {
        emit progressMessage(tr("rsync is functional.\n"));
    }

    d->deviceConfiguration->setExtraData(Constants::SupportsRSync, error.isEmpty());
    setFinished(result);
}

void GenericLinuxDeviceTester::setFinished(TestResult result)
{
    d->state = Inactive;
    disconnect(&d->portsGatherer, nullptr, this, nullptr);
    if (d->sftpTransfer) {
        disconnect(d->sftpTransfer.get(), nullptr, this, nullptr);
        d->sftpTransfer.release()->deleteLater();
    }
    if (d->connection) {
        disconnect(d->connection, nullptr, this, nullptr);
        releaseConnection(d->connection);
        d->connection = nullptr;
    }
    emit finished(result);
}

} // namespace RemoteLinux
