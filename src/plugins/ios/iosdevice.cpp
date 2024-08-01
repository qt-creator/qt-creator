// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iosdevice.h"

#include "devicectlutils.h"
#include "iosconfigurations.h"
#include "iosconstants.h"
#include "iossimulator.h"
#include "iostoolhandler.h"
#include "iostr.h"

#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevicefactory.h>
#include <projectexplorer/devicesupport/idevicewidget.h>
#include <projectexplorer/kitaspects.h>

#include <utils/layoutbuilder.h>
#include <utils/portlist.h>
#include <utils/qtcprocess.h>

#include <solutions/tasking/tasktree.h>

#include <QJsonArray>
#include <QJsonDocument>
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
using namespace Utils;

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

namespace Ios::Internal {

const char kHandler[] = "Handler";

class IosDeviceInfoWidget final : public IDeviceWidget
{
public:
    IosDeviceInfoWidget(const IDevice::Ptr &device)
        : IDeviceWidget(device)
    {
        const auto iosDevice = std::static_pointer_cast<IosDevice>(device);
        using namespace Layouting;
        // clang-format off
        Form {
            Tr::tr("Device name:"), iosDevice->deviceName(), br,
            Tr::tr("Identifier:"), iosDevice->uniqueInternalDeviceId(), br,
            Tr::tr("Product type:"), iosDevice->productType(), br,
            Tr::tr("CPU Architecture:"), iosDevice->cpuArchitecture(), br,
            Tr::tr("OS Version:"), iosDevice->osVersion(), br,
            noMargin
        }.attachTo(this);
        // clang-format on
    }

    void updateDeviceFromUi() final {}
};

IosDevice::IosDevice(CtorHelper)
    : m_lastPort(Constants::IOS_DEVICE_PORT_START)
{
    setType(Constants::IOS_DEVICE_TYPE);
    displayName.setDefaultValue(IosDevice::name());
    setDisplayType(Tr::tr("iOS"));
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
    return new IosDeviceInfoWidget(shared_from_this());
}

void IosDevice::fromMap(const Store &map)
{
    IDevice::fromMap(map);

    m_extraInfo.clear();
    const Store vMap = storeFromVariant(map.value(Constants::EXTRA_INFO_KEY));
    for (auto i = vMap.cbegin(), end = vMap.cend(); i != end; ++i)
        m_extraInfo.insert(stringFromKey(i.key()), i.value().toString());
    m_handler = Handler(map.value(kHandler).toInt());
}

void IosDevice::toMap(Store &map) const
{
    IDevice::toMap(map);

    Store vMap;
    for (auto i = m_extraInfo.cbegin(), end = m_extraInfo.cend(); i != end; ++i)
        vMap.insert(keyFromString(i.key()), i.value());
    map.insert(Constants::EXTRA_INFO_KEY, variantFromStore(vMap));
    map.insert(kHandler, int(m_handler));
}

QString IosDevice::deviceName() const
{
    return m_extraInfo.value(kDeviceName);
}

QString IosDevice::uniqueDeviceID() const
{
    return id().suffixAfter(Id(Constants::IOS_DEVICE_ID));
}

QString IosDevice::uniqueInternalDeviceId() const
{
    return m_extraInfo.value(kUniqueDeviceId);
}

QString IosDevice::name()
{
    return Tr::tr("iOS Device");
}

QString IosDevice::osVersion() const
{
    return m_extraInfo.value(kOsVersion);
}

QString IosDevice::productType() const
{
    return m_extraInfo.value(kProductType);
}

QString IosDevice::cpuArchitecture() const
{
    return m_extraInfo.value(kCpuArchitecture);
}

Utils::Port IosDevice::nextPort() const
{
    // use qrand instead?
    if (++m_lastPort >= Constants::IOS_DEVICE_PORT_END)
        m_lastPort = Constants::IOS_DEVICE_PORT_START;
    return Utils::Port(m_lastPort);
}

IosDevice::Handler IosDevice::handler() const
{
    return m_handler;
}

// IosDeviceManager

IosDeviceManager::TranslationMap IosDeviceManager::translationMap()
{
    static TranslationMap *translationMap = nullptr;
    if (translationMap)
        return *translationMap;
    TranslationMap &tMap = *new TranslationMap;
    tMap[kDeviceName] = Tr::tr("Device name");
    //: Whether the device is in developer mode.
    tMap[kDeveloperStatus]                 = Tr::tr("Developer status");
    tMap[kDeviceConnected]                 = Tr::tr("Connected");
    tMap[vYes]                             = Tr::tr("yes");
    tMap[QLatin1String("NO")]              = Tr::tr("no");
    tMap[QLatin1String("*unknown*")]       = Tr::tr("unknown");
    tMap[kOsVersion]                       = Tr::tr("OS version");
    tMap[kProductType] = Tr::tr("Product type");
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
    if (!dev) {
        auto newDev = new IosDevice(uid);
        if (!name.isNull())
            newDev->displayName.setValue(name);
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
    // if an update is currently still running for the device being connected, cancel that
    // erasing deletes the unique_ptr which deletes the TaskTree which stops it
    m_updateTasks.erase(uid);
    DeviceManager *devManager = DeviceManager::instance();
    Utils::Id baseDevId(Constants::IOS_DEVICE_ID);
    Utils::Id devType(Constants::IOS_DEVICE_TYPE);
    Utils::Id devId = baseDevId.withSuffix(uid);
    IDevice::ConstPtr dev = devManager->find(devId);
    if (!dev || dev->type() != devType) {
        qCWarning(detectLog) << "ignoring disconnection of ios device " << uid; // should neve happen
    } else {
        auto iosDev = static_cast<const IosDevice *>(dev.get());
        if (iosDev->m_extraInfo.isEmpty()
            || iosDev->m_extraInfo.value(kDeviceName) == QLatin1String("*unknown*")) {
            devManager->removeDevice(iosDev->id());
        } else if (iosDev->deviceState() != IDevice::DeviceDisconnected) {
            qCDebug(detectLog) << "disconnecting device " << iosDev->uniqueDeviceID();
            devManager->setDeviceState(iosDev->id(), IDevice::DeviceDisconnected);
        }
    }
}

void IosDeviceManager::updateInfo(const QString &devId)
{
    using namespace Tasking;

    const auto infoFromDeviceCtl = ProcessTask(
        [](Process &process) {
            process.setCommand({FilePath::fromString("/usr/bin/xcrun"),
                                {"devicectl", "list", "devices", "--quiet", "--json-output", "-"}});
        },
        [this, devId](const Process &process) {
            const expected_str<QMap<QString, QString>> result = parseDeviceInfo(process.rawStdOut(),
                                                                                devId);
            if (!result) {
                qCDebug(detectLog) << result.error();
                return DoneResult::Error;
            }
            deviceInfo(devId, IosDevice::Handler::DeviceCtl, *result);
            return DoneResult::Success;
        },
        CallDoneIf::Success);

    const auto infoFromIosTool = IosToolTask([this, devId](IosToolRunner &runner) {
        runner.setDeviceType(IosDeviceType::IosDevice);
        runner.setStartHandler([this, devId](IosToolHandler *handler) {
            connect(
                handler,
                &IosToolHandler::deviceInfo,
                this,
                [this](IosToolHandler *, const QString &uid, const Ios::IosToolHandler::Dict &info) {
                    deviceInfo(uid, IosDevice::Handler::IosTool, info);
                },
                Qt::QueuedConnection);
            handler->requestDeviceInfo(devId);
        });
    });

    const Group root{sequential, stopOnSuccess, infoFromDeviceCtl, infoFromIosTool};

    TaskTree *task = new TaskTree(root);
    m_updateTasks[devId].reset(task); // cancels any existing update, not calling done handlers
    connect(task, &TaskTree::done, this, [this, task, devId] {
        const auto taskIt = m_updateTasks.find(devId);
        QTC_ASSERT(taskIt != m_updateTasks.end(), return);
        QTC_ASSERT(taskIt->second.get() == task, return);
        taskIt->second.release()->deleteLater();
        m_updateTasks.erase(taskIt);
    });
    task->start();
}

void IosDeviceManager::deviceInfo(const QString &uid,
                                  IosDevice::Handler handler,
                                  const Ios::IosToolHandler::Dict &info)
{
    DeviceManager *devManager = DeviceManager::instance();
    Utils::Id baseDevId(Constants::IOS_DEVICE_ID);
    Utils::Id devType(Constants::IOS_DEVICE_TYPE);
    Utils::Id devId = baseDevId.withSuffix(uid);
    IDevice::ConstPtr dev = devManager->find(devId);
    bool skipUpdate = false;
    IosDevice *newDev = nullptr;
    if (dev && dev->type() == devType) {
        auto iosDev = static_cast<const IosDevice *>(dev.get());
        if (iosDev->m_handler == handler && iosDev->m_extraInfo == info) {
            skipUpdate = true;
            newDev = const_cast<IosDevice *>(iosDev);
        } else {
            Store store;
            iosDev->toMap(store);
            newDev = new IosDevice();
            newDev->fromMap(store);
        }
    } else {
        newDev = new IosDevice(uid);
    }
    if (!skipUpdate) {
        if (info.contains(kDeviceName))
            newDev->displayName.setValue(info.value(kDeviceName));
        newDev->m_extraInfo = info;
        newDev->m_handler = handler;
        qCDebug(detectLog) << "updated info of ios device " << uid;
        dev = IDevice::ConstPtr(newDev);
        devManager->addDevice(dev);
    }
    QLatin1String devStatusKey = QLatin1String("developerStatus");
    if (info.contains(devStatusKey)) {
        QString devStatus = info.value(devStatusKey);
        if (devStatus == vDevelopment) {
            devManager->setDeviceState(newDev->id(), IDevice::DeviceReadyToUse);
            m_userModeDeviceIds.removeOne(uid);
        } else {
            devManager->setDeviceState(newDev->id(), IDevice::DeviceConnected);
            bool shouldIgnore = newDev->m_ignoreDevice;
            newDev->m_ignoreDevice = true;
            if (devStatus == vOff) {
                if (!m_devModeDialog && !shouldIgnore && !IosConfigurations::ignoreAllDevices()) {
                    m_devModeDialog = new QMessageBox(Core::ICore::dialogParent());
                    m_devModeDialog->setText(
                        Tr::tr("An iOS device in user mode has been detected."));
                    m_devModeDialog->setInformativeText(
                        Tr::tr("Do you want to see how to set it up for development?"));
                    m_devModeDialog->setStandardButtons(QMessageBox::NoAll | QMessageBox::No
                                                        | QMessageBox::Yes);
                    m_devModeDialog->setDefaultButton(QMessageBox::Yes);
                    m_devModeDialog->setAttribute(Qt::WA_DeleteOnClose);
                    connect(m_devModeDialog, &QDialog::finished, this, [](int result) {
                        switch (result) {
                        case QMessageBox::Yes:
                            Core::HelpManager::showHelpUrl(
                                QLatin1String("qthelp://org.qt-project.qtcreator/doc/"
                                              "creator-developing-ios.html"));
                            break;
                        case QMessageBox::No:
                            break;
                        case QMessageBox::NoAll:
                            IosConfigurations::setIgnoreAllDevices(true);
                            break;
                        default:
                            break;
                        }
                    });
                    m_devModeDialog->show();
                }
            }
            if (!m_userModeDeviceIds.contains(uid))
                m_userModeDeviceIds.append(uid);
            m_userModeDevicesTimer.start();
        }
    }
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

#if QT_MACOS_DEPLOYMENT_TARGET_BELOW(120000)
    const mach_port_t port = kIOMasterPortDefault; // deprecated in macOS 12
#else
    const mach_port_t port = kIOMainPortDefault; // available since macOS 12
#endif
    IONotificationPortRef notificationPort = IONotificationPortCreate(port);
    CFRunLoopSourceRef runLoopSource = IONotificationPortGetRunLoopSource(notificationPort);

    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);

    // IOServiceAddMatchingNotification releases this, so we retain for the next call
    CFRetain(matchingDictionary);

    // Now set up a notification to be called when a device is first matched by I/O Kit.
    IOServiceAddMatchingNotification(notificationPort,
                                     kIOMatchedNotification,
                                     matchingDictionary,
                                     deviceConnectedCallback,
                                     NULL,
                                     &gAddedIter);

    IOServiceAddMatchingNotification(notificationPort,
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
    for (const QString &uid : std::as_const(m_userModeDeviceIds))
        updateInfo(uid);
}

IosDeviceManager *IosDeviceManager::instance()
{
    static IosDeviceManager obj;
    return &obj;
}

void IosDeviceManager::updateAvailableDevices(const QStringList &devices)
{
    for (const QString &uid : devices)
        deviceConnected(uid);

    DeviceManager *devManager = DeviceManager::instance();
    for (int iDevice = 0; iDevice < devManager->deviceCount(); ++iDevice) {
        IDevice::ConstPtr dev = devManager->deviceAt(iDevice);
        Utils::Id devType(Constants::IOS_DEVICE_TYPE);
        if (!dev || dev->type() != devType)
            continue;
        auto iosDev = static_cast<const IosDevice *>(dev.get());
        if (devices.contains(iosDev->uniqueDeviceID()))
            continue;
        if (iosDev->deviceState() != IDevice::DeviceDisconnected) {
            qCDebug(detectLog) << "disconnecting device " << iosDev->uniqueDeviceID();
            devManager->setDeviceState(iosDev->id(), IDevice::DeviceDisconnected);
        }
    }
}

// Factory

class IosDeviceFactory final : public IDeviceFactory
{
public:
    IosDeviceFactory()
        : IDeviceFactory(Constants::IOS_DEVICE_TYPE)
    {
        setDisplayName(IosDevice::name());
        setCombinedIcon(":/ios/images/iosdevicesmall.png",
                        ":/ios/images/iosdevice.png");
        setConstructionFunction([] { return IDevice::Ptr(new IosDevice); });
    }

    bool canRestore(const Utils::Store &map) const override
    {
        Store vMap = map.value(Constants::EXTRA_INFO_KEY).value<Store>();
        if (vMap.isEmpty() || vMap.value(kDeviceName).toString() == QLatin1String("*unknown*"))
            return false; // transient device (probably generated during an activation)
        return true;
    }
};

void setupIosDevice()
{
    static IosDeviceFactory theIosDeviceFactory;
}

} // Ios::Internal
