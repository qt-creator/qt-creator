// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/devicesupport/idevicefwd.h>

namespace ProjectExplorer { class DeployableFile; }

namespace RemoteLinux {

namespace Internal { class AbstractRemoteLinuxDeployStepPrivate; }

class REMOTELINUX_EXPORT AbstractRemoteLinuxDeployStep : public ProjectExplorer::BuildStep
{
public:
    explicit AbstractRemoteLinuxDeployStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);
    ~AbstractRemoteLinuxDeployStep() override;

protected:
    ProjectExplorer::IDeviceConstPtr deviceConfiguration() const;
    virtual Utils::expected_str<void> isDeploymentPossible() const;
    void handleStdOutData(const QString &data);
    void handleStdErrData(const QString &data);

    void fromMap(const Utils::Store &map) final;
    void toMap(Utils::Store &map) const final;
    bool init() final;

    void setInternalInitializer(const std::function<Utils::expected_str<void>()> &init);

    void saveDeploymentTimeStamp(const ProjectExplorer::DeployableFile &deployableFile,
                                 const QDateTime &remoteTimestamp);
    bool hasLocalFileChanged(const ProjectExplorer::DeployableFile &deployableFile) const;
    bool hasRemoteFileChanged(const ProjectExplorer::DeployableFile &deployableFile,
                              const QDateTime &remoteTimestamp) const;

    void addProgressMessage(const QString &message);
    void addErrorMessage(const QString &message);
    void addWarningMessage(const QString &message);

private:
    virtual bool isDeploymentNecessary() const;
    virtual Tasking::GroupItem deployRecipe() = 0;
    Tasking::GroupItem runRecipe() final;

    Internal::AbstractRemoteLinuxDeployStepPrivate *d;
};

} // RemoteLinux
