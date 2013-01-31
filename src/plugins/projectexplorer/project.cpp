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

#include "project.h"

#include "buildconfiguration.h"
#include "deployconfiguration.h"
#include "editorconfiguration.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectnodes.h"
#include "runconfiguration.h"
#include "target.h"
#include "settingsaccessor.h"

#include <coreplugin/idocument.h>
#include <coreplugin/icontext.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <limits>
#include <utils/qtcassert.h>
#include <utils/environment.h>

/*!
    \class ProjectExplorer::Project

    \brief A project.
*/

/*!
   \fn void ProjectExplorer::Project::environmentChanged()

   \brief Convenience signal emitted if the activeBuildConfiguration emits environmentChanged
   or if the activeBuildConfiguration changes (including due to the active target changing).
*/

/*!
   \fn void ProjectExplorer::Project::buildConfigurationEnabledChanged()

   \brief Convenience signal emitted if the activeBuildConfiguration emits isEnabledChanged()
   or if the activeBuildConfiguration changes (including due to the active target changing).
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

    QList<Target *> m_targets;
    Target *m_activeTarget;
    EditorConfiguration *m_editorConfiguration;
    Core::Context m_projectContext;
    Core::Context m_projectLanguage;
    QVariantMap m_pluginSettings;
    SettingsAccessor *m_accessor;
};

ProjectPrivate::ProjectPrivate() :
    m_activeTarget(0),
    m_editorConfiguration(new EditorConfiguration()),
    m_accessor(0)
{ }

ProjectPrivate::~ProjectPrivate()
{ delete m_accessor; }

Project::Project() : d(new ProjectPrivate)
{ }

Project::~Project()
{
    qDeleteAll(d->m_targets);
    delete d->m_editorConfiguration;
    delete d;
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

    ProjectExplorer::BuildManager *bm =
            ProjectExplorer::ProjectExplorerPlugin::instance()->buildManager();
    if (bm->isBuilding(target))
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

Target *Project::target(const Core::Id id) const
{
    foreach (Target *target, d->m_targets) {
        if (target->id() == id)
            return target;
    }
    return 0;
}

Target *Project::target(Kit *k) const
{
    foreach (Target *target, d->m_targets) {
        if (target->kit() == k)
            return target;
    }
    return 0;
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

Target *Project::restoreTarget(const QVariantMap &data)
{
    Core::Id id = idFromMap(data);
    if (target(id)) {
        qWarning("Warning: Duplicated target id found, not restoring second target with id '%s'. Continuing.",
                 qPrintable(id.toString()));
        return 0;
    }

    Kit *k = KitManager::instance()->find(id);
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
        d->m_accessor = new SettingsAccessor(this);
    d->m_accessor->saveSettings(toMap());
}

bool Project::restoreSettings()
{
    if (!d->m_accessor)
        d->m_accessor = new SettingsAccessor(this);
    QVariantMap map(d->m_accessor->restoreSettings());
    bool ok = fromMap(map);
    if (ok)
        emit settingsLoaded();
    return ok;
}


/*!
    \brief Serialize all data into a QVariantMap.

    This map is then saved in the .user file of the project.
    Just put all your data into the map.

    \note Do not forget to call your base class' toMap method.
    \note Do not forget to call setActiveBuildConfiguration when
          creating new BuilConfigurations.
*/

QVariantMap Project::toMap() const
{
    const QList<Target *> ts = targets();

    QVariantMap map;
    map.insert(QLatin1String(ACTIVE_TARGET_KEY), ts.indexOf(d->m_activeTarget));
    map.insert(QLatin1String(TARGET_COUNT_KEY), ts.size());
    for (int i = 0; i < ts.size(); ++i)
        map.insert(QString::fromLatin1(TARGET_KEY_PREFIX) + QString::number(i), ts.at(i)->toMap());

    map.insert(QLatin1String(EDITOR_SETTINGS_KEY), d->m_editorConfiguration->toMap());
    map.insert(QLatin1String(PLUGIN_SETTINGS_KEY), d->m_pluginSettings);

    return map;
}

QString Project::projectDirectory() const
{
    return projectDirectory(document()->fileName());
}

QString Project::projectDirectory(const QString &top)
{
    if (top.isEmpty())
        return QString();
    QFileInfo info(top);
    return info.absoluteDir().path();
}


bool Project::fromMap(const QVariantMap &map)
{
    if (map.contains(QLatin1String(EDITOR_SETTINGS_KEY))) {
        QVariantMap values(map.value(QLatin1String(EDITOR_SETTINGS_KEY)).toMap());
        d->m_editorConfiguration->fromMap(values);
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
            qWarning() << key << "was not found in data.";
            return false;
        }
        QVariantMap targetMap = map.value(key).toMap();

        Target *t = restoreTarget(targetMap);
        if (!t)
            continue;

        addTarget(t);
        if (i == active)
            setActiveTarget(t);
    }

    return true;
}

EditorConfiguration *Project::editorConfiguration() const
{
    return d->m_editorConfiguration;
}

QString Project::generatedUiHeader(const QString & /* formFile */) const
{
    return QString();
}

void Project::setProjectContext(Core::Context context)
{
    d->m_projectContext = context;
}

void Project::setProjectLanguage(Core::Context language)
{
    d->m_projectLanguage = language;
}

Core::Context Project::projectContext() const
{
    return d->m_projectContext;
}

Core::Context Project::projectLanguage() const
{
    return d->m_projectLanguage;
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

bool Project::supportsNoTargetPanel() const
{
    return false;
}

void Project::onBuildDirectoryChanged()
{
    Target *target = qobject_cast<Target *>(sender());
    if (target && target == activeTarget())
        emit buildDirectoryChanged();
}

} // namespace ProjectExplorer
