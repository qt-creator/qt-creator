/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "runcontrol.h"

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

#if defined (WITH_JOURNALD)
#include "journaldwatcher.h"
#endif

using namespace Utils;
using namespace ProjectExplorer::Internal;

namespace {
Q_LOGGING_CATEGORY(statesLog, "qtc.projectmanager.states", QtWarningMsg)
}

namespace ProjectExplorer {

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

    if (!m_supportedRunConfigurations.isEmpty()) {
        if (!m_supportedRunConfigurations.contains(runConfiguration->id()))
            return false;
    }

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

void RunWorkerFactory::setSupportedRunConfigurations(const QList<Core::Id> &ids)
{
    m_supportedRunConfigurations = ids;
}

void RunWorkerFactory::addSupportedRunConfiguration(Core::Id id)
{
    m_supportedRunConfigurations.append(id);
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
    RunControlPrivate(RunControl *parent, Core::Id mode)
        : q(parent), runMode(mode)
    {
        icon = Icons::RUN_SMALL_TOOLBAR;
        outputFormatter = new OutputFormatter();
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
    MacroExpander *macroExpander;
    QPointer<RunConfiguration> runConfiguration; // Not owned. Avoid use.
    Kit *kit = nullptr; // Not owned.
    QPointer<Target> target; // Not owned.
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

RunControl::RunControl(Core::Id mode) :
    d(std::make_unique<RunControlPrivate>(this,  mode))
{
}

void RunControl::setRunConfiguration(RunConfiguration *runConfig)
{
    QTC_ASSERT(runConfig, return);
    QTC_CHECK(!d->runConfiguration);
    d->runConfiguration = runConfig;
    d->runnable = runConfig->runnable();
    d->displayName  = runConfig->displayName();
    if (auto outputFormatter = runConfig->createOutputFormatter()) {
        delete d->outputFormatter;
        d->outputFormatter = outputFormatter;
    }
    d->macroExpander = runConfig->macroExpander();
    setTarget(runConfig->target());
}

void RunControl::setTarget(Target *target)
{
    QTC_ASSERT(target, return);
    QTC_CHECK(!d->target);
    d->target = target;
    setKit(target->kit());
    d->project = target->project();
}

void RunControl::setKit(Kit *kit)
{
    QTC_ASSERT(kit, return);
    QTC_CHECK(!d->kit);
    d->kit = kit;

    if (d->runnable.device)
        setDevice(d->runnable.device);
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
            appendMessage(message, Utils::OutputFormat::LogMessageFormat);
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

bool RunControl::createMainWorker()
{
    const auto canRun = std::bind(&RunWorkerFactory::canRun, std::placeholders::_1,
                                  d->runConfiguration, d->runMode);
    const QList<RunWorkerFactory *> candidates = Utils::filtered(g_runWorkerFactories, canRun);
    // There might be combinations that cannot run. But that should have been checked
    // with canRun below.
    QTC_ASSERT(!candidates.empty(), return false);

    // There should be at most one top-level producer feeling responsible per combination.
    // Breaking a tie should be done by tightening the restrictions on one of them.
    QTC_CHECK(candidates.size() == 1);
    return candidates.front()->producer()(this) != nullptr;
}

bool RunControl::canRun(RunConfiguration *runConfig, Core::Id runMode)
{
    const auto check = std::bind(&RunWorkerFactory::canRun, std::placeholders::_1, runConfig, runMode);
    return Utils::contains(g_runWorkerFactories, check);
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

MacroExpander *RunControl::macroExpander() const
{
    return d->macroExpander;
}

ProjectConfigurationAspect *RunControl::aspect(Core::Id id) const
{
    return d->runConfiguration ? d->runConfiguration->aspect(id) : nullptr;
}

ISettingsAspect *RunControl::settings(Core::Id id) const
{
    return d->runConfiguration ? d->runConfiguration->currentSettings(id) : nullptr;
}

QString RunControl::buildKey() const
{
    return d->runConfiguration ? d->runConfiguration->buildKey() : QString();
}

BuildTargetInfo RunControl::buildTargetInfo() const
{
    return d->runConfiguration->buildTargetInfo();
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

// SimpleTargetRunner

SimpleTargetRunner::SimpleTargetRunner(RunControl *runControl)
    : RunWorker(runControl)
{
    setId("SimpleTargetRunner");
    m_runnable = runControl->runnable(); // Default value. Can be overridden using setRunnable.
    m_device = runControl->device(); // Default value. Can be overridden using setDevice.
    if (auto terminalAspect = runControl->aspect<TerminalAspect>())
        m_useTerminal = terminalAspect->useTerminal();
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

        if (m_runnable.executable.isEmpty()) {
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
    const QString msg = userMessageForProcessError(error, m_runnable.executable);
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

QString RunWorker::userMessageForProcessError(QProcess::ProcessError error, const FilePath &program)
{
    QString failedToStart = tr("The process failed to start.");
    QString msg = tr("An unknown error in the process occurred.");
    switch (error) {
        case QProcess::FailedToStart:
            msg = failedToStart + ' ' + tr("Either the "
                "invoked program \"%1\" is missing, or you may have insufficient "
                "permissions to invoke the program.").arg(program.toUserOutput());
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

CommandLine Runnable::commandLine() const
{
    return CommandLine(executable, commandLineArguments, CommandLine::Raw);
}

void Runnable::setCommandLine(const CommandLine &cmdLine)
{
    executable = cmdLine.executable();
    commandLineArguments = cmdLine.arguments();
}

} // namespace ProjectExplorer
