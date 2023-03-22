// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/buildstep.h>
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

class CheckResult;

namespace Internal { class AbstractRemoteLinuxDeployStepPrivate; }

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

class REMOTELINUX_EXPORT AbstractRemoteLinuxDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    explicit AbstractRemoteLinuxDeployStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);
    ~AbstractRemoteLinuxDeployStep() override;

    void start();
    void stop();

    QVariantMap exportDeployTimes() const;
    void importDeployTimes(const QVariantMap &map);
    ProjectExplorer::IDeviceConstPtr deviceConfiguration() const;

    virtual CheckResult isDeploymentPossible() const;

    void handleStdOutData(const QString &data);
    void handleStdErrData(const QString &data);

protected:
    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;
    bool init() override;
    void doRun() final;
    void doCancel() override;

    void setInternalInitializer(const std::function<CheckResult()> &init);
    void setRunPreparer(const std::function<void()> &prep);

    void saveDeploymentTimeStamp(const ProjectExplorer::DeployableFile &deployableFile,
                                 const QDateTime &remoteTimestamp);
    bool hasLocalFileChanged(const ProjectExplorer::DeployableFile &deployableFile) const;
    bool hasRemoteFileChanged(const ProjectExplorer::DeployableFile &deployableFile,
                              const QDateTime &remoteTimestamp) const;

    void addProgressMessage(const QString &message);
    void addErrorMessage(const QString &message);
    void addWarningMessage(const QString &message);

    void handleFinished();

private:
    virtual bool isDeploymentNecessary() const;
    virtual Utils::Tasking::Group deployRecipe();

    Internal::AbstractRemoteLinuxDeployStepPrivate *d;
};

} // RemoteLinux
