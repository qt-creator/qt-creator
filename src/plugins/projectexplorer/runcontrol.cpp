// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "runcontrol.h"

#include "appoutputpane.h"
#include "buildconfiguration.h"
#include "buildsystem.h"
#include "customparser.h"
#include "devicesupport/devicekitaspects.h"
#include "devicesupport/devicemanager.h"
#include "devicesupport/idevice.h"
#include "devicesupport/idevicefactory.h"
#include "devicesupport/sshparameters.h"
#include "devicesupport/sshsettings.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "runconfigurationaspects.h"
#include "target.h"
#include "windebuginterface.h"

#include <coreplugin/icore.h>

#include <solutions/tasking/barrier.h>
#include <solutions/tasking/tasktree.h>
#include <solutions/tasking/tasktreerunner.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/fileinprojectfinder.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/terminalinterface.h>
#include <utils/url.h>
#include <utils/utilsicons.h>

#include <QLoggingCategory>
#include <QTimer>

#if defined (WITH_JOURNALD)
#include "journaldwatcher.h"
#endif

using namespace ProjectExplorer::Internal;
using namespace Tasking;
using namespace Utils;

namespace ProjectExplorer {

static Q_LOGGING_CATEGORY(statesLog, "qtc.projectmanager.states", QtWarningMsg)

static QList<RunWorkerFactory *> g_runWorkerFactories;

static QSet<Id> g_runModes;
static QSet<Id> g_runConfigs;

// RunWorkerFactory

RunWorkerFactory::RunWorkerFactory()
{
    g_runWorkerFactories.append(this);
}

RunWorkerFactory::~RunWorkerFactory()
{
    g_runWorkerFactories.removeOne(this);
}

void RunWorkerFactory::setProducer(const WorkerCreator &producer)
{
    m_producer = producer;
}

void RunWorkerFactory::setRecipeProducer(const RecipeCreator &producer)
{
    m_recipeCreator = producer;
    setProducer([producer](RunControl *runControl) {
        return new RunWorker(runControl, producer(runControl));
    });
}

void RunWorkerFactory::setSupportedRunConfigs(const QList<Id> &runConfigs)
{
    for (Id runConfig : runConfigs)
        g_runConfigs.insert(runConfig); // Debugging only.
    m_supportedRunConfigurations = runConfigs;
}

void RunWorkerFactory::addSupportedRunMode(Id runMode)
{
    g_runModes.insert(runMode); // Debugging only.
    m_supportedRunModes.append(runMode);
}

void RunWorkerFactory::addSupportedRunConfig(Id runConfig)
{
    g_runConfigs.insert(runConfig); // Debugging only.
    m_supportedRunConfigurations.append(runConfig);
}

void RunWorkerFactory::addSupportedDeviceType(Id deviceType)
{
    m_supportedDeviceTypes.append(deviceType);
}

void RunWorkerFactory::addSupportForLocalRunConfigs()
{
    addSupportedRunConfig(ProjectExplorer::Constants::QMAKE_RUNCONFIG_ID);
    addSupportedRunConfig(ProjectExplorer::Constants::QBS_RUNCONFIG_ID);
    addSupportedRunConfig(ProjectExplorer::Constants::CMAKE_RUNCONFIG_ID);
    addSupportedRunConfig(ProjectExplorer::Constants::CUSTOM_EXECUTABLE_RUNCONFIG_ID);
}

void RunWorkerFactory::cloneProduct(Id exitstingStepId)
{
    for (RunWorkerFactory *factory : std::as_const(g_runWorkerFactories)) {
        if (factory->m_id == exitstingStepId) {
            m_producer = factory->m_producer;
            m_recipeCreator = factory->m_recipeCreator;
            // Other bits are intentionally not copied as they are unlikely to be
            // useful in the cloner's context. The cloner can/has to finish the
            // setup on its own.
            return;
        }
    }
    // Existence should be guaranteed by plugin dependencies. In case it fails, bark.
    QTC_CHECK(false);
}

bool RunWorkerFactory::canCreate(Id runMode, Id deviceType, const QString &runConfigId) const
{
    if (!m_supportedRunModes.contains(runMode))
        return false;

    if (!m_supportedRunConfigurations.isEmpty()) {
        // FIXME: That's to be used after mangled ids are gone.
        //if (!m_supportedRunConfigurations.contains(runConfigId)
        // return false;
        bool ok = false;
        for (const Id &id : m_supportedRunConfigurations) {
            if (runConfigId.startsWith(id.toString())) {
                ok = true;
                break;
            }
        }

        if (!ok)
            return false;
    }

    if (!m_supportedDeviceTypes.isEmpty())
        return m_supportedDeviceTypes.contains(deviceType);

    return true;
}

RunWorker *RunWorkerFactory::create(RunControl *runControl) const
{
    QTC_ASSERT(m_producer, return nullptr);
    return m_producer(runControl);
}

Tasking::Group RunWorkerFactory::createRecipe(RunControl *runControl) const
{
    return m_recipeCreator ? m_recipeCreator(runControl) : runControl->noRecipeTask();
}

void RunWorkerFactory::dumpAll()
{
    const QList<Id> devices =
            transform(IDeviceFactory::allDeviceFactories(), &IDeviceFactory::deviceType);

    for (Id runMode : std::as_const(g_runModes)) {
        qDebug() << "";
        for (Id device : devices) {
            for (Id runConfig : std::as_const(g_runConfigs)) {
                const auto check = std::bind(&RunWorkerFactory::canCreate,
                                             std::placeholders::_1,
                                             runMode,
                                             device,
                                             runConfig.toString());
                const auto factory = findOrDefault(g_runWorkerFactories, check);
                qDebug() << "MODE:" << runMode << device << runConfig << factory;
            }
        }
    }
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

enum class RunState
{
    Initialized,      // Default value after creation.
    Starting,         // Actual process/tool starts.
    Running,          // All good and running.
    Canceled,         // initiateStop() was called, stop application/tool
    Done              // all good, but stopped. Can possibly be re-started
};

static QString stateName(RunState s)
{
#    define SN(x) case x: return QLatin1String(#x);
    switch (s) {
        SN(RunState::Initialized)
        SN(RunState::Starting)
        SN(RunState::Running)
        SN(RunState::Canceled)
        SN(RunState::Done)
    }
    return QString("<unknown: %1>").arg(int(s));
#    undef SN
}

class RunWorkerPrivate : public QObject
{
public:
    RunWorkerPrivate(RunWorker *runWorker, RunControl *runControl, const Group &recipe);

    bool canStart() const;
    bool canStop() const;

    RunWorker *q;
    RunState state = RunState::Initialized;
    const QPointer<RunControl> runControl;
    TaskTreeRunner taskTreeRunner;
    const Group recipe;
    QList<RunWorker *> startDependencies;
    QList<RunWorker *> stopDependencies;
};

class RunControlPrivateData
{
public:
    QString displayName;
    ProcessRunData runnable;
    QVariantHash extraData;
    IDevice::ConstPtr device;
    Icon icon;
    const MacroExpander *macroExpander = nullptr;
    AspectContainerData aspectData;
    QString buildKey;
    QMap<Id, Store> settingsData;
    Id runConfigId;
    BuildTargetInfo buildTargetInfo;
    FilePath buildDirectory;
    Environment buildEnvironment;
    Kit *kit = nullptr; // Not owned.
    QPointer<BuildConfiguration> buildConfiguration; // Not owned.
    QPointer<Project> project; // Not owned.
    std::function<bool(bool*)> promptToStop;
    std::vector<RunWorkerFactory> m_factories;

    // A handle to the actual application process.
    ProcessHandle applicationProcessHandle;

    QList<QPointer<RunWorker>> m_workers;
    RunState state = RunState::Initialized;
    bool printEnvironment = false;
    std::optional<Group> m_runRecipe;

    bool useDebugChannel = false;
    bool useQmlChannel = false;
    bool usePerfChannel = false;
    bool useWorkerChannel = false;
    QUrl debugChannel;
    QUrl qmlChannel;
    QUrl perfChannel;
    QUrl workerChannel;
    Utils::ProcessHandle m_attachPid;
};

class RunControlPrivate : public QObject, public RunControlPrivateData
{
    Q_OBJECT

public:
    RunControlPrivate(RunControl *parent, Id mode)
        : q(parent), runMode(mode)
    {
        icon = Icons::RUN_SMALL_TOOLBAR;
        connect(&m_taskTreeRunner, &TaskTreeRunner::aboutToStart, q, &RunControl::started);
        connect(&m_taskTreeRunner, &TaskTreeRunner::done, this, &RunControlPrivate::emitStopped);
    }

    ~RunControlPrivate() override
    {
        QTC_CHECK(state == RunState::Done || state == RunState::Initialized);
        disconnect();
        q = nullptr;
        qDeleteAll(m_workers);
        m_workers.clear();
    }

    void copyData(RunControlPrivateData *other) { RunControlPrivateData::operator=(*other); }

    void checkState(RunState expectedState);
    void setState(RunState state);

    void debugMessage(const QString &msg) const;

    void initiateStart();
    void startPortsGathererIfNeededAndContinueStart();
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

    static bool isAllowedTransition(RunState from, RunState to);
    bool isUsingTaskTree() const { return bool(m_runRecipe); }
    void startTaskTree();
    void emitStopped();

    bool isPortsGatherer() const
    { return useDebugChannel || useQmlChannel || usePerfChannel || useWorkerChannel; }
    QUrl getNextChannel(PortList *portList, const QList<Port> &usedPorts);

    RunControl *q;
    Id runMode;
    TaskTreeRunner m_taskTreeRunner;
    TaskTreeRunner m_portsGathererRunner;
};

} // Internal

using namespace Internal;

RunControl::RunControl(Id mode) :
    d(std::make_unique<RunControlPrivate>(this,  mode))
{
}

void RunControl::copyDataFromRunControl(RunControl *runControl)
{
    QTC_ASSERT(runControl, return);
    d->copyData(runControl->d.get());
}

Group RunControl::noRecipeTask()
{
    const auto onSync = [this] {
        postMessage(Tr::tr("No recipe producer."), ErrorMessageFormat);
        return false;
    };

    return { Sync(onSync) };
}

void RunControl::start()
{
    ProjectExplorerPlugin::startRunControl(this);
}

void RunControl::copyDataFromRunConfiguration(RunConfiguration *runConfig)
{
    QTC_ASSERT(runConfig, return);
    d->runConfigId = runConfig->id();
    d->runnable = runConfig->runnable();
    d->extraData = runConfig->extraData();
    d->displayName = runConfig->expandedDisplayName();
    d->buildKey = runConfig->buildKey();
    d->settingsData = runConfig->settingsData();
    d->aspectData = runConfig->aspectData();
    d->printEnvironment = runConfig->isPrintEnvironmentEnabled();

    setBuildConfiguration(runConfig->buildConfiguration());

    d->macroExpander = runConfig->macroExpander();
}

void RunControl::setBuildConfiguration(BuildConfiguration *bc)
{
    QTC_ASSERT(bc, return);
    QTC_CHECK(!d->buildConfiguration);
    d->buildConfiguration = bc;

    if (!d->buildKey.isEmpty())
        d->buildTargetInfo = bc->buildSystem()->buildTarget(d->buildKey);

    d->buildDirectory = bc->buildDirectory();
    d->buildEnvironment = bc->environment();

    setKit(bc->kit());
    d->macroExpander = bc->macroExpander();
    d->project = bc->project();
}

void RunControl::setKit(Kit *kit)
{
    QTC_ASSERT(kit, return);
    QTC_CHECK(!d->kit);
    d->kit = kit;
    d->macroExpander = kit->macroExpander();

    if (!d->runnable.command.isEmpty()) {
        setDevice(DeviceManager::deviceForPath(d->runnable.command.executable()));
        QTC_ASSERT(device(), setDevice(RunDeviceKitAspect::device(kit)));
    } else {
        setDevice(RunDeviceKitAspect::device(kit));
    }
}

void RunControl::setDevice(const IDevice::ConstPtr &device)
{
    QTC_CHECK(!d->device);
    d->device = device;
#ifdef WITH_JOURNALD
    if (device && device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
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
            appendMessage(message, OutputFormat::LogMessageFormat);
        });
    }
#endif
}

RunControl::~RunControl()
{
#ifdef WITH_JOURNALD
    JournaldWatcher::instance()->unsubscribe(this);
#endif
}

void RunControl::setRunRecipe(const Group &group)
{
    d->m_runRecipe = group;
}

void RunControl::initiateStart()
{
    emit aboutToStart();
    if (d->isUsingTaskTree())
        d->startTaskTree();
    else
        d->initiateStart();
}

void RunControl::initiateReStart()
{
    emit aboutToStart();
    if (d->isUsingTaskTree())
        d->startTaskTree();
    else
        d->initiateReStart();
}

void RunControl::initiateStop()
{
    if (d->isUsingTaskTree()) {
        d->m_taskTreeRunner.reset();
        d->emitStopped();
    } else {
        d->initiateStop();
    }
}

void RunControl::forceStop()
{
    if (d->isUsingTaskTree()) {
        d->m_taskTreeRunner.reset();
        d->emitStopped();
    } else {
        d->forceStop();
    }
}

RunWorker *RunControl::createWorker(Id runMode)
{
    const Id deviceType = RunDeviceTypeKitAspect::deviceTypeId(d->kit);
    for (RunWorkerFactory *factory : std::as_const(g_runWorkerFactories)) {
        if (factory->canCreate(runMode, deviceType, d->runConfigId.toString()))
            return factory->create(this);
    }
    return nullptr;
}

Group RunControl::createRecipe(Id runMode)
{
    const Id deviceType = RunDeviceTypeKitAspect::deviceTypeId(d->kit);
    for (RunWorkerFactory *factory : std::as_const(g_runWorkerFactories)) {
        if (factory->canCreate(runMode, deviceType, d->runConfigId.toString()))
            return factory->createRecipe(this);
    }
    return noRecipeTask();
}

bool RunControl::createMainWorker()
{
    const QList<RunWorkerFactory *> candidates
        = filtered(g_runWorkerFactories, [this](RunWorkerFactory *factory) {
              return factory->canCreate(d->runMode,
                                        RunDeviceTypeKitAspect::deviceTypeId(d->kit),
                                        d->runConfigId.toString());
          });

    // There might be combinations that cannot run. But that should have been checked
    // with canRun below.
    QTC_ASSERT(!candidates.empty(), return false);

    // There should be at most one top-level producer feeling responsible per combination.
    // Breaking a tie should be done by tightening the restrictions on one of them.
    QTC_CHECK(candidates.size() == 1);
    return candidates.front()->create(this) != nullptr;
}

bool RunControl::canRun(Id runMode, Id deviceType, Utils::Id runConfigId)
{
    for (const RunWorkerFactory *factory : std::as_const(g_runWorkerFactories)) {
        if (factory->canCreate(runMode, deviceType, runConfigId.toString()))
            return true;
    }
    return false;
}

void RunControl::postMessage(const QString &msg, Utils::OutputFormat format, bool appendNewLine)
{
    emit appendMessage((appendNewLine && !msg.endsWith('\n')) ? msg + '\n': msg, format);
}

void RunControlPrivate::initiateStart()
{
    checkState(RunState::Initialized);
    setState(RunState::Starting);
    debugMessage("Queue: Starting");

    startPortsGathererIfNeededAndContinueStart();
}

void RunControlPrivate::initiateReStart()
{
    checkState(RunState::Done);

    // Re-set worked on re-runs.
    for (RunWorker *worker : std::as_const(m_workers)) {
        if (worker->d->state == RunState::Done)
            worker->d->state = RunState::Initialized;
    }

    setState(RunState::Starting);
    debugMessage("Queue: ReStarting");

    startPortsGathererIfNeededAndContinueStart();
}

void RunControlPrivate::startPortsGathererIfNeededAndContinueStart()
{
    if (!isPortsGatherer()) {
        continueStart();
        return;
    }

    QTC_ASSERT(device, initiateStop(); return);

    const Storage<PortsOutputData> portsStorage;

    const auto onDone = [this, portsStorage] {
        const auto ports = *portsStorage;
        if (!ports) {
            onWorkerFailed(nullptr, ports.error());
            return;
        }
        PortList portList = device->freePorts();
        const QList<Port> usedPorts = *ports;
        q->appendMessage(Tr::tr("Found %n free ports.", nullptr, portList.count()) + '\n',
                         NormalMessageFormat);
        if (useDebugChannel)
            debugChannel = getNextChannel(&portList, usedPorts);
        if (useQmlChannel)
            qmlChannel = getNextChannel(&portList, usedPorts);
        if (usePerfChannel)
            perfChannel = getNextChannel(&portList, usedPorts);
        if (useWorkerChannel)
            workerChannel = getNextChannel(&portList, usedPorts);

        continueStart();
    };

    const Group recipe {
        portsStorage,
        device->portsGatheringRecipe(portsStorage),
        onGroupDone(onDone)
    };

    q->appendMessage(Tr::tr("Checking available ports...") + '\n', NormalMessageFormat);
    m_portsGathererRunner.start(recipe);
}

QUrl RunControlPrivate::getNextChannel(PortList *portList, const QList<Port> &usedPorts)
{
    QUrl result;
    if (q->device()->sshForwardDebugServerPort()) {
        result.setScheme(urlTcpScheme());
        result.setHost("localhost");
    } else {
        result = q->device()->toolControlChannel(IDevice::ControlChannelHint());
    }
    result.setPort(portList->getNextFreePort(usedPorts).number());
    return result;
}

void RunControl::requestDebugChannel()
{
    d->useDebugChannel = true;
}

bool RunControl::usesDebugChannel() const
{
    return d->useDebugChannel;
}

QUrl RunControl::debugChannel() const
{
    return d->debugChannel;
}

void RunControl::setDebugChannel(const QUrl &channel)
{
    d->debugChannel = channel;
}

void RunControl::requestQmlChannel()
{
    d->useQmlChannel = true;
}

bool RunControl::usesQmlChannel() const
{
    return d->useQmlChannel;
}

QUrl RunControl::qmlChannel() const
{
    return d->qmlChannel;
}

void RunControl::setQmlChannel(const QUrl &channel)
{
    d->qmlChannel = channel;
}

void RunControl::requestPerfChannel()
{
    d->usePerfChannel = true;
}

bool RunControl::usesPerfChannel() const
{
    return d->usePerfChannel;
}

QUrl RunControl::perfChannel() const
{
    return d->perfChannel;
}

void RunControl::requestWorkerChannel()
{
    d->useWorkerChannel = true;
}

QUrl RunControl::workerChannel() const
{
    return d->workerChannel;
}

void RunControl::setAttachPid(ProcessHandle pid)
{
    d->m_attachPid = pid;
}

ProcessHandle RunControl::attachPid() const
{
    return d->m_attachPid;
}

void RunControl::showOutputPane()
{
    appOutputPane().showOutputPaneForRunControl(this);
}

void RunControlPrivate::continueStart()
{
    checkState(RunState::Starting);
    bool allDone = true;
    debugMessage("Looking for next worker");
    for (RunWorker *worker : std::as_const(m_workers)) {
        if (worker) {
            debugMessage("  Examining worker ");
            switch (worker->d->state) {
                case RunState::Initialized:
                    debugMessage("  is not done yet.");
                    if (worker->d->canStart()) {
                        debugMessage("Starting");
                        worker->d->state = RunState::Starting;
                        QTimer::singleShot(0, worker, &RunWorker::initiateStart);
                        return;
                    }
                    allDone = false;
                    debugMessage("  cannot start.");
                    break;
                case RunState::Starting:
                    debugMessage("  currently starting");
                    allDone = false;
                    break;
                case RunState::Running:
                    debugMessage("  currently running");
                    break;
                case RunState::Canceled:
                    debugMessage("  currently stopping");
                    continue;
                case RunState::Done:
                    debugMessage("  was done before");
                    break;
            }
        } else {
            debugMessage("Found unknown deleted worker while starting");
        }
    }
    if (allDone)
        setState(RunState::Running);
}

void RunControlPrivate::initiateStop()
{
    if (state == RunState::Initialized)
        qDebug() << "Unexpected initiateStop() in state" << stateName(state);

    setState(RunState::Canceled);
    debugMessage("Queue: Stopping for all workers");

    continueStopOrFinish();
}

void RunControlPrivate::continueStopOrFinish()
{
    bool allDone = true;

    auto queueStop = [this](RunWorker *worker, const QString &message) {
        if (worker->d->canStop()) {
            debugMessage(message);
            worker->d->state = RunState::Canceled;
            QTimer::singleShot(0, worker, &RunWorker::initiateStop);
        } else {
            debugMessage("  is waiting for dependent workers to stop");
        }
    };

    for (RunWorker *worker : std::as_const(m_workers)) {
        if (worker) {
            debugMessage("  Examining worker");
            switch (worker->d->state) {
                case RunState::Initialized:
                    debugMessage("  was Initialized, setting to Done");
                    worker->d->state = RunState::Done;
                    break;
                case RunState::Canceled:
                    debugMessage("  was already Stopping. Keeping it that way");
                    allDone = false;
                    break;
                case RunState::Starting:
                    queueStop(worker, "  was Starting, queuing stop");
                    allDone = false;
                    break;
                case RunState::Running:
                    queueStop(worker, "  was Running, queuing stop");
                    allDone = false;
                    break;
                case RunState::Done:
                    debugMessage("  was Done. Good.");
                    break;
            }
        } else {
            debugMessage("Found unknown deleted worker");
        }
    }

    if (allDone) {
        debugMessage("All Stopped");
        setState(RunState::Done);
    } else {
        debugMessage("Not all workers Stopped. Waiting...");
    }
}

void RunControlPrivate::forceStop()
{
    if (state == RunState::Done) {
        debugMessage("Was finished, too late to force Stop");
        return;
    }
    for (RunWorker *worker : std::as_const(m_workers)) {
        if (worker) {
            debugMessage("  Examining worker");
            switch (worker->d->state) {
                case RunState::Initialized:
                    debugMessage("  was Initialized, setting to Done");
                    break;
                case RunState::Canceled:
                    debugMessage("  was already Stopping. Set it forcefully to Done.");
                    break;
                case RunState::Starting:
                    debugMessage("  was Starting. Set it forcefully to Done.");
                    break;
                case RunState::Running:
                    debugMessage("  was Running. Set it forcefully to Done.");
                    break;
                case RunState::Done:
                    debugMessage("  was Done. Good.");
                    break;
            }
            worker->d->state = RunState::Done;
        } else {
            debugMessage("Found unknown deleted worker");
        }
    }

    setState(RunState::Done);
    debugMessage("All Stopped");
}

void RunControlPrivate::onWorkerStarted(RunWorker *worker)
{
    worker->d->state = RunState::Running;

    if (state == RunState::Starting) {
        debugMessage("start succeeded");
        continueStart();
        return;
    }
    showError(Tr::tr("Unexpected run control state %1 when worker started.").arg(stateName(state)));
}

void RunControlPrivate::onWorkerFailed(RunWorker *worker, const QString &msg)
{
    if (worker)
        worker->d->state = RunState::Done;

    showError(msg);
    switch (state) {
    case RunState::Initialized:
        // FIXME 1: We don't have an output pane yet, so use some other mechanism for now.
        // FIXME 2: Translation...
        QMessageBox::critical(Core::ICore::dialogParent(), Tr::tr("Error"),
             QString("Failure during startup. Aborting.") + "<p>" + msg);
        continueStopOrFinish();
        break;
    case RunState::Starting:
    case RunState::Running:
        initiateStop();
        break;
    case RunState::Canceled:
        continueStopOrFinish();
        break;
    case RunState::Done:
        QTC_CHECK(false); // Should not happen.
        continueStopOrFinish();
        break;
    }
}

void RunControlPrivate::onWorkerStopped(RunWorker *worker)
{
    switch (worker->d->state) {
    case RunState::Running:
        // That was a spontaneous stop.
        worker->d->state = RunState::Done;
        debugMessage("stopped spontaneously.");
        break;
    case RunState::Canceled:
        worker->d->state = RunState::Done;
        debugMessage("stopped expectedly.");
        break;
    case RunState::Done:
        worker->d->state = RunState::Done;
        debugMessage("stopped twice. Huh? But harmless.");
        return; // Sic!
    default:
        debugMessage("stopped unexpectedly in state" + stateName(worker->d->state));
        worker->d->state = RunState::Done;
        break;
    }

    if (state == RunState::Canceled) {
        continueStopOrFinish();
        return;
    }

    for (RunWorker *dependent : std::as_const(worker->d->stopDependencies)) {
        switch (dependent->d->state) {
        case RunState::Done:
            break;
        case RunState::Initialized:
            dependent->d->state = RunState::Done;
            break;
        default:
            debugMessage("Killing dependent worker");
            dependent->d->state = RunState::Canceled;
            QTimer::singleShot(0, dependent, &RunWorker::initiateStop);
            break;
        }
    }

    debugMessage("Checking whether all stopped");
    bool allDone = true;
    for (RunWorker *worker : std::as_const(m_workers)) {
        if (worker) {
            debugMessage("  Examining worker ");
            switch (worker->d->state) {
                case RunState::Initialized:
                    debugMessage("  was Initialized.");
                    break;
                case RunState::Starting:
                    debugMessage("  was Starting, waiting for its response");
                    allDone = false;
                    break;
                case RunState::Running:
                    debugMessage("  was Running, waiting for its response");
                    allDone = false;
                    break;
                case RunState::Canceled:
                    debugMessage("  was already Stopping. Keeping it that way");
                    allDone = false;
                    break;
                case RunState::Done:
                    debugMessage("  was Done. Good.");
                    break;
            }
        } else {
            debugMessage("Found unknown deleted worker");
        }
    }

    if (allDone) {
        if (state == RunState::Done) {
            debugMessage("All workers stopped, but runControl was already stopped.");
        } else {
            debugMessage("All workers stopped. Set runControl to Stopped");
            setState(RunState::Done);
        }
    } else {
        debugMessage("Not all workers stopped. Waiting...");
    }
}

void RunControlPrivate::showError(const QString &msg)
{
    if (q && !msg.isEmpty())
        q->postMessage(msg + '\n', ErrorMessageFormat);
}

void RunControl::setupFormatter(OutputFormatter *formatter) const
{
    QList<Utils::OutputLineParser *> parsers = createOutputParsers(target());
    if (const auto customParsersAspect = aspectData<CustomParsersAspect>()) {
        for (const Id id : std::as_const(customParsersAspect->parsers)) {
            if (auto parser = createCustomParserFromId(id))
                parsers << parser;
        }
    }
    formatter->setLineParsers(parsers);
    if (project()) {
        Utils::FileInProjectFinder fileFinder;
        fileFinder.setProjectDirectory(project()->projectDirectory());
        fileFinder.setProjectFiles(project()->files(Project::AllFiles));
        formatter->setFileFinder(fileFinder);
    }
}

Utils::Id RunControl::runMode() const
{
    return d->runMode;
}

bool RunControl::isPrintEnvironmentEnabled() const
{
    return d->printEnvironment;
}

const ProcessRunData &RunControl::runnable() const
{
    return d->runnable;
}

const CommandLine &RunControl::commandLine() const
{
    return d->runnable.command;
}

void RunControl::setCommandLine(const CommandLine &command)
{
    d->runnable.command = command;
}

const FilePath &RunControl::workingDirectory() const
{
    return d->runnable.workingDirectory;
}

void RunControl::setWorkingDirectory(const FilePath &workingDirectory)
{
    d->runnable.workingDirectory = workingDirectory;
}

const Environment &RunControl::environment() const
{
    return d->runnable.environment;
}

void RunControl::setEnvironment(const Environment &environment)
{
    d->runnable.environment = environment;
}

const QVariantHash &RunControl::extraData() const
{
    return d->extraData;
}

void RunControl::setExtraData(const QVariantHash &extraData)
{
    d->extraData = extraData;
}

QString RunControl::displayName() const
{
    if (d->displayName.isEmpty())
        return d->runnable.command.executable().toUserOutput();
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

BuildConfiguration *RunControl::buildConfiguration() const
{
    return d->buildConfiguration;
}

Target *RunControl::target() const
{
    return buildConfiguration() ? buildConfiguration()->target() : nullptr;
}

Project *RunControl::project() const
{
    return d->project;
}

Kit *RunControl::kit() const
{
    return d->kit;
}

const MacroExpander *RunControl::macroExpander() const
{
    return d->macroExpander;
}

const BaseAspect::Data *RunControl::aspectData(Id instanceId) const
{
    return d->aspectData.aspect(instanceId);
}

const BaseAspect::Data *RunControl::aspectData(BaseAspect::Data::ClassId classId) const
{
    return d->aspectData.aspect(classId);
}

Store RunControl::settingsData(Id id) const
{
    return d->settingsData.value(id);
}

QString RunControl::buildKey() const
{
    return d->buildKey;
}

FilePath RunControl::buildDirectory() const
{
    return d->buildDirectory;
}

Environment RunControl::buildEnvironment() const
{
    return d->buildEnvironment;
}

FilePath RunControl::targetFilePath() const
{
    return d->buildTargetInfo.targetFilePath;
}

FilePath RunControl::projectFilePath() const
{
    return d->buildTargetInfo.projectFilePath;
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

    const QString msg = Tr::tr("<html><head/><body><center><i>%1</i> is still running.<center/>"
                           "<center>Force it to quit?</center></body></html>").arg(displayName());
    return showPromptToStopDialog(Tr::tr("Application Still Running"), msg,
                                  Tr::tr("Force &Quit"), Tr::tr("&Keep Running"),
                                  optionalPrompt);
}

void RunControl::setPromptToStop(const std::function<bool (bool *)> &promptToStop)
{
    d->promptToStop = promptToStop;
}

void RunControlPrivate::startTaskTree()
{
    m_taskTreeRunner.start(*m_runRecipe);
}

void RunControlPrivate::emitStopped()
{
    if (!q)
        return;

    q->setApplicationProcessHandle(Utils::ProcessHandle());
    emit q->stopped();
}

bool RunControl::isRunning() const
{
    if (d->isUsingTaskTree())
        return d->m_taskTreeRunner.isRunning();
    return d->state == RunState::Running;
}

bool RunControl::isStopped() const
{
    if (d->isUsingTaskTree())
        return !d->m_taskTreeRunner.isRunning();
    return d->state == RunState::Done;
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
    QMap<QMessageBox::StandardButton, QString> buttonTexts;
    if (!stopButtonText.isEmpty())
        buttonTexts[QMessageBox::Yes] = stopButtonText;
    if (!cancelButtonText.isEmpty())
        buttonTexts[QMessageBox::Cancel] = cancelButtonText;

    CheckableDecider decider;
    if (prompt)
        decider = CheckableDecider(prompt);

    auto selected = CheckableMessageBox::question(title,
                                                  text,
                                                  decider,
                                                  QMessageBox::Yes | QMessageBox::Cancel,
                                                  QMessageBox::Yes,
                                                  QMessageBox::Yes,
                                                  buttonTexts);

    return selected == QMessageBox::Yes;
}

void RunControl::provideAskPassEntry(Environment &env)
{
    const FilePath askpass = SshSettings::askpassFilePath();
    if (askpass.exists())
        env.setFallback("SUDO_ASKPASS", askpass.toUserOutput());
}

bool RunControlPrivate::isAllowedTransition(RunState from, RunState to)
{
    switch (from) {
    case RunState::Initialized:
        return to == RunState::Starting;
    case RunState::Starting:
        return to == RunState::Running || to == RunState::Canceled;
    case RunState::Running:
        return to == RunState::Canceled || to == RunState::Done;
    case RunState::Canceled:
        return to == RunState::Done;
    case RunState::Done:
        return to != RunState::Initialized;
    }
    return false;
}

void RunControlPrivate::checkState(RunState expectedState)
{
    if (state != expectedState)
        qDebug() << "Unexpected run control state " << stateName(expectedState)
                 << " have: " << stateName(state);
}

void RunControlPrivate::setState(RunState newState)
{
    if (!isAllowedTransition(state, newState))
        qDebug() << "Invalid run control state transition from" << stateName(state)
                 << "to" << stateName(newState);

    state = newState;

    debugMessage("Entering state " + stateName(newState));

    // Extra reporting.
    switch (state) {
    case RunState::Running:
        if (q)
            emit q->started();
        break;
    case RunState::Done:
        emitStopped();
        break;
    default:
        break;
    }
}

void RunControlPrivate::debugMessage(const QString &msg) const
{
    qCDebug(statesLog()) << msg;
}

namespace Internal {

} // Internal

ProcessTask processTask(RunControl *runControl,
                        const std::function<SetupResult(Process &)> &startModifier,
                        bool suppressDefaultStdOutHandling)
{
    const auto onSetup = [runControl, startModifier, suppressDefaultStdOutHandling](Process &process) {
        process.setProcessChannelMode(appOutputPane().settings().mergeChannels
                                          ? QProcess::MergedChannels : QProcess::SeparateChannels);
        process.setCommand(runControl->commandLine());
        process.setWorkingDirectory(runControl->workingDirectory());
        process.setEnvironment(runControl->environment());

        if (startModifier) {
            const SetupResult result = startModifier(process);
            if (result != SetupResult::Continue)
                return result;
        }

        const CommandLine command = process.commandLine();
        const bool isDesktop = command.executable().isLocal();
        if (isDesktop && command.isEmpty()) {
            runControl->postMessage(Tr::tr("No executable specified."), ErrorMessageFormat);
            return SetupResult::StopWithError;
        }

        bool useTerminal = false;
        if (auto terminalAspect = runControl->aspectData<TerminalAspect>())
            useTerminal = terminalAspect->useTerminal;

        const Environment environment = process.environment();
        process.setTerminalMode(useTerminal ? Utils::TerminalMode::Run : Utils::TerminalMode::Off);
        process.setReaperTimeout(
            std::chrono::seconds(projectExplorerSettings().reaperTimeoutInSeconds));

        runControl->postMessage(Tr::tr("Starting %1...").arg(command.displayName()), NormalMessageFormat);
        if (runControl->isPrintEnvironmentEnabled()) {
            runControl->postMessage(Tr::tr("Environment:"), NormalMessageFormat);
            environment.forEachEntry([runControl](const QString &key, const QString &value, bool enabled) {
                if (enabled)
                    runControl->postMessage(key + '=' + value, StdOutFormat);
            });
            runControl->postMessage({}, StdOutFormat);
        }

        CommandLine cmdLine = process.commandLine();
        Environment env = process.environment();

        if (cmdLine.executable().isLocal()) {
            // Running locally.
            bool runAsRoot = false;
            if (auto runAsRootAspect = runControl->aspectData<RunAsRootAspect>())
                runAsRoot = runAsRootAspect->value;

            if (runAsRoot)
                RunControl::provideAskPassEntry(env);

            WinDebugInterface::startIfNeeded();

            if (HostOsInfo::isMacHost()) {
                CommandLine disclaim(Core::ICore::libexecPath("disclaim"));
                disclaim.addCommandLineAsArgs(cmdLine);
                cmdLine = disclaim;
            }

            process.setRunAsRoot(runAsRoot);
        }

        const IDevice::ConstPtr device = DeviceManager::deviceForPath(cmdLine.executable());
        if (device && !device->allowEmptyCommand() && cmdLine.isEmpty()) {
            runControl->postMessage(Tr::tr("Cannot run: No command given."), NormalMessageFormat);
            return SetupResult::StopWithError;
        }

        QVariantHash extraData = runControl->extraData();
        QString shellName = runControl->displayName();

        if (runControl->buildConfiguration()) {
            if (BuildConfiguration *buildConfig = runControl->buildConfiguration())
                shellName += " - " + buildConfig->displayName();
        }

        extraData[TERMINAL_SHELL_NAME] = shellName;

        process.setCommand(cmdLine);
        process.setEnvironment(env);
        process.setExtraData(extraData);
        process.setForceDefaultErrorModeOnWindows(true);

        QObject::connect(&process, &Process::started, runStorage().activeStorage(),
                         [runControl, process = &process] {
            const bool isDesktop = process->commandLine().executable().isLocal();
            if (isDesktop) {
                // Console processes only know their pid after being started
                ProcessHandle pid{process->processId()};
                runControl->setApplicationProcessHandle(pid);
                pid.activate();
            }
        });
        QObject::connect(&process, &Process::readyReadStandardError, runControl, [runControl, process = &process] {
            runControl->postMessage(process->readAllStandardError(), StdErrFormat, false);
        });
        QObject::connect(&process, &Process::readyReadStandardOutput, runControl, [runControl, suppressDefaultStdOutHandling, process = &process] {
            if (suppressDefaultStdOutHandling)
                emit runControl->stdOutData(process->readAllRawStandardOutput());
            else
                runControl->postMessage(process->readAllStandardOutput(), StdOutFormat, false);
        });
        QObject::connect(&process, &Process::stoppingForcefully, runControl, [runControl] {
            runControl->postMessage(Tr::tr("Stopping process forcefully ...."), NormalMessageFormat);
        });

        if (WinDebugInterface::instance()) {
            QObject::connect(WinDebugInterface::instance(), &WinDebugInterface::cannotRetrieveDebugOutput,
                             &process, [runControl, process = &process] {
                QObject::disconnect(WinDebugInterface::instance(), nullptr, process, nullptr);
                runControl->postMessage(Tr::tr("Cannot retrieve debugging output.")
                                            + QLatin1Char('\n'), ErrorMessageFormat);
            });

            QObject::connect(WinDebugInterface::instance(), &WinDebugInterface::debugOutput,
                             &process, [runControl, process = &process](qint64 pid, const QList<QString> &messages) {
                if (process->processId() != pid)
                    return;
                for (const QString &message : messages)
                    runControl->postMessage(message, DebugFormat);
            });
        }
        QObject::connect(runStorage().activeStorage(), &RunInterface::canceled, &process,
                         [runControl, process = &process, isLocal = cmdLine.executable().isLocal()] {
            runControl->postMessage(Tr::tr("Requesting process to stop ...."), NormalMessageFormat);
            process->stop();
            QTimer::singleShot(2 * std::chrono::seconds(projectExplorerSettings().reaperTimeoutInSeconds),
                               process, [runControl, process, isLocal] {
                runControl->postMessage(Tr::tr("Process unexpectedly did not finish."), ErrorMessageFormat);
                if (!isLocal)
                    runControl->postMessage(Tr::tr("Connectivity lost?"), ErrorMessageFormat);
                process->kill();
                emit process->done();
            });
        });
        return SetupResult::Continue;
    };

    const auto onDone = [runControl](const Process &process) {
        runControl->postMessage(process.exitMessage(), NormalMessageFormat);
    };

    return ProcessTask(onSetup, onDone);
}

RunWorkerPrivate::RunWorkerPrivate(RunWorker *runWorker, RunControl *runControl, const Group &recipe)
    : q(runWorker)
    , runControl(runControl)
    , recipe(recipe)
{
    runControl->d->m_workers.append(runWorker);
}

bool RunWorkerPrivate::canStart() const
{
    if (state != RunState::Initialized)
        return false;
    for (RunWorker *worker : startDependencies) {
        QTC_ASSERT(worker, continue);
        if (worker->d->state != RunState::Done && worker->d->state != RunState::Running)
            return false;
    }
    return true;
}

bool RunWorkerPrivate::canStop() const
{
    if (state != RunState::Starting && state != RunState::Running)
        return false;
    for (RunWorker *worker : stopDependencies) {
        QTC_ASSERT(worker, continue);
        if (worker->d->state != RunState::Done)
            return false;
    }
    return true;
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

RunWorker::RunWorker(RunControl *runControl, const Group &recipe)
    : d(std::make_unique<RunWorkerPrivate>(this, runControl, recipe))
{ }

RunWorker::~RunWorker() = default;

/*!
 * This function is called by the RunControl once all dependencies
 * are fulfilled.
 */
void RunWorker::initiateStart()
{
    d->runControl->d->debugMessage("Initiate start");
    QTC_CHECK(!d->taskTreeRunner.isRunning());

    const auto onSetup = [this] {
        connect(this, &RunWorker::canceled,
                runStorage().activeStorage(), &RunInterface::canceled);
        connect(runStorage().activeStorage(), &RunInterface::started, this, [this] {
            d->runControl->d->onWorkerStarted(this);
            emit started();
        });
    };

    const Group recipe {
        runStorage(),
        onGroupSetup(onSetup),
        d->recipe
    };

    d->taskTreeRunner.start(recipe, {}, [this](DoneWith result) {
        if (result == DoneWith::Success) {
            QTC_ASSERT(d && d->runControl && d->runControl->d, return);
            d->runControl->d->onWorkerStopped(this);
            emit stopped();
        } else {
            d->runControl->d->onWorkerFailed(this, {});
        }
    });
}

/*!
 * This function is called by the RunControl in its own \c initiateStop
 * implementation, which is triggered in response to pressing the
 * stop button in the \uicontrol{Application Output} pane or on direct
 * request of one of the sibling RunWorkers.
 */
void RunWorker::initiateStop()
{
    d->runControl->d->debugMessage("Initiate stop");
    emit canceled();
}

void RunWorker::addStartDependency(RunWorker *dependency)
{
    d->startDependencies.append(dependency);
}

void RunWorker::addStopDependency(RunWorker *dependency)
{
    d->stopDependencies.append(dependency);
}

// Output parser factories

static QList<std::function<OutputLineParser *(Target *)>> g_outputParserFactories;

QList<OutputLineParser *> createOutputParsers(Target *target)
{
    QList<OutputLineParser *> formatters;
    for (auto factory : std::as_const(g_outputParserFactories)) {
        if (OutputLineParser *parser = factory(target))
            formatters << parser;
    }
    return formatters;
}

void addOutputParserFactory(const std::function<Utils::OutputLineParser *(Target *)> &factory)
{
    g_outputParserFactories.append(factory);
}

// ProcessRunnerFactory

ProcessRunnerFactory::ProcessRunnerFactory(const QList<Id> &runConfigs)
{
    setId("ProcessRunnerFactory");
    setRecipeProducer([](RunControl *runControl) { return processRecipe(processTask(runControl)); });
    addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    setSupportedRunConfigs(runConfigs);
}

Storage<RunInterface> runStorage()
{
    static Storage<RunInterface> theRunStorage;
    return theRunStorage;
}

Canceler canceler()
{
    return [] { return std::make_pair(runStorage().activeStorage(), &RunInterface::canceled); };
}

Group errorTask(RunControl *runControl, const QString &message)
{
    return {
        Sync([runControl, message] {
            runControl->postMessage(message, ErrorMessageFormat);
            return DoneResult::Error;
        })
    };
}

Group processRecipe(const ProcessTask &processTask)
{
    return {
        When (processTask, &Process::started) >> Do {
            Sync([] { emit runStorage()->started(); })
        }
    };
}

} // namespace ProjectExplorer


#ifdef WITH_TESTS

#include <QTest>

namespace ProjectExplorer::Internal {

class RunWorkerConflictTest : public QObject
{
    Q_OBJECT

private slots:

    /*
      This needs to be run with all potentially conflicting factories loaded, i.e.
      something like

      bin/qtcreator -load RemoteLinux -load Qnx -load QmlProfiler -load Debugger -load Android \
        -load Docker -load PerfProfiler -load QtApplicationManagerIntegration -load Boot2Qt \
        -load McuSupport -load QmlPreview \
        -test ProjectExplorer,testConflict
    */

    void testConflict()
    {
        bool ok = true;
        const QList<Id> devices =
            transform(IDeviceFactory::allDeviceFactories(), &IDeviceFactory::deviceType);

        int supported = 0;
        int conflicts = 0;
        for (Id runMode : std::as_const(g_runModes)) {
            for (Id device : devices) {
                for (Id runConfig : std::as_const(g_runConfigs)) {
                    QList<Id> creators;
                    for (RunWorkerFactory *factory : g_runWorkerFactories) {
                        if (factory->canCreate(runMode, device, runConfig.toString()))
                            creators.append(factory->id());
                    }
                    if (!creators.isEmpty())
                        ++supported;
                    if (creators.size() > 1) {
                        qDebug() << "CONFLICT FOR" << runMode << device << runConfig
                                 << " FACTORIES " << creators;
                        ok = false;
                        ++conflicts;
                    }
                }
            }
        }
        qDebug() << "SUPPORTED COMBINATIONS: " << supported;
        qDebug() << "CONFLICTING COMBINATIONS: " << conflicts;
        QVERIFY(ok);
    }
};

QObject *createRunWorkerConflictTest()
{
    return new RunWorkerConflictTest;
}

} // ProjectExplorer::Internal

#endif // WITH_TESTS

#include "runcontrol.moc"
