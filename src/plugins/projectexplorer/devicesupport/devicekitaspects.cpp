// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicekitaspects.h"

#include "devicemanager.h"
#include "devicemanagermodel.h"
#include "idevicefactory.h"
#include "sshparameters.h"
#include "../kit.h"
#include "../kitaspect.h"
#include "../kitmanager.h"
#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"

#include <utils/id.h>
#include <utils/layoutbuilder.h>
#include <utils/listmodel.h>
#include <utils/macroexpander.h>

#include <QComboBox>

#include <tuple>

using namespace Utils;

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// DeviceTypeKitAspect:
// --------------------------------------------------------------------------
namespace Internal {
class DeviceTypeKitAspectImpl final : public KitAspect
{
public:
    DeviceTypeKitAspectImpl(Kit *workingCopy, const KitAspectFactory *factory)
        : KitAspect(workingCopy, factory)
    {
        using ItemData = std::tuple<QString, Id, QIcon>;
        const auto model = new ListModel<ItemData>(this);
        model->setDataAccessor([](const ItemData &d, int column, int role) -> QVariant {
            if (column != 0)
                return {};
            if (role == Qt::DisplayRole)
                return std::get<0>(d);
            if (role == KitAspect::IdRole)
                return std::get<1>(d).toSetting();
            if (role == Qt::DecorationRole)
                return std::get<2>(d);
            return {};
        });
        auto getter = [](const Kit &k) { return DeviceTypeKitAspect::deviceTypeId(&k).toSetting(); };
        auto setter = [](Kit &k, const QVariant &type) {
            DeviceTypeKitAspect::setDeviceTypeId(&k, Id::fromSetting(type));
        };
        auto resetModel = [model] {
            model->clear();
            for (IDeviceFactory *factory : IDeviceFactory::allDeviceFactories()) {
                model->appendItem(
                    std::make_tuple(factory->displayName(), factory->deviceType(), factory->icon()));
            }
        };
        setListAspectSpec(
            {model, std::move(getter), std::move(setter), std::move(resetModel)});
    }
};

class DeviceTypeKitAspectFactory : public KitAspectFactory
{
public:
    DeviceTypeKitAspectFactory();

    void setup(Kit *k) override;
    Tasks validate(const Kit *k) const override;
    KitAspect *createKitAspect(Kit *k) const override;
    ItemList toUserOutput(const Kit *k) const override;

    QSet<Id> supportedPlatforms(const Kit *k) const override;
    QSet<Id> availableFeatures(const Kit *k) const override;
};

DeviceTypeKitAspectFactory::DeviceTypeKitAspectFactory()
{
    setId(DeviceTypeKitAspect::id());
    setDisplayName(Tr::tr("Run device type"));
    setDescription(Tr::tr("The type of device to run applications on."));
    setPriority(33000);
    makeEssential();
}

void DeviceTypeKitAspectFactory::setup(Kit *k)
{
    if (k && !k->hasValue(id()))
        k->setValue(id(), QByteArray(Constants::DESKTOP_DEVICE_TYPE));
}

Tasks DeviceTypeKitAspectFactory::validate(const Kit *k) const
{
    Q_UNUSED(k)
    return {};
}

KitAspect *DeviceTypeKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new Internal::DeviceTypeKitAspectImpl(k, this);
}

KitAspectFactory::ItemList DeviceTypeKitAspectFactory::toUserOutput(const Kit *k) const
{
    QTC_ASSERT(k, return {});
    Id type = DeviceTypeKitAspect::deviceTypeId(k);
    QString typeDisplayName = Tr::tr("Unknown device type");
    if (type.isValid()) {
        if (IDeviceFactory *factory = IDeviceFactory::find(type))
            typeDisplayName = factory->displayName();
    }
    return {{Tr::tr("Device type"), typeDisplayName}};
}

QSet<Id> DeviceTypeKitAspectFactory::supportedPlatforms(const Kit *k) const
{
    return {DeviceTypeKitAspect::deviceTypeId(k)};
}

QSet<Id> DeviceTypeKitAspectFactory::availableFeatures(const Kit *k) const
{
    Id id = DeviceTypeKitAspect::deviceTypeId(k);
    if (id.isValid())
        return {id.withPrefix("DeviceType.")};
    return {};
}

const DeviceTypeKitAspectFactory theDeviceTypeKitAspectFactory;
} // namespace Internal

const Id DeviceTypeKitAspect::id()
{
    return "PE.Profile.DeviceType";
}

const Id DeviceTypeKitAspect::deviceTypeId(const Kit *k)
{
    return k ? Id::fromSetting(k->value(DeviceTypeKitAspect::id())) : Id();
}

void DeviceTypeKitAspect::setDeviceTypeId(Kit *k, Id type)
{
    QTC_ASSERT(k, return);
    k->setValue(DeviceTypeKitAspect::id(), type.toSetting());
}

// --------------------------------------------------------------------------
// DeviceKitAspect:
// --------------------------------------------------------------------------
namespace Internal {
class DeviceKitAspectImpl final : public KitAspect
{
public:
    DeviceKitAspectImpl(Kit *workingCopy, const KitAspectFactory *factory)
        : KitAspect(workingCopy, factory)
    {
        setManagingPage(Constants::DEVICE_SETTINGS_PAGE_ID);

        const auto model = new DeviceManagerModel(DeviceManager::instance(), this);
        auto getter = [](const Kit &k) {
            auto device = DeviceKitAspect::device(&k);
            return device ? device->id().toSetting() : QVariant{};
        };
        auto setter = [](Kit &k, const QVariant &id) {
            DeviceKitAspect::setDeviceId(&k, Id::fromSetting(id));
        };
        auto resetModel = [this, model] {
            model->setTypeFilter(DeviceTypeKitAspect::deviceTypeId(kit()));
        };
        setListAspectSpec({model, std::move(getter), std::move(setter), std::move(resetModel)});

        connect(DeviceManager::instance(), &DeviceManager::updated,
                this, &DeviceKitAspectImpl::refresh);
    }

private:
    Id settingsPageItemToPreselect() const override { return DeviceKitAspect::deviceId(kit()); }
};

class DeviceKitAspectFactory : public KitAspectFactory
{
public:
    DeviceKitAspectFactory();

private:
    Tasks validate(const Kit *k) const override;
    void fix(Kit *k) override;
    void setup(Kit *k) override;

    KitAspect *createKitAspect(Kit *k) const override;

    QString displayNamePostfix(const Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    void addToMacroExpander(Kit *kit, MacroExpander *expander) const override;

    QVariant defaultValue(const Kit *k) const;

    void onKitsLoaded() override;
    void deviceUpdated(Id dataId);
    void devicesChanged();
    void kitUpdated(Kit *k);
};

DeviceKitAspectFactory::DeviceKitAspectFactory()
{
    setId(DeviceKitAspect::id());
    setDisplayName(Tr::tr("Run device"));
    setDescription(Tr::tr("The device to run the applications on."));
    setPriority(32000);
}

QVariant DeviceKitAspectFactory::defaultValue(const Kit *k) const
{
    Id type = DeviceTypeKitAspect::deviceTypeId(k);
    // Use default device if that is compatible:
    IDevice::ConstPtr dev = DeviceManager::instance()->defaultDevice(type);
    if (dev && dev->isCompatibleWith(k))
        return dev->id().toString();
    // Use any other device that is compatible:
    for (int i = 0; i < DeviceManager::instance()->deviceCount(); ++i) {
        dev = DeviceManager::instance()->deviceAt(i);
        if (dev && dev->isCompatibleWith(k))
            return dev->id().toString();
    }
    // Fail: No device set up.
    return {};
}

Tasks DeviceKitAspectFactory::validate(const Kit *k) const
{
    IDevice::ConstPtr dev = DeviceKitAspect::device(k);
    Tasks result;
    if (!dev)
        result.append(BuildSystemTask(Task::Warning, Tr::tr("No device set.")));
    else if (!dev->isCompatibleWith(k))
        result.append(BuildSystemTask(Task::Error, Tr::tr("Device is incompatible with this kit.")));

    if (dev)
        result.append(dev->validate());

    return result;
}

void DeviceKitAspectFactory::fix(Kit *k)
{
    IDevice::ConstPtr dev = DeviceKitAspect::device(k);
    if (dev && !dev->isCompatibleWith(k)) {
        qWarning("Device is no longer compatible with kit \"%s\", removing it.",
                 qPrintable(k->displayName()));
        DeviceKitAspect::setDeviceId(k, Id());
    }
}

void DeviceKitAspectFactory::setup(Kit *k)
{
    QTC_ASSERT(DeviceManager::instance()->isLoaded(), return);
    IDevice::ConstPtr dev = DeviceKitAspect::device(k);
    if (dev && dev->isCompatibleWith(k))
        return;

    DeviceKitAspect::setDeviceId(k, Id::fromSetting(defaultValue(k)));
}

KitAspect *DeviceKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new Internal::DeviceKitAspectImpl(k, this);
}

QString DeviceKitAspectFactory::displayNamePostfix(const Kit *k) const
{
    IDevice::ConstPtr dev = DeviceKitAspect::device(k);
    return dev ? dev->displayName() : QString();
}

KitAspectFactory::ItemList DeviceKitAspectFactory::toUserOutput(const Kit *k) const
{
    IDevice::ConstPtr dev = DeviceKitAspect::device(k);
    return {{Tr::tr("Device"), dev ? dev->displayName() : Tr::tr("Unconfigured") }};
}

void DeviceKitAspectFactory::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    QTC_ASSERT(kit, return);
    expander->registerVariable("Device:HostAddress", Tr::tr("Host address"), [kit] {
        const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
        return device ? device->sshParameters().host() : QString();
    });
    expander->registerVariable("Device:SshPort", Tr::tr("SSH port"), [kit] {
        const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
        return device ? QString::number(device->sshParameters().port()) : QString();
    });
    expander->registerVariable("Device:UserName", Tr::tr("User name"), [kit] {
        const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
        return device ? device->sshParameters().userName() : QString();
    });
    expander->registerVariable("Device:KeyFile", Tr::tr("Private key file"), [kit] {
        const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
        return device ? device->sshParameters().privateKeyFile.toString() : QString();
    });
    expander->registerVariable("Device:Name", Tr::tr("Device name"), [kit] {
        const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
        return device ? device->displayName() : QString();
    });
    expander->registerFileVariables("Device::Root", Tr::tr("Device root directory"), [kit] {
        const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
        return device ? device->rootPath() : FilePath{};
    });
}

void DeviceKitAspectFactory::onKitsLoaded()
{
    for (Kit *k : KitManager::kits())
        fix(k);

    DeviceManager *dm = DeviceManager::instance();
    connect(dm, &DeviceManager::deviceListReplaced, this, &DeviceKitAspectFactory::devicesChanged);
    connect(dm, &DeviceManager::deviceAdded, this, &DeviceKitAspectFactory::devicesChanged);
    connect(dm, &DeviceManager::deviceRemoved, this, &DeviceKitAspectFactory::devicesChanged);
    connect(dm, &DeviceManager::deviceUpdated, this, &DeviceKitAspectFactory::deviceUpdated);

    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &DeviceKitAspectFactory::kitUpdated);
    connect(KitManager::instance(), &KitManager::unmanagedKitUpdated,
            this, &DeviceKitAspectFactory::kitUpdated);
}

void DeviceKitAspectFactory::deviceUpdated(Id id)
{
    for (Kit *k : KitManager::kits()) {
        if (DeviceKitAspect::deviceId(k) == id)
            notifyAboutUpdate(k);
    }
}

void DeviceKitAspectFactory::kitUpdated(Kit *k)
{
    setup(k); // Set default device if necessary
}

void DeviceKitAspectFactory::devicesChanged()
{
    for (Kit *k : KitManager::kits())
        setup(k); // Set default device if necessary
}

const DeviceKitAspectFactory theDeviceKitAspectFactory;

} // namespace Internal


Id DeviceKitAspect::id()
{
    return "PE.Profile.Device";
}

IDevice::ConstPtr DeviceKitAspect::device(const Kit *k)
{
    QTC_ASSERT(DeviceManager::instance()->isLoaded(), return IDevice::ConstPtr());
    return DeviceManager::instance()->find(deviceId(k));
}

Id DeviceKitAspect::deviceId(const Kit *k)
{
    return k ? Id::fromSetting(k->value(DeviceKitAspect::id())) : Id();
}

void DeviceKitAspect::setDevice(Kit *k, IDevice::ConstPtr dev)
{
    setDeviceId(k, dev ? dev->id() : Id());
}

void DeviceKitAspect::setDeviceId(Kit *k, Id id)
{
    QTC_ASSERT(k, return);
    k->setValue(DeviceKitAspect::id(), id.toSetting());
}

FilePath DeviceKitAspect::deviceFilePath(const Kit *k, const QString &pathOnDevice)
{
    if (IDevice::ConstPtr dev = device(k))
        return dev->filePath(pathOnDevice);
    return FilePath::fromString(pathOnDevice);
}

// --------------------------------------------------------------------------
// BuildDeviceKitAspect:
// --------------------------------------------------------------------------
namespace Internal {
class BuildDeviceKitAspectImpl final : public KitAspect
{
public:
    BuildDeviceKitAspectImpl(Kit *workingCopy, const KitAspectFactory *factory)
        : KitAspect(workingCopy, factory)
    {
        setManagingPage(Constants::DEVICE_SETTINGS_PAGE_ID);

        const auto model = new DeviceManagerModel(DeviceManager::instance(), this);
        auto getter = [](const Kit &k) {
            return BuildDeviceKitAspect::device(&k)->id().toSetting();
        };
        auto setter = [](Kit &k, const QVariant &id) {
            BuildDeviceKitAspect::setDeviceId(&k, Id::fromSetting(id));
        };
        auto resetModel = [model] {
            QList<Id> blackList;
            const DeviceManager *dm = DeviceManager::instance();
            for (int i = 0; i < dm->deviceCount(); ++i) {
                IDevice::ConstPtr device = dm->deviceAt(i);
                if (!device->usableAsBuildDevice())
                    blackList.append(device->id());
            }
            model->setFilter(blackList);
        };
        setListAspectSpec({model, std::move(getter), std::move(setter), std::move(resetModel)});

        connect(DeviceManager::instance(), &DeviceManager::updated,
                this, &BuildDeviceKitAspectImpl::refresh);
    }
};

class BuildDeviceKitAspectFactory : public KitAspectFactory
{
public:
    BuildDeviceKitAspectFactory();

private:
    void setup(Kit *k) override;
    Tasks validate(const Kit *k) const override;

    KitAspect *createKitAspect(Kit *k) const override;

    QString displayNamePostfix(const Kit *k) const override;

    ItemList toUserOutput(const Kit *k) const override;

    void addToMacroExpander(Kit *kit, MacroExpander *expander) const override;

    void onKitsLoaded() override;
    void deviceUpdated(Id dataId);
    void devicesChanged();
    void kitUpdated(Kit *k);
};

BuildDeviceKitAspectFactory::BuildDeviceKitAspectFactory()
{
    setId(BuildDeviceKitAspect::id());
    setDisplayName(Tr::tr("Build device"));
    setDescription(Tr::tr("The device used to build applications on."));
    setPriority(31900);
}

static IDeviceConstPtr defaultDevice()
{
    return DeviceManager::defaultDesktopDevice();
}

void BuildDeviceKitAspectFactory::setup(Kit *k)
{
    QTC_ASSERT(DeviceManager::instance()->isLoaded(), return );
    IDevice::ConstPtr dev = BuildDeviceKitAspect::device(k);
    if (dev)
        return;

    dev = defaultDevice();
    BuildDeviceKitAspect::setDeviceId(k, dev ? dev->id() : Id());
}

Tasks BuildDeviceKitAspectFactory::validate(const Kit *k) const
{
    IDevice::ConstPtr dev = BuildDeviceKitAspect::device(k);
    Tasks result;
    if (!dev)
        result.append(BuildSystemTask(Task::Warning, Tr::tr("No build device set.")));

    return result;
}

KitAspect *BuildDeviceKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new Internal::BuildDeviceKitAspectImpl(k, this);
}

QString BuildDeviceKitAspectFactory::displayNamePostfix(const Kit *k) const
{
    IDevice::ConstPtr dev = BuildDeviceKitAspect::device(k);
    return dev ? dev->displayName() : QString();
}

KitAspectFactory::ItemList BuildDeviceKitAspectFactory::toUserOutput(const Kit *k) const
{
    IDevice::ConstPtr dev = BuildDeviceKitAspect::device(k);
    return {{Tr::tr("Build device"), dev ? dev->displayName() : Tr::tr("Unconfigured")}};
}

void BuildDeviceKitAspectFactory::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    QTC_ASSERT(kit, return);
    expander->registerVariable("BuildDevice:HostAddress", Tr::tr("Build host address"), [kit] {
        const IDevice::ConstPtr device = BuildDeviceKitAspect::device(kit);
        return device ? device->sshParameters().host() : QString();
    });
    expander->registerVariable("BuildDevice:SshPort", Tr::tr("Build SSH port"), [kit] {
        const IDevice::ConstPtr device = BuildDeviceKitAspect::device(kit);
        return device ? QString::number(device->sshParameters().port()) : QString();
    });
    expander->registerVariable("BuildDevice:UserName", Tr::tr("Build user name"), [kit] {
        const IDevice::ConstPtr device = BuildDeviceKitAspect::device(kit);
        return device ? device->sshParameters().userName() : QString();
    });
    expander->registerVariable("BuildDevice:KeyFile", Tr::tr("Build private key file"), [kit] {
        const IDevice::ConstPtr device = BuildDeviceKitAspect::device(kit);
        return device ? device->sshParameters().privateKeyFile.toString() : QString();
    });
    expander->registerVariable("BuildDevice:Name", Tr::tr("Build device name"), [kit] {
        const IDevice::ConstPtr device = BuildDeviceKitAspect::device(kit);
        return device ? device->displayName() : QString();
    });
    expander
        ->registerFileVariables("BuildDevice::Root", Tr::tr("Build device root directory"), [kit] {
            const IDevice::ConstPtr device = BuildDeviceKitAspect::device(kit);
            return device ? device->rootPath() : FilePath{};
        });
}

void BuildDeviceKitAspectFactory::onKitsLoaded()
{
    for (Kit *k : KitManager::kits())
        fix(k);

    DeviceManager *dm = DeviceManager::instance();
    connect(dm, &DeviceManager::deviceListReplaced,
            this, &BuildDeviceKitAspectFactory::devicesChanged);
    connect(dm, &DeviceManager::deviceAdded,
            this, &BuildDeviceKitAspectFactory::devicesChanged);
    connect(dm, &DeviceManager::deviceRemoved,
            this, &BuildDeviceKitAspectFactory::devicesChanged);
    connect(dm, &DeviceManager::deviceUpdated,
            this, &BuildDeviceKitAspectFactory::deviceUpdated);
    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &BuildDeviceKitAspectFactory::kitUpdated);
    connect(KitManager::instance(), &KitManager::unmanagedKitUpdated,
            this, &BuildDeviceKitAspectFactory::kitUpdated);
}

void BuildDeviceKitAspectFactory::deviceUpdated(Id id)
{
    const QList<Kit *> kits = KitManager::kits();
    for (Kit *k : kits) {
        if (BuildDeviceKitAspect::deviceId(k) == id)
            notifyAboutUpdate(k);
    }
}

void BuildDeviceKitAspectFactory::kitUpdated(Kit *k)
{
    setup(k); // Set default device if necessary
}

void BuildDeviceKitAspectFactory::devicesChanged()
{
    const QList<Kit *> kits = KitManager::kits();
    for (Kit *k : kits)
        setup(k); // Set default device if necessary
}

const BuildDeviceKitAspectFactory theBuildDeviceKitAspectFactory;

} // namespace Internal

Id BuildDeviceKitAspect::id()
{
    return "PE.Profile.BuildDevice";
}

IDevice::ConstPtr BuildDeviceKitAspect::device(const Kit *k)
{
    QTC_ASSERT(DeviceManager::instance()->isLoaded(), return IDevice::ConstPtr());
    IDevice::ConstPtr dev = DeviceManager::instance()->find(deviceId(k));
    if (!dev)
        dev = Internal::defaultDevice();
    return dev;
}

Id BuildDeviceKitAspect::deviceId(const Kit *k)
{
    return k ? Id::fromSetting(k->value(BuildDeviceKitAspect::id())) : Id();
}

void BuildDeviceKitAspect::setDevice(Kit *k, IDevice::ConstPtr dev)
{
    setDeviceId(k, dev ? dev->id() : Id());
}

void BuildDeviceKitAspect::setDeviceId(Kit *k, Id id)
{
    QTC_ASSERT(k, return);
    k->setValue(BuildDeviceKitAspect::id(), id.toSetting());
}

} // namespace ProjectExplorer
