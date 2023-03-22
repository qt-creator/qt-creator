// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include "abstractremotelinuxdeploystep.h"

#include <projectexplorer/devicesupport/filetransfer.h>

#include <utils/tasktree.h>

namespace RemoteLinux {

class REMOTELINUX_EXPORT RsyncDeployStep : public AbstractRemoteLinuxDeployStep
{
public:
    RsyncDeployStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);
    ~RsyncDeployStep() override;

    static Utils::Id stepId();
    static QString displayName();

private:
    bool isDeploymentNecessary() const final;
    Utils::Tasking::Group deployRecipe() final;
    Utils::Tasking::TaskItem mkdirTask();
    Utils::Tasking::TaskItem transferTask();

    mutable ProjectExplorer::FilesToTransfer m_files;
    bool m_ignoreMissingFiles = false;
    QString m_flags;
};

} // namespace RemoteLinux
