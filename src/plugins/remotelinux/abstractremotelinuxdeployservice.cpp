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

#include "abstractremotelinuxdeployservice.h"

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
class DeployParameters
{
public:
    DeployParameters(const DeployableFile &d, const QString &h, const QString &s)
        : file(d), host(h), sysroot(s) {}

    bool operator==(const DeployParameters &other) const {
        return file == other.file && host == other.host && sysroot == other.sysroot;
    }

    DeployableFile file;
    QString host;
    QString sysroot;
};
uint qHash(const DeployParameters &p) {
    return qHash(qMakePair(qMakePair(p.file, p.host), p.sysroot));
}

enum State { Inactive, SettingUpDevice, Connecting, Deploying };

// TODO: Just change these...
const char LastDeployedHostsKey[] = "Qt4ProjectManager.MaemoRunConfiguration.LastDeployedHosts";
const char LastDeployedSysrootsKey[] = "Qt4ProjectManager.MaemoRunConfiguration.LastDeployedSysroots";
const char LastDeployedFilesKey[] = "Qt4ProjectManager.MaemoRunConfiguration.LastDeployedFiles";
const char LastDeployedRemotePathsKey[] = "Qt4ProjectManager.MaemoRunConfiguration.LastDeployedRemotePaths";
const char LastDeployedTimesKey[] = "Qt4ProjectManager.MaemoRunConfiguration.LastDeployedTimes";

} // anonymous namespace

class AbstractRemoteLinuxDeployServicePrivate
{
public:
    AbstractRemoteLinuxDeployServicePrivate()
        : kit(0), connection(0), state(Inactive), stopRequested(false) {}

    IDevice::ConstPtr deviceConfiguration;
    QPointer<Target> target;
    Kit *kit;
    SshConnection *connection;
    State state;
    bool stopRequested;

    QHash<DeployParameters, QDateTime> lastDeployed;
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
    return d->kit;
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
    if (!d->target)
        return;
    QString systemRoot;
    if (SysRootKitInformation::hasSysRoot(d->kit))
        systemRoot = SysRootKitInformation::sysRoot(d->kit).toString();
    d->lastDeployed.insert(DeployParameters(deployableFile,
                                            deviceConfiguration()->sshParameters().host,
                                            systemRoot),
                           QDateTime::currentDateTime());
}

bool AbstractRemoteLinuxDeployService::hasChangedSinceLastDeployment(const DeployableFile &deployableFile) const
{
    if (!target())
        return true;
    QString systemRoot;
    if (SysRootKitInformation::hasSysRoot(d->kit))
        systemRoot = SysRootKitInformation::sysRoot(d->kit).toString();
    const QDateTime &lastDeployed = d->lastDeployed.value(DeployParameters(deployableFile,
        deviceConfiguration()->sshParameters().host, systemRoot));
    return !lastDeployed.isValid()
        || deployableFile.localFilePath().toFileInfo().lastModified() > lastDeployed;
}

void AbstractRemoteLinuxDeployService::setTarget(Target *target)
{
    d->target = target;
    if (target)
        d->kit = target->kit();
    else
        d->kit = 0;
    d->deviceConfiguration = DeviceKitInformation::device(d->kit);
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
    QVariantMap map;
    QVariantList hostList;
    QVariantList fileList;
    QVariantList sysrootList;
    QVariantList remotePathList;
    QVariantList timeList;
    typedef QHash<DeployParameters, QDateTime>::ConstIterator DepIt;
    for (DepIt it = d->lastDeployed.begin(); it != d->lastDeployed.end(); ++it) {
        fileList << it.key().file.localFilePath().toString();
        remotePathList << it.key().file.remoteDirectory();
        hostList << it.key().host;
        sysrootList << it.key().sysroot;
        timeList << it.value();
    }
    map.insert(QLatin1String(LastDeployedHostsKey), hostList);
    map.insert(QLatin1String(LastDeployedSysrootsKey), sysrootList);
    map.insert(QLatin1String(LastDeployedFilesKey), fileList);
    map.insert(QLatin1String(LastDeployedRemotePathsKey), remotePathList);
    map.insert(QLatin1String(LastDeployedTimesKey), timeList);
    return map;
}

void AbstractRemoteLinuxDeployService::importDeployTimes(const QVariantMap &map)
{
    const QVariantList &hostList = map.value(QLatin1String(LastDeployedHostsKey)).toList();
    const QVariantList &sysrootList = map.value(QLatin1String(LastDeployedSysrootsKey)).toList();
    const QVariantList &fileList = map.value(QLatin1String(LastDeployedFilesKey)).toList();
    const QVariantList &remotePathList
        = map.value(QLatin1String(LastDeployedRemotePathsKey)).toList();
    const QVariantList &timeList = map.value(QLatin1String(LastDeployedTimesKey)).toList();
    const int elemCount
        = qMin(qMin(qMin(hostList.size(), fileList.size()),
              qMin(remotePathList.size(), timeList.size())), sysrootList.size());
    for (int i = 0; i < elemCount; ++i) {
        const DeployableFile df(fileList.at(i).toString(), remotePathList.at(i).toString());
        d->lastDeployed.insert(DeployParameters(df, hostList.at(i).toString(),
            sysrootList.at(i).toString()), timeList.at(i).toDateTime());
    }
}

void AbstractRemoteLinuxDeployService::handleDeviceSetupDone(bool success)
{
    QTC_ASSERT(d->state == SettingUpDevice, return);

    if (!success || d->stopRequested) {
        setFinished();
        return;
    }

    d->state = Connecting;
    d->connection = SshConnectionManager::instance().acquireConnection(deviceConfiguration()->sshParameters());
    connect(d->connection, SIGNAL(error(QSsh::SshError)),
        SLOT(handleConnectionFailure()));
    if (d->connection->state() == SshConnection::Connected) {
        handleConnected();
    } else {
        connect(d->connection, SIGNAL(connected()), SLOT(handleConnected()));
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
        if (deviceConfiguration()->machineType() == IDevice::Emulator)
            errorMsg += tr("\nDid the emulator fail to start?");
        else
            errorMsg += tr("\nIs the device connected and set up for network access?");
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
        SshConnectionManager::instance().releaseConnection(d->connection);
        d->connection = 0;
    }
    d->stopRequested = false;
    emit finished();
}

} // namespace RemoteLinux
