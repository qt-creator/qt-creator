// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractremotelinuxdeploystep.h"

#include "abstractremotelinuxdeployservice.h"
#include "remotelinuxtr.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitinformation.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class AbstractRemoteLinuxDeployStepPrivate
{
public:
    bool hasError;
    std::function<CheckResult()> internalInit;
    std::function<void()> runPreparer;
    AbstractRemoteLinuxDeployService *deployService = nullptr;
};

} // namespace Internal

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
