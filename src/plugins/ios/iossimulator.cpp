/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "iossimulator.h"
#include "iosconstants.h"

#include <projectexplorer/kitinformation.h>

#include <QCoreApplication>

using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

const char *SIMULATOR_PATH_KEY = "SIMULATOR_PATH";

IosSimulator::IosSimulator(Core::Id id, Utils::FileName simulatorPath)
    : IDevice(Core::Id(Constants::IOS_SIMULATOR_TYPE),
              IDevice::AutoDetected,
              IDevice::Emulator,
              id),
      m_simulatorPath(simulatorPath)
{
    setDisplayName(QCoreApplication::translate("Ios::Internal::IosSimulator", "iOS Simulator"));
    setDeviceState(DeviceReadyToUse);
}

IosSimulator::IosSimulator()
    : IDevice(Core::Id(Constants::IOS_SIMULATOR_TYPE),
                             IDevice::AutoDetected,
                             IDevice::Emulator,
                             Core::Id(Constants::IOS_SIMULATOR_DEVICE_ID))
{
    setDisplayName(QCoreApplication::translate("Ios::Internal::IosSimulator", "iOS Simulator"));
    setDeviceState(DeviceReadyToUse);
}

IosSimulator::IosSimulator(const IosSimulator &other)
    : IDevice(other)
{
    setDisplayName(QCoreApplication::translate("Ios::Internal::IosSimulator", "iOS Simulator"));
    setDeviceState(DeviceReadyToUse);
}


IDevice::DeviceInfo IosSimulator::deviceInformation() const
{
    return IDevice::DeviceInfo();
}

QString IosSimulator::displayType() const
{
    return QCoreApplication::translate("Ios::Internal::IosSimulator", "iOS Simulator");
}

IDeviceWidget *IosSimulator::createWidget()
{
    return 0;
}

QList<Core::Id> IosSimulator::actionIds() const
{
    return QList<Core::Id>();
}

QString IosSimulator::displayNameForActionId(Core::Id actionId) const
{
    Q_UNUSED(actionId)
    return QString();
}

void IosSimulator::executeAction(Core::Id actionId, QWidget *parent)
{
    Q_UNUSED(actionId)
    Q_UNUSED(parent)
}

DeviceProcessSignalOperation::Ptr IosSimulator::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr();
}

Utils::FileName IosSimulator::simulatorPath() const
{
    return m_simulatorPath;
}

IDevice::Ptr IosSimulator::clone() const
{
    return IDevice::Ptr(new IosSimulator(*this));
}

void IosSimulator::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
    m_simulatorPath = Utils::FileName::fromString(map.value(QLatin1String(SIMULATOR_PATH_KEY))
                                                  .toString());
}

QVariantMap IosSimulator::toMap() const
{
    QVariantMap res = IDevice::toMap();
    res.insert(QLatin1String(SIMULATOR_PATH_KEY), simulatorPath().toString());
    return res;
}

IosSimulator::ConstPtr IosKitInformation::simulator(Kit *kit)
{
    if (!kit)
        return IosSimulator::ConstPtr();
    ProjectExplorer::IDevice::ConstPtr dev = ProjectExplorer::DeviceKitInformation::device(kit);
    IosSimulator::ConstPtr res = dev.dynamicCast<const IosSimulator>();
    return res;
}

} // namespace Internal
} // namespace Ios
