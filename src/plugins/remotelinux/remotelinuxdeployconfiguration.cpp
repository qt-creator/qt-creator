/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/
#include "remotelinuxdeployconfiguration.h"

#include "deploymentinfo.h"
#include "remotelinuxdeployconfigurationwidget.h"
#include "typespecificdeviceconfigurationlistmodel.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4project.h>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace {
const char DEPLOYMENT_INFO_SETTING[] = "RemoteLinux.DeploymentInfo";
} // namespace

namespace RemoteLinux {

using namespace Internal;

RemoteLinuxDeployConfiguration::RemoteLinuxDeployConfiguration(ProjectExplorer::Target *target,
        const Core::Id id, const QString &defaultDisplayName)
    : DeployConfiguration(target, id)
{
    setDefaultDisplayName(defaultDisplayName);

    // Make sure we have deploymentInfo, but create it only once:
    DeploymentInfo *info
            = qobject_cast<DeploymentInfo *>(target->project()->namedSettings(QLatin1String(DEPLOYMENT_INFO_SETTING)).value<QObject *>());
    if (!info) {
        info = new DeploymentInfo(static_cast<Qt4ProjectManager::Qt4Project *>(target->project()));
        QVariant data = QVariant::fromValue(static_cast<QObject *>(info));
        target->project()->setNamedSettings(QLatin1String(DEPLOYMENT_INFO_SETTING), data);
    }
}

RemoteLinuxDeployConfiguration::RemoteLinuxDeployConfiguration(ProjectExplorer::Target *target,
        RemoteLinuxDeployConfiguration *source)
    : DeployConfiguration(target, source)
{ }

DeploymentInfo *RemoteLinuxDeployConfiguration::deploymentInfo() const
{
    DeploymentInfo *info
            = qobject_cast<DeploymentInfo *>(target()->project()->namedSettings(QLatin1String(DEPLOYMENT_INFO_SETTING)).value<QObject *>());
    return info;
}

QString RemoteLinuxDeployConfiguration::qmakeScope() const
{
    return QLatin1String("unix");
}

QString RemoteLinuxDeployConfiguration::installPrefix() const
{
    return QString();
}

DeployConfigurationWidget *RemoteLinuxDeployConfiguration::configurationWidget() const
{
    return new RemoteLinuxDeployConfigurationWidget;
}

} // namespace RemoteLinux
