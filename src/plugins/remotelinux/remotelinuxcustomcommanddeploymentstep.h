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

#include "abstractremotelinuxdeploystep.h"
#include "remotelinuxcustomcommanddeployservice.h"

namespace RemoteLinux {
namespace Internal {
class AbstractRemoteLinuxCustomCommandDeploymentStepPrivate;
class GenericRemoteLinuxCustomCommandDeploymentStepPrivate;
} // namespace Internal


class REMOTELINUX_EXPORT AbstractRemoteLinuxCustomCommandDeploymentStep
    : public AbstractRemoteLinuxDeployStep
{
    Q_OBJECT
public:
    ~AbstractRemoteLinuxCustomCommandDeploymentStep() override;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    void setCommandLine(const QString &commandLine);
    QString commandLine() const;

protected:
    AbstractRemoteLinuxCustomCommandDeploymentStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);
    AbstractRemoteLinuxCustomCommandDeploymentStep(ProjectExplorer::BuildStepList *bsl,
        AbstractRemoteLinuxCustomCommandDeploymentStep *other);

    bool initInternal(QString *error = 0) override;

private:
    void ctor();

    RemoteLinuxCustomCommandDeployService *deployService() const  override = 0;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    Internal::AbstractRemoteLinuxCustomCommandDeploymentStepPrivate *d;
};


class REMOTELINUX_EXPORT GenericRemoteLinuxCustomCommandDeploymentStep
    : public AbstractRemoteLinuxCustomCommandDeploymentStep
{
    Q_OBJECT
public:
    explicit GenericRemoteLinuxCustomCommandDeploymentStep(ProjectExplorer::BuildStepList *bsl);
    GenericRemoteLinuxCustomCommandDeploymentStep(ProjectExplorer::BuildStepList *bsl,
        GenericRemoteLinuxCustomCommandDeploymentStep *other);
    ~GenericRemoteLinuxCustomCommandDeploymentStep() override;

    static Core::Id stepId();
    static QString stepDisplayName();

private:
    RemoteLinuxCustomCommandDeployService *deployService() const override;
    void ctor();

    Internal::GenericRemoteLinuxCustomCommandDeploymentStepPrivate *d;
};

} // namespace RemoteLinux
