// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/makestep.h>

namespace RemoteLinux {

class REMOTELINUX_EXPORT MakeInstallStep : public ProjectExplorer::MakeStep
{
    Q_OBJECT
public:
    MakeInstallStep(ProjectExplorer::BuildStepList *parent, Utils::Id id);

    static Utils::Id stepId();
    static QString displayName();

private:
    bool fromMap(const QVariantMap &map) override;
    QWidget *createConfigWidget() override;
    bool init() override;
    void finish(bool success) override;
    bool isJobCountSupported() const override { return false; }

    Utils::FilePath installRoot() const;
    bool cleanInstallRoot() const;

    void updateCommandFromAspect();
    void updateArgsFromAspect();
    void updateFullCommandLine();
    void updateFromCustomCommandLineAspect();

    Utils::StringAspect *customCommandLineAspect() const;

    ProjectExplorer::DeploymentData m_deploymentData;
    bool m_noInstallTarget = false;
    bool m_isCmakeProject = false;
};

} // namespace RemoteLinux
