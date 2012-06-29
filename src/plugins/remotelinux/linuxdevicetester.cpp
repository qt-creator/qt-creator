/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
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
#include "linuxdevicetester.h"

#include "linuxdeviceconfiguration.h"
#include "remotelinuxusedportsgatherer.h"

#include <utils/qtcassert.h>
#include <ssh/sshremoteprocess.h>
#include <ssh/sshconnection.h>

using namespace QSsh;

namespace RemoteLinux {
namespace Internal {
namespace {

enum State { Inactive, Connecting, RunningUname, TestingPorts };

} // anonymous namespace

class GenericLinuxDeviceTesterPrivate
{
public:
    GenericLinuxDeviceTesterPrivate() : connection(0), state(Inactive) {}

    LinuxDeviceConfiguration::ConstPtr deviceConfiguration;
    SshConnection *connection;
    SshRemoteProcess::Ptr process;
    RemoteLinuxUsedPortsGatherer portsGatherer;
    State state;
};

} // namespace Internal

using namespace Internal;

AbstractLinuxDeviceTester::AbstractLinuxDeviceTester(QObject *parent) : QObject(parent)
{
}


GenericLinuxDeviceTester::GenericLinuxDeviceTester(QObject *parent)
    : AbstractLinuxDeviceTester(parent), d(new GenericLinuxDeviceTesterPrivate)
{
}

GenericLinuxDeviceTester::~GenericLinuxDeviceTester()
{
    delete d;
}

void GenericLinuxDeviceTester::testDevice(const LinuxDeviceConfiguration::ConstPtr &deviceConfiguration)
{
    QTC_ASSERT(d->state == Inactive, return);

    d->deviceConfiguration = deviceConfiguration;
    d->connection = new SshConnection(deviceConfiguration->sshParameters(), this);
    connect(d->connection, SIGNAL(connected()), SLOT(handleConnected()));
    connect(d->connection, SIGNAL(error(QSsh::SshError)),
        SLOT(handleConnectionFailure()));

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
    case Inactive:
        break;
    }

    setFinished(TestFailure);
}

RemoteLinuxUsedPortsGatherer *GenericLinuxDeviceTester::usedPortsGatherer() const
{
    return &d->portsGatherer;
}

void GenericLinuxDeviceTester::handleConnected()
{
    QTC_ASSERT(d->state == Connecting, return);

    d->process = d->connection->createRemoteProcess("uname -rsm");
    connect(d->process.data(), SIGNAL(closed(int)), SLOT(handleProcessFinished(int)));

    emit progressMessage("Checking kernel version...");
    d->state = RunningUname;
    d->process->start();
}

void GenericLinuxDeviceTester::handleConnectionFailure()
{
    QTC_ASSERT(d->state != Inactive, return);

    emit errorMessage(tr("SSH connection failure: %1\n").arg(d->connection->errorString()));
    setFinished(TestFailure);
}

void GenericLinuxDeviceTester::handleProcessFinished(int exitStatus)
{
    QTC_ASSERT(d->state == RunningUname, return);

    if (exitStatus != SshRemoteProcess::NormalExit || d->process->exitCode() != 0) {
        const QByteArray stderrOutput = d->process->readAllStandardError();
        if (!stderrOutput.isEmpty())
            emit errorMessage(tr("uname failed: %1\n").arg(QString::fromUtf8(stderrOutput)));
        else
            emit errorMessage(tr("uname failed.\n"));
    } else {
        emit progressMessage(QString::fromUtf8(d->process->readAllStandardOutput()));
    }

    connect(&d->portsGatherer, SIGNAL(error(QString)), SLOT(handlePortsGatheringError(QString)));
    connect(&d->portsGatherer, SIGNAL(portListReady()), SLOT(handlePortListReady()));

    emit progressMessage(tr("Checking if specified ports are available..."));
    d->state = TestingPorts;
    d->portsGatherer.start(d->deviceConfiguration);
}

void GenericLinuxDeviceTester::handlePortsGatheringError(const QString &message)
{
    QTC_ASSERT(d->state == TestingPorts, return);

    emit errorMessage(tr("Error gathering ports: %1\n").arg(message));
    setFinished(TestFailure);
}

void GenericLinuxDeviceTester::handlePortListReady()
{
    QTC_ASSERT(d->state == TestingPorts, return);

    if (d->portsGatherer.usedPorts().isEmpty()) {
        emit progressMessage("All specified ports are available.\n");
    } else {
        QString portList;
        foreach (const int port, d->portsGatherer.usedPorts())
            portList += QString::number(port) + QLatin1String(", ");
        portList.remove(portList.count() - 2, 2);
        emit errorMessage(tr("The following specified ports are currently in use: %1\n")
            .arg(portList));
    }
    setFinished(TestSuccess);
}

void GenericLinuxDeviceTester::setFinished(TestResult result)
{
    d->state = Inactive;
    disconnect(&d->portsGatherer, 0, this, 0);
    if (d->connection) {
        disconnect(d->connection, 0, this, 0);
        d->connection->deleteLater();
        d->connection = 0;
    }
    emit finished(result);
}

} // namespace RemoteLinux
