// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractremotelinuxdeployservice.h"

#include "deploymenttimeinfo.h"
#include "remotelinuxtr.h"

#include <projectexplorer/deployablefile.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>
#include <utils/tasktree.h>

#include <QDateTime>
#include <QPointer>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class AbstractRemoteLinuxDeployServicePrivate
{
public:
    IDevice::ConstPtr deviceConfiguration;
    QPointer<Target> target;

    DeploymentTimeInfo deployTimes;
    std::unique_ptr<TaskTree> m_taskTree;
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
    QTC_ASSERT(!d->m_taskTree, return);

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

    d->m_taskTree.reset(new TaskTree(deployRecipe()));
    const auto endHandler = [this] {
        d->m_taskTree.release()->deleteLater();
        emit finished();
    };
    connect(d->m_taskTree.get(), &TaskTree::done, this, endHandler);
    connect(d->m_taskTree.get(), &TaskTree::errorOccurred, this, endHandler);
    d->m_taskTree->start();
}

void AbstractRemoteLinuxDeployService::stop()
{
    if (!d->m_taskTree)
        return;
    d->m_taskTree.reset();
    emit finished();
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

} // namespace RemoteLinux
