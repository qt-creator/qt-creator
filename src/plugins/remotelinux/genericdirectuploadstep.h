// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include "abstractremotelinuxdeploystep.h"

namespace RemoteLinux {

class REMOTELINUX_EXPORT GenericDirectUploadStep : public AbstractRemoteLinuxDeployStep
{
    Q_OBJECT

public:
    GenericDirectUploadStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id,
                            bool offerIncrementalDeployment = true);
    ~GenericDirectUploadStep() override;

    bool isDeploymentNecessary() const final;
    Utils::Tasking::Group deployRecipe() final;

    static Utils::Id stepId();
    static QString displayName();

private:
    friend class GenericDirectUploadStepPrivate;
    class GenericDirectUploadStepPrivate *d;
};

} // RemoteLinux
