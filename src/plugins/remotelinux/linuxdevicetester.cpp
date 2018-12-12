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

enum State { Inactive, Connecting, RunningUname, TestingPorts, TestingSftp };

} // anonymous namespace

class GenericLinuxDeviceTesterPrivate
{
public:
    IDevice::ConstPtr deviceConfiguration;
    SshConnection *connection = nullptr;
    SshRemoteProcessPtr process;
    DeviceUsedPortsGatherer portsGatherer;
    SftpTransferPtr sftpUpload;
    State state = Inactive;
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

void GenericLinuxDeviceTester::testDevice(const IDevice::ConstPtr &deviceConfiguration)
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
        d->sftpUpload->stop();
        break;
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
    d->sftpUpload = d->connection->createUpload(FilesToTransfer(),
                                                FileTransferErrorHandling::Abort);
    connect(d->sftpUpload.get(), &SftpTransfer::done,
            this, &GenericLinuxDeviceTester::handleSftpFinished);
    d->state = TestingSftp;
    d->sftpUpload->start();
}

void GenericLinuxDeviceTester::handleSftpFinished(const QString &error)
{
    QTC_ASSERT(d->state == TestingSftp, return);
    if (!error.isEmpty()) {
        emit errorMessage(tr("Error setting up SFTP connection: %1\n").arg(error));
        setFinished(TestFailure);
    } else {
        emit progressMessage(tr("SFTP service available.\n"));
        setFinished(TestSuccess);
    }
}

void GenericLinuxDeviceTester::setFinished(TestResult result)
{
    d->state = Inactive;
    disconnect(&d->portsGatherer, nullptr, this, nullptr);
    if (d->sftpUpload) {
        disconnect(d->sftpUpload.get(), nullptr, this, nullptr);
        d->sftpUpload.release()->deleteLater();
    }
    if (d->connection) {
        disconnect(d->connection, nullptr, this, nullptr);
        releaseConnection(d->connection);
        d->connection = nullptr;
    }
    emit finished(result);
}

} // namespace RemoteLinux
