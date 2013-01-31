/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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
    AbstractRemoteLinuxCustomCommandDeploymentStep(ProjectExplorer::BuildStepList *bsl, const Core::Id id);
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
