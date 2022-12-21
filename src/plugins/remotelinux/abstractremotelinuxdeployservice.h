// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevicefwd.h>

#include <QtCore/qcontainerfwd.h>
#include <QObject>

namespace ProjectExplorer {
class DeployableFile;
class Kit;
class Target;
}

namespace Utils::Tasking { class Group; }

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
    const ProjectExplorer::Kit *kit() const;
    ProjectExplorer::IDeviceConstPtr deviceConfiguration() const;

    void saveDeploymentTimeStamp(const ProjectExplorer::DeployableFile &deployableFile,
                                 const QDateTime &remoteTimestamp);
    bool hasLocalFileChanged(const ProjectExplorer::DeployableFile &deployableFile) const;
    bool hasRemoteFileChanged(const ProjectExplorer::DeployableFile &deployableFile,
                              const QDateTime &remoteTimestamp) const;

private:
    virtual bool isDeploymentNecessary() const = 0;
    virtual Utils::Tasking::Group deployRecipe() = 0;

    Internal::AbstractRemoteLinuxDeployServicePrivate * const d;
};

} // namespace RemoteLinux
