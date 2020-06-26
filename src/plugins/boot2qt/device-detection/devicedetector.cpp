/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "devicedetector.h"

#include "../qdbutils.h"
#include "../qdbconstants.h"
#include "../qdbdevice.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <utils/qtcassert.h>

#include <QHash>
#include <QObject>

using namespace ProjectExplorer;

namespace Qdb {
namespace Internal {

static bool isAutodetectedQdbDevice(const IDevice::ConstPtr &device)
{
    return device
        && device->type() == Qdb::Constants::QdbLinuxOsType
        && device->isAutoDetected();
}

DeviceDetector::DeviceDetector()
{
}

DeviceDetector::~DeviceDetector()
{
    stop();
}

void DeviceDetector::start()
{
    QTC_ASSERT(m_state == Inactive, return);

    connect(&m_deviceTracker, &QdbDeviceTracker::deviceEvent,
            this, &DeviceDetector::handleDeviceEvent);
    connect(&m_deviceTracker, &QdbDeviceTracker::trackerError,
            this, &DeviceDetector::handleTrackerError);

    resetDevices();
    m_state = WaitingForDeviceUpdates;
    m_deviceTracker.start();
    m_messageTracker.start();
}

void DeviceDetector::stop()
{
    m_messageTracker.stop();

    switch (m_state) {
    case WaitingForDeviceUpdates:
        m_deviceTracker.stop();
        resetDevices();
        break;
    case Inactive:
        break;
    }
    m_state = Inactive;
}

void DeviceDetector::handleDeviceEvent(QdbDeviceTracker::DeviceEventType eventType,
                                       const QMap<QString, QString> &info)
{
    const QString serial = info.value("serial");
    if (serial.isEmpty()) {
        showMessage("Error: Did not get a serial number in a device event from QDB", false);
        return;
    }

    const Utils::Id deviceId = Constants::QdbHardwareDevicePrefix.withSuffix(':' + serial);
    const auto messagePrefix = tr("Device \"%1\" %2").arg(serial);
    DeviceManager * const dm = DeviceManager::instance();

    if (eventType == QdbDeviceTracker::NewDevice) {
        const QString name = tr("Qt Debug Bridge device %1").arg(serial);
        QdbDevice::Ptr device = QdbDevice::create();
        device->setupId(IDevice::AutoDetected, deviceId);
        device->setDisplayName(name);
        device->setType(Qdb::Constants::QdbLinuxOsType);
        device->setMachineType(IDevice::Hardware);

        const QString ipAddress = info["ipAddress"];
        device->setupDefaultNetworkSettings(ipAddress);

        IDevice::DeviceState state;
        if (ipAddress.isEmpty())
            state = IDevice::DeviceConnected;
        else
            state = IDevice::DeviceReadyToUse;
        device->setDeviceState(state);

        dm->addDevice(device);

        if (state == IDevice::DeviceConnected)
            showMessage(messagePrefix.arg("connected, waiting for IP"), false);
        else
            showMessage(messagePrefix.arg("is ready to use at ").append(ipAddress), false);
    } else if (eventType == QdbDeviceTracker::DisconnectedDevice) {
        dm->setDeviceState(deviceId, IDevice::DeviceDisconnected);
        showMessage(messagePrefix.arg("disconnected"), false);
    }
}

void DeviceDetector::handleTrackerError(const QString &errorMessage)
{
    showMessage(tr("Device detection error: %1").arg(errorMessage), true);
    stop();
}

void DeviceDetector::resetDevices()
{
    DeviceManager * const dm = DeviceManager::instance();
    for (int i = 0; i < dm->deviceCount(); ++i) {
        const IDevice::ConstPtr device = dm->deviceAt(i);
        if (isAutodetectedQdbDevice(device))
            dm->setDeviceState(device->id(), IDevice::DeviceDisconnected);
    }
}

} // namespace Internal
} // namespace Qdb
