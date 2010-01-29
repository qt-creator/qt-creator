/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "serialdevicelister.h"

#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtCore/QFileInfo>
#include <QtGui/QApplication>
#include <QtGui/QWidget>
#include <QtDebug>

using namespace Qt4ProjectManager::Internal;

namespace {
    const char * const REGKEY_CURRENT_CONTROL_SET = "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet";
    const char * const USBSER = "Services/usbser/Enum";
}

const char *SerialDeviceLister::linuxBlueToothDeviceRootC = "/dev/rfcomm";

CommunicationDevice::CommunicationDevice(DeviceCommunicationType t,
                                         const QString &p,
                                         const QString &f) :
    portName(p),
    friendlyName(f),
    type(t)
{
}

SerialDeviceLister::SerialDeviceLister(QObject *parent)
        : QObject(parent),
        m_initialized(false)
{

}

SerialDeviceLister::~SerialDeviceLister()
{
}

QList<CommunicationDevice> SerialDeviceLister::communicationDevices() const
{
    if (!m_initialized) {
        updateSilently();
        m_initialized = true;
    }
    return m_devices2;
}

QString SerialDeviceLister::friendlyNameForPort(const QString &port) const
{
    foreach (const CommunicationDevice &device, m_devices2) {
        if (device.portName == port)
            return device.friendlyName;
    }
    return QString();
}

void SerialDeviceLister::update()
{
    updateSilently();
    emit updated();
}

void SerialDeviceLister::updateSilently() const
{
    m_devices2 = serialPorts() + blueToothDevices();
}

QList<CommunicationDevice> SerialDeviceLister::serialPorts() const
{
    QList<CommunicationDevice> rc;
#ifdef Q_OS_WIN32
    QSettings registry(REGKEY_CURRENT_CONTROL_SET, QSettings::NativeFormat);
    const int count = registry.value(QString::fromLatin1("%1/Count").arg(USBSER)).toInt();
    for (int i = 0; i < count; ++i) {
        QString driver = registry.value(QString::fromLatin1("%1/%2").arg(USBSER).arg(i)).toString();
        if (driver.contains("JAVACOMM")) {
            driver.replace('\\', '/');
            CommunicationDevice device(SerialPortCommunication);
            device.friendlyName = registry.value(QString::fromLatin1("Enum/%1/FriendlyName").arg(driver)).toString();
            device.portName = registry.value(QString::fromLatin1("Enum/%1/Device Parameters/PortName").arg(driver)).toString();
            rc.append(device);
        }
    }
#endif
    return rc;
}

QList<CommunicationDevice> SerialDeviceLister::blueToothDevices() const
{
    QList<CommunicationDevice> rc;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // Bluetooth devices are created on connection. List the existing ones
    // or at least the first one.
    const QString prefix = QLatin1String(linuxBlueToothDeviceRootC);
    const QString friendlyFormat = QLatin1String("Bluetooth device (%1)");
    for (int d = 0; d < 4; d++) {
        CommunicationDevice device(BlueToothCommunication, prefix + QString::number(d));
        if (d == 0 || QFileInfo(device.portName).exists()) {
            device.friendlyName = friendlyFormat.arg(device.portName);
            rc.push_back(device);
        }
    }
#endif
    return rc;
}
