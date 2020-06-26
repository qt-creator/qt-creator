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
#include "buildinfo.h"
#include "buildsteplist.h"
#include "buildstepspage.h"
#include "buildsystem.h"
#include "customparser.h"
#include "environmentwidget.h"
#include "kit.h"
#include "kitinformation.h"
#include "kitmanager.h"
#include "namedwidget.h"
#include "projectexplorerconstants.h"
#include "projectexplorer.h"
#include "project.h"
#include "projectmacroexpander.h"
#include "projecttree.h"
#include "session.h"
#include "target.h"
#include "toolchain.h"

#include <coreplugin/idocument.h>
#include <coreplugin/variablechooser.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/macroexpander.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/mimetypes/mimetype.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QDebug>
#include <QFormLayout>
#include <QVBoxLayout>

using namespace Utils;

const char BUILD_STEP_LIST_COUNT[] = "ProjectExplorer.BuildConfiguration.BuildStepListCount";
const char BUILD_STEP_LIST_PREFIX[] = "ProjectExplorer.BuildConfiguration.BuildStepList.";
const char CLEAR_SYSTEM_ENVIRONMENT_KEY[] = "ProjectExplorer.BuildConfiguration.ClearSystemEnvironment";
const char USER_ENVIRONMENT_CHANGES_KEY[] = "ProjectExplorer.BuildConfiguration.UserEnvironmentChanges";
const char CUSTOM_PARSERS_KEY[] = "ProjectExplorer.BuildConfiguration.CustomParsers";

namespace ProjectExplorer {
namespace Internal {

class BuildEnvironmentWidget : public NamedWidget
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::Internal::BuildEnvironmentWidget)

public:
    explicit BuildEnvironmentWidget(BuildConfiguration *bc)
        : NamedWidget(tr("Build Environment"))
    {
        auto clearBox = new QCheckBox(tr("Clear system environment"), this);
        clearBox->setChecked(!bc->useSystemEnvironment());

        auto envWidget = new EnvironmentWidget(this, EnvironmentWidget::TypeLocal, clearBox);
        envWidget->setBaseEnvironment(bc->baseEnvironment());
        envWidget->setBaseEnvironmentText(bc->baseEnvironmentText());
        envWidget->setUserChanges(bc->userEnvironmentChanges());

        connect(envWidget, &EnvironmentWidget::userChangesChanged, this, [bc, envWidget] {
            bc->setUserEnvironmentChanges(envWidget->userChanges());
        });

        connect(clearBox, &QAbstractButton::toggled, this, [bc, envWidget](bool checked) {
            bc->setUseSystemEnvironment(!checked);
            envWidget->setBaseEnvironment(bc->baseEnvironment());
            envWidget->setBaseEnvironmentText(bc->baseEnvironmentText());
        });

        connect(bc, &BuildConfiguration::environmentChanged, this, [bc, envWidget] {
            envWidget->setBaseEnvironment(bc->baseEnvironment());
            envWidget->setBaseEnvironmentText(bc->baseEnvironmentText());
        });

        auto vbox = new QVBoxLayout(this);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->addWidget(clearBox);
        vbox->addWidget(envWidget);
    }
};

class CustomParsersBuildWidget : public NamedWidget
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::Internal::CustomParsersBuildWidget)
public:
    CustomParsersBuildWidget(BuildConfiguration *bc) : NamedWidget(tr("Custom Output Parsers"))
    {
        const auto selectionWidget = new CustomParsersSelectionWidget(this);
        const auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(selectionWidget);

        connect(selectionWidget, &CustomParsersSelectionWidget::selectionChanged,
                [selectionWidget, bc] {
            bc->setCustomParsers(selectionWidget->selectedParsers());
        });
        selectionWidget->setSelectedParsers(bc->customParsers());
    }
};


class BuildConfigurationPrivate
{
public:
    BuildConfigurationPrivate(BuildConfiguration *bc)
        : m_buildSteps(bc, Constants::BUILDSTEPS_BUILD),
          m_cleanSteps(bc, Constants::BUILDSTEPS_CLEAN)
    {}

    bool m_clearSystemEnvironment = false;
    EnvironmentItems m_userEnvironmentChanges;
    BuildStepList m_buildSteps;
    BuildStepList m_cleanSteps;
    BuildDirectoryAspect *m_buildDirectoryAspect = nullptr;
    FilePath m_lastEmittedBuildDirectory;
    mutable Environment m_cachedEnvironment;
    QString m_configWidgetDisplayName;
    bool m_configWidgetHasFrame = false;
    QList<Utils::Id> m_initialBuildSteps;
    QList<Utils::Id> m_initialCleanSteps;
    Utils::MacroExpander m_macroExpander;
    QList<Utils::Id> m_customParsers;

    // FIXME: Remove.
    BuildConfiguration::BuildType m_initialBuildType = BuildConfiguration::Unknown;
    std::function<void(const BuildInfo &)> m_initializer;
};

} // Internal

BuildConfiguration::BuildConfiguration(Target *target, Utils::Id id)
    : ProjectConfiguration(target, id), d(new Internal::BuildConfigurationPrivate(this))
{
    QTC_CHECK(target && target == this->target());

    MacroExpander *expander = macroExpander();
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
    connect(target->project(), &Project::environmentChanged,
            this, &BuildConfiguration::environmentChanged);
    // Many macroexpanders are based on the current project, so they may change the environment:
    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            this, &BuildConfiguration::updateCacheAndEmitEnvironmentChanged);

    d->m_buildDirectoryAspect = addAspect<BuildDirectoryAspect>();
    d->m_buildDirectoryAspect->setBaseFileName(target->project()->projectDirectory());
    d->m_buildDirectoryAspect->setEnvironment(environment());
    d->m_buildDirectoryAspect->setMacroExpanderProvider([this] { return macroExpander(); });
    connect(d->m_buildDirectoryAspect, &BaseStringAspect::changed,
            this, &BuildConfiguration::emitBuildDirectoryChanged);
    connect(this, &BuildConfiguration::environmentChanged, this, [this] {
        d->m_buildDirectoryAspect->setEnvironment(environment());
        this->target()->buildEnvironmentChanged(this);
    });

    connect(target, &Target::parsingStarted, this, &BuildConfiguration::enabledChanged);
    connect(target, &Target::parsingFinished, this, &BuildConfiguration::enabledChanged);
    connect(this, &BuildConfiguration::enabledChanged, this, [this] {
        if (isActive() && project() == SessionManager::startupProject()) {
            ProjectExplorerPlugin::updateActions();
            ProjectExplorerPlugin::updateRunActions();
        }
    });
}

BuildConfiguration::~BuildConfiguration()
{
    delete d;
}

FilePath BuildConfiguration::buildDirectory() const
{
    QString path = environment().expandVariables(d->m_buildDirectoryAspect->value().trimmed());
    path = QDir::cleanPath(macroExpander()->expand(path));
    return FilePath::fromString(QDir::cleanPath(QDir(target()->project()->projectDirectory().toString()).absoluteFilePath(path)));
}

FilePath BuildConfiguration::rawBuildDirectory() const
{
    return d->m_buildDirectoryAspect->filePath();
}

void BuildConfiguration::setBuildDirectory(const FilePath &dir)
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

    adder(new Internal::BuildStepListWidget(buildSteps()));
    adder(new Internal::BuildStepListWidget(cleanSteps()));

    QList<NamedWidget *> subConfigWidgets = createSubConfigWidgets();
    foreach (NamedWidget *subConfigWidget, subConfigWidgets)
        adder(subConfigWidget);
}

void BuildConfiguration::doInitialize(const BuildInfo &info)
{
    setDisplayName(info.displayName);
    setDefaultDisplayName(info.displayName);
    setBuildDirectory(info.buildDirectory);

    d->m_initialBuildType = info.buildType;

    for (Utils::Id id : qAsConst(d->m_initialBuildSteps))
        d->m_buildSteps.appendStep(id);

    for (Utils::Id id : qAsConst(d->m_initialCleanSteps))
        d->m_cleanSteps.appendStep(id);

    acquaintAspects();

    if (d->m_initializer)
        d->m_initializer(info);
}

MacroExpander *BuildConfiguration::macroExpander() const
{
    return &d->m_macroExpander;
}

bool BuildConfiguration::createBuildDirectory()
{
    QDir dir;
    const auto result = dir.mkpath(buildDirectory().toString());
    buildDirectoryAspect()->validateInput();
    return result;
}

void BuildConfiguration::setInitializer(const std::function<void(const BuildInfo &)> &initializer)
{
    d->m_initializer = initializer;
}

NamedWidget *BuildConfiguration::createConfigWidget()
{
    NamedWidget *named = new NamedWidget(d->m_configWidgetDisplayName);

    QWidget *widget = nullptr;

    if (d->m_configWidgetHasFrame) {
        auto container = new DetailsWidget(named);
        widget = new QWidget(container);
        container->setState(DetailsWidget::NoSummary);
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

QList<NamedWidget *> BuildConfiguration::createSubConfigWidgets()
{
    return {
        new Internal::BuildEnvironmentWidget(this),
        new Internal::CustomParsersBuildWidget(this)
    };
}

BuildSystem *BuildConfiguration::buildSystem() const
{
    QTC_CHECK(target()->fallbackBuildSystem());
    return target()->fallbackBuildSystem();
}

BuildStepList *BuildConfiguration::buildSteps() const
{
    return &d->m_buildSteps;
}

BuildStepList *BuildConfiguration::cleanSteps() const
{
    return &d->m_cleanSteps;
}

void BuildConfiguration::appendInitialBuildStep(Utils::Id id)
{
    d->m_initialBuildSteps.append(id);
}

void BuildConfiguration::appendInitialCleanStep(Utils::Id id)
{
    d->m_initialCleanSteps.append(id);
}

QVariantMap BuildConfiguration::toMap() const
{
    QVariantMap map = ProjectConfiguration::toMap();

    map.insert(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY), d->m_clearSystemEnvironment);
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY), EnvironmentItem::toStringList(d->m_userEnvironmentChanges));

    map.insert(QLatin1String(BUILD_STEP_LIST_COUNT), 2);
    map.insert(QLatin1String(BUILD_STEP_LIST_PREFIX) + QString::number(0), d->m_buildSteps.toMap());
    map.insert(QLatin1String(BUILD_STEP_LIST_PREFIX) + QString::number(1), d->m_cleanSteps.toMap());

    map.insert(CUSTOM_PARSERS_KEY, transform(d->m_customParsers,&Utils::Id::toSetting));

    return map;
}

bool BuildConfiguration::fromMap(const QVariantMap &map)
{
    d->m_clearSystemEnvironment = map.value(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY)).toBool();
    d->m_userEnvironmentChanges = EnvironmentItem::fromStringList(map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());

    updateCacheAndEmitEnvironmentChanged();

    d->m_buildSteps.clear();
    d->m_cleanSteps.clear();

    int maxI = map.value(QLatin1String(BUILD_STEP_LIST_COUNT), 0).toInt();
    for (int i = 0; i < maxI; ++i) {
        QVariantMap data = map.value(QLatin1String(BUILD_STEP_LIST_PREFIX) + QString::number(i)).toMap();
        if (data.isEmpty()) {
            qWarning() << "No data for build step list" << i << "found!";
            continue;
        }
        Utils::Id id = idFromMap(data);
        if (id == Constants::BUILDSTEPS_BUILD) {
            if (!d->m_buildSteps.fromMap(data))
                qWarning() << "Failed to restore build step list";
        } else if (id == Constants::BUILDSTEPS_CLEAN) {
            if (!d->m_cleanSteps.fromMap(data))
                qWarning() << "Failed to restore clean step list";
        } else {
            qWarning() << "Ignoring unknown step list";
        }
    }

    d->m_customParsers = transform(map.value(CUSTOM_PARSERS_KEY).toList(), &Utils::Id::fromSetting);

    return ProjectConfiguration::fromMap(map);
}

void BuildConfiguration::updateCacheAndEmitEnvironmentChanged()
{
    Environment env = baseEnvironment();
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

Environment BuildConfiguration::baseEnvironment() const
{
    Environment result;
    if (useSystemEnvironment())
        result = Environment::systemEnvironment();
    addToEnvironment(result);
    target()->kit()->addToEnvironment(result);
    result.modify(project()->additionalEnvironment());
    return result;
}

QString BuildConfiguration::baseEnvironmentText() const
{
    if (useSystemEnvironment())
        return tr("System Environment");
    else
        return tr("Clean Environment");
}

Environment BuildConfiguration::environment() const
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

void BuildConfiguration::addToEnvironment(Environment &env) const
{
    Q_UNUSED(env)
}

const QList<Utils::Id> BuildConfiguration::customParsers() const
{
    return d->m_customParsers;
}

void BuildConfiguration::setCustomParsers(const QList<Utils::Id> &parsers)
{
    d->m_customParsers = parsers;
}

bool BuildConfiguration::useSystemEnvironment() const
{
    return !d->m_clearSystemEnvironment;
}

EnvironmentItems BuildConfiguration::userEnvironmentChanges() const
{
    return d->m_userEnvironmentChanges;
}

void BuildConfiguration::setUserEnvironmentChanges(const EnvironmentItems &diff)
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

void BuildConfiguration::restrictNextBuild(const RunConfiguration *rc)
{
    Q_UNUSED(rc)
}

BuildConfiguration::BuildType BuildConfiguration::buildType() const
{
    return d->m_initialBuildType;
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

void BuildConfiguration::prependCompilerPathToEnvironment(Kit *k, Environment &env)
{
    const ToolChain *tc = ToolChainKitAspect::cxxToolChain(k);

    if (!tc)
        return;

    const FilePath compilerDir = tc->compilerCommand().parentDir();
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
    QTC_ASSERT(m_buildGenerator, return {});
    QList<BuildInfo> list = m_buildGenerator(parent->kit(), parent->project()->projectFilePath(), false);
    for (BuildInfo &info : list) {
        info.factory = this;
        info.kitId = parent->kit()->id();
    }
    return list;
}

const QList<BuildInfo>
    BuildConfigurationFactory::allAvailableSetups(const Kit *k, const FilePath &projectPath) const
{
    QTC_ASSERT(m_buildGenerator, return {});
    QList<BuildInfo> list = m_buildGenerator(k, projectPath, /* forSetup = */ true);
    for (BuildInfo &info : list) {
        info.factory = this;
        info.kitId = k->id();
    }
    return list;
}

bool BuildConfigurationFactory::supportsTargetDeviceType(Utils::Id id) const
{
    if (m_supportedTargetDeviceTypes.isEmpty())
        return true;
    return m_supportedTargetDeviceTypes.contains(id);
}

// setup
BuildConfigurationFactory *BuildConfigurationFactory::find(const Kit *k, const FilePath &projectPath)
{
    QTC_ASSERT(k, return nullptr);
    const Utils::Id deviceType = DeviceTypeKitAspect::deviceTypeId(k);
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

void BuildConfigurationFactory::setSupportedProjectType(Utils::Id id)
{
    m_supportedProjectType = id;
}

void BuildConfigurationFactory::setSupportedProjectMimeTypeName(const QString &mimeTypeName)
{
    m_supportedProjectMimeTypeName = mimeTypeName;
}

void BuildConfigurationFactory::addSupportedTargetDeviceType(Utils::Id id)
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

void BuildConfigurationFactory::setBuildGenerator(const BuildGenerator &buildGenerator)
{
    m_buildGenerator = buildGenerator;
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
    if (bc)
        bc->doInitialize(info);

    return bc;
}

BuildConfiguration *BuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    const Utils::Id id = idFromMap(map);
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
