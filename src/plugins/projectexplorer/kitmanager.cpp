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

#include "kitmanager.h"

#include "kit.h"
#include "kitconfigwidget.h"
#include "kitinformation.h"
#include "kitmanagerconfigwidget.h"
#include "project.h"

#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/persistentsettings.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

#include <QFormLayout>
#include <QLabel>

static const char KIT_DATA_KEY[] = "Profile.";
static const char KIT_COUNT_KEY[] = "Profile.Count";
static const char KIT_FILE_VERSION_KEY[] = "Version";
static const char KIT_DEFAULT_KEY[] = "Profile.Default";
static const char KIT_FILENAME[] = "/qtcreator/profiles.xml";

using Utils::PersistentSettingsWriter;
using Utils::PersistentSettingsReader;

static Utils::FileName settingsFileName()
{
    QFileInfo settingsLocation(ExtensionSystem::PluginManager::settings()->fileName());
    return Utils::FileName::fromString(settingsLocation.absolutePath() + QLatin1String(KIT_FILENAME));
}

namespace ProjectExplorer {

KitManager *KitManager::m_instance = 0;

namespace Internal {

// --------------------------------------------------------------------------
// KitManagerPrivate:
// --------------------------------------------------------------------------

class KitManagerPrivate
{
public:
    KitManagerPrivate();
    ~KitManagerPrivate();

    Kit *m_defaultKit;
    bool m_initialized;
    QList<KitInformation *> m_informationList;
    QList<Kit *> m_kitList;
    Utils::PersistentSettingsWriter *m_writer;
};

KitManagerPrivate::KitManagerPrivate()
    : m_defaultKit(0), m_initialized(false), m_writer(0)
{ }

KitManagerPrivate::~KitManagerPrivate()
{
    qDeleteAll(m_informationList);
    qDeleteAll(m_kitList);
    delete m_writer;
}

} // namespace Internal

// --------------------------------------------------------------------------
// KitManager:
// --------------------------------------------------------------------------

KitManager *KitManager::instance()
{
    return m_instance;
}

KitManager::KitManager(QObject *parent) :
    QObject(parent),
    d(new Internal::KitManagerPrivate())
{
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
            k->setup();
        }

        // SDK kits are always considered to be up for validation since they might have been
        // extended with additional information by creator in the meantime:
        kitsToValidate = system.kits;
    }

    // read all kits from user file
    KitList userKits = restoreKits(settingsFileName());
    foreach (Kit *k, userKits.kits) {
        if (k->isAutoDetected())
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
                    if (current->hasValue(ki->dataId()))
                        toStore->setValue(ki->dataId(), current->value(ki->dataId()));
                }

                delete current;
                break;
            }
        }
        addKit(toStore);
    }

    // Delete all loaded autodetected kits that were not rediscovered:
    qDeleteAll(kitsToCheck);

    // Store manual kits
    foreach (Kit *k, kitsToRegister)
        addKit(k);

    if (kits().isEmpty()) {
        Kit *defaultKit = new Kit; // One kit using default values
        defaultKit->setDisplayName(tr("Desktop"));
        defaultKit->setAutoDetected(false);
        defaultKit->setIconPath(QLatin1String(":///DESKTOP///"));

        defaultKit->setup();

        addKit(defaultKit);
    }

    Kit *k = find(userKits.defaultKit);
    if (k)
        setDefaultKit(k);

    d->m_writer = new Utils::PersistentSettingsWriter(settingsFileName(), QLatin1String("QtCreatorProfiles"));
    d->m_initialized = true;
    emit kitsChanged();
}

KitManager::~KitManager()
{
    saveKits(); // Make sure we save the current state on exit!
    // Clean out kit information to avoid calling them during deregistration:
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

bool greaterPriority(KitInformation *a, KitInformation *b)
{
    return a->priority() > b->priority();
}

void KitManager::registerKitInformation(KitInformation *ki)
{
    QList<KitInformation *>::iterator it
            = qLowerBound(d->m_informationList.begin(), d->m_informationList.end(), ki, greaterPriority);
    d->m_informationList.insert(it, ki);

    connect(ki, SIGNAL(validationNeeded()), this, SLOT(validateKits()));

    if (!d->m_initialized)
        return;

    foreach (Kit *k, kits()) {
        if (!k->hasValue(ki->dataId()))
            k->setValue(ki->dataId(), ki->defaultValue(k));
        else
            ki->fix(k);
    }

    return;
}

void KitManager::deregisterKitInformation(KitInformation *ki)
{
    QTC_CHECK(d->m_informationList.contains(ki));
    d->m_informationList.removeAll(ki);
    delete ki;
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

        Kit *k = new Kit;
        if (k->fromMap(stMap)) {
            result.kits.append(k);
        } else {
            delete k;
            qWarning("Warning: Unable to restore kits stored in %s at position %d.",
                     qPrintable(fileName.toUserOutput()), i);
        }
    }
    const QString defaultId = data.value(QLatin1String(KIT_DEFAULT_KEY)).toString();
    if (defaultId.isEmpty())
        return result;

    const Core::Id id = Core::Id(defaultId);
    foreach (Kit *k, result.kits) {
        if (k->id() == id) {
            result.defaultKit = id;
            break;
        }
    }
    return result;
}

QList<Kit *> KitManager::kits(const KitMatcher *m) const
{
    if (!d->m_initialized)
        const_cast<KitManager *>(this)->restoreKits();

    QList<Kit *> result;
    foreach (Kit *k, d->m_kitList) {
        if (!m || m->matches(k))
            result.append(k);
    }
    return result;
}

Kit *KitManager::find(const Core::Id &id) const
{
    if (!id.isValid())
        return 0;

    foreach (Kit *k, kits()) {
        if (k->id() == id)
            return k;
    }
    return 0;
}

Kit *KitManager::find(const KitMatcher *m) const
{
    QList<Kit *> matched = kits(m);
    return matched.isEmpty() ? 0 : matched.first();
}

Kit *KitManager::defaultKit() const
{
    if (!d->m_initialized)
        const_cast<KitManager *>(this)->restoreKits();
    return d->m_defaultKit;
}

QList<KitInformation *> KitManager::kitInformation() const
{
    return d->m_informationList;
}

Internal::KitManagerConfigWidget *KitManager::createConfigWidget(Kit *k) const
{
    Internal::KitManagerConfigWidget *result = new Internal::KitManagerConfigWidget(k);
    foreach (KitInformation *ki, d->m_informationList)
        result->addConfigWidget(ki->createConfigWidget(result->workingCopy()));

    result->updateVisibility();

    return result;
}

void KitManager::notifyAboutUpdate(ProjectExplorer::Kit *k)
{
    if (!k)
        return;
    if (kits().contains(k) && d->m_initialized)
        emit kitUpdated(k);
    else
        emit unmanagedKitUpdated(k);
}

bool KitManager::registerKit(ProjectExplorer::Kit *k)
{
    if (!k)
        return true;
    foreach (Kit *current, kits()) {
        if (k == current)
            return false;
    }

    // make sure we have all the information in our kits:
    addKit(k);
    if (d->m_initialized)
        emit kitAdded(k);
    return true;
}

void KitManager::deregisterKit(Kit *k)
{
    if (!k || !kits().contains(k))
        return;
    d->m_kitList.removeOne(k);
    if (d->m_defaultKit == k) {
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
    if (d->m_initialized)
        emit kitRemoved(k);
    delete k;
}

void KitManager::setDefaultKit(Kit *k)
{
    if (d->m_defaultKit == k)
        return;
    if (k && !kits().contains(k))
        return;
    d->m_defaultKit = k;
    if (d->m_initialized)
        emit defaultkitChanged();
}

void KitManager::validateKits()
{
    foreach (Kit *k, kits())
        k->validate();
}

void KitManager::addKit(Kit *k)
{
    if (!k)
        return;

    {
        KitGuard g(k);
        foreach (KitInformation *ki, d->m_informationList) {
            if (!k->hasValue(ki->dataId()))
                k->setValue(ki->dataId(), ki->defaultValue(k));
            else
                ki->fix(k);
        }
    }

    d->m_kitList.append(k);
    if (!d->m_defaultKit ||
            (!d->m_defaultKit->isValid() && k->isValid()))
        setDefaultKit(k);
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

void KitInformation::notifyAboutUpdate(Kit *k)
{
    KitManager::instance()->notifyAboutUpdate(k);
}

} // namespace ProjectExplorer
