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

#include "iosdevice.h"
#include "iossimulator.h"
#include "iosconstants.h"
#include "iosconfigurations.h"
#include "iostoolhandler.h"
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitinformation.h>
#include <coreplugin/helpmanager.h>
#include <utils/portlist.h>

#include <QVariant>
#include <QVariantMap>
#include <QMessageBox>

#ifdef Q_OS_MAC
#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <CoreFoundation/CoreFoundation.h>

// Work around issue with not being able to retrieve USB serial number.
// See QTCREATORBUG-23460.
// For an unclear reason USBSpec.h in macOS SDK 10.15 uses a different value if
// MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_14, which just does not work.
#if MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_14
#undef kUSBSerialNumberString
#define kUSBSerialNumberString "USB Serial Number"
#endif

#endif

#include <exception>

using namespace ProjectExplorer;

namespace {
static Q_LOGGING_CATEGORY(detectLog, "qtc.ios.deviceDetect", QtWarningMsg)
}

#ifdef Q_OS_MAC
static QString CFStringRef2QString(CFStringRef s)
{
    unsigned char buf[250];
    CFIndex len = CFStringGetLength(s);
    CFIndex usedBufLen;
    CFIndex converted = CFStringGetBytes(s, CFRangeMake(0,len), kCFStringEncodingUTF8,
                                         '?', false, &buf[0], sizeof(buf), &usedBufLen);
    if (converted == len)
        return QString::fromUtf8(reinterpret_cast<char *>(&buf[0]), usedBufLen);
    size_t bufSize = sizeof(buf)
            + CFStringGetMaximumSizeForEncoding(len - converted, kCFStringEncodingUTF8);
    unsigned char *bigBuf = new unsigned char[bufSize];
    memcpy(bigBuf, buf, usedBufLen);
    CFIndex newUseBufLen;
    CFStringGetBytes(s, CFRangeMake(converted,len), kCFStringEncodingUTF8,
                     '?', false, &bigBuf[usedBufLen], bufSize, &newUseBufLen);
    QString res = QString::fromUtf8(reinterpret_cast<char *>(bigBuf), usedBufLen + newUseBufLen);
    delete[] bigBuf;
    return res;
}
#endif

namespace Ios {
namespace Internal {

IosDevice::IosDevice(CtorHelper)
    : m_lastPort(Constants::IOS_DEVICE_PORT_START)
{
    setType(Constants::IOS_DEVICE_TYPE);
    setDefaultDisplayName(IosDevice::name());
    setDisplayType(tr("iOS"));
    setMachineType(IDevice::Hardware);
    setOsType(Utils::OsTypeMac);
    setDeviceState(DeviceDisconnected);
}

IosDevice::IosDevice()
    : IosDevice(CtorHelper{})
{
    setupId(IDevice::AutoDetected, Constants::IOS_DEVICE_ID);

    Utils::PortList ports;
    ports.addRange(Utils::Port(Constants::IOS_DEVICE_PORT_START),
                   Utils::Port(Constants::IOS_DEVICE_PORT_END));
    setFreePorts(ports);
}

IosDevice::IosDevice(const QString &uid)
    : IosDevice(CtorHelper{})
{
    setupId(IDevice::AutoDetected, Utils::Id(Constants::IOS_DEVICE_ID).withSuffix(uid));
}

IDevice::DeviceInfo IosDevice::deviceInformation() const
{
    IDevice::DeviceInfo res;
    for (auto i = m_extraInfo.cbegin(), end = m_extraInfo.cend(); i != end; ++i) {
        IosDeviceManager::TranslationMap tMap = IosDeviceManager::translationMap();
        if (tMap.contains(i.key()))
            res.append(DeviceInfoItem(tMap.value(i.key()), tMap.value(i.value(), i.value())));
    }
    return res;
}

IDeviceWidget *IosDevice::createWidget()
{
    return nullptr;
}

DeviceProcessSignalOperation::Ptr IosDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr();
}

void IosDevice::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);

    m_extraInfo.clear();
    const QVariantMap vMap = map.value(QLatin1String(Constants::EXTRA_INFO_KEY)).toMap();
    for (auto i = vMap.cbegin(), end = vMap.cend(); i != end; ++i)
        m_extraInfo.insert(i.key(), i.value().toString());
}

QVariantMap IosDevice::toMap() const
{
    QVariantMap res = IDevice::toMap();
    QVariantMap vMap;
    for (auto i = m_extraInfo.cbegin(), end = m_extraInfo.cend(); i != end; ++i)
        vMap.insert(i.key(), i.value());
    res.insert(QLatin1String(Constants::EXTRA_INFO_KEY), vMap);
    return res;
}

QString IosDevice::uniqueDeviceID() const
{
    return id().suffixAfter(Utils::Id(Constants::IOS_DEVICE_ID));
}

QString IosDevice::name()
{
    return QCoreApplication::translate("Ios::Internal::IosDevice", "iOS Device");
}

QString IosDevice::osVersion() const
{
    return m_extraInfo.value(QLatin1String("osVersion"));
}

Utils::Port IosDevice::nextPort() const
{
    // use qrand instead?
    if (++m_lastPort >= Constants::IOS_DEVICE_PORT_END)
        m_lastPort = Constants::IOS_DEVICE_PORT_START;
    return Utils::Port(m_lastPort);
}

bool IosDevice::canAutoDetectPorts() const
{
    return true;
}


// IosDeviceManager

IosDeviceManager::TranslationMap IosDeviceManager::translationMap()
{
    static TranslationMap *translationMap = nullptr;
    if (translationMap)
        return *translationMap;
    TranslationMap &tMap = *new TranslationMap;
    tMap[QLatin1String("deviceName")]      = tr("Device name");
    //: Whether the device is in developer mode.
    tMap[QLatin1String("developerStatus")] = tr("Developer status");
    tMap[QLatin1String("deviceConnected")] = tr("Connected");
    tMap[QLatin1String("YES")]             = tr("yes");
    tMap[QLatin1String("NO")]              = tr("no");
    tMap[QLatin1String("YES")]             = tr("yes");
    tMap[QLatin1String("*unknown*")]       = tr("unknown");
    tMap[QLatin1String("osVersion")]       = tr("OS version");
    translationMap = &tMap;
    return tMap;
}

void IosDeviceManager::deviceConnected(const QString &uid, const QString &name)
{
    DeviceManager *devManager = DeviceManager::instance();
    Utils::Id baseDevId(Constants::IOS_DEVICE_ID);
    Utils::Id devType(Constants::IOS_DEVICE_TYPE);
    Utils::Id devId = baseDevId.withSuffix(uid);
    IDevice::ConstPtr dev = devManager->find(devId);
    if (dev.isNull()) {
        auto newDev = new IosDevice(uid);
        if (!name.isNull())
            newDev->setDisplayName(name);
        qCDebug(detectLog) << "adding ios device " << uid;
        devManager->addDevice(IDevice::ConstPtr(newDev));
    } else if (dev->deviceState() != IDevice::DeviceConnected &&
               dev->deviceState() != IDevice::DeviceReadyToUse) {
        qCDebug(detectLog) << "updating ios device " << uid;
        if (dev->type() == devType) // FIXME: Should that be a QTC_ASSERT?
            devManager->addDevice(dev->clone());
        else
            devManager->addDevice(IDevice::ConstPtr(new IosDevice(uid)));
    }
    updateInfo(uid);
}

void IosDeviceManager::deviceDisconnected(const QString &uid)
{
    qCDebug(detectLog) << "detected disconnection of ios device " << uid;
    DeviceManager *devManager = DeviceManager::instance();
    Utils::Id baseDevId(Constants::IOS_DEVICE_ID);
    Utils::Id devType(Constants::IOS_DEVICE_TYPE);
    Utils::Id devId = baseDevId.withSuffix(uid);
    IDevice::ConstPtr dev = devManager->find(devId);
    if (dev.isNull() || dev->type() != devType) {
        qCWarning(detectLog) << "ignoring disconnection of ios device " << uid; // should neve happen
    } else {
        auto iosDev = static_cast<const IosDevice *>(dev.data());
        if (iosDev->m_extraInfo.isEmpty()
                || iosDev->m_extraInfo.value(QLatin1String("deviceName")) == QLatin1String("*unknown*")) {
            devManager->removeDevice(iosDev->id());
        } else if (iosDev->deviceState() != IDevice::DeviceDisconnected) {
            qCDebug(detectLog) << "disconnecting device " << iosDev->uniqueDeviceID();
            devManager->setDeviceState(iosDev->id(), IDevice::DeviceDisconnected);
        }
    }
}

void IosDeviceManager::updateInfo(const QString &devId)
{
    IosToolHandler *requester = new IosToolHandler(IosDeviceType(IosDeviceType::IosDevice), this);
    connect(requester, &IosToolHandler::deviceInfo,
            this, &IosDeviceManager::deviceInfo, Qt::QueuedConnection);
    connect(requester, &IosToolHandler::finished,
            this, &IosDeviceManager::infoGathererFinished);
    requester->requestDeviceInfo(devId);
}

void IosDeviceManager::deviceInfo(IosToolHandler *, const QString &uid,
                                  const Ios::IosToolHandler::Dict &info)
{
    DeviceManager *devManager = DeviceManager::instance();
    Utils::Id baseDevId(Constants::IOS_DEVICE_ID);
    Utils::Id devType(Constants::IOS_DEVICE_TYPE);
    Utils::Id devId = baseDevId.withSuffix(uid);
    IDevice::ConstPtr dev = devManager->find(devId);
    bool skipUpdate = false;
    IosDevice *newDev = nullptr;
    if (!dev.isNull() && dev->type() == devType) {
        auto iosDev = static_cast<const IosDevice *>(dev.data());
        if (iosDev->m_extraInfo == info) {
            skipUpdate = true;
            newDev = const_cast<IosDevice *>(iosDev);
        } else {
            newDev = new IosDevice();
            newDev->fromMap(iosDev->toMap());
        }
    } else {
        newDev = new IosDevice(uid);
    }
    if (!skipUpdate) {
        QString devNameKey = QLatin1String("deviceName");
        if (info.contains(devNameKey))
            newDev->setDisplayName(info.value(devNameKey));
        newDev->m_extraInfo = info;
        qCDebug(detectLog) << "updated info of ios device " << uid;
        dev = IDevice::ConstPtr(newDev);
        devManager->addDevice(dev);
    }
    QLatin1String devStatusKey = QLatin1String("developerStatus");
    if (info.contains(devStatusKey)) {
        QString devStatus = info.value(devStatusKey);
        if (devStatus == QLatin1String("Development")) {
            devManager->setDeviceState(newDev->id(), IDevice::DeviceReadyToUse);
            m_userModeDeviceIds.removeOne(uid);
        } else {
            devManager->setDeviceState(newDev->id(), IDevice::DeviceConnected);
            bool shouldIgnore = newDev->m_ignoreDevice;
            newDev->m_ignoreDevice = true;
            if (devStatus == QLatin1String("*off*")) {
                if (!shouldIgnore && !IosConfigurations::ignoreAllDevices()) {
                    QMessageBox mBox;
                    mBox.setText(tr("An iOS device in user mode has been detected."));
                    mBox.setInformativeText(tr("Do you want to see how to set it up for development?"));
                    mBox.setStandardButtons(QMessageBox::NoAll | QMessageBox::No | QMessageBox::Yes);
                    mBox.setDefaultButton(QMessageBox::Yes);
                    int ret = mBox.exec();
                    switch (ret) {
                    case QMessageBox::Yes:
                        Core::HelpManager::showHelpUrl(
                                    QLatin1String("qthelp://org.qt-project.qtcreator/doc/creator-developing-ios.html"));
                        break;
                    case QMessageBox::No:
                        break;
                    case QMessageBox::NoAll:
                        IosConfigurations::setIgnoreAllDevices(true);
                        break;
                    default:
                        break;
                    }
                }
            }
            if (!m_userModeDeviceIds.contains(uid))
                m_userModeDeviceIds.append(uid);
            m_userModeDevicesTimer.start();
        }
    }
}

void IosDeviceManager::infoGathererFinished(IosToolHandler *gatherer)
{
    gatherer->deleteLater();
}

#ifdef Q_OS_MAC
namespace {
io_iterator_t gAddedIter;
io_iterator_t gRemovedIter;

extern "C" {
void deviceConnectedCallback(void *refCon, io_iterator_t iterator)
{
    try {
        kern_return_t       kr;
        io_service_t        usbDevice;
        (void) refCon;

        while ((usbDevice = IOIteratorNext(iterator))) {
            io_name_t       deviceName;

            // Get the USB device's name.
            kr = IORegistryEntryGetName(usbDevice, deviceName);
            QString name;
            if (KERN_SUCCESS == kr)
                name = QString::fromLocal8Bit(deviceName);
            qCDebug(detectLog) << "ios device " << name << " in deviceAddedCallback";

            CFStringRef cfUid = static_cast<CFStringRef>(IORegistryEntryCreateCFProperty(
                                                             usbDevice,
                                                             CFSTR(kUSBSerialNumberString),
                                                             kCFAllocatorDefault, 0));
            if (cfUid) {
                QString uid = CFStringRef2QString(cfUid);
                CFRelease(cfUid);
                qCDebug(detectLog) << "device UID is" << uid;
                IosDeviceManager::instance()->deviceConnected(uid, name);
            } else {
                qCDebug(detectLog) << "failed to retrieve device's UID";
            }

            // Done with this USB device; release the reference added by IOIteratorNext
            kr = IOObjectRelease(usbDevice);
        }
    }
    catch (const std::exception &e) {
        qCWarning(detectLog) << "Exception " << e.what() << " in iosdevice.cpp deviceConnectedCallback";
    }
    catch (...) {
        qCWarning(detectLog) << "Exception in iosdevice.cpp deviceConnectedCallback";
        throw;
    }
}

void deviceDisconnectedCallback(void *refCon, io_iterator_t iterator)
{
    try {
        kern_return_t       kr;
        io_service_t        usbDevice;
        (void) refCon;

        while ((usbDevice = IOIteratorNext(iterator))) {
            io_name_t       deviceName;

            // Get the USB device's name.
            kr = IORegistryEntryGetName(usbDevice, deviceName);
            QString name;
            if (KERN_SUCCESS == kr)
                name = QString::fromLocal8Bit(deviceName);
            qCDebug(detectLog) << "ios device " << name << " in deviceDisconnectedCallback";

            CFStringRef cfUid = static_cast<CFStringRef>(
                IORegistryEntryCreateCFProperty(usbDevice,
                                                CFSTR(kUSBSerialNumberString),
                                                kCFAllocatorDefault,
                                                0));
            if (cfUid) {
                QString uid = CFStringRef2QString(cfUid);
                CFRelease(cfUid);
                IosDeviceManager::instance()->deviceDisconnected(uid);
            } else {
                qCDebug(detectLog) << "failed to retrieve device's UID";
            }

            // Done with this USB device; release the reference added by IOIteratorNext
            kr = IOObjectRelease(usbDevice);
        }
    }
    catch (const std::exception &e) {
        qCWarning(detectLog) << "Exception " << e.what() << " in iosdevice.cpp deviceDisconnectedCallback";
    }
    catch (...) {
        qCWarning(detectLog) << "Exception in iosdevice.cpp deviceDisconnectedCallback";
        throw;
    }
}

} // extern C

} // anonymous namespace
#endif

void IosDeviceManager::monitorAvailableDevices()
{
#ifdef Q_OS_MAC
    CFMutableDictionaryRef  matchingDictionary =
                                        IOServiceMatching("IOUSBDevice" );
    {
        UInt32 vendorId = 0x05ac;
        CFNumberRef cfVendorValue = CFNumberCreate( kCFAllocatorDefault, kCFNumberSInt32Type,
                                                    &vendorId );
        CFDictionaryAddValue( matchingDictionary, CFSTR( kUSBVendorID ), cfVendorValue);
        CFRelease( cfVendorValue );
        UInt32 productId = 0x1280;
        CFNumberRef cfProductIdValue = CFNumberCreate( kCFAllocatorDefault, kCFNumberSInt32Type,
                                                       &productId );
        CFDictionaryAddValue( matchingDictionary, CFSTR( kUSBProductID ), cfProductIdValue);
        CFRelease( cfProductIdValue );
        UInt32 productIdMask = 0xFFC0;
        CFNumberRef cfProductIdMaskValue = CFNumberCreate( kCFAllocatorDefault, kCFNumberSInt32Type,
                                                           &productIdMask );
        CFDictionaryAddValue( matchingDictionary, CFSTR( kUSBProductIDMask ), cfProductIdMaskValue);
        CFRelease( cfProductIdMaskValue );
    }

    IONotificationPortRef notificationPort = IONotificationPortCreate(kIOMasterPortDefault);
    CFRunLoopSourceRef runLoopSource = IONotificationPortGetRunLoopSource(notificationPort);

    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);

    // IOServiceAddMatchingNotification releases this, so we retain for the next call
    CFRetain(matchingDictionary);

    // Now set up a notification to be called when a device is first matched by I/O Kit.
    kern_return_t kr;
    kr = IOServiceAddMatchingNotification(notificationPort,
                                          kIOMatchedNotification,
                                          matchingDictionary,
                                          deviceConnectedCallback,
                                          NULL,
                                          &gAddedIter);


    kr = IOServiceAddMatchingNotification(notificationPort,
                                          kIOTerminatedNotification,
                                          matchingDictionary,
                                          deviceDisconnectedCallback,
                                          NULL,
                                          &gRemovedIter);

    // Iterate once to get already-present devices and arm the notification
    deviceConnectedCallback(NULL, gAddedIter);
    deviceDisconnectedCallback(NULL, gRemovedIter);
#endif
}


IosDeviceManager::IosDeviceManager(QObject *parent) :
    QObject(parent)
{
    m_userModeDevicesTimer.setSingleShot(true);
    m_userModeDevicesTimer.setInterval(8000);
    connect(&m_userModeDevicesTimer, &QTimer::timeout,
            this, &IosDeviceManager::updateUserModeDevices);
}

void IosDeviceManager::updateUserModeDevices()
{
    foreach (const QString &uid, m_userModeDeviceIds)
        updateInfo(uid);
}

IosDeviceManager *IosDeviceManager::instance()
{
    static IosDeviceManager obj;
    return &obj;
}

void IosDeviceManager::updateAvailableDevices(const QStringList &devices)
{
    foreach (const QString &uid, devices)
        deviceConnected(uid);

    DeviceManager *devManager = DeviceManager::instance();
    for (int iDevice = 0; iDevice < devManager->deviceCount(); ++iDevice) {
        IDevice::ConstPtr dev = devManager->deviceAt(iDevice);
        Utils::Id devType(Constants::IOS_DEVICE_TYPE);
        if (dev.isNull() || dev->type() != devType)
            continue;
        auto iosDev = static_cast<const IosDevice *>(dev.data());
        if (devices.contains(iosDev->uniqueDeviceID()))
            continue;
        if (iosDev->deviceState() != IDevice::DeviceDisconnected) {
            qCDebug(detectLog) << "disconnecting device " << iosDev->uniqueDeviceID();
            devManager->setDeviceState(iosDev->id(), IDevice::DeviceDisconnected);
        }
    }
}

// Factory

IosDeviceFactory::IosDeviceFactory()
    : IDeviceFactory(Constants::IOS_DEVICE_TYPE)
{
    setDisplayName(IosDevice::name());
    setCombinedIcon(":/ios/images/iosdevicesmall.png",
                     ":/ios/images/iosdevice.png");
    setConstructionFunction([] { return IDevice::Ptr(new IosDevice); });
}

bool IosDeviceFactory::canRestore(const QVariantMap &map) const
{
    QVariantMap vMap = map.value(QLatin1String(Constants::EXTRA_INFO_KEY)).toMap();
    if (vMap.isEmpty()
            || vMap.value(QLatin1String("deviceName")).toString() == QLatin1String("*unknown*"))
        return false; // transient device (probably generated during an activation)
    return true;
}

} // namespace Internal
} // namespace Ios
