// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/devicesupport/idevicefwd.h>

#include <QObject>

namespace ProjectExplorer { class DeployableFile; }
namespace Tasking { class GroupItem; }

namespace RemoteLinux {

namespace Internal { class AbstractRemoteLinuxDeployStepPrivate; }

class REMOTELINUX_EXPORT AbstractRemoteLinuxDeployStep : public ProjectExplorer::BuildStep
{
public:
    explicit AbstractRemoteLinuxDeployStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);
    ~AbstractRemoteLinuxDeployStep() override;

    ProjectExplorer::IDeviceConstPtr deviceConfiguration() const;

    virtual Utils::expected_str<void> isDeploymentPossible() const;

    void handleStdOutData(const QString &data);
    void handleStdErrData(const QString &data);

protected:
    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;
    bool init() override;
    void doRun() final;
    void doCancel() override;

    void setInternalInitializer(const std::function<Utils::expected_str<void>()> &init);

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
    virtual Tasking::GroupItem deployRecipe() = 0;
    Tasking::GroupItem runRecipe();

    Internal::AbstractRemoteLinuxDeployStepPrivate *d;
};

} // RemoteLinux
