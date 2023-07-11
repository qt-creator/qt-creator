// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractremotelinuxdeploystep.h"

#include "deploymenttimeinfo.h"
#include "remotelinuxtr.h"

#include <projectexplorer/deployablefile.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>

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
    bool hasError;
    std::function<expected_str<void>()> internalInit;

    DeploymentTimeInfo deployTimes;
    std::unique_ptr<TaskTree> m_taskTree;
};

} // Internal

using namespace Internal;

AbstractRemoteLinuxDeployStep::AbstractRemoteLinuxDeployStep(BuildStepList *bsl, Id id)
    : BuildStep(bsl, id), d(new AbstractRemoteLinuxDeployStepPrivate)
{}

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

bool AbstractRemoteLinuxDeployStep::fromMap(const QVariantMap &map)
{
    if (!BuildStep::fromMap(map))
        return false;
    d->deployTimes.importDeployTimes(map);
    return true;
}

QVariantMap AbstractRemoteLinuxDeployStep::toMap() const
{
    QVariantMap map = BuildStep::toMap();
    map.insert(d->deployTimes.exportDeployTimes());
    return map;
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

void AbstractRemoteLinuxDeployStep::doRun()
{
    d->hasError = false;

    QTC_ASSERT(!d->m_taskTree, return);

    d->m_taskTree.reset(new TaskTree({runRecipe()}));
    const auto endHandler = [this] {
        d->m_taskTree.release()->deleteLater();
        handleFinished();
    };
    connect(d->m_taskTree.get(), &TaskTree::done, this, endHandler);
    connect(d->m_taskTree.get(), &TaskTree::errorOccurred, this, endHandler);
    d->m_taskTree->start();
}

void AbstractRemoteLinuxDeployStep::doCancel()
{
    if (d->hasError)
        return;

    emit addOutput(Tr::tr("User requests deployment to stop; cleaning up."),
                   OutputFormat::NormalMessage);
    d->hasError = true;

    if (!d->m_taskTree)
        return;
    d->m_taskTree.reset();
    handleFinished();
}

void AbstractRemoteLinuxDeployStep::addProgressMessage(const QString &message)
{
    emit addOutput(message, OutputFormat::NormalMessage);
}

void AbstractRemoteLinuxDeployStep::addErrorMessage(const QString &message)
{
    emit addOutput(message, OutputFormat::ErrorMessage);
    emit addTask(DeploymentTask(Task::Error, message), 1); // TODO correct?
    d->hasError = true;
}

void AbstractRemoteLinuxDeployStep::addWarningMessage(const QString &message)
{
    emit addOutput(message, OutputFormat::ErrorMessage);
    emit addTask(DeploymentTask(Task::Warning, message), 1); // TODO correct?
}

void AbstractRemoteLinuxDeployStep::handleFinished()
{
    if (d->hasError)
        emit addOutput(Tr::tr("Deploy step failed."), OutputFormat::ErrorMessage);
    else
        emit addOutput(Tr::tr("Deploy step finished."), OutputFormat::NormalMessage);

    emit finished(!d->hasError);
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
            handleFinished();
            return SetupResult::StopWithError;
        }
        if (!isDeploymentNecessary()) {
            addProgressMessage(Tr::tr("No deployment action necessary. Skipping."));
            handleFinished();
            return SetupResult::StopWithDone;
        }
        return SetupResult::Continue;
    };
    return Group { onGroupSetup(onSetup), deployRecipe() };
}

} // namespace RemoteLinux
