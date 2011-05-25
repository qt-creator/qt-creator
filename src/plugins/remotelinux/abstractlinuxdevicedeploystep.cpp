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
#include "abstractlinuxdevicedeploystep.h"

#include "maemoconstants.h"
#include "maemodeploystepwidget.h"
#include "maemopertargetdeviceconfigurationlistmodel.h"
#include "qt4maemodeployconfiguration.h"

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {


AbstractLinuxDeviceDeployStep::AbstractLinuxDeviceDeployStep(DeployConfiguration *dc)
    : m_helper(qobject_cast<Qt4MaemoDeployConfiguration *>(dc))
{
}

bool AbstractLinuxDeviceDeployStep::isDeploymentPossible(QString &whyNot) const
{
    if (!m_helper.deviceConfig()) {
        whyNot = tr("No valid device set.");
        return false;
    }
    return isDeploymentPossibleInternal(whyNot);
}

bool AbstractLinuxDeviceDeployStep::initialize(QString &errorMsg)
{
    if (!isDeploymentPossible(errorMsg))
        return false;
    m_helper.prepareDeployment();
    return true;
}


LinuxDeviceDeployStepHelper::LinuxDeviceDeployStepHelper(Qt4MaemoDeployConfiguration *dc)
    : m_deployConfiguration(dc)
{
    m_deviceConfig = dc->deviceConfigModel()->defaultDeviceConfig();
    connect(dc->deviceConfigModel().data(), SIGNAL(updated()),
        SLOT(handleDeviceConfigurationsUpdated()));
}

LinuxDeviceDeployStepHelper::~LinuxDeviceDeployStepHelper() {}

void LinuxDeviceDeployStepHelper::handleDeviceConfigurationsUpdated()
{
    setDeviceConfig(MaemoDeviceConfigurations::instance()->internalId(m_deviceConfig));
}

void LinuxDeviceDeployStepHelper::setDeviceConfig(MaemoDeviceConfig::Id internalId)
{
    m_deviceConfig = deployConfiguration()->deviceConfigModel()->find(internalId);
    emit deviceConfigChanged();
}

void LinuxDeviceDeployStepHelper::setDeviceConfig(int i)
{
    m_deviceConfig = deployConfiguration()->deviceConfigModel()->deviceAt(i);
    emit deviceConfigChanged();
}

QVariantMap LinuxDeviceDeployStepHelper::toMap() const
{
    QVariantMap map;
    map.insert(DeviceIdKey,
        MaemoDeviceConfigurations::instance()->internalId(m_deviceConfig));
    return map;
}

bool LinuxDeviceDeployStepHelper::fromMap(const QVariantMap &map)
{
    setDeviceConfig(map.value(DeviceIdKey, MaemoDeviceConfig::InvalidId).toULongLong());
    return true;
}

} // namespace Internal
} // namespace RemoteLinux
