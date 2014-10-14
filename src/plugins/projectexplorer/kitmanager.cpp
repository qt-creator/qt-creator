/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "kitmanager.h"

#include "kit.h"
#include "kitfeatureprovider.h"
#include "kitmanagerconfigwidget.h"
#include "project.h"
#include "task.h"

#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/environment.h>
#include <utils/algorithm.h>

#include <QSettings>

static const char KIT_DATA_KEY[] = "Profile.";
static const char KIT_COUNT_KEY[] = "Profile.Count";
static const char KIT_FILE_VERSION_KEY[] = "Version";
static const char KIT_DEFAULT_KEY[] = "Profile.Default";
static const char KIT_FILENAME[] = "/qtcreator/profiles.xml";

using Utils::PersistentSettingsWriter;
using Utils::PersistentSettingsReader;

static Utils::FileName settingsFileName()
{
    QFileInfo settingsLocation(Core::ICore::settings()->fileName());
    return Utils::FileName::fromString(settingsLocation.absolutePath() + QLatin1String(KIT_FILENAME));
}

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// KitManagerPrivate:
// --------------------------------------------------------------------------

class KitManagerPrivate
{
public:
    KitManagerPrivate();
    ~KitManagerPrivate();

    void insertKit(Kit *k)
    {
        // Keep list of kits sorted by displayname:
        int i =0;
        for (; i < m_kitList.count(); ++i)
            if (m_kitList.at(i)->displayName() > k->displayName())
                break;
        m_kitList.insert(i, k);
    }

    void moveKit(int pos)
    {
        if (pos < 0 || pos >= m_kitList.count())
            return;

        Kit *current = m_kitList.at(pos);
        int prev = pos - 1;
        int next = pos + 1;

        if (prev >= 0
                && m_kitList.at(prev)->displayName() > current->displayName()) {
            std::swap(m_kitList[prev], m_kitList[pos]);
            moveKit(prev);
        } else if (next < m_kitList.count()
                   && m_kitList.at(next)->displayName() < current->displayName()) {
            std::swap(m_kitList[pos], m_kitList[next]);
            moveKit(next);
        }
    }

    Kit *m_defaultKit;
    bool m_initialized;
    QList<KitInformation *> m_informationList;
    QList<Kit *> m_kitList;
    Utils::PersistentSettingsWriter *m_writer;
};

KitManagerPrivate::KitManagerPrivate() :
    m_defaultKit(0), m_initialized(false), m_writer(0)
{ }

KitManagerPrivate::~KitManagerPrivate()
{
    qDeleteAll(m_informationList);
    delete m_writer;
}

} // namespace Internal

// --------------------------------------------------------------------------
// KitManager:
// --------------------------------------------------------------------------

static Internal::KitManagerPrivate *d;
static KitManager *m_instance;

QObject *KitManager::instance()
{
    return m_instance;
}

KitManager::KitManager(QObject *parent) :
    QObject(parent)
{
    d = new Internal::KitManagerPrivate;
    QTC_CHECK(!m_instance);
    m_instance = this;

    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()),
            this, SLOT(saveKits()));

    connect(this, SIGNAL(kitAdded(ProjectExplorer::Kit*)),
            this, SIGNAL(kitsChanged()));
    connect(this, SIGNAL(kitRemoved(ProjectExplorer::Kit*)),
            this, SIGNAL(kitsChanged()));
    connect(this, SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            this, SIGNAL(kitsChanged()));
}

void KitManager::restoreKits()
{
    QTC_ASSERT(!d->m_initialized, return);
    static bool initializing = false;

    if (initializing) // kits will call kits() to check their display names, which will trigger another
                      // call to restoreKits, which ...
        return;

    initializing = true;

    QList<Kit *> kitsToRegister;
    QList<Kit *> kitsToValidate;
    QList<Kit *> kitsToCheck;

    // read all kits from SDK
    QFileInfo systemSettingsFile(Core::ICore::settings(QSettings::SystemScope)->fileName());
    QFileInfo kitFile(systemSettingsFile.absolutePath() + QLatin1String(KIT_FILENAME));
    if (kitFile.exists()) {
        KitList system = restoreKits(Utils::FileName(kitFile));
        // make sure we mark these as autodetected and run additional setup logic
        foreach (Kit *k, system.kits) {
            k->setAutoDetected(true);
            k->setSdkProvided(true);
            k->makeSticky();
            k->setup();
        }

        // SDK kits are always considered to be up for validation since they might have been
        // extended with additional information by creator in the meantime:
        kitsToValidate = system.kits;
    }

    // read all kits from user file
    KitList userKits;
    Utils::FileName userSettingsFile(settingsFileName());
    if (userSettingsFile.toFileInfo().exists())
        userKits = restoreKits(userSettingsFile);
    foreach (Kit *k, userKits.kits) {
        if (k->isSdkProvided())
            kitsToCheck.append(k);
        else
            kitsToRegister.append(k);
    }

    Kit *toStore = 0;
    foreach (Kit *current, kitsToValidate) {
        toStore = current;

        // Check whether we had this kit stored and prefer the stored one:
        for (int i = 0; i < kitsToCheck.count(); ++i) {
            if (kitsToCheck.at(i)->id() == current->id()) {
                toStore = kitsToCheck.at(i);
                kitsToCheck.removeAt(i);

                // Overwrite settings that the SDK sets to those values:
                foreach (const KitInformation *ki, kitInformation()) {
                    // Copy sticky settings over:
                    if (current->isSticky(ki->id())) {
                        toStore->setValue(ki->id(), current->value(ki->id()));
                        toStore->setSticky(ki->id(), true);
                    }
                }

                delete current;
                break;
            }
        }
        addKit(toStore);
    }

    // Delete all loaded autodetected kits that were not rediscovered:
    foreach (Kit *k, kitsToCheck)
        delete k;
    kitsToCheck.clear();

    // Store manual kits
    foreach (Kit *k, kitsToRegister)
        addKit(k);

    if (kits().isEmpty()) {
        Kit *defaultKit = new Kit; // One kit using default values
        defaultKit->setUnexpandedDisplayName(tr("Desktop"));
        defaultKit->setSdkProvided(false);
        defaultKit->setAutoDetected(false);
        defaultKit->setIconPath(Utils::FileName::fromLatin1(":///DESKTOP///"));

        defaultKit->setup();

        addKit(defaultKit);
        setDefaultKit(defaultKit);
    }

    Kit *k = find(userKits.defaultKit);
    if (k) {
        setDefaultKit(k);
    } else if (!defaultKit()) {
        k = Utils::findOr(kitsToRegister, 0, [](Kit *k) {
                              return k->isValid();
                          });
        if (k)
            setDefaultKit(k);
    }

    d->m_writer = new Utils::PersistentSettingsWriter(settingsFileName(), QLatin1String("QtCreatorProfiles"));
    d->m_initialized = true;
    emit kitsLoaded();
    emit kitsChanged();
}

KitManager::~KitManager()
{
    foreach (Kit *k, d->m_kitList)
        delete k;
    d->m_kitList.clear();
    delete d;
    m_instance = 0;
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
    d->m_writer->save(data, Core::ICore::mainWindow());
}

static bool isLoaded()
{
    return d->m_initialized;
}

bool greaterPriority(KitInformation *a, KitInformation *b)
{
    return a->priority() > b->priority();
}

void KitManager::registerKitInformation(KitInformation *ki)
{
    QTC_CHECK(!isLoaded());
    QTC_ASSERT(!d->m_informationList.contains(ki), return);

    QList<KitInformation *>::iterator it
            = qLowerBound(d->m_informationList.begin(),
                          d->m_informationList.end(), ki, greaterPriority);
    d->m_informationList.insert(it, ki);

    if (!isLoaded())
        return;

    foreach (Kit *k, kits()) {
        if (!k->hasValue(ki->id()))
            k->setValue(ki->id(), ki->defaultValue(k));
        else
            ki->fix(k);
    }

    return;
}

void KitManager::deregisterKitInformation(KitInformation *ki)
{
    QTC_CHECK(d->m_informationList.contains(ki));
    d->m_informationList.removeOne(ki);
    delete ki;
}

QSet<QString> KitManager::availablePlatforms()
{
    QSet<QString> platforms;
    foreach (const Kit *k, kits())
        platforms.unite(k->availablePlatforms());
    return platforms;
}

QString KitManager::displayNameForPlatform(const QString &platform)
{
    foreach (const Kit *k, kits()) {
        const QString displayName = k->displayNameForPlatform(platform);
        if (!displayName.isEmpty())
            return displayName;
    }
    return QString();
}

Core::FeatureSet KitManager::availableFeatures(const QString &platform)
{
    Core::FeatureSet features;
    foreach (const Kit *k, kits()) {
        QSet<QString> kitPlatforms = k->availablePlatforms();
        if (kitPlatforms.isEmpty() || kitPlatforms.contains(platform))
            features |= k->availableFeatures();
    }
    return features;
}

KitManager::KitList KitManager::restoreKits(const Utils::FileName &fileName)
{
    KitList result;

    PersistentSettingsReader reader;
    if (!reader.load(fileName)) {
        qWarning("Warning: Failed to read \"%s\", cannot restore kits!", qPrintable(fileName.toUserOutput()));
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

        Kit *k = new Kit(stMap);
        if (k->id().isValid()) {
            result.kits.append(k);
        } else {
            // If the Id is broken, then do not trust the rest of the data either.
            delete k;
            qWarning("Warning: Unable to restore kits stored in %s at position %d.",
                     qPrintable(fileName.toUserOutput()), i);
        }
    }
    const Core::Id id = Core::Id::fromSetting(data.value(QLatin1String(KIT_DEFAULT_KEY)));
    if (!id.isValid())
        return result;

    foreach (Kit *k, result.kits) {
        if (k->id() == id) {
            result.defaultKit = id;
            break;
        }
    }
    return result;
}

QList<Kit *> KitManager::kits()
{
    return d->m_kitList;
}

QList<Kit *> KitManager::matchingKits(const KitMatcher &matcher)
{
    QList<Kit *> result;
    foreach (Kit *k, d->m_kitList)
        if (matcher.matches(k))
            result.append(k);
    return result;
}

Kit *KitManager::find(Core::Id id)
{
    if (!id.isValid())
        return 0;

    return Utils::findOrDefault(kits(), Utils::equal(&Kit::id, id));
}

Kit *KitManager::find(const KitMatcher &matcher)
{
    return Utils::findOrDefault(d->m_kitList, [&matcher](Kit *k) {
        return matcher.matches(k);
    });
}

Kit *KitManager::defaultKit()
{
    return d->m_defaultKit;
}

QList<KitInformation *> KitManager::kitInformation()
{
    return d->m_informationList;
}

Internal::KitManagerConfigWidget *KitManager::createConfigWidget(Kit *k)
{
    Internal::KitManagerConfigWidget *result = new Internal::KitManagerConfigWidget(k);
    foreach (KitInformation *ki, kitInformation())
        result->addConfigWidget(ki->createConfigWidget(result->workingCopy()));

    result->updateVisibility();

    return result;
}

void KitManager::deleteKit(Kit *k)
{
    QTC_ASSERT(!KitManager::kits().contains(k), return);
    delete k;
}

void KitManager::notifyAboutUpdate(ProjectExplorer::Kit *k)
{
    if (!k || !isLoaded())
        return;

    if (d->m_kitList.contains(k))
        emit m_instance->kitUpdated(k);
    else
        emit m_instance->unmanagedKitUpdated(k);
}

bool KitManager::registerKit(ProjectExplorer::Kit *k)
{
    QTC_ASSERT(isLoaded(), return false);

    if (!k)
        return true;

    QTC_ASSERT(k->id().isValid(), return false);

    if (kits().contains(k))
        return false;

    // make sure we have all the information in our kits:
    m_instance->addKit(k);

    if (!d->m_defaultKit ||
            (!d->m_defaultKit->isValid() && k->isValid()))
        setDefaultKit(k);

    emit m_instance->kitAdded(k);
    return true;
}

void KitManager::deregisterKit(Kit *k)
{
    if (!k || !kits().contains(k))
        return;
    d->m_kitList.removeOne(k);
    if (defaultKit() == k) {
        QList<Kit *> stList = kits();
        Kit *newDefault = 0;
        foreach (Kit *cur, stList) {
            if (cur->isValid()) {
                newDefault = cur;
                break;
            }
        }
        setDefaultKit(newDefault);
    }
    emit m_instance->kitRemoved(k);
    delete k;
}

void KitManager::setDefaultKit(Kit *k)
{
    if (defaultKit() == k)
        return;
    if (k && !kits().contains(k))
        return;
    d->m_defaultKit = k;
    emit m_instance->defaultkitChanged();
}

void KitManager::addKit(Kit *k)
{
    if (!k)
        return;

    {
        KitGuard g(k);
        foreach (KitInformation *ki, d->m_informationList) {
            if (!k->hasValue(ki->id()))
                k->setValue(ki->id(), ki->defaultValue(k));
            else
                ki->fix(k);
        }
    }

    d->insertKit(k);
}

void KitInformation::addToEnvironment(const Kit *k, Utils::Environment &env) const
{
    Q_UNUSED(k);
    Q_UNUSED(env);
}

IOutputParser *KitInformation::createOutputParser(const Kit *k) const
{
    Q_UNUSED(k);
    return 0;
}

QString KitInformation::displayNamePostfix(const Kit *k) const
{
    Q_UNUSED(k);
    return QString();
}

QSet<QString> KitInformation::availablePlatforms(const Kit *k) const
{
    Q_UNUSED(k);
    return QSet<QString>();
}

QString KitInformation::displayNameForPlatform(const Kit *k, const QString &platform) const
{
    Q_UNUSED(k);
    Q_UNUSED(platform);
    return QString();
}

Core::FeatureSet KitInformation::availableFeatures(const Kit *k) const
{
    Q_UNUSED(k);
    return Core::FeatureSet();
}

void KitInformation::notifyAboutUpdate(Kit *k)
{
    if (k)
        k->kitUpdated();
}

// --------------------------------------------------------------------
// KitFeatureProvider:
// --------------------------------------------------------------------

Core::FeatureSet Internal::KitFeatureProvider::availableFeatures(const QString &platform) const
{
    return KitManager::availableFeatures(platform);
}

QStringList Internal::KitFeatureProvider::availablePlatforms() const
{
    return KitManager::availablePlatforms().toList();
}

QString Internal::KitFeatureProvider::displayNameForPlatform(const QString &string) const
{
    return KitManager::displayNameForPlatform(string);
}

} // namespace ProjectExplorer
