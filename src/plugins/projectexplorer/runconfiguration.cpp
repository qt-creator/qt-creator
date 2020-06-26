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

#include "runconfiguration.h"

#include "abi.h"
#include "buildconfiguration.h"
#include "buildsystem.h"
#include "environmentaspect.h"
#include "kitinformation.h"
#include "kitinformation.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectnodes.h"
#include "runconfigurationaspects.h"
#include "runcontrol.h"
#include "session.h"
#include "target.h"
#include "toolchain.h"

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/detailswidget.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/variablechooser.h>

#include <QDir>
#include <QFormLayout>
#include <QHash>
#include <QPushButton>
#include <QTimer>
#include <QLoggingCategory>
#include <QSettings>

using namespace Utils;
using namespace ProjectExplorer::Internal;

namespace ProjectExplorer {

const char BUILD_KEY[] = "ProjectExplorer.RunConfiguration.BuildKey";

///////////////////////////////////////////////////////////////////////
//
// ISettingsAspect
//
///////////////////////////////////////////////////////////////////////

ISettingsAspect::ISettingsAspect() = default;

QWidget *ISettingsAspect::createConfigWidget() const
{
    QTC_ASSERT(m_configWidgetCreator, return nullptr);
    return m_configWidgetCreator();
}

void ISettingsAspect::setConfigWidgetCreator(const ConfigWidgetCreator &configWidgetCreator)
{
    m_configWidgetCreator = configWidgetCreator;
}


///////////////////////////////////////////////////////////////////////
//
// IRunConfigurationAspect
//
///////////////////////////////////////////////////////////////////////

GlobalOrProjectAspect::GlobalOrProjectAspect() = default;

GlobalOrProjectAspect::~GlobalOrProjectAspect()
{
    delete m_projectSettings;
}

void GlobalOrProjectAspect::setProjectSettings(ISettingsAspect *settings)
{
    m_projectSettings = settings;
}

void GlobalOrProjectAspect::setGlobalSettings(ISettingsAspect *settings)
{
    m_globalSettings = settings;
}

void GlobalOrProjectAspect::setUsingGlobalSettings(bool value)
{
    m_useGlobalSettings = value;
}

ISettingsAspect *GlobalOrProjectAspect::currentSettings() const
{
   return m_useGlobalSettings ? m_globalSettings : m_projectSettings;
}

void GlobalOrProjectAspect::fromMap(const QVariantMap &map)
{
    if (m_projectSettings)
        m_projectSettings->fromMap(map);
    m_useGlobalSettings = map.value(m_id.toString() + QLatin1String(".UseGlobalSettings"), true).toBool();
}

void GlobalOrProjectAspect::toMap(QVariantMap &map) const
{
    if (m_projectSettings)
        m_projectSettings->toMap(map);
    map.insert(m_id.toString() + QLatin1String(".UseGlobalSettings"), m_useGlobalSettings);
}

void GlobalOrProjectAspect::resetProjectToGlobalSettings()
{
    QTC_ASSERT(m_globalSettings, return);
    QVariantMap map;
    m_globalSettings->toMap(map);
    if (m_projectSettings)
        m_projectSettings->fromMap(map);
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

RunConfiguration::RunConfiguration(Target *target, Utils::Id id)
    : ProjectConfiguration(target, id)
{
    QTC_CHECK(target && target == this->target());
    connect(target, &Target::parsingFinished, this, &RunConfiguration::update);

    m_expander.setDisplayName(tr("Run Settings"));
    m_expander.setAccumulating(true);
    m_expander.registerSubProvider([target] {
        BuildConfiguration *bc = target->activeBuildConfiguration();
        return bc ? bc->macroExpander() : target->macroExpander();
    });
    m_expander.registerPrefix("CurrentRun:Env", tr("Variables in the current run environment"),
                             [this](const QString &var) {
        const auto envAspect = aspect<EnvironmentAspect>();
        return envAspect ? envAspect->environment().expandedValueForKey(var) : QString();
    });

    m_expander.registerVariable(Constants::VAR_CURRENTRUN_WORKINGDIR,
                               tr("The currently active run configuration's working directory"),
                               [this] {
        const auto wdAspect = aspect<WorkingDirectoryAspect>();
        return wdAspect ? wdAspect->workingDirectory(&m_expander).toString() : QString();
    });

    m_expander.registerVariable(Constants::VAR_CURRENTRUN_NAME,
            QCoreApplication::translate("ProjectExplorer", "The currently active run configuration's name."),
            [this] { return displayName(); }, false);

    m_commandLineGetter = [this] {
        FilePath executable;
        if (const auto executableAspect = aspect<ExecutableAspect>())
            executable = executableAspect->executable();
        QString arguments;
        if (const auto argumentsAspect = aspect<ArgumentsAspect>())
            arguments = argumentsAspect->arguments(macroExpander());
        return CommandLine{executable, arguments, CommandLine::Raw};
    };
}

RunConfiguration::~RunConfiguration() = default;

QString RunConfiguration::disabledReason() const
{
    BuildSystem *bs = activeBuildSystem();
    return bs ? bs->disabledReason(m_buildKey) : tr("No build system active");
}

bool RunConfiguration::isEnabled() const
{
    BuildSystem *bs = activeBuildSystem();
    return bs && bs->hasParsingData();
}

QWidget *RunConfiguration::createConfigurationWidget()
{
    auto widget = new QWidget;
    {
        LayoutBuilder builder(widget);
        for (ProjectConfigurationAspect *aspect : m_aspects) {
            if (aspect->isVisible())
                aspect->addToLayout(builder.startNewRow());
        }
    }

    Core::VariableChooser::addSupportForChildWidgets(widget, &m_expander);

    auto detailsWidget = new Utils::DetailsWidget;
    detailsWidget->setState(DetailsWidget::NoSummary);
    detailsWidget->setWidget(widget);
    return detailsWidget;
}

void RunConfiguration::addAspectFactory(const AspectFactory &aspectFactory)
{
    theAspectFactories.push_back(aspectFactory);
}

QMap<Utils::Id, QVariantMap> RunConfiguration::aspectData() const
{
    QMap<Utils::Id, QVariantMap> data;
    for (ProjectConfigurationAspect *aspect : m_aspects)
        aspect->toMap(data[aspect->id()]);
    return data;
}

BuildSystem *RunConfiguration::activeBuildSystem() const
{
    return target()->buildSystem();
}

void RunConfiguration::setUpdater(const Updater &updater)
{
    m_updater = updater;
}

Task RunConfiguration::createConfigurationIssue(const QString &description) const
{
    return BuildSystemTask(Task::Error, description);
}

QVariantMap RunConfiguration::toMap() const
{
    QVariantMap map = ProjectConfiguration::toMap();

    map.insert(BUILD_KEY, m_buildKey);

    // FIXME: Remove this id mangling, e.g. by using a separate entry for the build key.
    if (!m_buildKey.isEmpty()) {
        const Utils::Id mangled = id().withSuffix(m_buildKey);
        map.insert(settingsIdKey(), mangled.toSetting());
    }

    return map;
}

void RunConfiguration::setCommandLineGetter(const CommandLineGetter &cmdGetter)
{
    m_commandLineGetter = cmdGetter;
}

CommandLine RunConfiguration::commandLine() const
{
    return m_commandLineGetter();
}

void RunConfiguration::update()
{
    if (m_updater)
        m_updater();

    emit enabledChanged();

    const bool isActive = target()->isActive() && target()->activeRunConfiguration() == this;

    if (isActive && project() == SessionManager::startupProject())
        ProjectExplorerPlugin::updateRunActions();
}

BuildTargetInfo RunConfiguration::buildTargetInfo() const
{
    BuildSystem *bs = target()->buildSystem();
    QTC_ASSERT(bs, return {});
    return bs->buildTarget(m_buildKey);
}

ProjectNode *RunConfiguration::productNode() const
{
    return project()->rootProjectNode()->findProjectNode([this](const ProjectNode *candidate) {
        return candidate->buildKey() == buildKey();
    });
}

bool RunConfiguration::fromMap(const QVariantMap &map)
{
    if (!ProjectConfiguration::fromMap(map))
        return false;

    m_buildKey = map.value(BUILD_KEY).toString();

    if (m_buildKey.isEmpty()) {
        const Utils::Id mangledId = Utils::Id::fromSetting(map.value(settingsIdKey()));
        m_buildKey = mangledId.suffixAfter(id());

        // Hack for cmake projects 4.10 -> 4.11.
        const QString magicSeparator = "///::///";
        const int magicIndex = m_buildKey.indexOf(magicSeparator);
        if (magicIndex != -1)
            m_buildKey = m_buildKey.mid(magicIndex + magicSeparator.length());
    }

    return  true;
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

Runnable RunConfiguration::runnable() const
{
    Runnable r;
    r.setCommandLine(commandLine());
    if (auto workingDirectoryAspect = aspect<WorkingDirectoryAspect>())
        r.workingDirectory = workingDirectoryAspect->workingDirectory(macroExpander()).toString();
    if (auto environmentAspect = aspect<EnvironmentAspect>())
        r.environment = environmentAspect->environment();
    return r;
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

static QList<RunConfigurationFactory *> g_runConfigurationFactories;

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

QString RunConfigurationFactory::decoratedTargetName(const QString &targetName, Target *target)
{
    QString displayName;
    if (!targetName.isEmpty())
        displayName = QFileInfo(targetName).completeBaseName();
    Utils::Id devType = DeviceTypeKitAspect::deviceTypeId(target->kit());
    if (devType != Constants::DESKTOP_DEVICE_TYPE) {
        if (IDevice::ConstPtr dev = DeviceKitAspect::device(target->kit())) {
            if (displayName.isEmpty()) {
                //: Shown in Run configuration if no executable is given, %1 is device name
                displayName = RunConfiguration::tr("Run on %1").arg(dev->displayName());
            } else {
                //: Shown in Run configuration, Add menu: "name of runnable (on device name)"
                displayName = RunConfiguration::tr("%1 (on %2)").arg(displayName, dev->displayName());
            }
        }
    }
    return displayName;
}

QList<RunConfigurationCreationInfo>
RunConfigurationFactory::availableCreators(Target *target) const
{
    const QList<BuildTargetInfo> buildTargets = target->buildSystem()->applicationTargets();
    const bool hasAnyQtcRunnable = Utils::anyOf(buildTargets,
                                            Utils::equal(&BuildTargetInfo::isQtcRunnable, true));
    return Utils::transform(buildTargets, [&](const BuildTargetInfo &ti) {
        QString displayName = ti.displayName;
        if (displayName.isEmpty())
            displayName = decoratedTargetName(ti.buildKey, target);
        else if (m_decorateDisplayNames)
            displayName = decoratedTargetName(displayName, target);
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
                    DeviceTypeKitAspect::deviceTypeId(kit)))
            return false;

    return true;
}

RunConfiguration *RunConfigurationFactory::create(Target *target) const
{
    QTC_ASSERT(m_creator, return nullptr);
    RunConfiguration *rc = m_creator(target);
    QTC_ASSERT(rc, return nullptr);

    // Add the universal aspects.
    for (const RunConfiguration::AspectFactory &factory : theAspectFactories)
        rc->m_aspects.append(factory(target));

    rc->acquaintAspects();
    return rc;
}

RunConfiguration *RunConfigurationCreationInfo::create(Target *target) const
{
    QTC_ASSERT(factory->canHandle(target), return nullptr);

    RunConfiguration *rc = factory->create(target);
    if (!rc)
        return nullptr;

    rc->m_buildKey = buildKey;
    rc->update();
    rc->setDisplayName(displayName);

    return rc;
}

RunConfiguration *RunConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    for (RunConfigurationFactory *factory : g_runConfigurationFactories) {
        if (factory->canHandle(parent)) {
            const Utils::Id id = idFromMap(map);
            if (id.name().startsWith(factory->m_runConfigurationId.name())) {
                RunConfiguration *rc = factory->create(parent);
                if (rc->fromMap(map)) {
                    rc->update();
                    return rc;
                }
                delete rc;
                return nullptr;
            }
        }
    }
    return nullptr;
}

RunConfiguration *RunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    return restore(parent, source->toMap());
}

const QList<RunConfigurationCreationInfo> RunConfigurationFactory::creatorsForTarget(Target *parent)
{
    QList<RunConfigurationCreationInfo> items;
    for (RunConfigurationFactory *factory : g_runConfigurationFactories) {
        if (factory->canHandle(parent))
            items.append(factory->availableCreators(parent));
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
FixedRunConfigurationFactory::availableCreators(Target *parent) const
{
    QString displayName = m_decorateTargetName ? decoratedTargetName(m_fixedBuildTarget, parent)
                                               : m_fixedBuildTarget;
    RunConfigurationCreationInfo rci;
    rci.factory = this;
    rci.displayName = displayName;
    return {rci};
}

} // namespace ProjectExplorer
