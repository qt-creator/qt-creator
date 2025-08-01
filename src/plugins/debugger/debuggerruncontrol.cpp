// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggerruncontrol.h"

#include "console/console.h"
#include "debuggeractions.h"
#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggerkitaspect.h"
#include "debuggermainwindow.h"
#include "debuggertr.h"
#include "enginemanager.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/taskhub.h>

#include <remotelinux/remotelinux_constants.h>

#include <qtsupport/baseqtversion.h>

#include <solutions/tasking/barrier.h>
#include <solutions/tasking/conditional.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>
#include <utils/temporaryfile.h>
#include <utils/winutils.h>

using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

enum { debug = 0 };

namespace Debugger {
namespace Internal {

DebuggerEngine *createCdbEngine();
DebuggerEngine *createGdbEngine();
DebuggerEngine *createPdbEngine();
DebuggerEngine *createQmlEngine();
DebuggerEngine *createLldbEngine();
DebuggerEngine *createUvscEngine();
DebuggerEngine *createDapEngine(Id runMode = ProjectExplorer::Constants::NO_RUN_MODE);

static QString noEngineMessage()
{
   return Tr::tr("Unable to create a debugging engine.");
}

static QString noDebuggerInKitMessage()
{
   return Tr::tr("The kit does not have a debugger set.");
}

class EnginesDriver : public QObject
{
    Q_OBJECT

public:
    ~EnginesDriver() { clearEngines(); }

    Result<> setupEngines(RunControl *runControl, const DebuggerRunParameters &rp);
    Result<> checkBreakpoints() const;
    QString debuggerName() const
    {
        return Utils::transform<QStringList>(m_engines, &DebuggerEngine::objectName).join(" ");
    }
    QString startParameters() const
    {
        return m_engines.isEmpty() ? QString() : m_engines.first()->formatStartParameters();
    }
    bool isRunning() const { return m_runningEngines > 0; }
    void start();
    void stop()
    {
        Utils::reverseForeach(m_engines, [](DebuggerEngine *engine) { engine->quitDebugger(); });
    }
    void showMessage(const QString &msg, int channel = LogDebug, int timeout = -1);

signals:
    void done(DoneResult result);
    void interruptTerminalRequested();
    void kickoffTerminalProcessRequested();
    void started();

private:
    void clearEngines()
    {
        qDeleteAll(m_engines);
        m_engines.clear();
    }

    RunControl *m_runControl = nullptr;
    QList<QPointer<Internal::DebuggerEngine>> m_engines;
    int m_runningEngines = 0;
    int m_snapshotCounter = 0;
};

} // namespace Internal

struct DebuggerData
{
    ~DebuggerData() {
        if (terminalProcess && runControl) {
            auto processInterface = terminalProcess->takeProcessInterface();
            if (processInterface)
                processInterface->setParent(runControl);
        }
    }
    DebuggerRunParameters runParameters;
    RunControl *runControl = nullptr;
    EnginesDriver enginesDriver = {};
    FilePath tempCoreFile = {};
    std::unique_ptr<Process> terminalProcess = {};
};

ExecutableItem coreFileRecipe(const Storage<DebuggerData> &storage)
{
    const Storage<QFile> fileStorage; // tempCoreFile

    const auto onSetup = [storage, fileStorage](Process &process) {
        const FilePath coreFile = storage->runParameters.coreFile();
        if (!coreFile.endsWith(".gz") && !coreFile.endsWith(".lzo"))
            return SetupResult::StopWithSuccess;

        {
            TemporaryFile tmp("tmpcore-XXXXXX");
            QTC_CHECK(tmp.open());
            storage->tempCoreFile = tmp.filePath();
        }
        QFile *tempCoreFile = fileStorage.activeStorage();
        process.setWorkingDirectory(TemporaryDirectory::masterDirectoryFilePath());
        const QString msg = Tr::tr("Unpacking core file to %1");
        storage->runControl->postMessage(msg.arg(storage->tempCoreFile.toUserOutput()), LogMessageFormat);

        if (coreFile.endsWith(".lzo")) {
            process.setCommand({"lzop", {"-o", storage->tempCoreFile.path(), "-x", coreFile.path()}});
        } else { // ".gz"
            tempCoreFile->setFileName(storage->tempCoreFile.path());
            QTC_CHECK(tempCoreFile->open(QFile::WriteOnly));
            QObject::connect(&process, &Process::readyReadStandardOutput, &process,
                             [tempCoreFile, processPtr = &process] {
                tempCoreFile->write(processPtr->readAllRawStandardOutput());
            });
            process.setCommand({"gzip", {"-c", "-d", coreFile.path()}});
        }
        return SetupResult::Continue;
    };

    const auto onDone = [storage, fileStorage](DoneWith result) {
        if (result == DoneWith::Success) {
            storage->runParameters.setCoreFilePath(storage->tempCoreFile);
        } else {
            storage->runControl->postMessage("Error unpacking "
                + storage->runParameters.coreFile().toUserOutput(), ErrorMessageFormat);
        }
        if (fileStorage->isOpen())
            fileStorage->close();
    };

    return Group {
        fileStorage,
        ProcessTask(onSetup, onDone)
    };
}

ExecutableItem terminalRecipe(const Storage<DebuggerData> &storage, const StoredBarrier &barrier)
{
    const auto onSetup = [storage, barrier] {
        DebuggerRunParameters &runParameters = storage->runParameters;
        const bool useCdbConsole = runParameters.cppEngineType() == CdbEngineType
                                   && (runParameters.startMode() == StartInternal
                                       || runParameters.startMode() == StartExternal)
                                   && settings().useCdbConsole();
        if (useCdbConsole)
            runParameters.setUseTerminal(false);
        if (!runParameters.useTerminal()) {
            barrier->advance();
            return;
        }

        Process *process = new Process;
        storage->terminalProcess.reset(process);
        ProcessRunData stub = runParameters.inferior();
        if (runParameters.runAsRoot()) {
            process->setRunAsRoot(true);
            RunControl::provideAskPassEntry(stub.environment);
        }
        process->setTerminalMode(TerminalMode::Debug);
        process->setRunData(stub);

        QObject::connect(process, &Process::started, process,
                         [runParameters = &runParameters, process, barrier = barrier.activeStorage()] {
            runParameters->setApplicationPid(process->processId());
            runParameters->setApplicationMainThreadId(process->applicationMainThreadId());
            barrier->advance();
        });

        EnginesDriver *driver = &storage->enginesDriver;
        QObject::connect(driver, &EnginesDriver::interruptTerminalRequested,
                         process, &Process::interrupt);
        QObject::connect(driver, &EnginesDriver::kickoffTerminalProcessRequested,
                         process, &Process::kickoffProcess);
        process->start();
    };

    return Sync(onSetup);
}

ExecutableItem fixupParamsRecipe(const Storage<DebuggerData> &storage)
{
    return Sync([storage] {
        RunControl *runControl = storage->runControl;
        DebuggerRunParameters &runParameters = storage->runParameters;
        TaskHub::clearTasks(Constants::TASK_CATEGORY_DEBUGGER_RUNTIME);

        if (runControl->usesDebugChannel())
            runParameters.setRemoteChannel(runControl->debugChannel().toString());

        if (runControl->usesQmlChannel()) {
            runParameters.setQmlServer(runControl->qmlChannel());
            if (runParameters.isAddQmlServerInferiorCmdArgIfNeeded()
                && runParameters.isQmlDebugging()
                && runParameters.isCppDebugging()) {

                const int qmlServerPort = runParameters.qmlServer().port();
                QTC_ASSERT(qmlServerPort > 0, return false);
                const QString mode = QString("port:%1").arg(qmlServerPort);

                auto inferior = runParameters.inferior();
                CommandLine cmd{inferior.command.executable()};
                cmd.addArg(qmlDebugCommandLineArguments(QmlDebuggerServices, mode, true));
                cmd.addArgs(runParameters.inferior().command.arguments(), CommandLine::Raw);
                inferior.command = cmd;
                runParameters.setInferior(inferior);
            }
        }

        // User canceled input dialog asking for executable when working on library project.
        if (runParameters.startMode() == StartInternal
            && runParameters.inferior().command.isEmpty()
            && runParameters.interpreter().isEmpty()) {
            runControl->postMessage(Tr::tr("No executable specified."), ErrorMessageFormat);
            return false;
        }

        // FIXME: Disabled due to Android. Make Android device report available ports instead.
        // int neededPorts = 0;
        // if (useQmlDebugger())
        //     ++neededPorts;
        // if (useCppDebugger())
        //     ++neededPorts;
        // if (neededPorts > device()->freePorts().count()) {
        //    reportFailure(Tr::tr("Cannot debug: Not enough free ports available."));
        //    return;
        // }

        if (Result<> res = runParameters.fixupParameters(runControl); !res) {
            runControl->postMessage(res.error(), ErrorMessageFormat);
            return false;
        }

        if (runParameters.cppEngineType() == CdbEngineType
            && Utils::is64BitWindowsBinary(runParameters.inferior().command.executable())
            && !Utils::is64BitWindowsBinary(runParameters.debugger().command.executable())) {
            runControl->postMessage(Tr::tr(
                    "%1 is a 64 bit executable which can not be debugged by a 32 bit Debugger.\n"
                    "Please select a 64 bit Debugger in the kit settings for this kit.")
                    .arg(runParameters.inferior().command.executable().toUserOutput()),
                    ErrorMessageFormat);
            return false;
        }

        Utils::globalMacroExpander()->registerFileVariables(
            "DebuggedExecutable", Tr::tr("Debugged executable"),
            [exec = runParameters.inferior().command.executable()] { return exec; });

        runControl->setDisplayName(runParameters.displayName());

        if (runParameters.isQmlDebugging())
            runParameters.populateQmlFileFinder(runControl);

        if (auto interpreterAspect = runControl->aspectData<FilePathAspect>()) {
            if (auto mainScriptAspect = runControl->aspectData<MainScriptAspect>()) {
                const FilePath mainScript = mainScriptAspect->filePath;
                const FilePath interpreter = interpreterAspect->filePath;
                if (!interpreter.isEmpty() && mainScript.endsWith(".py")) {
                    runParameters.setMainScript(mainScript);
                    runParameters.setInterpreter(interpreter);
                }
            }
        }

        return true;
    });
}

ProcessTask debugServerTask(const Storage<DebuggerData> &storage)
{
    const auto onSetup = [storage](Process &process) {
        process.setUtf8Codec();
        DebuggerRunParameters &runParameters = storage->runParameters;
        RunControl *runControl = storage->runControl;
        CommandLine commandLine = runParameters.inferior().command;
        CommandLine cmd;

        if (runControl->usesQmlChannel() && !runControl->usesDebugChannel()) {
            // FIXME: Case should not happen?
            cmd.setExecutable(commandLine.executable());
            cmd.addArg(qmlDebugTcpArguments(QmlDebuggerServices, runControl->qmlChannel()));
            cmd.addArgs(commandLine.arguments(), CommandLine::Raw);
        } else {
            cmd.setExecutable(runControl->device()->deviceToolPath(Constants::DEBUGSERVER_TOOL_ID));

            if (cmd.isEmpty()) {
                if (runControl->device()->osType() == Utils::OsTypeMac) {
                    const FilePath debugServerLocation = runControl->device()->filePath(
                        "/Applications/Xcode.app/Contents/SharedFrameworks/LLDB.framework/"
                        "Resources/debugserver");

                    if (debugServerLocation.isExecutableFile()) {
                        cmd.setExecutable(debugServerLocation);
                    } else {
                        // TODO: In the future it is expected that the debugserver will be
                        // replaced by lldb-server. Remove the check for debug server at that point.
                        const FilePath lldbserver
                            = runControl->device()->filePath("lldb-server").searchInPath();
                        if (lldbserver.isExecutableFile())
                            cmd.setExecutable(lldbserver);
                    }
                } else {
                    const FilePath gdbServerPath
                        = runControl->device()->filePath("gdbserver").searchInPath();
                    FilePath lldbServerPath
                        = runControl->device()->filePath("lldb-server").searchInPath();

                    // TODO: Which one should we prefer?
                    if (gdbServerPath.isExecutableFile())
                        cmd.setExecutable(gdbServerPath);
                    else if (lldbServerPath.isExecutableFile()) {
                        // lldb-server will fail if we start it through a link.
                        // see: https://github.com/llvm/llvm-project/issues/61955
                        //
                        // So we first search for the real executable.

                        // This is safe because we already checked that the file is executable.
                        while (lldbServerPath.isSymLink())
                            lldbServerPath = lldbServerPath.symLinkTarget();

                        cmd.setExecutable(lldbServerPath);
                    }
                }
            }
            QTC_ASSERT(runControl->usesDebugChannel(), return SetupResult::StopWithError);
            if (cmd.executable().baseName().contains("lldb-server")) {
                cmd.addArg("platform");
                cmd.addArg("--listen");
                cmd.addArg(QString("*:%1").arg(runControl->debugChannel().port()));
                cmd.addArg("--server");
            } else if (cmd.executable().baseName() == "debugserver") {
                const QString ipAndPort("`echo $SSH_CLIENT | cut -d ' ' -f 1`:%1");
                cmd.addArgs(ipAndPort.arg(runControl->debugChannel().port()), CommandLine::Raw);

                if (runParameters.serverAttachPid().isValid())
                    cmd.addArgs({"--attach", QString::number(runParameters.serverAttachPid().pid())});
                else
                    cmd.addCommandLineAsArgs(runControl->commandLine());
            } else {
                // Something resembling gdbserver
                if (runParameters.serverUseMulti())
                    cmd.addArg("--multi");
                if (runParameters.serverAttachPid().isValid())
                    cmd.addArg("--attach");

                const auto port = runControl->debugChannel().port();
                cmd.addArg(QString(":%1").arg(port));

                if (runControl->device()->sshForwardDebugServerPort()) {
                    QVariantHash extraData;
                    extraData[RemoteLinux::Constants::SshForwardPort] = port;
                    extraData[RemoteLinux::Constants::DisableSharing] = true;
                    process.setExtraData(extraData);
                }

                if (runParameters.serverAttachPid().isValid())
                    cmd.addArg(QString::number(runParameters.serverAttachPid().pid()));
            }
        }

        if (auto terminalAspect = runControl->aspectData<TerminalAspect>()) {
            const bool useTerminal = terminalAspect->useTerminal;
            process.setTerminalMode(useTerminal ? TerminalMode::Run : TerminalMode::Off);
        }

        process.setCommand(cmd);
        process.setWorkingDirectory(runParameters.inferior().workingDirectory);

        QObject::connect(&process, &Process::readyReadStandardOutput, runControl,
                         [runControl, process = &process] {
            runControl->postMessage(process->readAllStandardOutput(), StdOutFormat, false);
        });

        QObject::connect(&process, &Process::readyReadStandardError, runControl,
                         [runControl, process = &process] {
            runControl->postMessage(process->readAllStandardError(), StdErrFormat, false);
        });

        return SetupResult::Continue;
    };

    const auto onDone = [storage](const Process &process) {
        storage->runControl->postMessage(process.errorString(), ErrorMessageFormat);
    };

    return ProcessTask(onSetup, onDone, CallDone::OnError);
}

static ExecutableItem doneAwaiter(const Storage<DebuggerData> &storage)
{
    return BarrierTask([storage](Barrier &barrier) {
        QObject::connect(&storage->enginesDriver, &EnginesDriver::done, &barrier, &Barrier::stopWithResult,
                         static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    });
}

ExecutableItem startEnginesRecipe(const Storage<DebuggerData> &storage)
{
    const auto setupEngines = [storage] {
        RunControl *runControl = storage->runControl;
        DebuggerRunParameters &runParameters = storage->runParameters;
        EnginesDriver *driver = &storage->enginesDriver;
        if (Result<> res = driver->setupEngines(runControl, runParameters); !res) {
            runControl->postMessage(res.error(), ErrorMessageFormat);
            return false;
        }
        if (Result<> res = driver->checkBreakpoints(); !res) {
            driver->showMessage(res.error(), LogWarning);
            if (settings().showUnsupportedBreakpointWarning()) {
                bool doNotAskAgain = false;
                CheckableDecider decider(&doNotAskAgain);
                CheckableMessageBox::information(Tr::tr("Debugger"), res.error(), decider, QMessageBox::Ok);
                if (doNotAskAgain) {
                    settings().showUnsupportedBreakpointWarning.setValue(false);
                    settings().showUnsupportedBreakpointWarning.writeSettings();
                }
            }
        }
        runControl->postMessage(Tr::tr("Debugging %1 ...").arg(runParameters.inferior().command.toUserOutput()),
                                NormalMessageFormat);
        const QString message = Tr::tr("Starting debugger \"%1\" for ABI \"%2\"...")
                                .arg(driver->debuggerName(), runParameters.toolChainAbi().toString());
        DebuggerMainWindow::showStatusMessage(message, 10000);
        driver->showMessage(driver->startParameters(), LogDebug);
        driver->showMessage(DebuggerSettings::dump(), LogDebug);

        driver->start();
        QObject::connect(driver, &EnginesDriver::started, runControl, &RunControl::reportStarted);
        return true;
    };

    return If (setupEngines) >> Then {
        doneAwaiter(storage)
    };
}

static ExecutableItem terminalAwaiter(const Storage<DebuggerData> &storage)
{
    return BarrierTask([storage](Barrier &barrier) {
        QObject::connect(storage->terminalProcess.get(), &Process::done, &barrier, &Barrier::advance,
                         static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    });
}

ExecutableItem finalizeRecipe(const Storage<DebuggerData> &storage)
{
    const auto isRunning = [storage] { return storage->enginesDriver.isRunning(); };
    const auto isTerminalRunning = [storage] {
        return storage->terminalProcess && storage->terminalProcess->isRunning();
    };

    return Group {
        continueOnError,
        If (isRunning) >> Then {
            Sync([storage] { storage->enginesDriver.stop(); }),
            doneAwaiter(storage)
        },
        If (isTerminalRunning) >> Then {
            terminalAwaiter(storage)
        }
    };
}

static Result<QList<QPointer<Internal::DebuggerEngine>>> createEngines(
    RunControl *runControl, const DebuggerRunParameters &rp)
{
    if (auto dapEngine = createDapEngine(runControl->runMode()))
        return QList<QPointer<Internal::DebuggerEngine>>{dapEngine};

    QList<QPointer<Internal::DebuggerEngine>> engines;
    if (rp.isCppDebugging()) {
        switch (rp.cppEngineType()) {
        case GdbEngineType:
            engines << createGdbEngine();
            break;
        case CdbEngineType:
            if (!HostOsInfo::isWindowsHost())
                return make_unexpected(Tr::tr("Unsupported CDB host system."));
            engines << createCdbEngine();
            break;
        case LldbEngineType:
            engines << createLldbEngine();
            break;
        case GdbDapEngineType:
            engines << createDapEngine(ProjectExplorer::Constants::DAP_GDB_DEBUG_RUN_MODE);
            break;
        case LldbDapEngineType:
            engines << createDapEngine(ProjectExplorer::Constants::DAP_LLDB_DEBUG_RUN_MODE);
            break;
        case UvscEngineType:
            engines << createUvscEngine();
            break;
        default:
            if (!rp.isQmlDebugging()) {
                return make_unexpected(noEngineMessage() + '\n' +
                                       Tr::tr("Specify Debugger settings in Projects > Run."));
            }
            break; // Can happen for pure Qml.
        }
    }

    if (rp.isPythonDebugging())
        engines << createPdbEngine();

    if (rp.isQmlDebugging())
        engines << createQmlEngine();

    if (engines.isEmpty()) {
        QString msg = noEngineMessage();
        if (!DebuggerKitAspect::debugger(runControl->kit()))
            msg += '\n' + noDebuggerInKitMessage();
        return make_unexpected(msg);
    }
    return engines;
}

static int newRunId()
{
    static int toolRunCount = 0;
    if (EngineManager::engines().isEmpty())
        toolRunCount = 0;
    return ++toolRunCount;
}

Result<> EnginesDriver::setupEngines(RunControl *runControl, const DebuggerRunParameters &rp)
{
    m_runControl = runControl;
    clearEngines();
    const auto engines = createEngines(runControl, rp);
    if (!engines)
        return ResultError(engines.error());

    m_engines = *engines;
    const QString runId = QString::number(newRunId());
    for (auto engine : std::as_const(m_engines)) {
        if (engine != m_engines.first())
            engine->setSecondaryEngine();
        engine->setRunParameters(rp);
        engine->setRunId(runId);
        for (auto companion : std::as_const(m_engines)) {
            if (companion != engine)
                engine->addCompanionEngine(companion);
        }
        engine->setDevice(m_runControl->device());
    }

    return ResultOk;
}

Result<> EnginesDriver::checkBreakpoints() const
{
    QStringList unhandledIds;
    bool hasQmlBreakpoints = false;
    for (const GlobalBreakpoint &gbp : BreakpointManager::globalBreakpoints()) {
        if (gbp->isEnabled()) {
            const BreakpointParameters &bp = gbp->requestedParameters();
            hasQmlBreakpoints = hasQmlBreakpoints || bp.isQmlFileAndLineBreakpoint();
            const auto engineAcceptsBp = [bp](const DebuggerEngine *engine) {
                return engine->acceptsBreakpoint(bp);
            };
            if (!Utils::anyOf(m_engines, engineAcceptsBp))
                unhandledIds.append(gbp->displayName());
        }
    }

    if (unhandledIds.isEmpty())
        return ResultOk;

    QString warningMessage = Debugger::Tr::tr(
                                 "Some breakpoints cannot be handled by the debugger "
                                 "languages currently active, and will be ignored.<p>"
                                 "Affected are breakpoints %1")
                                 .arg(unhandledIds.join(", "));
    if (hasQmlBreakpoints) {
        warningMessage += "<p>"
                          + Debugger::Tr::tr(
                              "QML debugging needs to be enabled both in the Build "
                              "and the Run settings.");
    }
    return ResultError(warningMessage);
}

void EnginesDriver::start()
{
    const QString runId = QString::number(newRunId());
    for (auto engine : std::as_const(m_engines)) {
        connect(engine, &DebuggerEngine::interruptTerminalRequested,
                this, &EnginesDriver::interruptTerminalRequested);
        connect(engine, &DebuggerEngine::kickoffTerminalProcessRequested,
                this, &EnginesDriver::kickoffTerminalProcessRequested);
        connect(engine, &DebuggerEngine::requestRunControlStop, m_runControl, &RunControl::initiateStop);

        connect(engine, &DebuggerEngine::engineStarted, this, [this, engine] {
            ++m_runningEngines;
            if (engine->isPrimaryEngine()) {
                EngineManager::activateDebugMode();
                emit started();
            }
        });

        connect(engine, &DebuggerEngine::engineFinished, this, [this, engine] {
            engine->prepareForRestart();
            if (--m_runningEngines == 0) {
                const QString cmd = engine->runParameters().inferior().command.toUserOutput();
                const QString msg
                    = engine->runParameters().exitCode() // Main engine.
                          ? Debugger::Tr::tr("Debugging of %1 has finished with exit code %2.")
                                .arg(cmd)
                                .arg(*engine->runParameters().exitCode())
                          : Debugger::Tr::tr("Debugging of %1 has finished.").arg(cmd);
                m_runControl->postMessage(msg, NormalMessageFormat);
                emit done(engine->runParameters().exitCode() ? DoneResult::Error : DoneResult::Success);
            }
        });
        connect(engine, &DebuggerEngine::postMessageRequested, m_runControl, &RunControl::postMessage);

        if (engine->isPrimaryEngine()) {
            connect(engine, &DebuggerEngine::attachToCoreRequested, this, [this](const FilePath &coreFile) {
                auto rc = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
                rc->copyDataFromRunControl(m_runControl);
                auto name = QString(
                    Debugger::Tr::tr("%1 - Snapshot %2")
                        .arg(m_runControl->displayName())
                        .arg(++m_snapshotCounter));
                DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(rc);
                rp.setStartMode(AttachToCore);
                rp.setCloseMode(DetachAtClose);
                rp.setDisplayName(name);
                rp.setCoreFilePath(coreFile);
                rp.setSnapshot(true);
                rc->setRunRecipe(debuggerRecipe(rc, rp));
                rc->start();
            });
        }
    }
    Utils::reverseForeach(m_engines, [](DebuggerEngine *engine) { engine->start(); });
}

void EnginesDriver::showMessage(const QString &msg, int channel, int timeout)
{
    if (channel == ConsoleOutput)
        debuggerConsole()->printItem(ConsoleItem::DefaultType, msg);

    QTC_ASSERT(!m_engines.isEmpty(), qDebug() << msg; return);

    for (auto engine : std::as_const(m_engines))
        engine->showMessage(msg, channel, timeout);
    switch (channel) {
    case AppOutput:
        m_runControl->postMessage(msg, StdOutFormat);
        break;
    case AppError:
        m_runControl->postMessage(msg, StdErrFormat);
        break;
    case AppStuff:
        m_runControl->postMessage(msg, DebugFormat);
        break;
    default:
        break;
    }
}

Group debuggerRecipe(RunControl *runControl, const DebuggerRunParameters &initialParameters,
                     const std::function<void(DebuggerRunParameters &)> &parametersModifier)
{
    const Storage<DebuggerData> storage{initialParameters, runControl};

    const auto onSetup = [storage, parametersModifier] {
        storage->runParameters.setAttachPid(storage->runControl->attachPid());
        if (parametersModifier)
            parametersModifier(storage->runParameters);
    };

    const auto terminalKicker = [storage](const StoredBarrier &barrier) {
        return terminalRecipe(storage, barrier);
    };

    const auto onDone = [storage] {
        if (storage->tempCoreFile.exists())
            storage->tempCoreFile.removeFile();
        if (storage->runParameters.isSnapshot() && !storage->runParameters.coreFile().isEmpty())
            storage->runParameters.coreFile().removeFile();
    };

    const auto useDebugServer = [storage] {
        return storage->runControl->usesDebugChannel() && !storage->runParameters.skipDebugServer();
    };

    const ExecutableItem enginesRecipe = startEnginesRecipe(storage);

    return {
        storage,
        continueOnError,
        onGroupSetup(onSetup),
        Group {
            fixupParamsRecipe(storage),
            coreFileRecipe(storage),
            When (terminalKicker) >> Do {
                If (useDebugServer) >> Then {
                    When (debugServerTask(storage), &Process::started) >> Do {
                        enginesRecipe
                    }
                } >> Else {
                    enginesRecipe
                }
            }
        }.withCancel(runControl->canceler()),
        finalizeRecipe(storage),
        onGroupDone(onDone)
    };
}

class DebuggerRunWorkerFactory final : public RunWorkerFactory
{
public:
    DebuggerRunWorkerFactory()
    {
        setId(Constants::DEBUGGER_RUN_FACTORY);
        setRecipeProducer([](RunControl *runControl) {
            return debuggerRecipe(runControl, DebuggerRunParameters::fromRunControl(runControl));
        });

        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        addSupportedRunMode(ProjectExplorer::Constants::DAP_CMAKE_DEBUG_RUN_MODE);
        addSupportedRunMode(ProjectExplorer::Constants::DAP_GDB_DEBUG_RUN_MODE);
        addSupportedRunMode(ProjectExplorer::Constants::DAP_LLDB_DEBUG_RUN_MODE);

        addSupportedDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
        addSupportedDeviceType(ProjectExplorer::Constants::DOCKER_DEVICE_TYPE);

        addSupportForLocalRunConfigs();
    }
};

void setupDebuggerRunWorker()
{
    static DebuggerRunWorkerFactory theDebuggerRunWorkerFactory;
}

} // Debugger

#include "debuggerruncontrol.moc"
