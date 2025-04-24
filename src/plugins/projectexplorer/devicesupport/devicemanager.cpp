// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicemanager.h"

#include "idevicefactory.h"
#include "../projectexplorertr.h"
#include "../projectexplorerconstants.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/fsengine/fsengine.h>
#include <utils/persistentsettings.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/terminalhooks.h>

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QVariantList>

#include <memory>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

const char DeviceManagerKey[] = "DeviceManager";
const char DeviceListKey[] = "DeviceList";
const char DefaultDevicesKey[] = "DefaultDevices";

class DeviceManagerPrivate
{
public:
    DeviceManagerPrivate() = default;

    int indexForId(Id id) const
    {
        for (int i = 0; i < devices.count(); ++i) {
            if (devices.at(i)->id() == id)
                return i;
        }
        return -1;
    }

    QList<IDevice::Ptr> deviceList() const
    {
        QMutexLocker locker(&mutex);
        return devices;
    }

    mutable QMutex mutex;
    QList<IDevice::Ptr> devices;
    QHash<Id, Id> defaultDevices;
    PersistentSettingsWriter *writer = nullptr;
};

} // namespace Internal

using namespace Internal;

static DeviceManager *m_instance = nullptr;
static std::unique_ptr<DeviceManagerPrivate> d;

DeviceManager *DeviceManager::instance()
{
    return m_instance;
}

int DeviceManager::deviceCount()
{
    return d->devices.count();
}

void DeviceManager::save()
{
    Store data;
    data.insert(DeviceManagerKey, variantFromStore(toMap()));
    d->writer->save(data);
}

static FilePath settingsFilePath(const QString &extension)
{
    return Core::ICore::userResourcePath(extension);
}

static FilePath systemSettingsFilePath(const QString &deviceFileRelativePath)
{
    return Core::ICore::installerResourcePath(deviceFileRelativePath);
}

void DeviceManager::load()
{
    QTC_ASSERT(!d->writer, return);

    // Only create writer now: We do not want to save before the settings were read!
    d->writer = new PersistentSettingsWriter(settingsFilePath("devices.xml"), "QtCreatorDevices");

    PersistentSettingsReader reader;
    // read devices file from global settings path
    QHash<Id, Id> defaultDevices;
    QList<IDevice::Ptr> sdkDevices;
    if (reader.load(systemSettingsFilePath("devices.xml")))
        sdkDevices = fromMap(storeFromVariant(reader.restoreValues().value(DeviceManagerKey)), &defaultDevices);
    // read devices file from user settings path
    QList<IDevice::Ptr> userDevices;
    if (reader.load(settingsFilePath("devices.xml")))
        userDevices = fromMap(storeFromVariant(reader.restoreValues().value(DeviceManagerKey)), &defaultDevices);
    // Insert devices into the model. Prefer the higher device version when there are multiple
    // devices with the same id.
    for (IDevice::Ptr device : std::as_const(userDevices)) {
        for (const IDevice::Ptr &sdkDevice : std::as_const(sdkDevices)) {
            if (device->id() == sdkDevice->id() || device->rootPath() == sdkDevice->rootPath()) {
                if (device->version() < sdkDevice->version())
                    device = sdkDevice;
                sdkDevices.removeOne(sdkDevice);
                break;
            }
        }
        addDevice(device);
    }
    // Append the new SDK devices to the model.
    for (const IDevice::Ptr &sdkDevice : std::as_const(sdkDevices))
        addDevice(sdkDevice);

    // Overwrite with the saved default devices.
    for (auto itr = defaultDevices.constBegin(); itr != defaultDevices.constEnd(); ++itr) {
        IDevice::ConstPtr device = find(itr.value());
        if (device)
            d->defaultDevices[device->type()] = device->id();
    }

    emit m_instance->devicesLoaded();
}

static const IDeviceFactory *restoreFactory(const Store &map)
{
    const Id deviceType = IDevice::typeFromMap(map);
    IDeviceFactory *factory = Utils::findOrDefault(IDeviceFactory::allDeviceFactories(),
        [&map, deviceType](IDeviceFactory *factory) {
            return factory->canRestore(map) && factory->deviceType() == deviceType;
        });

    if (!factory)
        qWarning("Warning: No factory found for device '%s' of type '%s'.",
                 qPrintable(IDevice::idFromMap(map).toString()),
                 qPrintable(IDevice::typeFromMap(map).toString()));
    return factory;
}

QList<IDevice::Ptr> DeviceManager::fromMap(const Store &map, QHash<Id, Id> *defaultDevices)
{
    QList<IDevice::Ptr> devices;

    if (defaultDevices) {
        const Store defaultDevsMap = storeFromVariant(map.value(DefaultDevicesKey));
        for (auto it = defaultDevsMap.constBegin(); it != defaultDevsMap.constEnd(); ++it)
            defaultDevices->insert(Id::fromString(stringFromKey(it.key())), Id::fromSetting(it.value()));
    }
    const QVariantList deviceList = map.value(DeviceListKey).toList();
    for (const QVariant &v : deviceList) {
        const Store map = storeFromVariant(v);
        const IDeviceFactory * const factory = restoreFactory(map);
        if (!factory)
            continue;
        const IDevice::Ptr device = factory->construct();
        QTC_ASSERT(device, continue);
        device->fromMap(map);
        devices << device;
    }
    return devices;
}

Store DeviceManager::toMap()
{
    Store map;
    Store defaultDeviceMap;
    for (auto it = d->defaultDevices.constBegin(); it != d->defaultDevices.constEnd(); ++it)
        defaultDeviceMap.insert(keyFromString(it.key().toString()), it.value().toSetting());

    map.insert(DefaultDevicesKey, variantFromStore(defaultDeviceMap));
    QVariantList deviceList;
    for (const IDevice::Ptr &device : std::as_const(d->devices)) {
        Store store;
        device->toMap(store);
        deviceList << variantFromStore(store);
    }
    map.insert(DeviceListKey, deviceList);
    return map;
}

void DeviceManager::addDevice(const IDevice::Ptr &device)
{
    QStringList names;
    for (const IDevice::Ptr &tmp : std::as_const(d->devices)) {
        if (tmp->id() != device->id())
            names << tmp->displayName();
    }

    // TODO: make it thread safe?
    device->setDisplayName(Utils::makeUniquelyNumbered(device->displayName(), names));

    const int pos = d->indexForId(device->id());

    if (!defaultDevice(device->type()))
        d->defaultDevices.insert(device->type(), device->id());

    if (pos >= 0) {
        {
            QMutexLocker locker(&d->mutex);
            d->devices[pos] = device;
        }
        emit m_instance->deviceUpdated(device->id());
    } else {
        {
            QMutexLocker locker(&d->mutex);
            d->devices << device;
        }
        emit m_instance->deviceAdded(device->id());

        if (FSEngine::isAvailable())
            FSEngine::addDevice(device->rootPath());
    }

    emit m_instance->updated();
}

void DeviceManager::removeDevice(Id id)
{
    const IDevice::Ptr device = mutableDevice(id);
    QTC_ASSERT(device, return);

    const bool wasDefault = d->defaultDevices.value(device->type()) == device->id();
    const Id deviceType = device->type();
    {
        QMutexLocker locker(&d->mutex);
        d->devices.removeAt(d->indexForId(id));
    }
    emit m_instance->deviceRemoved(device->id());

    if (FSEngine::isAvailable())
        FSEngine::removeDevice(device->rootPath());

    if (wasDefault) {
        for (int i = 0; i < d->devices.count(); ++i) {
            if (deviceAt(i)->type() == deviceType) {
                d->defaultDevices.insert(deviceAt(i)->type(), deviceAt(i)->id());
                emit m_instance->deviceUpdated(deviceAt(i)->id());
                break;
            }
        }
    }

    emit m_instance->updated();
}

void DeviceManager::setDeviceState(Id deviceId, IDevice::DeviceState deviceState)
{
    const int pos = d->indexForId(deviceId);
    if (pos < 0)
        return;
    IDevice::Ptr &device = d->devices[pos];
    if (device->deviceState() == deviceState)
        return;

    // TODO: make it thread safe?
    device->setDeviceState(deviceState);
    emit m_instance->deviceUpdated(deviceId);
    emit m_instance->updated();
}

bool DeviceManager::isLoaded()
{
    return d->writer;
}

// Thread safe
IDevice::ConstPtr DeviceManager::deviceForPath(const FilePath &path)
{
    const QList<IDevice::Ptr> devices = d->deviceList();

    if (path.scheme() == u"device") {
        for (const IDevice::Ptr &dev : devices) {
            if (path.host() == dev->id().toString())
                return dev;
        }
        return {};
    }

    for (const IDevice::Ptr &dev : devices) {
        // TODO: ensure handlesFile is thread safe
        if (dev->handlesFile(path))
            return dev;
    }
    return {};
}

IDevice::ConstPtr DeviceManager::defaultDesktopDevice()
{
    return m_instance->defaultDevice(Constants::DESKTOP_DEVICE_TYPE);
}

void DeviceManager::setDefaultDevice(Id id)
{
    const IDevice::ConstPtr &device = find(id);
    QTC_ASSERT(device, return);
    const IDevice::ConstPtr &oldDefaultDevice = defaultDevice(device->type());
    if (device == oldDefaultDevice)
        return;
    d->defaultDevices.insert(device->type(), device->id());
    emit m_instance->deviceUpdated(device->id());
    emit m_instance->deviceUpdated(oldDefaultDevice->id());

    emit m_instance->updated();
}

DeviceManager::DeviceManager()
{
    d = std::make_unique<DeviceManagerPrivate>();
    m_instance = this;
    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &DeviceManager::save);

    DeviceFileHooks deviceHooks;

    deviceHooks.isSameDevice = [](const FilePath &left, const FilePath &right) {
        auto leftDevice = DeviceManager::deviceForPath(left);
        auto rightDevice = DeviceManager::deviceForPath(right);

        return leftDevice == rightDevice;
    };

    deviceHooks.localSource = [](const FilePath &file) -> Result<FilePath> {
        auto device = DeviceManager::deviceForPath(file);
        if (!device)
            return make_unexpected(Tr::tr("No device for path \"%1\"").arg(file.toUserOutput()));
        return device->localSource(file);
    };

    deviceHooks.fileAccess = [](const FilePath &filePath) -> Result<DeviceFileAccess *> {
        if (filePath.isLocal())
            return DesktopDeviceFileAccess::instance();
        IDevice::ConstPtr device = DeviceManager::deviceForPath(filePath);
        if (!device) {
            return make_unexpected(
                Tr::tr("No device found for path \"%1\"").arg(filePath.toUserOutput()));
        }
        DeviceFileAccess *fileAccess = device->fileAccess();
        if (!fileAccess) {
            return make_unexpected(
                Tr::tr("No file access for device \"%1\"").arg(device->displayName()));
        }
        return fileAccess;
    };

    deviceHooks.environment = [](const FilePath &filePath) -> Result<Environment> {
        auto device = DeviceManager::deviceForPath(filePath);
        if (!device) {
            return make_unexpected(
                Tr::tr("No device found for path \"%1\"").arg(filePath.toUserOutput()));
        }
        return device->systemEnvironmentWithError();
    };

    deviceHooks.deviceDisplayName = [](const FilePath &filePath) {
        auto device = DeviceManager::deviceForPath(filePath);
        if (device)
            return device->displayName();
        return filePath.host().toString();
    };

    deviceHooks.ensureReachable = [](const FilePath &filePath, const FilePath &other) {
        auto device = DeviceManager::deviceForPath(filePath);
        QTC_ASSERT(device, return false);
        return device->ensureReachable(other);
    };

    deviceHooks.openTerminal = [](const FilePath &filePath, const Environment &env) {
        auto device = DeviceManager::deviceForPath(filePath);
        QTC_ASSERT(device, return);
        device->openTerminal(env, filePath);
    };

    deviceHooks.osType = [](const FilePath &filePath) {
        auto device = DeviceManager::deviceForPath(filePath);
        if (!device)
            return OsTypeLinux;
        return device->osType();
    };

    DeviceFileHooks::setupDeviceFileHooks(deviceHooks);

    DeviceProcessHooks processHooks;

    processHooks.processImplHook = [](const FilePath &filePath) -> ProcessInterface * {
        auto device = DeviceManager::deviceForPath(filePath);
        QTC_ASSERT(device, return nullptr);
        return device->createProcessInterface();
    };

    Process::setRemoteProcessHooks(processHooks);
}

DeviceManager::~DeviceManager()
{
    delete d->writer;
    m_instance = nullptr;
}

IDevice::Ptr DeviceManager::deviceAt(int idx)
{
    QTC_ASSERT(idx >= 0 && idx < deviceCount(), return IDevice::Ptr());
    return d->devices.at(idx);
}

void DeviceManager::forEachDevice(const std::function<void(const IDeviceConstPtr &)> &func)
{
    const QList<IDevice::Ptr> devices = d->deviceList();

    for (const IDevice::Ptr &device : devices)
        func(device);
}

IDevice::Ptr DeviceManager::mutableDevice(Id id)
{
    const int index = d->indexForId(id);
    return index == -1 ? IDevice::Ptr() : d->devices.at(index);
}

bool DeviceManager::hasDevice(const QString &name)
{
    return Utils::anyOf(d->devices, [&name](const IDevice::Ptr &device) {
        return device->displayName() == name;
    });
}

IDevice::Ptr DeviceManager::find(Id id)
{
    const int index = d->indexForId(id);
    return index == -1 ? IDevice::Ptr() : deviceAt(index);
}

IDevice::Ptr DeviceManager::defaultDevice(Id deviceType)
{
    const Id id = d->defaultDevices.value(deviceType);
    return id.isValid() ? find(id) : IDevice::Ptr();
}

} // namespace ProjectExplorer


#ifdef WITH_TESTS
#include <projectexplorer/projectexplorer_test.h>
#include <QSignalSpy>
#include <QTest>

namespace ProjectExplorer {

class TestDevice : public IDevice
{
public:
    TestDevice()
    {
        setupId(AutoDetected, Id::generate());
        setType(testTypeId());
        setMachineType(Hardware);
        setOsType(HostOsInfo::hostOs());
        setDisplayType("blubb");
    }

    static Id testTypeId() { return "TestType"; }

private:
    IDeviceWidget *createWidget() override { return nullptr; }
};

class TestDeviceFactory final : public IDeviceFactory
{
public:
    TestDeviceFactory() : IDeviceFactory(TestDevice::testTypeId())
    {
        setConstructionFunction([] { return IDevice::Ptr(new TestDevice); });
    }
};

void ProjectExplorerTest::testDeviceManager()
{
    TestDeviceFactory factory;

    TestDevice::Ptr dev = IDevice::Ptr(new TestDevice);
    dev->setDisplayName(QLatin1String("blubbdiblubbfurz!"));
    QVERIFY(dev->isAutoDetected());
    QCOMPARE(dev->deviceState(), IDevice::DeviceStateUnknown);
    QCOMPARE(dev->type(), TestDevice::testTypeId());

    QVERIFY(!DeviceManager::find(dev->id()));
    const int oldDeviceCount = DeviceManager::deviceCount();

    DeviceManager * const mgr = DeviceManager::instance();
    QSignalSpy deviceAddedSpy(mgr, &DeviceManager::deviceAdded);
    QSignalSpy deviceRemovedSpy(mgr, &DeviceManager::deviceRemoved);
    QSignalSpy deviceUpdatedSpy(mgr, &DeviceManager::deviceUpdated);
    QSignalSpy updatedSpy(mgr, &DeviceManager::updated);

    DeviceManager::addDevice(dev);
    QCOMPARE(DeviceManager::deviceCount(), oldDeviceCount + 1);
    QVERIFY(DeviceManager::find(dev->id()));
    QVERIFY(DeviceManager::hasDevice(dev->displayName()));
    QCOMPARE(deviceAddedSpy.count(), 1);
    QCOMPARE(deviceRemovedSpy.count(), 0);
    QCOMPARE(deviceUpdatedSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 1);
    deviceAddedSpy.clear();
    updatedSpy.clear();

    DeviceManager::setDeviceState(dev->id(), IDevice::DeviceStateUnknown);
    QCOMPARE(deviceAddedSpy.count(), 0);
    QCOMPARE(deviceRemovedSpy.count(), 0);
    QCOMPARE(deviceUpdatedSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 0);

    DeviceManager::setDeviceState(dev->id(), IDevice::DeviceReadyToUse);
    QCOMPARE(DeviceManager::find(dev->id())->deviceState(), IDevice::DeviceReadyToUse);
    QCOMPARE(deviceAddedSpy.count(), 0);
    QCOMPARE(deviceRemovedSpy.count(), 0);
    QCOMPARE(deviceUpdatedSpy.count(), 1);
    QCOMPARE(updatedSpy.count(), 1);
    deviceUpdatedSpy.clear();
    updatedSpy.clear();

    TestDevice::Ptr dev3 = IDevice::Ptr(new TestDevice);
    QVERIFY(dev->id() != dev3->id());

    dev3->setDisplayName(dev->displayName());
    DeviceManager::addDevice(dev3);
    QCOMPARE(
        DeviceManager::deviceAt(DeviceManager::deviceCount() - 1)->displayName(),
        QString(dev->displayName() + QLatin1Char('2')));
    QCOMPARE(deviceAddedSpy.count(), 1);
    QCOMPARE(deviceRemovedSpy.count(), 0);
    QCOMPARE(deviceUpdatedSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 1);
    deviceAddedSpy.clear();
    updatedSpy.clear();

    DeviceManager::removeDevice(dev->id());
    DeviceManager::removeDevice(dev3->id());
    QCOMPARE(DeviceManager::deviceCount(), oldDeviceCount);
    QVERIFY(!DeviceManager::find(dev->id()));
    QVERIFY(!DeviceManager::find(dev3->id()));
    QCOMPARE(deviceAddedSpy.count(), 0);
    QCOMPARE(deviceRemovedSpy.count(), 2);
    //    QCOMPARE(deviceUpdatedSpy.count(), 0); Uncomment once the "default" stuff is gone.
    QCOMPARE(updatedSpy.count(), 2);
}

} // namespace ProjectExplorer

#endif // WITH_TESTS
