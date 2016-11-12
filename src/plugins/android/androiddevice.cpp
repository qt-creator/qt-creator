/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androiddevice.h"
#include "androidconstants.h"
#include "androidsignaloperation.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runnables.h>

#include <QCoreApplication>

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

AndroidDevice::AndroidDevice()
    : IDevice(Core::Id(Constants::ANDROID_DEVICE_TYPE),
                             IDevice::AutoDetected,
                             IDevice::Hardware,
                             Core::Id(Constants::ANDROID_DEVICE_ID))
{
    setDisplayName(QCoreApplication::translate("Android::Internal::AndroidDevice", "Run on Android"));
    setDeviceState(DeviceReadyToUse);
}

AndroidDevice::AndroidDevice(const AndroidDevice &other)
    : IDevice(other)
{ }


IDevice::DeviceInfo AndroidDevice::deviceInformation() const
{
    return IDevice::DeviceInfo();
}

QString AndroidDevice::displayType() const
{
    return QCoreApplication::translate("Android::Internal::AndroidDevice", "Android");
}

IDeviceWidget *AndroidDevice::createWidget()
{
    return 0;
}

QList<Core::Id> AndroidDevice::actionIds() const
{
    return QList<Core::Id>();
}

QString AndroidDevice::displayNameForActionId(Core::Id actionId) const
{
    Q_UNUSED(actionId)
    return QString();
}

void AndroidDevice::executeAction(Core::Id actionId, QWidget *parent)
{
    Q_UNUSED(actionId)
    Q_UNUSED(parent)
}

bool AndroidDevice::canAutoDetectPorts() const
{
    return true;
}

DeviceProcessSignalOperation::Ptr AndroidDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(new AndroidSignalOperation());
}

IDevice::Ptr AndroidDevice::clone() const
{
    return IDevice::Ptr(new AndroidDevice(*this));
}

Connection AndroidDevice::toolControlChannel(const ControlChannelHint &) const
{
    return HostName("localhost");
}

} // namespace Internal
} // namespace Android
