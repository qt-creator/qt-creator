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

#include "kitmanager.h"

#include "abi.h"
#include "devicesupport/idevicefactory.h"
#include "kit.h"
#include "kitfeatureprovider.h"
#include "kitinformation.h"
#include "kitmanagerconfigwidget.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "task.h"
#include "toolchainmanager.h"

#include <coreplugin/icore.h>

#include <android/androidconstants.h>
#include <baremetal/baremetalconstants.h>
#include <qnx/qnxconstants.h>
#include <remotelinux/remotelinux_constants.h>

#include <utils/environment.h>
#include <utils/persistentsettings.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QHash>
#include <QSettings>
#include <QStyle>

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
const char KIT_FILENAME[] = "/profiles.xml";

static FilePath settingsFileName()
{
    return FilePath::fromString(ICore::userResourcePath() + KIT_FILENAME);
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

    void addKitAspect(KitAspect *ki)
    {
        QTC_ASSERT(!m_aspectList.contains(ki), return);
        m_aspectList.append(ki);
        m_aspectListIsSorted = false;
    }

    void removeKitAspect(KitAspect *ki)
    {
        int removed = m_aspectList.removeAll(ki);
        QTC_CHECK(removed == 1);
    }

    const QList<KitAspect *> kitAspects()
    {
        if (!m_aspectListIsSorted) {
            Utils::sort(m_aspectList, [](const KitAspect *a, const KitAspect *b) {
                return a->priority() > b->priority();
            });
            m_aspectListIsSorted = true;
        }
        return m_aspectList;
    }

    void setBinaryForKit(const FilePath &fp) { m_binaryForKit = fp; }
    FilePath binaryForKit() const { return m_binaryForKit; }

private:
    // Sorted by priority, in descending order...
    QList<KitAspect *> m_aspectList;
    // ... if this here is set:
    bool m_aspectListIsSorted = true;

    FilePath m_binaryForKit;
};

} // namespace Internal

// --------------------------------------------------------------------------
// KitManager:
// --------------------------------------------------------------------------

static Internal::KitManagerPrivate *d = nullptr;
static KitManager *m_instance = nullptr;

KitManager *KitManager::instance()
{
    if (!m_instance)
        m_instance = new KitManager;
    return m_instance;
}

KitManager::KitManager()
{
    d = new KitManagerPrivate;
    QTC_CHECK(!m_instance);
    m_instance = this;

    connect(ICore::instance(), &ICore::saveSettingsRequested, this, &KitManager::saveKits);

    connect(this, &KitManager::kitAdded, this, &KitManager::kitsChanged);
    connect(this, &KitManager::kitRemoved, this, &KitManager::kitsChanged);
    connect(this, &KitManager::kitUpdated, this, &KitManager::kitsChanged);
}

void KitManager::destroy()
{
    delete d;
    d = nullptr;
    delete m_instance;
    m_instance = nullptr;
}

void KitManager::restoreKits()
{
    QTC_ASSERT(!d->m_initialized, return );

    std::vector<std::unique_ptr<Kit>> resultList;

    // read all kits from user file
    Utils::Id defaultUserKit;
    std::vector<std::unique_ptr<Kit>> kitsToCheck;
    {
        KitList userKits = restoreKitsHelper(settingsFileName());
        defaultUserKit = userKits.defaultKit;

        for (auto &k : userKits.kits) {
            if (k->isSdkProvided()) {
                kitsToCheck.emplace_back(std::move(k));
            } else {
                completeKit(k.get()); // Store manual kits
                resultList.emplace_back(std::move(k));
            }
        }
    }

    // read all kits from SDK
    {
        KitList system = restoreKitsHelper
                (FilePath::fromString(ICore::installerResourcePath() + KIT_FILENAME));

        // SDK kits need to get updated with the user-provided extra settings:
        for (auto &current : system.kits) {
            // make sure we mark these as autodetected and run additional setup logic
            current->setAutoDetected(true);
            current->setSdkProvided(true);
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
                for (const KitAspect *aspect : KitManager::kitAspects()) {
                    // Copy sticky settings over:
                    if (ptr->isSticky(aspect->id())) {
                        ptr->setValue(aspect->id(), toStore->value(aspect->id()));
                        ptr->setSticky(aspect->id(), true);
                    }
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

    static const auto kitMatchesAbiList = [](const Kit *kit, const Abis &abis) {
        const QList<ToolChain *> toolchains = ToolChainKitAspect::toolChains(kit);
        for (const ToolChain * const tc : toolchains) {
            const Abi tcAbi = tc->targetAbi();
            for (const Abi &abi : abis) {
                if (tcAbi.os() == abi.os() && tcAbi.architecture() == abi.architecture()
                        && (tcAbi.os() != Abi::LinuxOS || tcAbi.osFlavor() == abi.osFlavor())) {
                    return true;
                }
            }
        }
        return false;
    };

    const Abis abisOfBinary = d->binaryForKit().isEmpty()
            ? Abis() : Abi::abisOfBinary(d->binaryForKit());
    const auto kitMatchesAbiOfBinary = [&abisOfBinary](const Kit *kit) {
        return kitMatchesAbiList(kit, abisOfBinary);
    };
    const bool haveKitForBinary = abisOfBinary.isEmpty()
            || contains(resultList, [&kitMatchesAbiOfBinary](const std::unique_ptr<Kit> &kit) {
        return kitMatchesAbiOfBinary(kit.get());
    });
    Kit *kitForBinary = nullptr;

    if (resultList.empty() || !haveKitForBinary) {
        // No kits exist yet, so let's try to autoconfigure some from the toolchains we know.
        QHash<Abi, QHash<Utils::Id, ToolChain *>> uniqueToolchains;

        // On Linux systems, we usually detect a plethora of same-ish toolchains. The following
        // algorithm gives precedence to icecc and ccache and otherwise simply chooses the one with
        // the shortest path. This should also take care of ensuring matching C/C++ pairs.
        // TODO: This should not need to be done here. Instead, it should be a convenience
        // operation on some lower level, e.g. in the toolchain class(es).
        // Also, we shouldn't detect so many doublets in the first place.
        for (ToolChain * const tc : ToolChainManager::toolChains()) {
            ToolChain *&bestTc = uniqueToolchains[tc->targetAbi()][tc->language()];
            if (!bestTc) {
                bestTc = tc;
                continue;
            }
            const QString bestFilePath = bestTc->compilerCommand().toString();
            const QString currentFilePath = tc->compilerCommand().toString();
            if (bestFilePath.contains("icecc"))
                continue;
            if (currentFilePath.contains("icecc")) {
                bestTc = tc;
                continue;
            }

            if (bestFilePath.contains("ccache"))
                continue;
            if (currentFilePath.contains("ccache")) {
                bestTc = tc;
                continue;
            }
            if (bestFilePath.length() > currentFilePath.length())
                bestTc = tc;
        }

        static const auto isHostKit = [](const Kit *kit) {
            return kitMatchesAbiList(kit, {Abi::hostAbi()});
        };

        static const auto deviceTypeForKit = [](const Kit *kit) {
            if (isHostKit(kit))
                return Constants::DESKTOP_DEVICE_TYPE;
            const QList<ToolChain *> toolchains = ToolChainKitAspect::toolChains(kit);
            for (const ToolChain * const tc : toolchains) {
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

        // Create temporary kits for all toolchains found.
        decltype(resultList) tempList;
        for (auto it = uniqueToolchains.cbegin(); it != uniqueToolchains.cend(); ++it) {
            auto kit = std::make_unique<Kit>();
            kit->setSdkProvided(false);
            kit->setAutoDetected(false); // TODO: Why false? What does autodetected mean here?
            for (ToolChain * const tc : it.value())
                ToolChainKitAspect::setToolChain(kit.get(), tc);
            if (contains(resultList, [&kit](const std::unique_ptr<Kit> &existingKit) {
                return ToolChainKitAspect::toolChains(kit.get())
                         == ToolChainKitAspect::toolChains(existingKit.get());
            })) {
                continue;
            }
            if (isHostKit(kit.get()))
                kit->setUnexpandedDisplayName(tr("Desktop (%1)").arg(it.key().toString()));
            else
                kit->setUnexpandedDisplayName(it.key().toString());
            DeviceTypeKitAspect::setDeviceTypeId(kit.get(), deviceTypeForKit(kit.get()));
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
                if (kitMatchesAbiOfBinary(it->get())) {
                    kitForBinary = it->get();
                    resultList.emplace_back(std::move(*it));
                    tempList.erase(it);
                    break;
                }
            }
        }
        QList<Kit *> hostKits;
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
            hostKits.first()->setUnexpandedDisplayName(tr("Desktop"));
    }

    Kit *k = kitForBinary;
    if (!k)
        k = Utils::findOrDefault(resultList, Utils::equal(&Kit::id, defaultUserKit));
    if (!k)
        k = Utils::findOrDefault(resultList, &Kit::isValid);
    std::swap(resultList, d->m_kitList);
    setDefaultKit(k);

    d->m_writer = std::make_unique<PersistentSettingsWriter>(settingsFileName(), "QtCreatorProfiles");
    d->m_initialized = true;
    emit m_instance->kitsLoaded();
    emit m_instance->kitsChanged();
}

KitManager::~KitManager()
{
}

void KitManager::saveKits()
{
    QTC_ASSERT(d, return);
    if (!d->m_writer) // ignore save requests while we are not initialized.
        return;

    QVariantMap data;
    data.insert(QLatin1String(KIT_FILE_VERSION_KEY), 1);

    int count = 0;
    foreach (Kit *k, kits()) {
        QVariantMap tmp = k->toMap();
        if (tmp.isEmpty())
            continue;
        data.insert(QString::fromLatin1(KIT_DATA_KEY) + QString::number(count), tmp);
        ++count;
    }
    data.insert(QLatin1String(KIT_COUNT_KEY), count);
    data.insert(QLatin1String(KIT_DEFAULT_KEY),
                d->m_defaultKit ? QString::fromLatin1(d->m_defaultKit->id().name()) : QString());
    data.insert(KIT_IRRELEVANT_ASPECTS_KEY,
                transform<QVariantList>(d->m_irrelevantAspects, &Id::toSetting));
    d->m_writer->save(data, ICore::dialogParent());
}

bool KitManager::isLoaded()
{
    return d->m_initialized;
}

void KitManager::registerKitAspect(KitAspect *ki)
{
    instance();
    QTC_ASSERT(d, return);
    d->addKitAspect(ki);

    // Adding this aspect to possibly already existing kits is currently not
    // needed here as kits are only created after all aspects are created
    // in *Plugin::initialize().
    // Make sure we notice when this assumption breaks:
    QTC_CHECK(d->m_kitList.empty());
}

void KitManager::deregisterKitAspect(KitAspect *ki)
{
    // Happens regularly for the aspects from the ProjectExplorerPlugin as these
    // are destroyed after the manual call to KitManager::destroy() there, but as
    // this here is just for sanity reasons that the KitManager does not access
    // a destroyed aspect, a destroyed KitManager is not a problem.
    if (d)
        d->removeKitAspect(ki);
}

void KitManager::setBinaryForKit(const FilePath &binary)
{
    QTC_ASSERT(d, return);
    d->setBinaryForKit(binary);
}

QList<Kit *> KitManager::sortKits(const QList<Kit *> &kits)
{
    // This method was added to delay the sorting of kits as long as possible.
    // Since the displayName can contain variables it can be costly (e.g. involve
    // calling executables to find version information, etc.) to call that
    // method!
    // Avoid lots of potentially expensive calls to Kit::displayName():
    QList<QPair<QString, Kit *>> sortList = Utils::transform(kits, [](Kit *k) {
        return qMakePair(k->displayName(), k);
    });
    Utils::sort(sortList,
                [](const QPair<QString, Kit *> &a, const QPair<QString, Kit *> &b) -> bool {
                    if (a.first == b.first)
                        return a.second < b.second;
                    return a.first < b.first;
                });
    return Utils::transform(sortList, &QPair<QString, Kit *>::second);
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
    QVariantMap data = reader.restoreValues();

    // Check version:
    int version = data.value(QLatin1String(KIT_FILE_VERSION_KEY), 0).toInt();
    if (version < 1) {
        qWarning("Warning: Kit file version %d not supported, cannot restore kits!", version);
        return result;
    }

    const int count = data.value(QLatin1String(KIT_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(KIT_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;

        const QVariantMap stMap = data.value(key).toMap();

        auto k = std::make_unique<Kit>(stMap);
        if (k->id().isValid()) {
            result.kits.emplace_back(std::move(k));
        } else {
            qWarning("Warning: Unable to restore kits stored in %s at position %d.",
                     qPrintable(fileName.toUserOutput()),
                     i);
        }
    }
    const Id id = Id::fromSetting(data.value(QLatin1String(KIT_DEFAULT_KEY)));
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
    return Utils::toRawPointer<QList>(d->m_kitList);
}

Kit *KitManager::kit(Id id)
{
    if (!id.isValid())
        return nullptr;

    return Utils::findOrDefault(d->m_kitList, Utils::equal(&Kit::id, id));
}

Kit *KitManager::kit(const Kit::Predicate &predicate)
{
    return Utils::findOrDefault(kits(), predicate);
}

Kit *KitManager::defaultKit()
{
    return d->m_defaultKit;
}

const QList<KitAspect *> KitManager::kitAspects()
{
    return d->kitAspects();
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

    if (Utils::contains(d->m_kitList, k))
        emit m_instance->kitUpdated(k);
    else
        emit m_instance->unmanagedKitUpdated(k);
}

Kit *KitManager::registerKit(const std::function<void (Kit *)> &init, Utils::Id id)
{
    QTC_ASSERT(isLoaded(), return nullptr);

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

    emit m_instance->kitAdded(kptr);
    return kptr;
}

void KitManager::deregisterKit(Kit *k)
{
    if (!k || !Utils::contains(d->m_kitList, k))
        return;
    auto taken = Utils::take(d->m_kitList, k);
    if (defaultKit() == k) {
        Kit *newDefault = Utils::findOrDefault(kits(), [](Kit *k) { return k->isValid(); });
        setDefaultKit(newDefault);
    }
    emit m_instance->kitRemoved(k);
}

void KitManager::setDefaultKit(Kit *k)
{
    if (defaultKit() == k)
        return;
    if (k && !Utils::contains(d->m_kitList, k))
        return;
    d->m_defaultKit = k;
    emit m_instance->defaultkitChanged();
}

void KitManager::completeKit(Kit *k)
{
    QTC_ASSERT(k, return);
    KitGuard g(k);
    for (KitAspect *ki : d->kitAspects()) {
        ki->upgrade(k);
        if (!k->hasValue(ki->id()))
            ki->setup(k);
        else
            ki->fix(k);
    }
}

// --------------------------------------------------------------------
// KitAspect:
// --------------------------------------------------------------------

KitAspect::KitAspect()
{
    KitManager::registerKitAspect(this);
}

KitAspect::~KitAspect()
{
    KitManager::deregisterKitAspect(this);
}

int KitAspect::weight(const Kit *k) const
{
    return k->value(id()).isValid() ? 1 : 0;
}

void KitAspect::addToEnvironment(const Kit *k, Environment &env) const
{
    Q_UNUSED(k)
    Q_UNUSED(env)
}

QList<OutputLineParser *> KitAspect::createOutputParsers(const Kit *k) const
{
    Q_UNUSED(k)
    return {};
}

QString KitAspect::displayNamePostfix(const Kit *k) const
{
    Q_UNUSED(k)
    return QString();
}

QSet<Id> KitAspect::supportedPlatforms(const Kit *k) const
{
    Q_UNUSED(k)
    return QSet<Id>();
}

QSet<Id> KitAspect::availableFeatures(const Kit *k) const
{
    Q_UNUSED(k)
    return QSet<Id>();
}

void KitAspect::addToMacroExpander(Kit *k, MacroExpander *expander) const
{
    Q_UNUSED(k)
    Q_UNUSED(expander)
}

void KitAspect::notifyAboutUpdate(Kit *k)
{
    if (k)
        k->kitUpdated();
}

KitAspectWidget::KitAspectWidget(Kit *kit, const KitAspect *ki) : m_kit(kit),
    m_kitInformation(ki), m_isSticky(kit->isSticky(ki->id()))
{ }

Utils::Id KitAspectWidget::kitInformationId() const
{
    return m_kitInformation->id();
}

QString KitAspectWidget::msgManage()
{
    return tr("Manage...");
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
        const QString dn = f->displayName();
        QTC_CHECK(!dn.isEmpty());
        return dn;
    }
    return QString();
}

} // namespace ProjectExplorer
