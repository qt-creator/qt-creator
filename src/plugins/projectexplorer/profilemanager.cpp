/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "profilemanager.h"

#include "profile.h"
#include "profileconfigwidget.h"
#include "profilemanagerconfigwidget.h"
#include "project.h"

#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/persistentsettings.h>
#include <utils/environment.h>

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

#include <QFormLayout>
#include <QLabel>
#include <QMainWindow>

static const char PROFILE_DATA_KEY[] = "Profile.";
static const char PROFILE_COUNT_KEY[] = "Profile.Count";
static const char PROFILE_FILE_VERSION_KEY[] = "Version";
static const char PROFILE_DEFAULT_KEY[] = "Profile.Default";
static const char PROFILE_FILENAME[] = "/qtcreator/profiles.xml";

using Utils::PersistentSettingsWriter;
using Utils::PersistentSettingsReader;

static QString settingsFileName()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    QFileInfo settingsLocation(pm->settings()->fileName());
    return settingsLocation.absolutePath() + QLatin1String(PROFILE_FILENAME);
}

namespace ProjectExplorer {

ProfileManager *ProfileManager::m_instance = 0;

namespace Internal {

// --------------------------------------------------------------------------
// ProfileManagerPrivate:
// --------------------------------------------------------------------------

class ProfileManagerPrivate
{
public:
    ProfileManagerPrivate();
    ~ProfileManagerPrivate();
    QList<Task> validateProfile(Profile *p) const;

    Profile *m_defaultProfile;
    bool m_initialized;
    QList<ProfileInformation *> m_informationList;
    QList<Profile *> m_profileList;
};

ProfileManagerPrivate::ProfileManagerPrivate()
    : m_defaultProfile(0), m_initialized(false)
{ }

ProfileManagerPrivate::~ProfileManagerPrivate()
{
}

QList<Task> ProfileManagerPrivate::validateProfile(Profile *p) const
{
    Q_ASSERT(p);
    QList<Task> result;
    bool hasError = false;
    foreach (ProfileInformation *pi, m_informationList) {
        QList<Task> tmp = pi->validate(p);
        foreach (const Task &t, tmp)
            if (t.type == Task::Error)
                hasError = true;
        result << tmp;
    }
    p->setValid(!hasError);
    return result;
}

} // namespace Internal

// --------------------------------------------------------------------------
// ProfileManager:
// --------------------------------------------------------------------------

ProfileManager *ProfileManager::instance()
{
    return m_instance;
}

ProfileManager::ProfileManager(QObject *parent) :
    QObject(parent),
    d(new Internal::ProfileManagerPrivate())
{
    Q_ASSERT(!m_instance);
    m_instance = this;

    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()),
            this, SLOT(saveProfiles()));

    connect(this, SIGNAL(profileAdded(ProjectExplorer::Profile*)),
            this, SIGNAL(profilesChanged()));
    connect(this, SIGNAL(profileRemoved(ProjectExplorer::Profile*)),
            this, SIGNAL(profilesChanged()));
    connect(this, SIGNAL(profileUpdated(ProjectExplorer::Profile*)),
            this, SIGNAL(profilesChanged()));
}

void ProfileManager::restoreProfiles()
{
    QList<Profile *> stsToRegister;
    QList<Profile *> stsToCheck;

    // read all profiles from SDK
    QFileInfo systemSettingsFile(Core::ICore::settings(QSettings::SystemScope)->fileName());
    ProfileList system = restoreProfiles(systemSettingsFile.absolutePath() + QLatin1String(PROFILE_FILENAME));
    QList<Profile *> readSts = system.profiles;
    // make sure we mark these as autodetected!
    foreach (Profile *p, readSts)
        p->setAutoDetected(true);

    stsToRegister = readSts; // SDK profiles are always considered to be up-to-date, so no need to
                             // recheck them.

    // read all profile chains from user file
    ProfileList userProfiles = restoreProfiles(settingsFileName());
    readSts = userProfiles.profiles;

    foreach (Profile *p, readSts) {
        if (p->isAutoDetected())
            stsToCheck.append(p);
        else
            stsToRegister.append(p);
    }
    readSts.clear();

    // Then auto create profiles:
    QList<Profile *> detectedSts;
    Profile *defaultProfile = new Profile; // One profile using default values
    defaultProfile->setDisplayName(tr("Desktop"));
    defaultProfile->setAutoDetected(true);
    defaultProfile->setIconPath(QLatin1String(":///DESKTOP///"));

    detectedSts << defaultProfile;

    // Find/update autodetected profiles:
    Profile *toStore = 0;
    foreach (Profile *currentDetected, detectedSts) {
        toStore = currentDetected;

        // Check whether we had this profile stored and prefer the old one with the old id:
        for (int i = 0; i < stsToCheck.count(); ++i) {
            if (*(stsToCheck.at(i)) == *currentDetected) {
                toStore = stsToCheck.at(i);
                stsToCheck.removeAt(i);
                delete currentDetected;
                break;
            }
        }
        addProfile(toStore);
    }

    // Delete all loaded autodetected profiles that were not rediscovered:
    qDeleteAll(stsToCheck);

    // Store manual tool chains
    foreach (Profile *p, stsToRegister)
        addProfile(p);

    Profile *p = find(userProfiles.defaultProfile);
    if (p)
        setDefaultProfile(p);
}

ProfileManager::~ProfileManager()
{
    // Clean out profile information to avoid calling them during deregistration:
    qDeleteAll(d->m_informationList);
    qDeleteAll(d->m_profileList);
    delete d;
    m_instance = 0;
}

void ProfileManager::saveProfiles()
{
    PersistentSettingsWriter writer;
    writer.saveValue(QLatin1String(PROFILE_FILE_VERSION_KEY), 1);

    int count = 0;
    foreach (Profile *p, profiles()) {
        QVariantMap tmp = p->toMap();
        if (tmp.isEmpty())
            continue;
        writer.saveValue(QString::fromLatin1(PROFILE_DATA_KEY) + QString::number(count), tmp);
        ++count;
    }
    writer.saveValue(QLatin1String(PROFILE_COUNT_KEY), count);
    writer.saveValue(QLatin1String(PROFILE_DEFAULT_KEY),
                     d->m_defaultProfile ? QString::fromLatin1(d->m_defaultProfile->id().name()) : QString());
    writer.save(settingsFileName(), QLatin1String("QtCreatorProfiles"), Core::ICore::mainWindow());
}

bool greaterPriority(ProfileInformation *a, ProfileInformation *b)
{
    return a->priority() > b->priority();
}

void ProfileManager::registerProfileInformation(ProfileInformation *pi)
{
    QList<ProfileInformation *>::iterator it
            = qLowerBound(d->m_informationList.begin(), d->m_informationList.end(), pi, greaterPriority);
    d->m_informationList.insert(it, pi);

    connect(pi, SIGNAL(validationNeeded()), this, SLOT(validateProfiles()));

    if (!d->m_initialized)
        return;

    foreach (Profile *p, profiles()) {
        if (!p->hasValue(pi->dataId()))
            p->setValue(pi->dataId(), pi->defaultValue(p));
    }

    return;
}

void ProfileManager::deregisterProfileInformation(ProfileInformation *pi)
{
    Q_ASSERT(d->m_informationList.contains(pi));
    d->m_informationList.removeAll(pi);
    delete pi;
}

ProfileManager::ProfileList ProfileManager::restoreProfiles(const QString &fileName)
{
    ProfileList result;

    PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return result;
    QVariantMap data = reader.restoreValues();

    // Check version:
    int version = data.value(QLatin1String(PROFILE_FILE_VERSION_KEY), 0).toInt();
    if (version < 1)
        return result;

    const int count = data.value(QLatin1String(PROFILE_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(PROFILE_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;

        const QVariantMap stMap = data.value(key).toMap();

        Profile *p = new Profile;
        if (p->fromMap(stMap)) {
            result.profiles.append(p);
        } else {
            delete p;
            qWarning("Warning: Unable to restore profiles stored in %s at position %d.",
                     qPrintable(QDir::toNativeSeparators(fileName)), i);
        }
    }
    const QString defaultId = data.value(QLatin1String(PROFILE_DEFAULT_KEY)).toString();
    if (defaultId.isEmpty())
        return result;

    const Core::Id id = Core::Id(defaultId);
    foreach (Profile *i, result.profiles) {
        if (i->id() == id) {
            result.defaultProfile = id;
            break;
        }
    }
    return result;
}

QList<Profile *> ProfileManager::profiles(const ProfileMatcher *m) const
{
    if (!d->m_initialized) {
        d->m_initialized = true;
        const_cast<ProfileManager *>(this)->restoreProfiles();
    }

    QList<Profile *> result;
    foreach (Profile *p, d->m_profileList) {
        if (!m || m->matches(p))
            result.append(p);
    }
    return result;
}

Profile *ProfileManager::find(const Core::Id &id) const
{
    if (!id.isValid())
        return 0;

    foreach (Profile *p, profiles()) {
        if (p->id() == id)
            return p;
    }
    return 0;
}

Profile *ProfileManager::find(const ProfileMatcher *m) const
{
    QList<Profile *> matched = profiles(m);
    return matched.isEmpty() ? 0 : matched.first();
}

Profile *ProfileManager::defaultProfile()
{
    if (!d->m_initialized) {
        d->m_initialized = true;
        restoreProfiles();
    }
    return d->m_defaultProfile;
}

QList<ProfileInformation *> ProfileManager::profileInformation() const
{
    return d->m_informationList;
}

ProfileConfigWidget *ProfileManager::createConfigWidget(Profile *p) const
{
    if (!p)
        return 0;

    Internal::ProfileManagerConfigWidget *result = new Internal::ProfileManagerConfigWidget(p);
    foreach (ProfileInformation *pi, d->m_informationList)
        result->addConfigWidget(pi->createConfigWidget(p));

    return result;
}

void ProfileManager::notifyAboutUpdate(ProjectExplorer::Profile *p)
{
    if (!p || !profiles().contains(p))
        return;
    d->validateProfile(p);
    emit profileUpdated(p);
}

bool ProfileManager::registerProfile(ProjectExplorer::Profile *p)
{
    if (!p)
        return true;
    foreach (Profile *current, profiles()) {
        if (p == current)
            return false;
    }

    // Make name unique:
    QStringList names;
    foreach (Profile *tmp, profiles())
        names << tmp->displayName();
    p->setDisplayName(Project::makeUnique(p->displayName(), names));

    // make sure we have all the information in our profiles:
    foreach (ProfileInformation *pi, d->m_informationList) {
        if (!p->hasValue(pi->dataId()))
            p->setValue(pi->dataId(), pi->defaultValue(p));
    }

    addProfile(p);
    emit profileAdded(p);
    return true;
}

void ProfileManager::deregisterProfile(Profile *p)
{
    if (!p || !profiles().contains(p))
        return;
    d->m_profileList.removeOne(p);
    if (d->m_defaultProfile == p) {
        QList<Profile *> stList = profiles();
        Profile *newDefault = 0;
        foreach (Profile *cur, stList) {
            if (cur->isValid()) {
                newDefault = cur;
                break;
            }
        }
        setDefaultProfile(newDefault);
    }
    emit profileRemoved(p);
    delete p;
}

QList<Task> ProfileManager::validateProfile(Profile *p)
{
    QList<Task> result = d->validateProfile(p);
    qSort(result);
    return result;
}

void ProfileManager::setDefaultProfile(Profile *p)
{
    if (d->m_defaultProfile == p)
        return;
    if (p && !profiles().contains(p))
        return;
    d->m_defaultProfile = p;
    emit defaultProfileChanged();
}

void ProfileManager::validateProfiles()
{
    foreach (Profile *p, profiles())
        d->validateProfile(p);
}

void ProfileManager::addProfile(Profile *p)
{
    if (!p)
        return;
    d->validateProfile(p);
    d->m_profileList.append(p);
    if (!d->m_defaultProfile ||
            (!d->m_defaultProfile->isValid() && p->isValid()))
        setDefaultProfile(p);
}


void ProfileInformation::addToEnvironment(const Profile *p, Utils::Environment &env) const
{
    Q_UNUSED(p);
    Q_UNUSED(env);
}

} // namespace ProjectExplorer
