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

class AbstractRemoteLinuxDeployStepPrivate
{
public:
    bool hasError;
    std::function<CheckResult()> internalInit;
    std::function<void()> runPreparer;
    AbstractRemoteLinuxDeployService *deployService = nullptr;
};

} // Internal

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



AbstractRemoteLinuxDeployStep::AbstractRemoteLinuxDeployStep(BuildStepList *bsl, Utils::Id id)
    : BuildStep(bsl, id), d(new Internal::AbstractRemoteLinuxDeployStepPrivate)
{
}

void AbstractRemoteLinuxDeployStep::setInternalInitializer(const std::function<CheckResult ()> &init)
{
    d->internalInit = init;
}

void AbstractRemoteLinuxDeployStep::setRunPreparer(const std::function<void ()> &prep)
{
    d->runPreparer = prep;
}

void AbstractRemoteLinuxDeployStep::setDeployService(AbstractRemoteLinuxDeployService *service)
{
    d->deployService = service;
}

AbstractRemoteLinuxDeployStep::~AbstractRemoteLinuxDeployStep()
{
    delete d->deployService;
    delete d;
}

bool AbstractRemoteLinuxDeployStep::fromMap(const QVariantMap &map)
{
    if (!BuildStep::fromMap(map))
        return false;
    d->deployService->importDeployTimes(map);
    return true;
}

QVariantMap AbstractRemoteLinuxDeployStep::toMap() const
{
    QVariantMap map = BuildStep::toMap();
    map.insert(d->deployService->exportDeployTimes());
    return map;
}

bool AbstractRemoteLinuxDeployStep::init()
{
    d->deployService->setTarget(target());

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

    connect(d->deployService, &AbstractRemoteLinuxDeployService::errorMessage,
            this, &AbstractRemoteLinuxDeployStep::handleErrorMessage);
    connect(d->deployService, &AbstractRemoteLinuxDeployService::progressMessage,
            this, &AbstractRemoteLinuxDeployStep::handleProgressMessage);
    connect(d->deployService, &AbstractRemoteLinuxDeployService::warningMessage,
            this, &AbstractRemoteLinuxDeployStep::handleWarningMessage);
    connect(d->deployService, &AbstractRemoteLinuxDeployService::stdOutData,
            this, &AbstractRemoteLinuxDeployStep::handleStdOutData);
    connect(d->deployService, &AbstractRemoteLinuxDeployService::stdErrData,
            this, &AbstractRemoteLinuxDeployStep::handleStdErrData);
    connect(d->deployService, &AbstractRemoteLinuxDeployService::finished,
            this, &AbstractRemoteLinuxDeployStep::handleFinished);

    d->hasError = false;
    d->deployService->start();
}

void AbstractRemoteLinuxDeployStep::doCancel()
{
    if (d->hasError)
        return;

    emit addOutput(Tr::tr("User requests deployment to stop; cleaning up."),
                   OutputFormat::NormalMessage);
    d->hasError = true;
    d->deployService->stop();
}

void AbstractRemoteLinuxDeployStep::handleProgressMessage(const QString &message)
{
    emit addOutput(message, OutputFormat::NormalMessage);
}

void AbstractRemoteLinuxDeployStep::handleErrorMessage(const QString &message)
{
    emit addOutput(message, OutputFormat::ErrorMessage);
    emit addTask(DeploymentTask(Task::Error, message), 1); // TODO correct?
    d->hasError = true;
}

void AbstractRemoteLinuxDeployStep::handleWarningMessage(const QString &message)
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
    disconnect(d->deployService, nullptr, this, nullptr);
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

} // namespace RemoteLinux
