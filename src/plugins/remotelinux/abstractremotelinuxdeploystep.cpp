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

#include "abstractremotelinuxdeploystep.h"

#include "abstractremotelinuxdeployservice.h"
#include "remotelinuxdeployconfiguration.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitinformation.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class AbstractRemoteLinuxDeployStepPrivate
{
public:
    bool hasError;
    QFutureInterface<bool> future;
};

} // namespace Internal

AbstractRemoteLinuxDeployStep::AbstractRemoteLinuxDeployStep(BuildStepList *bsl, Core::Id id)
    : BuildStep(bsl, id), d(new Internal::AbstractRemoteLinuxDeployStepPrivate)
{
}

AbstractRemoteLinuxDeployStep::AbstractRemoteLinuxDeployStep(BuildStepList *bsl,
        AbstractRemoteLinuxDeployStep *other)
    : BuildStep(bsl, other), d(new Internal::AbstractRemoteLinuxDeployStepPrivate)
{
}

AbstractRemoteLinuxDeployStep::~AbstractRemoteLinuxDeployStep()
{
    delete d;
}

bool AbstractRemoteLinuxDeployStep::fromMap(const QVariantMap &map)
{
    if (!BuildStep::fromMap(map))
        return false;
    deployService()->importDeployTimes(map);
    return true;
}

QVariantMap AbstractRemoteLinuxDeployStep::toMap() const
{
    return BuildStep::toMap().unite(deployService()->exportDeployTimes());
}

bool AbstractRemoteLinuxDeployStep::init(QList<const BuildStep *> &earlierSteps)
{
    Q_UNUSED(earlierSteps);
    QString error;
    deployService()->setTarget(target());
    const bool canDeploy = initInternal(&error);
    if (!canDeploy)
        emit addOutput(tr("Cannot deploy: %1").arg(error), OutputFormat::ErrorMessage);
    return canDeploy;
}

void AbstractRemoteLinuxDeployStep::run(QFutureInterface<bool> &fi)
{
    connect(deployService(), &AbstractRemoteLinuxDeployService::errorMessage,
            this, &AbstractRemoteLinuxDeployStep::handleErrorMessage);
    connect(deployService(), &AbstractRemoteLinuxDeployService::progressMessage,
            this, &AbstractRemoteLinuxDeployStep::handleProgressMessage);
    connect(deployService(), &AbstractRemoteLinuxDeployService::warningMessage,
            this, &AbstractRemoteLinuxDeployStep::handleWarningMessage);
    connect(deployService(), &AbstractRemoteLinuxDeployService::stdOutData,
            this, &AbstractRemoteLinuxDeployStep::handleStdOutData);
    connect(deployService(), &AbstractRemoteLinuxDeployService::stdErrData,
            this, &AbstractRemoteLinuxDeployStep::handleStdErrData);
    connect(deployService(), &AbstractRemoteLinuxDeployService::finished,
            this, &AbstractRemoteLinuxDeployStep::handleFinished);

    d->hasError = false;
    d->future = fi;
    deployService()->start();
}

void AbstractRemoteLinuxDeployStep::cancel()
{
    if (d->hasError)
        return;

    emit addOutput(tr("User requests deployment to stop; cleaning up."),
                   OutputFormat::NormalMessage);
    d->hasError = true;
    deployService()->stop();
}

BuildStepConfigWidget *AbstractRemoteLinuxDeployStep::createConfigWidget()
{
    return new SimpleBuildStepConfigWidget(this);
}

RemoteLinuxDeployConfiguration *AbstractRemoteLinuxDeployStep::deployConfiguration() const
{
    return qobject_cast<RemoteLinuxDeployConfiguration *>(BuildStep::deployConfiguration());
}

void AbstractRemoteLinuxDeployStep::handleProgressMessage(const QString &message)
{
    emit addOutput(message, OutputFormat::NormalMessage);
}

void AbstractRemoteLinuxDeployStep::handleErrorMessage(const QString &message)
{
    ProjectExplorer::Task task = Task(Task::Error, message, Utils::FileName(), -1,
                                      Constants::TASK_CATEGORY_DEPLOYMENT);
    emit addTask(task, 1); // TODO correct?
    emit addOutput(message, OutputFormat::ErrorMessage);
    d->hasError = true;
}

void AbstractRemoteLinuxDeployStep::handleWarningMessage(const QString &message)
{
    ProjectExplorer::Task task = Task(Task::Warning, message, Utils::FileName(), -1,
                                      Constants::TASK_CATEGORY_DEPLOYMENT);
    emit addTask(task, 1); // TODO correct?
    emit addOutput(message, OutputFormat::ErrorMessage);
}

void AbstractRemoteLinuxDeployStep::handleFinished()
{
    if (d->hasError)
        emit addOutput(tr("Deploy step failed."), OutputFormat::ErrorMessage);
    else
        emit addOutput(tr("Deploy step finished."), OutputFormat::NormalMessage);
    disconnect(deployService(), 0, this, 0);
    reportRunResult(d->future, !d->hasError);
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
