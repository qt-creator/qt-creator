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

#include "buildconfiguration.h"

#include "buildsteplist.h"
#include "projectexplorer.h"
#include "kitmanager.h"
#include "target.h"
#include "project.h"
#include "kit.h"

#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacroexpander.h>
#include <projectexplorer/target.h>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/idocument.h>

#include <utils/qtcassert.h>
#include <utils/macroexpander.h>
#include <utils/algorithm.h>

#include <QDebug>

static const char BUILD_STEP_LIST_COUNT[] = "ProjectExplorer.BuildConfiguration.BuildStepListCount";
static const char BUILD_STEP_LIST_PREFIX[] = "ProjectExplorer.BuildConfiguration.BuildStepList.";
static const char CLEAR_SYSTEM_ENVIRONMENT_KEY[] = "ProjectExplorer.BuildConfiguration.ClearSystemEnvironment";
static const char USER_ENVIRONMENT_CHANGES_KEY[] = "ProjectExplorer.BuildConfiguration.UserEnvironmentChanges";
static const char BUILDDIRECTORY_KEY[] = "ProjectExplorer.BuildConfiguration.BuildDirectory";

namespace ProjectExplorer {

BuildConfiguration::BuildConfiguration(Target *target, Core::Id id) :
    ProjectConfiguration(target),
    m_clearSystemEnvironment(false)
{
    initialize(id);
    Q_ASSERT(target);
    auto bsl = new BuildStepList(this, Core::Id(Constants::BUILDSTEPS_BUILD));
    //: Display name of the build build step list. Used as part of the labels in the project window.
    bsl->setDefaultDisplayName(tr("Build"));
    m_stepLists.append(bsl);
    bsl = new BuildStepList(this, Core::Id(Constants::BUILDSTEPS_CLEAN));
    //: Display name of the clean build step list. Used as part of the labels in the project window.
    bsl->setDefaultDisplayName(tr("Clean"));
    m_stepLists.append(bsl);

    updateCacheAndEmitEnvironmentChanged();

    connect(target, &Target::kitChanged,
            this, &BuildConfiguration::handleKitUpdate);
    connect(this, &BuildConfiguration::environmentChanged,
            this, &BuildConfiguration::emitBuildDirectoryChanged);

    ctor();
}

BuildConfiguration::BuildConfiguration(Target *target, BuildConfiguration *source) :
    ProjectConfiguration(target),
    m_clearSystemEnvironment(source->m_clearSystemEnvironment),
    m_userEnvironmentChanges(source->m_userEnvironmentChanges),
    m_buildDirectory(source->m_buildDirectory)
{
    copyFrom(source);
    Q_ASSERT(target);
    // Do not clone stepLists here, do that in the derived constructor instead
    // otherwise BuildStepFactories might reject to set up a BuildStep for us
    // since we are not yet the derived class!

    updateCacheAndEmitEnvironmentChanged();

    connect(target, &Target::kitChanged,
            this, &BuildConfiguration::handleKitUpdate);

    ctor();
}

void BuildConfiguration::ctor()
{
    Utils::MacroExpander *expander = macroExpander();
    expander->setDisplayName(tr("Build Settings"));
    expander->setAccumulating(true);
    expander->registerSubProvider([this] { return target()->macroExpander(); });

    expander->registerVariable("buildDir", tr("Build directory"),
            [this] { return buildDirectory().toUserOutput(); });

    expander->registerVariable(Constants::VAR_CURRENTBUILD_NAME, tr("Name of current build"),
            [this] { return displayName(); }, false);

    expander->registerPrefix(Constants::VAR_CURRENTBUILD_ENV,
                             tr("Variables in the current build environment"),
                             [this](const QString &var) { return environment().value(var); });
}

Utils::FileName BuildConfiguration::buildDirectory() const
{
    QString path = QDir::cleanPath(environment().expandVariables(m_buildDirectory.toString()));
    return Utils::FileName::fromString(QDir::cleanPath(QDir(target()->project()->projectDirectory().toString()).absoluteFilePath(path)));
}

Utils::FileName BuildConfiguration::rawBuildDirectory() const
{
    return m_buildDirectory;
}

void BuildConfiguration::setBuildDirectory(const Utils::FileName &dir)
{
    if (dir == m_buildDirectory)
        return;
    m_buildDirectory = dir;
    emitBuildDirectoryChanged();
}

QList<NamedWidget *> BuildConfiguration::createSubConfigWidgets()
{
    return QList<NamedWidget *>() << new BuildEnvironmentWidget(this);
}

QList<Core::Id> BuildConfiguration::knownStepLists() const
{
    return Utils::transform(m_stepLists, &BuildStepList::id);
}

BuildStepList *BuildConfiguration::stepList(Core::Id id) const
{
    return Utils::findOrDefault(m_stepLists, Utils::equal(&BuildStepList::id, id));
}

QVariantMap BuildConfiguration::toMap() const
{
    QVariantMap map(ProjectConfiguration::toMap());
    map.insert(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY), m_clearSystemEnvironment);
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY), Utils::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    map.insert(QLatin1String(BUILDDIRECTORY_KEY), m_buildDirectory.toString());

    map.insert(QLatin1String(BUILD_STEP_LIST_COUNT), m_stepLists.count());
    for (int i = 0; i < m_stepLists.count(); ++i)
        map.insert(QLatin1String(BUILD_STEP_LIST_PREFIX) + QString::number(i), m_stepLists.at(i)->toMap());

    return map;
}

bool BuildConfiguration::fromMap(const QVariantMap &map)
{
    m_clearSystemEnvironment = map.value(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY)).toBool();
    m_userEnvironmentChanges = Utils::EnvironmentItem::fromStringList(map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());
    m_buildDirectory = Utils::FileName::fromString(map.value(QLatin1String(BUILDDIRECTORY_KEY)).toString());

    updateCacheAndEmitEnvironmentChanged();

    qDeleteAll(m_stepLists);
    m_stepLists.clear();

    int maxI = map.value(QLatin1String(BUILD_STEP_LIST_COUNT), 0).toInt();
    for (int i = 0; i < maxI; ++i) {
        QVariantMap data = map.value(QLatin1String(BUILD_STEP_LIST_PREFIX) + QString::number(i)).toMap();
        if (data.isEmpty()) {
            qWarning() << "No data for build step list" << i << "found!";
            continue;
        }
        auto list = new BuildStepList(this, Core::Id());
        if (!list->fromMap(data)) {
            qWarning() << "Failed to restore build step list" << i;
            delete list;
            return false;
        }
        if (list->id() == Constants::BUILDSTEPS_BUILD)
            list->setDefaultDisplayName(tr("Build"));
        else if (list->id() == Constants::BUILDSTEPS_CLEAN)
            list->setDefaultDisplayName(tr("Clean"));
        m_stepLists.append(list);
    }

    // We currently assume there to be at least a clean and build list!
    QTC_CHECK(knownStepLists().contains(Core::Id(Constants::BUILDSTEPS_BUILD)));
    QTC_CHECK(knownStepLists().contains(Core::Id(Constants::BUILDSTEPS_CLEAN)));

    return ProjectConfiguration::fromMap(map);
}

void BuildConfiguration::updateCacheAndEmitEnvironmentChanged()
{
    Utils::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    if (env == m_cachedEnvironment)
        return;
    m_cachedEnvironment = env;
    emit environmentChanged(); // might trigger buildDirectoryChanged signal!
}

void BuildConfiguration::handleKitUpdate()
{
    updateCacheAndEmitEnvironmentChanged();
}

void BuildConfiguration::emitBuildDirectoryChanged()
{
    if (buildDirectory() != m_lastEmmitedBuildDirectory) {
        m_lastEmmitedBuildDirectory = buildDirectory();
        emit buildDirectoryChanged();
    }
}

Target *BuildConfiguration::target() const
{
    return static_cast<Target *>(parent());
}

Project *BuildConfiguration::project() const
{
    return target()->project();
}

Utils::Environment BuildConfiguration::baseEnvironment() const
{
    Utils::Environment result;
    if (useSystemEnvironment())
        result = Utils::Environment::systemEnvironment();
    addToEnvironment(result);
    target()->kit()->addToEnvironment(result);
    return result;
}

QString BuildConfiguration::baseEnvironmentText() const
{
    if (useSystemEnvironment())
        return tr("System Environment");
    else
        return tr("Clean Environment");
}

Utils::Environment BuildConfiguration::environment() const
{
    return m_cachedEnvironment;
}

void BuildConfiguration::setUseSystemEnvironment(bool b)
{
    if (useSystemEnvironment() == b)
        return;
    m_clearSystemEnvironment = !b;
    updateCacheAndEmitEnvironmentChanged();
}

void BuildConfiguration::addToEnvironment(Utils::Environment &env) const
{
    Q_UNUSED(env);
}

bool BuildConfiguration::useSystemEnvironment() const
{
    return !m_clearSystemEnvironment;
}

QList<Utils::EnvironmentItem> BuildConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void BuildConfiguration::setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges == diff)
        return;
    m_userEnvironmentChanges = diff;
    updateCacheAndEmitEnvironmentChanged();
}

void BuildConfiguration::cloneSteps(BuildConfiguration *source)
{
    if (source == this)
        return;
    qDeleteAll(m_stepLists);
    m_stepLists.clear();
    foreach (BuildStepList *bsl, source->m_stepLists) {
        auto newBsl = new BuildStepList(this, bsl);
        newBsl->cloneSteps(bsl);
        m_stepLists.append(newBsl);
    }
}

bool BuildConfiguration::isEnabled() const
{
    return true;
}

QString BuildConfiguration::disabledReason() const
{
    return QString();
}

QString BuildConfiguration::buildTypeName(BuildConfiguration::BuildType type)
{
    switch (type) {
    case ProjectExplorer::BuildConfiguration::Debug:
        return QLatin1String("debug");
    case ProjectExplorer::BuildConfiguration::Profile:
        return QLatin1String("profile");
    case ProjectExplorer::BuildConfiguration::Release:
        return QLatin1String("release");
    case ProjectExplorer::BuildConfiguration::Unknown: // fallthrough
    default:
        return QLatin1String("unknown");
    }
}

bool BuildConfiguration::isActive() const
{
    return target()->isActive() && target()->activeBuildConfiguration() == this;
}

/*!
 * Helper function that prepends the directory containing the C++ toolchain to
 * PATH. This is used to in build configurations targeting broken build systems
 * to provide hints about which compiler to use.
 */
void BuildConfiguration::prependCompilerPathToEnvironment(Utils::Environment &env) const
{
    const ToolChain *tc
            = ToolChainKitInformation::toolChain(target()->kit(),
                                                 ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    if (!tc)
        return;

    const Utils::FileName compilerDir = tc->compilerCommand().parentDir();
    if (!compilerDir.isEmpty())
        env.prependOrSetPath(compilerDir.toString());
}

///
// IBuildConfigurationFactory
///

IBuildConfigurationFactory::IBuildConfigurationFactory(QObject *parent) :
    QObject(parent)
{ }

IBuildConfigurationFactory::~IBuildConfigurationFactory()
{ }

// restore
IBuildConfigurationFactory *IBuildConfigurationFactory::find(Target *parent, const QVariantMap &map)
{
    QList<IBuildConfigurationFactory *> factories
            = ExtensionSystem::PluginManager::getObjects<IBuildConfigurationFactory>(
                [&parent, map](IBuildConfigurationFactory *factory) {
                    return factory->canRestore(parent, map);
                });

    IBuildConfigurationFactory *factory = 0;
    int priority = -1;
    foreach (IBuildConfigurationFactory *i, factories) {
        int iPriority = i->priority(parent);
        if (iPriority > priority) {
            factory = i;
            priority = iPriority;
        }
    }
    return factory;
}

// setup
IBuildConfigurationFactory *IBuildConfigurationFactory::find(const Kit *k, const QString &projectPath)
{
    QList<IBuildConfigurationFactory *> factories
            = ExtensionSystem::PluginManager::instance()->getObjects<IBuildConfigurationFactory>();
    IBuildConfigurationFactory *factory = 0;
    int priority = -1;
    foreach (IBuildConfigurationFactory *i, factories) {
        int iPriority = i->priority(k, projectPath);
        if (iPriority > priority) {
            factory = i;
            priority = iPriority;
        }
    }
    return factory;
}

// create
IBuildConfigurationFactory * IBuildConfigurationFactory::find(Target *parent)
{
    QList<IBuildConfigurationFactory *> factories
            = ExtensionSystem::PluginManager::instance()->getObjects<IBuildConfigurationFactory>();
    IBuildConfigurationFactory *factory = 0;
    int priority = -1;
    foreach (IBuildConfigurationFactory *i, factories) {
        int iPriority = i->priority(parent);
        if (iPriority > priority) {
            factory = i;
            priority = iPriority;
        }
    }
    return factory;
}

// clone
IBuildConfigurationFactory *IBuildConfigurationFactory::find(Target *parent, BuildConfiguration *bc)
{
    QList<IBuildConfigurationFactory *> factories
            = ExtensionSystem::PluginManager::getObjects<IBuildConfigurationFactory>(
                [&parent, &bc](IBuildConfigurationFactory *factory) {
                    return factory->canClone(parent, bc);
                });

    IBuildConfigurationFactory *factory = 0;
    int priority = -1;
    foreach (IBuildConfigurationFactory *i, factories) {
        int iPriority = i->priority(parent);
        if (iPriority > priority) {
            factory = i;
            priority = iPriority;
        }
    }
    return factory;
}
} // namespace ProjectExplorer
