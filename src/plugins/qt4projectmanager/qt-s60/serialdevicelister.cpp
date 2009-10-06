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
#include <QtGui/QApplication>
#include <QtGui/QWidget>
#include <QtDebug>

using namespace Qt4ProjectManager::Internal;

namespace {
    const char * const REGKEY_CURRENT_CONTROL_SET = "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet";
    const char * const USBSER = "Services/usbser/Enum";
}

SerialDeviceLister::SerialDeviceLister(QObject *parent)
        : QObject(parent),
        m_initialized(false)
{

}

SerialDeviceLister::~SerialDeviceLister()
{
}

QList<SerialDeviceLister::SerialDevice> SerialDeviceLister::serialDevices() const
{
    if (!m_initialized) {
        updateSilently();
        m_initialized = true;
    }
    return m_devices;
}

QString SerialDeviceLister::friendlyNameForPort(const QString &port) const
{
    foreach (const SerialDevice &device, m_devices) {
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
    m_devices.clear();
#ifdef Q_OS_WIN32
    QSettings registry(REGKEY_CURRENT_CONTROL_SET, QSettings::NativeFormat);
    int count = registry.value(QString::fromLatin1("%1/Count").arg(USBSER)).toInt();
    for (int i = 0; i < count; ++i) {
        QString driver = registry.value(QString::fromLatin1("%1/%2").arg(USBSER).arg(i)).toString();
        if (driver.contains("JAVACOMM")) {
            driver.replace('\\', '/');
            SerialDeviceLister::SerialDevice device;
            device.friendlyName = registry.value(QString::fromLatin1("Enum/%1/FriendlyName").arg(driver)).toString();
            device.portName = registry.value(QString::fromLatin1("Enum/%1/Device Parameters/PortName").arg(driver)).toString();
            m_devices.append(device);
        }
    }
#endif
}
