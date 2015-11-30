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

#include "desktopdevice.h"
#include "desktopdeviceprocess.h"
#include "deviceprocesslist.h"
#include "localprocesslist.h"
#include "desktopdeviceconfigurationwidget.h"
#include "desktopprocesssignaloperation.h"
#include <projectexplorer/projectexplorerconstants.h>

#include <ssh/sshconnection.h>
#include <utils/portlist.h>

#include <QCoreApplication>

using namespace ProjectExplorer::Constants;

namespace ProjectExplorer {

DesktopDevice::DesktopDevice() : IDevice(Core::Id(DESKTOP_DEVICE_TYPE),
                                         IDevice::AutoDetected,
                                         IDevice::Hardware,
                                         Core::Id(DESKTOP_DEVICE_ID))
{
    setDisplayName(QCoreApplication::translate("ProjectExplorer::DesktopDevice", "Local PC"));
    setDeviceState(IDevice::DeviceStateUnknown);
    const QString portRange =
            QString::fromLatin1("%1-%2").arg(DESKTOP_PORT_START).arg(DESKTOP_PORT_END);
    setFreePorts(Utils::PortList::fromString(portRange));
}

DesktopDevice::DesktopDevice(const DesktopDevice &other) :
    IDevice(other)
{ }

IDevice::DeviceInfo DesktopDevice::deviceInformation() const
{
    return DeviceInfo();
}

QString DesktopDevice::displayType() const
{
    return QCoreApplication::translate("ProjectExplorer::DesktopDevice", "Desktop");
}

IDeviceWidget *DesktopDevice::createWidget()
{
    return 0;
    // DesktopDeviceConfigurationWidget currently has just one editable field viz. free ports.
    // Querying for an available port is quite straightforward. Having a field for the port
    // range can be confusing to the user. Hence, disabling the widget for now.
}

QList<Core::Id> DesktopDevice::actionIds() const
{
    return QList<Core::Id>();
}

QString DesktopDevice::displayNameForActionId(Core::Id actionId) const
{
    Q_UNUSED(actionId);
    return QString();
}

void DesktopDevice::executeAction(Core::Id actionId, QWidget *parent)
{
    Q_UNUSED(actionId);
    Q_UNUSED(parent);
}

bool DesktopDevice::canAutoDetectPorts() const
{
    return true;
}

bool DesktopDevice::canCreateProcessModel() const
{
    return true;
}

DeviceProcessList *DesktopDevice::createProcessListModel(QObject *parent) const
{
    return new Internal::LocalProcessList(sharedFromThis(), parent);
}

DeviceProcess *DesktopDevice::createProcess(QObject *parent) const
{
    return new Internal::DesktopDeviceProcess(sharedFromThis(), parent);
}

DeviceProcessSignalOperation::Ptr DesktopDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(new DesktopProcessSignalOperation());
}

QString DesktopDevice::qmlProfilerHost() const
{
    return QLatin1String("localhost");
}

IDevice::Ptr DesktopDevice::clone() const
{
    return Ptr(new DesktopDevice(*this));
}

} // namespace ProjectExplorer
