// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "kitmanager.h"

#include "abi.h"
#include "devicesupport/devicekitaspects.h"
#include "devicesupport/idevicefactory.h"
#include "kit.h"
#include "kitfeatureprovider.h"
#include "kitaspect.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "toolchainkitaspect.h"
#include "toolchainmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <android/androidconstants.h>
#include <baremetal/baremetalconstants.h>
#include <qnx/qnxconstants.h>
#include <remotelinux/remotelinux_constants.h>

#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/persistentsettings.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <nanotrace/nanotrace.h>

#include <QElapsedTimer>
#include <QHash>

#include <optional>

using namespace Core;
using namespace Utils;
using namespace ProjectExplorer::Internal;

namespace ProjectExplorer {

class KitList
{
public:
    Utils::Id defaultKit;
    std::vector<std::unique_ptr<Kit>> kits;
};

static KitList restoreKitsHelper(const Utils::FilePath &fileName);

namespace Internal {

const char KIT_DATA_KEY[] = "Profile.";
const char KIT_COUNT_KEY[] = "Profile.Count";
const char KIT_FILE_VERSION_KEY[] = "Version";
const char KIT_DEFAULT_KEY[] = "Profile.Default";
const char KIT_IRRELEVANT_ASPECTS_KEY[] = "Kit.IrrelevantAspects";
const char KIT_FILENAME[] = "profiles.xml";

static FilePath settingsFileName()
{
    return ICore::userResourcePath(KIT_FILENAME);
}

// --------------------------------------------------------------------------
// KitManagerPrivate:
// --------------------------------------------------------------------------

class KitManagerPrivate
{
public:
    Kit *m_defaultKit = nullptr;
    bool m_initialized = false;
    std::vector<std::unique_ptr<Kit>> m_kitList;
    std::unique_ptr<PersistentSettingsWriter> m_writer;
    QSet<Id> m_irrelevantAspects;
    FilePath m_binaryForKit;
};

} // namespace Internal

// --------------------------------------------------------------------------
// KitManager:
// --------------------------------------------------------------------------

static Internal::KitManagerPrivate *d = nullptr;

KitManager *KitManager::instance()
{
    static KitManager theManager;
    return &theManager;
}

KitManager::KitManager()
{
    d = new KitManagerPrivate;
}

void KitManager::destroy()
{
    delete d;
    d = nullptr;
}

static bool kitMatchesAbiList(const Kit *kit, const Abis &abis)
{
    const QList<Toolchain *> toolchains = ToolchainKitAspect::toolChains(kit);
    for (const Toolchain * const tc : toolchains) {
        const Abi tcAbi = tc->targetAbi();
        for (const Abi &abi : abis) {
            if (tcAbi == abi)
                return true;
        }
    }
    return false;
};

static bool isHostKit(const Kit *kit)
{
    const Abi hostAbi = Abi::hostAbi();
    if (HostOsInfo::isMacHost() && hostAbi.architecture() == Abi::ArmArchitecture) {
        const Abi x86Abi(Abi::X86Architecture,
                         hostAbi.os(),
                         hostAbi.osFlavor(),
                         hostAbi.binaryFormat(),
                         hostAbi.wordWidth());

        return kitMatchesAbiList(kit, {hostAbi, x86Abi});
    }
    return kitMatchesAbiList(kit, {hostAbi});
};

static Id deviceTypeForKit(const Kit *kit)
{
    if (isHostKit(kit))
        return Constants::DESKTOP_DEVICE_TYPE;
    const QList<Toolchain *> toolchains = ToolchainKitAspect::toolChains(kit);
    for (const Toolchain * const tc : toolchains) {
        const Abi tcAbi = tc->targetAbi();
        switch (tcAbi.os()) {
        case Abi::BareMetalOS:
            return BareMetal::Constants::BareMetalOsType;
        case Abi::BsdOS:
        case Abi::DarwinOS:
        case Abi::UnixOS:
            return RemoteLinux::Constants::GenericLinuxOsType;
        case Abi::LinuxOS:
            if (tcAbi.osFlavor() == Abi::AndroidLinuxFlavor)
                return Android::Constants::ANDROID_DEVICE_TYPE;
            return RemoteLinux::Constants::GenericLinuxOsType;
        case Abi::QnxOS:
            return Qnx::Constants::QNX_QNX_OS_TYPE;
        case Abi::VxWorks:
            return "VxWorks.Device.Type";
        default:
            break;
        }
    }
    return Constants::DESKTOP_DEVICE_TYPE;
};

void KitManager::restoreKits()
{
    NANOTRACE_SCOPE("ProjectExplorer", "KitManager::restoreKits");
    QTC_ASSERT(!d->m_initialized, return );

    connect(ICore::instance(), &ICore::saveSettingsRequested, &KitManager::saveKits);

    std::vector<std::unique_ptr<Kit>> resultList;

    // read all kits from user file
    Utils::Id defaultUserKit;
    std::vector<std::unique_ptr<Kit>> kitsToCheck;
    {
        KitList userKits = restoreKitsHelper(settingsFileName());
        defaultUserKit = userKits.defaultKit;

        for (auto &k : userKits.kits) {
            if (k->detectionSource().isSdkProvided()) {
                kitsToCheck.emplace_back(std::move(k));
            } else {
                completeKit(k.get()); // Store manual kits
                resultList.emplace_back(std::move(k));
            }
        }
    }

    // read all kits from SDK
    {
        KitList system = restoreKitsHelper(ICore::installerResourcePath(KIT_FILENAME));

        // SDK kits need to get updated with the user-provided extra settings:
        for (auto &current : system.kits) {
            // make sure we mark these as autodetected and run additional setup logic
            current->setDetectionSource(DetectionSource::FromSdk);
            current->makeSticky();

            // Process:
            auto toStore = std::move(current);
            toStore->upgrade();
            toStore->setup(); // Make sure all kitinformation are properly set up before merging them
            // with the information from the user settings file

            // Check whether we had this kit stored and prefer the stored one:
            const auto i = std::find_if(std::begin(kitsToCheck),
                                        std::end(kitsToCheck),
                                        Utils::equal(&Kit::id, toStore->id()));
            if (i != std::end(kitsToCheck)) {
                Kit *ptr = i->get();

                // Overwrite settings that the SDK sets to those values:
                for (const KitAspectFactory *factory : KitManager::kitAspectFactories()) {
                    const Id id = factory->id();
                    // Copy sticky settings over:
                    ptr->setSticky(id, toStore->isSticky(id));
                    if (ptr->isSticky(id))
                        ptr->setValue(id, toStore->value(id));
                }
                toStore = std::move(*i);
                kitsToCheck.erase(i);
            }
            completeKit(toStore.get()); // Store manual kits
            resultList.emplace_back(std::move(toStore));
        }
    }

    // Delete all loaded autodetected kits that were not rediscovered:
    kitsToCheck.clear();

    // Remove replacement kits for which the original kit has turned up again.
    for (auto it = resultList.begin(); it != resultList.end();) {
        const auto &k = *it;
        if (k->isReplacementKit() && contains(resultList, [&k](const std::unique_ptr<Kit> &other) {
                                              return other->id() == k->id() && other != k; })) {
            it = resultList.erase(it);
        } else {
            ++it;
        }
    }

    const Abis abisOfBinary = d->m_binaryForKit.isEmpty()
            ? Abis() : Abi::abisOfBinary(d->m_binaryForKit);
    const bool haveKitForBinary = abisOfBinary.isEmpty()
            || contains(resultList, [&abisOfBinary](const std::unique_ptr<Kit> &kit) {
        return kitMatchesAbiList(kit.get(), abisOfBinary);
    });
    Kit *kitForBinary = nullptr;

    QList<Kit *> hostKits;
    if (resultList.empty() || !haveKitForBinary) {
        // No kits exist yet, so let's try to autoconfigure some from the toolchains we know.
        QHash<Abi, QHash<LanguageCategory, std::optional<ToolchainBundle>>> uniqueToolchains;

        const QList<ToolchainBundle> bundles = ToolchainBundle::collectBundles(
            ToolchainBundle::HandleMissing::CreateAndRegister);
        for (const ToolchainBundle &bundle : bundles) {
            auto &bestBundle
                = uniqueToolchains[bundle.targetAbi()][bundle.factory()->languageCategory()];
            if (!bestBundle) {
                bestBundle = bundle;
                continue;
            }

            if (ToolchainManager::isBetterToolchain(bundle, *bestBundle))
                bestBundle = bundle;
        }

        // Create temporary kits for all toolchains found.
        decltype(resultList) tempList;
        for (auto it = uniqueToolchains.cbegin(); it != uniqueToolchains.cend(); ++it) {
            auto kit = std::make_unique<Kit>();
            kit->setDetectionSource(DetectionSource::Manual); // TODO: Why manual? What does autodetected mean here?
            for (const auto &bundle : it.value())
                ToolchainKitAspect::setBundle(kit.get(), *bundle);
            if (contains(resultList, [&kit](const std::unique_ptr<Kit> &existingKit) {
                return ToolchainKitAspect::toolChains(kit.get())
                         == ToolchainKitAspect::toolChains(existingKit.get());
            })) {
                continue;
            }
            if (isHostKit(kit.get()))
                kit->setUnexpandedDisplayName(Tr::tr("Desktop (%1)").arg(it.key().toString()));
            else
                kit->setUnexpandedDisplayName(it.key().toString());
            RunDeviceTypeKitAspect::setDeviceTypeId(kit.get(), deviceTypeForKit(kit.get()));
            kit->setup();
            tempList.emplace_back(std::move(kit));
        }

        // Now make the "best" temporary kits permanent. The logic is as follows:
        //     - If the user has requested a kit for a given binary and one or more kits
        //       with a matching ABI exist, then we randomly choose exactly one among those with
        //       the highest weight.
        //     - If the user has not requested a kit for a given binary or no such kit could
        //       be created, we choose all kits with the highest weight. If none of these
        //       is a host kit, then we also add the host kit with the highest weight.
        Utils::sort(tempList, [](const std::unique_ptr<Kit> &k1, const std::unique_ptr<Kit> &k2) {
            return k1->weight() > k2->weight();
        });
        if (!abisOfBinary.isEmpty()) {
            for (auto it = tempList.begin(); it != tempList.end(); ++it) {
                if (kitMatchesAbiList(it->get(), abisOfBinary)) {
                    kitForBinary = it->get();
                    resultList.emplace_back(std::move(*it));
                    tempList.erase(it);
                    break;
                }
            }
        }
        if (!kitForBinary && !tempList.empty()) {
            const int maxWeight = tempList.front()->weight();
            for (auto it = tempList.begin(); it != tempList.end(); it = tempList.erase(it)) {
                if ((*it)->weight() < maxWeight)
                    break;
                if (isHostKit(it->get()))
                    hostKits << it->get();
                resultList.emplace_back(std::move(*it));
            }
            if (!contains(resultList, [](const std::unique_ptr<Kit> &kit) {
                          return isHostKit(kit.get());})) {
                QTC_ASSERT(hostKits.isEmpty(), hostKits.clear());
                for (auto &kit : tempList) {
                    if (isHostKit(kit.get())) {
                        hostKits << kit.get();
                        resultList.emplace_back(std::move(kit));
                        break;
                    }
                }
            }
        }

        if (hostKits.size() == 1)
            hostKits.first()->setUnexpandedDisplayName(Tr::tr("Desktop"));
    }

    Kit *k = kitForBinary;
    if (!k)
        k = Utils::findOrDefault(resultList, Utils::equal(&Kit::id, defaultUserKit));
    if (!k)
        k = Utils::findOrDefault(hostKits, &Kit::isValid);
    if (!k)
        k = Utils::findOrDefault(resultList, &Kit::isValid);
    std::swap(resultList, d->m_kitList);
    d->m_initialized = true;
    setDefaultKit(k);

    d->m_writer = std::make_unique<PersistentSettingsWriter>(settingsFileName(), "QtCreatorProfiles");

    KitAspectFactory::handleKitsLoaded();

    emit instance()->kitsLoaded();
    emit instance()->kitsChanged();
}

KitManager::~KitManager()
{
}

void KitManager::saveKits()
{
    QTC_ASSERT(d, return);
    if (!d->m_writer) // ignore save requests while we are not initialized.
        return;

    Store data;
    data.insert(KIT_FILE_VERSION_KEY, 1);

    int count = 0;
    const QList<Kit *> kits = KitManager::kits();
    for (Kit *k : kits) {
        Store tmp = k->toMap();
        if (tmp.isEmpty())
            continue;
        data.insert(numberedKey(KIT_DATA_KEY, count), variantFromStore(tmp));
        ++count;
    }
    data.insert(KIT_COUNT_KEY, count);
    data.insert(KIT_DEFAULT_KEY,
                d->m_defaultKit ? QString::fromLatin1(d->m_defaultKit->id().name()) : QString());
    data.insert(KIT_IRRELEVANT_ASPECTS_KEY,
                transform<QVariantList>(d->m_irrelevantAspects, &Id::toSetting));
    d->m_writer->save(data);
}

bool KitManager::isLoaded()
{
    return d->m_initialized;
}

bool KitManager::waitForLoaded(const int timeout)
{
    if (isLoaded())
        return true;
    showLoadingProgress();
    QElapsedTimer timer;
    timer.start();
    while (!isLoaded() && !timer.hasExpired(timeout))
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    return KitManager::isLoaded();
}

void KitManager::showLoadingProgress()
{
    if (isLoaded())
        return;
    static QFutureInterface<void> futureInterface;
    if (futureInterface.isRunning())
        return;
    futureInterface.reportStarted();
    using namespace std::chrono_literals;
    Core::ProgressManager::addTimedTask(futureInterface.future(),
                                        Tr::tr("Loading Kits"),
                                        "LoadingKitsProgress",
                                        5s);
    connect(instance(), &KitManager::kitsLoaded, []() { futureInterface.reportFinished(); });
}

void KitManager::setBinaryForKit(const FilePath &binary)
{
    QTC_ASSERT(d, return);
    d->m_binaryForKit = binary;
}

const QList<Kit *> KitManager::sortedKits()
{
    QTC_ASSERT(KitManager::isLoaded(), return {});
    // This method was added to delay the sorting of kits as long as possible.
    // Since the displayName can contain variables it can be costly (e.g. involve
    // calling executables to find version information, etc.) to call that
    // method!
    // Avoid lots of potentially expensive calls to Kit::displayName():
    std::vector<QPair<QString, Kit *>> sortList =
        Utils::transform(d->m_kitList, [](const std::unique_ptr<Kit> &k) {
        return qMakePair(k->displayName(), k.get());
    });
    Utils::sort(sortList,
                [](const QPair<QString, Kit *> &a, const QPair<QString, Kit *> &b) -> bool {
                    const int nameResult = Utils::caseFriendlyCompare(a.first, b.first);
                    if (nameResult != 0)
                        return nameResult < 0;
                    return a.second < b.second;
                });
    return Utils::transform<QList>(sortList, &QPair<QString, Kit *>::second);
}

static KitList restoreKitsHelper(const FilePath &fileName)
{
    KitList result;

    if (!fileName.exists())
        return result;

    PersistentSettingsReader reader;
    if (!reader.load(fileName)) {
        qWarning("Warning: Failed to read \"%s\", cannot restore kits!",
                 qPrintable(fileName.toUserOutput()));
        return result;
    }
    Store data = reader.restoreValues();

    // Check version:
    int version = data.value(KIT_FILE_VERSION_KEY, 0).toInt();
    if (version < 1) {
        qWarning("Warning: Kit file version %d not supported, cannot restore kits!", version);
        return result;
    }

    const int count = data.value(KIT_COUNT_KEY, 0).toInt();
    for (int i = 0; i < count; ++i) {
        const Key key = numberedKey(KIT_DATA_KEY, i);
        if (!data.contains(key))
            break;

        const Store stMap = storeFromVariant(data.value(key));

        auto k = std::make_unique<Kit>(stMap);
        if (k->id().isValid()) {
            result.kits.emplace_back(std::move(k));
        } else {
            qWarning("Warning: Unable to restore kits stored in %s at position %d.",
                     qPrintable(fileName.toUserOutput()),
                     i);
        }
    }
    const Id id = Id::fromSetting(data.value(KIT_DEFAULT_KEY));
    if (!id.isValid())
        return result;

    if (Utils::contains(result.kits, [id](const std::unique_ptr<Kit> &k) { return k->id() == id; }))
        result.defaultKit = id;
    const auto it = data.constFind(KIT_IRRELEVANT_ASPECTS_KEY);
    if (it != data.constEnd())
        d->m_irrelevantAspects = transform<QSet<Id>>(it.value().toList(), &Id::fromSetting);

    return result;
}

const QList<Kit *> KitManager::kits()
{
    QTC_ASSERT(KitManager::isLoaded(), return {});
    return Utils::toRawPointer<QList>(d->m_kitList);
}

Kit *KitManager::kit(Id id)
{
    if (!id.isValid())
        return nullptr;

    QTC_ASSERT(KitManager::isLoaded(), return {});
    return Utils::findOrDefault(d->m_kitList, Utils::equal(&Kit::id, id));
}

Kit *KitManager::kit(const Kit::Predicate &predicate)
{
    return Utils::findOrDefault(kits(), predicate);
}

Kit *KitManager::defaultKit()
{
    QTC_ASSERT(KitManager::isLoaded(), return {});
    return d->m_defaultKit;
}

const QList<KitAspectFactory *> KitManager::kitAspectFactories()
{
    return KitAspectFactory::kitAspectFactories();
}

const QSet<Id> KitManager::irrelevantAspects()
{
    return d->m_irrelevantAspects;
}

void KitManager::setIrrelevantAspects(const QSet<Id> &aspects)
{
    d->m_irrelevantAspects = aspects;
}

void KitManager::notifyAboutUpdate(Kit *k)
{
    if (!k || !isLoaded())
        return;

    if (Utils::contains(d->m_kitList, k)) {
        emit instance()->kitUpdated(k);
        emit instance()->kitsChanged();
    } else {
        emit instance()->unmanagedKitUpdated(k);
    }
}

Kit *KitManager::registerKit(const std::function<void (Kit *)> &init, Utils::Id id)
{
    QTC_ASSERT(isLoaded(), return {});

    auto k = std::make_unique<Kit>(id);
    QTC_ASSERT(k->id().isValid(), return nullptr);

    Kit *kptr = k.get();
    if (init)
        init(kptr);

    // make sure we have all the information in our kits:
    completeKit(kptr);

    d->m_kitList.emplace_back(std::move(k));

    if (!d->m_defaultKit || (!d->m_defaultKit->isValid() && kptr->isValid()))
        setDefaultKit(kptr);

    emit instance()->kitAdded(kptr);
    emit instance()->kitsChanged();
    return kptr;
}

void KitManager::deregisterKit(Kit *k)
{
    deregisterKits({k});
}

void KitManager::deregisterKits(const QList<Kit *> kitList)
{
    QTC_ASSERT(KitManager::isLoaded(), return);
    std::vector<std::unique_ptr<Kit>> removed; // to keep them alive until the end of the function
    bool defaultKitRemoved = false;
    for (Kit *k : kitList) {
        QTC_ASSERT(k, continue);
        std::optional<std::unique_ptr<Kit>> taken = Utils::take(d->m_kitList, k);
        QTC_ASSERT(taken, continue);
        if (defaultKit() == k)
            defaultKitRemoved = true;
        removed.push_back(std::move(*taken));
    }

    if (defaultKitRemoved) {
        d->m_defaultKit = Utils::findOrDefault(kits(), &Kit::isValid);
        emit instance()->defaultkitChanged();
    }

    for (auto it = removed.cbegin(); it != removed.cend(); ++it)
        emit instance()->kitRemoved(it->get());
    emit instance()->kitsChanged();

    // FIXME: TargetSetupPage potentially deregisters kits on destruction, after the final
    //        ICore::saveSettingsRequested() was emitted.
    saveKits();
}

void KitManager::setDefaultKit(Kit *k)
{
    QTC_ASSERT(KitManager::isLoaded(), return);

    if (defaultKit() == k)
        return;
    if (k && !Utils::contains(d->m_kitList, k))
        return;
    d->m_defaultKit = k;
    emit instance()->defaultkitChanged();
}

void KitManager::completeKit(Kit *k)
{
    QTC_ASSERT(k, return);
    KitGuard g(k);
    for (KitAspectFactory *factory : kitAspectFactories()) {
        if (!k->isAspectRelevant(factory->id()))
            continue;
        factory->upgrade(k);
        if (!k->hasValue(factory->id()))
            factory->setup(k);
        else
            factory->fix(k);
    }
}

// --------------------------------------------------------------------
// KitFeatureProvider:
// --------------------------------------------------------------------

// This FeatureProvider maps the platforms onto the device types.

QSet<Id> KitFeatureProvider::availableFeatures(Id id) const
{
    QSet<Id> features;
    for (const Kit *k : KitManager::kits()) {
        if (k->supportedPlatforms().contains(id))
            features.unite(k->availableFeatures());
    }
    return features;
}

QSet<Id> KitFeatureProvider::availablePlatforms() const
{
    QSet<Id> platforms;
    for (const Kit *k : KitManager::kits())
        platforms.unite(k->supportedPlatforms());
    return platforms;
}

QString KitFeatureProvider::displayNameForPlatform(Id id) const
{
    if (IDeviceFactory *f = IDeviceFactory::find(id)) {
        QString dn = f->displayName();
        const QString deviceStr = QStringLiteral("device");
        if (dn.endsWith(deviceStr, Qt::CaseInsensitive))
            dn = dn.remove(deviceStr, Qt::CaseInsensitive).trimmed();
        QTC_CHECK(!dn.isEmpty());
        return dn;
    }
    return {};
}

} // namespace ProjectExplorer
