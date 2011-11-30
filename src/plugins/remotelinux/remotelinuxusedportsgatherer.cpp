/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include "portlist.h"

#include <utils/ssh/sshremoteprocessrunner.h>

#include <QtCore/QString>

using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxUsedPortsGathererPrivate
{
 public:
    RemoteLinuxUsedPortsGathererPrivate() : running(false) {}

    SshRemoteProcessRunner procRunner;
    PortList portsToCheck;
    QList<int> usedPorts;
    QByteArray remoteStdout;
    QByteArray remoteStderr;
    bool running; // TODO: Redundant due to being in sync with procRunner?
};

} // namespace Internal

using namespace Internal;

RemoteLinuxUsedPortsGatherer::RemoteLinuxUsedPortsGatherer(QObject *parent) :
    QObject(parent), d(new RemoteLinuxUsedPortsGathererPrivate)
{
}

RemoteLinuxUsedPortsGatherer::~RemoteLinuxUsedPortsGatherer()
{
    delete d;
}

void RemoteLinuxUsedPortsGatherer::start(const Utils::SshConnection::Ptr &connection,
    const LinuxDeviceConfiguration::ConstPtr &devConf)
{
    if (d->running)
        qWarning("Unexpected call of %s in running state", Q_FUNC_INFO);
    d->portsToCheck = devConf->freePorts();
    d->usedPorts.clear();
    d->remoteStdout.clear();
    d->remoteStderr.clear();
    connect(&d->procRunner, SIGNAL(connectionError()), SLOT(handleConnectionError()));
    connect(&d->procRunner, SIGNAL(processClosed(int)), SLOT(handleProcessClosed(int)));
    connect(&d->procRunner, SIGNAL(processOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdOut(QByteArray)));
    connect(&d->procRunner, SIGNAL(processErrorOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdErr(QByteArray)));
    QString procFilePath;
    int addressLength;
    if (connection->ipProtocolVersion() == QAbstractSocket::IPv4Protocol) {
        procFilePath = QLatin1String("/proc/net/tcp");
        addressLength = 8;
    } else {
        procFilePath = QLatin1String("/proc/net/tcp6");
        addressLength = 32;
    }
    const QString command = QString::fromLocal8Bit("sed "
        "'s/.*: [[:xdigit:]]\\{%1\\}:\\([[:xdigit:]]\\{4\\}\\).*/\\1/g' %2")
        .arg(addressLength).arg(procFilePath);

    // TODO: We should not use an SshRemoteProcessRunner here, because we have to check
    // for the type of the connection before we can say what the exact command line is.
    d->procRunner.run(command.toUtf8(), connection->connectionParameters());
    d->running = true;
}

void RemoteLinuxUsedPortsGatherer::stop()
{
    if (!d->running)
        return;
    d->running = false;
    disconnect(&d->procRunner, 0, this, 0);
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
    if (!d->running)
        return;
    emit error(tr("Connection error: %1").arg(d->procRunner.lastConnectionErrorString()));
    stop();
}

void RemoteLinuxUsedPortsGatherer::handleProcessClosed(int exitStatus)
{
    if (!d->running)
        return;
    QString errMsg;
    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        errMsg = tr("Could not start remote process: %1")
            .arg(d->procRunner.processErrorString());
        break;
    case SshRemoteProcess::KilledBySignal:
        errMsg = tr("Remote process crashed: %1")
            .arg(d->procRunner.processErrorString());
        break;
    case SshRemoteProcess::ExitedNormally:
        if (d->procRunner.processExitCode() == 0) {
            setupUsedPorts();
        } else {
            errMsg = tr("Remote process failed; exit code was %1.")
                .arg(d->procRunner.processExitCode());
        }
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

void RemoteLinuxUsedPortsGatherer::handleRemoteStdOut(const QByteArray &output)
{
    d->remoteStdout += output;
}

void RemoteLinuxUsedPortsGatherer::handleRemoteStdErr(const QByteArray &output)
{
    d->remoteStderr += output;
}

} // namespace RemoteLinux
