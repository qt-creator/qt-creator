/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "remotelinuxdeployconfiguration.h"

#include "abstractembeddedlinuxtarget.h"
#include "deploymentinfo.h"
#include "remotelinuxdeployconfigurationwidget.h"
#include "typespecificdeviceconfigurationlistmodel.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <qt4projectmanager/qt4target.h>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {
namespace {
const char DeviceIdKey[] = "Qt4ProjectManager.MaemoRunConfiguration.DeviceId";
} // anonymous namespace

class RemoteLinuxDeployConfigurationPrivate
{
public:
    QSharedPointer<const LinuxDeviceConfiguration> deviceConfiguration;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxDeployConfiguration::RemoteLinuxDeployConfiguration(ProjectExplorer::Target *target,
        const QString &id, const QString &defaultDisplayName)
    : DeployConfiguration(target, id), d(new RemoteLinuxDeployConfigurationPrivate)
{
    setDefaultDisplayName(defaultDisplayName);

    initialize();
}

RemoteLinuxDeployConfiguration::RemoteLinuxDeployConfiguration(ProjectExplorer::Target *target,
        RemoteLinuxDeployConfiguration *source)
    : DeployConfiguration(target, source), d(new RemoteLinuxDeployConfigurationPrivate)
{
    initialize();
}

RemoteLinuxDeployConfiguration::~RemoteLinuxDeployConfiguration()
{
    delete d;
}

void RemoteLinuxDeployConfiguration::initialize()
{
    d->deviceConfiguration = target()->deviceConfigModel()->defaultDeviceConfig();
    connect(target()->deviceConfigModel(), SIGNAL(updated()),
        SLOT(handleDeviceConfigurationListUpdated()));
}

void RemoteLinuxDeployConfiguration::handleDeviceConfigurationListUpdated()
{
    setDeviceConfig(DeviceManager::instance()->internalId(d->deviceConfiguration));
}

void RemoteLinuxDeployConfiguration::setDeviceConfig(LinuxDeviceConfiguration::Id internalId)
{
    d->deviceConfiguration = target()->deviceConfigModel()->find(internalId);
    emit deviceConfigurationListChanged();
    emit currentDeviceConfigurationChanged();
}

bool RemoteLinuxDeployConfiguration::fromMap(const QVariantMap &map)
{
    if (!DeployConfiguration::fromMap(map))
        return false;
    setDeviceConfig(map.value(QLatin1String(DeviceIdKey), IDevice::invalidId()).toULongLong());
    return true;
}

QVariantMap RemoteLinuxDeployConfiguration::toMap() const
{
    QVariantMap map = DeployConfiguration::toMap();
    map.insert(QLatin1String(DeviceIdKey),
        DeviceManager::instance()->internalId(d->deviceConfiguration));
    return map;
}

void RemoteLinuxDeployConfiguration::setDeviceConfiguration(int index)
{
    const LinuxDeviceConfiguration::ConstPtr &newDevConf
        = target()->deviceConfigModel()->deviceAt(index);
    if (d->deviceConfiguration != newDevConf) {
        d->deviceConfiguration = newDevConf;
        emit currentDeviceConfigurationChanged();
    }
}

AbstractEmbeddedLinuxTarget *RemoteLinuxDeployConfiguration::target() const
{
    return qobject_cast<AbstractEmbeddedLinuxTarget *>(DeployConfiguration::target());
}

DeploymentInfo *RemoteLinuxDeployConfiguration::deploymentInfo() const
{
    return target()->deploymentInfo();
}

DeployConfigurationWidget *RemoteLinuxDeployConfiguration::configurationWidget() const
{
    return new RemoteLinuxDeployConfigurationWidget;
}

LinuxDeviceConfiguration::ConstPtr RemoteLinuxDeployConfiguration::deviceConfiguration() const
{
    return d->deviceConfiguration;
}

} // namespace RemoteLinux
