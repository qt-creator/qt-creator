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

#include "abstractremotelinuxdeploystep.h"

#include "abstractremotelinuxdeployservice.h"
#include "remotelinuxdeployconfiguration.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>

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

AbstractRemoteLinuxDeployStep::AbstractRemoteLinuxDeployStep(BuildStepList *bsl, const Core::Id id)
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

bool AbstractRemoteLinuxDeployStep::init()
{
    QString error;
    deployService()->setTarget(target());
    const bool canDeploy = initInternal(&error);
    if (!canDeploy)
        emit addOutput(tr("Cannot deploy: %1").arg(error), ErrorMessageOutput);
    return canDeploy;
}

void AbstractRemoteLinuxDeployStep::run(QFutureInterface<bool> &fi)
{
    connect(deployService(), SIGNAL(errorMessage(QString)), SLOT(handleErrorMessage(QString)));
    connect(deployService(), SIGNAL(progressMessage(QString)),
        SLOT(handleProgressMessage(QString)));
    connect(deployService(), SIGNAL(warningMessage(QString)), SLOT(handleWarningMessage(QString)));
    connect(deployService(), SIGNAL(stdOutData(QString)), SLOT(handleStdOutData(QString)));
    connect(deployService(), SIGNAL(stdErrData(QString)), SLOT(handleStdErrData(QString)));
    connect(deployService(), SIGNAL(finished()), SLOT(handleFinished()));

    d->hasError = false;
    d->future = fi;
    deployService()->start();
}

void AbstractRemoteLinuxDeployStep::cancel()
{
    if (d->hasError)
        return;

    emit addOutput(tr("User requests deployment to stop; cleaning up."), MessageOutput);
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
    emit addOutput(message, MessageOutput);
}

void AbstractRemoteLinuxDeployStep::handleErrorMessage(const QString &message)
{
    emit addOutput(message, ErrorMessageOutput);
    emit addTask(Task(Task::Error, message, Utils::FileName(), -1,
                      Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));
    d->hasError = true;
}

void AbstractRemoteLinuxDeployStep::handleWarningMessage(const QString &message)
{
    emit addOutput(message, ErrorMessageOutput);
    emit addTask(Task(Task::Warning, message, Utils::FileName(), -1,
                      Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM)));
}

void AbstractRemoteLinuxDeployStep::handleFinished()
{
    if (d->hasError)
        emit addOutput(tr("Deploy step failed."), ErrorMessageOutput);
    else
        emit addOutput(tr("Deploy step finished."), MessageOutput);
    disconnect(deployService(), 0, this, 0);
    d->future.reportResult(!d->hasError);
    emit finished();
}

void AbstractRemoteLinuxDeployStep::handleStdOutData(const QString &data)
{
    emit addOutput(data, NormalOutput, DontAppendNewline);
}

void AbstractRemoteLinuxDeployStep::handleStdErrData(const QString &data)
{
    emit addOutput(data, ErrorOutput, DontAppendNewline);
}

} // namespace RemoteLinux
