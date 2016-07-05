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

#include <projectexplorer/buildstep.h>

#include <QVariantMap>

namespace RemoteLinux {
class AbstractRemoteLinuxDeployService;
class RemoteLinuxDeployConfiguration;

namespace Internal { class AbstractRemoteLinuxDeployStepPrivate; }

class REMOTELINUX_EXPORT AbstractRemoteLinuxDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    ~AbstractRemoteLinuxDeployStep() override;
    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;
    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;
    bool runInGuiThread() const override { return true; }
    void cancel() override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    RemoteLinuxDeployConfiguration *deployConfiguration() const;

    virtual AbstractRemoteLinuxDeployService *deployService() const = 0;

protected:
    AbstractRemoteLinuxDeployStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);
    AbstractRemoteLinuxDeployStep(ProjectExplorer::BuildStepList *bsl,
        AbstractRemoteLinuxDeployStep *other);
    virtual bool initInternal(QString *error = 0) = 0;

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
