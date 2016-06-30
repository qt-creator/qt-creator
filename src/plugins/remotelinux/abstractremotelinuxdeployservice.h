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

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <QObject>
#include <QVariantMap>

namespace QSsh { class SshConnection; }

namespace ProjectExplorer {
class DeployableFile;
class Kit;
class Target;
}

namespace RemoteLinux {
namespace Internal { class AbstractRemoteLinuxDeployServicePrivate; }

class REMOTELINUX_EXPORT AbstractRemoteLinuxDeployService : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractRemoteLinuxDeployService)
public:
    explicit AbstractRemoteLinuxDeployService(QObject *parent = 0);
    ~AbstractRemoteLinuxDeployService();

    void setTarget(ProjectExplorer::Target *bc);
    // Only use setDevice() as fallback if no target is available
    void setDevice(const ProjectExplorer::IDevice::ConstPtr &device);
    void start();
    void stop();

    QVariantMap exportDeployTimes() const;
    void importDeployTimes(const QVariantMap &map);

    virtual bool isDeploymentPossible(QString *whyNot = 0) const;

signals:
    void errorMessage(const QString &message);
    void progressMessage(const QString &message);
    void warningMessage(const QString &message);
    void stdOutData(const QString &data);
    void stdErrData(const QString &data);
    void finished(); // Used by Qnx.

protected:
    const ProjectExplorer::Target *target() const;
    const ProjectExplorer::Kit *profile() const;
    ProjectExplorer::IDevice::ConstPtr deviceConfiguration() const;
    QSsh::SshConnection *connection() const;

    void saveDeploymentTimeStamp(const ProjectExplorer::DeployableFile &deployableFile);
    bool hasChangedSinceLastDeployment(const ProjectExplorer::DeployableFile &deployableFile) const;

    void handleDeviceSetupDone(bool success);
    void handleDeploymentDone();

private:
    void handleConnected();
    void handleConnectionFailure();

    virtual bool isDeploymentNecessary() const = 0;

    // Should do things needed *before* connecting. Call handleDeviceSetupDone() afterwards.
    virtual void doDeviceSetup() = 0;
    virtual void stopDeviceSetup() = 0;

    virtual void doDeploy() = 0;
    virtual void stopDeployment() = 0;

    void setFinished();

    Internal::AbstractRemoteLinuxDeployServicePrivate * const d;
};

} // namespace RemoteLinux
