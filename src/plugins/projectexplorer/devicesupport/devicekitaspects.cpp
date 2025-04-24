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
#include "../toolchainkitaspect.h"
#include "../windowsappsdksettings.h"

#include <utils/environment.h>
#include <utils/id.h>
#include <utils/layoutbuilder.h>
#include <utils/listmodel.h>
#include <utils/macroexpander.h>

#include <QComboBox>
#include <QHBoxLayout>

#include <tuple>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

template <typename Aspect> class DeviceTypeKitAspectImpl final : public KitAspect
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
        auto getter = [](const Kit &k) { return Aspect::deviceTypeId(&k).toSetting(); };
        auto setter = [](Kit &k, const QVariant &type) {
            Aspect::setDeviceTypeId(&k, Id::fromSetting(type));
        };
        auto resetModel = [model] {
            model->clear();
            for (IDeviceFactory *factory : IDeviceFactory::allDeviceFactories()) {
                model->appendItem(
                    std::make_tuple(factory->displayName(), factory->deviceType(), factory->icon()));
            }
        };
        addListAspectSpec(
            {model, std::move(getter), std::move(setter), std::move(resetModel)});
    }
};

template <typename TypeAspect, typename DeviceAspect>
class DeviceKitAspectImpl final : public KitAspect
{
public:
    DeviceKitAspectImpl(Kit *workingCopy, const KitAspectFactory *factory)
        : KitAspect(workingCopy, factory)
    {
        setManagingPage(Constants::DEVICE_SETTINGS_PAGE_ID);

        const auto model = new DeviceManagerModel(this);
        auto getter = [](const Kit &k) {
            auto device = DeviceAspect::device(&k);
            return device ? device->id().toSetting() : QVariant{};
        };
        auto setter = [](Kit &k, const QVariant &id) {
            DeviceAspect::setDeviceId(&k, Id::fromSetting(id));
        };
        auto resetModel = [this, model] {
            model->setTypeFilter(TypeAspect::deviceTypeId(kit()));
        };
        addListAspectSpec({model, std::move(getter), std::move(setter), std::move(resetModel)});

        connect(DeviceManager::instance(), &DeviceManager::updated,
                this, &DeviceKitAspectImpl::refresh);
    }

private:
    Id settingsPageItemToPreselect() const override { return DeviceAspect::deviceId(kit()); }

    void addToInnerLayout(Layouting::Layout &layout) override
    {
        if (const QList<KitAspect *> embedded = aspectsToEmbed(); !embedded.isEmpty()) {
            Layouting::Layout box(new QHBoxLayout);
            box.addItem(createSubWidget<QLabel>(Tr::tr("Type:")));
            embedded.first()->addToInnerLayout(box);
            box.addItem(createSubWidget<QLabel>(Tr::tr("Device:")));
            KitAspect::addToInnerLayout(box);
            QSizePolicy p = comboBoxes().first()->sizePolicy();
            p.setHorizontalStretch(1);
            comboBoxes().first()->setSizePolicy(p);
            layout.addItem(box);
        } else {
            KitAspect::addToInnerLayout(layout);
        }
    }
};

template <typename DeviceTypeKitAspect>
class DeviceTypeKitAspectFactory : public KitAspectFactory
{
public:
    DeviceTypeKitAspectFactory()
    {
        setId(DeviceTypeKitAspect::id());
        makeEssential();
    }

    void setup(Kit *k) override
    {
        if (k && !k->hasValue(id()))
            k->setValue(id(), QByteArray(Constants::DESKTOP_DEVICE_TYPE));
    }

    KitAspect *createKitAspect(Kit *k) const override
    {
        QTC_ASSERT(k, return nullptr);
        return new DeviceTypeKitAspectImpl<DeviceTypeKitAspect>(k, this);
    }

    ItemList toUserOutput(const Kit *k) const override
    {
        QTC_ASSERT(k, return {});
        const Id type = DeviceTypeKitAspect::deviceTypeId(k);
        QString typeDisplayName = Tr::tr("Unknown device type");
        if (type.isValid()) {
            if (IDeviceFactory *factory = IDeviceFactory::find(type))
                typeDisplayName = factory->displayName();
        }
        return {{Tr::tr("Device type"), typeDisplayName}};
    }

    QSet<Id> availableFeatures(const Kit *k) const override
    {
        if (const Id id = DeviceTypeKitAspect::deviceTypeId(k); id.isValid())
            return {id.withPrefix("DeviceType.")};
        return {};
    }

    QSet<Id> supportedPlatforms(const Kit *k) const override
    {
        return {DeviceTypeKitAspect::deviceTypeId(k)};
    }

    Tasks validate(const Kit *) const override { return {}; }
};

template <typename TypeAspect, typename DeviceAspect>
class DeviceKitAspectFactory : public KitAspectFactory
{
public:
    DeviceKitAspectFactory(const QByteArray &varPrefix) : m_varPrefix(varPrefix)
    {
        setId(DeviceAspect::id());
        setEmbeddableAspects({TypeAspect::id()});
    }

    static Id defaultValue(const Kit *k)
    {
        if (const IDeviceConstPtr dev = DeviceManager::defaultDevice(TypeAspect::deviceTypeId(k)))
            return dev->id();
        return {};
    }

private:
    static bool isCompatible(const IDevice::ConstPtr &dev, const Kit *k)
    {
        return dev->type() == TypeAspect::deviceTypeId(k);
    }

    Tasks validate(const Kit *k) const override
    {
        const auto noDeviceMsg = [] {
            if constexpr (std::is_same_v<TypeAspect, BuildDeviceTypeKitAspect>)
                return Tr::tr("No build device set.");
            if constexpr (std::is_same_v<TypeAspect, RunDeviceTypeKitAspect>)
                return Tr::tr("No run device set.");
        };
        const auto incompatibleDeviceMsg = [] {
            if constexpr (std::is_same_v<TypeAspect, BuildDeviceTypeKitAspect>)
                return Tr::tr("Build device is incompatible with this kit.");
            if constexpr (std::is_same_v<TypeAspect, RunDeviceTypeKitAspect>)
                return Tr::tr("Run device is incompatible with this kit.");
        };
        const IDevice::ConstPtr dev = DeviceAspect::device(k);
        Tasks result;
        if (!dev)
            result.append(BuildSystemTask(Task::Warning, noDeviceMsg()));
        else if (!isCompatible(dev, k)) // FIXME: Impossible?
            result.append(BuildSystemTask(Task::Error, incompatibleDeviceMsg()));
        if (dev)
            result.append(dev->validate());
        return result;
    }

    void fix(Kit *k) override
    {
        const IDevice::ConstPtr dev = DeviceAspect::device(k);
        if (dev && !isCompatible(dev, k))
            DeviceAspect::setDeviceId(k, defaultValue(k));
    }

    void setup(Kit *k) override
    {
        QTC_ASSERT(DeviceManager::instance(), return);
        if (const IDevice::ConstPtr dev = DeviceAspect::device(k); dev && isCompatible(dev, k))
            return;
        DeviceAspect::setDeviceId(k, defaultValue(k));
    }

    KitAspect *createKitAspect(Kit *k) const override
    {
        QTC_ASSERT(k, return nullptr);
        return new DeviceKitAspectImpl<TypeAspect, DeviceAspect>(k, this);
    }

    QString displayNamePostfix(const Kit *k) const override
    {
        if (const IDevice::ConstPtr dev = DeviceAspect::device(k))
            return dev->displayName();
        return {};
    }

    ItemList toUserOutput(const Kit *k) const override
    {
        const IDevice::ConstPtr dev = DeviceAspect::device(k);
        return {{displayName(), dev ? dev->displayName() : Tr::tr("Unconfigured") }};
    }

    QByteArray varName(const QByteArray &suffix) const
    {
        return m_varPrefix + ':' + suffix;
    }

    QString varDescription(const QString &pattern) const
    {
        return pattern.arg(displayName());
    }

    void addToMacroExpander(Kit *kit, MacroExpander *expander) const override
    {
        QTC_ASSERT(kit, return);
        expander->registerVariable(
            varName("HostAddress"), varDescription(Tr::tr("Host address (%1)")), [kit] {
                const IDevice::ConstPtr device = DeviceAspect::device(kit);
                return device ? device->sshParameters().host() : QString();
            });
        expander
            ->registerVariable(varName("SshPort"), varDescription(Tr::tr("SSH port (%1)")), [kit] {
                const IDevice::ConstPtr device = DeviceAspect::device(kit);
                return device ? QString::number(device->sshParameters().port()) : QString();
            });
        expander
            ->registerVariable(varName("UserName"), varDescription(Tr::tr("User name (%1)")), [kit] {
                const IDevice::ConstPtr device = DeviceAspect::device(kit);
                return device ? device->sshParameters().userName() : QString();
            });
        expander->registerVariable(
            varName("KeyFile"), varDescription(Tr::tr("Private key file (%1)")), [kit] {
                const IDevice::ConstPtr device = DeviceAspect::device(kit);
                return device ? device->sshParameters().privateKeyFile().toUrlishString() : QString();
            });
        expander
            ->registerVariable(varName("Name"), varDescription(Tr::tr("Device name (%1)")), [kit] {
                const IDevice::ConstPtr device = DeviceAspect::device(kit);
                return device ? device->displayName() : QString();
            });
        expander->registerFileVariables(
            varName("Root"), varDescription(Tr::tr("Device root directory (%1)")), [kit] {
                const IDevice::ConstPtr device = DeviceAspect::device(kit);
                return device ? device->rootPath() : FilePath{};
            });
    }

    void onKitsLoaded() override
    {
        for (Kit *k : KitManager::kits())
            fix(k);

        DeviceManager *dm = DeviceManager::instance();
        connect(dm, &DeviceManager::deviceAdded, this, &DeviceKitAspectFactory::devicesChanged);
        connect(dm, &DeviceManager::deviceRemoved, this, &DeviceKitAspectFactory::devicesChanged);
        connect(dm, &DeviceManager::deviceUpdated, this, &DeviceKitAspectFactory::deviceUpdated);

        connect(KitManager::instance(), &KitManager::kitUpdated,
                this, &DeviceKitAspectFactory::setup);
        connect(KitManager::instance(), &KitManager::unmanagedKitUpdated,
                this, &DeviceKitAspectFactory::setup);
    }

    void deviceUpdated(Id id)
    {
        for (Kit *k : KitManager::kits()) {
            if (DeviceAspect::deviceId(k) == id)
                notifyAboutUpdate(k);
        }
    }

    void devicesChanged()
    {
        for (Kit *k : KitManager::kits())
            setup(k); // Set default device if necessary
    }

    const QByteArray m_varPrefix;
};

// --------------------------------------------------------------------------
// RunDeviceTypeKitAspect:
// --------------------------------------------------------------------------
class RunDeviceTypeKitAspectFactory : public DeviceTypeKitAspectFactory<RunDeviceTypeKitAspect>
{
public:
    RunDeviceTypeKitAspectFactory()
    {
        setPriority(33000);
        setDisplayName(Tr::tr("Run device type"));
        setDescription(Tr::tr("The type of device to run applications on."));
    }
};

const RunDeviceTypeKitAspectFactory theRunDeviceTypeKitAspectFactory;

} // namespace Internal

const Id RunDeviceTypeKitAspect::id()
{
    return "PE.Profile.DeviceType";
}

const Id RunDeviceTypeKitAspect::deviceTypeId(const Kit *k)
{
    if (!k)
        return {};
    if (const Id theId = Id::fromSetting(k->value(id())); theId.isValid())
        return theId;
    return Constants::DESKTOP_DEVICE_TYPE;
}

void RunDeviceTypeKitAspect::setDeviceTypeId(Kit *k, Id type)
{
    QTC_ASSERT(k, return);
    k->setValue(id(), type.toSetting());
}

// --------------------------------------------------------------------------
// RunDeviceKitAspect:
// --------------------------------------------------------------------------
namespace Internal {

class RunDeviceKitAspectFactory
    : public DeviceKitAspectFactory<RunDeviceTypeKitAspect, RunDeviceKitAspect>
{
public:
    RunDeviceKitAspectFactory() : DeviceKitAspectFactory("Device")
    {
        setDisplayName(Tr::tr("Run device"));
        setDescription(Tr::tr("The device to run the applications on."));
        setPriority(31899);
    }
};

const RunDeviceKitAspectFactory theDeviceKitAspectFactory;

} // namespace Internal


Id RunDeviceKitAspect::id()
{
    return "PE.Profile.Device";
}

IDevice::ConstPtr RunDeviceKitAspect::device(const Kit *k)
{
    QTC_ASSERT(DeviceManager::isLoaded(), return IDevice::ConstPtr());
    return DeviceManager::find(deviceId(k));
}

Id RunDeviceKitAspect::deviceId(const Kit *k)
{
    if (!k)
        return {};
    if (const Id theId = Id::fromSetting(k->value(id())); theId.isValid())
        return theId;
    return Internal::RunDeviceKitAspectFactory::defaultValue(k);
}

void RunDeviceKitAspect::setDevice(Kit *k, IDevice::ConstPtr dev)
{
    setDeviceId(k, dev ? dev->id() : Id());
}

void RunDeviceKitAspect::setDeviceId(Kit *k, Id id)
{
    QTC_ASSERT(k, return);
    k->setValue(RunDeviceKitAspect::id(), id.toSetting());
}

FilePath RunDeviceKitAspect::deviceFilePath(const Kit *k, const QString &pathOnDevice)
{
    if (IDevice::ConstPtr dev = device(k))
        return dev->filePath(pathOnDevice);
    return FilePath::fromString(pathOnDevice);
}

// --------------------------------------------------------------------------
// BuildDeviceTypeKitAspect:
// --------------------------------------------------------------------------
namespace Internal {

class BuildDeviceTypeKitAspectFactory : public DeviceTypeKitAspectFactory<BuildDeviceTypeKitAspect>
{
public:
    BuildDeviceTypeKitAspectFactory()
    {
        setDisplayName(Tr::tr("Build device type"));
        setDescription(Tr::tr("The type of device to build on."));
        setPriority(32000);
    }

private:
    // QtC < 16 did not have a build device type, but the user might have set the build device.
    void upgrade(Kit *k)
    {
        if (!Id::fromSetting(k->value(id())).isValid()) {
            if (const IDevice::ConstPtr dev = BuildDeviceKitAspect::device(k))
                BuildDeviceTypeKitAspect::setDeviceTypeId(k, dev->type());
        }
    }
};

const BuildDeviceTypeKitAspectFactory theBuildDeviceTypeKitAspectFactory;

} // namespace Internal

Id BuildDeviceTypeKitAspect::id()
{
    return "PE.Profile.BuildDeviceType";
}

Id BuildDeviceTypeKitAspect::deviceTypeId(const Kit *k)
{
    if (!k)
        return {};
    if (const Id theId = Id::fromSetting(k->value(id())); theId.isValid())
        return theId;
    return Constants::DESKTOP_DEVICE_TYPE;
}

void BuildDeviceTypeKitAspect::setDeviceTypeId(Kit *k, Utils::Id type)
{
    QTC_ASSERT(k, return);
    k->setValue(id(), type.toSetting());
}

// --------------------------------------------------------------------------
// BuildDeviceKitAspect:
// --------------------------------------------------------------------------
namespace Internal {
class BuildDeviceKitAspectFactory
    : public DeviceKitAspectFactory<BuildDeviceTypeKitAspect, BuildDeviceKitAspect>
{
public:
    BuildDeviceKitAspectFactory() : DeviceKitAspectFactory("BuildDevice")
    {
        setDisplayName(Tr::tr("Build device"));
        setDescription(Tr::tr("The device used to build applications on."));
        setPriority(31900);
    }

private:
    void addToBuildEnvironment(const Kit *k, Utils::Environment &env) const override
    {
        IDevice::ConstPtr dev = BuildDeviceKitAspect::device(k);
        if (!dev)
            return;
        if (dev->osType() == OsType::OsTypeWindows
            && dev->type() == Constants::DESKTOP_DEVICE_TYPE) {
            if (const FilePath appSdkLocation = windowsAppSdkSettings().windowsAppSdkLocation();
                !appSdkLocation.isEmpty()) {
                env.set(Constants::WINDOWS_WINAPPSDK_ROOT_ENV_KEY, appSdkLocation.path());
            }
        }
    }
};

const BuildDeviceKitAspectFactory theBuildDeviceKitAspectFactory;

} // namespace Internal

Id BuildDeviceKitAspect::id()
{
    return "PE.Profile.BuildDevice";
}

IDevice::ConstPtr BuildDeviceKitAspect::device(const Kit *k)
{
    QTC_ASSERT(DeviceManager::isLoaded(), return IDevice::ConstPtr());
    return DeviceManager::find(deviceId(k));
}

Id BuildDeviceKitAspect::deviceId(const Kit *k)
{
    if (!k)
        return {};
    if (const Id theId = Id::fromSetting(k->value(id())); theId.isValid())
        return theId;
    return Internal::BuildDeviceKitAspectFactory::defaultValue(k);
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
