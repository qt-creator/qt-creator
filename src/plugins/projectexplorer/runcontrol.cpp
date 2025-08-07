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
#include <solutions/tasking/conditional.h>
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

void RunWorkerFactory::setRecipeProducer(const RecipeCreator &producer)
{
    m_recipeCreator = producer;
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

bool RunWorkerFactory::canCreate(Id runMode, Id deviceType, Id runConfigId) const
{
    if (!m_supportedRunModes.contains(runMode))
        return false;

    if (!m_supportedRunConfigurations.isEmpty() && !m_supportedRunConfigurations.contains(runConfigId))
        return false;

    if (!m_supportedDeviceTypes.isEmpty())
        return m_supportedDeviceTypes.contains(deviceType);

    return true;
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
                                             runConfig);
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

class RunControlPrivateData
{
public:
    bool isPortsGatherer() const
    { return useDebugChannel || useQmlChannel || usePerfChannel || useWorkerChannel; }

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

    // A handle to the actual application process.
    ProcessHandle applicationProcessHandle;

    bool printEnvironment = false;
    Group m_runRecipe {};

    bool useDebugChannel = false;
    bool useQmlChannel = false;
    bool usePerfChannel = false;
    bool useWorkerChannel = false;
    QUrl debugChannel;
    QUrl qmlChannel;
    QUrl perfChannel;
    QUrl workerChannel;
    ProcessHandle m_attachPid;
};

class RunControlPrivate
{
public:
    RunControlPrivate(RunControl *parent, Id mode)
        : q(parent), runMode(mode)
    {
        data.icon = Icons::RUN_SMALL_TOOLBAR;
    }

    ~RunControlPrivate()
    {
        QTC_CHECK(!m_taskTreeRunner.isRunning());
    }

    void debugMessage(const QString &msg) const;

    void startTaskTree();
    void emitStopped();

    QUrl getNextChannel(PortList *portList, const QList<Port> &usedPorts) const;

    Group portsGathererRecipe();

    RunControl *q;
    RunControlPrivateData data;
    Id runMode;
    TaskTreeRunner m_taskTreeRunner;
};

} // Internal

using namespace Internal;

RunControl::RunControl(Id mode)
    : d(std::make_unique<RunControlPrivate>(this,  mode))
{}

void RunControl::copyDataFromRunControl(RunControl *runControl)
{
    QTC_ASSERT(runControl, return);
    d->data = runControl->d->data;
}

Group RunControl::noRecipeTask()
{
    return errorTask(Tr::tr("No recipe producer."));
}

Group RunControl::errorTask(const QString &message)
{
    return {
        Sync([this, message] {
           postMessage(message, ErrorMessageFormat);
           return false;
        })
    };
}

Group RunControl::processRecipe(const ProcessTask &processTask)
{
    return {
        When (processTask, &Process::started) >> Do {
            Sync([this] { reportStarted(); })
        }
    };
}

void RunControl::start()
{
    ProjectExplorerPlugin::startRunControl(this);
}

void RunControl::reportStarted()
{
    d->debugMessage("Started");
    emit started();
}

void RunControl::copyDataFromRunConfiguration(RunConfiguration *runConfig)
{
    QTC_ASSERT(runConfig, return);
    d->data.runConfigId = runConfig->id();
    d->data.runnable = runConfig->runnable();
    d->data.extraData = runConfig->extraData();
    d->data.displayName = runConfig->expandedDisplayName();
    d->data.buildKey = runConfig->buildKey();
    d->data.settingsData = runConfig->settingsData();
    d->data.aspectData = runConfig->aspectData();
    d->data.printEnvironment = runConfig->isPrintEnvironmentEnabled();

    setBuildConfiguration(runConfig->buildConfiguration());

    d->data.macroExpander = runConfig->macroExpander();
}

void RunControl::setBuildConfiguration(BuildConfiguration *bc)
{
    QTC_ASSERT(bc, return);
    QTC_CHECK(!d->data.buildConfiguration);
    d->data.buildConfiguration = bc;

    if (!d->data.buildKey.isEmpty())
        d->data.buildTargetInfo = bc->buildSystem()->buildTarget(d->data.buildKey);

    d->data.buildDirectory = bc->buildDirectory();
    d->data.buildEnvironment = bc->environment();

    setKit(bc->kit());
    d->data.macroExpander = bc->macroExpander();
    d->data.project = bc->project();
}

void RunControl::setKit(Kit *kit)
{
    QTC_ASSERT(kit, return);
    QTC_CHECK(!d->data.kit);
    d->data.kit = kit;
    d->data.macroExpander = kit->macroExpander();

    if (!d->data.runnable.command.isEmpty()) {
        setDevice(DeviceManager::deviceForPath(d->data.runnable.command.executable()));
        QTC_ASSERT(device(), setDevice(RunDeviceKitAspect::device(kit)));
    } else {
        setDevice(RunDeviceKitAspect::device(kit));
    }
}

void RunControl::setDevice(const IDevice::ConstPtr &device)
{
    QTC_CHECK(!d->data.device);
    d->data.device = device;
#ifdef WITH_JOURNALD
    if (device && device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
        JournaldWatcher::instance()->subscribe(this, [this](const JournaldWatcher::LogEntry &entry) {

            if (entry.value("_MACHINE_ID") != JournaldWatcher::instance()->machineId())
                return;

            const QByteArray pid = entry.value("_PID");
            if (pid.isEmpty())
                return;

            const qint64 pidNum = static_cast<qint64>(QString::fromLatin1(pid).toInt());
            if (pidNum != d->data.applicationProcessHandle.pid())
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
    d->data.m_runRecipe = group;
}

void RunControl::initiateStart()
{
    emit aboutToStart();
    d->startTaskTree();
}

void RunControl::initiateStop()
{
    emit canceled();
}

void RunControl::forceStop()
{
    d->m_taskTreeRunner.reset();
    d->emitStopped();
}

Group RunControl::createRecipe(Id runMode)
{
    const Id deviceType = RunDeviceTypeKitAspect::deviceTypeId(d->data.kit);
    for (RunWorkerFactory *factory : std::as_const(g_runWorkerFactories)) {
        if (factory->canCreate(runMode, deviceType, d->data.runConfigId))
            return factory->createRecipe(this);
    }
    return noRecipeTask();
}

bool RunControl::createMainRecipe()
{
    const QList<RunWorkerFactory *> candidates
        = filtered(g_runWorkerFactories, [this](RunWorkerFactory *factory) {
              return factory->canCreate(d->runMode,
                                        RunDeviceTypeKitAspect::deviceTypeId(d->data.kit),
                                        d->data.runConfigId);
          });

    // There might be combinations that cannot run. But that should have been checked
    // with canRun below.
    QTC_ASSERT(!candidates.empty(), return false);

    // There should be at most one top-level producer feeling responsible per combination.
    // Breaking a tie should be done by tightening the restrictions on one of them.
    QTC_CHECK(candidates.size() == 1);
    setRunRecipe(candidates.front()->createRecipe(this));
    return true;
}

bool RunControl::canRun(Id runMode, Id deviceType, Id runConfigId)
{
    for (const RunWorkerFactory *factory : std::as_const(g_runWorkerFactories)) {
        if (factory->canCreate(runMode, deviceType, runConfigId))
            return true;
    }
    return false;
}

void RunControl::postMessage(const QString &msg, OutputFormat format, bool appendNewLine)
{
    emit appendMessage((appendNewLine && !msg.endsWith('\n')) ? msg + '\n': msg, format);
}

QUrl RunControlPrivate::getNextChannel(PortList *portList, const QList<Port> &usedPorts) const
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

Group RunControlPrivate::portsGathererRecipe()
{
    const Storage<PortsOutputData> portsStorage;

    const auto onSetup = [this] {
        if (!data.device) {
            q->postMessage(Tr::tr("Can't use ports gatherer - no device set."), ErrorMessageFormat);
            return SetupResult::StopWithError;
        }
        q->postMessage(Tr::tr("Checking available ports..."), NormalMessageFormat);
        return SetupResult::Continue;
    };

    const auto onDone = [this, portsStorage] {
        const auto ports = *portsStorage;
        if (!ports) {
            q->postMessage(Tr::tr("No free ports found."), ErrorMessageFormat);
            return DoneResult::Error;
        }
        PortList portList = data.device->freePorts();
        const QList<Port> usedPorts = *ports;
        q->postMessage(Tr::tr("Found %n free ports.", nullptr, portList.count()), NormalMessageFormat);
        if (data.useDebugChannel)
            data.debugChannel = getNextChannel(&portList, usedPorts);
        if (data.useQmlChannel)
            data.qmlChannel = getNextChannel(&portList, usedPorts);
        if (data.usePerfChannel)
            data.perfChannel = getNextChannel(&portList, usedPorts);
        if (data.useWorkerChannel)
            data.workerChannel = getNextChannel(&portList, usedPorts);
        return DoneResult::Success;
    };

    return {
        portsStorage,
        onGroupSetup(onSetup),
        data.device->portsGatheringRecipe(portsStorage),
        onGroupDone(onDone)
    };
}

void RunControl::requestDebugChannel()
{
    d->data.useDebugChannel = true;
}

bool RunControl::usesDebugChannel() const
{
    return d->data.useDebugChannel;
}

QUrl RunControl::debugChannel() const
{
    return d->data.debugChannel;
}

void RunControl::setDebugChannel(const QUrl &channel)
{
    d->data.debugChannel = channel;
}

void RunControl::requestQmlChannel()
{
    d->data.useQmlChannel = true;
}

bool RunControl::usesQmlChannel() const
{
    return d->data.useQmlChannel;
}

QUrl RunControl::qmlChannel() const
{
    return d->data.qmlChannel;
}

void RunControl::setQmlChannel(const QUrl &channel)
{
    d->data.qmlChannel = channel;
}

void RunControl::requestPerfChannel()
{
    d->data.usePerfChannel = true;
}

bool RunControl::usesPerfChannel() const
{
    return d->data.usePerfChannel;
}

QUrl RunControl::perfChannel() const
{
    return d->data.perfChannel;
}

void RunControl::requestWorkerChannel()
{
    d->data.useWorkerChannel = true;
}

QUrl RunControl::workerChannel() const
{
    return d->data.workerChannel;
}

void RunControl::setAttachPid(ProcessHandle pid)
{
    d->data.m_attachPid = pid;
}

ProcessHandle RunControl::attachPid() const
{
    return d->data.m_attachPid;
}

void RunControl::showOutputPane()
{
    appOutputPane().showOutputPaneForRunControl(this);
}

void RunControl::setupFormatter(OutputFormatter *formatter) const
{
    QList<OutputLineParser *> parsers = createOutputParsers(buildConfiguration());
    if (const auto customParsersAspect = aspectData<CustomParsersAspect>()) {
        for (const Id id : std::as_const(customParsersAspect->parsers)) {
            if (auto parser = createCustomParserFromId(id))
                parsers << parser;
        }
    }
    formatter->setLineParsers(parsers);
    if (project()) {
        FileInProjectFinder fileFinder;
        fileFinder.setProjectDirectory(project()->projectDirectory());
        fileFinder.setProjectFiles(project()->files(Project::AllFiles));
        formatter->setFileFinder(fileFinder);
    }
}

Id RunControl::runMode() const
{
    return d->runMode;
}

bool RunControl::isPrintEnvironmentEnabled() const
{
    return d->data.printEnvironment;
}

const ProcessRunData &RunControl::runnable() const
{
    return d->data.runnable;
}

const CommandLine &RunControl::commandLine() const
{
    return d->data.runnable.command;
}

void RunControl::setCommandLine(const CommandLine &command)
{
    d->data.runnable.command = command;
}

const FilePath &RunControl::workingDirectory() const
{
    return d->data.runnable.workingDirectory;
}

void RunControl::setWorkingDirectory(const FilePath &workingDirectory)
{
    d->data.runnable.workingDirectory = workingDirectory;
}

const Environment &RunControl::environment() const
{
    return d->data.runnable.environment;
}

void RunControl::setEnvironment(const Environment &environment)
{
    d->data.runnable.environment = environment;
}

const QVariantHash &RunControl::extraData() const
{
    return d->data.extraData;
}

void RunControl::setExtraData(const QVariantHash &extraData)
{
    d->data.extraData = extraData;
}

QString RunControl::displayName() const
{
    if (d->data.displayName.isEmpty())
        return d->data.runnable.command.executable().toUserOutput();
    return d->data.displayName;
}

void RunControl::setDisplayName(const QString &displayName)
{
    d->data.displayName = displayName;
}

void RunControl::setIcon(const Icon &icon)
{
    d->data.icon = icon;
}

Icon RunControl::icon() const
{
    return d->data.icon;
}

IDevice::ConstPtr RunControl::device() const
{
   return d->data.device;
}

BuildConfiguration *RunControl::buildConfiguration() const
{
    return d->data.buildConfiguration;
}

Target *RunControl::target() const
{
    return buildConfiguration() ? buildConfiguration()->target() : nullptr;
}

Project *RunControl::project() const
{
    return d->data.project;
}

Kit *RunControl::kit() const
{
    return d->data.kit;
}

const MacroExpander *RunControl::macroExpander() const
{
    return d->data.macroExpander;
}

const BaseAspect::Data *RunControl::aspectData(Id instanceId) const
{
    return d->data.aspectData.aspect(instanceId);
}

const BaseAspect::Data *RunControl::aspectData(BaseAspect::Data::ClassId classId) const
{
    return d->data.aspectData.aspect(classId);
}

Store RunControl::settingsData(Id id) const
{
    return d->data.settingsData.value(id);
}

QString RunControl::buildKey() const
{
    return d->data.buildKey;
}

FilePath RunControl::buildDirectory() const
{
    return d->data.buildDirectory;
}

Environment RunControl::buildEnvironment() const
{
    return d->data.buildEnvironment;
}

FilePath RunControl::targetFilePath() const
{
    return d->data.buildTargetInfo.targetFilePath;
}

FilePath RunControl::projectFilePath() const
{
    return d->data.buildTargetInfo.projectFilePath;
}

/*!
    A handle to the application process.

    This is typically a process id, but should be treated as
    opaque handle to the process controled by this \c RunControl.
*/

ProcessHandle RunControl::applicationProcessHandle() const
{
    return d->data.applicationProcessHandle;
}

void RunControl::setApplicationProcessHandle(const ProcessHandle &handle)
{
    if (d->data.applicationProcessHandle != handle) {
        d->data.applicationProcessHandle = handle;
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
    if (d->data.promptToStop)
        return d->data.promptToStop(optionalPrompt);

    const QString msg = Tr::tr("<html><head/><body><center><i>%1</i> is still running.<center/>"
                           "<center>Force it to quit?</center></body></html>").arg(displayName());
    return showPromptToStopDialog(Tr::tr("Application Still Running"), msg,
                                  Tr::tr("Force &Quit"), Tr::tr("&Keep Running"),
                                  optionalPrompt);
}

void RunControl::setPromptToStop(const std::function<bool (bool *)> &promptToStop)
{
    d->data.promptToStop = promptToStop;
}

void RunControlPrivate::startTaskTree()
{
    debugMessage("Starting...");
    QTC_CHECK(!m_taskTreeRunner.isRunning());

    const auto needPortsGatherer = [this] { return data.isPortsGatherer(); };

    const Group recipe {
        If (needPortsGatherer) >> Then {
            portsGathererRecipe().withCancel(q->canceler())
        },
        data.m_runRecipe
    };

    m_taskTreeRunner.start(recipe, {}, [this] {
        debugMessage("Done");
        emitStopped();
    });
}

void RunControlPrivate::emitStopped()
{
    q->setApplicationProcessHandle(ProcessHandle());
    emit q->stopped();
}

bool RunControl::isRunning() const
{
    return d->m_taskTreeRunner.isRunning();
}

bool RunControl::isStopped() const
{
    return !d->m_taskTreeRunner.isRunning();
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

void RunControlPrivate::debugMessage(const QString &msg) const
{
    qCDebug(statesLog()) << msg;
}

ProcessTask RunControl::processTask(const std::function<SetupResult(Process &)> &startModifier,
                                    const ProcessSetupConfig &config)
{
    const auto onSetup = [this, startModifier, config](Process &process) {
        process.setProcessChannelMode(appOutputPane().settings().mergeChannels
                                          ? QProcess::MergedChannels : QProcess::SeparateChannels);
        process.setCommand(commandLine());
        process.setWorkingDirectory(workingDirectory());
        process.setEnvironment(environment());

        if (startModifier) {
            const SetupResult result = startModifier(process);
            if (result != SetupResult::Continue)
                return result;
        }

        const CommandLine command = process.commandLine();
        const bool isDesktop = command.executable().isLocal();
        if (isDesktop && command.isEmpty()) {
            postMessage(Tr::tr("No executable specified."), ErrorMessageFormat);
            return SetupResult::StopWithError;
        }

        bool useTerminal = false;
        if (auto terminalAspect = aspectData<TerminalAspect>())
            useTerminal = terminalAspect->useTerminal;

        const Environment environment = process.environment();
        process.setTerminalMode(useTerminal ? Utils::TerminalMode::Run : Utils::TerminalMode::Off);
        process.setReaperTimeout(
            std::chrono::seconds(ProjectExplorerSettings::get(this).reaperTimeoutInSeconds()));

        postMessage(Tr::tr("Starting %1...").arg(command.displayName()), NormalMessageFormat);
        if (isPrintEnvironmentEnabled()) {
            postMessage(Tr::tr("Environment:"), NormalMessageFormat);
            environment.forEachEntry([this](const QString &key, const QString &value, bool enabled) {
                if (enabled)
                    postMessage(key + '=' + value, StdOutFormat);
            });
            postMessage({}, StdOutFormat);
        }

        CommandLine cmdLine = process.commandLine();
        Environment env = process.environment();

        if (cmdLine.executable().isLocal()) {
            // Running locally.
            bool runAsRoot = false;
            if (auto runAsRootAspect = aspectData<RunAsRootAspect>())
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
            postMessage(Tr::tr("Cannot run: No command given."), NormalMessageFormat);
            return SetupResult::StopWithError;
        }

        QVariantHash extra = extraData();
        QString shellName = displayName();

        if (buildConfiguration()) {
            if (BuildConfiguration *buildConfig = buildConfiguration())
                shellName += " - " + buildConfig->displayName();
        }

        extra[TERMINAL_SHELL_NAME] = shellName;

        process.setCommand(cmdLine);
        process.setEnvironment(env);
        process.setExtraData(extra);
        process.setForceDefaultErrorModeOnWindows(true);

        QObject::connect(&process, &Process::started, [this, process = &process] {
            const bool isDesktop = process->commandLine().executable().isLocal();
            if (isDesktop) {
                // Console processes only know their pid after being started
                ProcessHandle pid{process->processId()};
                setApplicationProcessHandle(pid);
                pid.activate();
            }
        });
        QObject::connect(&process, &Process::readyReadStandardError, this, [this, process = &process] {
            postMessage(process->readAllStandardError(), StdErrFormat, false);
        });
        QObject::connect(&process, &Process::readyReadStandardOutput, this, [this, config, process = &process] {
            if (config.suppressDefaultStdOutHandling)
                emit stdOutData(process->readAllRawStandardOutput());
            else
                postMessage(process->readAllStandardOutput(), StdOutFormat, false);
        });
        QObject::connect(&process, &Process::stoppingForcefully, this, [this] {
            postMessage(Tr::tr("Stopping process forcefully..."), NormalMessageFormat);
        });

        if (WinDebugInterface::instance()) {
            QObject::connect(WinDebugInterface::instance(), &WinDebugInterface::cannotRetrieveDebugOutput,
                             &process, [this, process = &process] {
                QObject::disconnect(WinDebugInterface::instance(), nullptr, process, nullptr);
                postMessage(Tr::tr("Cannot retrieve debugging output.")
                                            + QLatin1Char('\n'), ErrorMessageFormat);
            });

            QObject::connect(WinDebugInterface::instance(), &WinDebugInterface::debugOutput,
                             &process, [this, process = &process](qint64 pid, const QList<QString> &messages) {
                if (process->processId() != pid)
                    return;
                for (const QString &message : messages)
                    postMessage(message, DebugFormat);
            });
        }
        if (config.setupCanceler) {
            QObject::connect(this, &RunControl::canceled, &process, [this, process = &process] {
                handleProcessCancellation(process);
            });
        }
        return SetupResult::Continue;
    };

    const auto onDone = [this](const Process &process) {
        postMessage(process.exitMessage(), NormalMessageFormat);
        if (process.usesTerminal()) {
            Process &mutableProcess = const_cast<Process &>(process);
            auto processInterface = mutableProcess.takeProcessInterface();
            if (processInterface)
                processInterface->setParent(this);
        }
    };

    return ProcessTask(onSetup, onDone);
}

// Output parser factories

static QList<std::function<OutputLineParser *(BuildConfiguration *)>> g_outputParserFactories;

QList<OutputLineParser *> createOutputParsers(BuildConfiguration *bc)
{
    QList<OutputLineParser *> formatters;
    for (auto factory : std::as_const(g_outputParserFactories)) {
        if (OutputLineParser *parser = factory(bc))
            formatters << parser;
    }
    return formatters;
}

void addOutputParserFactory(const std::function<OutputLineParser *(Target *)> &factory)
{
    g_outputParserFactories.append(
        [factory](BuildConfiguration *bc) { return factory(bc ? bc->target() : nullptr); });
}

void addOutputParserFactory(const std::function<OutputLineParser *(BuildConfiguration *)> &factory)
{
    g_outputParserFactories.append(factory);
}

// ProcessRunnerFactory

ProcessRunnerFactory::ProcessRunnerFactory(const QList<Id> &runConfigs)
{
    setId("ProcessRunnerFactory");
    setRecipeProducer([](RunControl *runControl) { return runControl->processRecipe(runControl->processTask()); });
    addSupportedRunMode(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    setSupportedRunConfigs(runConfigs);
}

Canceler RunControl::canceler()
{
    return [this] { return std::make_pair(this, &RunControl::canceled); };
}

void RunControl::handleProcessCancellation(Process *process)
{
    postMessage(Tr::tr("Requesting process to stop..."), NormalMessageFormat);
    process->stop();
    QTimer::singleShot(
        2 * std::chrono::seconds(ProjectExplorerSettings::get(this).reaperTimeoutInSeconds()),
        process,
        [this, process] {
            postMessage(Tr::tr("Process unexpectedly did not finish."), ErrorMessageFormat);
            if (!process->commandLine().executable().isLocal())
                postMessage(Tr::tr("Connectivity lost?"), ErrorMessageFormat);
            process->kill();
            emit process->done();
        });
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
                        if (factory->canCreate(runMode, device, runConfig))
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

#include "runcontrol.moc"

#endif // WITH_TESTS
