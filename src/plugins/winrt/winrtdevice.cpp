/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "winrtdevice.h"
#include "winrtconstants.h"

#include <projectexplorer/devicesupport/desktopprocesssignaloperation.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QCoreApplication>
#include <QWizard>

using namespace ProjectExplorer;

namespace WinRt {
namespace Internal {

WinRtDevice::WinRtDevice() : m_deviceId(-1)
{
}

WinRtDevice::WinRtDevice(Core::Id type, MachineType machineType, Core::Id internalId, int deviceId)
    : IDevice(type, AutoDetected, machineType, internalId), m_deviceId(deviceId)
{
}

WinRtDevice::WinRtDevice(const WinRtDevice &other)
    : IDevice(other), m_deviceId(other.m_deviceId)
{
}

QString WinRtDevice::displayType() const
{
    return displayNameForType(type());
}

IDeviceWidget *WinRtDevice::createWidget()
{
    return 0;
}

QList<Core::Id> WinRtDevice::actionIds() const
{
    return QList<Core::Id>();
}

QString WinRtDevice::displayNameForActionId(Core::Id actionId) const
{
    Q_UNUSED(actionId);
    return QString();
}

void WinRtDevice::executeAction(Core::Id actionId, QWidget *parent)
{
    Q_UNUSED(actionId);
    Q_UNUSED(parent);
}

DeviceProcessSignalOperation::Ptr WinRtDevice::signalOperation() const
{
    class WinRtDesktopSignalOperation : public DesktopProcessSignalOperation
    {
    public:
        WinRtDesktopSignalOperation() {}
        ~WinRtDesktopSignalOperation() {}
    };

    return DeviceProcessSignalOperation::Ptr(new WinRtDesktopSignalOperation());
}

void WinRtDevice::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
    m_deviceId = map.value(QStringLiteral("WinRtRunnerDeviceId")).toInt();
}

QVariantMap WinRtDevice::toMap() const
{
    QVariantMap map = IDevice::toMap();
    map.insert(QStringLiteral("WinRtRunnerDeviceId"), m_deviceId);
    return map;
}

IDevice::Ptr WinRtDevice::clone() const
{
    return IDevice::Ptr(new WinRtDevice(*this));
}

QString WinRtDevice::displayNameForType(Core::Id type)
{
    if (type == Constants::WINRT_DEVICE_TYPE_LOCAL)
        return QCoreApplication::translate("WinRt::Internal::WinRtDevice",
                                           "Windows Runtime (Local)");
    if (type == Constants::WINRT_DEVICE_TYPE_PHONE)
        return QCoreApplication::translate("WinRt::Internal::WinRtDevice",
                                           "Windows Phone");
    if (type == Constants::WINRT_DEVICE_TYPE_EMULATOR)
        return QCoreApplication::translate("WinRt::Internal::WinRtDevice",
                                           "Windows Phone Emulator");
    return QString();
}

} // Internal
} // WinRt
