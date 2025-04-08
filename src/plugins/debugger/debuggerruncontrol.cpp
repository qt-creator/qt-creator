// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggerruncontrol.h"

#include "debuggermainwindow.h"
#include "debuggertr.h"

#include "console/console.h"
#include "debuggeractions.h"
#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggerkitaspect.h"
#include "breakhandler.h"
#include "enginemanager.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/deviceprocessesdialog.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/environmentaspect.h> // For the environment
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/qmldebugcommandlinearguments.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <remotelinux/remotelinux_constants.h>

#include <solutions/tasking/barrier.h>
#include <solutions/tasking/conditional.h>
#include <solutions/tasking/tasktreerunner.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/portlist.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>
#include <utils/temporaryfile.h>
#include <utils/url.h>
#include <utils/winutils.h>

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/messagebox.h>

using namespace Core;
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
DebuggerEngine *createDapEngine(Utils::Id runMode = ProjectExplorer::Constants::NO_RUN_MODE);

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

class DebuggerRunToolPrivate
{
public:
    ExecutableItem coreFileRecipe(const Storage<FilePath> &tempCoreFileStorage);
    ExecutableItem terminalRecipe(const SingleBarrier &barrier,
                                  const Storage<EnginesDriver> &driverStorage);
    ExecutableItem fixupParamsRecipe();
    ExecutableItem debugServerRecipe(const SingleBarrier &barrier);
    ExecutableItem startEnginesRecipe(const Storage<EnginesDriver> &driverStorage);
    ExecutableItem finishEnginesRecipe(const Storage<EnginesDriver> &driverStorage);

    DebuggerRunTool *q = nullptr;
    DebuggerRunParameters m_runParameters;

    // TaskTree
    Tasking::TaskTreeRunner m_taskTreeRunner = {};
};

} // namespace Internal

ExecutableItem DebuggerRunToolPrivate::coreFileRecipe(const Storage<FilePath> &tempCoreFileStorage)
{
    const FilePath coreFile = m_runParameters.coreFile();
    if (!coreFile.endsWith(".gz") && !coreFile.endsWith(".lzo"))
        return Group {};

    const Storage<QFile> storage; // tempCoreFile

    const auto onSetup = [this, storage, coreFile, tempCoreFileStorage](Process &process) {
        {
            TemporaryFile tmp("tmpcore-XXXXXX");
            QTC_CHECK(tmp.open());
            *tempCoreFileStorage = FilePath::fromString(tmp.fileName());
        }
        QFile *tempCoreFile = storage.activeStorage();
        process.setWorkingDirectory(TemporaryDirectory::masterDirectoryFilePath());
        const QString msg = Tr::tr("Unpacking core file to %1");
        q->appendMessage(msg.arg(tempCoreFileStorage->toUserOutput()), LogMessageFormat);

        if (coreFile.endsWith(".lzo")) {
            process.setCommand({"lzop", {"-o", tempCoreFileStorage->path(), "-x", coreFile.path()}});
        } else { // ".gz"
            tempCoreFile->setFileName(tempCoreFileStorage->path());
            QTC_CHECK(tempCoreFile->open(QFile::WriteOnly));
            QObject::connect(&process, &Process::readyReadStandardOutput, &process,
                             [tempCoreFile, processPtr = &process] {
                tempCoreFile->write(processPtr->readAllRawStandardOutput());
            });
            process.setCommand({"gzip", {"-c", "-d", coreFile.path()}});
        }
    };

    const auto onDone = [this, storage, tempCoreFileStorage](DoneWith result) {
        if (result == DoneWith::Success)
            m_runParameters.setCoreFilePath(*tempCoreFileStorage);
        else
            q->reportFailure("Error unpacking " + m_runParameters.coreFile().toUserOutput());
        if (storage->isOpen())
            storage->close();
    };

    return Group {
        storage,
        ProcessTask(onSetup, onDone)
    };
}

ExecutableItem DebuggerRunToolPrivate::terminalRecipe(const SingleBarrier &barrier,
                                                      const Storage<EnginesDriver> &driverStorage)
{
    const auto useTerminal = [this] {
        const bool useCdbConsole = m_runParameters.cppEngineType() == CdbEngineType
                                   && (m_runParameters.startMode() == StartInternal
                                       || m_runParameters.startMode() == StartExternal)
                                   && settings().useCdbConsole();
        if (useCdbConsole)
            m_runParameters.setUseTerminal(false);
        return m_runParameters.useTerminal();
    };

    const auto onSetup = [this, barrier, driverStorage](Process &process) {
        ProcessRunData stub = m_runParameters.inferior();
        if (m_runParameters.runAsRoot()) {
            process.setRunAsRoot(true);
            RunControl::provideAskPassEntry(stub.environment);
        }
        process.setTerminalMode(TerminalMode::Debug);
        process.setRunData(stub);

        QObject::connect(&process, &Process::started, &process,
                         [this, processPtr = &process, barrier = barrier->barrier()] {
            m_runParameters.setApplicationPid(processPtr->processId());
            m_runParameters.setApplicationMainThreadId(processPtr->applicationMainThreadId());
            barrier->advance();
        });

        EnginesDriver *driver = driverStorage.activeStorage();
        QObject::connect(driver, &EnginesDriver::interruptTerminalRequested,
                         &process, &Process::interrupt);
        QObject::connect(driver, &EnginesDriver::kickoffTerminalProcessRequested,
                         &process, &Process::kickoffProcess);
    };
    const auto onDone = [this](const Process &process, DoneWith result) {
        if (result != DoneWith::Success)
            q->reportFailure(process.errorString());
        else
            q->reportDone();
    };

    return Group {
        If (useTerminal) >> Then {
            ProcessTask(onSetup, onDone)
        } >> Else {
            Sync([barrier] { barrier->barrier()->advance(); })
        }
    };
}

ExecutableItem DebuggerRunToolPrivate::fixupParamsRecipe()
{
    return Sync([this] {
        TaskHub::clearTasks(Constants::TASK_CATEGORY_DEBUGGER_RUNTIME);

        if (q->runControl()->usesDebugChannel())
            m_runParameters.setRemoteChannel(q->runControl()->debugChannel().toString());

        if (q->runControl()->usesQmlChannel()) {
            m_runParameters.setQmlServer(q->runControl()->qmlChannel());
            if (m_runParameters.isAddQmlServerInferiorCmdArgIfNeeded()
                && m_runParameters.isQmlDebugging()
                && m_runParameters.isCppDebugging()) {

                const int qmlServerPort = m_runParameters.qmlServer().port();
                QTC_ASSERT(qmlServerPort > 0, q->reportFailure(); return false);
                const QString mode = QString("port:%1").arg(qmlServerPort);

                auto inferior = m_runParameters.inferior();
                CommandLine cmd{inferior.command.executable()};
                cmd.addArg(qmlDebugCommandLineArguments(QmlDebuggerServices, mode, true));
                cmd.addArgs(m_runParameters.inferior().command.arguments(), CommandLine::Raw);
                inferior.command = cmd;
                m_runParameters.setInferior(inferior);
            }
        }

        // User canceled input dialog asking for executable when working on library project.
        if (m_runParameters.startMode() == StartInternal
            && m_runParameters.inferior().command.isEmpty()
            && m_runParameters.interpreter().isEmpty()) {
            q->reportFailure(Tr::tr("No executable specified."));
            return false;
        }

        // QML and/or mixed are not prepared for it.
        // q->runControl()->setSupportsReRunning(!q->m_runParameters.isQmlDebugging);
        q->runControl()->setSupportsReRunning(false); // FIXME: Broken in general.

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

        if (Result<> res = m_runParameters.fixupParameters(q->runControl()); !res) {
            q->reportFailure(res.error());
            return false;
        }

        if (m_runParameters.cppEngineType() == CdbEngineType
            && Utils::is64BitWindowsBinary(m_runParameters.inferior().command.executable())
            && !Utils::is64BitWindowsBinary(m_runParameters.debugger().command.executable())) {
            q->reportFailure(Tr::tr(
                    "%1 is a 64 bit executable which can not be debugged by a 32 bit Debugger.\n"
                    "Please select a 64 bit Debugger in the kit settings for this kit.")
                    .arg(m_runParameters.inferior().command.executable().toUserOutput()));
            return false;
        }

        Utils::globalMacroExpander()->registerFileVariables(
            "DebuggedExecutable", Tr::tr("Debugged executable"),
            [this] { return m_runParameters.inferior().command.executable(); });

        q->runControl()->setDisplayName(m_runParameters.displayName());

        if (auto interpreterAspect = q->runControl()->aspectData<FilePathAspect>()) {
            if (auto mainScriptAspect = q->runControl()->aspectData<MainScriptAspect>()) {
                const FilePath mainScript = mainScriptAspect->filePath;
                const FilePath interpreter = interpreterAspect->filePath;
                if (!interpreter.isEmpty() && mainScript.endsWith(".py")) {
                    m_runParameters.setMainScript(mainScript);
                    m_runParameters.setInterpreter(interpreter);
                }
            }
        }

        return true;
    });
}

ExecutableItem DebuggerRunToolPrivate::debugServerRecipe(const SingleBarrier &barrier)
{
    const auto useDebugServer = [this] {
        return q->runControl()->usesDebugChannel() && !m_runParameters.skipDebugServer();
    };

    const auto onSetup = [this, barrier](Process &process) {
        process.setUtf8Codec();
        CommandLine commandLine = m_runParameters.inferior().command;
        CommandLine cmd;

        if (q->runControl()->usesQmlChannel() && !q->runControl()->usesDebugChannel()) {
            // FIXME: Case should not happen?
            cmd.setExecutable(commandLine.executable());
            cmd.addArg(qmlDebugTcpArguments(QmlDebuggerServices, q->runControl()->qmlChannel()));
            cmd.addArgs(commandLine.arguments(), CommandLine::Raw);
        } else {
            cmd.setExecutable(q->runControl()->device()->debugServerPath());

            if (cmd.isEmpty()) {
                if (q->runControl()->device()->osType() == Utils::OsTypeMac) {
                    const FilePath debugServerLocation = q->runControl()->device()->filePath(
                        "/Applications/Xcode.app/Contents/SharedFrameworks/LLDB.framework/"
                        "Resources/debugserver");

                    if (debugServerLocation.isExecutableFile()) {
                        cmd.setExecutable(debugServerLocation);
                    } else {
                        // TODO: In the future it is expected that the debugserver will be
                        // replaced by lldb-server. Remove the check for debug server at that point.
                        const FilePath lldbserver
                            = q->runControl()->device()->filePath("lldb-server").searchInPath();
                        if (lldbserver.isExecutableFile())
                            cmd.setExecutable(lldbserver);
                    }
                } else {
                    const FilePath gdbServerPath
                        = q->runControl()->device()->filePath("gdbserver").searchInPath();
                    FilePath lldbServerPath
                        = q->runControl()->device()->filePath("lldb-server").searchInPath();

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
            QTC_ASSERT(q->runControl()->usesDebugChannel(),
                       q->reportFailure({}); return SetupResult::StopWithError);
            if (cmd.executable().baseName().contains("lldb-server")) {
                cmd.addArg("platform");
                cmd.addArg("--listen");
                cmd.addArg(QString("*:%1").arg(q->runControl()->debugChannel().port()));
                cmd.addArg("--server");
            } else if (cmd.executable().baseName() == "debugserver") {
                const QString ipAndPort("`echo $SSH_CLIENT | cut -d ' ' -f 1`:%1");
                cmd.addArgs(ipAndPort.arg(q->runControl()->debugChannel().port()), CommandLine::Raw);

                if (m_runParameters.serverAttachPid().isValid())
                    cmd.addArgs({"--attach", QString::number(m_runParameters.serverAttachPid().pid())});
                else
                    cmd.addCommandLineAsArgs(q->runControl()->commandLine());
            } else {
                // Something resembling gdbserver
                if (m_runParameters.serverUseMulti())
                    cmd.addArg("--multi");
                if (m_runParameters.serverAttachPid().isValid())
                    cmd.addArg("--attach");

                const auto port = q->runControl()->debugChannel().port();
                cmd.addArg(QString(":%1").arg(port));

                if (q->runControl()->device()->extraData(ProjectExplorer::Constants::SSH_FORWARD_DEBUGSERVER_PORT).toBool()) {
                    QVariantHash extraData;
                    extraData[RemoteLinux::Constants::SshForwardPort] = port;
                    extraData[RemoteLinux::Constants::DisableSharing] = true;
                    process.setExtraData(extraData);
                }

                if (m_runParameters.serverAttachPid().isValid())
                    cmd.addArg(QString::number(m_runParameters.serverAttachPid().pid()));
            }
        }

        if (auto terminalAspect = q->runControl()->aspectData<TerminalAspect>()) {
            const bool useTerminal = terminalAspect->useTerminal;
            process.setTerminalMode(useTerminal ? TerminalMode::Run : TerminalMode::Off);
        }

        process.setCommand(cmd);
        process.setWorkingDirectory(m_runParameters.inferior().workingDirectory);

        QObject::connect(&process, &Process::readyReadStandardOutput, q, [this, process = &process] {
            const QString msg = process->readAllStandardOutput();
            q->runControl()->postMessage(msg, StdOutFormat, false);
        });

        QObject::connect(&process, &Process::readyReadStandardError, q, [this, process = &process] {
            const QString msg = process->readAllStandardError();
            q->runControl()->postMessage(msg, StdErrFormat, false);
        });

        QObject::connect(&process, &Process::started, q, [barrier = barrier->barrier()] {
            barrier->advance();
        });

        return SetupResult::Continue;
    };

    const auto onDone = [this](const Process &process, DoneWith result) {
        if (result != DoneWith::Success)
            q->reportFailure(process.errorString());
        else
            q->reportDone();
    };

    return Group {
        If (useDebugServer) >> Then {
            ProcessTask(onSetup, onDone)
        } >> Else {
            Sync([barrier] { barrier->barrier()->advance(); })
        }
    };
}

static ExecutableItem doneAwaiter(const Storage<EnginesDriver> &driverStorage)
{
    return BarrierTask([driverStorage](Barrier &barrier) {
        QObject::connect(driverStorage.activeStorage(), &EnginesDriver::done, &barrier, &Barrier::stopWithResult,
                         static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    });
}

ExecutableItem DebuggerRunToolPrivate::startEnginesRecipe(const Storage<EnginesDriver> &driverStorage)
{
    const auto setupEngines = [this, driverStorage] {
        EnginesDriver *driver = driverStorage.activeStorage();
        RunControl *rc = q->runControl();
        if (Result<> res = driver->setupEngines(rc, m_runParameters); !res) {
            q->reportFailure(res.error());
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
        rc->postMessage(Tr::tr("Debugging %1 ...").arg(m_runParameters.inferior().command.toUserOutput()),
                        NormalMessageFormat);
        const QString message = Tr::tr("Starting debugger \"%1\" for ABI \"%2\"...")
                                .arg(driver->debuggerName(), m_runParameters.toolChainAbi().toString());
        DebuggerMainWindow::showStatusMessage(message, 10000);
        driver->showMessage(driver->startParameters(), LogDebug);
        driver->showMessage(DebuggerSettings::dump(), LogDebug);

        driver->start();
        QObject::connect(driver, &EnginesDriver::started,
                         runStorage().activeStorage(), &RunInterface::started);
        return true;
    };

    return If (setupEngines) >> Then {
        doneAwaiter(driverStorage)
    };
}

ExecutableItem DebuggerRunToolPrivate::finishEnginesRecipe(const Storage<EnginesDriver> &driverStorage)
{
    const auto isRunning = [driverStorage] { return driverStorage->isRunning(); };

    return If (isRunning) >> Then {
        Sync([driverStorage] { driverStorage->stop(); }),
        doneAwaiter(driverStorage)
    };
}

static expected_str<QList<QPointer<Internal::DebuggerEngine>>> createEngines(
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

    QString warningMessage = Tr::tr("Some breakpoints cannot be handled by the debugger "
                                    "languages currently active, and will be ignored.<p>"
                                    "Affected are breakpoints %1").arg(unhandledIds.join(", "));
    if (hasQmlBreakpoints) {
        warningMessage += "<p>" + Tr::tr("QML debugging needs to be enabled both in the Build "
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
                const QString msg = engine->runParameters().exitCode() // Main engine.
                                        ? Tr::tr("Debugging of %1 has finished with exit code %2.")
                                              .arg(cmd)
                                              .arg(*engine->runParameters().exitCode())
                                        : Tr::tr("Debugging of %1 has finished.").arg(cmd);
                m_runControl->postMessage(msg, NormalMessageFormat);
                emit done(engine->runParameters().exitCode() ? DoneResult::Error : DoneResult::Success);
            }
        });
        connect(engine, &DebuggerEngine::postMessageRequested, m_runControl, &RunControl::postMessage);

        if (engine->isPrimaryEngine()) {
            connect(engine, &DebuggerEngine::attachToCoreRequested, this, [this](const QString &coreFile) {
                auto rc = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
                rc->copyDataFromRunControl(m_runControl);
                rc->resetDataForAttachToCore();
                auto name = QString(Tr::tr("%1 - Snapshot %2").arg(m_runControl->displayName()).arg(++m_snapshotCounter));
                auto debugger = new DebuggerRunTool(rc);
                DebuggerRunParameters &rp = debugger->runParameters();
                rp.setStartMode(AttachToCore);
                rp.setCloseMode(DetachAtClose);
                rp.setDisplayName(name);
                rp.setCoreFilePath(FilePath::fromString(coreFile));
                rp.setSnapshot(true);
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

void DebuggerRunTool::start()
{
    const Storage<EnginesDriver> driverStorage;
    const Storage<FilePath> tempCoreFileStorage;

    const auto onSetup = [this] {
        RunInterface *iface = runStorage().activeStorage();
        connect(this, &DebuggerRunTool::canceled, iface, &RunInterface::canceled);
        connect(iface, &RunInterface::started, this, &RunWorker::reportStarted);
        d->m_runParameters.setAttachPid(runControl()->attachPid());
    };

    const auto terminalKicker = [this, driverStorage](const SingleBarrier &barrier) {
        return d->terminalRecipe(barrier, driverStorage);
    };

    const auto debugServerKicker = [this](const SingleBarrier &barrier) {
        return d->debugServerRecipe(barrier);
    };

    const auto onDone = [this, tempCoreFileStorage] {
        if (tempCoreFileStorage->exists())
            tempCoreFileStorage->removeFile();
        if (d->m_runParameters.isSnapshot() && !d->m_runParameters.coreFile().isEmpty())
            d->m_runParameters.coreFile().removeFile();
    };

    const Group recipe {
        runStorage(),
        driverStorage,
        tempCoreFileStorage,
        continueOnError,
        onGroupSetup(onSetup),
        Group {
            d->coreFileRecipe(tempCoreFileStorage),
            When (terminalKicker) >> Do {
                d->fixupParamsRecipe(),
                When (debugServerKicker) >> Do {
                    d->startEnginesRecipe(driverStorage)
                }
            }
        }.withCancel(canceler()),
        d->finishEnginesRecipe(driverStorage),
        onGroupDone(onDone)
    };
    d->m_taskTreeRunner.start(recipe, {}, [this](DoneWith result) {
        if (result == DoneWith::Success)
            reportStopped();
        else
            reportFailure();
    });
}

void DebuggerRunTool::stop()
{
    if (!d->m_taskTreeRunner.isRunning())
        return;

    emit canceled();
}

void DebuggerRunTool::setupPortsGatherer()
{
    if (d->m_runParameters.isCppDebugging())
        runControl()->requestDebugChannel();

    if (d->m_runParameters.isQmlDebugging())
        runControl()->requestQmlChannel();
}

DebuggerRunParameters &DebuggerRunTool::runParameters()
{
    return d->m_runParameters;
}

DebuggerRunTool::DebuggerRunTool(RunControl *runControl)
    : RunWorker(runControl)
    , d(new DebuggerRunToolPrivate{this, DebuggerRunParameters::fromRunControl(runControl)})
{
    setId("DebuggerRunTool");
    runControl->setIcon(ProjectExplorer::Icons::DEBUG_START_SMALL_TOOLBAR);
    runControl->setPromptToStop([](bool *optionalPrompt) {
        return RunControl::showPromptToStopDialog(
            Tr::tr("Close Debugging Session"),
            Tr::tr("A debugging session is still in progress. "
                                "Terminating the session in the current"
                                " state can leave the target in an inconsistent state."
                                " Would you still like to terminate it?"),
                QString(), QString(), optionalPrompt);
    });
}

DebuggerRunTool::~DebuggerRunTool()
{
    delete d;
}

class DebuggerRunWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    DebuggerRunWorkerFactory()
    {
        setProduct<DebuggerRunTool>();
        setId(Constants::DEBUGGER_RUN_FACTORY);

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
