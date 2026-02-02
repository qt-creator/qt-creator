// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "runconfiguration.h"

#include "buildconfiguration.h"
#include "buildsystem.h"
#include "devicesupport/devicekitaspects.h"
#include "environmentaspect.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "projectnodes.h"
#include "projecttree.h"
#include "runconfigurationaspects.h"
#include "target.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/detailswidget.h>
#include <utils/layoutbuilder.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/variablechooser.h>

#include <QCheckBox>
#include <QHash>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QLoggingCategory>

using namespace Utils;
using namespace ProjectExplorer::Internal;

namespace ProjectExplorer {

const char BUILD_KEY[] = "ProjectExplorer.RunConfiguration.BuildKey";
const char CUSTOMIZED_KEY[] = "ProjectExplorer.RunConfiguration.Customized";
const char UNIQUE_ID_KEY[] = "ProjectExplorer.RunConfiguration.UniqueId";

///////////////////////////////////////////////////////////////////////
//
// GlobalOrProjectAspect
//
///////////////////////////////////////////////////////////////////////

GlobalOrProjectAspect::GlobalOrProjectAspect()
{
    addDataExtractor(this, &GlobalOrProjectAspect::currentSettings, &Data::currentSettings);
}

GlobalOrProjectAspect::~GlobalOrProjectAspect()
{
    delete m_projectSettings;
}

void GlobalOrProjectAspect::setProjectSettings(AspectContainer *settings)
{
    m_projectSettings = settings;
    m_projectSettings->setAutoApply(true);
}

void GlobalOrProjectAspect::setGlobalSettings(AspectContainer *settings, Id settingsPage)
{
    m_globalSettings = settings;
    m_globalSettings->setAutoApply(false);
    m_settingsPage = settingsPage;
}

void GlobalOrProjectAspect::setUsingGlobalSettings(bool value)
{
    if (m_useGlobalSettings != value) {
        m_useGlobalSettings = value;
        emit currentSettingsChanged();
    }
}

AspectContainer *GlobalOrProjectAspect::currentSettings() const
{
   return m_useGlobalSettings ? m_globalSettings : m_projectSettings;
}

void GlobalOrProjectAspect::fromMap(const Store &map)
{
    if (m_projectSettings)
        m_projectSettings->fromMap(map);
    m_useGlobalSettings = map.value(id().toKey() + ".UseGlobalSettings", true).toBool();
}

void GlobalOrProjectAspect::toMap(Store &map) const
{
    if (m_projectSettings)
        m_projectSettings->toMap(map);
    map.insert(id().toKey() + ".UseGlobalSettings", m_useGlobalSettings);
}

void GlobalOrProjectAspect::toActiveMap(Store &data) const
{
    if (m_useGlobalSettings)
        m_globalSettings->toMap(data);
    else if (m_projectSettings)
        m_projectSettings->toMap(data);
    // The debugger accesses the data directly, so this can actually happen.
    //else
    //    QTC_CHECK(false);
}

void GlobalOrProjectAspect::resetProjectToGlobalSettings()
{
    QTC_ASSERT(m_globalSettings, return);
    if (!m_projectSettings)
        return;
    Store map;
    m_globalSettings->toMap(map);
    m_projectSettings->fromMap(map);
    emit wasResetToGlobalValues();
}


class GlobalOrProjectAspectWidget : public QWidget
{
public:
    explicit GlobalOrProjectAspectWidget(GlobalOrProjectAspect *aspect)
    {
        using namespace Layouting;

        const auto useGlobalCheckBox = new QCheckBox;
        const auto useGlobalLabel = new QLabel;
        if (aspect->settingsPage().isValid()) {
            useGlobalLabel->setText(Tr::tr("Use <a href=\"dummy\">global settings</a>"));
            connect(useGlobalLabel, &QLabel::linkActivated, aspect, [aspect] {
                Core::ICore::showSettings(aspect->settingsPage());
            });
        } else {
            useGlobalLabel->setText(Tr::tr("Use global settings"));
        }

        auto restoreButton = new QPushButton(Tr::tr("Restore Global"));

        auto innerPane = new QWidget;
        auto configWidget = aspect->projectSettings()->layouter()().emerge();

        Column {
            Row { useGlobalCheckBox, useGlobalLabel, st },
            hr,
            Row { restoreButton, st },
            configWidget
        }.attachTo(innerPane);

        Column { innerPane }.attachTo(this);

        innerPane->layout()->setContentsMargins(0, 0, 0, 0);
        layout()->setContentsMargins(0, 0, 0, 0);

        auto chooseSettings = [=](bool isGlobal) {
            useGlobalCheckBox->setChecked(isGlobal);
            aspect->setUsingGlobalSettings(isGlobal);
            configWidget->setEnabled(!isGlobal);
            restoreButton->setEnabled(!isGlobal);
        };

        chooseSettings(aspect->isUsingGlobalSettings());

        connect(useGlobalCheckBox, &QCheckBox::clicked, this, chooseSettings);
        connect(restoreButton, &QPushButton::clicked,
                aspect, &ProjectExplorer::GlobalOrProjectAspect::resetProjectToGlobalSettings);
    }
};

class RunConfigAspectWidget : public QWidget
{
public:
    explicit RunConfigAspectWidget(GlobalOrProjectAspect *aspect)
    {
        using namespace Layouting;

        auto details = new DetailsWidget;
        details->setWidget(createGlobalOrProjectAspectWidget(aspect));

        Column { details }.attachTo(this);

        details->layout()->setContentsMargins(0, 0, 0, 0);
        layout()->setContentsMargins(0, 0, 0, 0);

        const auto updateDetails = [details, aspect] {
            details->setSummaryText(aspect->isUsingGlobalSettings()
                                    ? Tr::tr("Use Global Settings")
                                    : Tr::tr("Use Customized Settings"));
        };
        connect(aspect, &GlobalOrProjectAspect::currentSettingsChanged, this, updateDetails);
        updateDetails();
    }
};

QWidget *createGlobalOrProjectAspectWidget(GlobalOrProjectAspect *aspect)
{
    return new GlobalOrProjectAspectWidget(aspect);
}

QWidget *createRunConfigAspectWidget(GlobalOrProjectAspect *aspect)
{
    return new RunConfigAspectWidget(aspect);
}


/*!
    \class ProjectExplorer::RunConfiguration
    \inmodule QtCreator
    \inheaderfile projectexplorer/runconfiguration.h

    \brief The RunConfiguration class is the base class for a run configuration.

    A run configuration specifies how a target should be run, while a runner
    does the actual running.

    The target owns the RunConfigurations and a RunControl will need to copy all
    necessary data as the RunControl may continue to exist after the RunConfiguration
    has been destroyed.

    A RunConfiguration disables itself if the project has no parsing
    data available. The disabledReason() method can be used to get a user-facing string
    describing why the RunConfiguration considers itself unfit for use.
*/

static std::vector<RunConfiguration::AspectFactory> theAspectFactories;

static QList<RunConfigurationFactory *> g_runConfigurationFactories;

RunConfiguration::RunConfiguration(BuildConfiguration *bc, Id id)
    : ProjectConfiguration(bc->target(), id), m_buildConfiguration(bc)
{
    forceDisplayNameSerialization();
    connect(bc->buildSystem(), &BuildSystem::parsingFinished, this, &RunConfiguration::update);

    MacroExpander &expander = *macroExpander();
    expander.setDisplayName(Tr::tr("Run Settings"));
    expander.setAccumulating(true);
    expander.registerSubProvider({this, [bc] { return bc->macroExpander(); }});
    setupMacroExpander(expander, this, false);
    expander.registerVariable("RunConfig:WorkingDir",
                               Tr::tr("The run configuration's working directory."),
                               [this] {
        const auto wdAspect = aspect<WorkingDirectoryAspect>();
        return wdAspect ? wdAspect->workingDirectory().toUrlishString() : QString();
    });

    m_commandLineGetter = [this] {
        Launcher launcher;
        if (const auto launcherAspect = aspect<LauncherAspect>())
            launcher = launcherAspect->currentLauncher();
        FilePath executable;
        if (const auto executableAspect = aspect<ExecutableAspect>())
            executable = executableAspect->executable();
        QString arguments;
        if (const auto argumentsAspect = aspect<ArgumentsAspect>())
            arguments = argumentsAspect->arguments();

        if (launcher.command.isEmpty())
            return CommandLine{executable, arguments, CommandLine::Raw};

        CommandLine launcherCommand(launcher.command, launcher.arguments);
        launcherCommand.addArg(executable.toUrlishString());
        launcherCommand.addArgs(arguments, CommandLine::Raw);

        return launcherCommand;
    };

    connect(bc->buildSystem(), &BuildSystem::updated, this, &RunConfiguration::update);
    connect(bc->buildSystem(), &BuildSystem::deploymentDataChanged,
            this, &RunConfiguration::update);
    connect(bc, &BuildConfiguration::kitChanged, this, &RunConfiguration::update);

    connect(this, &AspectContainer::subAspectChanged, this, [this](BaseAspect *aspect) {
        if (!buildKey().isEmpty() && qobject_cast<ExecutableAspect *>(aspect))
            return;
        forEachLinkedRunConfig([aspect](RunConfiguration *rc) {
            // We assume that no run configuration has more than one aspect
            // of the same type.
            if (BaseAspect * const other = rc->aspect(aspect->id())) {
                Store map;
                aspect->toMap(map);
                other->fromMap(map);
            }
        });
    });
}

RunConfiguration::~RunConfiguration() = default;

QString RunConfiguration::disabledReason(Id) const
{
    return buildSystem()->disabledReason(m_buildKey);
}

bool RunConfiguration::isEnabled(Id) const
{
    return buildSystem()->hasParsingData();
}

QWidget *RunConfiguration::createConfigurationWidget()
{
    Layouting::Form form;
    form.setNoMargins();
    for (BaseAspect *aspect : std::as_const(*this)) {
        if (aspect->isVisible()) {
            form.addItem(aspect);
            form.flush();
        }
    }
    auto widget = form.emerge();

    VariableChooser::addSupportForChildWidgets(widget, {this, macroExpander()});

    auto detailsWidget = new DetailsWidget;
    detailsWidget->setState(DetailsWidget::NoSummary);
    detailsWidget->setWidget(widget);
    return detailsWidget;
}

bool RunConfiguration::isConfigured() const
{
    return !Utils::anyOf(checkForIssues(), &Task::isError);
}

bool RunConfiguration::isCustomized() const
{
    if (m_customized)
        return true;
    Store state;
    toMapSimple(state);

    // TODO: Why do we save this at all? It's a computed value.
    state.remove("RunConfiguration.WorkingDirectory.default");

    return state != m_pristineState;
}

bool RunConfiguration::hasCreator() const
{
    for (RunConfigurationFactory *factory : std::as_const(g_runConfigurationFactories)) {
        if (factory->runConfigurationId() == id()) {
            if (factory->supportsBuildKey(buildConfiguration(), buildKey()))
                return true;
        }
    }
    return false;
}

void RunConfiguration::setPristineState()
{
    if (!m_customized) {
        m_pristineState.clear();
        toMapSimple(m_pristineState);
        m_pristineState.remove("RunConfiguration.WorkingDirectory.default");
    }
}

void RunConfiguration::addAspectFactory(const AspectFactory &aspectFactory)
{
    theAspectFactories.push_back(aspectFactory);
}

QMap<Id, Store> RunConfiguration::settingsData() const
{
    QMap<Id, Store> data;
    for (BaseAspect *aspect : *this)
        aspect->toActiveMap(data[aspect->id()]);
    return data;
}

AspectContainerData RunConfiguration::aspectData() const
{
    AspectContainerData data;
    for (BaseAspect *aspect : *this)
        data.append(aspect->extractData());
    return data;
}

BuildSystem *RunConfiguration::buildSystem() const
{
    return m_buildConfiguration->buildSystem();
}

bool RunConfiguration::equals(const RunConfiguration *other) const
{
    Store map;
    toMapSimple(map);
    Store otherMap;
    other->toMapSimple(otherMap);
    return map == otherMap;
}

void RunConfiguration::setupMacroExpander(
    MacroExpander &exp, const RunConfiguration *rc, bool documentationOnly)
{
    exp.registerPrefix(
        "RunConfig:Env",
        "USER",
        Tr::tr("Variables in the run environment."),
        [rc](const QString &var) {
            if (!rc)
                return QString();
            const auto envAspect = rc->aspect<EnvironmentAspect>();
            return envAspect ? envAspect->environment().expandedValueForKey(var) : QString();
        },
        true,
        !documentationOnly);

    exp.registerVariable("RunConfig:Name", Tr::tr("The run configuration's name."), [rc] {
            return rc ? rc->displayName() : QString();
        }, true, !documentationOnly);

    exp.registerFileVariables(
        "RunConfig:Executable", Tr::tr("The run configuration's executable."), [rc] {
            return rc ? rc->commandLine().executable() : FilePath();
        }, true, !documentationOnly);
}

void RunConfiguration::setExecutionType(Id executionType)
{
    m_executionType = executionType;
}

Id RunConfiguration::executionType() const
{
    return m_executionType;
}

void RunConfiguration::setUpdater(const Updater &updater)
{
    m_updater = updater;
}

Task RunConfiguration::createConfigurationIssue(const QString &description) const
{
    return BuildSystemTask(Task::Error, description);
}

void RunConfiguration::toMap(Store &map) const
{
    toMapSimple(map);
    map.insert(CUSTOMIZED_KEY, isCustomized());
}

void RunConfiguration::toMapSimple(Store &map) const
{
    ProjectConfiguration::toMap(map);

    if (m_usesEmptyBuildKeys) {
        QTC_CHECK(m_buildKey.isEmpty());
    } else {
        QTC_CHECK(!m_buildKey.isEmpty());
    }

    map.insert(BUILD_KEY, m_buildKey);
    map.insert(UNIQUE_ID_KEY, m_uniqueId);
}

void RunConfiguration::setCommandLineGetter(const CommandLineGetter &cmdGetter)
{
    m_commandLineGetter = cmdGetter;
}

CommandLine RunConfiguration::commandLine() const
{
    return m_commandLineGetter();
}

bool RunConfiguration::isPrintEnvironmentEnabled() const
{
    if (const auto envAspect = aspect<EnvironmentAspect>())
        return envAspect->isPrintOnRunEnabled();
    return false;
}

void RunConfiguration::setRunnableModifier(const RunnableModifier &runnableModifier)
{
    m_runnableModifier = runnableModifier;
}

void RunConfiguration::update()
{
    if (m_updater)
        m_updater();

    emit enabledChanged();

    if (activeRunConfigForActiveProject() == this)
        ProjectExplorerPlugin::updateRunActions();
}

RunConfiguration *RunConfiguration::clone(BuildConfiguration *bc)
{
    Store map;
    toMap(map);
    return RunConfigurationFactory::restore(bc, map);
}

void RunConfiguration::cloneFromOther(const RunConfiguration *rc)
{
    Store ownData;
    toMap(ownData);

    Store copyData;
    rc->toMap(copyData);
    copyData.insert(Constants::CONFIGURATION_ID_KEY, ownData.value(Constants::CONFIGURATION_ID_KEY));
    copyData.insert(Constants::DISPLAY_NAME_KEY, ownData.value(Constants::DISPLAY_NAME_KEY));
    copyData.insert(BUILD_KEY, ownData.value(BUILD_KEY));
    fromMap(copyData);
}

const QList<BuildConfiguration *> RunConfiguration::syncableBuildConfigurations() const
{
    QList<BuildConfiguration *> buildConfigs;
    switch (ProjectExplorerSettings::get(this).syncRunConfigurations.value()) {
    case SyncRunConfigs::Off:
        break;
    case SyncRunConfigs::SameKit:
        buildConfigs = target()->buildConfigurations();
        break;
    case SyncRunConfigs::All:
        buildConfigs = project()->allBuildConfigurations();
        break;
    }
    buildConfigs.removeOne(buildConfiguration());
    return buildConfigs;
}

void RunConfiguration::forEachLinkedRunConfig(const std::function<void(RunConfiguration *)> &handler)
{
    for (BuildConfiguration * const bc : syncableBuildConfigurations()) {
        for (RunConfiguration * const rc : bc->runConfigurations()) {
            if (rc->uniqueId() == uniqueId()) {
                handler(rc);
                break;
            }
        }
    }
}

void RunConfiguration::makeActive()
{
    buildConfiguration()->setActiveRunConfiguration(this);
    forEachLinkedRunConfig(
        [](RunConfiguration *rc) { rc->buildConfiguration()->setActiveRunConfiguration(rc); });
}

BuildTargetInfo RunConfiguration::buildTargetInfo() const
{
    BuildSystem *bs = buildSystem();
    QTC_ASSERT(bs, return {});
    return bs->buildTarget(m_buildKey);
}

ProjectNode *RunConfiguration::productNode() const
{
    return project()->rootProjectNode()->findProjectNode([this](const ProjectNode *candidate) {
        return candidate->buildKey() == buildKey();
    });
}

void RunConfiguration::fromMap(const Store &map)
{
    ProjectConfiguration::fromMap(map);
    if (hasError())
        return;

    m_customized = m_customized || map.value(CUSTOMIZED_KEY, false).toBool();
    m_buildKey = map.value(BUILD_KEY).toString();
    m_uniqueId = map.value(UNIQUE_ID_KEY).toString();

    if (m_usesEmptyBuildKeys) {
        QTC_CHECK(m_buildKey.isEmpty());
    } else {
        QTC_CHECK(!m_buildKey.isEmpty());
    }
}

/*!
    \class ProjectExplorer::GlobalOrProjectAspect

    \brief The GlobalOrProjectAspect class provides an additional
    configuration aspect.

    Aspects are a mechanism to add RunControl-specific options to a run
    configuration without subclassing the run configuration for every addition.
    This prevents a combinatorial explosion of subclasses and eliminates
    the need to add all options to the base class.
*/

/*!
    \internal

    \class ProjectExplorer::Runnable

    \brief The ProjectExplorer::Runnable class wraps information needed
    to execute a process on a target device.

    A target specific \l RunConfiguration implementation can specify
    what information it considers necessary to execute a process
    on the target. Target specific) \n RunWorker implementation
    can use that information either unmodified or tweak it or ignore
    it when setting up a RunControl.

    From Qt Creator's core perspective a Runnable object is opaque.
*/

/*!
    \internal

    \brief Returns a \l Runnable described by this RunConfiguration.
*/

ProcessRunData RunConfiguration::runnable() const
{
    ProcessRunData r;
    r.command = commandLine();
    if (auto workingDirectoryAspect = aspect<WorkingDirectoryAspect>())
        r.workingDirectory = r.command.executable().withNewMappedPath(workingDirectoryAspect->workingDirectory());
    if (auto environmentAspect = aspect<EnvironmentAspect>())
    {
        r.environment = environmentAspect->expandedEnvironment(*macroExpander());
        auto enableCategoriesFilterAspect = aspect<EnableCategoriesFilterAspect>();
        if (enableCategoriesFilterAspect && enableCategoriesFilterAspect->value()) {
            r.environment.set("QT_LOGGING_RULES", "_logging_categories=true;*.debug=true");
            r.environment.set("QT_MESSAGE_PATTERN",
                              "%{category}:[%{if-debug}D%{endif}%{if-info}I%{endif}"
                              "%{if-warning}W%{endif}%{if-critical}C%{endif}"
                              "%{if-fatal}F%{endif}] %{message}");
        }
    }
    if (m_runnableModifier)
        m_runnableModifier(r);

    // TODO: Do expansion in commandLine()?
    if (!r.command.isEmpty()) {
        const FilePath expanded = macroExpander()->expand(r.command.executable());
        r.command.setExecutable(expanded);
    }

    return r;
}

QVariantHash RunConfiguration::extraData() const
{
    QVariantHash data;
    if (auto forwardingAspect = aspect<X11ForwardingAspect>())
        data.insert("Ssh.X11ForwardToDisplay", forwardingAspect->display());
    return data;
}

/*!
    \class ProjectExplorer::RunConfigurationFactory
    \inmodule QtCreator
    \inheaderfile projectexplorer/runconfiguration.h

    \brief The RunConfigurationFactory class is used to create and persist
    run configurations.

    The run configuration factory is used for restoring run configurations from
    settings and for creating new run configurations in the \gui {Run Settings}
    dialog.

    A RunConfigurationFactory instance is responsible for handling one type of
    run configurations. This can be restricted to certain project and device
    types.

    RunConfigurationFactory instances register themselves into a global list on
    construction and deregister on destruction. It is recommended to make them
    a plain data member of a structure that is allocated in your plugin's
    ExtensionSystem::IPlugin::initialize() method.
*/

/*!
    Constructs a RunConfigurationFactory instance and registers it into a global
    list.

    Derived classes should set suitably properties to specify the type of
    run configurations they can handle.
*/

RunConfigurationFactory::RunConfigurationFactory()
{
    g_runConfigurationFactories.append(this);
}

/*!
    De-registers the instance from the global list of factories and destructs it.
*/

RunConfigurationFactory::~RunConfigurationFactory()
{
    g_runConfigurationFactories.removeOne(this);
}

QString RunConfigurationFactory::decoratedTargetName(const QString &targetName, Kit *kit)
{
    QString displayName = targetName;
    Id devType = RunDeviceTypeKitAspect::deviceTypeId(kit);

    if (devType == Constants::DESKTOP_DEVICE_TYPE)
        return displayName;

    //: Shown in Run configuration if no executable is given
    if (displayName.isEmpty())
        return Tr::tr("Run on %{Device:Name}");

    //: Shown in Run configuration, Add menu: "name of runnable (on device name)"
    return Tr::tr("%1 (on %{Device:Name})").arg(displayName);
}

QList<RunConfigurationCreationInfo>
RunConfigurationFactory::availableCreators(BuildConfiguration *bc) const
{
    const auto buildTargets = bc->buildSystem()->applicationTargets();
    const bool hasAnyQtcRunnable = Utils::anyOf(buildTargets,
                                            Utils::equal(&BuildTargetInfo::isQtcRunnable, true));
    return Utils::transform(buildTargets, [&](const BuildTargetInfo &ti) {
        QString displayName = ti.displayName;
        if (displayName.isEmpty())
            displayName = decoratedTargetName(ti.buildKey, bc->kit());
        else if (m_decorateDisplayNames)
            displayName = decoratedTargetName(displayName, bc->kit());
        RunConfigurationCreationInfo rci;
        rci.factory = this;
        rci.buildKey = ti.buildKey;
        rci.projectFilePath = ti.projectFilePath;
        rci.displayName = displayName;
        rci.displayNameUniquifier = ti.displayNameUniquifier;
        rci.creationMode = ti.isQtcRunnable || !hasAnyQtcRunnable
                ? RunConfigurationCreationInfo::AlwaysCreate
                : RunConfigurationCreationInfo::ManualCreationOnly;
        rci.useTerminal = ti.usesTerminal;
        rci.buildKey = ti.buildKey;
        return rci;
    });
}

bool RunConfigurationFactory::supportsBuildKey(BuildConfiguration *bc, const QString &key) const
{
    if (!canHandle(bc->target()))
        return false;
    const QList<BuildTargetInfo> buildTargets = bc->buildSystem()->applicationTargets();
    return anyOf(buildTargets, [&key](const BuildTargetInfo &info) { return info.buildKey == key; });
}

/*!
    Adds a device type for which this RunConfigurationFactory
    can create RunConfigurations.

    If this function is never called for a RunConfigurationFactory,
    the factory will create RunConfiguration objects for all device types.

    This function should be used in the constructor of derived classes.

    \sa addSupportedProjectType()
*/

void RunConfigurationFactory::addSupportedTargetDeviceType(Id id)
{
    m_supportedTargetDeviceTypes.append(id);
}

void RunConfigurationFactory::setDecorateDisplayNames(bool on)
{
    m_decorateDisplayNames = on;
}

/*!
    Adds a project type for which this RunConfigurationFactory
    can create RunConfigurations.

    If this function is never called for a RunConfigurationFactory,
    the factory will create RunConfigurations for all project types.

    This function should be used in the constructor of derived classes.

    \sa addSupportedTargetDeviceType()
*/

void RunConfigurationFactory::addSupportedProjectType(Id id)
{
    m_supportedProjectTypes.append(id);
}

void RunConfigurationFactory::setExecutionTypeId(Id executionType)
{
    m_executionType = executionType;
}

Id RunConfigurationFactory::executionTypeId() const
{
    return m_executionType;
}

bool RunConfigurationFactory::canHandle(Target *target) const
{
    const Project *project = target->project();
    Kit *kit = target->kit();

    if (containsType(target->project()->projectIssues(kit), Task::TaskType::Error))
        return false;

    const bool supportsAnyExecutionType = !m_executionType.isValid();
    if (!supportsAnyExecutionType && RunDeviceTypeKitAspect::executionTypeId(kit) != m_executionType)
        return false;

    if (!m_supportedProjectTypes.isEmpty()) {
        if (!m_supportedProjectTypes.contains(project->type()))
            return false;
    }

    if (!m_supportedTargetDeviceTypes.isEmpty()) {
        if (!m_supportedTargetDeviceTypes.contains(RunDeviceTypeKitAspect::deviceTypeId(kit)))
            return false;
    }

    return true;
}

RunConfiguration *RunConfigurationFactory::create(BuildConfiguration *bc) const
{
    QTC_ASSERT(m_creator, return nullptr);
    RunConfiguration *rc = m_creator(bc);
    QTC_ASSERT(rc, return nullptr);

    // Add the universal aspects.
    for (const RunConfiguration::AspectFactory &factory : theAspectFactories)
        rc->registerAspect(factory(bc), true);

    rc->setExecutionType(executionTypeId());

    return rc;
}

RunConfiguration *RunConfigurationCreationInfo::create(BuildConfiguration *bc) const
{
    QTC_ASSERT(factory->canHandle(bc->target()), return nullptr);

    RunConfiguration *rc = factory->create(bc);
    if (!rc)
        return nullptr;

    rc->m_buildKey = buildKey;
    rc->update();
    rc->setDisplayName(displayName);

    // THIS MUST BE CALLED LAST!
    rc->setPristineState();

    return rc;
}

RunConfiguration *RunConfigurationFactory::restore(BuildConfiguration *bc, const Store &map)
{
    for (RunConfigurationFactory *factory : std::as_const(g_runConfigurationFactories)) {
        if (factory->canHandle(bc->target())) {
            const Id id = idFromMap(map);
            if (id.name().startsWith(factory->m_runConfigurationId.name())) {
                RunConfiguration *rc = factory->create(bc);
                rc->fromMap(map);
                if (!rc->hasError()) {
                    rc->update();
                    rc->setPristineState();
                    return rc;
                }
                delete rc;
                return nullptr;
            }
        }
    }
    return nullptr;
}

const QList<RunConfigurationCreationInfo> RunConfigurationFactory::creatorsForBuildConfig(
    BuildConfiguration *bc)
{
    QList<RunConfigurationCreationInfo> items;
    for (RunConfigurationFactory *factory : std::as_const(g_runConfigurationFactories)) {
        if (factory->canHandle(bc->target()))
            items.append(factory->availableCreators(bc));
    }
    QHash<QString, QList<RunConfigurationCreationInfo *>> itemsPerDisplayName;
    for (RunConfigurationCreationInfo &item : items)
        itemsPerDisplayName[item.displayName] << &item;
    for (auto it = itemsPerDisplayName.cbegin(); it != itemsPerDisplayName.cend(); ++it) {
        if (it.value().size() == 1)
            continue;
        for (RunConfigurationCreationInfo * const rci : it.value())
            rci->displayName += rci->displayNameUniquifier;
    }
    return items;
}

FixedRunConfigurationFactory::FixedRunConfigurationFactory(const QString &displayName,
                                                           bool addDeviceName) :
    m_fixedBuildTarget(displayName),
    m_decorateTargetName(addDeviceName)
{ }

QList<RunConfigurationCreationInfo>
FixedRunConfigurationFactory::availableCreators(BuildConfiguration *bc) const
{
    QString displayName = m_decorateTargetName ? decoratedTargetName(m_fixedBuildTarget, bc->kit())
                                               : m_fixedBuildTarget;
    RunConfigurationCreationInfo rci;
    rci.factory = this;
    rci.displayName = displayName;
    return {rci};
}

bool FixedRunConfigurationFactory::supportsBuildKey(BuildConfiguration *bc, const QString &key) const
{
    Q_UNUSED(bc)
    Q_UNUSED(key)
    return true;
}

RunConfiguration *activeRunConfig(const Project *project)
{
    return project ? project->activeRunConfiguration() : nullptr;
}

RunConfiguration *activeRunConfigForActiveProject()
{
    return activeRunConfig(ProjectManager::startupProject());
}

RunConfiguration *activeRunConfigForCurrentProject()
{
    return activeRunConfig(ProjectTree::currentProject());
}

QString ProjectExplorer::RunConfiguration::uniqueId() const
{
    if (!m_uniqueId.isEmpty())
        return m_uniqueId;
    return buildKey();
}

void RunConfiguration::setUniqueId(const QString &id)
{
    m_uniqueId = id;
}

QString RunConfiguration::expandedDisplayName() const
{
    QString displayName = ProjectConfiguration::expandedDisplayName();
    if (hasCreator())
        return displayName;

    return joinStrings({displayName, QString("[%1]").arg(Tr::tr("unavailable"))}, ' ');
}

} // namespace ProjectExplorer
