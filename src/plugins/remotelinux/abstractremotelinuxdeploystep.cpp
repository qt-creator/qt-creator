// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractremotelinuxdeploystep.h"

#include "deploymenttimeinfo.h"
#include "remotelinuxtr.h"

#include <projectexplorer/deployablefile.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <solutions/tasking/tasktree.h>

#include <utils/qtcassert.h>

#include <QDateTime>
#include <QPointer>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class AbstractRemoteLinuxDeployStepPrivate
{
public:
    bool hasError;
    std::function<CheckResult()> internalInit;
    std::function<void()> runPreparer;

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

CheckResult AbstractRemoteLinuxDeployStep::isDeploymentPossible() const
{
    if (!deviceConfiguration())
        return CheckResult::failure(Tr::tr("No device configuration set."));
    return CheckResult::success();
}

void AbstractRemoteLinuxDeployStep::setInternalInitializer(const std::function<CheckResult ()> &init)
{
    d->internalInit = init;
}

void AbstractRemoteLinuxDeployStep::setRunPreparer(const std::function<void ()> &prep)
{
    d->runPreparer = prep;
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
    const CheckResult canDeploy = d->internalInit();
    if (!canDeploy) {
        emit addOutput(Tr::tr("Cannot deploy: %1").arg(canDeploy.errorMessage()),
                       OutputFormat::ErrorMessage);
    }
    return canDeploy;
}

void AbstractRemoteLinuxDeployStep::doRun()
{
    if (d->runPreparer)
        d->runPreparer();

    d->hasError = false;

    QTC_ASSERT(!d->m_taskTree, return);

    const CheckResult check = isDeploymentPossible();
    if (!check) {
        addErrorMessage(check.errorMessage());
        handleFinished();
        return;
    }

    if (!isDeploymentNecessary()) {
        addProgressMessage(Tr::tr("No deployment action necessary. Skipping."));
        handleFinished();
        return;
    }

    d->m_taskTree.reset(new TaskTree(deployRecipe()));
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

Group AbstractRemoteLinuxDeployStep::deployRecipe()
{
    return {};
}

} // namespace RemoteLinux
