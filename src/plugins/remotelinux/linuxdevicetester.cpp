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

#include "linuxdevicetester.h"

#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <utils/qtcassert.h>
#include <ssh/sshremoteprocess.h>
#include <ssh/sshconnection.h>

using namespace ProjectExplorer;
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

    IDevice::ConstPtr deviceConfiguration;
    SshConnection *connection;
    SshRemoteProcess::Ptr process;
    DeviceUsedPortsGatherer portsGatherer;
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

void GenericLinuxDeviceTester::testDevice(const IDevice::ConstPtr &deviceConfiguration)
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

DeviceUsedPortsGatherer *GenericLinuxDeviceTester::usedPortsGatherer() const
{
    return &d->portsGatherer;
}

void GenericLinuxDeviceTester::handleConnected()
{
    QTC_ASSERT(d->state == Connecting, return);

    d->process = d->connection->createRemoteProcess("uname -rsm");
    connect(d->process.data(), SIGNAL(closed(int)), SLOT(handleProcessFinished(int)));

    emit progressMessage(tr("Checking kernel version..."));
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
        emit progressMessage(tr("All specified ports are available.\n"));
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
