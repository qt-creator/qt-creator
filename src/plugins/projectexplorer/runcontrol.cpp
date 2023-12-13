// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "runcontrol.h"

#include "buildconfiguration.h"
#include "customparser.h"
#include "devicesupport/devicemanager.h"
#include "devicesupport/idevice.h"
#include "devicesupport/idevicefactory.h"
#include "devicesupport/sshsettings.h"
#include "kitaspects.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "runconfigurationaspects.h"
#include "target.h"
#include "windebuginterface.h"

#include <coreplugin/icore.h>

#include <solutions/tasking/tasktree.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/fileinprojectfinder.h>
#include <utils/outputformatter.h>
#include <utils/process.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/terminalinterface.h>
#include <utils/utilsicons.h>

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <QLoggingCategory>
#include <QPushButton>
#include <QTextCodec>
#include <QTimer>

#if defined (WITH_JOURNALD)
#include "journaldwatcher.h"
#endif

using namespace ProjectExplorer::Internal;
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

    RunWorker *q;
    RunWorkerState state = RunWorkerState::Initialized;
    const QPointer<RunControl> runControl;
    QList<RunWorker *> startDependencies;
    QList<RunWorker *> stopDependencies;
    QString id;

    Store data;
    bool supportsReRunning = true;
    bool essential = false;
};

enum class RunControlState
{
    Initialized,      // Default value after creation.
    Starting,         // Actual process/tool starts.
    Running,          // All good and running.
    Stopping,         // initiateStop() was called, stop application/tool
    Stopped           // all good, but stopped. Can possibly be re-started
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
    return QString("<unknown: %1>").arg(int(s));
#    undef SN
}

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
    QPointer<Target> target; // Not owned.
    QPointer<Project> project; // Not owned.
    std::function<bool(bool*)> promptToStop;
    std::vector<RunWorkerFactory> m_factories;

    // A handle to the actual application process.
    ProcessHandle applicationProcessHandle;

    QList<QPointer<RunWorker>> m_workers;
    RunControlState state = RunControlState::Initialized;
    bool printEnvironment = false;
    bool autoDelete = false;
    bool m_supportsReRunning = true;
    std::optional<Tasking::Group> m_runRecipe;
};

class RunControlPrivate : public QObject, public RunControlPrivateData
{
    Q_OBJECT

public:
    RunControlPrivate(RunControl *parent, Id mode)
        : q(parent), runMode(mode)
    {
        icon = Icons::RUN_SMALL_TOOLBAR;
    }

    ~RunControlPrivate() override
    {
        QTC_CHECK(state == RunControlState::Stopped || state == RunControlState::Initialized);
        disconnect();
        q = nullptr;
        qDeleteAll(m_workers);
        m_workers.clear();
    }

    void copyData(RunControlPrivateData *other) { RunControlPrivateData::operator=(*other); }

    Q_ENUM(RunControlState)

    void checkState(RunControlState expectedState);
    void setState(RunControlState state);

    void debugMessage(const QString &msg) const;

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
    bool isUsingTaskTree() const { return bool(m_runRecipe); }
    void startTaskTree();
    void checkAutoDeleteAndEmitStopped();

    RunControl *q;
    Id runMode;
    std::unique_ptr<Tasking::TaskTree> m_taskTree;
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

    setTarget(runConfig->target());

    d->macroExpander = runConfig->macroExpander();
}

void RunControl::setTarget(Target *target)
{
    QTC_ASSERT(target, return);
    QTC_CHECK(!d->target);
    d->target = target;

    if (!d->buildKey.isEmpty() && target->buildSystem())
        d->buildTargetInfo = target->buildTarget(d->buildKey);

    if (auto bc = target->activeBuildConfiguration()) {
        d->buildDirectory = bc->buildDirectory();
        d->buildEnvironment = bc->environment();
    }

    setKit(target->kit());
    d->macroExpander = target->macroExpander();
    d->project = target->project();
}

void RunControl::setKit(Kit *kit)
{
    QTC_ASSERT(kit, return);
    QTC_CHECK(!d->kit);
    d->kit = kit;
    d->macroExpander = kit->macroExpander();

    if (!d->runnable.command.isEmpty())
        setDevice(DeviceManager::deviceForPath(d->runnable.command.executable()));
    else
        setDevice(DeviceKitAspect::device(kit));
}

void RunControl::setDevice(const IDevice::ConstPtr &device)
{
    QTC_CHECK(!d->device);
    d->device = device;
#ifdef WITH_JOURNALD
    if (!device.isNull() && device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
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

void RunControl::setAutoDeleteOnStop(bool autoDelete)
{
    d->autoDelete = autoDelete;
}

void RunControl::setRunRecipe(const Tasking::Group &group)
{
    d->m_runRecipe = group;
}

void RunControl::initiateStart()
{
    if (d->isUsingTaskTree()) {
        d->startTaskTree();
    } else {
        emit aboutToStart();
        d->initiateStart();
    }
}

void RunControl::initiateReStart()
{
    if (d->isUsingTaskTree()) {
        d->startTaskTree();
    } else {
        emit aboutToStart();
        d->initiateReStart();
    }
}

void RunControl::initiateStop()
{
    if (d->isUsingTaskTree()) {
        d->m_taskTree.reset();
        d->checkAutoDeleteAndEmitStopped();
    } else {
        d->initiateStop();
    }
}

void RunControl::forceStop()
{
    if (d->isUsingTaskTree()) {
        d->m_taskTree.reset();
        emit stopped();
    } else {
        d->forceStop();
    }
}

RunWorker *RunControl::createWorker(Id workerId)
{
    const Id deviceType = DeviceTypeKitAspect::deviceTypeId(d->kit);
    for (RunWorkerFactory *factory : std::as_const(g_runWorkerFactories)) {
        if (factory->canCreate(workerId, deviceType, QString()))
            return factory->create(this);
    }
    return nullptr;
}

bool RunControl::createMainWorker()
{
    const QList<RunWorkerFactory *> candidates
        = filtered(g_runWorkerFactories, [this](RunWorkerFactory *factory) {
              return factory->canCreate(d->runMode,
                                        DeviceTypeKitAspect::deviceTypeId(d->kit),
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
    checkState(RunControlState::Initialized);
    setState(RunControlState::Starting);
    debugMessage("Queue: Starting");

    continueStart();
}

void RunControlPrivate::initiateReStart()
{
    checkState(RunControlState::Stopped);

    // Re-set worked on re-runs.
    for (RunWorker *worker : std::as_const(m_workers)) {
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
    for (RunWorker *worker : std::as_const(m_workers)) {
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
    if (state == RunControlState::Initialized)
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

    for (RunWorker *worker : std::as_const(m_workers)) {
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
    if (state == RunControlState::Stopping)
        targetState = RunControlState::Stopped;

    if (allDone) {
        debugMessage("All Stopped");
        setState(targetState);
    } else {
        debugMessage("Not all workers Stopped. Waiting...");
    }
}

void RunControlPrivate::forceStop()
{
    if (state == RunControlState::Stopped) {
        debugMessage("Was finished, too late to force Stop");
        return;
    }
    for (RunWorker *worker : std::as_const(m_workers)) {
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

void RunControlPrivate::onWorkerStarted(RunWorker *worker)
{
    worker->d->state = RunWorkerState::Running;

    if (state == RunControlState::Starting) {
        debugMessage(worker->d->id + " start succeeded");
        continueStart();
        return;
    }
    showError(Tr::tr("Unexpected run control state %1 when worker %2 started.")
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
        QMessageBox::critical(Core::ICore::dialogParent(), Tr::tr("Error"),
             QString("Failure during startup. Aborting.") + "<p>" + msg);
        continueStopOrFinish();
        break;
    case RunControlState::Starting:
    case RunControlState::Running:
        initiateStop();
        break;
    case RunControlState::Stopping:
        continueStopOrFinish();
        break;
    case RunControlState::Stopped:
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

    if (state == RunControlState::Stopping) {
        continueStopOrFinish();
        return;
    } else if (worker->isEssential()) {
        debugMessage(workerId + " is essential. Stopping all others.");
        initiateStop();
        return;
    }

    for (RunWorker *dependent : std::as_const(worker->d->stopDependencies)) {
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
    for (RunWorker *worker : std::as_const(m_workers)) {
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
        q->postMessage(msg + '\n', ErrorMessageFormat);
}

void RunControl::setupFormatter(OutputFormatter *formatter) const
{
    QList<Utils::OutputLineParser *> parsers = OutputFormatterFactory::createFormatters(target());
    if (const auto customParsersAspect = aspect<CustomParsersAspect>()) {
        for (const Id id : std::as_const(customParsersAspect->parsers)) {
            if (CustomParser * const parser = CustomParser::createFromId(id))
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

Target *RunControl::target() const
{
    return d->target;
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

const BaseAspect::Data *RunControl::aspect(Id instanceId) const
{
    return d->aspectData.aspect(instanceId);
}

const BaseAspect::Data *RunControl::aspect(BaseAspect::Data::ClassId classId) const
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

void RunControl::setSupportsReRunning(bool reRunningSupported)
{
    d->m_supportsReRunning = reRunningSupported;
}

bool RunControl::supportsReRunning() const
{
    if (d->isUsingTaskTree())
        return d->m_supportsReRunning;
    return d->supportsReRunning();
}

bool RunControlPrivate::supportsReRunning() const
{
    for (RunWorker *worker : m_workers) {
        if (!worker) {
            debugMessage("Found unknown deleted worker when checking for re-run support");
            return false;
        }
        if (!worker->d->supportsReRunning)
            return false;
        if (worker->d->state != RunWorkerState::Done)
            return false;
    }
    return true;
}

void RunControlPrivate::startTaskTree()
{
    using namespace Tasking;

    m_taskTree.reset(new TaskTree(*m_runRecipe));
    connect(m_taskTree.get(), &TaskTree::started, q, &RunControl::started);
    const auto finalize = [this] {
        m_taskTree.release()->deleteLater();
        checkAutoDeleteAndEmitStopped();
    };
    connect(m_taskTree.get(), &TaskTree::done, this, finalize);
    connect(m_taskTree.get(), &TaskTree::errorOccurred, this, finalize);
    m_taskTree->start();
}

void RunControlPrivate::checkAutoDeleteAndEmitStopped()
{
    if (autoDelete) {
        debugMessage("All finished. Deleting myself");
        q->deleteLater();
    } else {
        q->setApplicationProcessHandle(Utils::ProcessHandle());
    }
    emit q->stopped();
}

bool RunControl::isRunning() const
{
    if (d->isUsingTaskTree())
        return d->m_taskTree.get();
    return d->state == RunControlState::Running;
}

bool RunControl::isStarting() const
{
    if (d->isUsingTaskTree())
        return false;
    return d->state == RunControlState::Starting;
}

bool RunControl::isStopped() const
{
    if (d->isUsingTaskTree())
        return !d->m_taskTree.get();
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
    QMap<QMessageBox::StandardButton, QString> buttonTexts;
    if (!stopButtonText.isEmpty())
        buttonTexts[QMessageBox::Yes] = stopButtonText;
    if (!cancelButtonText.isEmpty())
        buttonTexts[QMessageBox::Cancel] = cancelButtonText;

    CheckableDecider decider;
    if (prompt)
        decider = CheckableDecider(prompt);

    auto selected = CheckableMessageBox::question(Core::ICore::dialogParent(),
                                                  title,
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

bool RunControlPrivate::isAllowedTransition(RunControlState from, RunControlState to)
{
    switch (from) {
    case RunControlState::Initialized:
        return to == RunControlState::Starting;
    case RunControlState::Starting:
        return to == RunControlState::Running || to == RunControlState::Stopping;
    case RunControlState::Running:
        return to == RunControlState::Stopping || to == RunControlState::Stopped;
    case RunControlState::Stopping:
        return to == RunControlState::Stopped;
    case RunControlState::Stopped:
        return to != RunControlState::Initialized;
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
        qDebug() << "Invalid run control state transition from" << stateName(state)
                 << "to" << stateName(newState);

    state = newState;

    debugMessage("Entering state " + stateName(newState));

    // Extra reporting.
    switch (state) {
    case RunControlState::Running:
        emit q->started();
        break;
    case RunControlState::Stopped:
        checkAutoDeleteAndEmitStopped();
        break;
    default:
        break;
    }
}

void RunControlPrivate::debugMessage(const QString &msg) const
{
    qCDebug(statesLog()) << msg;
}


// SimpleTargetRunnerPrivate

namespace Internal {

class SimpleTargetRunnerPrivate : public QObject
{
public:
    explicit SimpleTargetRunnerPrivate(SimpleTargetRunner *parent);
    ~SimpleTargetRunnerPrivate() override;

    void start();
    void stop();

    Utils::ProcessHandle applicationPID() const;

    enum State { Inactive, Run };

    void handleStandardOutput();
    void handleStandardError();
    void handleDone();

    // Local
    qint64 privateApplicationPID() const;
    bool isRunning() const;

    SimpleTargetRunner *q = nullptr;

    bool m_runAsRoot = false;

    Process m_process;

    QTextCodec *m_outputCodec = nullptr;
    QTextCodec::ConverterState m_outputCodecState;
    QTextCodec::ConverterState m_errorCodecState;

    State m_state = Inactive;
    bool m_stopRequested = false;

    Utils::CommandLine m_command;
    Utils::FilePath m_workingDirectory;
    Utils::Environment m_environment;
    QVariantHash m_extraData;

    ProcessResultData m_resultData;

    std::function<void()> m_startModifier;

    bool m_stopReported = false;
    bool m_stopForced = false;

    void forwardStarted();
    void forwardDone();
};

} // Internal

static QProcess::ProcessChannelMode defaultProcessChannelMode()
{
    return ProjectExplorerPlugin::appOutputSettings().mergeChannels
            ? QProcess::MergedChannels : QProcess::SeparateChannels;
}

SimpleTargetRunnerPrivate::SimpleTargetRunnerPrivate(SimpleTargetRunner *parent)
    : q(parent)
{
    m_process.setProcessChannelMode(defaultProcessChannelMode());
    connect(&m_process, &Process::started, this, &SimpleTargetRunnerPrivate::forwardStarted);
    connect(&m_process, &Process::done, this, &SimpleTargetRunnerPrivate::handleDone);
    connect(&m_process, &Process::readyReadStandardError,
                this, &SimpleTargetRunnerPrivate::handleStandardError);
    connect(&m_process, &Process::readyReadStandardOutput,
                this, &SimpleTargetRunnerPrivate::handleStandardOutput);

    if (WinDebugInterface::instance()) {
        connect(WinDebugInterface::instance(), &WinDebugInterface::cannotRetrieveDebugOutput,
            this, [this] {
                disconnect(WinDebugInterface::instance(), nullptr, this, nullptr);
                q->appendMessage(Tr::tr("Cannot retrieve debugging output.")
                          + QLatin1Char('\n'), ErrorMessageFormat);
        });

        connect(WinDebugInterface::instance(), &WinDebugInterface::debugOutput,
            this, [this](qint64 pid, const QString &message) {
            if (privateApplicationPID() == pid)
                q->appendMessage(message, DebugFormat);
        });
    }
}

SimpleTargetRunnerPrivate::~SimpleTargetRunnerPrivate()
{
    if (m_state == Run)
        forwardDone();
}

void SimpleTargetRunnerPrivate::stop()
{
    m_resultData.m_exitStatus = QProcess::CrashExit;

    const bool isLocal = !m_command.executable().needsDevice();
    if (isLocal) {
        if (!isRunning())
            return;
        m_process.stop();
        m_process.waitForFinished();
        QTimer::singleShot(100, this, [this] { forwardDone(); });
    } else {
        if (m_stopRequested)
            return;
        m_stopRequested = true;
        q->appendMessage(Tr::tr("User requested stop. Shutting down..."), NormalMessageFormat);
        switch (m_state) {
        case Run:
            m_process.stop();
            if (!m_process.waitForFinished(2000)) { // TODO: it may freeze on some devices
                q->appendMessage(Tr::tr("Remote process did not finish in time. "
                                        "Connectivity lost?"), ErrorMessageFormat);
                m_process.close();
                m_state = Inactive;
                forwardDone();
            }
            break;
        case Inactive:
            break;
        }
    }
}

bool SimpleTargetRunnerPrivate::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

qint64 SimpleTargetRunnerPrivate::privateApplicationPID() const
{
    if (!isRunning())
        return 0;

    return m_process.processId();
}

void SimpleTargetRunnerPrivate::handleDone()
{
    m_resultData = m_process.resultData();
    QTC_ASSERT(m_state == Run, forwardDone(); return);

    m_state = Inactive;
    forwardDone();
}

void SimpleTargetRunnerPrivate::handleStandardOutput()
{
    const QByteArray data = m_process.readAllRawStandardOutput();
    const QString msg = m_outputCodec->toUnicode(
                data.constData(), data.length(), &m_outputCodecState);
    q->appendMessage(msg, StdOutFormat, false);
}

void SimpleTargetRunnerPrivate::handleStandardError()
{
    const QByteArray data = m_process.readAllRawStandardError();
    const QString msg = m_outputCodec->toUnicode(
                data.constData(), data.length(), &m_errorCodecState);
    q->appendMessage(msg, StdErrFormat, false);
}

void SimpleTargetRunnerPrivate::start()
{
    const bool isLocal = !m_command.executable().needsDevice();

    CommandLine cmdLine = m_command;
    Environment env = m_environment;

    m_resultData = {};
    QTC_ASSERT(m_state == Inactive, return);

    if (isLocal) {
        if (m_runAsRoot)
            RunControl::provideAskPassEntry(env);

        WinDebugInterface::startIfNeeded();

        if (HostOsInfo::isMacHost()) {
            CommandLine disclaim(Core::ICore::libexecPath("disclaim"));
            disclaim.addCommandLineAsArgs(cmdLine);
            cmdLine = disclaim;
        }

        m_process.setRunAsRoot(m_runAsRoot);
    }

    const IDevice::ConstPtr device = DeviceManager::deviceForPath(m_command.executable());
    if (device && !device->isEmptyCommandAllowed() && m_command.isEmpty()) {
        m_resultData.m_errorString = Tr::tr("Cannot run: No command given.");
        m_resultData.m_error = QProcess::FailedToStart;
        m_resultData.m_exitStatus = QProcess::CrashExit;
        forwardDone();
        return;
    }

    m_stopRequested = false;

    QVariantHash extraData = m_extraData;
    extraData[TERMINAL_SHELL_NAME] = m_command.executable().fileName();

    m_process.setCommand(cmdLine);
    m_process.setEnvironment(env);
    m_process.setExtraData(extraData);

    m_state = Run;
    m_process.setWorkingDirectory(m_workingDirectory);

    if (isLocal)
        m_outputCodec = QTextCodec::codecForLocale();
    else
        m_outputCodec = QTextCodec::codecForName("utf8");

    m_process.start();
}


/*!
    \class ProjectExplorer::SimpleTargetRunner

    \brief The SimpleTargetRunner class is the application launcher of the
    ProjectExplorer plugin.

    Encapsulates processes running in a console or as GUI processes,
    captures debug output of GUI processes on Windows (outputDebugString()).

    \sa Utils::Process
*/

SimpleTargetRunner::SimpleTargetRunner(RunControl *runControl)
    : RunWorker(runControl), d(new Internal::SimpleTargetRunnerPrivate(this))
{
    setId("SimpleTargetRunner");
}

SimpleTargetRunner::~SimpleTargetRunner() = default;

void SimpleTargetRunnerPrivate::forwardDone()
{
    if (m_stopReported)
        return;
    const QString executable = m_command.executable().displayName();
    QString msg = Tr::tr("%1 exited with code %2").arg(executable).arg(m_resultData.m_exitCode);
    if (m_resultData.m_exitStatus == QProcess::CrashExit)
        msg = Tr::tr("%1 crashed.").arg(executable);
    else if (m_stopForced)
        msg = Tr::tr("The process was ended forcefully.");
    else if (m_resultData.m_error != QProcess::UnknownError)
        msg = RunWorker::userMessageForProcessError(m_resultData.m_error, m_command.executable());
    q->appendMessage(msg, NormalMessageFormat);
    m_stopReported = true;
    q->reportStopped();
}

void SimpleTargetRunnerPrivate::forwardStarted()
{
    const bool isDesktop = !m_command.executable().needsDevice();
    if (isDesktop) {
        // Console processes only know their pid after being started
        ProcessHandle pid{privateApplicationPID()};
        q->runControl()->setApplicationProcessHandle(pid);
        pid.activate();
    }

    q->reportStarted();
}

void SimpleTargetRunner::start()
{
    d->m_command = runControl()->commandLine();
    d->m_workingDirectory = runControl()->workingDirectory();
    d->m_environment = runControl()->environment();
    d->m_extraData = runControl()->extraData();

    if (d->m_startModifier)
        d->m_startModifier();

    bool useTerminal = false;
    if (auto terminalAspect = runControl()->aspect<TerminalAspect>())
        useTerminal = terminalAspect->useTerminal;

    bool runAsRoot = false;
    if (auto runAsRootAspect = runControl()->aspect<RunAsRootAspect>())
        runAsRoot = runAsRootAspect->value;

    d->m_stopForced = false;
    d->m_stopReported = false;
    d->disconnect(this);
    d->m_process.setTerminalMode(useTerminal ? Utils::TerminalMode::Run : Utils::TerminalMode::Off);
    d->m_runAsRoot = runAsRoot;

    const QString msg = Tr::tr("Starting %1...").arg(d->m_command.displayName());
    appendMessage(msg, NormalMessageFormat);
    if (runControl()->isPrintEnvironmentEnabled()) {
        appendMessage(Tr::tr("Environment:"), NormalMessageFormat);
        runControl()->runnable().environment
            .forEachEntry([this](const QString &key, const QString &value, bool enabled) {
                if (enabled)
                    appendMessage(key + '=' + value, StdOutFormat);
            });
        appendMessage({}, StdOutFormat);
    }

    const bool isDesktop = !d->m_command.executable().needsDevice();
    if (isDesktop && d->m_command.isEmpty()) {
        reportFailure(Tr::tr("No executable specified."));
        return;
    }
    d->start();
}

void SimpleTargetRunner::stop()
{
    d->m_stopForced = true;
    d->stop();
}

void SimpleTargetRunner::setStartModifier(const std::function<void ()> &startModifier)
{
    d->m_startModifier = startModifier;
}

CommandLine SimpleTargetRunner::commandLine() const
{
    return d->m_command;
}

void SimpleTargetRunner::setCommandLine(const Utils::CommandLine &commandLine)
{
    d->m_command = commandLine;
}

void SimpleTargetRunner::setEnvironment(const Environment &environment)
{
    d->m_environment = environment;
}

void SimpleTargetRunner::setWorkingDirectory(const FilePath &workingDirectory)
{
    d->m_workingDirectory = workingDirectory;
}

void SimpleTargetRunner::forceRunOnHost()
{
    const FilePath executable = d->m_command.executable();
    if (executable.needsDevice()) {
        QTC_CHECK(false);
        d->m_command.setExecutable(FilePath::fromString(executable.path()));
    }
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
    d->runControl->d->onWorkerFailed(this, msg);
}

/*!
 * Appends a message in the specified \a format to
 * the owning RunControl's \uicontrol{Application Output} pane.
 */
void RunWorker::appendMessage(const QString &msg, OutputFormat format, bool appendNewLine)
{
    d->runControl->postMessage(msg, format, appendNewLine);
}

IDevice::ConstPtr RunWorker::device() const
{
    return d->runControl->device();
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

void RunWorker::recordData(const Key &channel, const QVariant &data)
{
    d->data[channel] = data;
}

QVariant RunWorker::recordedData(const Key &channel) const
{
    return d->data[channel];
}

void RunWorker::setSupportsReRunning(bool reRunningSupported)
{
    d->supportsReRunning = reRunningSupported;
}

QString RunWorker::userMessageForProcessError(QProcess::ProcessError error, const FilePath &program)
{
    QString failedToStart = Tr::tr("The process failed to start.");
    QString msg = Tr::tr("An unknown error in the process occurred.");
    switch (error) {
        case QProcess::FailedToStart:
            msg = failedToStart + ' ' + Tr::tr("Either the "
                "invoked program \"%1\" is missing, or you may have insufficient "
                "permissions to invoke the program.").arg(program.toUserOutput());
            break;
        case QProcess::Crashed:
            msg = Tr::tr("The process crashed.");
            break;
        case QProcess::Timedout:
            // "The last waitFor...() function timed out. "
            //   "The state of QProcess is unchanged, and you can try calling "
            // "waitFor...() again."
            return {}; // sic!
        case QProcess::WriteError:
            msg = Tr::tr("An error occurred when attempting to write "
                "to the process. For example, the process may not be running, "
                "or it may have closed its input channel.");
            break;
        case QProcess::ReadError:
            msg = Tr::tr("An error occurred when attempting to read from "
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

// OutputFormatterFactory

static QList<OutputFormatterFactory *> g_outputFormatterFactories;

OutputFormatterFactory::OutputFormatterFactory()
{
    g_outputFormatterFactories.append(this);
}

OutputFormatterFactory::~OutputFormatterFactory()
{
    g_outputFormatterFactories.removeOne(this);
}

QList<OutputLineParser *> OutputFormatterFactory::createFormatters(Target *target)
{
    QList<OutputLineParser *> formatters;
    for (auto factory : std::as_const(g_outputFormatterFactories))
        formatters << factory->m_creator(target);
    return formatters;
}

void OutputFormatterFactory::setFormatterCreator(const FormatterCreator &creator)
{
    m_creator = creator;
}

// SimpleTargetRunnerFactory

SimpleTargetRunnerFactory::SimpleTargetRunnerFactory(const QList<Id> &runConfigs)
{
    setProduct<SimpleTargetRunner>();
    addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    setSupportedRunConfigs(runConfigs);
}

} // namespace ProjectExplorer

#include "runcontrol.moc"
