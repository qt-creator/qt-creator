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

#include "blackberrydeviceconfiguration.h"

#include "qnxconstants.h"
#include "blackberrydeviceconfigurationwidget.h"

#include <projectexplorer/profileinformation.h>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryDeviceConfiguration::BlackBerryDeviceConfiguration()
    : RemoteLinux::LinuxDeviceConfiguration()
{
}

BlackBerryDeviceConfiguration::BlackBerryDeviceConfiguration(const QString &name, Core::Id type,
                                                             RemoteLinux::LinuxDeviceConfiguration::MachineType machineType,
                                                             ProjectExplorer::IDevice::Origin origin, Core::Id id)
    : RemoteLinux::LinuxDeviceConfiguration(name, type, machineType, origin, id)
{
}

BlackBerryDeviceConfiguration::BlackBerryDeviceConfiguration(const BlackBerryDeviceConfiguration &other)
    : RemoteLinux::LinuxDeviceConfiguration(other)
    , m_debugToken(other.m_debugToken)
{
}

BlackBerryDeviceConfiguration::Ptr BlackBerryDeviceConfiguration::create()
{
    return Ptr(new BlackBerryDeviceConfiguration);
}

BlackBerryDeviceConfiguration::Ptr BlackBerryDeviceConfiguration::create(const QString &name, Core::Id type,
                                                                         RemoteLinux::LinuxDeviceConfiguration::MachineType machineType,
                                                                         ProjectExplorer::IDevice::Origin origin, Core::Id id)
{
    return Ptr(new BlackBerryDeviceConfiguration(name, type, machineType, origin, id));
}

QString BlackBerryDeviceConfiguration::debugToken() const
{
    return m_debugToken;
}

void BlackBerryDeviceConfiguration::setDebugToken(const QString &debugToken)
{
    m_debugToken = debugToken;
}

void BlackBerryDeviceConfiguration::fromMap(const QVariantMap &map)
{
    RemoteLinux::LinuxDeviceConfiguration::fromMap(map);
    m_debugToken = map.value(QLatin1String(Constants::QNX_DEBUG_TOKEN_KEY)).toString();
}

ProjectExplorer::IDevice::Ptr BlackBerryDeviceConfiguration::clone() const
{
    return Ptr(new BlackBerryDeviceConfiguration(*this));
}

BlackBerryDeviceConfiguration::ConstPtr BlackBerryDeviceConfiguration::device(const ProjectExplorer::Profile *p)
{
    ProjectExplorer::IDevice::ConstPtr dev = ProjectExplorer::DeviceProfileInformation::device(p);
    return dev.dynamicCast<const BlackBerryDeviceConfiguration>();
}

QString BlackBerryDeviceConfiguration::displayType() const
{
    return tr("BlackBerry");
}

ProjectExplorer::IDeviceWidget *BlackBerryDeviceConfiguration::createWidget()
{
    return new BlackBerryDeviceConfigurationWidget(sharedFromThis()
                                                   .staticCast<BlackBerryDeviceConfiguration>());
}

QList<Core::Id> BlackBerryDeviceConfiguration::actionIds() const
{
    return QList<Core::Id>();
}

QString BlackBerryDeviceConfiguration::displayNameForActionId(Core::Id actionId) const
{
    Q_UNUSED(actionId);
    return QString();
}

void BlackBerryDeviceConfiguration::executeAction(Core::Id actionId, QWidget *parent) const
{
    Q_UNUSED(actionId);
    Q_UNUSED(parent);
}

QVariantMap BlackBerryDeviceConfiguration::toMap() const
{
    QVariantMap map = RemoteLinux::LinuxDeviceConfiguration::toMap();
    map.insert(QLatin1String(Constants::QNX_DEBUG_TOKEN_KEY), m_debugToken);
    return map;
}
