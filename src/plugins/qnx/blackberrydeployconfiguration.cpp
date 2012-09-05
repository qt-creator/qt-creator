/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "blackberrydeployconfiguration.h"

#include "qnxconstants.h"
#include "blackberrydeployconfigurationwidget.h"
#include "blackberrydeployinformation.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <ssh/sshconnection.h>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char DEPLOYMENT_INFO_SETTING[] = "QNX.BlackBerry.DeploymentInfo";
}

BlackBerryDeployConfiguration::BlackBerryDeployConfiguration(ProjectExplorer::Target *parent)
    : ProjectExplorer::DeployConfiguration(parent, Core::Id(Constants::QNX_BB_DEPLOYCONFIGURATION_ID))
{
    ctor();
}

BlackBerryDeployConfiguration::BlackBerryDeployConfiguration(ProjectExplorer::Target *parent,
                                               BlackBerryDeployConfiguration *source)
    : ProjectExplorer::DeployConfiguration(parent, source)
{
    ctor();
}

void BlackBerryDeployConfiguration::ctor()
{
    BlackBerryDeployInformation *info
            = qobject_cast<BlackBerryDeployInformation *>(target()->project()->namedSettings(QLatin1String(DEPLOYMENT_INFO_SETTING)).value<QObject *>());
    if (!info) {
        info = new BlackBerryDeployInformation(static_cast<Qt4ProjectManager::Qt4Project *>(target()->project()));
        QVariant data = QVariant::fromValue(static_cast<QObject *>(info));
        target()->project()->setNamedSettings(QLatin1String(DEPLOYMENT_INFO_SETTING), data);
    }

    setDefaultDisplayName(tr("Deploy to BlackBerry Device"));
}

BlackBerryDeployConfiguration::~BlackBerryDeployConfiguration()
{
}

BlackBerryDeployInformation *BlackBerryDeployConfiguration::deploymentInfo() const
{
    BlackBerryDeployInformation *info
            = qobject_cast<BlackBerryDeployInformation *>(target()->project()->namedSettings(QLatin1String(DEPLOYMENT_INFO_SETTING)).value<QObject *>());
    return info;
}

QString BlackBerryDeployConfiguration::deviceHost() const
{
    BlackBerryDeviceConfiguration::ConstPtr device = BlackBerryDeviceConfiguration::device(target()->kit());
    if (!device)
        return QString();

    return device->sshParameters().host;
}

QString BlackBerryDeployConfiguration::password() const
{
    BlackBerryDeviceConfiguration::ConstPtr device = BlackBerryDeviceConfiguration::device(target()->kit());
    return device->sshParameters().password;
}

QString BlackBerryDeployConfiguration::deviceName() const
{
    BlackBerryDeviceConfiguration::ConstPtr device = BlackBerryDeviceConfiguration::device(target()->kit());
    if (!device)
        return QString();

    return device->displayName();
}

ProjectExplorer::DeployConfigurationWidget *BlackBerryDeployConfiguration::configurationWidget() const
{
    return new BlackBerryDeployConfigurationWidget;
}
