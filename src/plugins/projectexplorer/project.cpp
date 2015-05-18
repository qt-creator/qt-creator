/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "project.h"

#include "buildinfo.h"
#include "buildconfiguration.h"
#include "editorconfiguration.h"
#include "projectexplorer.h"
#include "target.h"
#include "settingsaccessor.h"

#include <coreplugin/idocument.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kitmanager.h>

#include <utils/algorithm.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <limits>

/*!
    \class ProjectExplorer::Project

    \brief The Project class implements a project node in the project explorer.
*/

/*!
   \fn void ProjectExplorer::Project::environmentChanged()

   A convenience signal emitted if activeBuildConfiguration emits
   environmentChanged or if the active build configuration changes
   (including due to the active target changing).
*/

/*!
   \fn void ProjectExplorer::Project::buildConfigurationEnabledChanged()

   A convenience signal emitted if activeBuildConfiguration emits
   isEnabledChanged() or if the active build configuration changes
   (including due to the active target changing).
*/

namespace {
const char ACTIVE_TARGET_KEY[] = "ProjectExplorer.Project.ActiveTarget";
const char TARGET_KEY_PREFIX[] = "ProjectExplorer.Project.Target.";
const char TARGET_COUNT_KEY[] = "ProjectExplorer.Project.TargetCount";
const char EDITOR_SETTINGS_KEY[] = "ProjectExplorer.Project.EditorSettings";
const char PLUGIN_SETTINGS_KEY[] = "ProjectExplorer.Project.PluginSettings";
} // namespace

namespace ProjectExplorer {
// -------------------------------------------------------------------------
// Project
// -------------------------------------------------------------------------

class ProjectPrivate
{
public:
    ProjectPrivate();
    ~ProjectPrivate();

    Core::Id m_id;
    QList<Target *> m_targets;
    Target *m_activeTarget;
    EditorConfiguration m_editorConfiguration;
    Core::Context m_projectContext;
    Core::Context m_projectLanguages;
    QVariantMap m_pluginSettings;
    Internal::UserFileAccessor *m_accessor;

    KitMatcher m_requiredKitMatcher;
    KitMatcher m_preferredKitMatcher;

    Utils::MacroExpander m_macroExpander;
};

ProjectPrivate::ProjectPrivate() :
    m_activeTarget(0),
    m_accessor(0)
{ }

ProjectPrivate::~ProjectPrivate()
{ delete m_accessor; }

Project::Project() : d(new ProjectPrivate)
{
    d->m_macroExpander.setDisplayName(tr("Project"));
    d->m_macroExpander.registerVariable("Project:Name", tr("Project Name"),
            [this] { return displayName(); });
}

Project::~Project()
{
    qDeleteAll(d->m_targets);
    delete d;
}

Core::Id Project::id() const
{
    QTC_CHECK(d->m_id.isValid());
    return d->m_id;
}

Utils::FileName Project::projectFilePath() const
{
    return document()->filePath();
}

bool Project::hasActiveBuildSettings() const
{
    return activeTarget() && IBuildConfigurationFactory::find(activeTarget());
}

QString Project::makeUnique(const QString &preferredName, const QStringList &usedNames)
{
    if (!usedNames.contains(preferredName))
        return preferredName;
    int i = 2;
    QString tryName = preferredName + QString::number(i);
    while (usedNames.contains(tryName))
        tryName = preferredName + QString::number(++i);
    return tryName;
}

void Project::changeEnvironment()
{
    Target *t = qobject_cast<Target *>(sender());
    if (t == activeTarget())
        emit environmentChanged();
}

void Project::changeBuildConfigurationEnabled()
{
    Target *t = qobject_cast<Target *>(sender());
    if (t == activeTarget())
        emit buildConfigurationEnabledChanged();
}

void Project::addTarget(Target *t)
{
    QTC_ASSERT(t && !d->m_targets.contains(t), return);
    QTC_ASSERT(!target(t->kit()), return);
    Q_ASSERT(t->project() == this);

    t->setDefaultDisplayName(t->displayName());

    // add it
    d->m_targets.push_back(t);
    connect(t, SIGNAL(environmentChanged()),
            SLOT(changeEnvironment()));
    connect(t, SIGNAL(buildConfigurationEnabledChanged()),
            this, SLOT(changeBuildConfigurationEnabled()));
    connect(t, SIGNAL(buildDirectoryChanged()),
            this, SLOT(onBuildDirectoryChanged()));
    emit addedTarget(t);

    // check activeTarget:
    if (activeTarget() == 0)
        setActiveTarget(t);
}

bool Project::removeTarget(Target *target)
{
    if (!target || !d->m_targets.contains(target))
        return false;

    if (BuildManager::isBuilding(target))
        return false;

    if (target == activeTarget()) {
        if (d->m_targets.size() == 1)
            setActiveTarget(0);
        else if (d->m_targets.first() == target)
            setActiveTarget(d->m_targets.at(1));
        else
            setActiveTarget(d->m_targets.at(0));
    }

    emit aboutToRemoveTarget(target);
    d->m_targets.removeOne(target);
    emit removedTarget(target);

    delete target;
    return true;
}

QList<Target *> Project::targets() const
{
    return d->m_targets;
}

Target *Project::activeTarget() const
{
    return d->m_activeTarget;
}

void Project::setActiveTarget(Target *target)
{
    if ((!target && !d->m_targets.isEmpty()) ||
        (target && d->m_targets.contains(target) && d->m_activeTarget != target)) {
        d->m_activeTarget = target;
        emit activeTargetChanged(d->m_activeTarget);
        emit environmentChanged();
        emit buildConfigurationEnabledChanged();
    }
}

Target *Project::target(Core::Id id) const
{
    return Utils::findOrDefault(d->m_targets, Utils::equal(&Target::id, id));
}

Target *Project::target(Kit *k) const
{
    return Utils::findOrDefault(d->m_targets, Utils::equal(&Target::kit, k));
}

bool Project::supportsKit(Kit *k, QString *errorMessage) const
{
    Q_UNUSED(k);
    Q_UNUSED(errorMessage);
    return true;
}

Target *Project::createTarget(Kit *k)
{
    if (!k || target(k))
        return 0;

    Target *t = new Target(this, k);
    if (!setupTarget(t)) {
        delete t;
        return 0;
    }
    return t;
}

bool Project::setupTarget(Target *t)
{
    t->updateDefaultBuildConfigurations();
    t->updateDefaultDeployConfigurations();
    t->updateDefaultRunConfigurations();
    return true;
}

void Project::setId(Core::Id id)
{
    d->m_id = id;
}

Target *Project::restoreTarget(const QVariantMap &data)
{
    Core::Id id = idFromMap(data);
    if (target(id)) {
        qWarning("Warning: Duplicated target id found, not restoring second target with id '%s'. Continuing.",
                 qPrintable(id.toString()));
        return 0;
    }

    Kit *k = KitManager::find(id);
    if (!k) {
        qWarning("Warning: No kit '%s' found. Continuing.", qPrintable(id.toString()));
        return 0;
    }

    Target *t = new Target(this, k);
    if (!t->fromMap(data)) {
        delete t;
        return 0;
    }

    return t;
}

void Project::saveSettings()
{
    emit aboutToSaveSettings();
    if (!d->m_accessor)
        d->m_accessor = new Internal::UserFileAccessor(this);
    if (!targets().isEmpty())
        d->m_accessor->saveSettings(toMap(), Core::ICore::mainWindow());
}

Project::RestoreResult Project::restoreSettings(QString *errorMessage)
{
    if (!d->m_accessor)
        d->m_accessor = new Internal::UserFileAccessor(this);
    QVariantMap map(d->m_accessor->restoreSettings(Core::ICore::mainWindow()));
    RestoreResult result = fromMap(map, errorMessage);
    if (result == RestoreResult::Ok)
        emit settingsLoaded();
    return result;
}


/*!
    Serializes all data into a QVariantMap.

    This map is then saved in the .user file of the project.
    Just put all your data into the map.

    \note Do not forget to call your base class' toMap function.
    \note Do not forget to call setActiveBuildConfiguration when
    creating new build configurations.
*/

QVariantMap Project::toMap() const
{
    const QList<Target *> ts = targets();

    QVariantMap map;
    map.insert(QLatin1String(ACTIVE_TARGET_KEY), ts.indexOf(d->m_activeTarget));
    map.insert(QLatin1String(TARGET_COUNT_KEY), ts.size());
    for (int i = 0; i < ts.size(); ++i)
        map.insert(QString::fromLatin1(TARGET_KEY_PREFIX) + QString::number(i), ts.at(i)->toMap());

    map.insert(QLatin1String(EDITOR_SETTINGS_KEY), d->m_editorConfiguration.toMap());
    map.insert(QLatin1String(PLUGIN_SETTINGS_KEY), d->m_pluginSettings);

    return map;
}

Utils::FileName Project::projectDirectory() const
{
    return projectDirectory(projectFilePath());
}

Utils::FileName Project::projectDirectory(const Utils::FileName &top)
{
    if (top.isEmpty())
        return Utils::FileName();
    return Utils::FileName::fromString(top.toFileInfo().absoluteDir().path());
}


Project::RestoreResult Project::fromMap(const QVariantMap &map, QString *errorMessage)
{
    if (map.contains(QLatin1String(EDITOR_SETTINGS_KEY))) {
        QVariantMap values(map.value(QLatin1String(EDITOR_SETTINGS_KEY)).toMap());
        d->m_editorConfiguration.fromMap(values);
    }

    if (map.contains(QLatin1String(PLUGIN_SETTINGS_KEY)))
        d->m_pluginSettings = map.value(QLatin1String(PLUGIN_SETTINGS_KEY)).toMap();

    bool ok;
    int maxI(map.value(QLatin1String(TARGET_COUNT_KEY), 0).toInt(&ok));
    if (!ok || maxI < 0)
        maxI = 0;
    int active(map.value(QLatin1String(ACTIVE_TARGET_KEY), 0).toInt(&ok));
    if (!ok || active < 0 || active >= maxI)
        active = 0;

    for (int i = 0; i < maxI; ++i) {
        const QString key(QString::fromLatin1(TARGET_KEY_PREFIX) + QString::number(i));
        if (!map.contains(key)) {
            if (errorMessage)
                *errorMessage = tr("Target key %1 was not found in project settings.").arg(key);
            return RestoreResult::Error;
        }
        QVariantMap targetMap = map.value(key).toMap();

        Target *t = restoreTarget(targetMap);
        if (!t)
            continue;

        addTarget(t);
        if (i == active)
            setActiveTarget(t);
    }

    return RestoreResult::Ok;
}

EditorConfiguration *Project::editorConfiguration() const
{
    return &d->m_editorConfiguration;
}

QString Project::generatedUiHeader(const Utils::FileName & /* formFile */) const
{
    return QString();
}

void Project::setProjectContext(Core::Context context)
{
    if (d->m_projectContext == context)
        return;
    d->m_projectContext = context;
    emit projectContextUpdated();
}

void Project::setProjectLanguages(Core::Context language)
{
    if (d->m_projectLanguages == language)
        return;
    d->m_projectLanguages = language;
    emit projectLanguagesUpdated();
}

void Project::addProjectLanguage(Core::Id id)
{
    Core::Context lang = projectLanguages();
    int pos = lang.indexOf(id);
    if (pos < 0)
        lang.add(id);
    setProjectLanguages(lang);
}

void Project::removeProjectLanguage(Core::Id id)
{
    Core::Context lang = projectLanguages();
    int pos = lang.indexOf(id);
    if (pos >= 0)
        lang.removeAt(pos);
    setProjectLanguages(lang);
}

void Project::setProjectLanguage(Core::Id id, bool enabled)
{
    if (enabled)
        addProjectLanguage(id);
    else
        removeProjectLanguage(id);
}

Core::Context Project::projectContext() const
{
    return d->m_projectContext;
}

Core::Context Project::projectLanguages() const
{
    return d->m_projectLanguages;
}

QVariant Project::namedSettings(const QString &name) const
{
    return d->m_pluginSettings.value(name);
}

void Project::setNamedSettings(const QString &name, const QVariant &value)
{
    if (value.isNull())
        d->m_pluginSettings.remove(name);
    else
        d->m_pluginSettings.insert(name, value);
}

bool Project::needsConfiguration() const
{
    return false;
}

void Project::configureAsExampleProject(const QStringList &platforms)
{
    Q_UNUSED(platforms);
}

bool Project::requiresTargetPanel() const
{
    return true;
}

bool Project::needsSpecialDeployment() const
{
    return false;
}

void Project::setup(QList<const BuildInfo *> infoList)
{
    QList<Target *> toRegister;
    foreach (const BuildInfo *info, infoList) {
        Kit *k = KitManager::find(info->kitId);
        if (!k)
            continue;
        Target *t = target(k);
        if (!t) {
            t = Utils::findOrDefault(toRegister, Utils::equal(&Target::kit, k));
        }
        if (!t) {
            t = new Target(this, k);
            toRegister << t;
        }

        BuildConfiguration *bc = info->factory()->create(t, info);
        if (!bc)
            continue;
        t->addBuildConfiguration(bc);
    }
    foreach (Target *t, toRegister) {
        t->updateDefaultDeployConfigurations();
        t->updateDefaultRunConfigurations();
        addTarget(t);
    }
}

Utils::MacroExpander *Project::macroExpander() const
{
    return &d->m_macroExpander;
}

ProjectImporter *Project::createProjectImporter() const
{
    return 0;
}

KitMatcher Project::requiredKitMatcher() const
{
    return d->m_requiredKitMatcher;
}

void Project::setRequiredKitMatcher(const KitMatcher &matcher)
{
    d->m_requiredKitMatcher = matcher;
}

KitMatcher Project::preferredKitMatcher() const
{
    return d->m_preferredKitMatcher;
}

void Project::setPreferredKitMatcher(const KitMatcher &matcher)
{
    d->m_preferredKitMatcher = matcher;
}

void Project::onBuildDirectoryChanged()
{
    Target *target = qobject_cast<Target *>(sender());
    if (target && target == activeTarget())
        emit buildDirectoryChanged();
}

} // namespace ProjectExplorer
