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
#include "runnables.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>

#include <QDir>
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
Q_LOGGING_CATEGORY(statesLog, "qtc.projectmanager.states")
}

namespace ProjectExplorer {

///////////////////////////////////////////////////////////////////////
//
// ISettingsAspect
//
///////////////////////////////////////////////////////////////////////

ISettingsAspect *ISettingsAspect::clone() const
{
    ISettingsAspect *other = create();
    QVariantMap data;
    toMap(data);
    other->fromMap(data);
    return other;
}

///////////////////////////////////////////////////////////////////////
//
// IRunConfigurationAspect
//
///////////////////////////////////////////////////////////////////////

IRunConfigurationAspect::IRunConfigurationAspect(RunConfiguration *runConfig) :
    m_runConfiguration(runConfig)
{ }

IRunConfigurationAspect::~IRunConfigurationAspect()
{
    delete m_projectSettings;
}

/*!
    Returns the widget used to configure this run configuration. Ownership is
    transferred to the caller.
*/

RunConfigWidget *IRunConfigurationAspect::createConfigurationWidget() const
{
    return m_runConfigWidgetCreator ? m_runConfigWidgetCreator() : nullptr;
}

void IRunConfigurationAspect::setProjectSettings(ISettingsAspect *settings)
{
    m_projectSettings = settings;
}

void IRunConfigurationAspect::setGlobalSettings(ISettingsAspect *settings)
{
    m_globalSettings = settings;
}

void IRunConfigurationAspect::setUsingGlobalSettings(bool value)
{
    m_useGlobalSettings = value;
}

ISettingsAspect *IRunConfigurationAspect::currentSettings() const
{
   return m_useGlobalSettings ? m_globalSettings : m_projectSettings;
}

void IRunConfigurationAspect::fromMap(const QVariantMap &map)
{
    m_projectSettings->fromMap(map);
    m_useGlobalSettings = map.value(m_id.toString() + QLatin1String(".UseGlobalSettings"), true).toBool();
}

void IRunConfigurationAspect::toMap(QVariantMap &map) const
{
    m_projectSettings->toMap(map);
    map.insert(m_id.toString() + QLatin1String(".UseGlobalSettings"), m_useGlobalSettings);
}

void IRunConfigurationAspect::setRunConfigWidgetCreator(const RunConfigWidgetCreator &runConfigWidgetCreator)
{
    m_runConfigWidgetCreator = runConfigWidgetCreator;
}

IRunConfigurationAspect *IRunConfigurationAspect::clone(RunConfiguration *runConfig) const
{
    IRunConfigurationAspect *other = create(runConfig);
    if (m_projectSettings)
        other->m_projectSettings = m_projectSettings->clone();
    other->m_globalSettings = m_globalSettings;
    other->m_useGlobalSettings = m_useGlobalSettings;
    return other;
}

void IRunConfigurationAspect::resetProjectToGlobalSettings()
{
    QTC_ASSERT(m_globalSettings, return);
    QVariantMap map;
    m_globalSettings->toMap(map);
    m_projectSettings->fromMap(map);
}


/*!
    \class ProjectExplorer::RunConfiguration
    \brief The RunConfiguration class is the base class for a run configuration.

    A run configuration specifies how a target should be run, while a runner
    does the actual running.

    All RunControls and the target hold a shared pointer to the run
    configuration. That is, the lifetime of the run configuration might exceed
    the life of the target.
    The user might still have a RunControl running (or output tab of that RunControl open)
    and yet unloaded the target.

    Also, a run configuration might be already removed from the list of run
    configurations
    for a target, but still be runnable via the output tab.
*/

RunConfiguration::RunConfiguration(Target *target, Core::Id id) :
    ProjectConfiguration(target, id)
{
    Q_ASSERT(target);
    ctor();

    addExtraAspects();
}

RunConfiguration::RunConfiguration(Target *target, RunConfiguration *source) :
    ProjectConfiguration(target, source)
{
    Q_ASSERT(target);
    ctor();
    foreach (IRunConfigurationAspect *aspect, source->m_aspects) {
        IRunConfigurationAspect *clone = aspect->clone(this);
        if (clone)
            m_aspects.append(clone);
    }
}

RunConfiguration::~RunConfiguration()
{
    qDeleteAll(m_aspects);
}

void RunConfiguration::addExtraAspects()
{
    foreach (IRunControlFactory *factory, ExtensionSystem::PluginManager::getObjects<IRunControlFactory>())
        addExtraAspect(factory->createRunConfigurationAspect(this));
}

void RunConfiguration::addExtraAspect(IRunConfigurationAspect *aspect)
{
    if (aspect)
        m_aspects += aspect;
}

void RunConfiguration::ctor()
{
    connect(this, &RunConfiguration::enabledChanged,
            this, &RunConfiguration::requestRunActionsUpdate);

    Utils::MacroExpander *expander = macroExpander();
    expander->setDisplayName(tr("Run Settings"));
    expander->setAccumulating(true);
    expander->registerSubProvider([this]() -> Utils::MacroExpander * {
        BuildConfiguration *bc = target()->activeBuildConfiguration();
        return bc ? bc->macroExpander() : target()->macroExpander();
    });
    expander->registerPrefix("CurrentRun:Env", tr("Variables in the current run environment"),
                             [this](const QString &var) {
        const auto envAspect = extraAspect<EnvironmentAspect>();
        return envAspect ? envAspect->environment().value(var) : QString();
    });
    expander->registerVariable(Constants::VAR_CURRENTRUN_NAME,
            QCoreApplication::translate("ProjectExplorer", "The currently active run configuration's name."),
            [this] { return displayName(); }, false);
}

/*!
    Checks whether a run configuration is enabled.
*/

bool RunConfiguration::isEnabled() const
{
    return true;
}

QString RunConfiguration::disabledReason() const
{
    return QString();
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

QVariantMap RunConfiguration::toMap() const
{
    QVariantMap map = ProjectConfiguration::toMap();

    foreach (IRunConfigurationAspect *aspect, m_aspects)
        aspect->toMap(map);

    return map;
}

Abi RunConfiguration::abi() const
{
    BuildConfiguration *bc = target()->activeBuildConfiguration();
    if (!bc)
        return Abi::hostAbi();
    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit(), Constants::CXX_LANGUAGE_ID);
    if (!tc)
        return Abi::hostAbi();
    return tc->targetAbi();
}

bool RunConfiguration::fromMap(const QVariantMap &map)
{
    foreach (IRunConfigurationAspect *aspect, m_aspects)
        aspect->fromMap(map);

    return ProjectConfiguration::fromMap(map);
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
    Returns extra aspects.

    \sa ProjectExplorer::IRunConfigurationAspect
*/

QList<IRunConfigurationAspect *> RunConfiguration::extraAspects() const
{
    return m_aspects;
}

IRunConfigurationAspect *RunConfiguration::extraAspect(Core::Id id) const
{
    return Utils::findOrDefault(m_aspects, Utils::equal(&IRunConfigurationAspect::id, id));
}

/*!
    \internal

    \class ProjectExplorer::Runnable

    \brief The ProjectExplorer::Runnable class wraps information needed
    to execute a process on a target device.

    A target specific \l RunConfiguration implementation can specify
    what information it considers necessary to execute a process
    on the target. Target specific) \n IRunControlFactory implementation
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
    return Runnable();
}

Utils::OutputFormatter *RunConfiguration::createOutputFormatter() const
{
    return new Utils::OutputFormatter();
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

IRunConfigurationFactory::IRunConfigurationFactory(QObject *parent) :
    QObject(parent)
{
}

RunConfiguration *IRunConfigurationFactory::create(Target *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return nullptr;
    RunConfiguration *rc = doCreate(parent, id);
    if (!rc)
        return nullptr;
    return rc;
}

RunConfiguration *IRunConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return nullptr;
    RunConfiguration *rc = doRestore(parent, map);
    if (!rc->fromMap(map)) {
        delete rc;
        rc = nullptr;
    }
    return rc;
}

IRunConfigurationFactory *IRunConfigurationFactory::find(Target *parent, const QVariantMap &map)
{
    return ExtensionSystem::PluginManager::getObject<IRunConfigurationFactory>(
        [&parent, &map](IRunConfigurationFactory *factory) {
            return factory->canRestore(parent, map);
        });
}

IRunConfigurationFactory *IRunConfigurationFactory::find(Target *parent, RunConfiguration *rc)
{
    return ExtensionSystem::PluginManager::getObject<IRunConfigurationFactory>(
        [&parent, rc](IRunConfigurationFactory *factory) {
            return factory->canClone(parent, rc);
        });
}

QList<IRunConfigurationFactory *> IRunConfigurationFactory::find(Target *parent)
{
    return ExtensionSystem::PluginManager::getObjects<IRunConfigurationFactory>(
        [&parent](IRunConfigurationFactory *factory) {
            return !factory->availableCreationIds(parent).isEmpty();
        });
}

/*!
    \class ProjectExplorer::IRunControlFactory

    \brief The IRunControlFactory class creates RunControl objects matching a
    run configuration.
*/

/*!
    \fn RunConfigWidget *ProjectExplorer::IRunConfigurationAspect::createConfigurationWidget()

    Returns a widget used to configure this runner. Ownership is transferred to
    the caller.

    Returns null if @p \a runConfiguration is not suitable for RunControls from this
    factory, or no user-accessible
    configuration is required.
*/

IRunControlFactory::IRunControlFactory(QObject *parent)
    : QObject(parent)
{
}

using WorkerFactories = std::vector<RunControl::WorkerFactory>;

static WorkerFactories &theWorkerFactories()
{
    static WorkerFactories factories;
    return factories;
}

bool IRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id runMode) const
{
    for (const RunControl::WorkerFactory &factory : theWorkerFactories()) {
        if (factory.runMode == runMode && factory.constraint(runConfiguration))
            return true;
    };
    return false;
}

RunControl *IRunControlFactory::create(RunConfiguration *runConfiguration, Core::Id runMode, QString *)
{
    for (const RunControl::WorkerFactory &factory : theWorkerFactories()) {
        if (factory.runMode == runMode && factory.constraint(runConfiguration)) {
            auto runControl = new RunControl(runConfiguration, runMode);
            factory.producer(runControl);
            return runControl;
        }
    };
    return nullptr;
}

/*!
    Returns an IRunConfigurationAspect to carry options for RunControls this
    factory can create.

    If no extra options are required, it is allowed to return null like the
    default implementation does. This function is intended to be called from the
    RunConfiguration constructor, so passing a RunConfiguration pointer makes
    no sense because that object is under construction at the time.
*/

IRunConfigurationAspect *IRunControlFactory::createRunConfigurationAspect(RunConfiguration *rc)
{
    Q_UNUSED(rc);
    return nullptr;
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
    Initialized, Starting, Running, Stopping, Done, Failed
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
        SN(RunWorkerState::Failed)
    }
    return QLatin1String("<unknown>");
#    undef SN
}

class RunWorkerPrivate : public QObject
{
public:
    RunWorkerPrivate(RunWorker *runWorker, RunControl *runControl);

    bool canStart() const;
    void timerEvent(QTimerEvent *ev) override;

    RunWorker *q;
    RunWorkerState state = RunWorkerState::Initialized;
    RunControl *runControl;
    QList<RunWorker *> dependencies;
    QString id;

    QVariantMap data;
    int startWatchdogInterval = 0;
    int startWatchdogTimerId = -1;
    int stopWatchdogInterval = 0; // 5000;
    int stopWatchdogTimerId = -1;
};

enum class RunControlState
{
    Initialized,      // Default value after creation.
    Starting,         // Actual process/tool starts.
    Running,          // All good and running.
    Stopping,         // initiateStop() was called, stop application/tool
    Stopped,          // all good, but stopped. Can possibly be re-started
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
    }
    return QLatin1String("<unknown>");
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
            device = DeviceKitInformation::device(runConfiguration->target()->kit());
            project = runConfiguration->target()->project();
        }
    }

    ~RunControlPrivate()
    {
        QTC_CHECK(state == RunControlState::Stopped || state == RunControlState::Initialized);
        qDeleteAll(m_workers);
        delete outputFormatter;
    }

    Q_ENUM(RunControlState)

    void checkState(RunControlState expectedState);
    void setState(RunControlState state);

    void debugMessage(const QString &msg);

    void initiateStart();
    void continueStart();
    void initiateStop();
    void continueStop();

    void onWorkerStarted(RunWorker *worker);
    void onWorkerStopped(RunWorker *worker);
    void onWorkerFailed(RunWorker *worker, const QString &msg);

    void showError(const QString &msg);

    static bool isAllowedTransition(RunControlState from, RunControlState to);

    RunControl *q;
    QString displayName;
    Runnable runnable;
    IDevice::ConstPtr device;
    Connection connection;
    Core::Id runMode;
    Utils::Icon icon;
    const QPointer<RunConfiguration> runConfiguration; // Not owned.
    QPointer<Project> project; // Not owned.
    Utils::OutputFormatter *outputFormatter = nullptr;
    std::function<bool(bool*)> promptToStop;
    std::vector<RunControl::WorkerFactory> m_factories;

    // A handle to the actual application process.
    Utils::ProcessHandle applicationProcessHandle;

    RunControlState state = RunControlState::Initialized;
    bool supportsReRunning = true;

    QList<QPointer<RunWorker>> m_workers;

#ifdef Q_OS_OSX
    // This is used to bring apps in the foreground on Mac
    int foregroundCount;
#endif
};

} // Internal

using namespace Internal;

RunControl::RunControl(RunConfiguration *runConfiguration, Core::Id mode) :
    d(new RunControlPrivate(this, runConfiguration, mode))
{
#ifdef WITH_JOURNALD
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
#endif
}

RunControl::~RunControl()
{
#ifdef WITH_JOURNALD
    JournaldWatcher::instance()->unsubscribe(this);
#endif
    disconnect();
    delete d;
    d = nullptr;
}

void RunControl::initiateStart()
{
    emit aboutToStart();
    start();
}

void RunControl::start()
{
    d->initiateStart();
}

void RunControl::initiateStop()
{
    stop();
}

void RunControl::stop()
{
    d->initiateStop();
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

QList<QPointer<RunWorker> > RunControl::workers() const
{
    return d->m_workers;
}

RunWorker *RunControl::createWorker(Core::Id id)
{
    auto keys = theWorkerCreators().keys();
    Q_UNUSED(keys);
    Producer creator = theWorkerCreators().value(id);
    if (creator)
        return creator(this);
    creator = device()->workerCreator(id);
    if (creator)
        return creator(this);
    return nullptr;
}

RunControl::Producer RunControl::producer(RunConfiguration *runConfiguration, Core::Id runMode)
{
    for (const auto &factory : theWorkerFactories()) {
        if (factory.runMode == runMode && factory.constraint(runConfiguration))
            return factory.producer;
    }
    return {};
}

void RunControl::addWorkerFactory(const RunControl::WorkerFactory &workerFactory)
{
    theWorkerFactories().push_back(workerFactory);
}

void RunControlPrivate::initiateStart()
{
    checkState(RunControlState::Initialized);
    setState(RunControlState::Starting);
    debugMessage("Queue: Starting");

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
                case RunWorkerState::Failed:
                    // Should not happen.
                    debugMessage("  " + workerId + " failed before");
                    QTC_CHECK(false);
                    //setState(RunControlState::Stopped);
                    break;
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
    checkState(RunControlState::Running);
    setState(RunControlState::Stopping);
    debugMessage("Queue: Stopping");

    continueStop();
}

void RunControlPrivate::continueStop()
{
    debugMessage("Continue Stopping");
    checkState(RunControlState::Stopping);
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
                    break;
                case RunWorkerState::Starting:
                    worker->d->state = RunWorkerState::Stopping;
                    debugMessage("  " + workerId + " was Starting, queuing stop");
                    QTimer::singleShot(0, worker, &RunWorker::initiateStop);
                    return; // Sic.
                case RunWorkerState::Running:
                    debugMessage("  " + workerId + " was Running, queuing stop");
                    worker->d->state = RunWorkerState::Stopping;
                    QTimer::singleShot(0, worker, &RunWorker::initiateStop);
                    return; // Sic.
                case RunWorkerState::Done:
                    debugMessage("  " + workerId + " was Done. Good.");
                    break;
                case RunWorkerState::Failed:
                    debugMessage("  " + workerId + " was Failed. Good");
                    break;
            }
        } else {
            debugMessage("Found unknown deleted worker");
        }
    }
    debugMessage("All workers stopped. Set runControl to Stopped");
    setState(RunControlState::Stopped);
}

void RunControlPrivate::onWorkerStarted(RunWorker *worker)
{
    worker->d->state = RunWorkerState::Running;

    if (state == RunControlState::Starting) {
        debugMessage(worker->d->id + " start succeeded");
        continueStart();
        return;
    }
    showError(tr("Unexpected run control state %1 when worker %2 started")
              .arg(stateName(state))
              .arg(worker->d->id));
    //setState(RunControlState::Stopped);
}

void RunControlPrivate::onWorkerFailed(RunWorker *worker, const QString &msg)
{
    worker->d->state = RunWorkerState::Failed;

    showError(msg);
    setState(RunControlState::Stopped);
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
    default:
        debugMessage(workerId + " stopped unexpectedly in state"
                     + stateName(worker->d->state));
        worker->d->state = RunWorkerState::Failed;
        break;
    }
    continueStop();
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

const Connection &RunControl::connection() const
{
    return d->connection;
}

void RunControl::setConnection(const Connection &connection)
{
    d->connection = connection;
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

Abi RunControl::abi() const
{
    if (const RunConfiguration *rc = d->runConfiguration.data())
        return rc->abi();
    return Abi();
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

bool RunControl::canReUseOutputPane(const RunControl *other) const
{
    if (other->isRunning())
        return false;

    return d->runnable.canReUseOutputPane(other->d->runnable);
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
    return d->supportsReRunning;
}

void RunControl::setSupportsReRunning(bool reRunningSupported)
{
    d->supportsReRunning = reRunningSupported;
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
        return to == RunControlState::Starting;
    case RunControlState::Starting:
        return to == RunControlState::Running;
    case RunControlState::Running:
        return to == RunControlState::Stopping
            || to == RunControlState::Stopped;
    case RunControlState::Stopping:
        return to == RunControlState::Stopped;
    case RunControlState::Stopped:
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
        foreach (auto worker, m_workers)
            if (worker)
                worker->onFinished();
        //state = RunControlState::Initialized; // Reset for potential re-running.
        emit q->finished();
        break;
    default:
        break;
    }
}

void RunControlPrivate::debugMessage(const QString &msg)
{
    //q->appendMessage(msg + '\n', Utils::DebugFormat);
    qCDebug(statesLog()) << msg;
}

/*!
    Brings the application determined by this RunControl's \c applicationProcessHandle
    to the foreground.

    The default implementation raises the application on Mac, and does
    nothing elsewhere.
*/
void RunControl::bringApplicationToForeground()
{
#ifdef Q_OS_OSX
    d->foregroundCount = 0;
    bringApplicationToForegroundInternal();
#endif
}

void RunControl::bringApplicationToForegroundInternal()
{
#ifdef Q_OS_OSX
    ProcessSerialNumber psn;
    GetProcessForPID(d->applicationProcessHandle.pid(), &psn);
    if (SetFrontProcess(&psn) == procNotFound && d->foregroundCount < 15) {
        // somehow the mac/carbon api says
        // "-600 no eligible process with specified process id"
        // if we call SetFrontProcess too early
        ++d->foregroundCount;
        QTimer::singleShot(200, this, &RunControl::bringApplicationToForegroundInternal);
    }
#endif
}

void RunControl::appendMessage(const QString &msg, Utils::OutputFormat format)
{
    emit appendMessageRequested(this, msg, format);
}

bool Runnable::canReUseOutputPane(const Runnable &other) const
{
    return d ? d->canReUseOutputPane(other.d) : (other.d.get() == 0);
}


// FIXME: Remove once ApplicationLauncher signalling does not depend on device.
static bool isSynchronousLauncher(RunControl *runControl)
{
    RunConfiguration *runConfig = runControl->runConfiguration();
    Target *target = runConfig ? runConfig->target() : nullptr;
    Kit *kit = target ? target->kit() : nullptr;
    Core::Id deviceId = DeviceTypeKitInformation::deviceTypeId(kit);
    return !deviceId.isValid() || deviceId == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}


// SimpleTargetRunner

SimpleTargetRunner::SimpleTargetRunner(RunControl *runControl)
    : RunWorker(runControl)
{
    setDisplayName("SimpleTargetRunner");
    m_runnable = runControl->runnable();
}

void SimpleTargetRunner::start()
{
    m_launcher.disconnect(this);

    QString msg = RunControl::tr("Starting %1...").arg(m_runnable.displayName());
    appendMessage(msg, Utils::NormalMessageFormat);

    if (isSynchronousLauncher(runControl())) {

        connect(&m_launcher, &ApplicationLauncher::appendMessage,
                this, &SimpleTargetRunner::appendMessage);
        connect(&m_launcher, &ApplicationLauncher::processStarted,
                this, &SimpleTargetRunner::onProcessStarted);
        connect(&m_launcher, &ApplicationLauncher::processExited,
                this, &SimpleTargetRunner::onProcessFinished);
        connect(&m_launcher, &ApplicationLauncher::error,
                this, &SimpleTargetRunner::onProcessError);

        QTC_ASSERT(m_runnable.is<StandardRunnable>(), return);
        const QString executable = m_runnable.as<StandardRunnable>().executable;
        if (executable.isEmpty()) {
            reportFailure(RunControl::tr("No executable specified."));
        }  else if (!QFileInfo::exists(executable)) {
            reportFailure(RunControl::tr("Executable %1 does not exist.")
                              .arg(QDir::toNativeSeparators(executable)));
        } else {
            m_launcher.start(m_runnable);
        }

    } else {

        connect(&m_launcher, &ApplicationLauncher::reportError,
                this, [this](const QString &msg) {
                    reportFailure(msg);
                });

        connect(&m_launcher, &ApplicationLauncher::remoteStderr,
                this, [this](const QByteArray &output) {
                    appendMessage(QString::fromUtf8(output), Utils::StdErrFormatSameLine);
                });

        connect(&m_launcher, &ApplicationLauncher::remoteStdout,
                this, [this](const QByteArray &output) {
                    appendMessage(QString::fromUtf8(output), Utils::StdOutFormatSameLine);
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
    runControl()->setApplicationProcessHandle(m_launcher.applicationPID());
    runControl()->bringApplicationToForeground();
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
    reportStopped();
}

void SimpleTargetRunner::onProcessError(QProcess::ProcessError)
{
    QString msg = tr("%1 finished.");
    appendMessage(msg.arg(m_runnable.displayName()), Utils::NormalMessageFormat);
    reportStopped();
}

void SimpleTargetRunner::setRunnable(const Runnable &runnable)
{
    m_runnable = runnable;
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
    for (RunWorker *worker : dependencies) {
        QTC_ASSERT(worker, return true);
        if (worker->d->state != RunWorkerState::Done
                && worker->d->state != RunWorkerState::Running)
            return false;
    }
    return true;
}

void RunWorkerPrivate::timerEvent(QTimerEvent *ev)
{
    if (ev->timerId() == startWatchdogTimerId) {
        q->reportFailure(tr("Worker start timed out"));
        return;
    }
    if (ev->timerId() == stopWatchdogTimerId) {
        q->reportFailure(tr("Worker stop timed out"));
        return;
    }
}

// RunWorker

RunWorker::RunWorker(RunControl *runControl)
    : d(new RunWorkerPrivate(this, runControl))
{
}

RunWorker::~RunWorker()
{
    delete d;
}

void RunWorker::initiateStart()
{
    if (d->startWatchdogInterval != 0)
        d->startWatchdogTimerId = d->startTimer(d->startWatchdogInterval);

    start();
}

void RunWorker::reportStarted()
{
    if (d->startWatchdogInterval != 0)
        d->killTimer(d->startWatchdogTimerId);
    d->runControl->d->onWorkerStarted(this);
    emit started();
}

void RunWorker::initiateStop()
{
    if (d->stopWatchdogInterval != 0)
        d->stopWatchdogTimerId = d->startTimer(d->stopWatchdogInterval);

    d->runControl->d->debugMessage("Initiate stop for " + d->id);
    stop();
}

void RunWorker::reportStopped()
{
    if (d->stopWatchdogInterval != 0)
        d->killTimer(d->stopWatchdogTimerId);
    d->runControl->d->onWorkerStopped(this);
    emit stopped();
}

void RunWorker::reportFailure(const QString &msg)
{
    d->runControl->d->onWorkerFailed(this, msg);
}

void RunWorker::appendMessage(const QString &msg, OutputFormat format)
{
    if (msg.endsWith('\n'))
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

const Connection &RunWorker::connection() const
{
    return d->runControl->connection();
}

Core::Id RunWorker::runMode() const
{
    return d->runControl->runMode();
}

void RunWorker::addDependency(RunWorker *dependency)
{
    d->dependencies.append(dependency);
}

RunControl *RunWorker::runControl() const
{
    return d->runControl;
}

void RunWorker::setId(const QString &id)
{
    d->id = id;
}

void RunWorker::setStartTimeout(int ms)
{
    d->startWatchdogInterval = ms;
}

void RunWorker::setStopTimeout(int ms)
{
    d->stopWatchdogInterval = ms;
}

void RunWorker::reportData(int channel, const QVariant &data)
{
    emit dataReported(channel, data);
}

void RunWorker::recordData(const QString &channel, const QVariant &data)
{
    d->data[channel] = data;
}

QVariant RunWorker::recordedData(const QString &channel) const
{
    return d->data[channel];
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
