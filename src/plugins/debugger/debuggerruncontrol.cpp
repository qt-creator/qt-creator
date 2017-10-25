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

#include "debuggerruncontrol.h"
#include "terminal.h"

#include "analyzer/analyzermanager.h"
#include "console/console.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerkitinformation.h"
#include "debuggerplugin.h"
#include "debuggerrunconfigurationaspect.h"
#include "breakhandler.h"
#include "shared/peutils.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/deviceprocessesdialog.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <projectexplorer/environmentaspect.h> // For the environment
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/checkablemessagebox.h>
#include <utils/fileutils.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>
#include <utils/temporaryfile.h>
#include <utils/url.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/messagebox.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

#include <qtsupport/qtkitinformation.h>

#include <ssh/sshconnection.h>

#include <QTcpServer>

using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace Utils;

enum { debug = 0 };

namespace Debugger {
namespace Internal {

DebuggerEngine *createCdbEngine();
DebuggerEngine *createGdbEngine();
DebuggerEngine *createPdbEngine();
DebuggerEngine *createQmlEngine();
DebuggerEngine *createQmlCppEngine(DebuggerEngine *cppEngine);
DebuggerEngine *createLldbEngine();

class LocalProcessRunner : public RunWorker
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::LocalProcessRunner)

public:
    LocalProcessRunner(RunControl *runControl, const StandardRunnable &runnable)
        : RunWorker(runControl), m_runnable(runnable)
    {
        connect(&m_proc, &QProcess::errorOccurred,
                this, &LocalProcessRunner::handleError);
        connect(&m_proc, &QProcess::readyReadStandardOutput,
                this, &LocalProcessRunner::handleStandardOutput);
        connect(&m_proc, &QProcess::readyReadStandardError,
                this, &LocalProcessRunner::handleStandardError);
        connect(&m_proc, static_cast<void (QProcess::*)(int)>(&QProcess::finished),
                this, &LocalProcessRunner::handleFinished);
    }

    void start() override
    {
        m_proc.setCommand(m_runnable.executable, m_runnable.commandLineArguments);
        m_proc.start();
    }

    void stop() override
    {
        m_proc.terminate();
    }

    void handleStandardOutput()
    {
        const QByteArray ba = m_proc.readAllStandardOutput();
        const QString msg = QString::fromLocal8Bit(ba, ba.length());
        showMessage(msg, LogOutput);
        showMessage(msg, AppOutput);
    }

    void handleStandardError()
    {
        const QByteArray ba = m_proc.readAllStandardError();
        const QString msg = QString::fromLocal8Bit(ba, ba.length());
        showMessage(msg, LogOutput);
        showMessage(msg, AppError);
    }

    void handleFinished()
    {
        if (m_proc.exitStatus() == QProcess::NormalExit && m_proc.exitCode() == 0) {
            // all good.
            reportDone();
        } else {
            reportFailure(tr("Upload failed: %1").arg(m_proc.errorString()));
        }
    }

    void handleError(QProcess::ProcessError error)
    {
        QString msg;
        switch (error) {
        case QProcess::FailedToStart:
            msg = tr("The upload process failed to start. Shell missing?");
            break;
        case QProcess::Crashed:
            msg = tr("The upload process crashed some time after starting "
                     "successfully.");
            break;
        case QProcess::Timedout:
            msg = tr("The last waitFor...() function timed out. "
                     "The state of QProcess is unchanged, and you can try calling "
                     "waitFor...() again.");
            break;
        case QProcess::WriteError:
            msg = tr("An error occurred when attempting to write "
                     "to the upload process. For example, the process may not be running, "
                     "or it may have closed its input channel.");
            break;
        case QProcess::ReadError:
            msg = tr("An error occurred when attempting to read from "
                     "the upload process. For example, the process may not be running.");
            break;
        default:
            msg = tr("An unknown error in the upload process occurred. "
                     "This is the default return value of error().");
        }

        showMessage(msg, StatusBar);
        Core::AsynchronousMessageBox::critical(tr("Error"), msg);
    }

    StandardRunnable m_runnable;
    Utils::QtcProcess m_proc;
};

class CoreUnpacker : public RunWorker
{
public:
    CoreUnpacker(RunControl *runControl, const QString &coreFileName)
        : RunWorker(runControl), m_coreFileName(coreFileName)
    {}

    QString coreFileName() const { return m_tempCoreFileName; }

private:
    ~CoreUnpacker() final
    {
        m_coreUnpackProcess.blockSignals(true);
        m_coreUnpackProcess.terminate();
        m_coreUnpackProcess.deleteLater();
        if (m_tempCoreFile.isOpen())
            m_tempCoreFile.close();

        QFile::remove(m_tempCoreFileName);
    }

    void start() final
    {
        {
            Utils::TemporaryFile tmp("tmpcore-XXXXXX");
            tmp.open();
            m_tempCoreFileName = tmp.fileName();
        }

        m_coreUnpackProcess.setWorkingDirectory(TemporaryDirectory::masterDirectoryPath());
        connect(&m_coreUnpackProcess, static_cast<void (QProcess::*)(int)>(&QProcess::finished),
                this, &CoreUnpacker::reportStarted);

        const QString msg = DebuggerRunTool::tr("Unpacking core file to %1");
        appendMessage(msg.arg(m_tempCoreFileName), LogMessageFormat);

        if (m_coreFileName.endsWith(".lzo")) {
            m_coreUnpackProcess.start("lzop", {"-o", m_tempCoreFileName, "-x", m_coreFileName});
            return;
        }

        if (m_coreFileName.endsWith(".gz")) {
            appendMessage(msg.arg(m_tempCoreFileName), LogMessageFormat);
            m_tempCoreFile.setFileName(m_tempCoreFileName);
            m_tempCoreFile.open(QFile::WriteOnly);
            connect(&m_coreUnpackProcess, &QProcess::readyRead, this, [this] {
                m_tempCoreFile.write(m_coreUnpackProcess.readAll());
            });
            m_coreUnpackProcess.start("gzip", {"-c", "-d", m_coreFileName});
            return;
        }

        QTC_CHECK(false);
        reportFailure("Unknown file extension in " + m_coreFileName);
    }

    QFile m_tempCoreFile;
    QString m_coreFileName;
    QString m_tempCoreFileName;
    QProcess m_coreUnpackProcess;
};

class DebuggerRunToolPrivate
{
public:
    QPointer<TerminalRunner> terminalRunner;
    QPointer<CoreUnpacker> coreUnpacker;
    QPointer<GdbServerPortsGatherer> portsGatherer;
    bool addQmlServerInferiorCommandLineArgumentIfNeeded = false;
};

} // namespace Internal

static bool breakOnMainNextTime = false;

void DebuggerRunTool::setBreakOnMainNextTime()
{
    breakOnMainNextTime = true;
}

void DebuggerRunTool::setStartMode(DebuggerStartMode startMode)
{
    if (startMode == AttachToQmlServer) {
        m_runParameters.startMode = AttachToRemoteProcess;
        m_runParameters.isCppDebugging = false;
        m_runParameters.isQmlDebugging = true;
        m_runParameters.closeMode = KillAtClose;

        // FIXME: This is horribly wrong.
        // get files from all the projects in the session
        QList<Project *> projects = SessionManager::projects();
        if (Project *startupProject = SessionManager::startupProject()) {
            // startup project first
            projects.removeOne(startupProject);
            projects.insert(0, startupProject);
        }
        foreach (Project *project, projects)
            m_runParameters.projectSourceFiles.append(project->files(Project::SourceFiles));
        if (!projects.isEmpty())
            m_runParameters.projectSourceDirectory = projects.first()->projectDirectory().toString();

    } else {
        m_runParameters.startMode = startMode;
    }
}

void DebuggerRunTool::setCloseMode(DebuggerCloseMode closeMode)
{
    m_runParameters.closeMode = closeMode;
}

void DebuggerRunTool::setAttachPid(ProcessHandle pid)
{
    m_runParameters.attachPID = pid;
}

void DebuggerRunTool::setAttachPid(qint64 pid)
{
    m_runParameters.attachPID = ProcessHandle(pid);
}

void DebuggerRunTool::setSysRoot(const QString &sysRoot)
{
    m_runParameters.sysRoot = sysRoot;
}

void DebuggerRunTool::setSymbolFile(const QString &symbolFile)
{
    if (symbolFile.isEmpty())
        reportFailure(tr("Cannot debug: Local executable is not set."));
    m_runParameters.symbolFile = symbolFile;
}

void DebuggerRunTool::setRemoteChannel(const QString &channel)
{
    m_runParameters.remoteChannel = channel;
}

void DebuggerRunTool::setRemoteChannel(const QString &host, int port)
{
    m_runParameters.remoteChannel = QString("%1:%2").arg(host).arg(port);
}

void DebuggerRunTool::setUseExtendedRemote(bool on)
{
    m_runParameters.useExtendedRemote = on;
}

void DebuggerRunTool::setUseContinueInsteadOfRun(bool on)
{
    m_runParameters.useContinueInsteadOfRun = on;
}

void DebuggerRunTool::setUseTargetAsync(bool on)
{
    m_runParameters.useTargetAsync = on;
}

void DebuggerRunTool::setContinueAfterAttach(bool on)
{
    m_runParameters.continueAfterAttach = on;
}

void DebuggerRunTool::setSkipExecutableValidation(bool on)
{
    m_runParameters.skipExecutableValidation = on;
}

void DebuggerRunTool::setUseCtrlCStub(bool on)
{
    m_runParameters.useCtrlCStub = on;
}

void DebuggerRunTool::setBreakOnMain(bool on)
{
    m_runParameters.breakOnMain = on;
}

void DebuggerRunTool::setUseTerminal(bool on)
{
    if (on && !d->terminalRunner && m_runParameters.cppEngineType == GdbEngineType) {
        d->terminalRunner = new TerminalRunner(this);
        addStartDependency(d->terminalRunner);
    }
    if (!on && d->terminalRunner) {
        QTC_CHECK(false); // User code can only switch from no terminal to one terminal.
    }
}

void DebuggerRunTool::setCommandsAfterConnect(const QString &commands)
{
    m_runParameters.commandsAfterConnect = commands;
}

void DebuggerRunTool::setCommandsForReset(const QString &commands)
{
    m_runParameters.commandsForReset = commands;
}

void DebuggerRunTool::setServerStartScript(const QString &serverStartScript)
{
    if (!serverStartScript.isEmpty()) {
        // Provide script information about the environment
        StandardRunnable serverStarter;
        serverStarter.executable = serverStartScript;
        QtcProcess::addArg(&serverStarter.commandLineArguments, m_runParameters.inferior.executable);
        QtcProcess::addArg(&serverStarter.commandLineArguments, m_runParameters.remoteChannel);
        addStartDependency(new LocalProcessRunner(runControl(), serverStarter));
    }
}

void DebuggerRunTool::setDebugInfoLocation(const QString &debugInfoLocation)
{
    m_runParameters.debugInfoLocation = debugInfoLocation;
}

void DebuggerRunTool::setQmlServer(const QUrl &qmlServer)
{
    m_runParameters.qmlServer = qmlServer;
}

void DebuggerRunTool::setIosPlatform(const QString &platform)
{
    m_runParameters.platform = platform;
}

void DebuggerRunTool::setDeviceSymbolsRoot(const QString &deviceSymbolsRoot)
{
    m_runParameters.deviceSymbolsRoot = deviceSymbolsRoot;
}

void DebuggerRunTool::setTestCase(int testCase)
{
    m_runParameters.testCase = testCase;
}

void DebuggerRunTool::setOverrideStartScript(const QString &script)
{
    m_runParameters.overrideStartScript = script;
}

void DebuggerRunTool::setInferior(const Runnable &runnable)
{
    QTC_ASSERT(runnable.is<StandardRunnable>(), reportFailure(); return);
    m_runParameters.inferior = runnable.as<StandardRunnable>();
    setUseTerminal(m_runParameters.inferior.runMode == ApplicationLauncher::Console);
}

void DebuggerRunTool::setInferiorExecutable(const QString &executable)
{
    m_runParameters.inferior.executable = executable;
}

void DebuggerRunTool::setInferiorEnvironment(const Utils::Environment &env)
{
    m_runParameters.inferior.environment = env;
}

void DebuggerRunTool::setRunControlName(const QString &name)
{
    m_runParameters.displayName = name;
}

void DebuggerRunTool::setStartMessage(const QString &msg)
{
    m_runParameters.startMessage = msg;
}

void DebuggerRunTool::setCoreFileName(const QString &coreFile, bool isSnapshot)
{
    if (coreFile.endsWith(".gz") || coreFile.endsWith(".lzo")) {
        d->coreUnpacker = new CoreUnpacker(runControl(), coreFile);
        addStartDependency(d->coreUnpacker);
    }

    m_runParameters.coreFile = coreFile;
    m_runParameters.isSnapshot = isSnapshot;
}

void DebuggerRunTool::appendInferiorCommandLineArgument(const QString &arg)
{
    if (!m_runParameters.inferior.commandLineArguments.isEmpty())
        m_runParameters.inferior.commandLineArguments.append(' ');
    m_runParameters.inferior.commandLineArguments.append(arg);
}

void DebuggerRunTool::prependInferiorCommandLineArgument(const QString &arg)
{
    if (!m_runParameters.inferior.commandLineArguments.isEmpty())
        m_runParameters.inferior.commandLineArguments.prepend(' ');
    m_runParameters.inferior.commandLineArguments.prepend(arg);
}

void DebuggerRunTool::addQmlServerInferiorCommandLineArgumentIfNeeded()
{
    d->addQmlServerInferiorCommandLineArgumentIfNeeded = true;
}

void DebuggerRunTool::setCrashParameter(const QString &event)
{
    m_runParameters.crashParameter = event;
}

void DebuggerRunTool::addExpectedSignal(const QString &signal)
{
    m_runParameters.expectedSignals.append(signal);
}

void DebuggerRunTool::addSearchDirectory(const QString &dir)
{
    m_runParameters.additionalSearchDirectories.append(dir);
}

void DebuggerRunTool::start()
{
    Debugger::Internal::saveModeToRestore();
    Debugger::selectPerspective(Debugger::Constants::CppPerspectiveId);
    TaskHub::clearTasks(Debugger::Constants::TASK_CATEGORY_DEBUGGER_DEBUGINFO);
    TaskHub::clearTasks(Debugger::Constants::TASK_CATEGORY_DEBUGGER_RUNTIME);

    if (d->portsGatherer) {
        setRemoteChannel(d->portsGatherer->gdbServerChannel());
        setQmlServer(d->portsGatherer->qmlServer());
        if (d->addQmlServerInferiorCommandLineArgumentIfNeeded
                && m_runParameters.isQmlDebugging
                && m_runParameters.isCppDebugging) {
            using namespace QmlDebug;
            int qmlServerPort = m_runParameters.qmlServer.port();
            QTC_ASSERT(qmlServerPort > 0, reportFailure(); return);
            QString mode = QString("port:%1").arg(qmlServerPort);
            QString qmlServerArg = qmlDebugCommandLineArguments(QmlDebuggerServices, mode, true);
            prependInferiorCommandLineArgument(qmlServerArg);
        }
    }

    // User canceled input dialog asking for executable when working on library project.
    if (m_runParameters.startMode == StartInternal
            && m_runParameters.inferior.executable.isEmpty()
            && m_runParameters.interpreter.isEmpty()) {
        reportFailure(tr("No executable specified."));
        return;
    }

    // QML and/or mixed are not prepared for it.
    setSupportsReRunning(!m_runParameters.isQmlDebugging);

    // FIXME: Disabled due to Android. Make Android device report available ports instead.
//    int portsUsed = portsUsedByDebugger();
//    if (portsUsed > device()->freePorts().count()) {
//        reportFailure(tr("Cannot debug: Not enough free ports available."));
//        return;
//    }

    if (d->coreUnpacker)
        m_runParameters.coreFile = d->coreUnpacker->coreFileName();

    if (!fixupParameters())
        return;

    Utils::globalMacroExpander()->registerFileVariables(
                "DebuggedExecutable", tr("Debugged executable"),
                [this] { return m_runParameters.inferior.executable; }
    );

    runControl()->setDisplayName(m_runParameters.displayName);

    DebuggerEngine *cppEngine = nullptr;
    if (!m_engine) {
        switch (m_runParameters.cppEngineType) {
            case GdbEngineType:
                cppEngine = createGdbEngine();
                break;
            case CdbEngineType:
                if (!HostOsInfo::isWindowsHost()) {
                    reportFailure(tr("Unsupported CDB host system."));
                    return;
                }
                cppEngine = createCdbEngine();
                break;
            case LldbEngineType:
                cppEngine = createLldbEngine();
                break;
            case PdbEngineType: // FIXME: Yes, Python counts as C++...
                cppEngine = createPdbEngine();
                break;
            default:
                // Can happen for pure Qml.
                break;
        }

        if (m_runParameters.isQmlDebugging) {
            if (cppEngine)
                m_engine = createQmlCppEngine(cppEngine);
            else
                m_engine = createQmlEngine();
        } else {
            m_engine = cppEngine;
        }
    }

    if (!m_engine) {
        reportFailure(DebuggerPlugin::tr("Unable to create a debugging engine."));
        return;
    }

    m_engine->setRunTool(this);

    if (m_runParameters.startMode == StartInternal) {
        QStringList unhandledIds;
        foreach (Breakpoint bp, breakHandler()->allBreakpoints()) {
            if (bp.isEnabled() && !m_engine->acceptsBreakpoint(bp))
                unhandledIds.append(bp.id().toString());
        }
        if (!unhandledIds.isEmpty()) {
            QString warningMessage =
                    DebuggerPlugin::tr("Some breakpoints cannot be handled by the debugger "
                                       "languages currently active, and will be ignored.\n"
                                       "Affected are breakpoints %1")
                    .arg(unhandledIds.join(QLatin1String(", ")));

            Internal::showMessage(warningMessage, LogWarning);

            static bool checked = true;
            if (checked)
                CheckableMessageBox::information(Core::ICore::mainWindow(),
                                                 tr("Debugger"),
                                                 warningMessage,
                                                 tr("&Show this message again."),
                                                 &checked, QDialogButtonBox::Ok);
        }
    }

    appendMessage(tr("Debugging starts"), NormalMessageFormat);
    Internal::runControlStarted(this);
    m_engine->start();
}

void DebuggerRunTool::stop()
{
    m_isDying = true;
    QTC_ASSERT(m_engine, reportStopped(); return);
    m_engine->quitDebugger();
}

const DebuggerRunParameters &DebuggerRunTool::runParameters() const
{
    return m_runParameters;
}

bool DebuggerRunTool::isCppDebugging() const
{
    return m_runParameters.isCppDebugging;
}

bool DebuggerRunTool::isQmlDebugging() const
{
    return m_runParameters.isQmlDebugging;
}

int DebuggerRunTool::portsUsedByDebugger() const
{
    return isCppDebugging() + isQmlDebugging();
}

void DebuggerRunTool::setUsePortsGatherer(bool useCpp, bool useQml)
{
    QTC_ASSERT(!d->portsGatherer, reportFailure(); return);
    d->portsGatherer = new GdbServerPortsGatherer(runControl());
    d->portsGatherer->setUseGdbServer(useCpp);
    d->portsGatherer->setUseQmlServer(useQml);
    addStartDependency(d->portsGatherer);
}

GdbServerPortsGatherer *DebuggerRunTool::portsGatherer() const
{
    return d->portsGatherer;
}

void DebuggerRunTool::setSolibSearchPath(const QStringList &list)
{
    m_runParameters.solibSearchPath = list;
}

void DebuggerRunTool::notifyInferiorIll()
{
    m_engine->notifyInferiorIll();
}

void DebuggerRunTool::notifyInferiorExited()
{
    m_engine->notifyInferiorExited();
}

void DebuggerRunTool::quitDebugger()
{
    m_isDying = true;
    m_engine->quitDebugger();
}

void DebuggerRunTool::abortDebugger()
{
    m_engine->abortDebugger();
}

bool DebuggerRunTool::fixupParameters()
{
    DebuggerRunParameters &rp = m_runParameters;
    if (rp.symbolFile.isEmpty())
        rp.symbolFile = rp.inferior.executable;

    // Copy over DYLD_IMAGE_SUFFIX etc
    for (auto var : QStringList({"DYLD_IMAGE_SUFFIX", "DYLD_LIBRARY_PATH", "DYLD_FRAMEWORK_PATH"}))
        if (rp.inferior.environment.hasKey(var))
            rp.debugger.environment.set(var, rp.inferior.environment.value(var));

    // validate debugger if C++ debugging is enabled
    if (rp.isCppDebugging && !rp.validationErrors.isEmpty()) {
        reportFailure(rp.validationErrors.join('\n'));
        return false;
    }

    if (rp.isQmlDebugging) {
        if (device() && device()->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
            if (rp.qmlServer.port() <= 0) {
                rp.qmlServer = Utils::urlFromLocalHostAndFreePort();
                if (rp.qmlServer.port() <= 0) {
                    reportFailure(DebuggerPlugin::tr("Not enough free ports for QML debugging."));
                    return false;
                }
            }
            // Makes sure that all bindings go through the JavaScript engine, so that
            // breakpoints are actually hit!
            const QString optimizerKey = "QML_DISABLE_OPTIMIZER";
            if (!rp.inferior.environment.hasKey(optimizerKey))
                rp.inferior.environment.set(optimizerKey, "1");
        }
    }

    if (!boolSetting(AutoEnrichParameters)) {
        const QString sysroot = rp.sysRoot;
        if (rp.debugInfoLocation.isEmpty())
            rp.debugInfoLocation = sysroot + "/usr/lib/debug";
        if (rp.debugSourceLocation.isEmpty()) {
            QString base = sysroot + "/usr/src/debug/";
            rp.debugSourceLocation.append(base + "qt5base/src/corelib");
            rp.debugSourceLocation.append(base + "qt5base/src/gui");
            rp.debugSourceLocation.append(base + "qt5base/src/network");
        }
    }

    if (rp.isQmlDebugging) {
        QmlDebug::QmlDebugServicesPreset service;
        if (rp.isCppDebugging) {
            if (rp.nativeMixedEnabled) {
                service = QmlDebug::QmlNativeDebuggerServices;
            } else {
                service = QmlDebug::QmlDebuggerServices;
            }
        } else {
            service = QmlDebug::QmlDebuggerServices;
        }
        if (rp.startMode != AttachExternal && rp.startMode != AttachCrashedExternal) {
            QString qmlarg = rp.isCppDebugging && rp.nativeMixedEnabled
                    ? QmlDebug::qmlDebugNativeArguments(service, false)
                    : QmlDebug::qmlDebugTcpArguments(service, Port(rp.qmlServer.port()));
            QtcProcess::addArg(&rp.inferior.commandLineArguments, qmlarg);
        }
    }

    if (rp.startMode == NoStartMode)
        rp.startMode = StartInternal;

    if (breakOnMainNextTime) {
        rp.breakOnMain = true;
        breakOnMainNextTime = false;
    }

    if (HostOsInfo::isWindowsHost()) {
        QtcProcess::SplitError perr;
        rp.inferior.commandLineArguments =
                QtcProcess::prepareArgs(rp.inferior.commandLineArguments, &perr,
                                        HostOsInfo::hostOs(), nullptr,
                                        &rp.inferior.workingDirectory).toWindowsArgs();
        if (perr != QtcProcess::SplitOk) {
            // perr == BadQuoting is never returned on Windows
            // FIXME? QTCREATORBUG-2809
            reportFailure(DebuggerPlugin::tr("Debugging complex command lines "
                                             "is currently not supported on Windows."));
            return false;
        }
    }

    if (rp.isNativeMixedDebugging())
        rp.inferior.environment.set("QV4_FORCE_INTERPRETER", "1");

    if (rp.isCppDebugging && !rp.skipExecutableValidation)
        rp.validateExecutable();

    return true;
}

Internal::TerminalRunner *DebuggerRunTool::terminalRunner() const
{
    return d->terminalRunner;
}

DebuggerRunTool::DebuggerRunTool(RunControl *runControl, Kit *kit)
    : RunWorker(runControl), d(new DebuggerRunToolPrivate)
{
    setDisplayName("DebuggerRunTool");

    RunConfiguration *runConfig = runControl->runConfiguration();

    runControl->setIcon(ProjectExplorer::Icons::DEBUG_START_SMALL_TOOLBAR);
    runControl->setPromptToStop([](bool *optionalPrompt) {
        return RunControl::showPromptToStopDialog(
            DebuggerRunTool::tr("Close Debugging Session"),
            DebuggerRunTool::tr("A debugging session is still in progress. "
                                "Terminating the session in the current"
                                " state can leave the target in an inconsistent state."
                                " Would you still like to terminate it?"),
                QString(), QString(), optionalPrompt);
    });

    if (runConfig)
        m_runParameters.displayName = runConfig->displayName();

    if (runConfig && !kit)
        kit = runConfig->target()->kit();
    QTC_ASSERT(kit, return);

    m_runParameters.sysRoot = SysRootKitInformation::sysRoot(kit).toString();
    m_runParameters.macroExpander = kit->macroExpander();
    m_runParameters.debugger = DebuggerKitInformation::runnable(kit);

    if (auto aspect = runConfig ? runConfig->extraAspect<DebuggerRunConfigurationAspect>() : nullptr) {
        m_runParameters.isCppDebugging = aspect->useCppDebugger();
        m_runParameters.isQmlDebugging = aspect->useQmlDebugger();
        m_runParameters.multiProcess = aspect->useMultiProcess();
    }

    if (m_runParameters.isCppDebugging)
        m_runParameters.cppEngineType = DebuggerKitInformation::engineType(kit);

    Runnable r = runnable();
    if (r.is<StandardRunnable>()) {
        m_runParameters.inferior = r.as<StandardRunnable>();
        // Normalize to work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch'...)
        m_runParameters.inferior.workingDirectory =
                FileUtils::normalizePathName(m_runParameters.inferior.workingDirectory);
        setUseTerminal(m_runParameters.inferior.runMode == ApplicationLauncher::Console);
    }

    const QByteArray envBinary = qgetenv("QTC_DEBUGGER_PATH");
    if (!envBinary.isEmpty())
        m_runParameters.debugger.executable = QString::fromLocal8Bit(envBinary);

    Project *project = runConfig ? runConfig->target()->project() : nullptr;
    if (project) {
        m_runParameters.projectSourceDirectory = project->projectDirectory().toString();
        m_runParameters.projectSourceFiles = project->files(Project::SourceFiles);
    }

    m_runParameters.toolChainAbi = ToolChainKitInformation::targetAbi(kit);

    bool ok = false;
    int nativeMixedOverride = qgetenv("QTC_DEBUGGER_NATIVE_MIXED").toInt(&ok);
    if (ok)
        m_runParameters.nativeMixedEnabled = bool(nativeMixedOverride);

    // This will only be shown in some cases, but we don't want to access
    // the kit at that time anymore.
    const QList<Task> tasks = DebuggerKitInformation::validateDebugger(kit);
    for (const Task &t : tasks) {
        if (t.type != Task::Warning)
            m_runParameters.validationErrors.append(t.description);
    }

    if (runConfig && runConfig->property("supportsDebugger").toBool()) {
        const QString mainScript = runConfig->property("mainScript").toString();
        const QString interpreter = runConfig->property("interpreter").toString();
        if (!interpreter.isEmpty() && mainScript.endsWith(".py")) {
            m_runParameters.mainScript = mainScript;
            m_runParameters.interpreter = interpreter;
            const QString args = runConfig->property("arguments").toString();
            if (!args.isEmpty()) {
                if (!m_runParameters.inferior.commandLineArguments.isEmpty())
                    m_runParameters.inferior.commandLineArguments.append(' ');
                m_runParameters.inferior.commandLineArguments.append(args);
            }
            m_engine = createPdbEngine();
        }
    }

    if (m_runParameters.cppEngineType == CdbEngineType
            && !boolSetting(UseCdbConsole)
            && m_runParameters.inferior.runMode == ApplicationLauncher::Console
            && (m_runParameters.startMode == StartInternal
                || m_runParameters.startMode == StartExternal)) {
        d->terminalRunner = new TerminalRunner(this);
        addStartDependency(d->terminalRunner);
    }
}

DebuggerEngine *DebuggerRunTool::activeEngine() const
{
    return m_engine ? m_engine->activeEngine() : nullptr;
}

void DebuggerRunTool::startRunControl()
{
    ProjectExplorerPlugin::startRunControl(runControl());
}

void DebuggerRunTool::addSolibSearchDir(const QString &str)
{
    QString path = str;
    path.replace("%{sysroot}", m_runParameters.sysRoot);
    m_runParameters.solibSearchPath.append(path);
}

DebuggerRunTool::~DebuggerRunTool()
{
    disconnect();
    if (m_engine) {
        DebuggerEngine *engine = m_engine;
        m_engine = 0;
        engine->disconnect();
        delete engine;
    }
    delete d;
}

void DebuggerRunTool::showMessage(const QString &msg, int channel, int timeout)
{
    if (channel == ConsoleOutput)
        debuggerConsole()->printItem(ConsoleItem::DefaultType, msg);

    Internal::showMessage(msg, channel, timeout);
    switch (channel) {
    case AppOutput:
        appendMessage(msg, StdOutFormatSameLine);
        break;
    case AppError:
        appendMessage(msg, StdErrFormatSameLine);
        break;
    case AppStuff:
        appendMessage(msg, DebugFormat);
        break;
    default:
        break;
    }
}

////////////////////////////////////////////////////////////////////////
//
// Externally visible helper.
//
////////////////////////////////////////////////////////////////////////

// GdbServerPortGatherer

GdbServerPortsGatherer::GdbServerPortsGatherer(RunControl *runControl)
    : RunWorker(runControl)
{
    setDisplayName("GdbServerPortsGatherer");

    connect(&m_portsGatherer, &DeviceUsedPortsGatherer::error,
            this, &RunWorker::reportFailure);
    connect(&m_portsGatherer, &DeviceUsedPortsGatherer::portListReady,
            this, &GdbServerPortsGatherer::handlePortListReady);

    m_device = runControl->device();
}

GdbServerPortsGatherer::~GdbServerPortsGatherer()
{
}

QString GdbServerPortsGatherer::gdbServerChannel() const
{
    const QString host = m_device->sshParameters().host;
    return QString("%1:%2").arg(host).arg(m_gdbServerPort.number());
}

QUrl GdbServerPortsGatherer::qmlServer() const
{
    QUrl server = m_device->toolControlChannel(IDevice::QmlControlChannel);
    server.setPort(m_qmlServerPort.number());
    return server;
}

void GdbServerPortsGatherer::setDevice(IDevice::ConstPtr device)
{
    m_device = device;
}

void GdbServerPortsGatherer::start()
{
    appendMessage(tr("Checking available ports..."), NormalMessageFormat);
    m_portsGatherer.start(m_device);
}

void GdbServerPortsGatherer::handlePortListReady()
{
    Utils::PortList portList = m_device->freePorts();
    appendMessage(tr("Found %n free ports.", nullptr, portList.count()), NormalMessageFormat);
    if (m_useGdbServer) {
        m_gdbServerPort = m_portsGatherer.getNextFreePort(&portList);
        if (!m_gdbServerPort.isValid()) {
            reportFailure(tr("Not enough free ports on device for C++ debugging."));
            return;
        }
    }
    if (m_useQmlServer) {
        m_qmlServerPort = m_portsGatherer.getNextFreePort(&portList);
        if (!m_qmlServerPort.isValid()) {
            reportFailure(tr("Not enough free ports on device for QML debugging."));
            return;
        }
    }
//    reportDone();
    reportStarted();
}

// GdbServerRunner

GdbServerRunner::GdbServerRunner(RunControl *runControl, GdbServerPortsGatherer *portsGatherer)
   : SimpleTargetRunner(runControl), m_portsGatherer(portsGatherer)
{
    setDisplayName("GdbServerRunner");
    if (runControl->runnable().is<StandardRunnable>())
        m_runnable = runControl->runnable().as<StandardRunnable>();
    addStartDependency(m_portsGatherer);
}

GdbServerRunner::~GdbServerRunner()
{
}

void GdbServerRunner::setRunnable(const StandardRunnable &runnable)
{
    m_runnable = runnable;
}

void GdbServerRunner::setUseMulti(bool on)
{
    m_useMulti = on;
}

void GdbServerRunner::setAttachPid(ProcessHandle pid)
{
    m_pid = pid;
}

void GdbServerRunner::start()
{
    QTC_ASSERT(m_portsGatherer, reportFailure(); return);

    StandardRunnable gdbserver;
    gdbserver.environment = m_runnable.environment;
    gdbserver.workingDirectory = m_runnable.workingDirectory;

    QStringList args = QtcProcess::splitArgs(m_runnable.commandLineArguments, OsTypeLinux);

    const bool isQmlDebugging = m_portsGatherer->useQmlServer();
    const bool isCppDebugging = m_portsGatherer->useGdbServer();

    if (isQmlDebugging) {
        args.prepend(QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices,
                                                    m_portsGatherer->qmlServerPort()));
    }
    if (isQmlDebugging && !isCppDebugging) {
        gdbserver.executable = m_runnable.executable; // FIXME: Case should not happen?
    } else {
        gdbserver.executable = device()->debugServerPath();
        if (gdbserver.executable.isEmpty())
            gdbserver.executable = "gdbserver";
        args.clear();
        if (m_useMulti)
            args.append("--multi");
        if (m_pid.isValid())
            args.append("--attach");
        args.append(QString(":%1").arg(m_portsGatherer->gdbServerPort().number()));
        if (m_pid.isValid())
            args.append(QString::number(m_pid.pid()));
    }
    gdbserver.commandLineArguments = QtcProcess::joinArgs(args, OsTypeLinux);

    SimpleTargetRunner::setRunnable(gdbserver);

    appendMessage(tr("Starting gdbserver..."), NormalMessageFormat);

    SimpleTargetRunner::start();
}

} // namespace Debugger
