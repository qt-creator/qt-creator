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

#include "devicesupport/idevicefactory.h"
#include "kit.h"
#include "kitfeatureprovider.h"
#include "kitmanagerconfigwidget.h"
#include "project.h"
#include "task.h"

#include <coreplugin/icore.h>

#include <utils/environment.h>
#include <utils/persistentsettings.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QSettings>

using namespace Core;
using namespace Utils;
using namespace ProjectExplorer::Internal;

namespace ProjectExplorer {
namespace Internal {

const char KIT_DATA_KEY[] = "Profile.";
const char KIT_COUNT_KEY[] = "Profile.Count";
const char KIT_FILE_VERSION_KEY[] = "Version";
const char KIT_DEFAULT_KEY[] = "Profile.Default";
const char KIT_FILENAME[] = "/profiles.xml";

static FileName settingsFileName()
{
    return FileName::fromString(ICore::userResourcePath() + KIT_FILENAME);
}

// --------------------------------------------------------------------------
// KitManagerPrivate:
// --------------------------------------------------------------------------

class KitManagerPrivate
{
public:
    Kit *m_defaultKit = nullptr;
    bool m_initialized = false;
    std::vector<std::unique_ptr<KitInformation>> m_informationList;
    std::vector<std::unique_ptr<Kit>> m_kitList;
    std::unique_ptr<PersistentSettingsWriter> m_writer;
};

} // namespace Internal

// --------------------------------------------------------------------------
// KitManager:
// --------------------------------------------------------------------------

static Internal::KitManagerPrivate *d = nullptr;
static KitManager *m_instance = nullptr;

KitManager *KitManager::instance()
{
    return m_instance;
}

KitManager::KitManager(QObject *parent)
    : QObject(parent)
{
    d = new KitManagerPrivate;
    QTC_CHECK(!m_instance);
    m_instance = this;

    connect(ICore::instance(), &ICore::saveSettingsRequested, this, &KitManager::saveKits);

    connect(this, &KitManager::kitAdded, this, &KitManager::kitsChanged);
    connect(this, &KitManager::kitRemoved, this, &KitManager::kitsChanged);
    connect(this, &KitManager::kitUpdated, this, &KitManager::kitsChanged);
}

void KitManager::restoreKits()
{
    QTC_ASSERT(!d->m_initialized, return );

    std::vector<std::unique_ptr<Kit>> resultList;

    // read all kits from user file
    Core::Id defaultUserKit;
    std::vector<std::unique_ptr<Kit>> kitsToCheck;
    {
        KitList userKits = restoreKits(settingsFileName());
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
        KitList system
                = restoreKits(FileName::fromString(ICore::installerResourcePath() + KIT_FILENAME));

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
                foreach (const KitInformation *ki, KitManager::kitInformation()) {
                    // Copy sticky settings over:
                    if (ptr->isSticky(ki->id())) {
                        ptr->setValue(ki->id(), toStore->value(ki->id()));
                        ptr->setSticky(ki->id(), true);
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

    if (resultList.size() == 0) {
        auto defaultKit = std::make_unique<Kit>(); // One kit using default values
        defaultKit->setUnexpandedDisplayName(tr("Desktop"));
        defaultKit->setSdkProvided(false);
        defaultKit->setAutoDetected(false);

        defaultKit->setup();

        completeKit(defaultKit.get()); // Store manual kits
        resultList.emplace_back(std::move(defaultKit));
    }

    Kit *k = Utils::findOrDefault(resultList, Utils::equal(&Kit::id, defaultUserKit));
    if (!k)
        k = Utils::findOrDefault(resultList, &Kit::isValid);
    std::swap(resultList, d->m_kitList);
    setDefaultKit(k);

    d->m_writer = std::make_unique<PersistentSettingsWriter>(settingsFileName(), "QtCreatorProfiles");
    d->m_initialized = true;
    emit kitsLoaded();
    emit kitsChanged();
}

KitManager::~KitManager()
{
    delete d;
    d = nullptr;
    m_instance = nullptr;
}

void KitManager::saveKits()
{
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
    d->m_writer->save(data, ICore::mainWindow());
}

bool KitManager::isLoaded()
{
    return d->m_initialized;
}

void KitManager::registerKitInformation(std::unique_ptr<KitInformation> &&ki)
{
    QTC_ASSERT(ki->id().isValid(), return );
    QTC_ASSERT(!Utils::contains(d->m_informationList, ki.get()), return );

    auto it = std::lower_bound(std::begin(d->m_informationList),
                               std::end(d->m_informationList),
                               ki,
                               [](const std::unique_ptr<KitInformation> &a,
                                  const std::unique_ptr<KitInformation> &b) {
                                   return a->priority() > b->priority();
                               });
    d->m_informationList.insert(it, std::move(ki));

    foreach (Kit *k, kits()) {
        if (!k->hasValue(ki->id()))
            k->setValue(ki->id(), ki->defaultValue(k));
        else
            ki->fix(k);
    }

    return;
}

QSet<Id> KitManager::supportedPlatforms()
{
    QSet<Id> platforms;
    foreach (const Kit *k, kits())
        platforms.unite(k->supportedPlatforms());
    return platforms;
}

QSet<Id> KitManager::availableFeatures(Core::Id platformId)
{
    QSet<Id> features;
    foreach (const Kit *k, kits()) {
        if (!k->supportedPlatforms().contains(platformId))
            continue;
        features.unite(k->availableFeatures());
    }
    return features;
}

QList<Kit *> KitManager::sortKits(const QList<Kit *> kits)
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

KitManager::KitList KitManager::restoreKits(const FileName &fileName)
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

    return result;
}

QList<Kit *> KitManager::kits(const Kit::Predicate &predicate)
{
    const QList<Kit *> result = Utils::toRawPointer<QList>(d->m_kitList);
    if (predicate)
        return Utils::filtered(result, predicate);
    return result;
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

QList<KitInformation *> KitManager::kitInformation()
{
    return Utils::toRawPointer<QList>(d->m_informationList);
}

KitManagerConfigWidget *KitManager::createConfigWidget(Kit *k)
{
    auto *result = new KitManagerConfigWidget(k);
    foreach (KitInformation *ki, kitInformation())
        result->addConfigWidget(ki->createConfigWidget(result->workingCopy()));

    result->updateVisibility();

    return result;
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

bool KitManager::registerKit(std::unique_ptr<Kit> &&k)
{
    QTC_ASSERT(isLoaded(), return false);

    if (!k)
        return true;

    QTC_ASSERT(k->id().isValid(), return false);

    Kit *kptr = k.get();
    if (Utils::contains(d->m_kitList, kptr))
        return false;

    // make sure we have all the information in our kits:
    completeKit(kptr);

    d->m_kitList.emplace_back(std::move(k));

    if (!d->m_defaultKit || (!d->m_defaultKit->isValid() && kptr->isValid()))
        setDefaultKit(kptr);

    emit m_instance->kitAdded(kptr);
    return true;
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
    for (const std::unique_ptr<KitInformation> &ki : d->m_informationList) {
        ki->upgrade(k);
        if (!k->hasValue(ki->id()))
            k->setValue(ki->id(), ki->defaultValue(k));
        else
            ki->fix(k);
    }
}

// --------------------------------------------------------------------
// KitInformation:
// --------------------------------------------------------------------

void KitInformation::addToEnvironment(const Kit *k, Environment &env) const
{
    Q_UNUSED(k);
    Q_UNUSED(env);
}

IOutputParser *KitInformation::createOutputParser(const Kit *k) const
{
    Q_UNUSED(k);
    return nullptr;
}

QString KitInformation::displayNamePostfix(const Kit *k) const
{
    Q_UNUSED(k);
    return QString();
}

QSet<Id> KitInformation::supportedPlatforms(const Kit *k) const
{
    Q_UNUSED(k);
    return QSet<Id>();
}

QSet<Id> KitInformation::availableFeatures(const Kit *k) const
{
    Q_UNUSED(k);
    return QSet<Id>();
}

void KitInformation::addToMacroExpander(Kit *k, MacroExpander *expander) const
{
    Q_UNUSED(k);
    Q_UNUSED(expander);
}

void KitInformation::notifyAboutUpdate(Kit *k)
{
    if (k)
        k->kitUpdated();
}

// --------------------------------------------------------------------
// KitFeatureProvider:
// --------------------------------------------------------------------

// This FeatureProvider maps the platforms onto the device types.

QSet<Id> KitFeatureProvider::availableFeatures(Id id) const
{
    return KitManager::availableFeatures(id);
}

QSet<Id> KitFeatureProvider::availablePlatforms() const
{
    return KitManager::supportedPlatforms();
}

QString KitFeatureProvider::displayNameForPlatform(Id id) const
{
    for (IDeviceFactory *f : IDeviceFactory::allDeviceFactories()) {
        if (f->availableCreationIds().contains(id)) {
            const QString dn = f->displayNameForId(id);
            QTC_ASSERT(!dn.isEmpty(), continue);
            return dn;
        }
    }
    return QString();
}

} // namespace ProjectExplorer
