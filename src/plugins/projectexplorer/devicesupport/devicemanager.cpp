// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicemanager.h"

#include "../projectexplorerconstants.h"
#include "../projectexplorericons.h"
#include "../projectexplorertr.h"
#include "idevicefactory.h"

#include <coreplugin/foldernavigationwidget.h>
#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/fsengine/fsengine.h>
#include <utils/persistentsettings.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/synchronizedvalue.h>
#include <utils/terminalhooks.h>

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QVariantList>

#include <memory>

#ifdef WITH_TESTS
#include <QSignalSpy>
#include <QTest>
#endif

using namespace Core;
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

    void clearDeviceState(Utils::Id deviceId)
    {
        const auto lockedStates = deviceStates.writeLocked();
        lockedStates->remove(deviceId);
    }

    void initDeviceState(Utils::Id deviceId)
    {
        const auto lockedStates = deviceStates.writeLocked();
        if (!lockedStates->contains(deviceId))
            lockedStates->insert(deviceId, IDevice::DeviceDisconnected);
    }

    mutable QMutex mutex;
    QList<IDevice::Ptr> devices;
    QHash<Id, Id> defaultDevices;
    SynchronizedValue<QMap<Id, IDevice::DeviceState>> deviceStates;
    PersistentSettingsWriter *writer = nullptr;
};

} // namespace Internal

using namespace Internal;

static DeviceManager *s_instance = nullptr;
static std::unique_ptr<DeviceManagerPrivate> d;

DeviceManager *DeviceManager::instance()
{
    return s_instance;
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
    for (IDevice::Ptr &device : sdkDevices)
        device->setFromSdk();
    // read devices file from user settings path
    QList<IDevice::Ptr> userDevices;
    if (reader.load(settingsFilePath("devices.xml")))
        userDevices = fromMap(storeFromVariant(reader.restoreValues().value(DeviceManagerKey)), &defaultDevices);
    // Insert devices into the model. Prefer the higher device version when there are multiple
    // devices with the same id.
    for (IDevice::Ptr device : std::as_const(userDevices)) {
        // Make sure devices removed via the sdktool really disappear.
        if (device->isFromSdk()) {
            const bool stillPresent
                = Utils::contains(sdkDevices, [id = device->id()](const IDevice::Ptr &sdkDev) {
                      return sdkDev->id() == id;
                  });
            if (!stillPresent)
                continue;
        }

        for (const IDevice::Ptr &sdkDevice : std::as_const(sdkDevices)) {
            if (device->id() == sdkDevice->id() || device->rootPath() == sdkDevice->rootPath()) {
                device->setFromSdk(); // For pre-17 settings.
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

    // Trigger auto-connection
    for (const IDevice::Ptr &device : d->devices)
        device->postLoad();

    emit s_instance->devicesLoaded();
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
        d->initDeviceState(device->id());
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
        if (store.isEmpty())
            continue;
        deviceList << variantFromStore(store);
    }
    map.insert(DeviceListKey, deviceList);
    return map;
}

IDevice::DeviceState DeviceManager::deviceState(Utils::Id deviceId)
{
    return d->deviceStates.readLocked()->value(deviceId, IDevice::DeviceStateUnknown);
}

void DeviceManager::setDeviceState(Id deviceId, IDevice::DeviceState newState, bool announce)
{
    if (!d || !s_instance)
        return;

    {
        const auto lockedStates = d->deviceStates.writeLocked();
        if (lockedStates->value(deviceId, IDevice::DeviceStateUnknown) == newState)
            return;
        lockedStates->insert(deviceId, newState);
    }

    if (announce && !ExtensionSystem::PluginManager::isShuttingDown()) {
        emit s_instance->deviceUpdated(deviceId);
        emit s_instance->updated();
    }
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
        emit s_instance->deviceUpdated(device->id());
    } else {
        {
            QMutexLocker locker(&d->mutex);
            d->devices << device;
        }
        emit s_instance->deviceAdded(device->id());

        if (FSEngine::isAvailable())
            FSEngine::addDevice(device->rootPath());
    }

    emit s_instance->updated();
}

void DeviceManager::removeDevice(Id id)
{
    const IDevice::Ptr device = mutableDevice(id);
    QTC_ASSERT(device, return);

    device->aboutToBeRemoved();
    emit s_instance->deviceAboutToBeRemoved(device->id());

    const bool wasDefault = d->defaultDevices.value(device->type()) == device->id();
    const Id deviceType = device->type();
    {
        QMutexLocker locker(&d->mutex);
        d->devices.removeAt(d->indexForId(id));
    }
    emit s_instance->deviceRemoved(device->id());

    if (FSEngine::isAvailable())
        FSEngine::removeDevice(device->rootPath());

    if (wasDefault) {
        for (int i = 0; i < d->devices.count(); ++i) {
            if (deviceAt(i)->type() == deviceType) {
                d->defaultDevices.insert(deviceAt(i)->type(), deviceAt(i)->id());
                emit s_instance->deviceUpdated(deviceAt(i)->id());
                break;
            }
        }
    }

    d->clearDeviceState(id);
    emit s_instance->updated();
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
    return s_instance->defaultDevice(Constants::DESKTOP_DEVICE_TYPE);
}

void DeviceManager::setDefaultDevice(Id id)
{
    const IDevice::ConstPtr &device = find(id);
    QTC_ASSERT(device, return);
    const IDevice::ConstPtr &oldDefaultDevice = defaultDevice(device->type());
    if (device == oldDefaultDevice)
        return;
    d->defaultDevices.insert(device->type(), device->id());
    emit s_instance->deviceUpdated(device->id());
    emit s_instance->deviceUpdated(oldDefaultDevice->id());

    emit s_instance->updated();
}

DeviceManager::DeviceManager()
{
    d = std::make_unique<DeviceManagerPrivate>();
    s_instance = this;
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

    deviceHooks.fileAccess = [](const FilePath &filePath) -> Result<DeviceFileAccessPtr> {
        if (filePath.isLocal())
            return DesktopDeviceFileAccess::instance()->shared_from_this();
        IDevice::ConstPtr device = DeviceManager::deviceForPath(filePath);
        if (!device) {
            return make_unexpected(
                Tr::tr("No device found for path \"%1\".").arg(filePath.toUserOutput()));
        }
        DeviceFileAccessPtr fileAccess = device->fileAccess();
        if (!fileAccess) {
            return make_unexpected(
                Tr::tr("No file access for device \"%1\".").arg(device->displayName()));
        }
        return fileAccess;
    };

    deviceHooks.environment = [](const FilePath &filePath) -> Result<Environment> {
        auto device = DeviceManager::deviceForPath(filePath);
        if (!device) {
            return make_unexpected(
                Tr::tr("No device found for path \"%1\".").arg(filePath.toUserOutput()));
        }
        return device->systemEnvironmentWithError();
    };

    deviceHooks.deviceDisplayName = [](const FilePath &filePath) {
        auto device = DeviceManager::deviceForPath(filePath);
        if (device)
            return device->displayName();
        return filePath.host().toString();
    };

    deviceHooks.ensureReachable = [](const FilePath &filePath, const FilePath &other) -> Result<> {
        auto device = DeviceManager::deviceForPath(filePath);
        QTC_ASSERT(
            device,
            return ResultError(
                Tr::tr("No device found for path \"%1\".").arg(filePath.toUserOutput())));
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

    const auto addDeviceToFileSystemView = [](Id deviceId) -> bool {
        IDevice::Ptr device = find(deviceId);
        if (!device || device->type() == Constants::DESKTOP_DEVICE_TYPE
            || !device->supportsFileAccess()) {
            return true; // ignore
        }
        if (device->deviceState() != IDevice::DeviceReadyToUse
            && device->deviceState() != IDevice::DeviceConnected) {
            return false; // cannot add
        }
        const QIcon icon = device->icon().isNull() ? Icons::DESKTOP_DEVICE.icon() : device->icon();
        FolderNavigationWidgetFactory::insertRootDirectory(
            {device->rootPath().toUrlishString(),
             /*sortValue=*/30,
             device->displayName(),
             device->rootPath(),
             icon},
            /*isProjectDirectory=*/false);
        return true;
    };
    const auto removeDeviceFromFileSystemView = [](Id deviceId) {
        IDevice::Ptr device = find(deviceId);
        QTC_ASSERT(device, return);
        const QString id = device->rootPath().toUrlishString();
        if (FolderNavigationWidgetFactory::hasRootDirectory(id))
            FolderNavigationWidgetFactory::removeRootDirectory(id);
    };
    connect(
        this,
        &DeviceManager::deviceAdded,
        FolderNavigationWidgetFactory::instance(),
        addDeviceToFileSystemView);
    connect(
        this,
        &DeviceManager::deviceAboutToBeRemoved,
        FolderNavigationWidgetFactory::instance(),
        removeDeviceFromFileSystemView);
    connect(
        this,
        &DeviceManager::deviceUpdated,
        FolderNavigationWidgetFactory::instance(),
        [addDeviceToFileSystemView, removeDeviceFromFileSystemView](Utils::Id id) {
            if (!addDeviceToFileSystemView(id))
                removeDeviceFromFileSystemView(id);
        });
}

DeviceManager::~DeviceManager()
{
    delete d->writer;
    s_instance = nullptr;
    d.reset();
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

#ifdef WITH_TESTS
namespace Internal {

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

class DeviceManagerTest : public QObject
{
    Q_OBJECT

private slots:

    void test()
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
        QSignalSpy deviceAboutToBeRemovedSpy(mgr, &DeviceManager::deviceAboutToBeRemoved);
        QSignalSpy deviceRemovedSpy(mgr, &DeviceManager::deviceRemoved);
        QSignalSpy deviceUpdatedSpy(mgr, &DeviceManager::deviceUpdated);
        QSignalSpy updatedSpy(mgr, &DeviceManager::updated);

        DeviceManager::addDevice(dev);
        QCOMPARE(DeviceManager::deviceCount(), oldDeviceCount + 1);
        QVERIFY(DeviceManager::find(dev->id()));
        QVERIFY(DeviceManager::hasDevice(dev->displayName()));
        QCOMPARE(deviceAddedSpy.count(), 1);
        QCOMPARE(deviceAboutToBeRemovedSpy.count(), 0);
        QCOMPARE(deviceRemovedSpy.count(), 0);
        QCOMPARE(deviceUpdatedSpy.count(), 0);
        QCOMPARE(updatedSpy.count(), 1);
        deviceAddedSpy.clear();
        updatedSpy.clear();

        DeviceManager::setDeviceState(dev->id(), IDevice::DeviceStateUnknown);
        QCOMPARE(deviceAddedSpy.count(), 0);
        QCOMPARE(deviceAboutToBeRemovedSpy.count(), 0);
        QCOMPARE(deviceRemovedSpy.count(), 0);
        QCOMPARE(deviceUpdatedSpy.count(), 0);
        QCOMPARE(updatedSpy.count(), 0);

        DeviceManager::setDeviceState(dev->id(), IDevice::DeviceReadyToUse);
        QCOMPARE(DeviceManager::find(dev->id())->deviceState(), IDevice::DeviceReadyToUse);
        QCOMPARE(deviceAddedSpy.count(), 0);
        QCOMPARE(deviceAboutToBeRemovedSpy.count(), 0);
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
        QCOMPARE(deviceAboutToBeRemovedSpy.count(), 0);
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
        QCOMPARE(deviceAboutToBeRemovedSpy.count(), 2);
        QCOMPARE(deviceRemovedSpy.count(), 2);
        //    QCOMPARE(deviceUpdatedSpy.count(), 0); Uncomment once the "default" stuff is gone.
        QCOMPARE(updatedSpy.count(), 2);
    }
};

QObject *createDeviceManagerTest()
{
    return new DeviceManagerTest;
}

} // namespace Internal
#endif // WITH_TESTS

} // namespace ProjectExplorer

#ifdef WITH_TESTS
#include <devicemanager.moc>
#endif
