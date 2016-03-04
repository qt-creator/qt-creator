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

#include "project.h"

#include "buildinfo.h"
#include "buildconfiguration.h"
#include "deployconfiguration.h"
#include "editorconfiguration.h"
#include "kit.h"
#include "projectexplorer.h"
#include "projectnodes.h"
#include "target.h"
#include "session.h"
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
    ~ProjectPrivate();

    Core::Id m_id;
    Core::IDocument *m_document = 0;
    IProjectManager *m_manager = 0;
    ProjectNode *m_rootProjectNode = 0;
    QList<Target *> m_targets;
    Target *m_activeTarget = 0;
    EditorConfiguration m_editorConfiguration;
    Core::Context m_projectContext;
    Core::Context m_projectLanguages;
    QVariantMap m_pluginSettings;
    Internal::UserFileAccessor *m_accessor = 0;

    KitMatcher m_requiredKitMatcher;
    KitMatcher m_preferredKitMatcher;

    Utils::MacroExpander m_macroExpander;
};

ProjectPrivate::~ProjectPrivate()
{
    // Make sure our root node is 0 when deleting
    ProjectNode *oldNode = m_rootProjectNode;
    m_rootProjectNode = 0;
    delete oldNode;

    delete m_document;
    delete m_accessor;
}

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

Core::IDocument *Project::document() const
{
    QTC_CHECK(d->m_document);
    return d->m_document;
}

Utils::FileName Project::projectFilePath() const
{
    QTC_ASSERT(document(), return Utils::FileName());
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
    connect(t, &Target::environmentChanged, this, &Project::changeEnvironment);
    connect(t, &Target::buildConfigurationEnabledChanged,
            this, &Project::changeBuildConfigurationEnabled);
    connect(t, &Target::buildDirectoryChanged, this, &Project::onBuildDirectoryChanged);
    emit addedTarget(t);

    // check activeTarget:
    if (activeTarget() == 0)
        setActiveTarget(t);
}

bool Project::removeTarget(Target *target)
{
    QTC_ASSERT(target && d->m_targets.contains(target), return false);

    if (BuildManager::isBuilding(target))
        return false;

    if (target == activeTarget()) {
        if (d->m_targets.size() == 1)
            SessionManager::setActiveTarget(this, 0, SetActive::Cascade);
        else if (d->m_targets.first() == target)
            SessionManager::setActiveTarget(this, d->m_targets.at(1), SetActive::Cascade);
        else
            SessionManager::setActiveTarget(this, d->m_targets.at(0), SetActive::Cascade);
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

Target *Project::cloneTarget(Target *sourceTarget, Kit *k)
{
    Target *newTarget = new Target(this, k);

    QStringList buildconfigurationError;
    QStringList deployconfigurationError;
    QStringList runconfigurationError;

    foreach (BuildConfiguration *sourceBc, sourceTarget->buildConfigurations()) {
        IBuildConfigurationFactory *factory = IBuildConfigurationFactory::find(newTarget, sourceBc);
        if (!factory) {
            buildconfigurationError << sourceBc->displayName();
            continue;
        }
        BuildConfiguration *newBc = factory->clone(newTarget, sourceBc);
        if (!newBc) {
            buildconfigurationError << sourceBc->displayName();
            continue;
        }
        newBc->setDisplayName(sourceBc->displayName());
        newTarget->addBuildConfiguration(newBc);
        if (sourceTarget->activeBuildConfiguration() == sourceBc)
            SessionManager::setActiveBuildConfiguration(newTarget, newBc, SetActive::NoCascade);
    }
    if (!newTarget->activeBuildConfiguration()) {
        QList<BuildConfiguration *> bcs = newTarget->buildConfigurations();
        if (!bcs.isEmpty())
            SessionManager::setActiveBuildConfiguration(newTarget, bcs.first(), SetActive::NoCascade);
    }

    foreach (DeployConfiguration *sourceDc, sourceTarget->deployConfigurations()) {
        DeployConfigurationFactory *factory = DeployConfigurationFactory::find(newTarget, sourceDc);
        if (!factory) {
            deployconfigurationError << sourceDc->displayName();
            continue;
        }
        DeployConfiguration *newDc = factory->clone(newTarget, sourceDc);
        if (!newDc) {
            deployconfigurationError << sourceDc->displayName();
            continue;
        }
        newDc->setDisplayName(sourceDc->displayName());
        newTarget->addDeployConfiguration(newDc);
        if (sourceTarget->activeDeployConfiguration() == sourceDc)
            SessionManager::setActiveDeployConfiguration(newTarget, newDc, SetActive::NoCascade);
    }
    if (!newTarget->activeBuildConfiguration()) {
        QList<DeployConfiguration *> dcs = newTarget->deployConfigurations();
        if (!dcs.isEmpty())
            SessionManager::setActiveDeployConfiguration(newTarget, dcs.first(), SetActive::NoCascade);
    }

    foreach (RunConfiguration *sourceRc, sourceTarget->runConfigurations()) {
        IRunConfigurationFactory *factory = IRunConfigurationFactory::find(newTarget, sourceRc);
        if (!factory) {
            runconfigurationError << sourceRc->displayName();
            continue;
        }
        RunConfiguration *newRc = factory->clone(newTarget, sourceRc);
        if (!newRc) {
            runconfigurationError << sourceRc->displayName();
            continue;
        }
        newRc->setDisplayName(sourceRc->displayName());
        newTarget->addRunConfiguration(newRc);
        if (sourceTarget->activeRunConfiguration() == sourceRc)
            newTarget->setActiveRunConfiguration(newRc);
    }
    if (!newTarget->activeRunConfiguration()) {
        QList<RunConfiguration *> rcs = newTarget->runConfigurations();
        if (!rcs.isEmpty())
            newTarget->setActiveRunConfiguration(rcs.first());
    }

    bool fatalError = false;
    if (buildconfigurationError.count() == sourceTarget->buildConfigurations().count())
        fatalError = true;

    if (deployconfigurationError.count() == sourceTarget->deployConfigurations().count())
        fatalError = true;

    if (runconfigurationError.count() == sourceTarget->runConfigurations().count())
        fatalError = true;

    if (fatalError) {
        // That could be a more granular error message
        QMessageBox::critical(Core::ICore::mainWindow(),
                              tr("Incompatible Kit"),
                              tr("Kit %1 is incompatible with kit %2.")
                              .arg(sourceTarget->kit()->displayName())
                              .arg(k->displayName()));

        delete newTarget;
        newTarget = 0;
    } else if (!buildconfigurationError.isEmpty()
               || !deployconfigurationError.isEmpty()
               || ! runconfigurationError.isEmpty()) {

        QString error;
        if (!buildconfigurationError.isEmpty())
            error += tr("Build configurations:") + QLatin1Char('\n')
                    + buildconfigurationError.join(QLatin1Char('\n'));

        if (!deployconfigurationError.isEmpty()) {
            if (!error.isEmpty())
                error.append(QLatin1Char('\n'));
            error += tr("Deploy configurations:") + QLatin1Char('\n')
                    + deployconfigurationError.join(QLatin1Char('\n'));
        }

        if (!runconfigurationError.isEmpty()) {
            if (!error.isEmpty())
                error.append(QLatin1Char('\n'));
            error += tr("Run configurations:") + QLatin1Char('\n')
                    + runconfigurationError.join(QLatin1Char('\n'));
        }

        QMessageBox msgBox(Core::ICore::mainWindow());
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle(tr("Partially Incompatible Kit"));
        msgBox.setText(tr("Some configurations could not be copied."));
        msgBox.setDetailedText(error);
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if (msgBox.exec() != QDialog::Accepted) {
            delete newTarget;
            newTarget = 0;
        }
    }

    return newTarget;
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

void Project::setDocument(Core::IDocument *doc)
{
    QTC_ASSERT(doc, return);
    QTC_ASSERT(!d->m_document, return);
    d->m_document = doc;
}

void Project::setProjectManager(IProjectManager *manager)
{
    QTC_ASSERT(manager, return);
    QTC_ASSERT(!d->m_manager, return);
    d->m_manager = manager;
}

void Project::setRootProjectNode(ProjectNode *root)
{
    ProjectNode *oldNode = d->m_rootProjectNode;
    d->m_rootProjectNode = root;
    delete oldNode;
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

/*!
    Returns the directory that contains the project.

    This includes the absolute path.
*/

Utils::FileName Project::projectDirectory() const
{
    return projectDirectory(projectFilePath());
}

/*!
    Returns the directory that contains the file \a top.

    This includes the absolute path.
*/

Utils::FileName Project::projectDirectory(const Utils::FileName &top)
{
    if (top.isEmpty())
        return Utils::FileName();
    return Utils::FileName::fromString(top.toFileInfo().absoluteDir().path());
}

IProjectManager *Project::projectManager() const
{
    QTC_CHECK(d->m_manager);
    return d->m_manager;
}

ProjectNode *Project::rootProjectNode() const
{
    return d->m_rootProjectNode;
}

Project::RestoreResult Project::fromMap(const QVariantMap &map, QString *errorMessage)
{
    Q_UNUSED(errorMessage);
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
        if (!map.contains(key))
            continue;
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

QStringList Project::filesGeneratedFrom(const QString &file) const
{
    Q_UNUSED(file);
    return QStringList();
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

void Project::projectLoaded()
{
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

void Project::configureAsExampleProject(const QSet<Core::Id> &platforms)
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
