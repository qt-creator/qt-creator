// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildconfiguration.h"

#include "buildaspects.h"
#include "buildinfo.h"
#include "buildpropertiessettings.h"
#include "buildsteplist.h"
#include "buildstepspage.h"
#include "buildsystem.h"
#include "customparser.h"
#include "environmentwidget.h"
#include "kit.h"
#include "kitaspects.h"
#include "namedwidget.h"
#include "projectexplorerconstants.h"
#include "projectexplorer.h"
#include "projectexplorertr.h"
#include "project.h"
#include "projectmanager.h"
#include "projecttree.h"
#include "target.h"

#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/devicesupport/idevice.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/variablechooser.h>

#include <QCheckBox>
#include <QDebug>
#include <QFormLayout>
#include <QLoggingCategory>
#include <QVBoxLayout>

using namespace Utils;

const char BUILD_STEP_LIST_COUNT[] = "ProjectExplorer.BuildConfiguration.BuildStepListCount";
const char BUILD_STEP_LIST_PREFIX[] = "ProjectExplorer.BuildConfiguration.BuildStepList.";
const char CUSTOM_PARSERS_KEY[] = "ProjectExplorer.BuildConfiguration.CustomParsers";
const char PARSE_STD_OUT_KEY[] = "ProjectExplorer.BuildConfiguration.ParseStandardOutput";

Q_LOGGING_CATEGORY(bcLog, "qtc.buildconfig", QtWarningMsg)

namespace ProjectExplorer {
namespace Internal {

class BuildEnvironmentWidget : public NamedWidget
{

public:
    explicit BuildEnvironmentWidget(BuildConfiguration *bc)
        : NamedWidget(Tr::tr("Build Environment"))
    {
        auto clearBox = new QCheckBox(Tr::tr("Clear system environment"), this);
        clearBox->setChecked(!bc->useSystemEnvironment());

        auto envWidget = new EnvironmentWidget(this, EnvironmentWidget::TypeLocal, clearBox);
        envWidget->setBaseEnvironment(bc->baseEnvironment());
        envWidget->setBaseEnvironmentText(bc->baseEnvironmentText());
        envWidget->setUserChanges(bc->userEnvironmentChanges());

        const EnvironmentWidget::OpenTerminalFunc openTerminalFunc
                = [bc](const Utils::Environment &env) {
            Core::FileUtils::openTerminal(bc->buildDirectory(), env);
        };
        envWidget->setOpenTerminalFunc(openTerminalFunc);

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
public:
    CustomParsersBuildWidget(BuildConfiguration *bc) : NamedWidget(Tr::tr("Custom Output Parsers"))
    {
        const auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        const auto pasteStdOutCB = new QCheckBox(Tr::tr("Parse standard output during build"), this);
        pasteStdOutCB->setToolTip(Tr::tr("Makes output parsers look for diagnostics "
                                     "on stdout rather than stderr."));
        pasteStdOutCB->setChecked(bc->parseStdOut());
        layout->addWidget(pasteStdOutCB);

        connect(pasteStdOutCB, &QCheckBox::clicked, bc, &BuildConfiguration::setParseStdOut);
        const auto selectionWidget = new CustomParsersSelectionWidget(this);
        layout->addWidget(selectionWidget);

        connect(selectionWidget, &CustomParsersSelectionWidget::selectionChanged, this,
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
        : m_buildSteps(bc, Constants::BUILDSTEPS_BUILD)
        , m_cleanSteps(bc, Constants::BUILDSTEPS_CLEAN)
        , m_buildDirectoryAspect(bc, bc)
        , m_tooltipAspect(bc)
    {}

    bool m_clearSystemEnvironment = false;
    EnvironmentItems m_userEnvironmentChanges;
    BuildStepList m_buildSteps;
    BuildStepList m_cleanSteps;
    BuildDirectoryAspect m_buildDirectoryAspect;
    StringAspect m_tooltipAspect;
    FilePath m_lastEmittedBuildDirectory;
    mutable Environment m_cachedEnvironment;
    QString m_configWidgetDisplayName;
    bool m_configWidgetHasFrame = false;
    QList<Utils::Id> m_initialBuildSteps;
    QList<Utils::Id> m_initialCleanSteps;
    Utils::MacroExpander m_macroExpander;
    bool m_parseStdOut = false;
    QList<Utils::Id> m_customParsers;

    // FIXME: Remove.
    BuildConfiguration::BuildType m_initialBuildType = BuildConfiguration::Unknown;
    std::function<void(const BuildInfo &)> m_initializer;
};

} // Internal

BuildConfiguration::BuildConfiguration(Target *target, Utils::Id id)
    : ProjectConfiguration(target, id)
    , d(new Internal::BuildConfigurationPrivate(this))
{
    MacroExpander *expander = macroExpander();
    expander->setDisplayName(Tr::tr("Build Settings"));
    expander->setAccumulating(true);
    expander->registerSubProvider([target] { return target->macroExpander(); });

    expander->registerVariable("buildDir", Tr::tr("Build directory"),
            [this] { return buildDirectory().toUserOutput(); });

    expander->registerFileVariables("BuildConfig:BuildDirectory",
                                    Tr::tr("Build directory"),
                                    [this] { return buildDirectory(); });

    expander->registerVariable("BuildConfig:Name", Tr::tr("Name of the build configuration"),
            [this] { return displayName(); });

    expander->registerPrefix("BuildConfig:Env",
                             Tr::tr("Variables in the build configuration's environment"),
                             [this](const QString &var) { return environment().expandedValueForKey(var); });

    connect(Core::ICore::instance(), &Core::ICore::systemEnvironmentChanged,
            this, &BuildConfiguration::updateCacheAndEmitEnvironmentChanged);
    connect(target, &Target::kitChanged,
            this, &BuildConfiguration::updateCacheAndEmitEnvironmentChanged);
    connect(this, &BuildConfiguration::environmentChanged,
            this, &BuildConfiguration::emitBuildDirectoryChanged);
    connect(target->project(), &Project::environmentChanged,
            this, &BuildConfiguration::updateCacheAndEmitEnvironmentChanged);
    // Many macroexpanders are based on the current project, so they may change the environment:
    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            this, &BuildConfiguration::updateCacheAndEmitEnvironmentChanged);

    d->m_buildDirectoryAspect.setBaseFileName(target->project()->projectDirectory());
    d->m_buildDirectoryAspect.setEnvironment(environment());
    d->m_buildDirectoryAspect.setMacroExpanderProvider([this] { return macroExpander(); });
    connect(&d->m_buildDirectoryAspect, &StringAspect::changed,
            this, &BuildConfiguration::emitBuildDirectoryChanged);
    connect(this, &BuildConfiguration::environmentChanged, this, [this] {
        d->m_buildDirectoryAspect.setEnvironment(environment());
        emit this->target()->buildEnvironmentChanged(this);
    });

    d->m_tooltipAspect.setLabelText(Tr::tr("Tooltip in target selector:"));
    d->m_tooltipAspect.setToolTip(Tr::tr("Appears as a tooltip when hovering the build configuration"));
    d->m_tooltipAspect.setDisplayStyle(StringAspect::LineEditDisplay);
    d->m_tooltipAspect.setSettingsKey("ProjectExplorer.BuildConfiguration.Tooltip");
    connect(&d->m_tooltipAspect, &StringAspect::changed, this, [this] {
        setToolTip(d->m_tooltipAspect());
    });

    connect(target, &Target::parsingStarted, this, &BuildConfiguration::enabledChanged);
    connect(target, &Target::parsingFinished, this, &BuildConfiguration::enabledChanged);
    connect(this, &BuildConfiguration::enabledChanged, this, [this] {
        if (isActive() && project() == ProjectManager::startupProject()) {
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
    FilePath path = FilePath::fromUserInput(
        environment().expandVariables(d->m_buildDirectoryAspect.value().trimmed()));
    // FIXME: If the macro expander is expected to be able to do some
    // structual changes, the fromUserInput() above might already have
    // mis-parsed. Should this here be encapsulated in the FilePathAspect?
    path = macroExpander()->expand(path);
    path = path.cleanPath();

    const FilePath projectDir = target()->project()->projectDirectory();

    return projectDir.resolvePath(path);
}

FilePath BuildConfiguration::rawBuildDirectory() const
{
    return d->m_buildDirectoryAspect();
}

void BuildConfiguration::setBuildDirectory(const FilePath &dir)
{
    if (dir == d->m_buildDirectoryAspect())
        return;
    d->m_buildDirectoryAspect.setValue(dir);
    const FilePath fixedDir = BuildDirectoryAspect::fixupDir(buildDirectory());
    if (!fixedDir.isEmpty())
        d->m_buildDirectoryAspect.setValue(fixedDir);
    emitBuildDirectoryChanged();
}

void BuildConfiguration::addConfigWidgets(const std::function<void(NamedWidget *)> &adder)
{
    if (NamedWidget *generalConfigWidget = createConfigWidget())
        adder(generalConfigWidget);

    adder(new Internal::BuildStepListWidget(buildSteps()));
    adder(new Internal::BuildStepListWidget(cleanSteps()));

    const QList<NamedWidget *> subConfigWidgets = createSubConfigWidgets();
    for (NamedWidget *subConfigWidget : subConfigWidgets)
        adder(subConfigWidget);
}

void BuildConfiguration::doInitialize(const BuildInfo &info)
{
    updateCacheAndEmitEnvironmentChanged();

    setDisplayName(info.displayName);
    setDefaultDisplayName(info.displayName);
    setBuildDirectory(info.buildDirectory);

    d->m_initialBuildType = info.buildType;

    for (Utils::Id id : std::as_const(d->m_initialBuildSteps))
        d->m_buildSteps.appendStep(id);

    for (Utils::Id id : std::as_const(d->m_initialCleanSteps))
        d->m_cleanSteps.appendStep(id);

    if (d->m_initializer)
        d->m_initializer(info);
}

MacroExpander *BuildConfiguration::macroExpander() const
{
    return &d->m_macroExpander;
}

bool BuildConfiguration::createBuildDirectory()
{
    const bool result = buildDirectory().ensureWritableDir().has_value();
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

    Layouting::Form form;
    for (BaseAspect *aspect : aspects()) {
        if (aspect->isVisible()) {
            form.addItem(aspect);
            form.addItem(Layouting::br);
        }
    }
    form.addItem(Layouting::noMargin);
    form.attachTo(widget);

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

void BuildConfiguration::toMap(Store &map) const
{
    ProjectConfiguration::toMap(map);

    map.insert(Constants::CLEAR_SYSTEM_ENVIRONMENT_KEY, d->m_clearSystemEnvironment);
    map.insert(Constants::USER_ENVIRONMENT_CHANGES_KEY,
               EnvironmentItem::toStringList(d->m_userEnvironmentChanges));

    map.insert(BUILD_STEP_LIST_COUNT, 2);
    map.insert(numberedKey(BUILD_STEP_LIST_PREFIX, 0), variantFromStore(d->m_buildSteps.toMap()));
    map.insert(numberedKey(BUILD_STEP_LIST_PREFIX, 1), variantFromStore(d->m_cleanSteps.toMap()));

    map.insert(PARSE_STD_OUT_KEY, d->m_parseStdOut);
    map.insert(CUSTOM_PARSERS_KEY, transform(d->m_customParsers, &Id::toSetting));
}

void BuildConfiguration::fromMap(const Store &map)
{
    d->m_clearSystemEnvironment = map.value(Constants::CLEAR_SYSTEM_ENVIRONMENT_KEY).toBool();
    d->m_userEnvironmentChanges = EnvironmentItem::fromStringList(
        map.value(Constants::USER_ENVIRONMENT_CHANGES_KEY).toStringList());

    updateCacheAndEmitEnvironmentChanged();

    d->m_buildSteps.clear();
    d->m_cleanSteps.clear();

    int maxI = map.value(BUILD_STEP_LIST_COUNT, 0).toInt();
    for (int i = 0; i < maxI; ++i) {
        Store data = storeFromVariant(map.value(numberedKey(BUILD_STEP_LIST_PREFIX, i)));
        if (data.isEmpty()) {
            qWarning() << "No data for build step list" << i << "found!";
            continue;
        }
        Id id = idFromMap(data);
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

    d->m_parseStdOut = map.value(PARSE_STD_OUT_KEY).toBool();
    d->m_customParsers = transform(map.value(CUSTOM_PARSERS_KEY).toList(), &Id::fromSetting);

    ProjectConfiguration::fromMap(map);
    setToolTip(d->m_tooltipAspect());
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
    return &d->m_buildDirectoryAspect;
}

void BuildConfiguration::setConfigWidgetDisplayName(const QString &display)
{
    d->m_configWidgetDisplayName = display;
}

void BuildConfiguration::setBuildDirectoryHistoryCompleter(const Key &history)
{
    d->m_buildDirectoryAspect.setHistoryCompleter(history);
}

void BuildConfiguration::setConfigWidgetHasFrame(bool configWidgetHasFrame)
{
    d->m_configWidgetHasFrame = configWidgetHasFrame;
}

void BuildConfiguration::setBuildDirectorySettingsKey(const Key &key)
{
    d->m_buildDirectoryAspect.setSettingsKey(key);
}

Environment BuildConfiguration::baseEnvironment() const
{
    Environment result;
    if (useSystemEnvironment()) {
        ProjectExplorer::IDevice::ConstPtr devicePtr = BuildDeviceKitAspect::device(kit());
        result = devicePtr ? devicePtr->systemEnvironment() : Environment::systemEnvironment();
    }
    addToEnvironment(result);
    kit()->addToBuildEnvironment(result);
    result.modify(project()->additionalEnvironment());
    return result;
}

QString BuildConfiguration::baseEnvironmentText() const
{
    if (useSystemEnvironment())
        return Tr::tr("System Environment");
    else
        return Tr::tr("Clean Environment");
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

bool BuildConfiguration::parseStdOut() const { return d->m_parseStdOut; }
void BuildConfiguration::setParseStdOut(bool b) { d->m_parseStdOut = b; }

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
    return buildSystem()->hasParsingData();
}

QString BuildConfiguration::disabledReason() const
{
    return buildSystem()->hasParsingData() ? QString()
                                           : Tr::tr("The project was not parsed successfully.");
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

FilePath BuildConfiguration::buildDirectoryFromTemplate(const FilePath &projectDir,
                                                        const FilePath &mainFilePath,
                                                        const QString &projectName,
                                                        const Kit *kit,
                                                        const QString &bcName,
                                                        BuildType buildType,
                                                        const QString &buildSystem)
{
    MacroExpander exp;

    qCDebug(bcLog) << Q_FUNC_INFO << projectDir << mainFilePath << projectName << bcName;

    exp.registerFileVariables("Project",
                              Tr::tr("Main file of the project"),
                              [mainFilePath] { return mainFilePath; });
    exp.registerVariable("Project:Name",
                         Tr::tr("Name of the project"),
                         [projectName] { return projectName; });
    exp.registerVariable("BuildConfig:Name",
                         Tr::tr("Name of the project's active build configuration"),
                         [bcName] { return bcName; });
    exp.registerVariable("BuildSystem:Name",
                         Tr::tr("Name of the project's active build system"),
                         [buildSystem] { return buildSystem; });
    exp.registerVariable("CurrentBuild:Type",
                         Tr::tr("Type of current build"),
                         [buildType] { return buildTypeName(buildType); }, false);
    exp.registerVariable("BuildConfig:Type",
                         Tr::tr("Type of the project's active build configuration"),
                         [buildType] { return buildTypeName(buildType); });
    exp.registerSubProvider([kit] { return kit->macroExpander(); });

    FilePath buildDir = FilePath::fromUserInput(buildPropertiesSettings().buildDirectoryTemplate());
    qCDebug(bcLog) << "build dir template:" << buildDir.toUserOutput();
    buildDir = exp.expand(buildDir);
    qCDebug(bcLog) << "expanded build:" << buildDir.toUserOutput();
    buildDir = buildDir.withNewPath(buildDir.path().replace(" ", "-"));

    auto buildDevice = BuildDeviceKitAspect::device(kit);

    if (buildDir.isAbsolutePath()) {
        bool isReachable = buildDevice->ensureReachable(buildDir);
        if (!isReachable)
            return {};
        return buildDevice->rootPath().withNewMappedPath(buildDir);
    }

    bool isReachable = buildDevice->ensureReachable(projectDir);
    if (!isReachable)
        return {};

    const FilePath baseDir = buildDevice->rootPath().withNewMappedPath(projectDir);
    return baseDir.resolvePath(buildDir);
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

const Tasks BuildConfigurationFactory::reportIssues(Kit *kit, const FilePath &projectPath,
                                                    const FilePath &buildDir) const
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
    for (BuildConfigurationFactory *factory : std::as_const(g_buildConfigurationFactories)) {
        if (Utils::mimeTypeForFile(projectPath).matchesName(factory->m_supportedProjectMimeTypeName)
            && factory->supportsTargetDeviceType(deviceType))
            return factory;
    }
    return nullptr;
}

// create
BuildConfigurationFactory * BuildConfigurationFactory::find(Target *parent)
{
    for (BuildConfigurationFactory *factory : std::as_const(g_buildConfigurationFactories)) {
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

BuildConfiguration *BuildConfigurationFactory::restore(Target *parent, const Store &map)
{
    const Utils::Id id = idFromMap(map);
    for (BuildConfigurationFactory *factory : std::as_const(g_buildConfigurationFactories)) {
        QTC_ASSERT(factory->m_creator, return nullptr);
        if (!factory->canHandle(parent))
            continue;
        if (!id.name().startsWith(factory->m_buildConfigId.name()))
            continue;
        BuildConfiguration *bc = factory->m_creator(parent);
        QTC_ASSERT(bc, return nullptr);
        bc->fromMap(map);
        if (bc->hasError()) {
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
    Store map;
    source->toMap(map);
    return restore(parent, map);
}

} // namespace ProjectExplorer
