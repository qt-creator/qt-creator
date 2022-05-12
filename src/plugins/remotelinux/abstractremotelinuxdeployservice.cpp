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
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>

#include <QDateTime>
#include <QFileInfo>
#include <QPointer>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

namespace {
enum State { Inactive, SettingUpDevice, Deploying };
} // anonymous namespace

class AbstractRemoteLinuxDeployServicePrivate
{
public:
    IDevice::ConstPtr deviceConfiguration;
    QPointer<Target> target;

    DeploymentTimeInfo deployTimes;
    State state = Inactive;
    bool stopRequested = false;
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

const Kit *AbstractRemoteLinuxDeployService::kit() const
{
    return d->target ? d->target->kit() : nullptr;
}

IDevice::ConstPtr AbstractRemoteLinuxDeployService::deviceConfiguration() const
{
    return d->deviceConfiguration;
}

void AbstractRemoteLinuxDeployService::saveDeploymentTimeStamp(const DeployableFile &deployableFile,
                                                               const QDateTime &remoteTimestamp)
{
    d->deployTimes.saveDeploymentTimeStamp(deployableFile, kit(), remoteTimestamp);
}

bool AbstractRemoteLinuxDeployService::hasLocalFileChanged(
        const DeployableFile &deployableFile) const
{
    return d->deployTimes.hasLocalFileChanged(deployableFile, kit());
}

bool AbstractRemoteLinuxDeployService::hasRemoteFileChanged(
        const DeployableFile &deployableFile, const QDateTime &remoteTimestamp) const
{
    return d->deployTimes.hasRemoteFileChanged(deployableFile, kit(), remoteTimestamp);
}

void AbstractRemoteLinuxDeployService::setTarget(Target *target)
{
    d->target = target;
    d->deviceConfiguration = DeviceKitAspect::device(kit());
}

void AbstractRemoteLinuxDeployService::setDevice(const IDevice::ConstPtr &device)
{
    d->deviceConfiguration = device;
}

void AbstractRemoteLinuxDeployService::start()
{
    QTC_ASSERT(d->state == Inactive, return);

    const CheckResult check = isDeploymentPossible();
    if (!check) {
        emit errorMessage(check.errorMessage());
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
    case Deploying:
        d->stopRequested = true;
        stopDeployment();
        break;
    }
}

CheckResult AbstractRemoteLinuxDeployService::isDeploymentPossible() const
{
    if (!deviceConfiguration())
        return CheckResult::failure(tr("No device configuration set."));
    return CheckResult::success();
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

    d->state = Deploying;
    doDeploy();
}

void AbstractRemoteLinuxDeployService::handleDeploymentDone()
{
    QTC_ASSERT(d->state == Deploying, return);

    setFinished();
}

void AbstractRemoteLinuxDeployService::setFinished()
{
    d->state = Inactive;
    d->stopRequested = false;
    emit finished();
}

} // namespace RemoteLinux
