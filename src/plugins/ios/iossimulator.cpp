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
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "iossimulator.h"
#include "iosconstants.h"

#include <projectexplorer/kitinformation.h>

#include <QCoreApplication>
#include <QProcess>

using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

IosSimulator::IosSimulator(Core::Id id)
    : IDevice(Core::Id(Constants::IOS_SIMULATOR_TYPE),
              IDevice::AutoDetected,
              IDevice::Emulator,
              id),
      m_lastPort(Constants::IOS_SIMULATOR_PORT_START)
{
    setDisplayName(QCoreApplication::translate("Ios::Internal::IosSimulator", "iOS Simulator"));
    setDeviceState(DeviceReadyToUse);
}

IosSimulator::IosSimulator()
    : IDevice(Core::Id(Constants::IOS_SIMULATOR_TYPE),
                             IDevice::AutoDetected,
                             IDevice::Emulator,
                             Core::Id(Constants::IOS_SIMULATOR_DEVICE_ID)),
      m_lastPort(Constants::IOS_SIMULATOR_PORT_START)
{
    setDisplayName(QCoreApplication::translate("Ios::Internal::IosSimulator", "iOS Simulator"));
    setDeviceState(DeviceReadyToUse);
}

IosSimulator::IosSimulator(const IosSimulator &other)
    : IDevice(other), m_lastPort(other.m_lastPort)
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

IDevice::Ptr IosSimulator::clone() const
{
    return IDevice::Ptr(new IosSimulator(*this));
}

void IosSimulator::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
}

QVariantMap IosSimulator::toMap() const
{
    QVariantMap res = IDevice::toMap();
    return res;
}

quint16 IosSimulator::nextPort() const
{
    for (int i = 0; i < 100; ++i) {
        // use qrand instead?
        if (++m_lastPort >= Constants::IOS_SIMULATOR_PORT_END)
            m_lastPort = Constants::IOS_SIMULATOR_PORT_START;
        QProcess portVerifier;
        // this is a bit too broad (it does not check just listening sockets, but also connections
        // to that port from this computer)
        portVerifier.start(QLatin1String("lsof"), QStringList() << QLatin1String("-n")
                         << QLatin1String("-P") << QLatin1String("-i")
                         << QString::fromLatin1(":%1").arg(m_lastPort));
        if (!portVerifier.waitForStarted())
            break;
        portVerifier.closeWriteChannel();
        if (!portVerifier.waitForFinished())
            break;
        if (portVerifier.exitStatus() != QProcess::NormalExit
                || portVerifier.exitCode() != 0)
            break;
    }
    return m_lastPort;
}

bool IosSimulator::canAutoDetectPorts() const
{
    return true;
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
