/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "devicemanager.h"

#include "idevicefactory.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/fileutils.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/portlist.h>

#include <QFileInfo>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariantList>

#include <limits>

namespace ProjectExplorer {
namespace Internal {

const char DeviceManagerKey[] = "DeviceManager";
const char DeviceListKey[] = "DeviceList";
const char DefaultDevicesKey[] = "DefaultDevices";

class DeviceManagerPrivate
{
public:
    DeviceManagerPrivate() : writer(0)
    { }

    int indexForId(Core::Id id) const
    {
        for (int i = 0; i < devices.count(); ++i) {
            if (devices.at(i)->id() == id)
                return i;
        }
        return -1;
    }

    static DeviceManager *instance;
    static DeviceManager *clonedInstance;
    QList<IDevice::Ptr> devices;
    QHash<Core::Id, Core::Id> defaultDevices;

    Utils::PersistentSettingsWriter *writer;
};
DeviceManager *DeviceManagerPrivate::clonedInstance = 0;
DeviceManager *DeviceManagerPrivate::instance = 0;

} // namespace Internal

using namespace Internal;


DeviceManager *DeviceManager::instance()
{
    return DeviceManagerPrivate::instance;
}

int DeviceManager::deviceCount() const
{
    return d->devices.count();
}

void DeviceManager::replaceInstance()
{
    copy(DeviceManagerPrivate::clonedInstance, instance(), false);
    emit instance()->deviceListChanged();
    emit instance()->updated();
}

void DeviceManager::removeClonedInstance()
{
    delete DeviceManagerPrivate::clonedInstance;
    DeviceManagerPrivate::clonedInstance = 0;
}

DeviceManager *DeviceManager::cloneInstance()
{
    QTC_ASSERT(!DeviceManagerPrivate::clonedInstance, return 0);

    DeviceManagerPrivate::clonedInstance = new DeviceManager(false);
    copy(instance(), DeviceManagerPrivate::clonedInstance, true);
    return DeviceManagerPrivate::clonedInstance;
}

void DeviceManager::copy(const DeviceManager *source, DeviceManager *target, bool deep)
{
    if (deep) {
        foreach (const IDevice::ConstPtr &device, source->d->devices)
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
    QVariantMap data;
    data.insert(QLatin1String(DeviceManagerKey), toMap());
    d->writer->save(data, Core::ICore::mainWindow());
}

void DeviceManager::load()
{
    QTC_ASSERT(!d->writer, return);

    // Only create writer now: We do not want to save before the settings were read!
    d->writer = new Utils::PersistentSettingsWriter(
                settingsFilePath(QLatin1String("/qtcreator/devices.xml")),
                QLatin1String("QtCreatorDevices"));

    Utils::PersistentSettingsReader reader;
    // read devices file from global settings path
    QList<IDevice::Ptr> sdkDevices;
    if (reader.load(systemSettingsFilePath(QLatin1String("/qtcreator/devices.xml"))))
        sdkDevices = fromMap(reader.restoreValues().value(QLatin1String(DeviceManagerKey)).toMap());
    // read devices file from user settings path
    QList<IDevice::Ptr> userDevices;
    if (reader.load(settingsFilePath(QLatin1String("/qtcreator/devices.xml"))))
        userDevices = fromMap(reader.restoreValues().value(QLatin1String(DeviceManagerKey)).toMap());
    // Insert devices into the model. Prefer the higher device version when there are multiple
    // devices with the same id.
    foreach (IDevice::Ptr device, userDevices) {
        foreach (const IDevice::Ptr &sdkDevice, sdkDevices) {
            if (device->id() == sdkDevice->id()) {
                if (device->version() < sdkDevice->version())
                    device = sdkDevice;
                sdkDevices.removeOne(sdkDevice);
                break;
            }
        }
        addDevice(device);
    }
    // Append the new SDK devices to the model.
    foreach (const IDevice::Ptr &sdkDevice, sdkDevices)
        addDevice(sdkDevice);

    ensureOneDefaultDevicePerType();

    emit devicesLoaded();
}

QList<IDevice::Ptr> DeviceManager::fromMap(const QVariantMap &map)
{
    QList<IDevice::Ptr> devices;
    const QVariantMap defaultDevsMap = map.value(QLatin1String(DefaultDevicesKey)).toMap();
    for (QVariantMap::ConstIterator it = defaultDevsMap.constBegin();
         it != defaultDevsMap.constEnd(); ++it) {
        d->defaultDevices.insert(Core::Id::fromString(it.key()), Core::Id::fromSetting(it.value()));
    }
    const QVariantList deviceList = map.value(QLatin1String(DeviceListKey)).toList();
    foreach (const QVariant &v, deviceList) {
        const QVariantMap map = v.toMap();
        const IDeviceFactory * const factory = restoreFactory(map);
        if (!factory)
            continue;
        const IDevice::Ptr device = factory->restore(map);
        QTC_ASSERT(device, continue);
        devices << device;
    }
    return devices;
}

QVariantMap DeviceManager::toMap() const
{
    QVariantMap map;
    QVariantMap defaultDeviceMap;
    typedef QHash<Core::Id, Core::Id> TypeIdHash;
    for (TypeIdHash::ConstIterator it = d->defaultDevices.constBegin();
             it != d->defaultDevices.constEnd(); ++it) {
        defaultDeviceMap.insert(it.key().toString(), it.value().toSetting());
    }
    map.insert(QLatin1String(DefaultDevicesKey), defaultDeviceMap);
    QVariantList deviceList;
    foreach (const IDevice::ConstPtr &device, d->devices)
        deviceList << device->toMap();
    map.insert(QLatin1String(DeviceListKey), deviceList);
    return map;
}

Utils::FileName DeviceManager::settingsFilePath(const QString &extension)
{
    return Utils::FileName::fromString(QFileInfo(Core::ICore::settings()->fileName()).absolutePath() + extension);
}

Utils::FileName DeviceManager::systemSettingsFilePath(const QString &deviceFileRelativePath)
{
    return Utils::FileName::fromString(
              QFileInfo(Core::ICore::settings(QSettings::SystemScope)->fileName()).absolutePath()
              + deviceFileRelativePath);
}

void DeviceManager::addDevice(const IDevice::ConstPtr &_device)
{
    const IDevice::Ptr device = _device->clone();

    QStringList names;
    foreach (const IDevice::ConstPtr &tmp, d->devices) {
        if (tmp->id() != device->id())
            names << tmp->displayName();
    }

    device->setDisplayName(Project::makeUnique(device->displayName(), names));

    const int pos = d->indexForId(device->id());

    if (!defaultDevice(device->type()))
        d->defaultDevices.insert(device->type(), device->id());
    if (this == DeviceManagerPrivate::instance && d->clonedInstance)
        d->clonedInstance->addDevice(device->clone());

    if (pos >= 0) {
        d->devices[pos] = device;
        emit deviceUpdated(device->id());
    } else {
        d->devices << device;
        emit deviceAdded(device->id());
    }

    emit updated();
}

void DeviceManager::removeDevice(Core::Id id)
{
    const IDevice::Ptr device = mutableDevice(id);
    QTC_ASSERT(device, return);
    QTC_ASSERT(this != instance() || device->isAutoDetected(), return);

    const bool wasDefault = d->defaultDevices.value(device->type()) == device->id();
    const Core::Id deviceType = device->type();
    d->devices.removeAt(d->indexForId(id));
    emit deviceRemoved(device->id());

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

void DeviceManager::setDeviceState(Core::Id deviceId, IDevice::DeviceState deviceState)
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

    device->setDeviceState(deviceState);
    emit deviceUpdated(deviceId);
    emit updated();
}

bool DeviceManager::isLoaded() const
{
    return d->writer;
}

void DeviceManager::setDefaultDevice(Core::Id id)
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

const IDeviceFactory *DeviceManager::restoreFactory(const QVariantMap &map)
{
    const QList<IDeviceFactory *> &factories
        = ExtensionSystem::PluginManager::getObjects<IDeviceFactory>();
    foreach (const IDeviceFactory * const factory, factories) {
        if (factory->canRestore(map))
            return factory;
    }
    qWarning("Warning: No factory found for device '%s' of type '%s'.",
             qPrintable(IDevice::idFromMap(map).toString()),
             qPrintable(IDevice::typeFromMap(map).toString()));
    return 0;
}

DeviceManager::DeviceManager(bool isInstance) : d(new DeviceManagerPrivate)
{
    if (isInstance) {
        connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()), SLOT(save()));
        QTC_CHECK(!DeviceManagerPrivate::instance);
        DeviceManagerPrivate::instance = this;
    }
}

DeviceManager::~DeviceManager()
{
    if (d->clonedInstance != this)
        delete d->writer;
    delete d;
}

IDevice::ConstPtr DeviceManager::deviceAt(int idx) const
{
    QTC_ASSERT(idx >= 0 && idx < deviceCount(), return IDevice::ConstPtr());
    return d->devices.at(idx);
}

IDevice::Ptr DeviceManager::mutableDevice(Core::Id id) const
{
    const int index = d->indexForId(id);
    return index == -1 ? IDevice::Ptr() : d->devices.at(index);
}

bool DeviceManager::hasDevice(const QString &name) const
{
    foreach (const IDevice::Ptr &device, d->devices) {
        if (device->displayName() == name)
            return true;
    }
    return false;
}

IDevice::ConstPtr DeviceManager::find(Core::Id id) const
{
    const int index = d->indexForId(id);
    return index == -1 ? IDevice::ConstPtr() : deviceAt(index);
}

IDevice::ConstPtr DeviceManager::defaultDevice(Core::Id deviceType) const
{
    const Core::Id id = d->defaultDevices.value(deviceType);
    return id.isValid() ? find(id) : IDevice::ConstPtr();
}

void DeviceManager::ensureOneDefaultDevicePerType()
{
    foreach (const IDevice::Ptr &device, d->devices) {
        if (!defaultDevice(device->type()))
            d->defaultDevices.insert(device->type(), device->id());
    }
}

IDevice::Ptr DeviceManager::fromRawPointer(IDevice *device) const
{
    foreach (const IDevice::Ptr &devPtr, d->devices) {
        if (devPtr == device)
            return devPtr;
    }

    if (this == instance() && d->clonedInstance)
        return d->clonedInstance->fromRawPointer(device);

    qWarning("%s: Device not found.", Q_FUNC_INFO);
    return IDevice::Ptr();
}

IDevice::ConstPtr DeviceManager::fromRawPointer(const IDevice *device) const
{
    // The const_cast is safe, because we convert the Ptr back to a ConstPtr before returning it.
    return fromRawPointer(const_cast<IDevice *>(device));
}

} // namespace ProjectExplorer


#ifdef WITH_TESTS
#include "projectexplorer.h"
#include <QSignalSpy>
#include <QTest>
#include <QUuid>

namespace ProjectExplorer {

class TestDevice : public IDevice
{
public:
    TestDevice()
        : IDevice(testTypeId(), AutoDetected, Hardware, Core::Id::fromString(QUuid::createUuid().toString())) {}

    static Core::Id testTypeId() { return Core::Id("TestType"); }
private:
    TestDevice(const TestDevice &other) : IDevice(other) {}
    QString displayType() const { return QLatin1String("blubb"); }
    IDeviceWidget *createWidget() { return 0; }
    QList<Core::Id> actionIds() const { return QList<Core::Id>(); }
    QString displayNameForActionId(Core::Id) const { return QString(); }
    void executeAction(Core::Id, QWidget *) const { }
    Ptr clone() const { return Ptr(new TestDevice(*this)); }
};

void ProjectExplorerPlugin::testDeviceManager()
{
    TestDevice::Ptr dev = IDevice::Ptr(new TestDevice);
    dev->setDisplayName(QLatin1String("blubbdiblubbfurz!"));
    QVERIFY(dev->isAutoDetected());
    QCOMPARE(dev->deviceState(), IDevice::DeviceStateUnknown);
    QCOMPARE(dev->type(), TestDevice::testTypeId());

    TestDevice::Ptr dev2 = dev->clone();
    QCOMPARE(dev->id(), dev2->id());

    DeviceManager * const mgr = DeviceManager::instance();
    QVERIFY(!mgr->find(dev->id()));
    const int oldDeviceCount = mgr->deviceCount();

    QSignalSpy deviceAddedSpy(mgr, SIGNAL(deviceAdded(Core::Id)));
    QSignalSpy deviceRemovedSpy(mgr, SIGNAL(deviceRemoved(Core::Id)));
    QSignalSpy deviceUpdatedSpy(mgr, SIGNAL(deviceUpdated(Core::Id)));
    QSignalSpy deviceListChangedSpy(mgr, SIGNAL(deviceListChanged()));
    QSignalSpy updatedSpy(mgr, SIGNAL(updated()));

    mgr->addDevice(dev);
    QCOMPARE(mgr->deviceCount(), oldDeviceCount + 1);
    QVERIFY(mgr->find(dev->id()));
    QVERIFY(mgr->hasDevice(dev->displayName()));
    QCOMPARE(deviceAddedSpy.count(), 1);
    QCOMPARE(deviceRemovedSpy.count(), 0);
    QCOMPARE(deviceUpdatedSpy.count(), 0);
    QCOMPARE(deviceListChangedSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 1);
    deviceAddedSpy.clear();
    updatedSpy.clear();

    mgr->setDeviceState(dev->id(), IDevice::DeviceStateUnknown);
    QCOMPARE(deviceAddedSpy.count(), 0);
    QCOMPARE(deviceRemovedSpy.count(), 0);
    QCOMPARE(deviceUpdatedSpy.count(), 0);
    QCOMPARE(deviceListChangedSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 0);

    mgr->setDeviceState(dev->id(), IDevice::DeviceReadyToUse);
    QCOMPARE(mgr->find(dev->id())->deviceState(), IDevice::DeviceReadyToUse);
    QCOMPARE(deviceAddedSpy.count(), 0);
    QCOMPARE(deviceRemovedSpy.count(), 0);
    QCOMPARE(deviceUpdatedSpy.count(), 1);
    QCOMPARE(deviceListChangedSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 1);
    deviceUpdatedSpy.clear();
    updatedSpy.clear();

    mgr->addDevice(dev2);
    QCOMPARE(mgr->deviceCount(), oldDeviceCount + 1);
    QVERIFY(mgr->find(dev->id()));
    QCOMPARE(deviceAddedSpy.count(), 0);
    QCOMPARE(deviceRemovedSpy.count(), 0);
    QCOMPARE(deviceUpdatedSpy.count(), 1);
    QCOMPARE(deviceListChangedSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 1);
    deviceUpdatedSpy.clear();
    updatedSpy.clear();

    TestDevice::Ptr dev3 = IDevice::Ptr(new TestDevice);
    QVERIFY(dev->id() != dev3->id());

    dev3->setDisplayName(dev->displayName());
    mgr->addDevice(dev3);
    QCOMPARE(mgr->deviceAt(mgr->deviceCount() - 1)->displayName(),
             QString(dev3->displayName() + QLatin1String("2")));
    QCOMPARE(deviceAddedSpy.count(), 1);
    QCOMPARE(deviceRemovedSpy.count(), 0);
    QCOMPARE(deviceUpdatedSpy.count(), 0);
    QCOMPARE(deviceListChangedSpy.count(), 0);
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
    QCOMPARE(deviceListChangedSpy.count(), 0);
    QCOMPARE(updatedSpy.count(), 2);
}

} // namespace ProjectExplorer

#endif // WITH_TESTS
