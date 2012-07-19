/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "symbianidevice.h"

#include "symbianideviceconfigwidget.h"
#include "symbianidevicefactory.h"

#include <symbianutils/symbiandevicemanager.h>

#include <QCoreApplication>

namespace {
const char SERIAL_PORT_NAME_KEY[] = "Qt4ProjectManager.S60DeployConfiguration.SerialPortName";
const char DEVICE_ADDRESS_KEY[] = "Qt4ProjectManager.S60DeployConfiguration.DeviceAddress";
const char DEVICE_PORT_KEY[] = "Qt4ProjectManager.S60DeployConfiguration.DevicePort";
const char COMMUNICATION_CHANNEL_KEY[] = "Qt4ProjectManager.S60DeployConfiguration.CommunicationChannel";

const char DEFAULT_CODA_TCP_PORT[] = "65029";
} // namespace

namespace Qt4ProjectManager {

SymbianIDevice::SymbianIDevice() :
    ProjectExplorer::IDevice(Internal::SymbianIDeviceFactory::deviceType(),
                             ProjectExplorer::IDevice::AutoDetected,
                             Core::Id("Symbian Device")),
    m_port(QLatin1String(DEFAULT_CODA_TCP_PORT)),
    m_communicationChannel(CommunicationCodaSerialConnection)
{
    setDisplayName("Symbian Device");
    updateState();
}

SymbianIDevice::SymbianIDevice(const QVariantMap &map)
{
    fromMap(map);
}

ProjectExplorer::IDevice::DeviceInfo SymbianIDevice::deviceInformation() const
{
    ProjectExplorer::IDevice::DeviceInfo result;
    switch (communicationChannel()) {
    case SymbianIDevice::CommunicationCodaSerialConnection: {
            const SymbianUtils::SymbianDeviceManager *sdm = SymbianUtils::SymbianDeviceManager::instance();
            const int deviceIndex = sdm->findByPortName(serialPortName());
            if (deviceIndex == -1) {
                result << ProjectExplorer::IDevice::DeviceInfoItem(
                              QCoreApplication::translate("Qt4ProjectManager::SymbianIDevice", "Device"),
                              QCoreApplication::translate("Qt4ProjectManager::SymbianIDevice", "Not connected"));
            } else {
                // device connected
                const SymbianUtils::SymbianDevice device = sdm->devices().at(deviceIndex);
                result << ProjectExplorer::IDevice::DeviceInfoItem(
                              QCoreApplication::translate("Qt4ProjectManager::SymbianIDevice", "Device"),
                              //: %1 device friendly name, %2 additional information
                              QCoreApplication::translate("Qt4ProjectManager::SymbianIDevice", "%1, %2")
                              .arg(device.friendlyName(), device.additionalInformation()));
            }
        }
        break;
    case SymbianIDevice::CommunicationCodaTcpConnection: {
            if (!address().isEmpty() && !port().isEmpty()) {
                result << ProjectExplorer::IDevice::DeviceInfoItem(
                              QCoreApplication::translate("Qt4ProjectManager::SymbianIDevice", "IP address"),
                              QCoreApplication::translate("Qt4ProjectManager::SymbianIDevice", "%1:%2")
                              .arg(address(), port()));
            }
        }
        break;
    default:
        break;
    }
    return result;
}

SymbianIDevice::SymbianIDevice(const SymbianIDevice &other) :
    ProjectExplorer::IDevice(other)
{
    m_address = other.m_address;
    m_communicationChannel = other.m_communicationChannel;
    m_port = other.m_port;
    m_serialPortName = other.m_serialPortName;
}

ProjectExplorer::IDevice::Ptr SymbianIDevice::clone() const
{
    return Ptr(new SymbianIDevice(*this));
}

QString SymbianIDevice::serialPortName() const
{
    return m_serialPortName;
}

void SymbianIDevice::setSerialPortName(const QString &name)
{
    const QString &candidate = name.trimmed();
    if (m_serialPortName == candidate)
        return;
    m_serialPortName = candidate;
    updateState();
}

QString SymbianIDevice::address() const
{
    return m_address;
}

void SymbianIDevice::setAddress(const QString &address)
{
    if (m_address != address) {
        m_address = address;
        setDeviceState(IDevice::DeviceStateUnknown);
    }
}


QString SymbianIDevice::port() const
{
    return m_port;
}

void SymbianIDevice::setPort(const QString &port)
{
    if (m_port != port) {
        if (port.isEmpty()) //setup the default CODA's port
            m_port = QLatin1String(DEFAULT_CODA_TCP_PORT);
        else
            m_port = port;
        setDeviceState(IDevice::DeviceStateUnknown);
    }
}

SymbianIDevice::CommunicationChannel SymbianIDevice::communicationChannel() const
{
    return m_communicationChannel;
}

void SymbianIDevice::setCommunicationChannel(CommunicationChannel channel)
{
    if (m_communicationChannel != channel) {
        m_communicationChannel = channel;
        setDeviceState(IDevice::DeviceStateUnknown);
    }
    updateState();
}

void SymbianIDevice::fromMap(const QVariantMap &map)
{
    ProjectExplorer::IDevice::fromMap(map);
    m_serialPortName = map.value(QLatin1String(SERIAL_PORT_NAME_KEY)).toString().trimmed();
    m_address = map.value(QLatin1String(DEVICE_ADDRESS_KEY)).toString();
    m_port = map.value(QLatin1String(DEVICE_PORT_KEY), QString(QLatin1String(DEFAULT_CODA_TCP_PORT))).toString();
    m_communicationChannel = static_cast<CommunicationChannel>(map.value(QLatin1String(COMMUNICATION_CHANNEL_KEY),
                                                                         QVariant(CommunicationCodaSerialConnection)).toInt());
    updateState();
}

QString SymbianIDevice::displayType() const
{
    return QCoreApplication::translate("Qt4ProjectManager::SymbianIDevice", "Symbian Device");
}

ProjectExplorer::IDeviceWidget *SymbianIDevice::createWidget()
{
    return new Internal::SymbianIDeviceConfigurationWidget(sharedFromThis());
}

QList<Core::Id> SymbianIDevice::actionIds() const
{
    return QList<Core::Id>();
}

QString SymbianIDevice::displayNameForActionId(Core::Id actionId) const
{
    Q_UNUSED(actionId);
    return QString();
}

void SymbianIDevice::executeAction(Core::Id actionId, QWidget *parent) const
{
    Q_UNUSED(actionId);
    Q_UNUSED(parent);
}

QVariantMap SymbianIDevice::toMap() const
{
    QVariantMap map(ProjectExplorer::IDevice::toMap());
    map.insert(QLatin1String(SERIAL_PORT_NAME_KEY), m_serialPortName);
    map.insert(QLatin1String(DEVICE_ADDRESS_KEY), QVariant(m_address));
    map.insert(QLatin1String(DEVICE_PORT_KEY), m_port);
    map.insert(QLatin1String(COMMUNICATION_CHANNEL_KEY), QVariant(m_communicationChannel));
    return map;
}

void SymbianIDevice::updateState()
{
    if (m_communicationChannel == CommunicationCodaSerialConnection) {
        SymbianUtils::SymbianDeviceManager *sdm = SymbianUtils::SymbianDeviceManager::instance();
        if (m_serialPortName.isEmpty()) {
            // Find first port in use:
            SymbianUtils::SymbianDeviceManager::SymbianDeviceList deviceList
                    = sdm->devices();
            foreach (const SymbianUtils::SymbianDevice &d, deviceList) {
                if (d.portName().isEmpty())
                    continue;

                m_serialPortName = d.portName();
                break;
            }
        }

        setDeviceState(sdm->findByPortName(m_serialPortName) >= 0
                        ? ProjectExplorer::IDevice::DeviceReadyToUse
                        : ProjectExplorer::IDevice::DeviceDisconnected);
    } else {
        setDeviceState(ProjectExplorer::IDevice::DeviceStateUnknown);
    }
}

} // namespace qt4projectmanager
