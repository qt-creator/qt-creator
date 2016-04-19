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

#include "deviceusedportsgatherer.h"

#include <utils/port.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <ssh/sshconnection.h>
#include <ssh/sshconnectionmanager.h>
#include <ssh/sshremoteprocess.h>

using namespace QSsh;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class DeviceUsedPortsGathererPrivate
{
 public:
    SshConnection *connection;
    SshRemoteProcess::Ptr process;
    QList<Port> usedPorts;
    QByteArray remoteStdout;
    QByteArray remoteStderr;
    IDevice::ConstPtr device;
};

} // namespace Internal

DeviceUsedPortsGatherer::DeviceUsedPortsGatherer(QObject *parent) :
    QObject(parent), d(new Internal::DeviceUsedPortsGathererPrivate)
{
    d->connection = 0;
}

DeviceUsedPortsGatherer::~DeviceUsedPortsGatherer()
{
    stop();
    delete d;
}

void DeviceUsedPortsGatherer::start(const IDevice::ConstPtr &device)
{
    QTC_ASSERT(!d->connection, return);
    QTC_ASSERT(device && device->portsGatheringMethod(), return);

    d->device = device;
    d->connection = QSsh::acquireConnection(device->sshParameters());
    connect(d->connection, &SshConnection::error,
            this, &DeviceUsedPortsGatherer::handleConnectionError);
    if (d->connection->state() == SshConnection::Connected) {
        handleConnectionEstablished();
        return;
    }
    connect(d->connection, &SshConnection::connected,
            this, &DeviceUsedPortsGatherer::handleConnectionEstablished);
    if (d->connection->state() == SshConnection::Unconnected)
        d->connection->connectToHost();
}

void DeviceUsedPortsGatherer::handleConnectionEstablished()
{
    const QAbstractSocket::NetworkLayerProtocol protocol
            = d->connection->connectionInfo().localAddress.protocol();
    const QByteArray commandLine = d->device->portsGatheringMethod()->commandLine(protocol);
    d->process = d->connection->createRemoteProcess(commandLine);

    connect(d->process.data(), &SshRemoteProcess::closed, this, &DeviceUsedPortsGatherer::handleProcessClosed);
    connect(d->process.data(), &SshRemoteProcess::readyReadStandardOutput, this, &DeviceUsedPortsGatherer::handleRemoteStdOut);
    connect(d->process.data(), &SshRemoteProcess::readyReadStandardError, this, &DeviceUsedPortsGatherer::handleRemoteStdErr);

    d->process->start();
}

void DeviceUsedPortsGatherer::stop()
{
    if (!d->connection)
        return;
    d->usedPorts.clear();
    d->remoteStdout.clear();
    d->remoteStderr.clear();
    if (d->process)
        disconnect(d->process.data(), 0, this, 0);
    d->process.clear();
    disconnect(d->connection, 0, this, 0);
    QSsh::releaseConnection(d->connection);
    d->connection = 0;
}

Port DeviceUsedPortsGatherer::getNextFreePort(PortList *freePorts) const
{
    while (freePorts->hasMore()) {
        const Port port = freePorts->getNext();
        if (!d->usedPorts.contains(port))
            return port;
    }
    return Port();
}

QList<Port> DeviceUsedPortsGatherer::usedPorts() const
{
    return d->usedPorts;
}

void DeviceUsedPortsGatherer::setupUsedPorts()
{
    d->usedPorts.clear();
    const QList<Port> usedPorts = d->device->portsGatheringMethod()->usedPorts(d->remoteStdout);
    foreach (const Port port, usedPorts) {
        if (d->device->freePorts().contains(port))
            d->usedPorts << port;
    }
    emit portListReady();
}

void DeviceUsedPortsGatherer::handleConnectionError()
{
    if (!d->connection)
        return;
    emit error(tr("Connection error: %1").arg(d->connection->errorString()));
    stop();
}

void DeviceUsedPortsGatherer::handleProcessClosed(int exitStatus)
{
    if (!d->connection)
        return;
    QString errMsg;
    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        errMsg = tr("Could not start remote process: %1").arg(d->process->errorString());
        break;
    case SshRemoteProcess::CrashExit:
        errMsg = tr("Remote process crashed: %1").arg(d->process->errorString());
        break;
    case SshRemoteProcess::NormalExit:
        if (d->process->exitCode() == 0)
            setupUsedPorts();
        else
            errMsg = tr("Remote process failed; exit code was %1.").arg(d->process->exitCode());
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid exit status");
    }

    if (!errMsg.isEmpty()) {
        if (!d->remoteStderr.isEmpty()) {
            errMsg += QLatin1Char('\n');
            errMsg += tr("Remote error output was: %1")
                .arg(QString::fromUtf8(d->remoteStderr));
        }
        emit error(errMsg);
    }
    stop();
}

void DeviceUsedPortsGatherer::handleRemoteStdOut()
{
    if (d->process)
        d->remoteStdout += d->process->readAllStandardOutput();
}

void DeviceUsedPortsGatherer::handleRemoteStdErr()
{
    if (d->process)
        d->remoteStderr += d->process->readAllStandardError();
}

} // namespace ProjectExplorer
