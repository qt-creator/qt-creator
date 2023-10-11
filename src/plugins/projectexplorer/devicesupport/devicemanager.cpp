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
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/terminalhooks.h>

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QVariantList>

#include <limits>
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

    static DeviceManager *clonedInstance;

    mutable QMutex mutex;
    QList<IDevice::Ptr> devices;
    QHash<Id, Id> defaultDevices;
    PersistentSettingsWriter *writer = nullptr;
};
DeviceManager *DeviceManagerPrivate::clonedInstance = nullptr;

} // namespace Internal

using namespace Internal;

DeviceManager *DeviceManager::m_instance = nullptr;

DeviceManager *DeviceManager::instance()
{
    return m_instance;
}

int DeviceManager::deviceCount() const
{
    return d->devices.count();
}

void DeviceManager::replaceInstance()
{
    const QList<Id> newIds =
        Utils::transform(DeviceManagerPrivate::clonedInstance->d->devices, &IDevice::id);

    for (const IDevice::Ptr &dev : std::as_const(m_instance->d->devices)) {
        if (!newIds.contains(dev->id()))
            dev->aboutToBeRemoved();
    }

    {
        QMutexLocker locker(&instance()->d->mutex);
        copy(DeviceManagerPrivate::clonedInstance, instance(), false);
    }

    emit instance()->deviceListReplaced();
    emit instance()->updated();
}

void DeviceManager::removeClonedInstance()
{
    delete DeviceManagerPrivate::clonedInstance;
    DeviceManagerPrivate::clonedInstance = nullptr;
}

DeviceManager *DeviceManager::cloneInstance()
{
    QTC_ASSERT(!DeviceManagerPrivate::clonedInstance, return nullptr);

    DeviceManagerPrivate::clonedInstance = new DeviceManager(false);
    copy(instance(), DeviceManagerPrivate::clonedInstance, true);
    return DeviceManagerPrivate::clonedInstance;
}

DeviceManager *DeviceManager::clonedInstance()
{
    return DeviceManagerPrivate::clonedInstance;
}

void DeviceManager::copy(const DeviceManager *source, DeviceManager *target, bool deep)
{
    if (deep) {
        for (const IDevice::Ptr &device : std::as_const(source->d->devices))
            target->d->devices << device->clone();
    } else {
        target->d->devices = source->d->devices;
    }
    target->d->defaultDevices = source->d->defaultDevices;
}

void DeviceManager::save()
{
    if (d->clonedInstance == this || !d->writer)
        return;
    Store data;
    data.insert(DeviceManagerKey, variantFromStore(toMap()));
    d->writer->save(data, Core::ICore::dialogParent());
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
    for (IDevice::ConstPtr device : std::as_const(userDevices)) {
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

    emit devicesLoaded();
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

Store DeviceManager::toMap() const
{
    Store map;
    Store defaultDeviceMap;
    for (auto it = d->defaultDevices.constBegin(); it != d->defaultDevices.constEnd(); ++it)
        defaultDeviceMap.insert(keyFromString(it.key().toString()), it.value().toSetting());

    map.insert(DefaultDevicesKey, variantFromStore(defaultDeviceMap));
    QVariantList deviceList;
    for (const IDevice::Ptr &device : std::as_const(d->devices))
        deviceList << variantFromStore(device->toMap());
    map.insert(DeviceListKey, deviceList);
    return map;
}

void DeviceManager::addDevice(const IDevice::ConstPtr &_device)
{
    const IDevice::Ptr device = _device->clone();

    QStringList names;
    for (const IDevice::Ptr &tmp : std::as_const(d->devices)) {
        if (tmp->id() != device->id())
            names << tmp->displayName();
    }

    // TODO: make it thread safe?
    device->settings()->displayName.setValue(
        Utils::makeUniquelyNumbered(device->displayName(), names));

    const int pos = d->indexForId(device->id());

    if (!defaultDevice(device->type()))
        d->defaultDevices.insert(device->type(), device->id());
    if (this == DeviceManager::instance() && d->clonedInstance)
        d->clonedInstance->addDevice(device->clone());

    if (pos >= 0) {
        {
            QMutexLocker locker(&d->mutex);
            d->devices[pos] = device;
        }
        emit deviceUpdated(device->id());
    } else {
        {
            QMutexLocker locker(&d->mutex);
            d->devices << device;
        }
        emit deviceAdded(device->id());

        if (FSEngine::isAvailable())
            FSEngine::addDevice(device->rootPath());
    }

    emit updated();
}

void DeviceManager::removeDevice(Id id)
{
    const IDevice::Ptr device = mutableDevice(id);
    QTC_ASSERT(device, return);
    QTC_ASSERT(this != instance() || device->isAutoDetected(), return);

    const bool wasDefault = d->defaultDevices.value(device->type()) == device->id();
    const Id deviceType = device->type();
    {
        QMutexLocker locker(&d->mutex);
        d->devices.removeAt(d->indexForId(id));
    }
    emit deviceRemoved(device->id());

    if (FSEngine::isAvailable())
        FSEngine::removeDevice(device->rootPath());

    if (wasDefault) {
        for (int i = 0; i < d->devices.count(); ++i) {
            if (deviceAt(i)->type() == deviceType) {
                d->defaultDevices.insert(deviceAt(i)->type(), deviceAt(i)->id());
                emit deviceUpdated(deviceAt(i)->id());
                break;
            }
        }
    }
    if (this == instance() && d->clonedInstance)
        d->clonedInstance->removeDevice(id);

    emit updated();
}

void DeviceManager::setDeviceState(Id deviceId, IDevice::DeviceState deviceState)
{
    // To see the state change in the DeviceSettingsWidget. This has to happen before
    // the pos check below, in case the device is only present in the cloned instance.
    if (this == instance() && d->clonedInstance)
        d->clonedInstance->setDeviceState(deviceId, deviceState);

    const int pos = d->indexForId(deviceId);
    if (pos < 0)
        return;
    IDevice::Ptr &device = d->devices[pos];
    if (device->deviceState() == deviceState)
        return;

    // TODO: make it thread safe?
    device->setDeviceState(deviceState);
    emit deviceUpdated(deviceId);
    emit updated();
}

bool DeviceManager::isLoaded() const
{
    return d->writer;
}

// Thread safe
IDevice::ConstPtr DeviceManager::deviceForPath(const FilePath &path)
{
    const QList<IDevice::Ptr> devices = instance()->d->deviceList();

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
    QTC_ASSERT(this != instance(), return);

    const IDevice::ConstPtr &device = find(id);
    QTC_ASSERT(device, return);
    const IDevice::ConstPtr &oldDefaultDevice = defaultDevice(device->type());
    if (device == oldDefaultDevice)
        return;
    d->defaultDevices.insert(device->type(), device->id());
    emit deviceUpdated(device->id());
    emit deviceUpdated(oldDefaultDevice->id());

    emit updated();
}

DeviceManager::DeviceManager(bool isInstance) : d(std::make_unique<DeviceManagerPrivate>())
{
    QTC_ASSERT(isInstance == !m_instance, return);

    if (!isInstance)
        return;

    m_instance = this;
    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &DeviceManager::save);

    DeviceFileHooks &deviceHooks = DeviceFileHooks::instance();

    deviceHooks.isSameDevice = [](const FilePath &left, const FilePath &right) {
        auto leftDevice = DeviceManager::deviceForPath(left);
        auto rightDevice = DeviceManager::deviceForPath(right);

        return leftDevice == rightDevice;
    };

    deviceHooks.localSource = [](const FilePath &file) -> expected_str<FilePath> {
        auto device = DeviceManager::deviceForPath(file);
        if (!device)
            return make_unexpected(Tr::tr("No device for path \"%1\"").arg(file.toUserOutput()));
        return device->localSource(file);
    };

    deviceHooks.fileAccess = [](const FilePath &filePath) -> expected_str<DeviceFileAccess *> {
        if (!filePath.needsDevice())
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

    deviceHooks.environment = [](const FilePath &filePath) -> expected_str<Environment> {
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
    if (d->clonedInstance != this)
        delete d->writer;
    if (m_instance == this)
        m_instance = nullptr;
}

IDevice::ConstPtr DeviceManager::deviceAt(int idx) const
{
    QTC_ASSERT(idx >= 0 && idx < deviceCount(), return IDevice::ConstPtr());
    return d->devices.at(idx);
}

void DeviceManager::forEachDevice(const std::function<void(const IDeviceConstPtr &)> &func) const
{
    const QList<IDevice::Ptr> devices = d->deviceList();

    for (const IDevice::Ptr &device : devices)
        func(device);
}

IDevice::Ptr DeviceManager::mutableDevice(Id id) const
{
    const int index = d->indexForId(id);
    return index == -1 ? IDevice::Ptr() : d->devices.at(index);
}

bool DeviceManager::hasDevice(const QString &name) const
{
    return Utils::anyOf(d->devices, [&name](const IDevice::Ptr &device) {
        return device->displayName() == name;
    });
}

IDevice::ConstPtr DeviceManager::find(Id id) const
{
    const int index = d->indexForId(id);
    return index == -1 ? IDevice::ConstPtr() : deviceAt(index);
}

IDevice::ConstPtr DeviceManager::defaultDevice(Id deviceType) const
{
    const Id id = d->defaultDevices.value(deviceType);
    return id.isValid() ? find(id) : IDevice::ConstPtr();
}

} // namespace ProjectExplorer


#ifdef WITH_TESTS
#include <projectexplorer/projectexplorer.h>
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

namespace ProjectExplorer {

class TestDevice : public IDevice
{
public:
    TestDevice()
    {
        setupId(AutoDetected, Id::fromString(QUuid::createUuid().toString()));
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

void ProjectExplorerPlugin::testDeviceManager()
{
    TestDeviceFactory factory;

    TestDevice::Ptr dev = IDevice::Ptr(new TestDevice);
    dev->settings()->displayName.setValue(QLatin1String("blubbdiblubbfurz!"));
    QVERIFY(dev->isAutoDetected());
    QCOMPARE(dev->deviceState(), IDevice::DeviceStateUnknown);
    QCOMPARE(dev->type(), TestDevice::testTypeId());

    TestDevice::Ptr dev2 = dev->clone();
    QCOMPARE(dev->id(), dev2->id());

    DeviceManager * const mgr = DeviceManager::instance();
    QVERIFY(!mgr->find(dev->id()));
    const int oldDeviceCount = mgr->deviceCount();

    QSignalSpy deviceAddedSpy(mgr, &DeviceManager::deviceAdded);
    QSignalSpy deviceRemovedSpy(mgr, &DeviceManager::deviceRemoved);
    QSignalSpy deviceUpdatedSpy(mgr, &DeviceManager::deviceUpdated);
    QSignalSpy deviceListReplacedSpy(mgr, &DeviceManager::deviceListReplaced);
    QSignalSpy updatedSpy(mgr, &DeviceManager::updated);

    mgr->addDevice(dev);
    QCOMPARE(mgr->deviceCount(), oldDeviceCount + 1);
    QVERIFY(mgr->find(dev->id()));
    QVERIFY(mgr->hasDevice(dev->displayName()));
    QCOMPARE(deviceAddedSpy.count(), 1);
    QCOMPARE(deviceRemovedSpy.count(), 0);
    QCOMPARE(deviceUpdatedSpy.count(), 0);
    QCOMPARE(deviceListReplacedSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 1);
    deviceAddedSpy.clear();
    updatedSpy.clear();

    mgr->setDeviceState(dev->id(), IDevice::DeviceStateUnknown);
    QCOMPARE(deviceAddedSpy.count(), 0);
    QCOMPARE(deviceRemovedSpy.count(), 0);
    QCOMPARE(deviceUpdatedSpy.count(), 0);
    QCOMPARE(deviceListReplacedSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 0);

    mgr->setDeviceState(dev->id(), IDevice::DeviceReadyToUse);
    QCOMPARE(mgr->find(dev->id())->deviceState(), IDevice::DeviceReadyToUse);
    QCOMPARE(deviceAddedSpy.count(), 0);
    QCOMPARE(deviceRemovedSpy.count(), 0);
    QCOMPARE(deviceUpdatedSpy.count(), 1);
    QCOMPARE(deviceListReplacedSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 1);
    deviceUpdatedSpy.clear();
    updatedSpy.clear();

    mgr->addDevice(dev2);
    QCOMPARE(mgr->deviceCount(), oldDeviceCount + 1);
    QVERIFY(mgr->find(dev->id()));
    QCOMPARE(deviceAddedSpy.count(), 0);
    QCOMPARE(deviceRemovedSpy.count(), 0);
    QCOMPARE(deviceUpdatedSpy.count(), 1);
    QCOMPARE(deviceListReplacedSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 1);
    deviceUpdatedSpy.clear();
    updatedSpy.clear();

    TestDevice::Ptr dev3 = IDevice::Ptr(new TestDevice);
    QVERIFY(dev->id() != dev3->id());

    dev3->settings()->displayName.setValue(dev->displayName());
    mgr->addDevice(dev3);
    QCOMPARE(mgr->deviceAt(mgr->deviceCount() - 1)->displayName(),
             QString(dev3->displayName() + QLatin1Char('2')));
    QCOMPARE(deviceAddedSpy.count(), 1);
    QCOMPARE(deviceRemovedSpy.count(), 0);
    QCOMPARE(deviceUpdatedSpy.count(), 0);
    QCOMPARE(deviceListReplacedSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 1);
    deviceAddedSpy.clear();
    updatedSpy.clear();

    mgr->removeDevice(dev->id());
    mgr->removeDevice(dev3->id());
    QCOMPARE(mgr->deviceCount(), oldDeviceCount);
    QVERIFY(!mgr->find(dev->id()));
    QVERIFY(!mgr->find(dev3->id()));
    QCOMPARE(deviceAddedSpy.count(), 0);
    QCOMPARE(deviceRemovedSpy.count(), 2);
//    QCOMPARE(deviceUpdatedSpy.count(), 0); Uncomment once the "default" stuff is gone.
    QCOMPARE(deviceListReplacedSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 2);
}

} // namespace ProjectExplorer

#endif // WITH_TESTS
