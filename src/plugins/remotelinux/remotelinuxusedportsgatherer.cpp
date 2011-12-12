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

#include <utils/ssh/sshconnectionmanager.h>
#include <utils/ssh/sshremoteprocessrunner.h>

#include <QtCore/QString>

using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxUsedPortsGathererPrivate
{
 public:
    RemoteLinuxUsedPortsGathererPrivate()
    { }

    PortList portsToCheck;
    QList<int> usedPorts;
    QByteArray keepSake;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxUsedPortsGatherer::RemoteLinuxUsedPortsGatherer(
        const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfiguration) :
    SimpleRunner(deviceConfiguration,
                 // Match
                 //  0: 0100007F:706E 00000000:0000 0A 00000000:00000000 00:00000000 00000000     0        0 8141 1 ce498900 300 0 0 2 -1
                 // and report   ^^^^ this local port number
                 QLatin1String("FILE=/proc/net/tcp ; "
                               "echo \"$SSH_CONNECTION\" | grep : > /dev/null && FILE=/proc/net/tcp6 ; "
                               "sed 's/^\\s\\+[0-9a-fA-F]\\+:\\s\\+[0-9a-fA-F]\\+:\\([0-9a-fA-F]\\+\\)\\s.*$/\\1/' $FILE")),
    d(new RemoteLinuxUsedPortsGathererPrivate)
{
    d->portsToCheck = deviceConfiguration->freePorts();

    connect(this, SIGNAL(aboutToStart()), this, SLOT(cleanup()));
}

RemoteLinuxUsedPortsGatherer::~RemoteLinuxUsedPortsGatherer()
{
    delete d;
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

void RemoteLinuxUsedPortsGatherer::handleStdOutput(const QByteArray &output)
{
    QByteArray data = d->keepSake + output;

    // make sure we only process complete lines:
    int lastEol = output.lastIndexOf('\n');
    if (lastEol != output.count() - 1) {
        d->keepSake = data.mid(lastEol + 1);
        data = data.left(lastEol);
    }

    QList<QByteArray> portStrings = data.split('\n');
    foreach (const QByteArray &portString, portStrings) {
        if (portString.isEmpty() || portString.contains("local_address"))
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
}

void RemoteLinuxUsedPortsGatherer::cleanup()
{
    d->keepSake.clear();
    d->usedPorts.clear();
}

int RemoteLinuxUsedPortsGatherer::processFinished(int exitStatus)
{
    if (!d->keepSake.isEmpty()) {
        d->keepSake.append('\n');
        handleStdOutput(d->keepSake);
    }

    if (usedPorts().isEmpty()) {
        emit progressMessage("All specified ports are available.\n");
    } else {
        QString portList;
        foreach (const int port, usedPorts())
            portList += QString::number(port) + QLatin1String(", ");
        portList.remove(portList.count() - 2, 2);
        emit errorMessage(tr("The following specified ports are currently in use: %1\n")
                          .arg(portList));
    }
    return SimpleRunner::processFinished(exitStatus);
}

} // namespace RemoteLinux
