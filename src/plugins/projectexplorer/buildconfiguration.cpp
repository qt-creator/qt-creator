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

#include "buildaspects.h"
#include "buildenvironmentwidget.h"
#include "buildinfo.h"
#include "buildsteplist.h"
#include "buildstepspage.h"
#include "buildsystem.h"
#include "namedwidget.h"
#include "kit.h"
#include "kitinformation.h"
#include "kitmanager.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectmacroexpander.h"
#include "projecttree.h"
#include "target.h"
#include "session.h"

#include <coreplugin/idocument.h>
#include <coreplugin/variablechooser.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/macroexpander.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/mimetypes/mimetype.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QFormLayout>

using namespace Utils;

static const char BUILD_STEP_LIST_COUNT[] = "ProjectExplorer.BuildConfiguration.BuildStepListCount";
static const char BUILD_STEP_LIST_PREFIX[] = "ProjectExplorer.BuildConfiguration.BuildStepList.";
static const char CLEAR_SYSTEM_ENVIRONMENT_KEY[] = "ProjectExplorer.BuildConfiguration.ClearSystemEnvironment";
static const char USER_ENVIRONMENT_CHANGES_KEY[] = "ProjectExplorer.BuildConfiguration.UserEnvironmentChanges";

namespace ProjectExplorer {
namespace Internal {

class BuildConfigurationPrivate
{
public:
    bool m_clearSystemEnvironment = false;
    Utils::EnvironmentItems m_userEnvironmentChanges;
    QList<BuildStepList *> m_stepLists;
    BuildDirectoryAspect *m_buildDirectoryAspect = nullptr;
    Utils::FilePath m_lastEmittedBuildDirectory;
    mutable Utils::Environment m_cachedEnvironment;
    QString m_configWidgetDisplayName;
    bool m_configWidgetHasFrame = false;

    // FIXME: Remove.
    BuildConfiguration::BuildType m_initialBuildType = BuildConfiguration::Unknown;
    Utils::FilePath m_initialBuildDirectory;
    QString m_initialDisplayName;
    QVariant m_extraInfo;
};

} // Internal

BuildConfiguration::BuildConfiguration(Target *target, Core::Id id)
    : ProjectConfiguration(target, id), d(new Internal::BuildConfigurationPrivate)
{
    QTC_CHECK(target && target == this->target());

    Utils::MacroExpander *expander = macroExpander();
    expander->setDisplayName(tr("Build Settings"));
    expander->setAccumulating(true);
    expander->registerSubProvider([target] { return target->macroExpander(); });

    expander->registerVariable("buildDir", tr("Build directory"),
            [this] { return buildDirectory().toUserOutput(); });

    expander->registerVariable(Constants::VAR_CURRENTBUILD_NAME, tr("Name of current build"),
            [this] { return displayName(); }, false);

    expander->registerPrefix(Constants::VAR_CURRENTBUILD_ENV,
                             tr("Variables in the current build environment"),
                             [this](const QString &var) { return environment().expandedValueForKey(var); });

    updateCacheAndEmitEnvironmentChanged();
    connect(target, &Target::kitChanged,
            this, &BuildConfiguration::updateCacheAndEmitEnvironmentChanged);
    connect(this, &BuildConfiguration::environmentChanged,
            this, &BuildConfiguration::emitBuildDirectoryChanged);
    // Many macroexpanders are based on the current project, so they may change the environment:
    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            this, &BuildConfiguration::updateCacheAndEmitEnvironmentChanged);

    d->m_buildDirectoryAspect = addAspect<BuildDirectoryAspect>();
    d->m_buildDirectoryAspect->setBaseFileName(target->project()->projectDirectory());
    d->m_buildDirectoryAspect->setEnvironment(environment());
    d->m_buildDirectoryAspect->setMacroExpanderProvider([this] { return macroExpander(); });
    connect(d->m_buildDirectoryAspect, &BaseStringAspect::changed,
            this, &BuildConfiguration::buildDirectoryChanged);
    connect(this, &BuildConfiguration::environmentChanged, this, [this] {
        d->m_buildDirectoryAspect->setEnvironment(environment());
        this->target()->buildEnvironmentChanged(this);
    });

    connect(target, &Target::parsingStarted, this, &BuildConfiguration::enabledChanged);
    connect(target, &Target::parsingFinished, this, &BuildConfiguration::enabledChanged);
    connect(this, &BuildConfiguration::enabledChanged, this, [this] {
        if (isActive() && project() == SessionManager::startupProject()) {
            ProjectExplorerPlugin::updateActions();
            emit ProjectExplorerPlugin::instance()->updateRunActions();
        }
    });
}

BuildConfiguration::~BuildConfiguration()
{
    delete d;
}

Utils::FilePath BuildConfiguration::buildDirectory() const
{
    QString path = environment().expandVariables(d->m_buildDirectoryAspect->value().trimmed());
    path = QDir::cleanPath(macroExpander()->expand(path));
    return Utils::FilePath::fromString(QDir::cleanPath(QDir(target()->project()->projectDirectory().toString()).absoluteFilePath(path)));
}

Utils::FilePath BuildConfiguration::rawBuildDirectory() const
{
    return d->m_buildDirectoryAspect->filePath();
}

void BuildConfiguration::setBuildDirectory(const Utils::FilePath &dir)
{
    if (dir == d->m_buildDirectoryAspect->filePath())
        return;
    d->m_buildDirectoryAspect->setFilePath(dir);
    emitBuildDirectoryChanged();
}

void BuildConfiguration::addConfigWidgets(const std::function<void(NamedWidget *)> &adder)
{
    if (NamedWidget *generalConfigWidget = createConfigWidget())
        adder(generalConfigWidget);

    adder(new Internal::BuildStepListWidget(stepList(Constants::BUILDSTEPS_BUILD)));
    adder(new Internal::BuildStepListWidget(stepList(Constants::BUILDSTEPS_CLEAN)));

    QList<NamedWidget *> subConfigWidgets = createSubConfigWidgets();
    foreach (NamedWidget *subConfigWidget, subConfigWidgets)
        adder(subConfigWidget);
}

NamedWidget *BuildConfiguration::createConfigWidget()
{
    NamedWidget *named = new NamedWidget(d->m_configWidgetDisplayName);

    QWidget *widget = nullptr;

    if (d->m_configWidgetHasFrame) {
        auto container = new Utils::DetailsWidget(named);
        widget = new QWidget(container);
        container->setState(Utils::DetailsWidget::NoSummary);
        container->setWidget(widget);

        auto vbox = new QVBoxLayout(named);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->addWidget(container);
    } else {
        widget = named;
    }

    LayoutBuilder builder(widget);
    for (ProjectConfigurationAspect *aspect : aspects()) {
        if (aspect->isVisible())
            aspect->addToLayout(builder.startNewRow());
    }

    return named;
}

void BuildConfiguration::initialize()
{
    d->m_stepLists.append(new BuildStepList(this, Constants::BUILDSTEPS_BUILD));
    d->m_stepLists.append(new BuildStepList(this, Constants::BUILDSTEPS_CLEAN));
}

QList<NamedWidget *> BuildConfiguration::createSubConfigWidgets()
{
    return {new BuildEnvironmentWidget(this)};
}

BuildSystem *BuildConfiguration::buildSystem() const
{
    QTC_CHECK(target()->fallbackBuildSystem());
    return target()->fallbackBuildSystem();
}

QList<Core::Id> BuildConfiguration::knownStepLists() const
{
    return Utils::transform(d->m_stepLists, &BuildStepList::id);
}

BuildStepList *BuildConfiguration::stepList(Core::Id id) const
{
    return Utils::findOrDefault(d->m_stepLists, Utils::equal(&BuildStepList::id, id));
}

QVariantMap BuildConfiguration::toMap() const
{
    QVariantMap map(ProjectConfiguration::toMap());
    map.insert(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY), d->m_clearSystemEnvironment);
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY), Utils::EnvironmentItem::toStringList(d->m_userEnvironmentChanges));

    map.insert(QLatin1String(BUILD_STEP_LIST_COUNT), d->m_stepLists.count());
    for (int i = 0; i < d->m_stepLists.count(); ++i)
        map.insert(QLatin1String(BUILD_STEP_LIST_PREFIX) + QString::number(i), d->m_stepLists.at(i)->toMap());

    return map;
}

bool BuildConfiguration::fromMap(const QVariantMap &map)
{
    d->m_clearSystemEnvironment = map.value(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY)).toBool();
    d->m_userEnvironmentChanges = Utils::EnvironmentItem::fromStringList(map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());

    updateCacheAndEmitEnvironmentChanged();

    qDeleteAll(d->m_stepLists);
    d->m_stepLists.clear();

    int maxI = map.value(QLatin1String(BUILD_STEP_LIST_COUNT), 0).toInt();
    for (int i = 0; i < maxI; ++i) {
        QVariantMap data = map.value(QLatin1String(BUILD_STEP_LIST_PREFIX) + QString::number(i)).toMap();
        if (data.isEmpty()) {
            qWarning() << "No data for build step list" << i << "found!";
            continue;
        }
        auto list = new BuildStepList(this, idFromMap(data));
        if (!list->fromMap(data)) {
            qWarning() << "Failed to restore build step list" << i;
            delete list;
            return false;
        }
        d->m_stepLists.append(list);
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
    if (env == d->m_cachedEnvironment)
        return;
    d->m_cachedEnvironment = env;
    emit environmentChanged(); // might trigger buildDirectoryChanged signal!
}

void BuildConfiguration::emitBuildDirectoryChanged()
{
    if (buildDirectory() != d->m_lastEmittedBuildDirectory) {
        d->m_lastEmittedBuildDirectory = buildDirectory();
        emit buildDirectoryChanged();
    }
}

QString BuildConfiguration::initialDisplayName() const
{
    return d->m_initialDisplayName;
}

QVariant BuildConfiguration::extraInfo() const
{
    return d->m_extraInfo;
}

ProjectExplorer::BuildDirectoryAspect *BuildConfiguration::buildDirectoryAspect() const
{
    return d->m_buildDirectoryAspect;
}

void BuildConfiguration::setConfigWidgetDisplayName(const QString &display)
{
    d->m_configWidgetDisplayName = display;
}

void BuildConfiguration::setBuildDirectoryHistoryCompleter(const QString &history)
{
    d->m_buildDirectoryAspect->setHistoryCompleter(history);
}

void BuildConfiguration::setConfigWidgetHasFrame(bool configWidgetHasFrame)
{
    d->m_configWidgetHasFrame = configWidgetHasFrame;
}

void BuildConfiguration::setBuildDirectorySettingsKey(const QString &key)
{
    d->m_buildDirectoryAspect->setSettingsKey(key);
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
    return d->m_cachedEnvironment;
}

void BuildConfiguration::setUseSystemEnvironment(bool b)
{
    if (useSystemEnvironment() == b)
        return;
    d->m_clearSystemEnvironment = !b;
    updateCacheAndEmitEnvironmentChanged();
}

void BuildConfiguration::addToEnvironment(Utils::Environment &env) const
{
    Q_UNUSED(env)
}

bool BuildConfiguration::useSystemEnvironment() const
{
    return !d->m_clearSystemEnvironment;
}

Utils::EnvironmentItems BuildConfiguration::userEnvironmentChanges() const
{
    return d->m_userEnvironmentChanges;
}

void BuildConfiguration::setUserEnvironmentChanges(const Utils::EnvironmentItems &diff)
{
    if (d->m_userEnvironmentChanges == diff)
        return;
    d->m_userEnvironmentChanges = diff;
    updateCacheAndEmitEnvironmentChanged();
}

bool BuildConfiguration::isEnabled() const
{
    return !buildSystem()->isParsing() && buildSystem()->hasParsingData();
}

QString BuildConfiguration::disabledReason() const
{
    if (buildSystem()->isParsing())
        return (tr("The project is currently being parsed."));
    if (!buildSystem()->hasParsingData())
        return (tr("The project was not parsed successfully."));
    return QString();
}

bool BuildConfiguration::regenerateBuildFiles(Node *node)
{
    Q_UNUSED(node)
    return false;
}

BuildConfiguration::BuildType BuildConfiguration::buildType() const
{
    return d->m_initialBuildType;
}

BuildConfiguration::BuildType BuildConfiguration::initialBuildType() const
{
    return d->m_initialBuildType;
}

FilePath BuildConfiguration::initialBuildDirectory() const
{
    return d->m_initialBuildDirectory;
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

void BuildConfiguration::prependCompilerPathToEnvironment(Kit *k, Utils::Environment &env)
{
    const ToolChain *tc
            = ToolChainKitAspect::toolChain(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    if (!tc)
        return;

    const Utils::FilePath compilerDir = tc->compilerCommand().parentDir();
    if (!compilerDir.isEmpty())
        env.prependOrSetPath(compilerDir.toString());
}

///
// IBuildConfigurationFactory
///

static QList<BuildConfigurationFactory *> g_buildConfigurationFactories;

BuildConfigurationFactory::BuildConfigurationFactory()
{
    // Note: Order matters as first-in-queue wins.
    g_buildConfigurationFactories.prepend(this);
}

BuildConfigurationFactory::~BuildConfigurationFactory()
{
    g_buildConfigurationFactories.removeOne(this);
}

const Tasks BuildConfigurationFactory::reportIssues(ProjectExplorer::Kit *kit, const QString &projectPath,
                                                          const QString &buildDir) const
{
    if (m_issueReporter)
        return m_issueReporter(kit, projectPath, buildDir);
    return {};
}

const QList<BuildInfo> BuildConfigurationFactory::allAvailableBuilds(const Target *parent) const
{
    return availableBuilds(parent->kit(), parent->project()->projectFilePath(), false);
}

const QList<BuildInfo>
    BuildConfigurationFactory::allAvailableSetups(const Kit *k, const FilePath &projectPath) const
{
    return availableBuilds(k, projectPath, /* forSetup = */ true);
}

bool BuildConfigurationFactory::supportsTargetDeviceType(Core::Id id) const
{
    if (m_supportedTargetDeviceTypes.isEmpty())
        return true;
    return m_supportedTargetDeviceTypes.contains(id);
}

// setup
BuildConfigurationFactory *BuildConfigurationFactory::find(const Kit *k, const FilePath &projectPath)
{
    QTC_ASSERT(k, return nullptr);
    const Core::Id deviceType = DeviceTypeKitAspect::deviceTypeId(k);
    for (BuildConfigurationFactory *factory : g_buildConfigurationFactories) {
        if (Utils::mimeTypeForFile(projectPath.toString())
                .matchesName(factory->m_supportedProjectMimeTypeName)
                && factory->supportsTargetDeviceType(deviceType))
            return factory;
    }
    return nullptr;
}

// create
BuildConfigurationFactory * BuildConfigurationFactory::find(Target *parent)
{
    for (BuildConfigurationFactory *factory : g_buildConfigurationFactories) {
        if (factory->canHandle(parent))
            return factory;
    }
    return nullptr;
}

void BuildConfigurationFactory::setSupportedProjectType(Core::Id id)
{
    m_supportedProjectType = id;
}

void BuildConfigurationFactory::setSupportedProjectMimeTypeName(const QString &mimeTypeName)
{
    m_supportedProjectMimeTypeName = mimeTypeName;
}

void BuildConfigurationFactory::addSupportedTargetDeviceType(Core::Id id)
{
    m_supportedTargetDeviceTypes.append(id);
}

bool BuildConfigurationFactory::canHandle(const Target *target) const
{
    if (m_supportedProjectType.isValid() && m_supportedProjectType != target->project()->id())
        return false;

    if (containsType(target->project()->projectIssues(target->kit()), Task::TaskType::Error))
        return false;

    if (!supportsTargetDeviceType(DeviceTypeKitAspect::deviceTypeId(target->kit())))
        return false;

    return true;
}

void BuildConfigurationFactory::setIssueReporter(const IssueReporter &issueReporter)
{
    m_issueReporter = issueReporter;
}

BuildConfiguration *BuildConfigurationFactory::create(Target *parent, const BuildInfo &info) const
{
    if (!canHandle(parent))
        return nullptr;
    QTC_ASSERT(m_creator, return nullptr);
    BuildConfiguration *bc = m_creator(parent);
    if (!bc)
        return nullptr;

    bc->setDisplayName(info.displayName);
    bc->setDefaultDisplayName(info.displayName);
    bc->setBuildDirectory(info.buildDirectory);

    bc->d->m_initialBuildType = info.buildType;
    bc->d->m_initialDisplayName = info.displayName;
    bc->d->m_initialBuildDirectory = info.buildDirectory;
    bc->d->m_extraInfo = info.extraInfo;

    bc->acquaintAspects();
    bc->initialize();

    return bc;
}

BuildConfiguration *BuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    const Core::Id id = idFromMap(map);
    for (BuildConfigurationFactory *factory : g_buildConfigurationFactories) {
        QTC_ASSERT(factory->m_creator, return nullptr);
        if (!factory->canHandle(parent))
            continue;
        if (!id.name().startsWith(factory->m_buildConfigId.name()))
            continue;
        BuildConfiguration *bc = factory->m_creator(parent);
        bc->acquaintAspects();
        QTC_ASSERT(bc, return nullptr);
        if (!bc->fromMap(map)) {
            delete bc;
            bc = nullptr;
        }
        return bc;
    }
    return nullptr;
}

BuildConfiguration *BuildConfigurationFactory::clone(Target *parent,
                                                      const BuildConfiguration *source)
{
    return restore(parent, source->toMap());
}

} // namespace ProjectExplorer
