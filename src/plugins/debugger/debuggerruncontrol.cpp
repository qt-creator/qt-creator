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

class DebuggerRunToolPrivate
{
public:
    DebuggerRunTool *q = nullptr;

    GroupItem coreFileRecipe();

    int snapshotCounter = 0;
    int engineStartsNeeded = 0;
    int engineStopsNeeded = 0;
    QString runId;

    // Core unpacker
    FilePath m_tempCoreFilePath; // TODO: Enclose in the recipe storage when all tasks are there.

    // Terminal
    Process terminalProc;

    // DebugServer
    Process debuggerServerProc;

    Tasking::TaskTreeRunner m_taskTreeRunner;
};

} // namespace Internal

void DebuggerRunTool::start()
{
    const Group recipe {
        d->coreFileRecipe()
    };
    d->m_taskTreeRunner.start(recipe, {}, [this](DoneWith result) {
        if (result == DoneWith::Success)
            startTerminalIfNeededAndContinueStartup();
    });
}

GroupItem DebuggerRunToolPrivate::coreFileRecipe()
{
    const FilePath coreFile = q->m_runParameters.coreFile();
    if (!coreFile.endsWith(".gz") && !coreFile.endsWith(".lzo"))
        return nullItem;

    const Storage<QFile> storage; // tempCoreFile

    const auto onSetup = [this, storage, coreFile](Process &process) {
        {
            TemporaryFile tmp("tmpcore-XXXXXX");
            QTC_CHECK(tmp.open());
            m_tempCoreFilePath = FilePath::fromString(tmp.fileName());
        }
        QFile *tempCoreFile = storage.activeStorage();
        process.setWorkingDirectory(TemporaryDirectory::masterDirectoryFilePath());
        const QString msg = Tr::tr("Unpacking core file to %1");
        q->appendMessage(msg.arg(m_tempCoreFilePath.toUserOutput()), LogMessageFormat);

        if (coreFile.endsWith(".lzo")) {
            process.setCommand({"lzop", {"-o", m_tempCoreFilePath.path(), "-x", coreFile.path()}});
        } else { // ".gz"
            tempCoreFile->setFileName(m_tempCoreFilePath.path());
            QTC_CHECK(tempCoreFile->open(QFile::WriteOnly));
            QObject::connect(&process, &Process::readyReadStandardOutput, &process,
                             [tempCoreFile, processPtr = &process] {
                tempCoreFile->write(processPtr->readAllRawStandardOutput());
            });
            process.setCommand({"gzip", {"-c", "-d", coreFile.path()}});
        }
    };

    const auto onDone = [this, storage](DoneWith result) {
        if (result == DoneWith::Success)
            q->m_runParameters.setCoreFilePath(m_tempCoreFilePath);
        else
            q->reportFailure("Error unpacking " + q->m_runParameters.coreFile().toUserOutput());
        if (storage->isOpen())
            storage->close();
    };

    return Group {
        storage,
        ProcessTask(onSetup, onDone, CallDoneIf::Success)
    };
}

void DebuggerRunTool::startTerminalIfNeededAndContinueStartup()
{
    // CDB has a built-in console that might be preferred by some.
    const bool useCdbConsole = m_runParameters.cppEngineType() == CdbEngineType
            && (m_runParameters.startMode() == StartInternal
                || m_runParameters.startMode() == StartExternal)
            && settings().useCdbConsole();
    if (useCdbConsole)
        m_runParameters.setUseTerminal(false);

    if (!m_runParameters.useTerminal()) {
        continueAfterTerminalStart();
        return;
    }

    // Actually start the terminal.
    ProcessRunData stub = m_runParameters.inferior();

    if (m_runParameters.runAsRoot()) {
        d->terminalProc.setRunAsRoot(true);
        RunControl::provideAskPassEntry(stub.environment);
    }

    d->terminalProc.setTerminalMode(TerminalMode::Debug);
    d->terminalProc.setRunData(stub);

    connect(&d->terminalProc, &Process::started, this, [this] {
        m_runParameters.setApplicationPid(d->terminalProc.processId());
        m_runParameters.setApplicationMainThreadId(d->terminalProc.applicationMainThreadId());
        continueAfterTerminalStart();
    });

    connect(&d->terminalProc, &Process::done, this, [this] {
        if (d->terminalProc.error() != QProcess::UnknownError)
            reportFailure(d->terminalProc.errorString());
        if (d->terminalProc.error() != QProcess::FailedToStart)
            reportDone();
    });

    d->terminalProc.start();
}

void DebuggerRunTool::continueAfterTerminalStart()
{
    TaskHub::clearTasks(Constants::TASK_CATEGORY_DEBUGGER_RUNTIME);

    if (runControl()->usesDebugChannel())
        m_runParameters.setRemoteChannel(runControl()->debugChannel());

    if (runControl()->usesQmlChannel()) {
        m_runParameters.setQmlServer(runControl()->qmlChannel());
        if (m_runParameters.isAddQmlServerInferiorCmdArgIfNeeded()
                && m_runParameters.isQmlDebugging()
                && m_runParameters.isCppDebugging()) {

            const int qmlServerPort = m_runParameters.qmlServer().port();
            QTC_ASSERT(qmlServerPort > 0, reportFailure(); return);
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
        reportFailure(Tr::tr("No executable specified."));
        return;
    }

    // QML and/or mixed are not prepared for it.
//    setSupportsReRunning(!m_runParameters.isQmlDebugging);
    runControl()->setSupportsReRunning(false); // FIXME: Broken in general.

    // FIXME: Disabled due to Android. Make Android device report available ports instead.
//    int portsUsed = portsUsedByDebugger();
//    if (portsUsed > device()->freePorts().count()) {
//        reportFailure(Tr::tr("Cannot debug: Not enough free ports available."));
//        return;
//    }

    if (Result res = m_runParameters.fixupParameters(runControl()); !res) {
        reportFailure(res.error());
        return;
    }

    if (m_runParameters.cppEngineType() == CdbEngineType
        && Utils::is64BitWindowsBinary(m_runParameters.inferior().command.executable())
            && !Utils::is64BitWindowsBinary(m_runParameters.debugger().command.executable())) {
        reportFailure(
            Tr::tr(
                "%1 is a 64 bit executable which can not be debugged by a 32 bit Debugger.\n"
                "Please select a 64 bit Debugger in the kit settings for this kit.")
                .arg(m_runParameters.inferior().command.executable().toUserOutput()));
        return;
    }

    startDebugServerIfNeededAndContinueStartup();
}

void DebuggerRunTool::continueAfterDebugServerStart()
{
    Utils::globalMacroExpander()->registerFileVariables(
                "DebuggedExecutable", Tr::tr("Debugged executable"),
                [this] { return m_runParameters.inferior().command.executable(); }
    );

    runControl()->setDisplayName(m_runParameters.displayName());

    if (auto dapEngine = createDapEngine(runControl()->runMode()))
        m_engines << dapEngine;

    if (m_engines.isEmpty()) {
        if (m_runParameters.isCppDebugging()) {
            switch (m_runParameters.cppEngineType()) {
            case GdbEngineType:
                m_engines << createGdbEngine();
                break;
            case CdbEngineType:
                if (!HostOsInfo::isWindowsHost()) {
                    reportFailure(Tr::tr("Unsupported CDB host system."));
                    return;
                }
                m_engines << createCdbEngine();
                break;
            case LldbEngineType:
                m_engines << createLldbEngine();
                break;
            case GdbDapEngineType:
                m_engines << createDapEngine(ProjectExplorer::Constants::DAP_GDB_DEBUG_RUN_MODE);
                break;
            case LldbDapEngineType:
                m_engines << createDapEngine(ProjectExplorer::Constants::DAP_LLDB_DEBUG_RUN_MODE);
                break;
            case UvscEngineType:
                m_engines << createUvscEngine();
                break;
            default:
                if (!m_runParameters.isQmlDebugging()) {
                    reportFailure(noEngineMessage() + '\n' +
                        Tr::tr("Specify Debugger settings in Projects > Run."));
                    return;
                }
                // Can happen for pure Qml.
                break;
            }
        }

        if (m_runParameters.isPythonDebugging())
            m_engines << createPdbEngine();

        if (m_runParameters.isQmlDebugging())
            m_engines << createQmlEngine();
    }

    if (m_engines.isEmpty()) {
        QString msg = noEngineMessage();
        if (!DebuggerKitAspect::debugger(runControl()->kit()))
            msg += '\n' + noDebuggerInKitMessage();
        reportFailure(msg);
        return;
    }

    if (auto interpreterAspect = runControl()->aspectData<FilePathAspect>()) {
        if (auto mainScriptAspect = runControl()->aspectData<MainScriptAspect>()) {
            const FilePath mainScript = mainScriptAspect->filePath;
            const FilePath interpreter = interpreterAspect->filePath;
            if (!interpreter.isEmpty() && mainScript.endsWith(".py")) {
                m_runParameters.setMainScript(mainScript);
                m_runParameters.setInterpreter(interpreter);
            }
        }
    }

    bool first = true;
    for (auto engine : m_engines) {
        engine->setRunParameters(m_runParameters);
        engine->setRunId(d->runId);
        for (auto companion : m_engines) {
            if (companion != engine)
                engine->addCompanionEngine(companion);
        }
        engine->setRunTool(this);
        if (!first)
            engine->setSecondaryEngine();
        auto rc = runControl();
        connect(engine, &DebuggerEngine::requestRunControlFinish, rc, [rc] {
                rc->setAutoDeleteOnStop(true);
                rc->initiateStop();
            }, Qt::QueuedConnection);
        connect(engine, &DebuggerEngine::requestRunControlStop, rc, &RunControl::initiateStop);

        connect(engine, &DebuggerEngine::engineStarted, this, [this, engine] {
            // Correct:
            // if (--d->engineStartsNeeded == 0) {
            //     EngineManager::activateDebugMode();
            //     reportStarted();
            // }

            // Feels better, as the QML Engine might attach late or not at all.
            if (engine == m_engines.first()) {
                EngineManager::activateDebugMode();
                reportStarted();
            }
        });

        connect(engine, &DebuggerEngine::engineFinished, this, [this, engine] {
            engine->prepareForRestart();
            if (--d->engineStopsNeeded == 0) {
                const QString cmd = m_runParameters.inferior().command.toUserOutput();
                const QString msg = engine->runParameters().exitCode() // Main engine.
                                        ? Tr::tr("Debugging of %1 has finished with exit code %2.")
                                              .arg(cmd)
                                              .arg(*engine->runParameters().exitCode())
                                        : Tr::tr("Debugging of %1 has finished.").arg(cmd);
                appendMessage(msg, NormalMessageFormat);
                reportStopped();
            }
        });
        connect(engine, &DebuggerEngine::appendMessageRequested,
                this, &DebuggerRunTool::appendMessage);
        ++d->engineStartsNeeded;
        ++d->engineStopsNeeded;

        if (first) {
            connect(engine, &DebuggerEngine::attachToCoreRequested, this, [this](const QString &coreFile) {
                auto rc = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
                rc->copyDataFromRunControl(runControl());
                rc->resetDataForAttachToCore();
                auto name = QString(Tr::tr("%1 - Snapshot %2").arg(runControl()->displayName()).arg(++d->snapshotCounter));
                auto debugger = new DebuggerRunTool(rc);
                DebuggerRunParameters &rp = debugger->runParameters();
                rp.setStartMode(AttachToCore);
                rp.setCloseMode(DetachAtClose);
                rp.setDisplayName(name);
                rp.setCoreFilePath(FilePath::fromString(coreFile));
                rp.setSnapshot(true);
                rc->start();
            });

            first = false;
        }
    }

    if (m_runParameters.startMode() != AttachToCore) {
        QStringList unhandledIds;
        bool hasQmlBreakpoints = false;
        for (const GlobalBreakpoint &gbp : BreakpointManager::globalBreakpoints()) {
            if (gbp->isEnabled()) {
                const BreakpointParameters &bp = gbp->requestedParameters();
                hasQmlBreakpoints = hasQmlBreakpoints || bp.isQmlFileAndLineBreakpoint();
                auto engineAcceptsBp = [bp](const DebuggerEngine *engine) {
                    return engine->acceptsBreakpoint(bp);
                };
                if (!Utils::anyOf(m_engines, engineAcceptsBp))
                    unhandledIds.append(gbp->displayName());
            }
        }
        if (!unhandledIds.isEmpty()) {
            QString warningMessage = Tr::tr("Some breakpoints cannot be handled by the debugger "
                                            "languages currently active, and will be ignored.<p>"
                                            "Affected are breakpoints %1")
                                         .arg(unhandledIds.join(", "));

            if (hasQmlBreakpoints) {
                warningMessage += "<p>"
                                  + Tr::tr("QML debugging needs to be enabled both in the Build "
                                           "and the Run settings.");
            }

            showMessage(warningMessage, LogWarning);

            if (settings().showUnsupportedBreakpointWarning()) {
                bool doNotAskAgain = false;
                CheckableDecider decider(&doNotAskAgain);
                CheckableMessageBox::information(
                    Tr::tr("Debugger"),
                    warningMessage,
                    decider,
                    QMessageBox::Ok);
                if (doNotAskAgain) {
                    settings().showUnsupportedBreakpointWarning.setValue(false);
                    settings().showUnsupportedBreakpointWarning.writeSettings();
                }
            }
        }
    }

    appendMessage(Tr::tr("Debugging %1 ...").arg(m_runParameters.inferior().command.toUserOutput()),
                  NormalMessageFormat);
    const QString debuggerName = Utils::transform<QStringList>(m_engines, &DebuggerEngine::objectName).join(" ");

    const QString message = Tr::tr("Starting debugger \"%1\" for ABI \"%2\"...")
            .arg(debuggerName).arg(m_runParameters.toolChainAbi().toString());
    DebuggerMainWindow::showStatusMessage(message, 10000);

    showMessage(m_engines.first()->formatStartParameters(), LogDebug);
    showMessage(DebuggerSettings::dump(), LogDebug);

    Utils::reverseForeach(m_engines, [](DebuggerEngine *engine) { engine->start(); });
}

void DebuggerRunTool::kickoffTerminalProcess()
{
    d->terminalProc.kickoffProcess();
}

void DebuggerRunTool::interruptTerminal()
{
    d->terminalProc.interrupt();
}

void DebuggerRunTool::stop()
{
    QTC_ASSERT(!m_engines.isEmpty(), reportStopped(); return);
    Utils::reverseForeach(m_engines, [](DebuggerEngine *engine) { engine->quitDebugger(); });
}

void DebuggerRunTool::setupPortsGatherer()
{
    if (m_runParameters.isCppDebugging())
        runControl()->requestDebugChannel();

    if (m_runParameters.isQmlDebugging())
        runControl()->requestQmlChannel();
}

DebuggerRunTool::DebuggerRunTool(RunControl *runControl)
    : RunWorker(runControl)
    , d(new DebuggerRunToolPrivate)
    , m_runParameters(DebuggerRunParameters::fromRunControl(runControl))
{
    d->q = this;

    setId("DebuggerRunTool");

    static int toolRunCount = 0;

    // Reset once all are gone.
    if (EngineManager::engines().isEmpty())
        toolRunCount = 0;

    d->debuggerServerProc.setUtf8Codec();

    d->runId = QString::number(++toolRunCount);

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
    if (d->m_tempCoreFilePath.exists())
        d->m_tempCoreFilePath.removeFile();

    if (m_runParameters.isSnapshot() && !m_runParameters.coreFile().isEmpty())
        m_runParameters.coreFile().removeFile();

    qDeleteAll(m_engines);
    m_engines.clear();

    delete d;
}

void DebuggerRunTool::showMessage(const QString &msg, int channel, int timeout)
{
    if (channel == ConsoleOutput)
        debuggerConsole()->printItem(ConsoleItem::DefaultType, msg);

    QTC_ASSERT(!m_engines.isEmpty(), qDebug() << msg; return);

    for (auto engine : m_engines)
        engine->showMessage(msg, channel, timeout);
    switch (channel) {
    case AppOutput:
        appendMessage(msg, StdOutFormat);
        break;
    case AppError:
        appendMessage(msg, StdErrFormat);
        break;
    case AppStuff:
        appendMessage(msg, DebugFormat);
        break;
    default:
        break;
    }
}

void DebuggerRunTool::startDebugServerIfNeededAndContinueStartup()
{
    if (!runControl()->usesDebugChannel() || m_runParameters.skipDebugServer()) {
        continueAfterDebugServerStart();
        return;
    }

    // FIXME: Indentation intentionally wrong to keep diff in gerrit small. Will fix later.

        CommandLine commandLine = m_runParameters.inferior().command;
        CommandLine cmd;

        if (runControl()->usesQmlChannel() && !runControl()->usesDebugChannel()) {
            // FIXME: Case should not happen?
            cmd.setExecutable(commandLine.executable());
            cmd.addArg(qmlDebugTcpArguments(QmlDebuggerServices, runControl()->qmlChannel()));
            cmd.addArgs(commandLine.arguments(), CommandLine::Raw);
        } else {
            cmd.setExecutable(runControl()->device()->debugServerPath());

            if (cmd.isEmpty()) {
                if (runControl()->device()->osType() == Utils::OsTypeMac) {
                    const FilePath debugServerLocation = runControl()->device()->filePath(
                        "/Applications/Xcode.app/Contents/SharedFrameworks/LLDB.framework/"
                        "Resources/debugserver");

                    if (debugServerLocation.isExecutableFile()) {
                        cmd.setExecutable(debugServerLocation);
                    } else {
                        // TODO: In the future it is expected that the debugserver will be
                        // replaced by lldb-server. Remove the check for debug server at that point.
                        const FilePath lldbserver
                                = runControl()->device()->filePath("lldb-server").searchInPath();
                        if (lldbserver.isExecutableFile())
                            cmd.setExecutable(lldbserver);
                    }
                } else {
                    const FilePath gdbServerPath
                            = runControl()->device()->filePath("gdbserver").searchInPath();
                    FilePath lldbServerPath
                            = runControl()->device()->filePath("lldb-server").searchInPath();

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
            QTC_ASSERT(runControl()->usesDebugChannel(), reportFailure({}));
            if (cmd.executable().baseName().contains("lldb-server")) {
                cmd.addArg("platform");
                cmd.addArg("--listen");
                cmd.addArg(QString("*:%1").arg(runControl()->debugChannel().port()));
                cmd.addArg("--server");
            } else if (cmd.executable().baseName() == "debugserver") {
                const QString ipAndPort("`echo $SSH_CLIENT | cut -d ' ' -f 1`:%1");
                cmd.addArgs(ipAndPort.arg(runControl()->debugChannel().port()), CommandLine::Raw);

                if (m_runParameters.serverAttachPid().isValid())
                    cmd.addArgs({"--attach", QString::number(m_runParameters.serverAttachPid().pid())});
                else
                    cmd.addCommandLineAsArgs(runControl()->commandLine());
            } else {
                // Something resembling gdbserver
                if (m_runParameters.serverUseMulti())
                    cmd.addArg("--multi");
                if (m_runParameters.serverAttachPid().isValid())
                    cmd.addArg("--attach");

                const auto port = runControl()->debugChannel().port();
                cmd.addArg(QString(":%1").arg(port));

                if (runControl()->device()->extraData(ProjectExplorer::Constants::SSH_FORWARD_DEBUGSERVER_PORT).toBool()) {
                    QVariantHash extraData;
                    extraData[RemoteLinux::Constants::SshForwardPort] = port;
                    extraData[RemoteLinux::Constants::DisableSharing] = true;
                    d->debuggerServerProc.setExtraData(extraData);
                }

                if (m_runParameters.serverAttachPid().isValid())
                    cmd.addArg(QString::number(m_runParameters.serverAttachPid().pid()));
            }
        }

    if (auto terminalAspect = runControl()->aspectData<TerminalAspect>()) {
        const bool useTerminal = terminalAspect->useTerminal;
        d->debuggerServerProc.setTerminalMode(useTerminal ? TerminalMode::Run : TerminalMode::Off);
    }

    d->debuggerServerProc.setCommand(cmd);
    d->debuggerServerProc.setWorkingDirectory(m_runParameters.inferior().workingDirectory);

    connect(&d->debuggerServerProc, &Process::readyReadStandardOutput,
                this, [this] {
        const QString msg = d->debuggerServerProc.readAllStandardOutput();
        runControl()->postMessage(msg, StdOutFormat, false);
    });

    connect(&d->debuggerServerProc, &Process::readyReadStandardError,
                this, [this] {
        const QString msg = d->debuggerServerProc.readAllStandardError();
        runControl()->postMessage(msg, StdErrFormat, false);
    });

    connect(&d->debuggerServerProc, &Process::started, this, [this] {
        continueAfterDebugServerStart();
    });

    connect(&d->debuggerServerProc, &Process::done, this, [this] {
        if (d->terminalProc.error() != QProcess::UnknownError)
            reportFailure(d->terminalProc.errorString());
        if (d->terminalProc.error() != QProcess::FailedToStart && m_runParameters.serverEssential())
            reportDone();
    });

    d->debuggerServerProc.start();
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
