// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildconfiguration.h"

#include "buildaspects.h"
#include "buildinfo.h"
#include "buildmanager.h"
#include "buildpropertiessettings.h"
#include "buildsteplist.h"
#include "buildstepspage.h"
#include "buildsystem.h"
#include "customparser.h"
#include "deployconfiguration.h"
#include "devicesupport/devicekitaspects.h"
#include "environmentwidget.h"
#include "kit.h"
#include "miniprojecttargetselector.h"
#include "projectconfigurationmodel.h"
#include "projectexplorerconstants.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "project.h"
#include "projectmanager.h"
#include "projecttree.h"
#include "target.h"

#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/idevice.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
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
const char EXTRA_DATA_KEY[] = "ProjectExplorer.Target.PluginSettings";

const char ACTIVE_DC_KEY[] = "ProjectExplorer.Target.ActiveDeployConfiguration";
const char DC_KEY_PREFIX[] = "ProjectExplorer.Target.DeployConfiguration.";
const char DC_COUNT_KEY[] = "ProjectExplorer.Target.DeployConfigurationCount";

const char ACTIVE_RC_KEY[] = "ProjectExplorer.Target.ActiveRunConfiguration";
const char RC_KEY_PREFIX[] = "ProjectExplorer.Target.RunConfiguration.";
const char RC_COUNT_KEY[] = "ProjectExplorer.Target.RunConfigurationCount";

Q_LOGGING_CATEGORY(bcLog, "qtc.buildconfig", QtWarningMsg)

namespace ProjectExplorer {
namespace Internal {

class BuildEnvironmentWidget : public QWidget
{
public:
    explicit BuildEnvironmentWidget(BuildConfiguration *bc)
    {
        setWindowTitle(Tr::tr("Build Environment"));

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

class CustomParsersBuildWidget : public QWidget
{
public:
    CustomParsersBuildWidget(BuildConfiguration *bc)
    {
        setWindowTitle(Tr::tr("Custom Output Parsers"));

        const auto pasteStdOutCB = new QCheckBox(Tr::tr("Parse standard output during build"), this);
        pasteStdOutCB->setToolTip(Tr::tr("Makes output parsers look for diagnostics "
                                     "on stdout rather than stderr."));
        pasteStdOutCB->setChecked(bc->parseStdOut());
        connect(pasteStdOutCB, &QCheckBox::clicked, bc, &BuildConfiguration::setParseStdOut);

        const auto selectionWidget =
                new CustomParsersSelectionWidget(CustomParsersSelectionWidget::InBuildConfig, this);

        const auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(pasteStdOutCB);
        layout->addWidget(selectionWidget);

        QList<Id> parsers = bc->customParsers();
        for (const auto &s : ProjectExplorerPlugin::customParsers()) {
            if (s.buildDefault && !parsers.contains(s.id))
                parsers.append(s.id);
        }
        selectionWidget->setSelectedParsers(parsers);
        connect(selectionWidget, &CustomParsersSelectionWidget::selectionChanged, this,
                [selectionWidget, bc] {
            bc->setCustomParsers(selectionWidget->selectedParsers());
        });
    }
};


class BuildConfigurationPrivate
{
public:
    BuildConfigurationPrivate(BuildConfiguration *bc)
        : m_buildSteps(bc, Constants::BUILDSTEPS_BUILD)
        , m_cleanSteps(bc, Constants::BUILDSTEPS_CLEAN)
        , m_buildDirectoryAspect(bc)
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
    bool m_parseStdOut = false;
    QList<Utils::Id> m_customParsers;
    Store m_extraData;
    BuildSystem *m_buildSystem = nullptr;
    QList<DeployConfiguration *> m_deployConfigurations;
    DeployConfiguration *m_activeDeployConfiguration = nullptr;
    QList<RunConfiguration *> m_runConfigurations;
    RunConfiguration* m_activeRunConfiguration = nullptr;

    ProjectConfigurationModel m_deployConfigurationModel;
    ProjectConfigurationModel m_runConfigurationModel;

    // FIXME: Remove.
    BuildConfiguration::BuildType m_initialBuildType = BuildConfiguration::Unknown;
    std::function<void(const BuildInfo &)> m_initializer;
};

} // Internal

BuildConfiguration::BuildConfiguration(Target *target, Utils::Id id)
    : ProjectConfiguration(target, id)
    , d(new Internal::BuildConfigurationPrivate(this))
{
    d->m_buildSystem = project()->createBuildSystem(this);

    MacroExpander *expander = macroExpander();
    expander->setDisplayName(Tr::tr("Build Settings"));
    expander->setAccumulating(true);
    expander->registerSubProvider([this] { return kit()->macroExpander(); });
    expander->registerVariable("sourceDir", Tr::tr("Source directory"),
                               [this] { return project()->projectDirectory().toUserOutput(); },
                               false);
    expander->registerVariable("BuildSystem:Name", Tr::tr("Build system"), [this] {
        return buildSystem()->name();
    });
    expander->registerVariable("Project:Name", Tr::tr("Name of current project"), [this] {
        return project()->displayName();
    });
    expander->registerVariable("buildDir", Tr::tr("Build directory"),
            [this] { return buildDirectory().toUserOutput(); }, false);
    expander->registerFileVariables("BuildConfig:BuildDirectory",
                                    Tr::tr("Build directory"),
                                    [this] { return buildDirectory(); });
    expander->registerFileVariables("Project", Tr::tr("Main file of the project"), [this] {
        return project()->projectFilePath();
    });
    expander->registerVariable("BuildConfig:Name", Tr::tr("Name of the build configuration"),
            [this] { return displayName(); });
    expander->registerPrefix(
        "BuildConfig:Env",
        "USER",
        Tr::tr("Variables in the build configuration's environment"),
        [this](const QString &var) { return environment().expandedValueForKey(var); });

    connect(Core::ICore::instance(), &Core::ICore::systemEnvironmentChanged,
            this, &BuildConfiguration::updateCacheAndEmitEnvironmentChanged);
    connect(this, &BuildConfiguration::kitChanged,
            this, &BuildConfiguration::updateCacheAndEmitEnvironmentChanged);
    connect(this, &BuildConfiguration::environmentChanged,
            this, &BuildConfiguration::emitBuildDirectoryChanged);
    connect(project(), &Project::environmentChanged,
            this, &BuildConfiguration::updateCacheAndEmitEnvironmentChanged);
    // Many macroexpanders are based on the current project, so they may change the environment:
    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            this, &BuildConfiguration::updateCacheAndEmitEnvironmentChanged);

    d->m_buildDirectoryAspect.setBaseDirectory(project()->projectDirectory());
    d->m_buildDirectoryAspect.setEnvironment(environment());
    connect(&d->m_buildDirectoryAspect, &StringAspect::changed,
            this, &BuildConfiguration::emitBuildDirectoryChanged);
    connect(this, &BuildConfiguration::environmentChanged, this, [this] {
        d->m_buildDirectoryAspect.setEnvironment(environment());
    });

    d->m_tooltipAspect.setLabelText(Tr::tr("Tooltip in target selector:"));
    d->m_tooltipAspect.setToolTip(Tr::tr("Appears as a tooltip when hovering the build configuration"));
    d->m_tooltipAspect.setDisplayStyle(StringAspect::LineEditDisplay);
    d->m_tooltipAspect.setSettingsKey("ProjectExplorer.BuildConfiguration.Tooltip");
    connect(&d->m_tooltipAspect, &StringAspect::changed, this, [this] {
        setToolTip(d->m_tooltipAspect());
    });

    connect(buildSystem(), &BuildSystem::parsingStarted, this, &BuildConfiguration::enabledChanged);
    connect(buildSystem(), &BuildSystem::parsingFinished, this, [this](bool success) {
        if (success)
            updateDefaultRunConfigurations();
        emit enabledChanged();
    }, Qt::QueuedConnection); // Must wait for run configs to change their enabled state.

    connect(this, &BuildConfiguration::enabledChanged, this, [this] {
        if (isActive() && project() == ProjectManager::startupProject()) {
            ProjectExplorerPlugin::updateActions();
            ProjectExplorerPlugin::updateRunActions();
        }
    });

    connect(target, &Target::kitChanged, this, [this] {
        updateDefaultDeployConfigurations();
        emit kitChanged();
    });
}

BuildConfiguration::~BuildConfiguration()
{
    qDeleteAll(d->m_deployConfigurations);
    qDeleteAll(d->m_runConfigurations);
    delete d->m_buildSystem;
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

    return project()->projectDirectory().resolvePath(path);
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

QList<QWidget *> BuildConfiguration::createConfigWidgets()
{
    QList<QWidget *> result;
    if (QWidget *generalConfigWidget = createConfigWidget()) {
        generalConfigWidget->setWindowTitle(d->m_configWidgetDisplayName);
        result.append(generalConfigWidget);
    }

    result.append(new Internal::BuildStepListWidget(buildSteps()));
    result.append(new Internal::BuildStepListWidget(cleanSteps()));

    result.append(createSubConfigWidgets());

    return result;
}

QList<QWidget *> BuildConfiguration::createSubConfigWidgets()
{
    return {
        new Internal::BuildEnvironmentWidget(this),
        new Internal::CustomParsersBuildWidget(this)
    };
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

    project()->syncRunConfigurations(true);
}

bool BuildConfiguration::createBuildDirectory()
{
    const bool result = bool(buildDirectory().ensureWritableDir());
    buildDirectoryAspect()->validateInput();
    return result;
}

void BuildConfiguration::setInitialArgs(const QStringList &)
{
    QTC_CHECK(false);
}

QStringList BuildConfiguration::initialArgs() const
{
    QTC_CHECK(false);
    return {};
}

QStringList BuildConfiguration::additionalArgs() const
{
    QTC_CHECK(false);
    return {};
}

void BuildConfiguration::setInitializer(const std::function<void(const BuildInfo &)> &initializer)
{
    d->m_initializer = initializer;
}

bool BuildConfiguration::addConfigurationsFromMap(
    const Utils::Store &map, bool setActiveConfigurations)
{
    bool ok = true;
    int dcCount = map.value(DC_COUNT_KEY, 0).toInt(&ok);
    if (!ok || dcCount < 0)
        dcCount = 0;
    int activeDc = map.value(ACTIVE_DC_KEY, 0).toInt(&ok);
    if (!ok || 0 > activeDc || dcCount < activeDc)
        activeDc = 0;
    if (!setActiveConfigurations)
        activeDc = -1;

    for (int i = 0; i < dcCount; ++i) {
        const Key key = numberedKey(DC_KEY_PREFIX, i);
        if (!map.contains(key))
            return false;
        Store valueMap = storeFromVariant(map.value(key));
        DeployConfiguration *dc = DeployConfigurationFactory::restore(this, valueMap);
        if (!dc) {
            Utils::Id id = idFromMap(valueMap);
            qWarning("No factory found to restore deployment configuration of id '%s'!",
                     id.isValid() ? qPrintable(id.toString()) : "UNKNOWN");
            continue;
        }
        QTC_CHECK(dc->id() == ProjectExplorer::idFromMap(valueMap));
        addDeployConfiguration(dc);
        if (i == activeDc)
            setActiveDeployConfiguration(dc);
    }


    int rcCount = map.value(RC_COUNT_KEY, 0).toInt(&ok);
    if (!ok || rcCount < 0)
        rcCount = 0;
    int activeRc = map.value(ACTIVE_RC_KEY, 0).toInt(&ok);
    if (!ok || 0 > activeRc || rcCount < activeRc)
        activeRc = 0;
    if (!setActiveConfigurations)
        activeRc = -1;

    for (int i = 0; i < rcCount; ++i) {
        const Key key = numberedKey(RC_KEY_PREFIX, i);
        if (!map.contains(key))
            return false;

        // Ignore missing RCs: We will just populate them using the default ones.
        Store valueMap = storeFromVariant(map.value(key));
        RunConfiguration *rc = RunConfigurationFactory::restore(this, valueMap);
        if (!rc)
            continue;
        addRunConfiguration(rc, NameHandling::Keep);
        if (i == activeRc)
            setActiveRunConfiguration(rc);
    }

    return true;
}

void BuildConfiguration::setExtraDataFromMap(const Utils::Store &map)
{
    d->m_extraData = storeFromVariant(map.value(EXTRA_DATA_KEY));
}

void BuildConfiguration::storeConfigurationsToMap(Utils::Store &map) const
{
    const QList<DeployConfiguration *> dcs = deployConfigurations();
    map.insert(ACTIVE_DC_KEY, dcs.indexOf(d->m_activeDeployConfiguration));
    map.insert(DC_COUNT_KEY, dcs.size());
    for (int i = 0; i < dcs.size(); ++i) {
        Store data;
        dcs.at(i)->toMap(data);
        map.insert(numberedKey(DC_KEY_PREFIX, i), variantFromStore(data));
    }

    const QList<RunConfiguration *> rcs = runConfigurations();
    map.insert(ACTIVE_RC_KEY, rcs.indexOf(d->m_activeRunConfiguration));
    map.insert(RC_COUNT_KEY, rcs.size());
    for (int i = 0; i < rcs.size(); ++i) {
        Store data;
        rcs.at(i)->toMap(data);
        map.insert(numberedKey(RC_KEY_PREFIX, i), variantFromStore(data));
    }
}

void BuildConfiguration::setActiveDeployConfiguration(DeployConfiguration *dc)
{
    if (dc) {
        QTC_ASSERT(d->m_deployConfigurations.contains(dc), return);
    } else {
        QTC_ASSERT(d->m_deployConfigurations.isEmpty(), return);
    }
    if (dc == d->m_activeDeployConfiguration)
        return;

    d->m_activeDeployConfiguration = dc;
    emit activeDeployConfigurationChanged(d->m_activeDeployConfiguration);
    if (this == target()->activeBuildConfiguration())
        emit target()->activeDeployConfigurationChanged(d->m_activeDeployConfiguration);
}

QWidget *BuildConfiguration::createConfigWidget()
{
    QWidget *named = new QWidget;

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
    form.setNoMargins();
    for (BaseAspect *aspect : aspects()) {
        if (aspect->isVisible()) {
            form.addItem(aspect);
            form.flush();
        }
    }
    form.attachTo(widget);

    return named;
}

BuildSystem *BuildConfiguration::buildSystem() const
{
    return d->m_buildSystem;
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

void BuildConfiguration::addDeployConfiguration(DeployConfiguration *dc)
{
    QTC_ASSERT(dc && !d->m_deployConfigurations.contains(dc), return);
    QTC_ASSERT(dc->buildConfiguration() == this, return);

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = dc->displayName();
    QStringList displayNames = Utils::transform(d->m_deployConfigurations, &DeployConfiguration::displayName);
    configurationDisplayName = Utils::makeUniquelyNumbered(configurationDisplayName, displayNames);
    dc->setDisplayName(configurationDisplayName);

    // add it
    d->m_deployConfigurations.push_back(dc);

    ProjectExplorerPlugin::targetSelector()->addedDeployConfiguration(dc); // TODO: Use signal instead?
    d->m_deployConfigurationModel.addProjectConfiguration(dc);
    emit addedDeployConfiguration(dc);
    if (this == target()->activeBuildConfiguration())
        emit target()->addedDeployConfiguration(dc);

    if (!d->m_activeDeployConfiguration)
        setActiveDeployConfiguration(dc);
    Q_ASSERT(activeDeployConfiguration());

}

bool BuildConfiguration::removeDeployConfiguration(DeployConfiguration *dc)
{
    if (!d->m_deployConfigurations.contains(dc))
        return false;

    if (BuildManager::isBuilding(dc))
        return false;

    d->m_deployConfigurations.removeOne(dc);

    if (activeDeployConfiguration() == dc) {
        if (d->m_deployConfigurations.isEmpty())
            setActiveDeployConfiguration(nullptr, SetActive::Cascade);
        else
            setActiveDeployConfiguration(d->m_deployConfigurations.at(0), SetActive::Cascade);
    }

    ProjectExplorerPlugin::targetSelector()->removedDeployConfiguration(dc);
    d->m_deployConfigurationModel.removeProjectConfiguration(dc);
    emit removedDeployConfiguration(dc);
    if (this == target()->activeBuildConfiguration())
        emit target()->removedDeployConfiguration(dc);

    delete dc;
    return true;

}

const QList<DeployConfiguration *> BuildConfiguration::deployConfigurations() const
{
    return d->m_deployConfigurations;
}

DeployConfiguration *BuildConfiguration::activeDeployConfiguration() const
{
    return d->m_activeDeployConfiguration;
}

void BuildConfiguration::setActiveDeployConfiguration(DeployConfiguration *dc, SetActive cascade)
{
    QTC_ASSERT(project(), return);

    if (project()->isShuttingDown() || target()->isShuttingDown()) // Do we need our own isShuttungDown()?
        return;

    setActiveDeployConfiguration(dc);

    if (!dc)
        return;
    if (cascade != SetActive::Cascade || !ProjectManager::isProjectConfigurationCascading())
        return;

    Id kitId = kit()->id();
    QString name = dc->displayName(); // We match on displayname
    for (Project *otherProject : ProjectManager::projects()) {
        if (otherProject == project())
            continue;
        Target *otherTarget = otherProject->activeTarget();
        if (!otherTarget || otherTarget->kit()->id() != kitId)
            continue;

        for (BuildConfiguration *otherBc : otherTarget->buildConfigurations()) {
            for (DeployConfiguration *otherDc : otherBc->deployConfigurations()) {
                if (otherDc->displayName() == name) {
                    otherBc->setActiveDeployConfiguration(otherDc);
                    break;
                }
            }
        }
    }
}

void BuildConfiguration::updateDefaultDeployConfigurations()
{
    const QList<DeployConfigurationFactory *> dcFactories = DeployConfigurationFactory::find(
        target());
    QTC_ASSERT(!dcFactories.isEmpty(), qDebug() << target()->id(); return);

    QList<Utils::Id> dcIds;
    for (const DeployConfigurationFactory *dcFactory : dcFactories)
        dcIds.append(dcFactory->creationId());

    const QList<DeployConfiguration *> dcList = deployConfigurations();
    QList<Utils::Id> toCreate = dcIds;

    for (DeployConfiguration *dc : dcList) {
        if (dcIds.contains(dc->id()))
            toCreate.removeOne(dc->id());
        else
            removeDeployConfiguration(dc);
    }

    for (Utils::Id id : std::as_const(toCreate)) {
        for (DeployConfigurationFactory *dcFactory : dcFactories) {
            if (dcFactory->creationId() == id) {
                DeployConfiguration *dc = dcFactory->create(this);
                if (dc) {
                    QTC_CHECK(dc->id() == id);
                    addDeployConfiguration(dc);
                }
            }
        }
    }
}

void BuildConfiguration::updateDefaultRunConfigurations()
{
    // Manual and Auto
    const QList<RunConfigurationCreationInfo> creators
        = RunConfigurationFactory::creatorsForBuildConfig(this);

    if (creators.isEmpty()) {
        qWarning("No run configuration factory found for target id '%s'.", qPrintable(id().toString()));
        return;
    }

    QList<RunConfiguration *> existingConfigured; // Existing configured RCs
    QList<RunConfiguration *> existingUnconfigured; // Existing unconfigured RCs
    QList<RunConfiguration *> newConfigured; // NEW configured Rcs
    QList<RunConfiguration *> newUnconfigured; // NEW unconfigured RCs

    // sort existing RCs into configured/unconfigured.
    std::tie(existingConfigured, existingUnconfigured)
        = Utils::partition(runConfigurations(),
                           [](const RunConfiguration *rc) { return rc->isConfigured(); });
    int configuredCount = existingConfigured.count();

    // Put outdated RCs into toRemove, do not bother with factories
    // that produce already existing RCs
    QList<RunConfiguration *> toRemove;
    QList<RunConfigurationCreationInfo> existing;
    for (RunConfiguration *rc : std::as_const(existingConfigured)) {
        bool present = false;
        for (const RunConfigurationCreationInfo &item : creators) {
            QString buildKey = rc->buildKey();
            if (item.factory->runConfigurationId() == rc->id() && item.buildKey == buildKey) {
                existing.append(item);
                present = true;
            }
        }
        if (!present &&
            ProjectExplorerSettings::get(this).automaticallyCreateRunConfigurations() &&
            !rc->isCustomized()) {
            toRemove.append(rc);
        }
    }
    configuredCount -= toRemove.count();

    bool removeExistingUnconfigured = false;
    if (ProjectExplorerSettings::get(this).automaticallyCreateRunConfigurations()) {
        // Create new "automatic" RCs and put them into newConfigured/newUnconfigured
        for (const RunConfigurationCreationInfo &item : creators) {
            if (item.creationMode == RunConfigurationCreationInfo::ManualCreationOnly)
                continue;
            bool exists = false;
            for (const RunConfigurationCreationInfo &ex : existing) {
                if (ex.factory == item.factory && ex.buildKey == item.buildKey)
                    exists = true;
            }
            if (exists)
                continue;

            RunConfiguration *rc = item.create(this);
            if (!rc)
                continue;
            QTC_CHECK(rc->id() == item.factory->runConfigurationId());
            if (!rc->isConfigured())
                newUnconfigured << rc;
            else
                newConfigured << rc;
        }
        configuredCount += newConfigured.count();

        // Decide what to do with the different categories:
        if (configuredCount > 0) {
            // new non-Custom Executable RCs were added
            removeExistingUnconfigured = true;
            qDeleteAll(newUnconfigured);
            newUnconfigured.clear();
        } else {
            // no new RCs, use old or new CERCs?
            if (!existingUnconfigured.isEmpty()) {
                qDeleteAll(newUnconfigured);
                newUnconfigured.clear();
            }
        }
    }

    // Do actual changes:
    for (RunConfiguration *rc : std::as_const(newConfigured))
        addRunConfiguration(rc, NameHandling::Uniquify);
    for (RunConfiguration *rc : std::as_const(newUnconfigured))
        addRunConfiguration(rc, NameHandling::Uniquify);

    // Generate complete list of RCs to remove later:
    QList<RunConfiguration *> removalList;
    for (RunConfiguration *rc : std::as_const(toRemove)) {
        removalList << rc;
        existingConfigured.removeOne(rc); // make sure to also remove them from existingConfigured!
    }

    if (removeExistingUnconfigured) {
        removalList.append(existingUnconfigured);
        existingUnconfigured.clear();
    }

    // Make sure a configured RC will be active after we delete the RCs:
    RunConfiguration *active = activeRunConfiguration();
    if (active && (removalList.contains(active) || !active->isEnabled(Constants::NORMAL_RUN_MODE))) {
        RunConfiguration *newConfiguredDefault = newConfigured.isEmpty() ? nullptr : newConfigured.at(0);

        RunConfiguration *rc = Utils::findOrDefault(existingConfigured, [](RunConfiguration *rc) {
            return rc->isEnabled(Constants::NORMAL_RUN_MODE);
        });
        if (!rc) {
            rc = Utils::findOr(newConfigured, newConfiguredDefault,
                               Utils::equal(&RunConfiguration::displayName, project()->displayName()));
        }
        if (!rc)
            rc = newUnconfigured.isEmpty() ? nullptr : newUnconfigured.at(0);
        if (!rc) {
            // No RCs will be deleted, so use the one that will emit the minimum number of signals.
            // One signal will be emitted from the next setActiveRunConfiguration, another one
            // when the RC gets removed (and the activeRunConfiguration turns into a nullptr).
            rc = removalList.isEmpty() ? nullptr : removalList.last();
        }

        if (rc)
            setActiveRunConfiguration(rc);
    }

    // Remove the RCs that are no longer needed:
    for (RunConfiguration *rc : std::as_const(removalList))
        removeRunConfiguration(rc);

    if (!newConfigured.isEmpty() || !newUnconfigured.isEmpty())
        project()->syncRunConfigurations(true);
    emit runConfigurationsUpdated();
    runConfigurationModel()->triggerUpdate();
}

const QList<RunConfiguration *> BuildConfiguration::runConfigurations() const
{
    return d->m_runConfigurations;
}

void BuildConfiguration::addRunConfiguration(RunConfiguration *rc, NameHandling nameHandling)
{
    QTC_ASSERT(rc && !d->m_runConfigurations.contains(rc), return);
    Q_ASSERT(rc->target() == target());

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = rc->displayName();
    if (!configurationDisplayName.isEmpty() && nameHandling == NameHandling::Uniquify) {
        QStringList displayNames = Utils::transform(d->m_runConfigurations,
                                                    &RunConfiguration::displayName);
        configurationDisplayName = Utils::makeUniquelyNumbered(configurationDisplayName,
                                                               displayNames);
        rc->setDisplayName(configurationDisplayName);
    }

    d->m_runConfigurations.push_back(rc);

    ProjectExplorerPlugin::targetSelector()->addedRunConfiguration(rc);
    d->m_runConfigurationModel.addProjectConfiguration(rc);
    emit addedRunConfiguration(rc);
    if (this == target()->activeBuildConfiguration())
        emit target()->addedRunConfiguration(rc);

    if (!activeRunConfiguration())
        setActiveRunConfiguration(rc);
}

void BuildConfiguration::removeRunConfiguration(RunConfiguration *rc)
{
    QTC_ASSERT(rc && d->m_runConfigurations.contains(rc), return);

    d->m_runConfigurations.removeOne(rc);

    if (activeRunConfiguration() == rc) {
        if (d->m_runConfigurations.isEmpty())
            setActiveRunConfiguration(nullptr);
        else
            setActiveRunConfiguration(d->m_runConfigurations.at(0));
    }

    emit removedRunConfiguration(rc);
    if (this == target()->activeBuildConfiguration())
        emit target()->removedRunConfiguration(rc);
    ProjectExplorerPlugin::targetSelector()->removedRunConfiguration(rc);
    d->m_runConfigurationModel.removeProjectConfiguration(rc);

    delete rc;
}

void BuildConfiguration::removeAllRunConfigurations()
{
    QList<RunConfiguration *> runConfigs = d->m_runConfigurations;
    d->m_runConfigurations.clear();
    setActiveRunConfiguration(nullptr);
    while (!runConfigs.isEmpty()) {
        RunConfiguration * const rc = runConfigs.takeFirst();
        emit removedRunConfiguration(rc);
        if (this == target()->activeBuildConfiguration())
            emit target()->removedRunConfiguration(rc);
        ProjectExplorerPlugin::targetSelector()->removedRunConfiguration(rc);
        d->m_runConfigurationModel.removeProjectConfiguration(rc);
        delete rc;
    }
}

RunConfiguration *BuildConfiguration::activeRunConfiguration() const
{
    return d->m_activeRunConfiguration;
}

void BuildConfiguration::setActiveRunConfiguration(RunConfiguration *rc)
{
    if (target()->isShuttingDown())
        return;

    if ((!rc && d->m_runConfigurations.isEmpty()) ||
        (rc && d->m_runConfigurations.contains(rc) &&
         rc != d->m_activeRunConfiguration)) {
        d->m_activeRunConfiguration = rc;
        emit activeRunConfigurationChanged(d->m_activeRunConfiguration);
        if (this == target()->activeBuildConfiguration())
            emit target()->activeRunConfigurationChanged(d->m_activeRunConfiguration);
        ProjectExplorerPlugin::updateActions();
    }
}

ProjectConfigurationModel *BuildConfiguration::runConfigurationModel() const
{
    return &d->m_runConfigurationModel;
}

QVariant BuildConfiguration::extraData(const Utils::Key &name) const
{
    return d->m_extraData.value(name);
}

void BuildConfiguration::setExtraData(const Utils::Key &name, const QVariant &value)
{
    if (value.isValid())
        d->m_extraData.insert(name, value);
    else
        d->m_extraData.remove(name);
}

ProjectConfigurationModel *BuildConfiguration::deployConfigurationModel() const
{
    return &d->m_deployConfigurationModel;
}

BuildConfiguration *BuildConfiguration::clone(Target *target) const
{
    Store map;
    toMap(map);
    return BuildConfigurationFactory::restore(target, map);
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

    if (!d->m_extraData.isEmpty())
        map.insert(EXTRA_DATA_KEY, variantFromStore(d->m_extraData));

    storeConfigurationsToMap(map);
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
    setExtraDataFromMap(map);
    addConfigurationsFromMap(map, true);
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
    return project()->activeBuildConfiguration() == this;
}

QString BuildConfiguration::activeBuildKey() const
{
    // Should not happen. If it does, return a buildKey that wont be found in
    // the project tree, so that the project()->findNodeForBuildKey(buildKey)
    // returns null.
    QTC_ASSERT(d->m_activeRunConfiguration, return QString(QChar(0)));
    return d->m_activeRunConfiguration->buildKey();
}

void BuildConfiguration::setupBuildDirMacroExpander(
    Utils::MacroExpander &exp,
    const Utils::FilePath &mainFilePath,
    const QString &projectName,
    const Kit *kit,
    const QString &bcName,
    BuildType buildType,
    const QString &buildSystem,
    bool documentationOnly)
{
    exp.registerFileVariables("Project",
                              Tr::tr("Main file of the project"),
                              [mainFilePath] { return mainFilePath; }, true, !documentationOnly);
    exp.registerVariable("Project:Name",
                         Tr::tr("Name of the project"),
                         [projectName] { return projectName; }, true, !documentationOnly);
    exp.registerVariable("BuildConfig:Name",
                         Tr::tr("Name of the project's active build configuration"),
                         [bcName] { return bcName; }, true, !documentationOnly);
    exp.registerVariable("BuildSystem:Name",
                         Tr::tr("Name of the project's active build system"),
                         [buildSystem] { return buildSystem; }, true, !documentationOnly);
    exp.registerVariable("CurrentBuild:Type",
                         Tr::tr("Type of current build"),
                         [buildType] { return buildTypeName(buildType); }, false, false);
    exp.registerVariable("BuildConfig:Type",
                         Tr::tr("Type of the project's active build configuration"),
                         [buildType] { return buildTypeName(buildType); }, true, !documentationOnly);
    if (kit)
        exp.registerSubProvider([kit] { return kit->macroExpander(); });
}

FilePath BuildConfiguration::buildDirectoryFromTemplate(const FilePath &projectDir,
                                                        const FilePath &mainFilePath,
                                                        const QString &projectName,
                                                        const Kit *kit,
                                                        const QString &bcName,
                                                        BuildType buildType,
                                                        const QString &buildSystem)
{
    qCDebug(bcLog) << Q_FUNC_INFO << projectDir << mainFilePath << projectName << bcName;

    MacroExpander exp;
    setupBuildDirMacroExpander(
        exp, mainFilePath, projectName, kit, bcName, buildType, buildSystem, false);

    auto project = ProjectManager::projectWithProjectFilePath(mainFilePath);
    auto environment = Environment::systemEnvironment();
    // This adds the environment variables from the <project>.shared file
    if (project)
        environment.modify(project->additionalEnvironment());

    FilePath buildDir = FilePath::fromUserInput(environment.value_or(
        Constants::QTC_DEFAULT_BUILD_DIRECTORY_TEMPLATE,
        buildPropertiesSettings().buildDirectoryTemplate()));
    qCDebug(bcLog) << "build dir template:" << buildDir.toUserOutput();
    buildDir = exp.expand(buildDir);
    qCDebug(bcLog) << "expanded build:" << buildDir.toUserOutput();
    buildDir = buildDir.withNewPath(buildDir.path().replace(" ", "-"));

    auto buildDevice = BuildDeviceKitAspect::device(kit);
    if (!buildDevice)
        return buildDir;

    if (buildDir.isAbsolutePath())
        return buildDevice->rootPath().withNewMappedPath(buildDir);

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

static Tasks defaultIssueReporter(
    Kit *kit, const Utils::FilePath &projectDir, const Utils::FilePath &buildDirectory)
{
    auto buildDevice = BuildDeviceKitAspect::device(kit);
    if (!buildDevice) {
        return {Task(
            Task::Error,
            Tr::tr("No build device is set for the kit \"%1\".").arg(kit->displayName()),
            {},
            0,
            ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)};
    }

    auto canMountHintFor = [&buildDevice](const FilePath &path) {
        if (buildDevice->canMount(path))
            return Tr::tr("You can try mounting the folder in your device settings.");
        return QString{};
    };

    if (!buildDevice->ensureReachable(projectDir)) {
        return {Task(
            Task::Error,
            Tr::tr("The build device \"%1\" cannot reach the project directory.")
                    .arg(buildDevice->displayName())
                + " " + canMountHintFor(projectDir),
            {},
            0,
            ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)};
    }

    if (!buildDirectory.isEmpty() && !buildDevice->ensureReachable(buildDirectory)) {
        return {Task(
            Task::Error,
            Tr::tr("The build device \"%1\" cannot reach the build directory.")
                    .arg(buildDevice->displayName())
                + " " + canMountHintFor(buildDirectory),
            {},
            0,
            ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)};
    }

    return {};
};

const Tasks BuildConfigurationFactory::reportIssues(Kit *kit, const FilePath &projectPath,
                                                    const FilePath &buildDir) const
{
    Tasks issues = defaultIssueReporter(kit, projectPath, buildDir);
    if (m_issueReporter)
        issues += m_issueReporter(kit, projectPath, buildDir);
    return issues;
}

QList<BuildInfo> BuildConfigurationFactory::buildListHelper(const Kit *kit,
                                                            const FilePath &projectPath,
                                                            bool forSetup) const
{
    QTC_ASSERT(m_buildGenerator, return {});
    auto buildDevice = BuildDeviceKitAspect::device(kit);
    if (!buildDevice)
        return {};

    FilePath buildRoot = buildDevice->rootPath();
    if (!buildDevice->ensureReachable(projectPath))
        return {};

    QList<BuildInfo> list = m_buildGenerator(kit, projectPath, forSetup);
    for (BuildInfo &info : list) {
        info.factory = this;
        info.kitId = kit->id();
    }
    return list;
}

const QList<BuildInfo> BuildConfigurationFactory::allAvailableBuilds(const Target *target) const
{
    const FilePath projectPath = target->project()->projectFilePath();
    return buildListHelper(target->kit(), projectPath, /* forSetup = */ false);
}

const QList<BuildInfo>
    BuildConfigurationFactory::allAvailableSetups(const Kit *kit, const FilePath &projectPath) const
{
    return buildListHelper(kit, projectPath, /* forSetup = */ true);
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
    const Utils::Id deviceType = RunDeviceTypeKitAspect::deviceTypeId(k);
    for (BuildConfigurationFactory *factory : std::as_const(g_buildConfigurationFactories)) {
        if (!factory->supportsTargetDeviceType(deviceType))
            continue;
        for (const QString &mimeType : std::as_const(factory->m_supportedProjectMimeTypeNames)) {
            if (Utils::mimeTypeForFile(projectPath).matchesName(mimeType))
                return factory;
        }
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
    setSupportedProjectMimeTypeNames({mimeTypeName});
}

void BuildConfigurationFactory::setSupportedProjectMimeTypeNames(const QStringList &mimeTypeNames)
{
    m_supportedProjectMimeTypeNames = mimeTypeNames;
}

void BuildConfigurationFactory::addSupportedTargetDeviceType(Utils::Id id)
{
    m_supportedTargetDeviceTypes.append(id);
}

bool BuildConfigurationFactory::canHandle(const Target *target) const
{
    if (m_supportedProjectType.isValid() && m_supportedProjectType != target->project()->type())
        return false;

    if (containsType(target->project()->projectIssues(target->kit()), Task::TaskType::Error))
        return false;

    if (!supportsTargetDeviceType(RunDeviceTypeKitAspect::deviceTypeId(target->kit())))
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

BuildConfiguration *activeBuildConfig(const Project *project)
{
    return project ? project->activeBuildConfiguration() : nullptr;
}

BuildConfiguration *activeBuildConfigForActiveProject()
{
    return activeBuildConfig(ProjectManager::startupProject());
}

BuildConfiguration *activeBuildConfigForCurrentProject()
{
    return activeBuildConfig(ProjectTree::currentProject());
}

} // namespace ProjectExplorer
