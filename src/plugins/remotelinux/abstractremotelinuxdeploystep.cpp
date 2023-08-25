// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractremotelinuxdeploystep.h"

#include "deploymenttimeinfo.h"
#include "remotelinuxtr.h"

#include <projectexplorer/deployablefile.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspects.h>

#include <solutions/tasking/tasktree.h>

#include <utils/qtcassert.h>

#include <QDateTime>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class AbstractRemoteLinuxDeployStepPrivate
{
public:
    std::function<expected_str<void>()> internalInit;

    DeploymentTimeInfo deployTimes;
};

} // Internal

using namespace Internal;

AbstractRemoteLinuxDeployStep::AbstractRemoteLinuxDeployStep(BuildStepList *bsl, Id id)
    : BuildStep(bsl, id), d(new AbstractRemoteLinuxDeployStepPrivate)
{
}

AbstractRemoteLinuxDeployStep::~AbstractRemoteLinuxDeployStep()
{
    delete d;
}

IDevice::ConstPtr AbstractRemoteLinuxDeployStep::deviceConfiguration() const
{
    return DeviceKitAspect::device(kit());
}

void AbstractRemoteLinuxDeployStep::saveDeploymentTimeStamp(const DeployableFile &deployableFile,
                                                            const QDateTime &remoteTimestamp)
{
    d->deployTimes.saveDeploymentTimeStamp(deployableFile, kit(), remoteTimestamp);
}

bool AbstractRemoteLinuxDeployStep::hasLocalFileChanged(
        const DeployableFile &deployableFile) const
{
    return d->deployTimes.hasLocalFileChanged(deployableFile, kit());
}

bool AbstractRemoteLinuxDeployStep::hasRemoteFileChanged(
        const DeployableFile &deployableFile, const QDateTime &remoteTimestamp) const
{
    return d->deployTimes.hasRemoteFileChanged(deployableFile, kit(), remoteTimestamp);
}

expected_str<void> AbstractRemoteLinuxDeployStep::isDeploymentPossible() const
{
    if (!deviceConfiguration())
        return make_unexpected(Tr::tr("No device configuration set."));
    return {};
}

void AbstractRemoteLinuxDeployStep::setInternalInitializer(
    const std::function<expected_str<void>()> &init)
{
    d->internalInit = init;
}

void AbstractRemoteLinuxDeployStep::fromMap(const Store &map)
{
    BuildStep::fromMap(map);
    if (hasError())
        return;
    d->deployTimes.importDeployTimes(map);
}

void AbstractRemoteLinuxDeployStep::toMap(Store &map) const
{
    BuildStep::toMap(map);
    map.insert(d->deployTimes.exportDeployTimes());
}

bool AbstractRemoteLinuxDeployStep::init()
{
    QTC_ASSERT(d->internalInit, return false);
    const auto canDeploy = d->internalInit();
    if (!canDeploy) {
        emit addOutput(Tr::tr("Cannot deploy: %1").arg(canDeploy.error()),
                       OutputFormat::ErrorMessage);
    }
    return bool(canDeploy);
}

void AbstractRemoteLinuxDeployStep::addProgressMessage(const QString &message)
{
    emit addOutput(message, OutputFormat::NormalMessage);
}

void AbstractRemoteLinuxDeployStep::addErrorMessage(const QString &message)
{
    emit addOutput(message, OutputFormat::ErrorMessage);
    emit addTask(DeploymentTask(Task::Error, message), 1); // TODO correct?
}

void AbstractRemoteLinuxDeployStep::addWarningMessage(const QString &message)
{
    emit addOutput(message, OutputFormat::ErrorMessage);
    emit addTask(DeploymentTask(Task::Warning, message), 1); // TODO correct?
}

void AbstractRemoteLinuxDeployStep::handleStdOutData(const QString &data)
{
    emit addOutput(data, OutputFormat::Stdout, DontAppendNewline);
}

void AbstractRemoteLinuxDeployStep::handleStdErrData(const QString &data)
{
    emit addOutput(data, OutputFormat::Stderr, DontAppendNewline);
}

bool AbstractRemoteLinuxDeployStep::isDeploymentNecessary() const
{
    return true;
}

GroupItem AbstractRemoteLinuxDeployStep::runRecipe()
{
    const auto onSetup = [this] {
        const auto canDeploy = isDeploymentPossible();
        if (!canDeploy) {
            addErrorMessage(canDeploy.error());
            return SetupResult::StopWithError;
        }
        if (!isDeploymentNecessary()) {
            addProgressMessage(Tr::tr("No deployment action necessary. Skipping."));
            return SetupResult::StopWithDone;
        }
        return SetupResult::Continue;
    };
    const auto onDone = [this] {
        emit addOutput(Tr::tr("Deploy step finished."), OutputFormat::NormalMessage);
    };
    const auto onError = [this] {
        emit addOutput(Tr::tr("Deploy step failed."), OutputFormat::ErrorMessage);
    };
    return Group {
        onGroupSetup(onSetup),
        deployRecipe(),
        onGroupDone(onDone),
        onGroupError(onError)
    };
}

} // namespace RemoteLinux
