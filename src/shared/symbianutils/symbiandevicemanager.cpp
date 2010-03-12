/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "symbiandevicemanager.h"
#include "trkdevice.h"

#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtCore/QFileInfo>
#include <QtCore/QtDebug>
#include <QtCore/QTextStream>
#include <QtCore/QSharedData>
#include <QtCore/QScopedPointer>
#include <QtCore/QSignalMapper>

namespace SymbianUtils {

enum { debug = 0 };

static const char REGKEY_CURRENT_CONTROL_SET[] = "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet";
static const char USBSER[] = "Services/usbser/Enum";

const char *SymbianDeviceManager::linuxBlueToothDeviceRootC = "/dev/rfcomm";

// ------------- SymbianDevice
class SymbianDeviceData : public QSharedData {
public:
    SymbianDeviceData();
    ~SymbianDeviceData();

    inline bool isOpen() const { return !device.isNull() && device->isOpen(); }
    void forcedClose();

    QString portName;
    QString friendlyName;
    QString deviceDesc;
    QString manufacturer;
    QString additionalInformation;

    DeviceCommunicationType type;
    QSharedPointer<trk::TrkDevice> device;
    bool deviceAcquired;
};

SymbianDeviceData::SymbianDeviceData() :
        type(SerialPortCommunication),
        deviceAcquired(false)
{
}

SymbianDeviceData::~SymbianDeviceData()
{
    forcedClose();
}

void SymbianDeviceData::forcedClose()
{
    // Close the device when unplugging. Should devices be in 'acquired' state,
    // their owners should hit on write failures.
    // Apart from the <shared item> destructor, also called by the devicemanager
    // to ensure it also happens if other shared instances are still around.
    if (isOpen()) {
        if (deviceAcquired)
            qWarning("Device on '%s' unplugged while an operation is in progress.",
                     qPrintable(portName));
        device->close();
    }
}

SymbianDevice::SymbianDevice(SymbianDeviceData *data) :
    m_data(data)
{
}

SymbianDevice::SymbianDevice() :
    m_data(new SymbianDeviceData)
{
}
SymbianDevice::SymbianDevice(const SymbianDevice &rhs) :
        m_data(rhs.m_data)
{
}

SymbianDevice &SymbianDevice::operator=(const SymbianDevice &rhs)
{
    if (this != &rhs)
        m_data = rhs.m_data;
    return *this;
}

SymbianDevice::~SymbianDevice()
{
}

void SymbianDevice::forcedClose()
{
    m_data->forcedClose();
}

QString SymbianDevice::portName() const
{
    return m_data->portName;
}

QString SymbianDevice::friendlyName() const
{
    return m_data->friendlyName;
}

QString SymbianDevice::additionalInformation() const
{
    return m_data->additionalInformation;
}

void SymbianDevice::setAdditionalInformation(const QString &a)
{
    m_data->additionalInformation = a;
}

SymbianDevice::TrkDevicePtr SymbianDevice::acquireDevice()
{
    if (debug)
        qDebug() << "SymbianDevice::acquireDevice" << m_data->portName
                << "acquired: " << m_data->deviceAcquired << " open: " << isOpen();
    if (isNull() || m_data->deviceAcquired)
        return TrkDevicePtr();
    if (m_data->device.isNull()) {
        m_data->device = TrkDevicePtr(new trk::TrkDevice);
        m_data->device->setPort(m_data->portName);
        m_data->device->setSerialFrame(m_data->type == SerialPortCommunication);
    }
    m_data->deviceAcquired = true;
    return m_data->device;
}

void SymbianDevice::releaseDevice(TrkDevicePtr *ptr /* = 0 */)
{
    if (debug)
        qDebug() << "SymbianDevice::releaseDevice" << m_data->portName
                << " open: " << isOpen();
    if (m_data->deviceAcquired) {
        if (m_data->device->isOpen())
            m_data->device->clearWriteQueue();
        // Release if a valid pointer was passed in.
        if (ptr && !ptr->isNull()) {
            ptr->data()->disconnect();
            *ptr = TrkDevicePtr();
        }
        m_data->deviceAcquired = false;
    } else {
        qWarning("Internal error: Attempt to release device that is not acquired.");
    }
}

QString SymbianDevice::deviceDesc() const
{
    return m_data->deviceDesc;
}

QString SymbianDevice::manufacturer() const
{
    return m_data->manufacturer;
}

DeviceCommunicationType SymbianDevice::type() const
{
    return m_data->type;
}

bool SymbianDevice::isNull() const
{
    return m_data->portName.isEmpty();
}

bool SymbianDevice::isOpen() const
{
    return m_data->isOpen();
}

QString SymbianDevice::toString() const
{
    QString rc;
    QTextStream str(&rc);
    format(str);
    return rc;
}

void SymbianDevice::format(QTextStream &str) const
{
    str << (m_data->type == BlueToothCommunication ? "Bluetooth: " : "Serial: ")
        << m_data->portName;
    if (!m_data->friendlyName.isEmpty()) {
        str << " (" << m_data->friendlyName;
        if (!m_data->deviceDesc.isEmpty())
          str << " / " << m_data->deviceDesc;
        str << ')';
    }
    if (!m_data->manufacturer.isEmpty())
        str << " [" << m_data->manufacturer << ']';
}

// Compare by port and friendly name
int SymbianDevice::compare(const SymbianDevice &rhs) const
{
    if (const int prc = m_data->portName.compare(rhs.m_data->portName))
        return prc;
    if (const int frc = m_data->friendlyName.compare(rhs.m_data->friendlyName))
        return frc;
    return 0;
}

SYMBIANUTILS_EXPORT QDebug operator<<(QDebug d, const SymbianDevice &cd)
{
    d.nospace() << cd.toString();
    return d;
}

// ------------- SymbianDeviceManagerPrivate
struct SymbianDeviceManagerPrivate {
    SymbianDeviceManagerPrivate() : m_initialized(false), m_destroyReleaseMapper(0) {}

    bool m_initialized;
    SymbianDeviceManager::SymbianDeviceList m_devices;
    QSignalMapper *m_destroyReleaseMapper;
};

SymbianDeviceManager::SymbianDeviceManager(QObject *parent) :
    QObject(parent),
    d(new SymbianDeviceManagerPrivate)
{
}

SymbianDeviceManager::~SymbianDeviceManager()
{
    delete d;
}

SymbianDeviceManager::SymbianDeviceList SymbianDeviceManager::devices() const
{
    ensureInitialized();
    return d->m_devices;
}

QString SymbianDeviceManager::toString() const
{
    QString rc;
    QTextStream str(&rc);
    str << d->m_devices.size() << " devices:\n";
    const int count = d->m_devices.size();
    for (int i = 0; i < count; i++) {
        str << '#' << i << ' ';
        d->m_devices.at(i).format(str);
        str << '\n';
    }
    return rc;
}

int SymbianDeviceManager::findByPortName(const QString &p) const
{
    ensureInitialized();
    const int count = d->m_devices.size();
    for (int i = 0; i < count; i++)
        if (d->m_devices.at(i).portName() == p)
            return i;
    return -1;
}

QString SymbianDeviceManager::friendlyNameForPort(const QString &port) const
{
    const int idx = findByPortName(port);
    return idx == -1 ? QString() : d->m_devices.at(idx).friendlyName();
}

SymbianDeviceManager::TrkDevicePtr
        SymbianDeviceManager::acquireDevice(const QString &port)
{
    ensureInitialized();
    const int idx = findByPortName(port);
    if (idx == -1) {
        qWarning("Attempt to acquire device '%s' that does not exist.", qPrintable(port));
        if (debug)
            qDebug() << *this;
        return TrkDevicePtr();
      }
    const TrkDevicePtr rc = d->m_devices[idx].acquireDevice();
    if (debug)
        qDebug() << "SymbianDeviceManager::acquireDevice" << port << " returns " << !rc.isNull();
    return rc;
}

void SymbianDeviceManager::update()
{
    update(true);
}

void SymbianDeviceManager::releaseDevice(const QString &port)
{
    const int idx = findByPortName(port);
    if (debug)
        qDebug() << "SymbianDeviceManager::releaseDevice" << port << idx << sender();
    if (idx != -1) {
        d->m_devices[idx].releaseDevice();
    } else {
        qWarning("Attempt to release non-existing device %s.", qPrintable(port));
    }
}

void SymbianDeviceManager::setAdditionalInformation(const QString &port, const QString &ai)
{
    const int idx = findByPortName(port);
    if (idx != -1)
        d->m_devices[idx].setAdditionalInformation(ai);
}

void SymbianDeviceManager::ensureInitialized() const
{
    if (!d->m_initialized) // Flag is set in update()
        const_cast<SymbianDeviceManager*>(this)->update(false);
}

void SymbianDeviceManager::update(bool emitSignals)
{
    static int n = 0;
    typedef SymbianDeviceList::iterator SymbianDeviceListIterator;

    if (debug)
        qDebug(">SerialDeviceLister::update(#%d, signals=%d)\n%s", n++, int(emitSignals),
               qPrintable(toString()));

    d->m_initialized = true;
    // Get ordered new list
    SymbianDeviceList newDevices = serialPorts() + blueToothDevices();
    if (newDevices.size() > 1)
        qStableSort(newDevices.begin(), newDevices.end());
    if (d->m_devices == newDevices) { // Happy, nothing changed.
        if (debug)
            qDebug("<SerialDeviceLister::update: unchanged\n");
        return;
    }
    // Merge the lists and emit the respective added/removed signals, assuming
    // no one can plug a different device on the same port at the speed of lightning
    if (!d->m_devices.isEmpty()) {
        // Find deleted devices
        for (SymbianDeviceListIterator oldIt = d->m_devices.begin(); oldIt != d->m_devices.end(); ) {
            if (newDevices.contains(*oldIt)) {
                ++oldIt;
            } else {
                SymbianDevice toBeDeleted = *oldIt;
                toBeDeleted.forcedClose();
                oldIt = d->m_devices.erase(oldIt);
                if (emitSignals)
                    emit deviceRemoved(toBeDeleted);
            }
        }
    }
    if (!newDevices.isEmpty()) {
        // Find new devices and insert in order
        foreach(const SymbianDevice &newDevice, newDevices) {
            if (!d->m_devices.contains(newDevice)) {
                d->m_devices.append(newDevice);
                if (emitSignals)
                    emit deviceAdded(newDevice);
            }
        }
        if (d->m_devices.size() > 1)
            qStableSort(d->m_devices.begin(), d->m_devices.end());
    }
    if (emitSignals)
        emit updated();

    if (debug)
        qDebug("<SerialDeviceLister::update\n%s\n", qPrintable(toString()));
}

SymbianDeviceManager::SymbianDeviceList SymbianDeviceManager::serialPorts() const
{
    SymbianDeviceList rc;
#ifdef Q_OS_WIN
    const QSettings registry(REGKEY_CURRENT_CONTROL_SET, QSettings::NativeFormat);
    const QString usbSerialRootKey = QLatin1String(USBSER) + QLatin1Char('/');
    const int count = registry.value(usbSerialRootKey + QLatin1String("Count")).toInt();
    for (int i = 0; i < count; ++i) {
        QString driver = registry.value(usbSerialRootKey + QString::number(i)).toString();
        if (driver.contains(QLatin1String("JAVACOMM"))) {
            driver.replace(QLatin1Char('\\'), QLatin1Char('/'));
            const QString driverRootKey = QLatin1String("Enum/") + driver + QLatin1Char('/');
            if (debug > 1)
                qDebug() << "SerialDeviceLister::serialPorts(): Checking " << i << count
                         << REGKEY_CURRENT_CONTROL_SET << usbSerialRootKey << driverRootKey;
            QScopedPointer<SymbianDeviceData> device(new SymbianDeviceData);
            device->type = SerialPortCommunication;
            device->friendlyName = registry.value(driverRootKey + QLatin1String("FriendlyName")).toString();
            device->portName = registry.value(driverRootKey + QLatin1String("Device Parameters/PortName")).toString();
            device->deviceDesc = registry.value(driverRootKey + QLatin1String("DeviceDesc")).toString();
            device->manufacturer = registry.value(driverRootKey + QLatin1String("Mfg")).toString();
            rc.append(SymbianDevice(device.take()));
        }
    }
#endif
    return rc;
}

SymbianDeviceManager::SymbianDeviceList SymbianDeviceManager::blueToothDevices() const
{
    SymbianDeviceList rc;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // Bluetooth devices are created on connection. List the existing ones
    // or at least the first one.
    const QString prefix = QLatin1String(linuxBlueToothDeviceRootC);
    const QString blueToothfriendlyFormat = QLatin1String("Bluetooth device (%1)");
    for (int d = 0; d < 4; d++) {
        QScopedPointer<SymbianDeviceData> device(new SymbianDeviceData);
        device->type = BlueToothCommunication;
        device->portName = prefix + QString::number(d);
        if (d == 0 || QFileInfo(device->portName).exists()) {
            device->friendlyName = blueToothfriendlyFormat.arg(device->portName);
            rc.push_back(SymbianDevice(device.take()));
        }
    }
    // New kernel versions support /dev/ttyUSB0, /dev/ttyUSB1. Trk responds
    // on the latter (usually), try first.
    static const char *usbTtyDevices[] = { "/dev/ttyUSB1", "/dev/ttyUSB0" };
    const int usbTtyCount = sizeof(usbTtyDevices)/sizeof(const char *);
    for (int d = 0; d < usbTtyCount; d++) {
        const QString ttyUSBDevice = QLatin1String(usbTtyDevices[d]);
        if (QFileInfo(ttyUSBDevice).exists()) {
            SymbianDeviceData *device = new SymbianDeviceData;
            device->type = SerialPortCommunication;
            device->portName = ttyUSBDevice;
            device->friendlyName = QString::fromLatin1("USB/Serial device (%1)").arg(device->portName);
            rc.push_back(SymbianDevice(device));
        }
    }
#endif
    return rc;
}

Q_GLOBAL_STATIC(SymbianDeviceManager, symbianDeviceManager)

SymbianDeviceManager *SymbianDeviceManager::instance()
{
    return symbianDeviceManager();
}

SYMBIANUTILS_EXPORT QDebug operator<<(QDebug d, const SymbianDeviceManager &sdm)
{
    d.nospace() << sdm.toString();
    return d;
}

} // namespace SymbianUtilsInternal
