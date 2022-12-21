// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/buildstep.h>

namespace RemoteLinux {

class AbstractRemoteLinuxDeployService;
class CheckResult;

namespace Internal { class AbstractRemoteLinuxDeployStepPrivate; }

class REMOTELINUX_EXPORT AbstractRemoteLinuxDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    ~AbstractRemoteLinuxDeployStep() override;

protected:
    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;
    bool init() override;
    void doRun() final;
    void doCancel() override;

    explicit AbstractRemoteLinuxDeployStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

    void setInternalInitializer(const std::function<CheckResult()> &init);
    void setRunPreparer(const std::function<void()> &prep);
    void setDeployService(AbstractRemoteLinuxDeployService *service);

private:
    void handleProgressMessage(const QString &message);
    void handleErrorMessage(const QString &message);
    void handleWarningMessage(const QString &message);
    void handleFinished();
    void handleStdOutData(const QString &data);
    void handleStdErrData(const QString &data);

    Internal::AbstractRemoteLinuxDeployStepPrivate *d;
};

} // namespace RemoteLinux
