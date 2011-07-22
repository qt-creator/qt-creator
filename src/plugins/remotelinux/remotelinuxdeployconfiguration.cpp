/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
#include "remotelinuxdeployconfiguration.h"

#include "deploymentinfo.h"
#include "linuxdeviceconfigurations.h"
#include "maemodeployconfigurationwidget.h"
#include "typespecificdeviceconfigurationlistmodel.h"

#include <qt4projectmanager/qt4target.h>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {
namespace {
const char DeviceIdKey[] = "Qt4ProjectManager.MaemoRunConfiguration.DeviceId";
} // anonymous namespace
} // namespace Internal

using namespace Internal;

RemoteLinuxDeployConfiguration::RemoteLinuxDeployConfiguration(ProjectExplorer::Target *target,
        const QString &id, const QString &defaultDisplayName, const QString &supportedOsType)
    : DeployConfiguration(target, id)
{
    setDefaultDisplayName(defaultDisplayName);

    // A DeploymentInfo object is only dependent on the active build
    // configuration and therefore can (and should) be shared among all
    // deploy steps. The per-target device configurations model is
    // similarly only dependent on the target.
    const QList<DeployConfiguration *> &deployConfigs
        = this->target()->deployConfigurations();
    foreach (const DeployConfiguration * const dc, deployConfigs) {
        const RemoteLinuxDeployConfiguration * const mdc
            = qobject_cast<const RemoteLinuxDeployConfiguration *>(dc);
        if (mdc) {
            m_deploymentInfo = mdc->deploymentInfo();
            m_devConfModel = mdc->m_devConfModel;
            break;
        }
    }
    if (!m_deploymentInfo) {
        m_deploymentInfo = QSharedPointer<DeploymentInfo>(new DeploymentInfo(qobject_cast<Qt4BaseTarget *>(target)));
        m_devConfModel = QSharedPointer<TypeSpecificDeviceConfigurationListModel>
            (new TypeSpecificDeviceConfigurationListModel(0, supportedOsType));
    }

    initialize();
}

RemoteLinuxDeployConfiguration::RemoteLinuxDeployConfiguration(ProjectExplorer::Target *target,
    RemoteLinuxDeployConfiguration *source) : DeployConfiguration(target, source)
{
    m_deploymentInfo = source->deploymentInfo();
    m_devConfModel = source->deviceConfigModel();
    initialize();
}

RemoteLinuxDeployConfiguration::~RemoteLinuxDeployConfiguration() {}

void RemoteLinuxDeployConfiguration::initialize()
{
    m_deviceConfiguration = deviceConfigModel()->defaultDeviceConfig();
    connect(deviceConfigModel().data(), SIGNAL(updated()),
        SLOT(handleDeviceConfigurationListUpdated()));
}

void RemoteLinuxDeployConfiguration::handleDeviceConfigurationListUpdated()
{
    setDeviceConfig(LinuxDeviceConfigurations::instance()->internalId(m_deviceConfiguration));
}

void RemoteLinuxDeployConfiguration::setDeviceConfig(LinuxDeviceConfiguration::Id internalId)
{
    m_deviceConfiguration = deviceConfigModel()->find(internalId);
    emit deviceConfigurationListChanged();
    emit currentDeviceConfigurationChanged();
}

bool RemoteLinuxDeployConfiguration::fromMap(const QVariantMap &map)
{
    if (!DeployConfiguration::fromMap(map))
        return false;
    setDeviceConfig(map.value(QLatin1String(DeviceIdKey),
        LinuxDeviceConfiguration::InvalidId).toULongLong());
    return true;
}

QVariantMap RemoteLinuxDeployConfiguration::toMap() const
{
    QVariantMap map = DeployConfiguration::toMap();
    map.insert(QLatin1String(DeviceIdKey),
        LinuxDeviceConfigurations::instance()->internalId(m_deviceConfiguration));
    return map;
}

void RemoteLinuxDeployConfiguration::setDeviceConfiguration(int index)
{
    const LinuxDeviceConfiguration::ConstPtr &newDevConf = deviceConfigModel()->deviceAt(index);
    if (m_deviceConfiguration != newDevConf) {
        m_deviceConfiguration = newDevConf;
        emit currentDeviceConfigurationChanged();
    }
}

DeployConfigurationWidget *RemoteLinuxDeployConfiguration::configurationWidget() const
{
    return new MaemoDeployConfigurationWidget;
}

QSharedPointer<DeploymentInfo> RemoteLinuxDeployConfiguration::deploymentInfo() const
{
    return m_deploymentInfo;
}

QSharedPointer<TypeSpecificDeviceConfigurationListModel> RemoteLinuxDeployConfiguration::deviceConfigModel() const
{
    return m_devConfModel;
}

LinuxDeviceConfiguration::ConstPtr RemoteLinuxDeployConfiguration::deviceConfiguration() const
{
    return m_deviceConfiguration;
}

} // namespace RemoteLinux
