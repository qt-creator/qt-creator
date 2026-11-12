// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicemanager.h"

#include "../projectexplorerconstants.h"
#include "../projectexplorericons.h"
#include "../projectexplorertr.h"
#include "idevicefactory.h"
#include "sshparameters.h"

#include <coreplugin/foldernavigationwidget.h>
#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/fsengine/fsengine.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/persistentsettings.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/synchronizedvalue.h>
#include <utils/terminalhooks.h>

#include <QHash>
#include <QMutex>
#include <QMenu>
#include <QMutexLocker>
#include <QPushButton>
#include <QVariantList>

#include <memory>

#ifdef WITH_TESTS
#include "../kitmanager.h"
#include "devicekitaspects.h"

#include <QScopeGuard>
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
    QList<Store> unrestoredDevices;
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

// Applies the rules load() uses for the user and SDK devices to the entries no factory could
// restore yet.
static void mergeUnrestoredSdkDevices(QList<Store> &unrestored, QList<Store> sdkStores)
{
    const Key originKey = "Origin";
    const Key versionKey = "Version";
    const auto hasId = [](Id id) {
        return [id](const Store &map) { return IDevice::idFromMap(map) == id; };
    };
    for (Store &sdkStore : sdkStores)
        sdkStore.insert(originKey, IDevice::AddedBySdk);
    // Make sure devices removed via the sdktool really disappear.
    unrestored.removeIf([&](const Store &map) {
        return map.value(originKey).toInt() == IDevice::AddedBySdk
               && !Utils::anyOf(sdkStores, hasId(IDevice::idFromMap(map)));
    });
    for (const Store &sdkStore : std::as_const(sdkStores)) {
        const auto user = std::find_if(unrestored.begin(), unrestored.end(),
                                       hasId(IDevice::idFromMap(sdkStore)));
        if (user == unrestored.end())
            unrestored.append(sdkStore);
        else if (user->value(versionKey, 0).toInt() < sdkStore.value(versionKey, 0).toInt())
            *user = sdkStore;
        else
            user->insert(originKey, IDevice::AddedBySdk); // For pre-17 settings.
    }
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
    QList<Store> unrestoredSdkDevices;
    if (reader.load(systemSettingsFilePath("devices.xml")))
        sdkDevices = fromMap(storeFromVariant(reader.restoreValues().value(DeviceManagerKey)),
                             &defaultDevices,
                             &unrestoredSdkDevices);
    for (IDevice::Ptr &device : sdkDevices)
        device->setFromSdk();
    // read devices file from user settings path
    QList<IDevice::Ptr> userDevices;
    if (reader.load(settingsFilePath("devices.xml")))
        userDevices = fromMap(storeFromVariant(reader.restoreValues().value(DeviceManagerKey)),
                              &defaultDevices,
                              &d->unrestoredDevices);
    mergeUnrestoredSdkDevices(d->unrestoredDevices, unrestoredSdkDevices);
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
        if (device) {
            d->defaultDevices[device->type()] = device->id();
        } else if (Utils::anyOf(d->unrestoredDevices, [&itr](const Store &map) {
                       return IDevice::idFromMap(map) == itr.value();
                   })) {
            d->defaultDevices.insert(itr.key(), itr.value());
        }
    }

    // Trigger auto-connection
    for (const IDevice::Ptr &device : std::as_const(d->devices))
        device->postLoad();

    emit s_instance->devicesLoaded();
}

static const IDeviceFactory *findRestoreFactory(const Store &map)
{
    const Id deviceType = IDevice::typeFromMap(map);
    return Utils::findOrDefault(IDeviceFactory::allDeviceFactories(),
        [&map, deviceType](IDeviceFactory *factory) {
            return factory->canRestore(map) && factory->deviceType() == deviceType;
        });
}

static const IDeviceFactory *restoreFactory(const Store &map)
{
    const IDeviceFactory * const factory = findRestoreFactory(map);
    if (!factory)
        qWarning("Warning: No factory found for device '%s' of type '%s'.",
                 qPrintable(IDevice::idFromMap(map).toString()),
                 qPrintable(IDevice::typeFromMap(map).toString()));
    return factory;
}

QList<IDevice::Ptr> DeviceManager::fromMap(
    const Store &map, QHash<Id, Id> *defaultDevices, QList<Store> *unrestoredDevices)
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
        if (!factory) {
            // Keep the entry, so disabling the plugin owning the type does not
            // delete the device on the next save.
            if (unrestoredDevices)
                unrestoredDevices->append(map);
            continue;
        }
        const IDevice::Ptr device = factory->construct();
        QTC_ASSERT(device, continue);
        device->fromMap(map);
        d->initDeviceState(device->id());
        devices << device;
    }
    return devices;
}

void DeviceManager::restoreDevicesWithUnknownType()
{
    QList<IDevice::Ptr> restored;
    for (auto it = d->unrestoredDevices.begin(); it != d->unrestoredDevices.end(); ) {
        // The plugin that brought the factory may have added this device by itself already.
        if (d->indexForId(IDevice::idFromMap(*it)) >= 0) {
            it = d->unrestoredDevices.erase(it);
            continue;
        }
        const IDeviceFactory * const factory = findRestoreFactory(*it);
        if (!factory) {
            ++it;
            continue;
        }
        const IDevice::Ptr device = factory->construct();
        QTC_ASSERT(device, ++it; continue);
        device->fromMap(*it);
        restored << device;
        it = d->unrestoredDevices.erase(it);
    }
    if (restored.isEmpty())
        return;

    // Make all of them findable before the first signal, so that listeners do not reassign
    // kits or the saved default device to whichever of them comes first.
    QStringList names = Utils::transform(d->devices, &IDevice::displayName);
    for (const IDevice::Ptr &device : std::as_const(restored)) {
        device->setDisplayName(Utils::makeUniquelyNumbered(device->displayName(), names));
        names << device->displayName();
        d->initDeviceState(device->id());
    }
    {
        QMutexLocker locker(&d->mutex);
        d->devices << restored;
    }
    for (const IDevice::Ptr &device : std::as_const(restored)) {
        if (!defaultDevice(device->type()))
            d->defaultDevices.insert(device->type(), device->id());
    }
    for (const IDevice::Ptr &device : std::as_const(restored)) {
        emit s_instance->deviceAdded(device->id());
        if (FSEngine::isAvailable())
            FSEngine::addDevice(device->rootPath());
    }
    emit s_instance->updated();

    for (const IDevice::Ptr &device : std::as_const(restored))
        device->postLoad();
}

bool DeviceManager::isUnrestored(Id id)
{
    return id.isValid() && Utils::anyOf(d->unrestoredDevices, [id](const Store &map) {
        return IDevice::idFromMap(map) == id;
    });
}

void DeviceManager::deviceFactoryAdded()
{
    // A factory was registered after the devices were loaded (e.g. by a soft-loaded plugin).
    // This runs from the IDeviceFactory base-class constructor, before the derived class has set
    // its construction function, so defer the retry to the event loop.
    if (!d || !d->writer || d->unrestoredDevices.isEmpty())
        return;
    static bool scheduled = false;
    if (scheduled)
        return;
    scheduled = true;
    QMetaObject::invokeMethod(
        s_instance,
        [] {
            scheduled = false;
            if (d)
                restoreDevicesWithUnknownType();
        },
        Qt::QueuedConnection);
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
        if (!device->isPersistent())
            continue;
        Store store;
        device->toMap(store);
        QTC_ASSERT(!store.isEmpty(), continue);
        deviceList << variantFromStore(store);
    }
    for (const Store &store : std::as_const(d->unrestoredDevices))
        deviceList << variantFromStore(store);
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
        const auto it = lockedStates->find(deviceId);
        if (it == lockedStates->end())
            lockedStates->insert(deviceId, newState);
        else if (*it != newState)
            *it = newState;
        else
            return;
    }

    if (announce)
        announceDeviceUpdate(deviceId);
}

void DeviceManager::announceDeviceUpdate(Id deviceId)
{
    if (!d || !s_instance || ExtensionSystem::PluginManager::isShuttingDown())
        return;

    emit s_instance->deviceUpdated(deviceId);
    emit s_instance->updated();
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

    if (!defaultDevice(device->type()) && !isUnrestored(d->defaultDevices.value(device->type())))
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

    d->initDeviceState(device->id());
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
        d->defaultDevices.remove(deviceType);
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
    // During shutdown the DeviceManager may already be gone (static d reset in
    // ~DeviceManager) while background tasks still touch the device-file hooks.
    // Bail out instead of dereferencing a null d.
    QTC_ASSERT(d, return {});

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
        if (leftDevice || rightDevice)
            return leftDevice == rightDevice;
        // Fallback if e.g. no devices are registered yet.
        return left.scheme() == right.scheme() && left.host() == right.host();
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

    deviceHooks.sourcedEnvironment = [](const FilePath &filePath) -> Result<Environment> {
        auto device = DeviceManager::deviceForPath(filePath);
        if (!device) {
            return make_unexpected(
                Tr::tr("No device found for path \"%1\".").arg(filePath.toUserOutput()));
        }
        return device->sourcedEnvironment(filePath);
    };

    deviceHooks.deviceDisplayName = [](const FilePath &filePath) {
        auto device = DeviceManager::deviceForPath(filePath);
        if (device)
            return device->displayName();
        return filePath.host().toString();
    };

    deviceHooks.deviceIcon = [](const FilePath &filePath) -> QIcon {
        auto device = DeviceManager::deviceForPath(filePath);
        if (!device)
            return {};
        return IDeviceFactory::iconForDeviceType(device->type());
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

void DeviceManager::addToolDetector(
    QObject *guard, const QString &searchMessage, const ToolDetector &detector)
{
    connect(instance(), &DeviceManager::toolDetectionRequested, guard,
            [searchMessage, detector](Id devId, const FilePaths &searchPaths, quint64 token,
                                      const ToolDetectionLogger &logger) {
        const IDevicePtr device = find(devId);
        QTC_ASSERT(device, return);
        device->registerToolDetectionTask(token);
        if (logger && !searchMessage.isEmpty())
            logger.logTopLevel(searchMessage);
        const auto done = [weakDevice = std::weak_ptr<IDevice>(device), token] {
            if (const IDevicePtr device = weakDevice.lock())
                device->deregisterToolDetectionTask(token);
        };
        detector({device, token, searchPaths, DetectionSource::forDevice(device), logger}, done);
    });
}

#ifdef WITH_TESTS
namespace Internal {

class TestDevice : public IDevice
{
public:
    explicit TestDevice(Id id = Id::generate())
    {
        setupId(AutoDetected, id);
        setType(testTypeId());
        setMachineType(Hardware);
        setOsType(HostOsInfo::hostOs());
        setDisplayType("blubb");
    }

    static Id testTypeId() { return "TestType"; }

    Store toStore() const
    {
        Store map;
        toMap(map);
        return map;
    }

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

// Knows one executable, which the host does not have.
class DeviceOnlyToolFileAccess final : public DesktopDeviceFileAccess
{
public:
    static QString toolPath() { return "/qtc-device-only/bin/qtc-test-tool"; }

    Result<bool> isExecutableFile(const FilePath &filePath) const final
    {
        return filePath.path() == toolPath();
    }
};

class DeviceOnlyToolDevice final : public IDevice
{
public:
    explicit DeviceOnlyToolDevice(OsType osType)
    {
        setupId(AutoDetected, Id::generate());
        setType(TestDevice::testTypeId());
        setOsType(osType);
        setFileAccess(std::make_shared<DeviceOnlyToolFileAccess>(), false);
    }

    void initToolAspects() { initDeviceToolAspects(); }

private:
    IDeviceWidget *createWidget() override { return nullptr; }
};

class TestToolFactory final : public DeviceToolAspectFactory
{
public:
    static Id testToolId() { return "Test.DeviceOnlyTool"; }

    explicit TestToolFactory(const Checker &checker = {})
    {
        setToolId(testToolId());
        setDisplayName("Test Tool");
        setFilePattern({"qtc-test-tool"});
        setChecker(checker);
    }
};

static DeviceToolAspect *findTestToolAspect(const IDevice::Ptr &dev)
{
    return Utils::findOrDefault(
        dev->deviceToolAspects(DeviceToolAspect::AllTools), [](DeviceToolAspect *aspect) {
            return aspect->toolId() == TestToolFactory::testToolId();
        });
}

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
        QCOMPARE(deviceUpdatedSpy.count(), 1);
        QCOMPARE(updatedSpy.count(), 1);

        DeviceManager::setDeviceState(dev->id(), IDevice::DeviceReadyToUse);
        QCOMPARE(DeviceManager::find(dev->id())->deviceState(), IDevice::DeviceReadyToUse);
        QCOMPARE(deviceAddedSpy.count(), 0);
        QCOMPARE(deviceAboutToBeRemovedSpy.count(), 0);
        QCOMPARE(deviceRemovedSpy.count(), 0);
        QCOMPARE(deviceUpdatedSpy.count(), 2);
        QCOMPARE(updatedSpy.count(), 2);
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

    void testRestoreWhenFactoryAppears()
    {
        QVERIFY(!IDeviceFactory::find(TestDevice::testTypeId()));
        QVERIFY(!DeviceManager::defaultDevice(TestDevice::testTypeId()));
        TestDevice first;
        TestDevice saved;
        TestDevice registeredMeanwhile;
        const IDevice::Ptr unrelated(new TestDevice);
        unrelated->setType("OtherTestType");
        d->unrestoredDevices << first.toStore() << saved.toStore()
                             << registeredMeanwhile.toStore();
        d->defaultDevices.insert(TestDevice::testTypeId(), saved.id());
        Kit * const kit = KitManager::registerKit([&saved](Kit *k) {
            RunDeviceTypeKitAspect::setDeviceTypeId(k, TestDevice::testTypeId());
            RunDeviceKitAspect::setDeviceId(k, saved.id());
        });
        const QScopeGuard cleanup([&] {
            const QList<Id> ids{first.id(), saved.id(), registeredMeanwhile.id(), unrelated->id()};
            d->unrestoredDevices.removeIf(
                [&ids](const Store &map) { return ids.contains(IDevice::idFromMap(map)); });
            for (const Id id : ids) {
                while (DeviceManager::find(id))
                    DeviceManager::removeDevice(id);
            }
            if (kit)
                KitManager::deregisterKit(kit);
            d->defaultDevices.remove(TestDevice::testTypeId());
        });
        QVERIFY(kit);
        const auto storedKitDevice = [kit] {
            return Id::fromSetting(kit->value(RunDeviceKitAspect::id()));
        };
        DeviceManager::addDevice(unrelated);
        QCOMPARE(storedKitDevice(), saved.id());

        const TestDeviceFactory factory;
        DeviceManager::addDevice(IDevice::Ptr(new TestDevice(registeredMeanwhile.id())));
        QTRY_VERIFY(DeviceManager::find(first.id()) && DeviceManager::find(saved.id()));
        QVERIFY(!DeviceManager::isUnrestored(first.id()));
        QVERIFY(!DeviceManager::isUnrestored(saved.id()));
        QVERIFY(!DeviceManager::isUnrestored(registeredMeanwhile.id()));
        int registeredCount = 0;
        for (int i = 0; i < DeviceManager::deviceCount(); ++i) {
            if (DeviceManager::deviceAt(i)->id() == registeredMeanwhile.id())
                ++registeredCount;
        }
        QCOMPARE(registeredCount, 1);
        const IDevice::ConstPtr defaultDevice = DeviceManager::defaultDevice(
            TestDevice::testTypeId());
        QVERIFY(defaultDevice);
        QCOMPARE(defaultDevice->id(), saved.id());
        QCOMPARE(storedKitDevice(), saved.id());
    }

    void testMergeUnrestoredSdkDevices()
    {
        const auto withVersion = [](const TestDevice &device, int version) {
            Store map = device.toStore();
            map.insert("Version", version);
            return map;
        };
        const auto fromSdk = [](Store map) {
            map.insert("Origin", IDevice::AddedBySdk);
            return map;
        };
        TestDevice userOnly;
        TestDevice removedFromSdk;
        TestDevice outdated;
        TestDevice current;
        TestDevice newInSdk;
        QList<Store> unrestored{userOnly.toStore(),
                                fromSdk(removedFromSdk.toStore()),
                                withVersion(outdated, 1),
                                withVersion(current, 2)};
        mergeUnrestoredSdkDevices(unrestored,
                                  {withVersion(outdated, 2),
                                   withVersion(current, 1),
                                   newInSdk.toStore()});

        const auto entry = [&unrestored](const TestDevice &device) {
            return Utils::findOrDefault(unrestored, [id = device.id()](const Store &map) {
                return IDevice::idFromMap(map) == id;
            });
        };
        QCOMPARE(unrestored.size(), 4);
        QVERIFY(entry(removedFromSdk).isEmpty());
        QCOMPARE(entry(userOnly).value("Origin").toInt(), int(IDevice::AutoDetected));
        QCOMPARE(entry(outdated).value("Version").toInt(), 2);
        QCOMPARE(entry(current).value("Version").toInt(), 2);
        for (const TestDevice *device : {&outdated, &current, &newInSdk})
            QCOMPARE(entry(*device).value("Origin").toInt(), int(IDevice::AddedBySdk));
    }

    void testIsDirty()
    {
        TestDevice::Ptr dev = IDevice::Ptr(new TestDevice);

        // A freshly created device has no pending (volatile) edits.
        QVERIFY(!dev->isDirty());

        // Editing an aspect that lives in the separate SSH parameters container
        // must register as dirty, even though it is a nested aspect container.
        StringAspect &host = dev->sshParametersAspectContainer().host;
        const QString original = host.volatileValue();
        host.setVolatileValue(original + "example.com");
        QVERIFY(dev->isDirty());

        // Reverting the edit clears the dirty state again.
        host.setVolatileValue(original);
        QVERIFY(!dev->isDirty());
    }

    void testDeviceLocalToolSurvivesDetection()
    {
        const TestToolFactory toolFactory;
        const auto dev = std::make_shared<DeviceOnlyToolDevice>(HostOsInfo::hostOs());
        dev->initToolAspects();
        DeviceManager::addDevice(dev);

        DeviceToolAspect *const toolAspect = findTestToolAspect(dev);
        QVERIFY(toolAspect);
        toolAspect->setValue(DeviceOnlyToolFileAccess::toolPath());

        QtTaskTree::QTaskTree::runBlocking(dev->autoDetectDeviceToolsRecipe());

        const FilePath toolPath = dev->deviceToolPath(TestToolFactory::testToolId());
        DeviceManager::removeDevice(dev->id());
        QCOMPARE(toolPath, dev->filePath(DeviceOnlyToolFileAccess::toolPath()));
    }

    void testDeviceToolSpelledTheDevicesWay()
    {
        const TestToolFactory toolFactory;
        const auto dev = std::make_shared<DeviceOnlyToolDevice>(OsTypeWindows);
        dev->initToolAspects();
        DeviceManager::addDevice(dev);

        DeviceToolAspect *const toolAspect = findTestToolAspect(dev);
        QVERIFY(toolAspect);
        const FilePath onDevice = dev->filePath("C:/tools/qtc-test-tool.exe");
        toolAspect->setToolPath(onDevice);
        const QString stored = toolAspect->value();
        const FilePath readBack = toolAspect->toolPath();

        DeviceManager::removeDevice(dev->id());
        QCOMPARE(stored, QString("C:\\tools\\qtc-test-tool.exe"));
        QCOMPARE(readBack, onDevice);
    }

    void testDeviceToolAlternativesSpelledTheDevicesWay()
    {
        const auto dev = std::make_shared<DeviceOnlyToolDevice>(OsTypeWindows);
        DeviceManager::addDevice(dev);

        PathChooser chooser;
        chooser.setBaseDirectory(dev->rootPath());
        chooser.setValueAlternatives(
            {dev->filePath("C:/tools/qtc-test-tool.exe"), dev->filePath("D:/qtc-test-tool.exe")});
        QStringList labels;
        QString chosen;
        const auto button = Utils::findOrDefault(
            chooser.findChildren<QPushButton *>(), [](QPushButton *b) { return b->menu(); });
        if (button) {
            const QList<QAction *> actions = button->menu()->actions();
            labels = Utils::transform(actions, &QAction::text);
            actions.first()->trigger();
            chosen = chooser.lineEdit()->text();
        }

        DeviceManager::removeDevice(dev->id());
        QCOMPARE(labels, QStringList({"C:\\tools\\qtc-test-tool.exe", "D:\\qtc-test-tool.exe"}));
        QCOMPARE(chosen, QString("C:\\tools\\qtc-test-tool.exe"));
    }

    void testDeviceToolValidatedOnTheDevice()
    {
        const TestToolFactory toolFactory([](const DeviceConstRef &, const FilePath &path) {
            return Result<>(ResultError("checked " + path.toUrlishString()));
        });
        const auto dev = std::make_shared<DeviceOnlyToolDevice>(HostOsInfo::hostOs());
        dev->initToolAspects();
        DeviceManager::addDevice(dev);

        DeviceToolAspect *const toolAspect = findTestToolAspect(dev);
        QVERIFY(toolAspect);
        Layouting::Column column;
        toolAspect->addToLayout(column);
        const std::unique_ptr<QWidget> widget(column.emerge());
        toolAspect->macroExpander()->registerVariable(
            "QtcTestToolDir", "Test tool directory", [] { return QString("/qtc-device-only/bin"); });
        toolAspect->setValue(QString("%{QtcTestToolDir}/qtc-test-tool"));
        PathChooser *const chooser = widget->findChild<PathChooser *>();
        QVERIFY(chooser);
        const QString expected
            = "checked " + dev->filePath(DeviceOnlyToolFileAccess::toolPath()).toUrlishString();
        QTRY_VERIFY(chooser->errorMessage().contains(expected));

        DeviceManager::removeDevice(dev->id());
    }

    void testDeviceToolKeepsMacros()
    {
        const TestToolFactory toolFactory;
        const auto dev = std::make_shared<DeviceOnlyToolDevice>(HostOsInfo::hostOs());
        dev->initToolAspects();
        DeviceManager::addDevice(dev);

        DeviceToolAspect *const toolAspect = findTestToolAspect(dev);
        QVERIFY(toolAspect);
        toolAspect->macroExpander()->registerVariable(
            "QtcTestToolDir", "Test tool directory", [] { return QString("/qtc-device-only/bin"); });
        const QString written = "%{QtcTestToolDir}/qtc-test-tool";
        toolAspect->setValue(written);
        const FilePath expanded = toolAspect->toolPath();

        QtTaskTree::QTaskTree::runBlocking(dev->autoDetectDeviceToolsRecipe());

        const QString stored = toolAspect->value();
        DeviceManager::removeDevice(dev->id());
        QCOMPARE(expanded, dev->filePath(DeviceOnlyToolFileAccess::toolPath()));
        QCOMPARE(stored, written);
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
