/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
#include "remotelinuxusedportsgatherer.h"

#include "linuxdeviceconfiguration.h"

#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <ssh/sshconnection.h>
#include <ssh/sshconnectionmanager.h>
#include <ssh/sshremoteprocess.h>

#include <QString>

using namespace QSsh;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxUsedPortsGathererPrivate
{
 public:
    SshConnection *connection;
    SshRemoteProcess::Ptr process;
    PortList portsToCheck;
    QList<int> usedPorts;
    QByteArray remoteStdout;
    QByteArray remoteStderr;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxUsedPortsGatherer::RemoteLinuxUsedPortsGatherer(QObject *parent) :
    QObject(parent), d(new RemoteLinuxUsedPortsGathererPrivate)
{
    d->connection = 0;
}

RemoteLinuxUsedPortsGatherer::~RemoteLinuxUsedPortsGatherer()
{
    stop();
    delete d;
}

void RemoteLinuxUsedPortsGatherer::start(const LinuxDeviceConfiguration::ConstPtr &devConf)
{
    QTC_ASSERT(!d->connection, return);
    d->portsToCheck = devConf->freePorts();
    d->connection = SshConnectionManager::instance().acquireConnection(devConf->sshParameters());
    connect(d->connection, SIGNAL(error(QSsh::SshError)), SLOT(handleConnectionError()));
    if (d->connection->state() == SshConnection::Connected) {
        handleConnectionEstablished();
        return;
    }
    connect(d->connection, SIGNAL(connected()), SLOT(handleConnectionEstablished()));
    if (d->connection->state() == SshConnection::Unconnected)
        d->connection->connectToHost();
}

void RemoteLinuxUsedPortsGatherer::handleConnectionEstablished()
{
    QString procFilePath;
    int addressLength;
    if (d->connection->connectionInfo().localAddress.protocol() == QAbstractSocket::IPv4Protocol) {
        procFilePath = QLatin1String("/proc/net/tcp");
        addressLength = 8;
    } else {
        procFilePath = QLatin1String("/proc/net/tcp6");
        addressLength = 32;
    }
    const QString command = QString::fromLatin1("sed "
        "'s/.*: [[:xdigit:]]\\{%1\\}:\\([[:xdigit:]]\\{4\\}\\).*/\\1/g' %2")
        .arg(addressLength).arg(procFilePath);
    d->process = d->connection->createRemoteProcess(command.toUtf8());

    connect(d->process.data(), SIGNAL(closed(int)), SLOT(handleProcessClosed(int)));
    connect(d->process.data(), SIGNAL(readyReadStandardOutput()), SLOT(handleRemoteStdOut()));
    connect(d->process.data(), SIGNAL(readyReadStandardError()), SLOT(handleRemoteStdErr()));

    d->process->start();
}

void RemoteLinuxUsedPortsGatherer::stop()
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

int RemoteLinuxUsedPortsGatherer::getNextFreePort(PortList *freePorts) const
{
    while (freePorts->hasMore()) {
        const int port = freePorts->getNext();
        if (!d->usedPorts.contains(port))
            return port;
    }
    return -1;
}

QList<int> RemoteLinuxUsedPortsGatherer::usedPorts() const
{
    return d->usedPorts;
}

void RemoteLinuxUsedPortsGatherer::setupUsedPorts()
{
    QList<QByteArray> portStrings = d->remoteStdout.split('\n');
    portStrings.removeFirst();
    foreach (const QByteArray &portString, portStrings) {
        if (portString.isEmpty())
            continue;
        bool ok;
        const int port = portString.toInt(&ok, 16);
        if (ok) {
            if (d->portsToCheck.contains(port) && !d->usedPorts.contains(port))
                d->usedPorts << port;
        } else {
            qWarning("%s: Unexpected string '%s' is not a port.",
                Q_FUNC_INFO, portString.data());
        }
    }
    emit portListReady();
}

void RemoteLinuxUsedPortsGatherer::handleConnectionError()
{
    if (!d->connection)
        return;
    emit error(tr("Connection error: %1").arg(d->connection->errorString()));
    stop();
}

void RemoteLinuxUsedPortsGatherer::handleProcessClosed(int exitStatus)
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

void RemoteLinuxUsedPortsGatherer::handleRemoteStdOut()
{
    if (d->process)
        d->remoteStdout += d->process->readAllStandardOutput();
}

void RemoteLinuxUsedPortsGatherer::handleRemoteStdErr()
{
    if (d->process)
        d->remoteStderr += d->process->readAllStandardError();
}

} // namespace RemoteLinux
