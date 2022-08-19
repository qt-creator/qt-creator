// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "abstractremotelinuxdeployservice.h"

#include "deploymenttimeinfo.h"
#include "remotelinuxtr.h"

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
enum State { Inactive, Deploying };
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
        emit progressMessage(Tr::tr("No deployment action necessary. Skipping."));
        emit finished();
        return;
    }

    d->state = Deploying;
    doDeploy();
}

void AbstractRemoteLinuxDeployService::stop()
{
    if (d->stopRequested)
        return;

    if (d->state == Deploying) {
        d->stopRequested = true;
        stopDeployment();
    }
}

CheckResult AbstractRemoteLinuxDeployService::isDeploymentPossible() const
{
    if (!deviceConfiguration())
        return CheckResult::failure(Tr::tr("No device configuration set."));
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
