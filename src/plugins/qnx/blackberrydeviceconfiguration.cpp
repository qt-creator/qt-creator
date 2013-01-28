/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberrydeviceconfiguration.h"

#include "qnxconstants.h"
#include "blackberrydeviceconfigurationwidget.h"

#include <projectexplorer/kitinformation.h>

using namespace Qnx;
using namespace Qnx::Internal;
using namespace ProjectExplorer;

BlackBerryDeviceConfiguration::BlackBerryDeviceConfiguration()
    : RemoteLinux::LinuxDevice()
{
}

BlackBerryDeviceConfiguration::BlackBerryDeviceConfiguration(const QString &name, Core::Id type,
                                                             IDevice::MachineType machineType,
                                                             IDevice::Origin origin, Core::Id id)
    : RemoteLinux::LinuxDevice(name, type, machineType, origin, id)
{
}

BlackBerryDeviceConfiguration::BlackBerryDeviceConfiguration(const BlackBerryDeviceConfiguration &other)
    : RemoteLinux::LinuxDevice(other)
    , m_debugToken(other.m_debugToken)
{
}

BlackBerryDeviceConfiguration::Ptr BlackBerryDeviceConfiguration::create()
{
    return Ptr(new BlackBerryDeviceConfiguration);
}

BlackBerryDeviceConfiguration::Ptr BlackBerryDeviceConfiguration::create(const QString &name, Core::Id type,
                                                                         IDevice::MachineType machineType,
                                                                         IDevice::Origin origin, Core::Id id)
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
    RemoteLinux::LinuxDevice::fromMap(map);
    m_debugToken = map.value(QLatin1String(Constants::QNX_DEBUG_TOKEN_KEY)).toString();
}

IDevice::Ptr BlackBerryDeviceConfiguration::clone() const
{
    return Ptr(new BlackBerryDeviceConfiguration(*this));
}

BlackBerryDeviceConfiguration::ConstPtr BlackBerryDeviceConfiguration::device(const Kit *k)
{
    IDevice::ConstPtr dev = DeviceKitInformation::device(k);
    return dev.dynamicCast<const BlackBerryDeviceConfiguration>();
}

QString BlackBerryDeviceConfiguration::displayType() const
{
    return tr("BlackBerry");
}

IDeviceWidget *BlackBerryDeviceConfiguration::createWidget()
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
    QVariantMap map = RemoteLinux::LinuxDevice::toMap();
    map.insert(QLatin1String(Constants::QNX_DEBUG_TOKEN_KEY), m_debugToken);
    return map;
}
