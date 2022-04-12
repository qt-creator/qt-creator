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

#include <projectexplorer/devicesupport/idevicefwd.h>

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

class REMOTELINUX_EXPORT CheckResult
{
public:
    static CheckResult success() { return {true, {}}; }
    static CheckResult failure(const QString &error = {}) { return {false, error}; }

    operator bool() const { return m_ok; }
    QString errorMessage() const { return m_error; }

private:
    CheckResult(bool ok, const QString &error) : m_ok(ok), m_error(error) {}

    bool m_ok = false;
    QString m_error;
};

class REMOTELINUX_EXPORT AbstractRemoteLinuxDeployService : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractRemoteLinuxDeployService)
public:
    explicit AbstractRemoteLinuxDeployService(QObject *parent = nullptr);
    ~AbstractRemoteLinuxDeployService() override;

    void setTarget(ProjectExplorer::Target *bc);
    // Only use setDevice() as fallback if no target is available
    void setDevice(const ProjectExplorer::IDeviceConstPtr &device);
    void start();
    void stop();

    QVariantMap exportDeployTimes() const;
    void importDeployTimes(const QVariantMap &map);

    virtual CheckResult isDeploymentPossible() const;

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
    ProjectExplorer::IDeviceConstPtr deviceConfiguration() const;
    QSsh::SshConnection *connection() const;

    void saveDeploymentTimeStamp(const ProjectExplorer::DeployableFile &deployableFile,
                                 const QDateTime &remoteTimestamp);

    bool hasLocalFileChanged(const ProjectExplorer::DeployableFile &deployableFile) const;
    bool hasRemoteFileChanged(const ProjectExplorer::DeployableFile &deployableFile,
                              const QDateTime &remoteTimestamp) const;

    void handleDeviceSetupDone(bool success);
    void handleDeploymentDone();

    // Should do things needed *before* connecting. Call default implementation afterwards.
    virtual void doDeviceSetup() { handleDeviceSetupDone(true); }
    virtual void stopDeviceSetup() { handleDeviceSetupDone(false); }

    void setFinished();

private:
    void handleConnected();
    void handleConnectionFailure();

    virtual bool isDeploymentNecessary() const = 0;

    virtual void doDeploy() = 0;
    virtual void stopDeployment() = 0;


    Internal::AbstractRemoteLinuxDeployServicePrivate * const d;
};

} // namespace RemoteLinux
