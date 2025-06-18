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

#include <QComboBox>
#include <QHash>
#include <QLayout>
#include <QPushButton>
#include <QLoggingCategory>

using namespace Utils;
using namespace ProjectExplorer::Internal;

namespace ProjectExplorer {

const char BUILD_KEY[] = "ProjectExplorer.RunConfiguration.BuildKey";
const char CUSTOMIZED_KEY[] = "ProjectExplorer.RunConfiguration.Customized";


///////////////////////////////////////////////////////////////////////
//
// IRunConfigurationAspect
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

void GlobalOrProjectAspect::setGlobalSettings(AspectContainer *settings)
{
    m_globalSettings = settings;
    m_globalSettings->setAutoApply(false);
}

void GlobalOrProjectAspect::setUsingGlobalSettings(bool value)
{
    m_useGlobalSettings = value;
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
    Store map;
    m_globalSettings->toMap(map);
    if (m_projectSettings)
        m_projectSettings->fromMap(map);
}


class RunConfigAspectWidget : public QWidget
{
public:
    explicit RunConfigAspectWidget(GlobalOrProjectAspect *aspect)
    {
        using namespace Layouting;

        auto settingsCombo = new QComboBox;
        settingsCombo->addItem(Tr::tr("Global"));
        settingsCombo->addItem(Tr::tr("Custom"));

        auto restoreButton = new QPushButton(Tr::tr("Restore Global"));

        auto innerPane = new QWidget;
        auto configWidget = aspect->projectSettings()->layouter()().emerge();

        auto details = new DetailsWidget;
        details->setWidget(innerPane);

        Column {
            Row { settingsCombo, restoreButton, st },
            configWidget
        }.attachTo(innerPane);

        Column { details }.attachTo(this);

        details->layout()->setContentsMargins(0, 0, 0, 0);
        innerPane->layout()->setContentsMargins(0, 0, 0, 0);
        layout()->setContentsMargins(0, 0, 0, 0);

        auto chooseSettings = [=](int setting) {
            const bool isCustom = (setting == 1);

            settingsCombo->setCurrentIndex(setting);
            aspect->setUsingGlobalSettings(!isCustom);
            configWidget->setEnabled(isCustom);
            restoreButton->setEnabled(isCustom);
            details->setSummaryText(isCustom
                                    ? Tr::tr("Use Customized Settings")
                                    : Tr::tr("Use Global Settings"));
        };

        chooseSettings(aspect->isUsingGlobalSettings() ? 0 : 1);

        connect(settingsCombo, &QComboBox::activated, this, chooseSettings);
        connect(restoreButton, &QPushButton::clicked,
                aspect, &ProjectExplorer::GlobalOrProjectAspect::resetProjectToGlobalSettings);
    }
};

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

RunConfiguration::RunConfiguration(BuildConfiguration *bc, Utils::Id id)
    : ProjectConfiguration(bc->target(), id), m_buildConfiguration(bc)
{
    forceDisplayNameSerialization();
    connect(bc->buildSystem(), &BuildSystem::parsingFinished, this, &RunConfiguration::update);

    MacroExpander &expander = *macroExpander();
    expander.setDisplayName(Tr::tr("Run Settings"));
    expander.setAccumulating(true);
    expander.registerSubProvider([bc] { return bc->macroExpander(); });
    expander.registerPrefix("RunConfig:Env", Tr::tr("Variables in the run environment."),
                             [this](const QString &var) {
        const auto envAspect = aspect<EnvironmentAspect>();
        return envAspect ? envAspect->environment().expandedValueForKey(var) : QString();
    });
    expander.registerVariable("RunConfig:WorkingDir",
                               Tr::tr("The run configuration's working directory."),
                               [this] {
        const auto wdAspect = aspect<WorkingDirectoryAspect>();
        return wdAspect ? wdAspect->workingDirectory().toUrlishString() : QString();
    });
    expander.registerVariable("RunConfig:Name", Tr::tr("The run configuration's name."),
            [this] { return displayName(); });
    expander.registerFileVariables("RunConfig:Executable",
                                     Tr::tr("The run configuration's executable."),
                                     [this] { return commandLine().executable(); });


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
}

RunConfiguration::~RunConfiguration() = default;

QString RunConfiguration::disabledReason(Utils::Id) const
{
    return buildSystem()->disabledReason(m_buildKey);
}

bool RunConfiguration::isEnabled(Utils::Id) const
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

    VariableChooser::addSupportForChildWidgets(widget, macroExpander());

    auto detailsWidget = new Utils::DetailsWidget;
    detailsWidget->setState(DetailsWidget::NoSummary);
    detailsWidget->setWidget(widget);
    return detailsWidget;
}

bool RunConfiguration::isConfigured() const
{
    return !Utils::anyOf(checkForIssues(), [](const Task &t) { return t.type == Task::Error; });
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

    if (m_usesEmptyBuildKeys) {
        QTC_CHECK(m_buildKey.isEmpty());
    } else {
        QTC_CHECK(!m_buildKey.isEmpty());
    }
}

/*!
    \class ProjectExplorer::IRunConfigurationAspect

    \brief The IRunConfigurationAspect class provides an additional
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
        r.environment = environmentAspect->expandedEnvironment(*macroExpander());
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
    Utils::Id devType = RunDeviceTypeKitAspect::deviceTypeId(kit);
    if (devType != Constants::DESKTOP_DEVICE_TYPE) {
        if (IDevice::ConstPtr dev = RunDeviceKitAspect::device(kit)) {
            if (displayName.isEmpty()) {
                //: Shown in Run configuration if no executable is given, %1 is device name
                displayName = Tr::tr("Run on %{Device:Name}");
            } else {
                //: Shown in Run configuration, Add menu: "name of runnable (on device name)"
                displayName = Tr::tr("%1 (on %{Device:Name})").arg(displayName);
            }
        }
    }
    return displayName;
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

void RunConfigurationFactory::addSupportedTargetDeviceType(Utils::Id id)
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

void RunConfigurationFactory::addSupportedProjectType(Utils::Id id)
{
    m_supportedProjectTypes.append(id);
}

bool RunConfigurationFactory::canHandle(Target *target) const
{
    const Project *project = target->project();
    Kit *kit = target->kit();

    if (containsType(target->project()->projectIssues(kit), Task::TaskType::Error))
        return false;

    if (!m_supportedProjectTypes.isEmpty())
        if (!m_supportedProjectTypes.contains(project->id()))
            return false;

    if (!m_supportedTargetDeviceTypes.isEmpty())
        if (!m_supportedTargetDeviceTypes.contains(
                    RunDeviceTypeKitAspect::deviceTypeId(kit)))
            return false;

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
    rc->setPristineState();

    return rc;
}

RunConfiguration *RunConfigurationFactory::restore(BuildConfiguration *bc, const Store &map)
{
    for (RunConfigurationFactory *factory : std::as_const(g_runConfigurationFactories)) {
        if (factory->canHandle(bc->target())) {
            const Utils::Id id = idFromMap(map);
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

} // namespace ProjectExplorer
