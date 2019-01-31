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

#include "project.h"
#include "target.h"
#include "toolchain.h"
#include "abi.h"
#include "buildconfiguration.h"
#include "environmentaspect.h"
#include "kitinformation.h"
#include "runconfigurationaspects.h"
#include "session.h"
#include "kitinformation.h"

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

#ifdef Q_OS_OSX
#include <ApplicationServices/ApplicationServices.h>
#endif

#if defined (WITH_JOURNALD)
#include "journaldwatcher.h"
#endif

using namespace Utils;
using namespace ProjectExplorer::Internal;

namespace {
Q_LOGGING_CATEGORY(statesLog, "qtc.projectmanager.states", QtWarningMsg)
}

namespace ProjectExplorer {

///////////////////////////////////////////////////////////////////////
//
// ISettingsAspect
//
///////////////////////////////////////////////////////////////////////

ISettingsAspect::ISettingsAspect(const ConfigWidgetCreator &creator)
    : m_configWidgetCreator(creator)
{}

QWidget *ISettingsAspect::createConfigWidget() const
{
    QTC_ASSERT(m_configWidgetCreator, return nullptr);
    return m_configWidgetCreator();
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
    \brief The RunConfiguration class is the base class for a run configuration.

    A run configuration specifies how a target should be run, while a runner
    does the actual running.

    The target owns the RunConfiguraitons and a RunControl will need to copy all
    necessary data as the RunControl may continue to exist after the RunConfiguration
    has been destroyed.

    A RunConfiguration disables itself when the project is parsing or has no parsing
    data available. The disabledReason() method can be used to get a user-facing string
    describing why the RunConfiguration considers itself unfit for use.

    Override updateEnabledState() to change the enabled state handling. Override
    disabledReasons() to provide better/more descriptions to the user.

    Connect signals that may change enabled state of your RunConfiguration to updateEnabledState.
*/

static std::vector<RunConfiguration::AspectFactory> theAspectFactories;

RunConfiguration::RunConfiguration(Target *target, Core::Id id)
    : StatefulProjectConfiguration(target, id)
{
    connect(target->project(), &Project::parsingStarted,
            this, [this]() { updateEnabledState(); });
    connect(target->project(), &Project::parsingFinished,
            this, [this]() { updateEnabledState(); });

    connect(target, &Target::addedRunConfiguration,
            this, [this](const RunConfiguration *rc) {
        if (rc == this)
            updateEnabledState();
    });

    connect(this, &RunConfiguration::enabledChanged,
            this, &RunConfiguration::requestRunActionsUpdate);

    Utils::MacroExpander *expander = macroExpander();
    expander->setDisplayName(tr("Run Settings"));
    expander->setAccumulating(true);
    expander->registerSubProvider([target] {
        BuildConfiguration *bc = target->activeBuildConfiguration();
        return bc ? bc->macroExpander() : target->macroExpander();
    });
    expander->registerPrefix("CurrentRun:Env", tr("Variables in the current run environment"),
                             [this](const QString &var) {
        const auto envAspect = aspect<EnvironmentAspect>();
        return envAspect ? envAspect->environment().value(var) : QString();
    });
    expander->registerVariable(Constants::VAR_CURRENTRUN_NAME,
            QCoreApplication::translate("ProjectExplorer", "The currently active run configuration's name."),
            [this] { return displayName(); }, false);

    for (const AspectFactory &factory : theAspectFactories)
        m_aspects.append(factory(target));
}

RunConfiguration::~RunConfiguration() = default;

bool RunConfiguration::isActive() const
{
    return target()->isActive() && target()->activeRunConfiguration() == this;
}

QString RunConfiguration::disabledReason() const
{
    if (target()->project()->isParsing())
        return tr("The Project is currently being parsed.");
    if (!target()->project()->hasParsingData())
        return tr("The project could not be fully parsed.");
    return QString();
}

QWidget *RunConfiguration::createConfigurationWidget()
{
    auto widget = new QWidget;
    auto formLayout = new QFormLayout(widget);
    formLayout->setMargin(0);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    for (ProjectConfigurationAspect *aspect : m_aspects) {
        if (aspect->isVisible())
            aspect->addToConfigurationLayout(formLayout);
    }

    Core::VariableChooser::addSupportForChildWidgets(widget, macroExpander());

    auto detailsWidget = new Utils::DetailsWidget;
    detailsWidget->setState(DetailsWidget::NoSummary);
    detailsWidget->setWidget(widget);
    return detailsWidget;
}

void RunConfiguration::updateEnabledState()
{
    Project *p = target()->project();

    setEnabled(!p->isParsing() && p->hasParsingData());
}

void RunConfiguration::addAspectFactory(const AspectFactory &aspectFactory)
{
    theAspectFactories.push_back(aspectFactory);
}

/*!
 * Returns the RunConfiguration of the currently active target
 * of the startup project, if such exists, or \c nullptr otherwise.
 */

RunConfiguration *RunConfiguration::startupRunConfiguration()
{
    if (Project *pro = SessionManager::startupProject()) {
        if (const Target *target = pro->activeTarget())
            return target->activeRunConfiguration();
    }
    return nullptr;
}

bool RunConfiguration::isConfigured() const
{
    return true;
}

RunConfiguration::ConfigurationState RunConfiguration::ensureConfigured(QString *errorMessage)
{
    if (isConfigured())
        return Configured;
    if (errorMessage)
        *errorMessage = tr("Unknown error.");
    return UnConfigured;
}

BuildConfiguration *RunConfiguration::activeBuildConfiguration() const
{
    if (!target())
        return nullptr;
    return target()->activeBuildConfiguration();
}

Target *RunConfiguration::target() const
{
    return static_cast<Target *>(parent());
}

Project *RunConfiguration::project() const
{
    return target()->project();
}

QVariantMap RunConfiguration::toMap() const
{
    QVariantMap map = ProjectConfiguration::toMap();

    // FIXME: Remove this id mangling, e.g. by using a separate entry for the build key.
    if (!m_buildKey.isEmpty()) {
        const Core::Id mangled = id().withSuffix(m_buildKey);
        map.insert(settingsIdKey(), mangled.toSetting());
    }

    return map;
}

BuildTargetInfo RunConfiguration::buildTargetInfo() const
{
    return target()->applicationTargets().buildTargetInfo(m_buildKey);
}

bool RunConfiguration::fromMap(const QVariantMap &map)
{
    if (!ProjectConfiguration::fromMap(map))
        return false;

    // FIXME: Remove this id mangling, e.g. by using a separate entry for the build key.
    const Core::Id mangledId = Core::Id::fromSetting(map.value(settingsIdKey()));
    m_buildKey = mangledId.suffixAfter(id());

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
    if (auto executableAspect = aspect<ExecutableAspect>())
        r.executable = executableAspect->executable().toString();
    if (auto argumentsAspect = aspect<ArgumentsAspect>())
        r.commandLineArguments = argumentsAspect->arguments(macroExpander());
    if (auto workingDirectoryAspect = aspect<WorkingDirectoryAspect>())
        r.workingDirectory = workingDirectoryAspect->workingDirectory(macroExpander()).toString();
    if (auto environmentAspect = aspect<EnvironmentAspect>())
        r.environment = environmentAspect->environment();
    return r;
}

OutputFormatter *RunConfiguration::createOutputFormatter() const
{
    if (m_outputFormatterCreator)
        return m_outputFormatterCreator(project());
    return new OutputFormatter();
}


/*!
    \class ProjectExplorer::IRunConfigurationFactory

    \brief The IRunConfigurationFactory class restores run configurations from
    settings.

    The run configuration factory is used for restoring run configurations from
    settings and for creating new run configurations in the \gui {Run Settings}
    dialog.
    To restore run configurations, use the
    \c {bool canRestore(Target *parent, const QString &id)}
    and \c {RunConfiguration* create(Target *parent, const QString &id)}
    functions.

    To generate a list of creatable run configurations, use the
    \c {QStringList availableCreationIds(Target *parent)} and
    \c {QString displayNameForType(const QString&)} functions. To create a
    run configuration, use \c create().
*/

/*!
    \fn QStringList ProjectExplorer::IRunConfigurationFactory::availableCreationIds(Target *parent) const

    Shows the list of possible additions to a target. Returns a list of types.
*/

/*!
    \fn QString ProjectExplorer::IRunConfigurationFactory::displayNameForId(Core::Id id) const
    Translates the types to names to display to the user.
*/

static QList<RunConfigurationFactory *> g_runConfigurationFactories;

RunConfigurationFactory::RunConfigurationFactory()
{
    g_runConfigurationFactories.append(this);
}

RunConfigurationFactory::~RunConfigurationFactory()
{
    g_runConfigurationFactories.removeOne(this);
    qDeleteAll(m_ownedRunWorkerFactories);
    m_ownedRunWorkerFactories.clear();
}

QString RunConfigurationFactory::decoratedTargetName(const QString &targetName, Target *target)
{
    QString displayName;
    if (!targetName.isEmpty())
        displayName = QFileInfo(targetName).completeBaseName();
    Core::Id devType = DeviceTypeKitInformation::deviceTypeId(target->kit());
    if (devType != Constants::DESKTOP_DEVICE_TYPE) {
        if (IDevice::ConstPtr dev = DeviceKitInformation::device(target->kit())) {
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
RunConfigurationFactory::availableCreators(Target *parent) const
{
    const QList<BuildTargetInfo> buildTargets = parent->applicationTargets().list;
    const bool hasAnyQtcRunnable = Utils::anyOf(buildTargets,
                                            Utils::equal(&BuildTargetInfo::isQtcRunnable, true));
    return Utils::transform(buildTargets, [&](const BuildTargetInfo &ti) {
        QString displayName = ti.displayName;
        if (displayName.isEmpty())
            displayName = decoratedTargetName(ti.buildKey, parent);
        else if (m_decorateDisplayNames)
            displayName = decoratedTargetName(displayName, parent);
        RunConfigurationCreationInfo rci;
        rci.factory = this;
        rci.id = m_runConfigBaseId;
        rci.buildKey = ti.buildKey;
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
    Adds a list of device types for which this RunConfigurationFactory
    can create RunConfigurations.

    If this function is never called for a RunConfiguarionFactory,
    the factory will create RunConfigurations for all device types.
*/

void RunConfigurationFactory::addSupportedTargetDeviceType(Core::Id id)
{
    m_supportedTargetDeviceTypes.append(id);
}

void RunConfigurationFactory::setDecorateDisplayNames(bool on)
{
    m_decorateDisplayNames = on;
}

RunWorkerFactory *RunConfigurationFactory::addRunWorkerFactoryHelper
    (Core::Id runMode, const std::function<RunWorker *(RunControl *)> &creator)
{
    auto factory = new RunWorkerFactory;
    factory->addConstraint(m_ownTypeChecker);
    factory->addSupportedRunMode(runMode);
    factory->setProducer(creator);
    return factory;
}

void RunConfigurationFactory::addSupportedProjectType(Core::Id id)
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
                    DeviceTypeKitInformation::deviceTypeId(kit)))
            return false;

    return true;
}

RunConfiguration *RunConfigurationCreationInfo::create(Target *target) const
{
    QTC_ASSERT(factory->canHandle(target), return nullptr);
    QTC_ASSERT(id == factory->runConfigurationBaseId(), return nullptr);
    QTC_ASSERT(factory->m_creator, return nullptr);

    RunConfiguration *rc = factory->m_creator(target);
    if (!rc)
        return nullptr;

    rc->m_buildKey = buildKey;
    rc->doAdditionalSetup(*this);
    rc->setDisplayName(displayName);

    return rc;
}

RunConfiguration *RunConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    for (RunConfigurationFactory *factory : g_runConfigurationFactories) {
        if (factory->canHandle(parent)) {
            const Core::Id id = idFromMap(map);
            if (id.name().startsWith(factory->m_runConfigBaseId.name())) {
                QTC_ASSERT(factory->m_creator, continue);
                RunConfiguration *rc = factory->m_creator(parent);
                if (rc->fromMap(map))
                    return rc;
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
    rci.id = runConfigurationBaseId();
    rci.displayName = displayName;
    return {rci};
}


// RunWorkerFactory

static QList<RunWorkerFactory *> g_runWorkerFactories;

RunWorkerFactory::RunWorkerFactory()
{
    g_runWorkerFactories.append(this);
}

RunWorkerFactory::~RunWorkerFactory()
{
    g_runWorkerFactories.removeOne(this);
}

bool RunWorkerFactory::canRun(RunConfiguration *runConfiguration, Core::Id runMode) const
{
    if (!m_supportedRunModes.contains(runMode))
        return false;

    for (const Constraint &constraint : m_constraints) {
        if (!constraint(runConfiguration))
            return false;
    }

    return true;
}

void RunWorkerFactory::setProducer(const WorkerCreator &producer)
{
    m_producer = producer;
}

void RunWorkerFactory::addConstraint(const Constraint &constraint)
{
    // Default constructed Constraints are not worth keeping.
    // FIXME: Make it a QTC_ASSERT once there is no code path
    // using this "feature" anymore.
    if (!constraint)
        return;
    m_constraints.append(constraint);
}

void RunWorkerFactory::addSupportedRunMode(Core::Id runMode)
{
    m_supportedRunModes.append(runMode);
}

void RunWorkerFactory::destroyRemainingRunWorkerFactories()
{
    qDeleteAll(g_runWorkerFactories);
}

/*!
    \class ProjectExplorer::RunControl
    \brief The RunControl class instances represent one item that is run.
*/

/*!
    \fn QIcon ProjectExplorer::RunControl::icon() const
    Returns the icon to be shown in the Outputwindow.

    TODO the icon differs currently only per "mode", so this is more flexible
    than it needs to be.
*/


namespace Internal {

enum class RunWorkerState
{
    Initialized, Starting, Running, Stopping, Done
};

static QString stateName(RunWorkerState s)
{
#    define SN(x) case x: return QLatin1String(#x);
    switch (s) {
        SN(RunWorkerState::Initialized)
        SN(RunWorkerState::Starting)
        SN(RunWorkerState::Running)
        SN(RunWorkerState::Stopping)
        SN(RunWorkerState::Done)
    }
    return QString("<unknown: %1>").arg(int(s));
#    undef SN
}

class RunWorkerPrivate : public QObject
{
public:
    RunWorkerPrivate(RunWorker *runWorker, RunControl *runControl);

    bool canStart() const;
    bool canStop() const;
    void timerEvent(QTimerEvent *ev) override;

    void killStartWatchdog()
    {
        if (startWatchdogTimerId != -1) {
            killTimer(startWatchdogTimerId);
            startWatchdogTimerId = -1;
        }
    }

    void killStopWatchdog()
    {
        if (stopWatchdogTimerId != -1) {
            killTimer(stopWatchdogTimerId);
            stopWatchdogTimerId = -1;
        }
    }

    void startStartWatchdog()
    {
        killStartWatchdog();
        killStopWatchdog();

        if (startWatchdogInterval != 0)
            startWatchdogTimerId = startTimer(startWatchdogInterval);
    }

    void startStopWatchdog()
    {
        killStopWatchdog();
        killStartWatchdog();

        if (stopWatchdogInterval != 0)
            stopWatchdogTimerId = startTimer(stopWatchdogInterval);
    }

    RunWorker *q;
    RunWorkerState state = RunWorkerState::Initialized;
    const QPointer<RunControl> runControl;
    QList<RunWorker *> startDependencies;
    QList<RunWorker *> stopDependencies;
    QString id;

    QVariantMap data;
    int startWatchdogInterval = 0;
    int startWatchdogTimerId = -1;
    std::function<void()> startWatchdogCallback;
    int stopWatchdogInterval = 0; // 5000;
    int stopWatchdogTimerId = -1;
    std::function<void()> stopWatchdogCallback;
    bool supportsReRunning = true;
    bool essential = false;
};

enum class RunControlState
{
    Initialized,      // Default value after creation.
    Starting,         // Actual process/tool starts.
    Running,          // All good and running.
    Stopping,         // initiateStop() was called, stop application/tool
    Stopped,          // all good, but stopped. Can possibly be re-started
    Finishing,        // Application tab manually closed
    Finished          // Final state, will self-destruct with deleteLater()
};

static QString stateName(RunControlState s)
{
#    define SN(x) case x: return QLatin1String(#x);
    switch (s) {
        SN(RunControlState::Initialized)
        SN(RunControlState::Starting)
        SN(RunControlState::Running)
        SN(RunControlState::Stopping)
        SN(RunControlState::Stopped)
        SN(RunControlState::Finishing)
        SN(RunControlState::Finished)
    }
    return QString("<unknown: %1>").arg(int(s));
#    undef SN
}

class RunControlPrivate : public QObject
{
public:
    RunControlPrivate(RunControl *parent, RunConfiguration *runConfiguration, Core::Id mode)
        : q(parent), runMode(mode), runConfiguration(runConfiguration)
    {
        icon = Icons::RUN_SMALL_TOOLBAR;
        if (runConfiguration) {
            runnable = runConfiguration->runnable();
            displayName  = runConfiguration->displayName();
            outputFormatter = runConfiguration->createOutputFormatter();
            device = runnable.device;
            if (!device)
                device = DeviceKitInformation::device(runConfiguration->target()->kit());
            project = runConfiguration->target()->project();
        } else {
            outputFormatter = new OutputFormatter();
        }
    }

    ~RunControlPrivate() override
    {
        QTC_CHECK(state == RunControlState::Finished || state == RunControlState::Initialized);
        disconnect();
        q = nullptr;
        qDeleteAll(m_workers);
        m_workers.clear();
        delete outputFormatter;
    }

    Q_ENUM(RunControlState)

    void checkState(RunControlState expectedState);
    void setState(RunControlState state);

    void debugMessage(const QString &msg);

    void initiateStart();
    void initiateReStart();
    void continueStart();
    void initiateStop();
    void forceStop();
    void continueStopOrFinish();
    void initiateFinish();

    void onWorkerStarted(RunWorker *worker);
    void onWorkerStopped(RunWorker *worker);
    void onWorkerFailed(RunWorker *worker, const QString &msg);

    void showError(const QString &msg);

    static bool isAllowedTransition(RunControlState from, RunControlState to);
    bool supportsReRunning() const;

    RunControl *q;
    QString displayName;
    Runnable runnable;
    IDevice::ConstPtr device;
    Core::Id runMode;
    Utils::Icon icon;
    const QPointer<RunConfiguration> runConfiguration; // Not owned.
    QPointer<Project> project; // Not owned.
    QPointer<Utils::OutputFormatter> outputFormatter = nullptr;
    std::function<bool(bool*)> promptToStop;
    std::vector<RunWorkerFactory> m_factories;

    // A handle to the actual application process.
    Utils::ProcessHandle applicationProcessHandle;

    RunControlState state = RunControlState::Initialized;

    QList<QPointer<RunWorker>> m_workers;
};

} // Internal

using namespace Internal;

RunControl::RunControl(RunConfiguration *runConfiguration, Core::Id mode) :
    d(std::make_unique<RunControlPrivate>(this, runConfiguration, mode))
{
#ifdef WITH_JOURNALD
    if (!device().isNull() && device()->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        JournaldWatcher::instance()->subscribe(this, [this](const JournaldWatcher::LogEntry &entry) {

            if (entry.value("_MACHINE_ID") != JournaldWatcher::instance()->machineId())
                return;

            const QByteArray pid = entry.value("_PID");
            if (pid.isEmpty())
                return;

            const qint64 pidNum = static_cast<qint64>(QString::fromLatin1(pid).toInt());
            if (pidNum != d->applicationProcessHandle.pid())
                return;

            const QString message = QString::fromUtf8(entry.value("MESSAGE")) + "\n";
            appendMessageRequested(this, message, Utils::OutputFormat::LogMessageFormat);
        });
    }
#endif
}

RunControl::RunControl(const IDevice::ConstPtr &device, Core::Id mode)
    : RunControl(nullptr, mode)
{
    d->device = device;
}

RunControl::~RunControl()
{
#ifdef WITH_JOURNALD
    JournaldWatcher::instance()->unsubscribe(this);
#endif
}

void RunControl::initiateStart()
{
    emit aboutToStart();
    d->initiateStart();
}

void RunControl::initiateReStart()
{
    emit aboutToStart();
    d->initiateReStart();
}

void RunControl::initiateStop()
{
    d->initiateStop();
}

void RunControl::forceStop()
{
    d->forceStop();
}

void RunControl::initiateFinish()
{
    QTimer::singleShot(0, d.get(), &RunControlPrivate::initiateFinish);
}

using WorkerCreators = QHash<Core::Id, RunControl::WorkerCreator>;

static WorkerCreators &theWorkerCreators()
{
    static WorkerCreators creators;
    return creators;
}

void RunControl::registerWorkerCreator(Core::Id id, const WorkerCreator &workerCreator)
{
    theWorkerCreators().insert(id, workerCreator);
    auto keys = theWorkerCreators().keys();
    Q_UNUSED(keys);
}

RunWorker *RunControl::createWorker(Core::Id id)
{
    auto keys = theWorkerCreators().keys();
    Q_UNUSED(keys);
    WorkerCreator creator = theWorkerCreators().value(id);
    if (creator)
        return creator(this);
    creator = device()->workerCreator(id);
    if (creator)
        return creator(this);
    return nullptr;
}

RunWorkerFactory::WorkerCreator RunControl::producer(RunConfiguration *runConfig, Core::Id runMode)
{
    const auto canRun = std::bind(&RunWorkerFactory::canRun, std::placeholders::_1, runConfig, runMode);
    const QList<RunWorkerFactory *> candidates = Utils::filtered(g_runWorkerFactories, canRun);

    // This is legit, there might be combinations that cannot run.
    if (candidates.empty())
        return {};

    // There should be at most one top-level producer feeling responsible per combination.
    // Breaking a tie should be done by tightening the restrictions on one of them.
    QTC_CHECK(candidates.size() == 1);
    return candidates.front()->producer();
}

void RunControlPrivate::initiateStart()
{
    checkState(RunControlState::Initialized);
    setState(RunControlState::Starting);
    debugMessage("Queue: Starting");

    continueStart();
}

void RunControlPrivate::initiateReStart()
{
    checkState(RunControlState::Stopped);

    // Re-set worked on re-runs.
    for (RunWorker *worker : m_workers) {
        if (worker->d->state == RunWorkerState::Done)
            worker->d->state = RunWorkerState::Initialized;
    }

    setState(RunControlState::Starting);
    debugMessage("Queue: ReStarting");

    continueStart();
}

void RunControlPrivate::continueStart()
{
    checkState(RunControlState::Starting);
    bool allDone = true;
    debugMessage("Looking for next worker");
    for (RunWorker *worker : m_workers) {
        if (worker) {
            const QString &workerId = worker->d->id;
            debugMessage("  Examining worker " + workerId);
            switch (worker->d->state) {
                case RunWorkerState::Initialized:
                    debugMessage("  " + workerId + " is not done yet.");
                    if (worker->d->canStart()) {
                        debugMessage("Starting " + workerId);
                        worker->d->state = RunWorkerState::Starting;
                        QTimer::singleShot(0, worker, &RunWorker::initiateStart);
                        return;
                    }
                    allDone = false;
                    debugMessage("  " + workerId + " cannot start.");
                    break;
                case RunWorkerState::Starting:
                    debugMessage("  " + workerId + " currently starting");
                    allDone = false;
                    break;
                case RunWorkerState::Running:
                    debugMessage("  " + workerId + " currently running");
                    break;
                case RunWorkerState::Stopping:
                    debugMessage("  " + workerId + " currently stopping");
                    continue;
                case RunWorkerState::Done:
                    debugMessage("  " + workerId + " was done before");
                    break;
            }
        } else {
            debugMessage("Found unknown deleted worker while starting");
        }
    }
    if (allDone)
        setState(RunControlState::Running);
}

void RunControlPrivate::initiateStop()
{
    if (state != RunControlState::Starting && state != RunControlState::Running)
        qDebug() << "Unexpected initiateStop() in state" << stateName(state);

    setState(RunControlState::Stopping);
    debugMessage("Queue: Stopping for all workers");

    continueStopOrFinish();
}

void RunControlPrivate::continueStopOrFinish()
{
    bool allDone = true;

    auto queueStop = [this](RunWorker *worker, const QString &message) {
        if (worker->d->canStop()) {
            debugMessage(message);
            worker->d->state = RunWorkerState::Stopping;
            QTimer::singleShot(0, worker, &RunWorker::initiateStop);
        } else {
            debugMessage(" " + worker->d->id + " is waiting for dependent workers to stop");
        }
    };

    for (RunWorker *worker : m_workers) {
        if (worker) {
            const QString &workerId = worker->d->id;
            debugMessage("  Examining worker " + workerId);
            switch (worker->d->state) {
                case RunWorkerState::Initialized:
                    debugMessage("  " + workerId + " was Initialized, setting to Done");
                    worker->d->state = RunWorkerState::Done;
                    break;
                case RunWorkerState::Stopping:
                    debugMessage("  " + workerId + " was already Stopping. Keeping it that way");
                    allDone = false;
                    break;
                case RunWorkerState::Starting:
                    queueStop(worker, "  " + workerId + " was Starting, queuing stop");
                    allDone = false;
                    break;
                case RunWorkerState::Running:
                    queueStop(worker, "  " + workerId + " was Running, queuing stop");
                    allDone = false;
                    break;
                case RunWorkerState::Done:
                    debugMessage("  " + workerId + " was Done. Good.");
                    break;
            }
        } else {
            debugMessage("Found unknown deleted worker");
        }
    }

    RunControlState targetState;
    if (state == RunControlState::Finishing) {
        targetState = RunControlState::Finished;
    } else {
        checkState(RunControlState::Stopping);
        targetState = RunControlState::Stopped;
    }

    if (allDone) {
        debugMessage("All Stopped");
        setState(targetState);
    } else {
        debugMessage("Not all workers Stopped. Waiting...");
    }
}

void RunControlPrivate::forceStop()
{
    if (state == RunControlState::Finished) {
        debugMessage("Was finished, too late to force Stop");
        return;
    }
    for (RunWorker *worker : m_workers) {
        if (worker) {
            const QString &workerId = worker->d->id;
            debugMessage("  Examining worker " + workerId);
            switch (worker->d->state) {
                case RunWorkerState::Initialized:
                    debugMessage("  " + workerId + " was Initialized, setting to Done");
                    break;
                case RunWorkerState::Stopping:
                    debugMessage("  " + workerId + " was already Stopping. Set it forcefully to Done.");
                    break;
                case RunWorkerState::Starting:
                    debugMessage("  " + workerId + " was Starting. Set it forcefully to Done.");
                    break;
                case RunWorkerState::Running:
                    debugMessage("  " + workerId + " was Running. Set it forcefully to Done.");
                    break;
                case RunWorkerState::Done:
                    debugMessage("  " + workerId + " was Done. Good.");
                    break;
            }
            worker->d->state = RunWorkerState::Done;
        } else {
            debugMessage("Found unknown deleted worker");
        }
    }

    setState(RunControlState::Stopped);
    debugMessage("All Stopped");
}

void RunControlPrivate::initiateFinish()
{
    setState(RunControlState::Finishing);
    debugMessage("Ramping down");

    continueStopOrFinish();
}

void RunControlPrivate::onWorkerStarted(RunWorker *worker)
{
    worker->d->state = RunWorkerState::Running;

    if (state == RunControlState::Starting) {
        debugMessage(worker->d->id + " start succeeded");
        continueStart();
        return;
    }
    showError(RunControl::tr("Unexpected run control state %1 when worker %2 started.")
                  .arg(stateName(state))
                  .arg(worker->d->id));
}

void RunControlPrivate::onWorkerFailed(RunWorker *worker, const QString &msg)
{
    worker->d->state = RunWorkerState::Done;

    showError(msg);
    switch (state) {
    case RunControlState::Initialized:
        // FIXME 1: We don't have an output pane yet, so use some other mechanism for now.
        // FIXME 2: Translation...
        QMessageBox::critical(Core::ICore::dialogParent(),
             QCoreApplication::translate("TaskHub", "Error"),
             QString("Failure during startup. Aborting.") + "<p>" + msg);
        continueStopOrFinish();
        break;
    case RunControlState::Starting:
    case RunControlState::Running:
        initiateStop();
        break;
    case RunControlState::Stopping:
    case RunControlState::Finishing:
        continueStopOrFinish();
        break;
    case RunControlState::Stopped:
    case RunControlState::Finished:
        QTC_CHECK(false); // Should not happen.
        continueStopOrFinish();
        break;
    }
}

void RunControlPrivate::onWorkerStopped(RunWorker *worker)
{
    const QString &workerId = worker->d->id;
    switch (worker->d->state) {
    case RunWorkerState::Running:
        // That was a spontaneous stop.
        worker->d->state = RunWorkerState::Done;
        debugMessage(workerId + " stopped spontaneously.");
        break;
    case RunWorkerState::Stopping:
        worker->d->state = RunWorkerState::Done;
        debugMessage(workerId + " stopped expectedly.");
        break;
    case RunWorkerState::Done:
        worker->d->state = RunWorkerState::Done;
        debugMessage(workerId + " stopped twice. Huh? But harmless.");
        return; // Sic!
    default:
        debugMessage(workerId + " stopped unexpectedly in state"
                     + stateName(worker->d->state));
        worker->d->state = RunWorkerState::Done;
        break;
    }

    if (state == RunControlState::Finishing || state == RunControlState::Stopping) {
        continueStopOrFinish();
        return;
    } else if (worker->isEssential()) {
        debugMessage(workerId + " is essential. Stopping all others.");
        initiateStop();
        return;
    }

    for (RunWorker *dependent : worker->d->stopDependencies) {
        switch (dependent->d->state) {
        case RunWorkerState::Done:
            break;
        case RunWorkerState::Initialized:
            dependent->d->state = RunWorkerState::Done;
            break;
        default:
            debugMessage("Killing " + dependent->d->id + " as it depends on stopped " + workerId);
            dependent->d->state = RunWorkerState::Stopping;
            QTimer::singleShot(0, dependent, &RunWorker::initiateStop);
            break;
        }
    }

    debugMessage("Checking whether all stopped");
    bool allDone = true;
    for (RunWorker *worker : m_workers) {
        if (worker) {
            const QString &workerId = worker->d->id;
            debugMessage("  Examining worker " + workerId);
            switch (worker->d->state) {
                case RunWorkerState::Initialized:
                    debugMessage("  " + workerId + " was Initialized.");
                    break;
                case RunWorkerState::Starting:
                    debugMessage("  " + workerId + " was Starting, waiting for its response");
                    allDone = false;
                    break;
                case RunWorkerState::Running:
                    debugMessage("  " + workerId + " was Running, waiting for its response");
                    allDone = false;
                    break;
                case RunWorkerState::Stopping:
                    debugMessage("  " + workerId + " was already Stopping. Keeping it that way");
                    allDone = false;
                    break;
                case RunWorkerState::Done:
                    debugMessage("  " + workerId + " was Done. Good.");
                    break;
            }
        } else {
            debugMessage("Found unknown deleted worker");
        }
    }

    if (allDone) {
        if (state == RunControlState::Stopped) {
            debugMessage("All workers stopped, but runControl was already stopped.");
        } else {
            debugMessage("All workers stopped. Set runControl to Stopped");
            setState(RunControlState::Stopped);
        }
    } else {
        debugMessage("Not all workers stopped. Waiting...");
    }
}

void RunControlPrivate::showError(const QString &msg)
{
    if (!msg.isEmpty())
        q->appendMessage(msg + '\n', ErrorMessageFormat);
}

Utils::OutputFormatter *RunControl::outputFormatter() const
{
    return d->outputFormatter;
}

Core::Id RunControl::runMode() const
{
    return d->runMode;
}

const Runnable &RunControl::runnable() const
{
    return d->runnable;
}

void RunControl::setRunnable(const Runnable &runnable)
{
    d->runnable = runnable;
}

QString RunControl::displayName() const
{
    return d->displayName;
}

void RunControl::setDisplayName(const QString &displayName)
{
    d->displayName = displayName;
}

void RunControl::setIcon(const Utils::Icon &icon)
{
    d->icon = icon;
}

Utils::Icon RunControl::icon() const
{
    return d->icon;
}

IDevice::ConstPtr RunControl::device() const
{
   return d->device;
}

RunConfiguration *RunControl::runConfiguration() const
{
    return d->runConfiguration.data();
}

Project *RunControl::project() const
{
    return d->project.data();
}

/*!
    A handle to the application process.

    This is typically a process id, but should be treated as
    opaque handle to the process controled by this \c RunControl.
*/

ProcessHandle RunControl::applicationProcessHandle() const
{
    return d->applicationProcessHandle;
}

void RunControl::setApplicationProcessHandle(const ProcessHandle &handle)
{
    if (d->applicationProcessHandle != handle) {
        d->applicationProcessHandle = handle;
        emit applicationProcessHandleChanged(QPrivateSignal());
    }
}

/*!
    Prompts to stop. If \a optionalPrompt is passed, a \gui {Do not ask again}
    checkbox is displayed and the result is returned in \a *optionalPrompt.
*/

bool RunControl::promptToStop(bool *optionalPrompt) const
{
    QTC_ASSERT(isRunning(), return true);
    if (optionalPrompt && !*optionalPrompt)
        return true;

    // Overridden.
    if (d->promptToStop)
        return d->promptToStop(optionalPrompt);

    const QString msg = tr("<html><head/><body><center><i>%1</i> is still running.<center/>"
                           "<center>Force it to quit?</center></body></html>").arg(displayName());
    return showPromptToStopDialog(tr("Application Still Running"), msg,
                                  tr("Force &Quit"), tr("&Keep Running"),
                                  optionalPrompt);
}

void RunControl::setPromptToStop(const std::function<bool (bool *)> &promptToStop)
{
    d->promptToStop = promptToStop;
}

bool RunControl::supportsReRunning() const
{
    return d->supportsReRunning();
}

bool RunControlPrivate::supportsReRunning() const
{
    for (RunWorker *worker : m_workers) {
        if (!worker->d->supportsReRunning)
            return false;
        if (worker->d->state != RunWorkerState::Done)
            return false;
    }
    return true;
}

bool RunControl::isRunning() const
{
    return d->state == RunControlState::Running;
}

bool RunControl::isStarting() const
{
    return d->state == RunControlState::Starting;
}

bool RunControl::isStopping() const
{
    return d->state == RunControlState::Stopping;
}

bool RunControl::isStopped() const
{
    return d->state == RunControlState::Stopped;
}

/*!
    Prompts to terminate the application with the \gui {Do not ask again}
    checkbox.
*/

bool RunControl::showPromptToStopDialog(const QString &title,
                                        const QString &text,
                                        const QString &stopButtonText,
                                        const QString &cancelButtonText,
                                        bool *prompt)
{
    // Show a question message box where user can uncheck this
    // question for this class.
    Utils::CheckableMessageBox messageBox(Core::ICore::mainWindow());
    messageBox.setWindowTitle(title);
    messageBox.setText(text);
    messageBox.setStandardButtons(QDialogButtonBox::Yes|QDialogButtonBox::Cancel);
    if (!stopButtonText.isEmpty())
        messageBox.button(QDialogButtonBox::Yes)->setText(stopButtonText);
    if (!cancelButtonText.isEmpty())
        messageBox.button(QDialogButtonBox::Cancel)->setText(cancelButtonText);
    messageBox.setDefaultButton(QDialogButtonBox::Yes);
    if (prompt) {
        messageBox.setCheckBoxText(Utils::CheckableMessageBox::msgDoNotAskAgain());
        messageBox.setChecked(false);
    } else {
        messageBox.setCheckBoxVisible(false);
    }
    messageBox.exec();
    const bool close = messageBox.clickedStandardButton() == QDialogButtonBox::Yes;
    if (close && prompt && messageBox.isChecked())
        *prompt = false;
    return close;
}

bool RunControlPrivate::isAllowedTransition(RunControlState from, RunControlState to)
{
    switch (from) {
    case RunControlState::Initialized:
        return to == RunControlState::Starting
            || to == RunControlState::Finishing;
    case RunControlState::Starting:
        return to == RunControlState::Running
            || to == RunControlState::Stopping
            || to == RunControlState::Finishing;
    case RunControlState::Running:
        return to == RunControlState::Stopping
            || to == RunControlState::Stopped
            || to == RunControlState::Finishing;
    case RunControlState::Stopping:
        return to == RunControlState::Stopped
            || to == RunControlState::Finishing;
    case RunControlState::Stopped:
        return to == RunControlState::Finishing;
    case RunControlState::Finishing:
        return to == RunControlState::Finished;
    case RunControlState::Finished:
        return false;
    }
    return false;
}

void RunControlPrivate::checkState(RunControlState expectedState)
{
    if (state != expectedState)
        qDebug() << "Unexpected run control state " << stateName(expectedState)
                 << " have: " << stateName(state);
}

void RunControlPrivate::setState(RunControlState newState)
{
    if (!isAllowedTransition(state, newState))
        qDebug() << "Invalid run control state transition from " << stateName(state)
                 << " to " << stateName(newState);

    state = newState;

    debugMessage("Entering state " + stateName(newState));

    // Extra reporting.
    switch (state) {
    case RunControlState::Running:
        emit q->started();
        break;
    case RunControlState::Stopped:
        q->setApplicationProcessHandle(Utils::ProcessHandle());
        emit q->stopped();
        break;
    case RunControlState::Finished:
        emit q->finished();
        debugMessage("All finished. Deleting myself");
        q->deleteLater();
        break;
    default:
        break;
    }
}

void RunControlPrivate::debugMessage(const QString &msg)
{
    qCDebug(statesLog()) << msg;
}

void RunControl::appendMessage(const QString &msg, Utils::OutputFormat format)
{
    emit appendMessageRequested(this, msg, format);
}

// SimpleTargetRunner

SimpleTargetRunner::SimpleTargetRunner(RunControl *runControl)
    : RunWorker(runControl)
{
    setId("SimpleTargetRunner");
    m_runnable = runControl->runnable(); // Default value. Can be overridden using setRunnable.
    m_device = runControl->device(); // Default value. Can be overridden using setDevice.
    if (auto runConfig = runControl->runConfiguration()) {
        if (auto terminalAspect = runConfig->aspect<TerminalAspect>())
            m_useTerminal = terminalAspect->useTerminal();
    }
}

void SimpleTargetRunner::start()
{
    m_stopReported = false;
    m_launcher.disconnect(this);
    m_launcher.setUseTerminal(m_useTerminal);

    const bool isDesktop = m_device.isNull()
            || m_device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
    const QString rawDisplayName = m_runnable.displayName();
    const QString displayName = isDesktop
            ? QDir::toNativeSeparators(rawDisplayName)
            : rawDisplayName;
    const QString msg = RunControl::tr("Starting %1 %2...")
            .arg(displayName).arg(m_runnable.commandLineArguments);
    appendMessage(msg, Utils::NormalMessageFormat);

    if (isDesktop) {

        connect(&m_launcher, &ApplicationLauncher::appendMessage,
                this, &SimpleTargetRunner::appendMessage);
        connect(&m_launcher, &ApplicationLauncher::processStarted,
                this, &SimpleTargetRunner::onProcessStarted);
        connect(&m_launcher, &ApplicationLauncher::processExited,
                this, &SimpleTargetRunner::onProcessFinished);
        connect(&m_launcher, &ApplicationLauncher::error,
                this, &SimpleTargetRunner::onProcessError);

        const QString executable = m_runnable.executable;
        if (executable.isEmpty()) {
            reportFailure(RunControl::tr("No executable specified."));
        } else {
            m_launcher.start(m_runnable);
        }

    } else {

        connect(&m_launcher, &ApplicationLauncher::reportError,
                this, [this](const QString &msg) {
                    reportFailure(msg);
                });

        connect(&m_launcher, &ApplicationLauncher::remoteStderr,
                this, [this](const QString &output) {
                    appendMessage(output, Utils::StdErrFormatSameLine, false);
                });

        connect(&m_launcher, &ApplicationLauncher::remoteStdout,
                this, [this](const QString &output) {
                    appendMessage(output, Utils::StdOutFormatSameLine, false);
                });

        connect(&m_launcher, &ApplicationLauncher::finished,
                this, [this] {
                    m_launcher.disconnect(this);
                    reportStopped();
                });

        connect(&m_launcher, &ApplicationLauncher::processStarted,
                this, [this] {
                    appendMessage("Application launcher started", Utils::NormalMessageFormat);
//                    reportStarted();
                });

        connect(&m_launcher, &ApplicationLauncher::processExited,
                this, [this] {
                    m_launcher.disconnect(this);
                    reportStopped();
                });

        connect(&m_launcher, &ApplicationLauncher::remoteProcessStarted,
                this, [this] {
                    reportStarted();
                });

        connect(&m_launcher, &ApplicationLauncher::reportProgress,
                this, [this](const QString &progressString) {
                    appendMessage(progressString, Utils::NormalMessageFormat);
                });

        m_launcher.start(m_runnable, device());
    }
}

void SimpleTargetRunner::stop()
{
    m_launcher.stop();
}

void SimpleTargetRunner::onProcessStarted()
{
    // Console processes only know their pid after being started
    ProcessHandle pid = m_launcher.applicationPID();
    runControl()->setApplicationProcessHandle(pid);
    pid.activate();
    reportStarted();
}

void SimpleTargetRunner::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    QString msg;
    if (status == QProcess::CrashExit)
        msg = tr("%1 crashed.");
    else
        msg = tr("%2 exited with code %1").arg(exitCode);
    appendMessage(msg.arg(m_runnable.displayName()), Utils::NormalMessageFormat);
    if (!m_stopReported) {
        m_stopReported = true;
        reportStopped();
    }
}

void SimpleTargetRunner::onProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::Timedout)
        return; // No actual change on the process side.
    QString msg = userMessageForProcessError(error, m_runnable.displayName());
    appendMessage(msg, Utils::NormalMessageFormat);
    if (!m_stopReported) {
        m_stopReported = true;
        reportStopped();
    }
}

IDevice::ConstPtr SimpleTargetRunner::device() const
{
    return m_device;
}

void SimpleTargetRunner::setRunnable(const Runnable &runnable)
{
    m_runnable = runnable;
}

void SimpleTargetRunner::setDevice(const IDevice::ConstPtr &device)
{
    m_device = device;
}

// RunWorkerPrivate

RunWorkerPrivate::RunWorkerPrivate(RunWorker *runWorker, RunControl *runControl)
    : q(runWorker), runControl(runControl)
{
    runControl->d->m_workers.append(runWorker);
}

bool RunWorkerPrivate::canStart() const
{
    if (state != RunWorkerState::Initialized)
        return false;
    for (RunWorker *worker : startDependencies) {
        QTC_ASSERT(worker, continue);
        if (worker->d->state != RunWorkerState::Done
                && worker->d->state != RunWorkerState::Running)
            return false;
    }
    return true;
}

bool RunWorkerPrivate::canStop() const
{
    if (state != RunWorkerState::Starting && state != RunWorkerState::Running)
        return false;
    for (RunWorker *worker : stopDependencies) {
        QTC_ASSERT(worker, continue);
        if (worker->d->state != RunWorkerState::Done)
            return false;
    }
    return true;
}

void RunWorkerPrivate::timerEvent(QTimerEvent *ev)
{
    if (ev->timerId() == startWatchdogTimerId) {
        if (startWatchdogCallback) {
            killStartWatchdog();
            startWatchdogCallback();
        } else {
            q->reportFailure(RunWorker::tr("Worker start timed out."));
        }
        return;
    }
    if (ev->timerId() == stopWatchdogTimerId) {
        if (stopWatchdogCallback) {
            killStopWatchdog();
            stopWatchdogCallback();
        } else {
            q->reportFailure(RunWorker::tr("Worker stop timed out."));
        }
        return;
    }
}

/*!
    \class ProjectExplorer::RunWorker

    \brief The RunWorker class encapsulates a task that forms part, or
    the whole of the operation of a tool for a certain \c RunConfiguration
    according to some \c RunMode.

    A typical example for a \c RunWorker is a process, either the
    application process itself, or a helper process, such as a watchdog
    or a log parser.

    A \c RunWorker has a simple state model covering the \c Initialized,
    \c Starting, \c Running, \c Stopping, and \c Done states.

    In the course of the operation of tools several \c RunWorkers
    may co-operate and form a combined state that is presented
    to the user as \c RunControl, with direct interaction made
    possible through the buttons in the \uicontrol{Application Output}
    pane.

    RunWorkers are typically created together with their RunControl.
    The startup order of RunWorkers under a RunControl can be
    specified by making a RunWorker dependent on others.

    When a RunControl starts, it calls \c initiateStart() on RunWorkers
    with fulfilled dependencies until all workers are \c Running, or in case
    of short-lived helper tasks, \c Done.

    A RunWorker can stop spontaneously, for example when the main application
    process ends. In this case, it typically calls \c initiateStop()
    on its RunControl, which in turn passes this to all sibling
    RunWorkers.

    Pressing the stop button in the \uicontrol{Application Output} pane
    also calls \c initiateStop on the RunControl.
*/

RunWorker::RunWorker(RunControl *runControl)
    : d(std::make_unique<RunWorkerPrivate>(this, runControl))
{ }

RunWorker::~RunWorker() = default;

/*!
 * This function is called by the RunControl once all dependencies
 * are fulfilled.
 */
void RunWorker::initiateStart()
{
    d->startStartWatchdog();
    d->runControl->d->debugMessage("Initiate start for " + d->id);
    start();
}

/*!
 * This function has to be called by a RunWorker implementation
 * to notify its RunControl about the successful start of this RunWorker.
 *
 * The RunControl may start other RunWorkers in response.
 */
void RunWorker::reportStarted()
{
    d->killStartWatchdog();
    d->runControl->d->onWorkerStarted(this);
    emit started();
}

/*!
 * This function is called by the RunControl in its own \c initiateStop
 * implementation, which is triggered in response to pressing the
 * stop button in the \uicontrol{Application Output} pane or on direct
 * request of one of the sibling RunWorkers.
 */
void RunWorker::initiateStop()
{
    d->startStopWatchdog();
    d->runControl->d->debugMessage("Initiate stop for " + d->id);
    stop();
}

/*!
 * This function has to be called by a RunWorker implementation
 * to notify its RunControl about this RunWorker having stopped.
 *
 * The stop can be spontaneous, or in response to an initiateStop()
 * or an initiateFinish() call.
 *
 * The RunControl will adjust its global state in response.
 */
void RunWorker::reportStopped()
{
    d->killStopWatchdog();
    d->runControl->d->onWorkerStopped(this);
    emit stopped();
}

/*!
 * This function can be called by a RunWorker implementation for short-lived
 * tasks to notify its RunControl about this task being successful finished.
 * Dependent startup tasks can proceed, in cases of spontaneous or scheduled
 * stops, the effect is the same as \c reportStopped().
 *
 */
void RunWorker::reportDone()
{
    d->killStartWatchdog();
    d->killStopWatchdog();
    switch (d->state) {
        case RunWorkerState::Initialized:
            QTC_CHECK(false);
            d->state = RunWorkerState::Done;
            break;
        case RunWorkerState::Starting:
            reportStarted();
            reportStopped();
            break;
        case RunWorkerState::Running:
        case RunWorkerState::Stopping:
            reportStopped();
            break;
        case RunWorkerState::Done:
            break;
    }
}

/*!
 * This function can be called by a RunWorker implementation to
 * signal a problem in the operation in this worker. The
 * RunControl will start to ramp down through initiateStop().
 */
void RunWorker::reportFailure(const QString &msg)
{
    d->killStartWatchdog();
    d->killStopWatchdog();
    d->runControl->d->onWorkerFailed(this, msg);
}

/*!
 * Appends a message in the specified \a format to
 * the owning RunControl's \uicontrol{Application Output} pane.
 */
void RunWorker::appendMessage(const QString &msg, OutputFormat format, bool appendNewLine)
{
    if (!appendNewLine || msg.endsWith('\n'))
        d->runControl->appendMessage(msg, format);
    else
        d->runControl->appendMessage(msg + '\n', format);
}

IDevice::ConstPtr RunWorker::device() const
{
    return d->runControl->device();
}

const Runnable &RunWorker::runnable() const
{
    return d->runControl->runnable();
}

Core::Id RunWorker::runMode() const
{
    return d->runControl->runMode();
}

void RunWorker::addStartDependency(RunWorker *dependency)
{
    d->startDependencies.append(dependency);
}

void RunWorker::addStopDependency(RunWorker *dependency)
{
    d->stopDependencies.append(dependency);
}

RunControl *RunWorker::runControl() const
{
    return d->runControl;
}

void RunWorker::setId(const QString &id)
{
    d->id = id;
}

void RunWorker::setStartTimeout(int ms, const std::function<void()> &callback)
{
    d->startWatchdogInterval = ms;
    d->startWatchdogCallback = callback;
}

void RunWorker::setStopTimeout(int ms, const std::function<void()> &callback)
{
    d->stopWatchdogInterval = ms;
    d->stopWatchdogCallback = callback;
}

void RunWorker::recordData(const QString &channel, const QVariant &data)
{
    d->data[channel] = data;
}

QVariant RunWorker::recordedData(const QString &channel) const
{
    return d->data[channel];
}

void RunWorker::setSupportsReRunning(bool reRunningSupported)
{
    d->supportsReRunning = reRunningSupported;
}

bool RunWorker::supportsReRunning() const
{
    return d->supportsReRunning;
}

QString RunWorker::userMessageForProcessError(QProcess::ProcessError error, const QString &program)
{
    QString failedToStart = tr("The process failed to start.");
    QString msg = tr("An unknown error in the process occurred.");
    switch (error) {
        case QProcess::FailedToStart:
            msg = failedToStart + ' ' + tr("Either the "
                "invoked program \"%1\" is missing, or you may have insufficient "
                "permissions to invoke the program.").arg(program);
            break;
        case QProcess::Crashed:
            msg = tr("The process was ended forcefully.");
            break;
        case QProcess::Timedout:
            // "The last waitFor...() function timed out. "
            //   "The state of QProcess is unchanged, and you can try calling "
            // "waitFor...() again."
            return QString(); // sic!
        case QProcess::WriteError:
            msg = tr("An error occurred when attempting to write "
                "to the process. For example, the process may not be running, "
                "or it may have closed its input channel.");
            break;
        case QProcess::ReadError:
            msg = tr("An error occurred when attempting to read from "
                "the process. For example, the process may not be running.");
            break;
        case QProcess::UnknownError:
            break;
    }
    return msg;
}

bool RunWorker::isEssential() const
{
    return d->essential;
}

void RunWorker::setEssential(bool essential)
{
    d->essential = essential;
}

void RunWorker::start()
{
    reportStarted();
}

void RunWorker::stop()
{
    reportStopped();
}

} // namespace ProjectExplorer
