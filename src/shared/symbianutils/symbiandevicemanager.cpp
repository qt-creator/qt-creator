/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "symbiandevicemanager.h"
#include "trkdevice.h"
#include "codadevice.h"
#include "virtualserialdevice.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtCore/QFileInfo>
#include <QtCore/QtDebug>
#include <QtCore/QTextStream>
#include <QtCore/QSharedData>
#include <QtCore/QScopedPointer>
#include <QtCore/QSignalMapper>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>
#include <QtCore/QTimer>

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

    bool isOpen() const;
    void forcedClose();

    QString portName;
    QString friendlyName;
    QString deviceDesc;
    QString manufacturer;
    QString additionalInformation;

    DeviceCommunicationType type;
    QSharedPointer<trk::TrkDevice> device;
    QSharedPointer<Coda::CodaDevice> codaDevice;
    int deviceAcquired;
};

SymbianDeviceData::SymbianDeviceData() :
        type(SerialPortCommunication),
        deviceAcquired(0)
{
}

bool SymbianDeviceData::isOpen() const
{
    if (device)
        return device->isOpen();
    if (codaDevice)
        return codaDevice->device()->isOpen();
    return false;
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
        if (device)
            device->close();
        else
            codaDevice->device()->close();
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
    m_data->deviceAcquired = 1;
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
        m_data->deviceAcquired = 0;
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
    SymbianDeviceManagerPrivate() : m_initialized(false) /*, m_destroyReleaseMapper(0),*/ {}

    bool m_initialized;
    SymbianDeviceManager::SymbianDeviceList m_devices;
    //QSignalMapper *m_destroyReleaseMapper;
    // The following 2 variables are needed to manage requests for a TCF port not coming from the main thread
    int m_constructTcfPortEventType;
    QMutex m_tcfPortWaitMutex;
};

class QConstructTcfPortEvent : public QEvent
{
public:
    QConstructTcfPortEvent(QEvent::Type eventId, const QString &portName, CodaDevicePtr *device, QWaitCondition *waiter) :
        QEvent(eventId), m_portName(portName), m_device(device), m_waiter(waiter)
       {}

    QString m_portName;
    CodaDevicePtr* m_device;
    QWaitCondition *m_waiter;
};


SymbianDeviceManager::SymbianDeviceManager(QObject *parent) :
    QObject(parent),
    d(new SymbianDeviceManagerPrivate)
{
    d->m_constructTcfPortEventType = QEvent::registerEventType();
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

CodaDevicePtr SymbianDeviceManager::getCodaDevice(const QString &port)
{
    ensureInitialized();
    const int idx = findByPortName(port);
    if (idx == -1) {
        qWarning("Attempt to acquire device '%s' that does not exist.", qPrintable(port));
        if (debug)
            qDebug() << *this;
        return CodaDevicePtr();
    }
    SymbianDevice& device = d->m_devices[idx];
    if (device.m_data->device && device.m_data->device.data()->isOpen()) {
        qWarning("Attempting to open a port '%s' that is configured for TRK!", qPrintable(port));
        return CodaDevicePtr();
    }
    CodaDevicePtr& devicePtr = device.m_data->codaDevice;
    if (devicePtr.isNull() || !devicePtr->device()->isOpen()) {
        // Check we instanciate in the correct thread - we can't afford to create the CodaDevice (and more specifically, open the VirtualSerialDevice) in a thread that isn't guaranteed to be long-lived.
        // Therefore, if we're not in SymbianDeviceManager's thread, rejig things so it's opened in the main thread
        if (QThread::currentThread() != thread()) {
            // SymbianDeviceManager is owned by the current thread
            d->m_tcfPortWaitMutex.lock();
            QWaitCondition waiter;
            QCoreApplication::postEvent(this, new QConstructTcfPortEvent((QEvent::Type)d->m_constructTcfPortEventType, port, &devicePtr, &waiter));
            waiter.wait(&d->m_tcfPortWaitMutex);
            // When the wait returns (due to the wakeAll in SymbianDeviceManager::customEvent), the CodaDevice will be fully set up
            d->m_tcfPortWaitMutex.unlock();
        } else {
            // We're in the main thread, just set it up directly
            constructTcfPort(devicePtr, port);
        }
    // We still carry on in the case we failed to open so the client can access the IODevice's errorString()
    }
    if (devicePtr->device()->isOpen())
        device.m_data->deviceAcquired++;
    return devicePtr;
}

void SymbianDeviceManager::constructTcfPort(CodaDevicePtr& device, const QString& portName)
{
    QMutexLocker locker(&d->m_tcfPortWaitMutex);
    if (device.isNull()) {
        device = QSharedPointer<Coda::CodaDevice>(new Coda::CodaDevice);
        const QSharedPointer<SymbianUtils::VirtualSerialDevice> serialDevice(new SymbianUtils::VirtualSerialDevice(portName));
        device->setSerialFrame(true);
        device->setDevice(serialDevice);
    }
    if (!device->device()->isOpen()) {
        bool ok = device->device().staticCast<SymbianUtils::VirtualSerialDevice>()->open(QIODevice::ReadWrite);
        if (!ok && debug) {
            qDebug("SymbianDeviceManager: Failed to open port %s", qPrintable(portName));
        }
    }
}

void SymbianDeviceManager::customEvent(QEvent *event)
{
    if (event->type() == d->m_constructTcfPortEventType) {
        QConstructTcfPortEvent* constructEvent = static_cast<QConstructTcfPortEvent*>(event);
        constructTcfPort(*constructEvent->m_device, constructEvent->m_portName);
        constructEvent->m_waiter->wakeAll(); // Should only ever be one thing waiting on this
    }
}

void SymbianDeviceManager::releaseCodaDevice(CodaDevicePtr &port)
{
    if (port) {
        // Check if this was the last reference to the port, if so close it after a short delay
        foreach (const SymbianDevice& device, d->m_devices) {
            if (device.m_data->codaDevice.data() == port.data()) {
                if (device.m_data->deviceAcquired > 0)
                    device.m_data->deviceAcquired--;
                if (device.m_data->deviceAcquired == 0) {
                    if (debug)
                        qDebug("Starting timer to close port %s", qPrintable(device.m_data->portName));
                    QTimer::singleShot(1000, this, SLOT(delayedClosePort()));
                }
                break;
            }
        }
        port.clear();
    }
}

void SymbianDeviceManager::delayedClosePort()
{
    // Find any coda ports that are still open but have a reference count of zero, and delete them
    foreach (const SymbianDevice& device, d->m_devices) {
        Coda::CodaDevice* codaDevice = device.m_data->codaDevice.data();
        if (codaDevice && device.m_data->deviceAcquired == 0 && codaDevice->device()->isOpen()) {
            if (debug)
                qDebug("Closing device %s", qPrintable(device.m_data->portName));
            device.m_data->codaDevice->device()->close();
        }
    }
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
    if (idx != -1)
        d->m_devices[idx].releaseDevice();
    else
        qWarning("Attempt to release non-existing device %s.", qPrintable(port));
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
    static const char *usbTtyDevices[] = {
        "/dev/ttyUSB3", "/dev/ttyUSB2", "/dev/ttyUSB1", "/dev/ttyUSB0",
        "/dev/ttyACM3", "/dev/ttyACM2", "/dev/ttyACM1", "/dev/ttyACM0"};
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

OstChannel *SymbianDeviceManager::getOstChannel(const QString &port, uchar channelId)
{
    CodaDevicePtr coda = getCodaDevice(port);
    if (coda.isNull() || !coda->device()->isOpen())
        return 0;
    return new OstChannel(coda, channelId);
}

struct OstChannelPrivate
{
    CodaDevicePtr m_codaPtr;
    QByteArray m_dataBuffer;
    uchar m_channelId;
    bool m_hasReceivedData;
};

OstChannel::OstChannel(const CodaDevicePtr &codaPtr, uchar channelId)
    : d(new OstChannelPrivate)
{
    d->m_codaPtr = codaPtr;
    d->m_channelId = channelId;
    d->m_hasReceivedData = false;
    connect(codaPtr.data(), SIGNAL(unknownEvent(uchar, QByteArray)), this, SLOT(ostDataReceived(uchar,QByteArray)));
    connect(codaPtr->device().data(), SIGNAL(aboutToClose()), this, SLOT(deviceAboutToClose()));
    QIODevice::open(ReadWrite|Unbuffered);
}

void OstChannel::close()
{
    QIODevice::close();
    if (d && d->m_codaPtr.data()) {
        disconnect(d->m_codaPtr.data(), 0, this, 0);
        SymbianDeviceManager::instance()->releaseCodaDevice(d->m_codaPtr);
    }
}

OstChannel::~OstChannel()
{
    close();
    delete d;
}

void OstChannel::flush()
{
    //TODO d->m_codaPtr->device()-
}

qint64 OstChannel::bytesAvailable() const
{
    return d->m_dataBuffer.size();
}

bool OstChannel::isSequential() const
{
    return true;
}

qint64 OstChannel::readData(char *data, qint64 maxSize)
{
    qint64 amount = qMin(maxSize, (qint64)d->m_dataBuffer.size());
    qMemCopy(data, d->m_dataBuffer.constData(), amount);
    d->m_dataBuffer.remove(0, amount);
    return amount;
}

qint64 OstChannel::writeData(const char *data, qint64 maxSize)
{
    static const qint64 KMaxOstPayload = 1022;
    // If necessary, split the packet up
    while (maxSize) {
        QByteArray dataBuf = QByteArray::fromRawData(data, qMin(KMaxOstPayload, maxSize));
        d->m_codaPtr->writeCustomData(d->m_channelId, dataBuf);
        data += dataBuf.length();
        maxSize -= dataBuf.length();
    }
    return maxSize;
}

void OstChannel::ostDataReceived(uchar channelId, const QByteArray &aData)
{
    if (channelId == d->m_channelId) {
        d->m_hasReceivedData = true;
        d->m_dataBuffer.append(aData);
        emit readyRead();
    }
}

Coda::CodaDevice& OstChannel::codaDevice() const
{
    return *d->m_codaPtr;
}

bool OstChannel::hasReceivedData() const
{
    return isOpen() && d->m_hasReceivedData;
}

void OstChannel::deviceAboutToClose()
{
    close();
}

} // namespace SymbianUtils
