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

    SshRemoteProcessRunner::Ptr procRunner;
    PortList portsToCheck;
    QList<int> usedPorts;
    QByteArray remoteStdout;
    QByteArray remoteStderr;
    bool running;
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
    d->procRunner = SshRemoteProcessRunner::create(connection);
    connect(d->procRunner.data(), SIGNAL(connectionError(Utils::SshError)),
        SLOT(handleConnectionError()));
    connect(d->procRunner.data(), SIGNAL(processClosed(int)),
        SLOT(handleProcessClosed(int)));
    connect(d->procRunner.data(), SIGNAL(processOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdOut(QByteArray)));
    connect(d->procRunner.data(), SIGNAL(processErrorOutputAvailable(QByteArray)),
        SLOT(handleRemoteStdErr(QByteArray)));
    const QString command = QLatin1String("sed "
        "'s/.*: [[:xdigit:]]\\{8\\}:\\([[:xdigit:]]\\{4\\}\\).*/\\1/g' /proc/net/tcp");
    d->procRunner->run(command.toUtf8());
    d->running = true;
}

void RemoteLinuxUsedPortsGatherer::stop()
{
    if (!d->running)
        return;
    d->running = false;
    disconnect(d->procRunner->connection().data(), 0, this, 0);
    if (d->procRunner->process())
        d->procRunner->process()->closeChannel();
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
    emit error(tr("Connection error: %1").
        arg(d->procRunner->connection()->errorString()));
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
            .arg(d->procRunner->process()->errorString());
        break;
    case SshRemoteProcess::KilledBySignal:
        errMsg = tr("Remote process crashed: %1")
            .arg(d->procRunner->process()->errorString());
        break;
    case SshRemoteProcess::ExitedNormally:
        if (d->procRunner->process()->exitCode() == 0) {
            setupUsedPorts();
        } else {
            errMsg = tr("Remote process failed; exit code was %1.")
                .arg(d->procRunner->process()->exitCode());
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
