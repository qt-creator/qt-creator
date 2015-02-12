/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef REMOTELINUXCUSTOMCOMMANDDEPLOYMENTSTEP_H
#define REMOTELINUXCUSTOMCOMMANDDEPLOYMENTSTEP_H

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
    ~AbstractRemoteLinuxCustomCommandDeploymentStep();

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    void setCommandLine(const QString &commandLine);
    QString commandLine() const;

protected:
    AbstractRemoteLinuxCustomCommandDeploymentStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);
    AbstractRemoteLinuxCustomCommandDeploymentStep(ProjectExplorer::BuildStepList *bsl,
        AbstractRemoteLinuxCustomCommandDeploymentStep *other);

    bool initInternal(QString *error = 0);

private:
    void ctor();

    RemoteLinuxCustomCommandDeployService *deployService() const = 0;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    Internal::AbstractRemoteLinuxCustomCommandDeploymentStepPrivate *d;
};


class REMOTELINUX_EXPORT GenericRemoteLinuxCustomCommandDeploymentStep
    : public AbstractRemoteLinuxCustomCommandDeploymentStep
{
    Q_OBJECT
public:
    GenericRemoteLinuxCustomCommandDeploymentStep(ProjectExplorer::BuildStepList *bsl);
    GenericRemoteLinuxCustomCommandDeploymentStep(ProjectExplorer::BuildStepList *bsl,
        GenericRemoteLinuxCustomCommandDeploymentStep *other);
    ~GenericRemoteLinuxCustomCommandDeploymentStep();

    static Core::Id stepId();
    static QString stepDisplayName();

private:
    RemoteLinuxCustomCommandDeployService *deployService() const;
    void ctor();

    Internal::GenericRemoteLinuxCustomCommandDeploymentStepPrivate *d;
};

} // namespace RemoteLinux

#endif // REMOTELINUXCUSTOMCOMMANDDEPLOYMENTSTEP_H
