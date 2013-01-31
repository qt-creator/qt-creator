/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "deviceusedportsgatherer.h"

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
    QList<int> usedPorts;
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
    d->connection = SshConnectionManager::instance().acquireConnection(device->sshParameters());
    connect(d->connection, SIGNAL(error(QSsh::SshError)), SLOT(handleConnectionError()));
    if (d->connection->state() == SshConnection::Connected) {
        handleConnectionEstablished();
        return;
    }
    connect(d->connection, SIGNAL(connected()), SLOT(handleConnectionEstablished()));
    if (d->connection->state() == SshConnection::Unconnected)
        d->connection->connectToHost();
}

void DeviceUsedPortsGatherer::handleConnectionEstablished()
{
    const QAbstractSocket::NetworkLayerProtocol protocol
            = d->connection->connectionInfo().localAddress.protocol();
    const QByteArray commandLine = d->device->portsGatheringMethod()->commandLine(protocol);
    d->process = d->connection->createRemoteProcess(commandLine);

    connect(d->process.data(), SIGNAL(closed(int)), SLOT(handleProcessClosed(int)));
    connect(d->process.data(), SIGNAL(readyReadStandardOutput()), SLOT(handleRemoteStdOut()));
    connect(d->process.data(), SIGNAL(readyReadStandardError()), SLOT(handleRemoteStdErr()));

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
    SshConnectionManager::instance().releaseConnection(d->connection);
    d->connection = 0;
}

int DeviceUsedPortsGatherer::getNextFreePort(PortList *freePorts) const
{
    while (freePorts->hasMore()) {
        const int port = freePorts->getNext();
        if (!d->usedPorts.contains(port))
            return port;
    }
    return -1;
}

QList<int> DeviceUsedPortsGatherer::usedPorts() const
{
    return d->usedPorts;
}

void DeviceUsedPortsGatherer::setupUsedPorts()
{
    d->usedPorts.clear();
    const QList<int> usedPorts = d->device->portsGatheringMethod()->usedPorts(d->remoteStdout);
    foreach (const int port, usedPorts) {
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
            errMsg += tr("\nRemote error output was: %1")
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
