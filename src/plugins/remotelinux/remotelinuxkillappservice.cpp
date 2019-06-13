/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "remotelinuxkillappservice.h"

namespace RemoteLinux {
namespace Internal {
class RemoteLinuxKillAppServicePrivate
{
public:
    QString remoteExecutable;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOp;
};
} // namespace Internal

RemoteLinuxKillAppService::RemoteLinuxKillAppService()
    : d(new Internal::RemoteLinuxKillAppServicePrivate)
{
}

RemoteLinuxKillAppService::~RemoteLinuxKillAppService()
{
    cleanup();
    delete d;
}

void RemoteLinuxKillAppService::setRemoteExecutable(const QString &filePath)
{
    d->remoteExecutable = filePath;
}

bool RemoteLinuxKillAppService::isDeploymentNecessary() const
{
    return !d->remoteExecutable.isEmpty();
}

void RemoteLinuxKillAppService::doDeploy()
{
    d->signalOp = deviceConfiguration()->signalOperation();
    if (!d->signalOp) {
        handleDeploymentDone();
        return;
    }
    connect(d->signalOp.data(), &ProjectExplorer::DeviceProcessSignalOperation::finished,
            this, &RemoteLinuxKillAppService::handleSignalOpFinished);
    emit progressMessage(tr("Trying to kill \"%1\" on remote device...").arg(d->remoteExecutable));
    d->signalOp->killProcess(d->remoteExecutable);
}

void RemoteLinuxKillAppService::cleanup()
{
    if (d->signalOp) {
        disconnect(d->signalOp.data(), nullptr, this, nullptr);
        d->signalOp.clear();
    }
}

void RemoteLinuxKillAppService::finishDeployment()
{
    cleanup();
    handleDeploymentDone();
}

void RemoteLinuxKillAppService::stopDeployment()
{
    finishDeployment();
}

void RemoteLinuxKillAppService::handleSignalOpFinished(const QString &errorMessage)
{
    if (errorMessage.isEmpty())
        emit progressMessage(tr("Remote application killed."));
    else
        emit progressMessage(tr("Failed to kill remote application. Assuming it was not running."));
    finishDeployment();
}

} // namespace RemoteLinux
