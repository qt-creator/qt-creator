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

#include "abstractremotelinuxdeployservice.h"
#include "deploymenttimeinfo.h"

#include <projectexplorer/deployablefile.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>
#include <ssh/sshconnection.h>
#include <ssh/sshconnectionmanager.h>

#include <QDateTime>
#include <QFileInfo>
#include <QPointer>
#include <QString>

using namespace ProjectExplorer;
using namespace QSsh;

namespace RemoteLinux {
namespace Internal {

namespace {
enum State { Inactive, SettingUpDevice, Connecting, Deploying };
} // anonymous namespace

class AbstractRemoteLinuxDeployServicePrivate
{
public:
    AbstractRemoteLinuxDeployServicePrivate()
        : connection(0), state(Inactive), stopRequested(false) {}

    IDevice::ConstPtr deviceConfiguration;
    QPointer<Target> target;

    DeploymentTimeInfo deployTimes;
    SshConnection *connection;
    State state;
    bool stopRequested;
};
} // namespace Internal

using namespace Internal;

AbstractRemoteLinuxDeployService::AbstractRemoteLinuxDeployService(QObject *parent)
    : QObject(parent), d(new AbstractRemoteLinuxDeployServicePrivate)
{
}

AbstractRemoteLinuxDeployService::~AbstractRemoteLinuxDeployService()
{
    delete d;
}

const Target *AbstractRemoteLinuxDeployService::target() const
{
    return d->target;
}

const Kit *AbstractRemoteLinuxDeployService::profile() const
{
    return d->target ? d->target->kit() : nullptr;
}

IDevice::ConstPtr AbstractRemoteLinuxDeployService::deviceConfiguration() const
{
    return d->deviceConfiguration;
}

SshConnection *AbstractRemoteLinuxDeployService::connection() const
{
    return d->connection;
}

void AbstractRemoteLinuxDeployService::saveDeploymentTimeStamp(const DeployableFile &deployableFile)
{
    d->deployTimes.saveDeploymentTimeStamp(deployableFile, profile());
}

bool AbstractRemoteLinuxDeployService::hasChangedSinceLastDeployment(const DeployableFile &deployableFile) const
{
    return d->deployTimes.hasChangedSinceLastDeployment(deployableFile, profile());
}

void AbstractRemoteLinuxDeployService::setTarget(Target *target)
{
    d->target = target;
    d->deviceConfiguration = DeviceKitInformation::device(profile());
}

void AbstractRemoteLinuxDeployService::setDevice(const IDevice::ConstPtr &device)
{
    d->deviceConfiguration = device;
}

void AbstractRemoteLinuxDeployService::start()
{
    QTC_ASSERT(d->state == Inactive, return);

    QString errorMsg;
    if (!isDeploymentPossible(&errorMsg)) {
        emit errorMessage(errorMsg);
        emit finished();
        return;
    }

    if (!isDeploymentNecessary()) {
        emit progressMessage(tr("No deployment action necessary. Skipping."));
        emit finished();
        return;
    }

    d->state = SettingUpDevice;
    doDeviceSetup();
}

void AbstractRemoteLinuxDeployService::stop()
{
    if (d->stopRequested)
        return;

    switch (d->state) {
    case Inactive:
        break;
    case SettingUpDevice:
        d->stopRequested = true;
        stopDeviceSetup();
        break;
    case Connecting:
        setFinished();
        break;
    case Deploying:
        d->stopRequested = true;
        stopDeployment();
        break;
    }
}

bool AbstractRemoteLinuxDeployService::isDeploymentPossible(QString *whyNot) const
{
    if (!deviceConfiguration()) {
        if (whyNot)
            *whyNot = tr("No device configuration set.");
        return false;
    }
    return true;
}

QVariantMap AbstractRemoteLinuxDeployService::exportDeployTimes() const
{
    return d->deployTimes.exportDeployTimes();
}

void AbstractRemoteLinuxDeployService::importDeployTimes(const QVariantMap &map)
{
    d->deployTimes.importDeployTimes(map);
}

void AbstractRemoteLinuxDeployService::handleDeviceSetupDone(bool success)
{
    QTC_ASSERT(d->state == SettingUpDevice, return);

    if (!success || d->stopRequested) {
        setFinished();
        return;
    }

    d->state = Connecting;
    d->connection = QSsh::acquireConnection(deviceConfiguration()->sshParameters());
    connect(d->connection, &SshConnection::error,
            this, &AbstractRemoteLinuxDeployService::handleConnectionFailure);
    if (d->connection->state() == SshConnection::Connected) {
        handleConnected();
    } else {
        connect(d->connection, &SshConnection::connected,
                this, &AbstractRemoteLinuxDeployService::handleConnected);
        emit progressMessage(tr("Connecting to device..."));
        if (d->connection->state() == SshConnection::Unconnected)
            d->connection->connectToHost();
    }
}

void AbstractRemoteLinuxDeployService::handleDeploymentDone()
{
    QTC_ASSERT(d->state == Deploying, return);

    setFinished();
}

void AbstractRemoteLinuxDeployService::handleConnected()
{
    QTC_ASSERT(d->state == Connecting, return);

    if (d->stopRequested) {
        setFinished();
        return;
    }

    d->state = Deploying;
    doDeploy();
}

void AbstractRemoteLinuxDeployService::handleConnectionFailure()
{
    switch (d->state) {
    case Inactive:
    case SettingUpDevice:
        qWarning("%s: Unexpected state %d.", Q_FUNC_INFO, d->state);
        break;
    case Connecting: {
        QString errorMsg = tr("Could not connect to host: %1").arg(d->connection->errorString());
        errorMsg += QLatin1Char('\n');
        if (deviceConfiguration()->machineType() == IDevice::Emulator)
            errorMsg += tr("Did the emulator fail to start?");
        else
            errorMsg += tr("Is the device connected and set up for network access?");
        emit errorMessage(errorMsg);
        setFinished();
        break;
    }
    case Deploying:
        emit errorMessage(tr("Connection error: %1").arg(d->connection->errorString()));
        stopDeployment();
    }
}

void AbstractRemoteLinuxDeployService::setFinished()
{
    d->state = Inactive;
    if (d->connection) {
        disconnect(d->connection, 0, this, 0);
        QSsh::releaseConnection(d->connection);
        d->connection = 0;
    }
    d->stopRequested = false;
    emit finished();
}

} // namespace RemoteLinux
