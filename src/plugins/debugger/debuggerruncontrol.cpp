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
#include "enginemanager.h"
#include "shared/peutils.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/deviceprocessesdialog.h>
#include <projectexplorer/devicesupport/deviceprocesslist.h>
#include <projectexplorer/environmentaspect.h> // For the environment
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/fileutils.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>
#include <utils/temporaryfile.h>
#include <utils/url.h>
#include <utils/winutils.h>

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/messagebox.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <qtsupport/qtkitinformation.h>

#include <ssh/sshconnection.h>

#include <QTcpServer>
#include <QTimer>

using namespace Core;
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
DebuggerEngine *createLldbEngine();

class LocalProcessRunner : public RunWorker
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::LocalProcessRunner)

public:
    LocalProcessRunner(DebuggerRunTool *runTool, const Runnable &runnable)
        : RunWorker(runTool->runControl()), m_runTool(runTool), m_runnable(runnable)
    {
        connect(&m_proc, &QProcess::errorOccurred,
                this, &LocalProcessRunner::handleError);
        connect(&m_proc, &QProcess::readyReadStandardOutput,
                this, &LocalProcessRunner::handleStandardOutput);
        connect(&m_proc, &QProcess::readyReadStandardError,
                this, &LocalProcessRunner::handleStandardError);
        connect(&m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
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
        m_runTool->appendMessage(msg, StdOutFormatSameLine);
    }

    void handleStandardError()
    {
        const QByteArray ba = m_proc.readAllStandardError();
        const QString msg = QString::fromLocal8Bit(ba, ba.length());
        m_runTool->appendMessage(msg, StdErrFormatSameLine);
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

        m_runTool->showMessage(msg, StatusBar);
        Core::AsynchronousMessageBox::critical(tr("Error"), msg);
    }

    QPointer<DebuggerRunTool> m_runTool;
    Runnable m_runnable;
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
        connect(&m_coreUnpackProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
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
    bool useTerminal = false;
    QPointer<CoreUnpacker> coreUnpacker;
    QPointer<GdbServerPortsGatherer> portsGatherer;
    bool addQmlServerInferiorCommandLineArgumentIfNeeded = false;
    TerminalRunner *terminalRunner = nullptr;
    int snapshotCounter = 0;
    int engineStartsNeeded = 0;
    int engineStopsNeeded = 0;
    QString runId;
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
        m_runParameters.cppEngineType = NoEngineType;
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
            m_runParameters.projectSourceDirectory = projects.first()->projectDirectory();

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

void DebuggerRunTool::setSysRoot(const Utils::FileName &sysRoot)
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

void DebuggerRunTool::setRemoteChannel(const QUrl &url)
{
    m_runParameters.remoteChannel = QString("%1:%2").arg(url.host()).arg(url.port());
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
    // CDB has a built-in console that might be preferred by some.
    bool useCdbConsole = m_runParameters.cppEngineType == CdbEngineType
            && (m_runParameters.startMode == StartInternal
                || m_runParameters.startMode == StartExternal)
            && boolSetting(UseCdbConsole);

    if (on && !d->terminalRunner && !useCdbConsole) {
        d->terminalRunner = new TerminalRunner(runControl(), m_runParameters.inferior);
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
        Runnable serverStarter;
        serverStarter.executable = serverStartScript;
        QtcProcess::addArg(&serverStarter.commandLineArguments, m_runParameters.inferior.executable);
        QtcProcess::addArg(&serverStarter.commandLineArguments, m_runParameters.remoteChannel);
        addStartDependency(new LocalProcessRunner(this, serverStarter));
    }
}

void DebuggerRunTool::setDebugInfoLocation(const QString &debugInfoLocation)
{
    m_runParameters.debugInfoLocation = debugInfoLocation;
}

QUrl DebuggerRunTool::qmlServer() const
{
    return m_runParameters.qmlServer;
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
    m_runParameters.inferior = runnable;
}

void DebuggerRunTool::setInferiorExecutable(const QString &executable)
{
    m_runParameters.inferior.executable = executable;
}

void DebuggerRunTool::setInferiorEnvironment(const Utils::Environment &env)
{
    m_runParameters.inferior.environment = env;
}

void DebuggerRunTool::setInferiorDevice(IDevice::ConstPtr device)
{
    m_runParameters.inferior.device = device;
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
    QtcProcess::addArg(&m_runParameters.inferior.commandLineArguments, arg,
                       device() ? device()->osType() : HostOsInfo::hostOs());
}

void DebuggerRunTool::prependInferiorCommandLineArgument(const QString &arg)
{
    if (!m_runParameters.inferior.commandLineArguments.isEmpty())
        m_runParameters.inferior.commandLineArguments.prepend(' ');
    m_runParameters.inferior.commandLineArguments.prepend(
                QtcProcess::quoteArg(arg, device() ? device()->osType() : HostOsInfo::hostOs()));
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

void DebuggerRunTool::addSearchDirectory(const Utils::FileName &dir)
{
    m_runParameters.additionalSearchDirectories.append(dir);
}

void DebuggerRunTool::start()
{
    TaskHub::clearTasks(Debugger::Constants::TASK_CATEGORY_DEBUGGER_DEBUGINFO);
    TaskHub::clearTasks(Debugger::Constants::TASK_CATEGORY_DEBUGGER_RUNTIME);

    if (d->portsGatherer) {
        setRemoteChannel(d->portsGatherer->gdbServer());
        setQmlServer(d->portsGatherer->qmlServer());
        if (d->addQmlServerInferiorCommandLineArgumentIfNeeded
                && m_runParameters.isQmlDebugging
                && m_runParameters.isCppDebugging()) {
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

    if (m_runParameters.cppEngineType == CdbEngineType
            && Utils::is64BitWindowsBinary(m_runParameters.inferior.executable)
            && !Utils::is64BitWindowsBinary(m_runParameters.debugger.executable)) {
        reportFailure(
            DebuggerPlugin::tr(
                "%1 is a 64 bit executable which can not be debugged by a 32 bit Debugger.\n"
                "Please select a 64 bit Debugger in the kit settings for this kit.")
                .arg(m_runParameters.inferior.executable));
        return;
    }

    Utils::globalMacroExpander()->registerFileVariables(
                "DebuggedExecutable", tr("Debugged executable"),
                [this] { return m_runParameters.inferior.executable; }
    );

    runControl()->setDisplayName(m_runParameters.displayName);

    if (!m_engine) {
        if (m_runParameters.isCppDebugging()) {
            switch (m_runParameters.cppEngineType) {
            case GdbEngineType:
                m_engine = createGdbEngine();
                break;
            case CdbEngineType:
                if (!HostOsInfo::isWindowsHost()) {
                    reportFailure(tr("Unsupported CDB host system."));
                    return;
                }
                m_engine = createCdbEngine();
                break;
            case LldbEngineType:
                m_engine = createLldbEngine();
                break;
            case PdbEngineType: // FIXME: Yes, Python counts as C++...
                QTC_CHECK(false); // Called from DebuggerRunTool constructor already.
//                m_engine = createPdbEngine();
                break;
            default:
                if (!m_runParameters.isQmlDebugging) {
                    reportFailure(DebuggerPlugin::tr("Unable to create a debugging engine. "
                                                     "Please select a Debugger Setting from the Run page of the project mode."));
                    return;
                }
                // Can happen for pure Qml.
                break;
            }
        }

        if (m_runParameters.isQmlDebugging) {
            if (m_engine) {
                m_engine2 = createQmlEngine();
            } else {
                m_engine = createQmlEngine();
            }
        }
    }

    if (!m_engine) {
        reportFailure(DebuggerPlugin::tr("Unable to create a debugging engine."));
        return;
    }

    m_engine->setRunParameters(m_runParameters);
    m_engine->setRunId(d->runId);
    m_engine->setRunTool(this);
    m_engine->setCompanionEngine(m_engine2);
    connect(m_engine, &DebuggerEngine::requestRunControlFinish,
            runControl(), &RunControl::initiateFinish);
    connect(m_engine, &DebuggerEngine::requestRunControlStop,
            runControl(), &RunControl::initiateStop);
    connect(m_engine, &DebuggerEngine::engineStarted,
            this, [this] { handleEngineStarted(m_engine); });
    connect(m_engine, &DebuggerEngine::engineFinished,
            this, [this] { handleEngineFinished(m_engine); });
    connect(m_engine, &DebuggerEngine::appendMessageRequested,
            this, &DebuggerRunTool::appendMessage);
    ++d->engineStartsNeeded;
    ++d->engineStopsNeeded;

    connect(m_engine, &DebuggerEngine::attachToCoreRequested, this, [this](const QString &coreFile) {
        auto runConfig = runControl()->runConfiguration();
        QTC_ASSERT(runConfig, return);
        auto rc = new RunControl(runConfig, ProjectExplorer::Constants::DEBUG_RUN_MODE);
        auto name = QString(tr("%1 - Snapshot %2").arg(runControl()->displayName()).arg(++d->snapshotCounter));
        auto debugger = new DebuggerRunTool(rc);
        debugger->setStartMode(AttachCore);
        debugger->setRunControlName(name);
        debugger->setCoreFileName(coreFile, true);
        debugger->startRunControl();
    });

    if (m_engine2) {
        m_engine2->setRunParameters(m_runParameters);
        m_engine2->setRunId(d->runId);
        m_engine2->setRunTool(this);
        m_engine2->setCompanionEngine(m_engine);
        m_engine2->setSecondaryEngine();
        connect(m_engine2, &DebuggerEngine::requestRunControlFinish,
                runControl(), &RunControl::initiateFinish);
        connect(m_engine2, &DebuggerEngine::requestRunControlStop,
                runControl(), &RunControl::initiateStop);
        connect(m_engine2, &DebuggerEngine::engineStarted,
                this, [this] { handleEngineStarted(m_engine2); });
        connect(m_engine2, &DebuggerEngine::engineFinished,
                this, [this] { handleEngineFinished(m_engine2); });
        connect(m_engine2, &DebuggerEngine::appendMessageRequested,
                this, &DebuggerRunTool::appendMessage);
        ++d->engineStartsNeeded;
        ++d->engineStopsNeeded;
    }

    if (m_runParameters.startMode == StartInternal) {
        QStringList unhandledIds;
        for (const GlobalBreakpoint bp : BreakpointManager::globalBreakpoints()) {
//            if (bp->isEnabled() && !m_engine->acceptsBreakpoint(bp))
//                unhandledIds.append(bp.id().toString());
        }
        if (!unhandledIds.isEmpty()) {
            QString warningMessage =
                    DebuggerPlugin::tr("Some breakpoints cannot be handled by the debugger "
                                       "languages currently active, and will be ignored.\n"
                                       "Affected are breakpoints %1")
                    .arg(unhandledIds.join(", "));

            showMessage(warningMessage, LogWarning);

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
    QString debuggerName = m_engine->objectName();
    if (m_engine2)
        debuggerName += ' ' + m_engine2->objectName();
    const QString message = tr("Starting debugger \"%1\" for ABI \"%2\"...")
            .arg(debuggerName).arg(m_runParameters.toolChainAbi.toString());
    showStatusMessage(message);

    showMessage(m_engine->formatStartParameters(), LogDebug);
    showMessage(DebuggerSettings::dump(), LogDebug);

    if (m_engine2)
        m_engine2->start();
    m_engine->start();
}

void DebuggerRunTool::stop()
{
    QTC_ASSERT(m_engine, reportStopped(); return);
    if (m_engine2)
        m_engine2->quitDebugger();
    m_engine->quitDebugger();
}

void DebuggerRunTool::handleEngineStarted(DebuggerEngine *engine)
{
    // Correct:
//    if (--d->engineStartsNeeded == 0) {
//        EngineManager::activateDebugMode();
//        reportStarted();
//    }

    // Feels better, as the QML Engine might attach late or not at all.
    if (engine == m_engine) {
        EngineManager::activateDebugMode();
        reportStarted();
    }
}

void DebuggerRunTool::handleEngineFinished(DebuggerEngine *engine)
{
    engine->prepareForRestart();
    if (--d->engineStopsNeeded == 0) {
        appendMessage(tr("Debugging has finished"), NormalMessageFormat);
        reportStopped();
    }
}

bool DebuggerRunTool::isCppDebugging() const
{
    return m_runParameters.isCppDebugging();
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

bool DebuggerRunTool::fixupParameters()
{
    DebuggerRunParameters &rp = m_runParameters;
    if (rp.symbolFile.isEmpty())
        rp.symbolFile = rp.inferior.executable;

    // Copy over DYLD_IMAGE_SUFFIX etc
    for (const auto &var :
         QStringList({"DYLD_IMAGE_SUFFIX", "DYLD_LIBRARY_PATH", "DYLD_FRAMEWORK_PATH"}))
        if (rp.inferior.environment.hasKey(var))
            rp.debugger.environment.set(var, rp.inferior.environment.value(var));

    // validate debugger if C++ debugging is enabled
    if (rp.isCppDebugging() && !rp.validationErrors.isEmpty()) {
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
        const QString sysroot = rp.sysRoot.toString();
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
        if (rp.isCppDebugging()) {
            if (rp.nativeMixedEnabled) {
                service = QmlDebug::QmlNativeDebuggerServices;
            } else {
                service = QmlDebug::QmlDebuggerServices;
            }
        } else {
            service = QmlDebug::QmlDebuggerServices;
        }
        if (rp.startMode != AttachExternal && rp.startMode != AttachCrashedExternal) {
            QString qmlarg = rp.isCppDebugging() && rp.nativeMixedEnabled
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

    return true;
}

Internal::TerminalRunner *DebuggerRunTool::terminalRunner() const
{
    return d->terminalRunner;
}

DebuggerRunTool::DebuggerRunTool(RunControl *runControl, Kit *kit, bool allowTerminal)
    : RunWorker(runControl), d(new DebuggerRunToolPrivate)
{
    setId("DebuggerRunTool");

    static int toolRunCount = 0;

    // Reset once all are gone.
    if (EngineManager::engines().isEmpty())
        toolRunCount = 0;

    d->runId = QString::number(++toolRunCount);

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

    if (runConfig) {
        m_runParameters.displayName = runConfig->displayName();
        if (auto symbolsAspect = runConfig->aspect<SymbolFileAspect>())
            m_runParameters.symbolFile = symbolsAspect->value();
        if (auto terminalAspect = runConfig->aspect<TerminalAspect>())
            m_runParameters.useTerminal = terminalAspect->useTerminal();
    }

    if (runConfig && !kit)
        kit = runConfig->target()->kit();
    QTC_ASSERT(kit, return);

    m_runParameters.sysRoot = SysRootKitInformation::sysRoot(kit);
    m_runParameters.macroExpander = kit->macroExpander();
    m_runParameters.debugger = DebuggerKitInformation::runnable(kit);
    m_runParameters.cppEngineType = DebuggerKitInformation::engineType(kit);

    if (QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(kit))
        m_runParameters.qtPackageSourceLocation = qtVersion->qtPackageSourcePath().toString();

    if (auto aspect = runConfig ? runConfig->aspect<DebuggerRunConfigurationAspect>() : nullptr) {
        if (!aspect->useCppDebugger())
            m_runParameters.cppEngineType = NoEngineType;
        m_runParameters.isQmlDebugging = aspect->useQmlDebugger();
        m_runParameters.multiProcess = aspect->useMultiProcess();
    }

    m_runParameters.inferior = runnable();
    // Normalize to work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch'...)
    m_runParameters.inferior.workingDirectory =
            FileUtils::normalizePathName(m_runParameters.inferior.workingDirectory);
    setUseTerminal(allowTerminal && m_runParameters.useTerminal);

    const QByteArray envBinary = qgetenv("QTC_DEBUGGER_PATH");
    if (!envBinary.isEmpty())
        m_runParameters.debugger.executable = QString::fromLocal8Bit(envBinary);

    Project *project = runConfig ? runConfig->target()->project() : nullptr;
    if (project) {
        m_runParameters.projectSourceDirectory = project->projectDirectory();
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
}

void DebuggerRunTool::startRunControl()
{
    ProjectExplorerPlugin::startRunControl(runControl());
}

void DebuggerRunTool::addSolibSearchDir(const QString &str)
{
    QString path = str;
    path.replace("%{sysroot}", m_runParameters.sysRoot.toString());
    m_runParameters.solibSearchPath.append(path);
}

DebuggerRunTool::~DebuggerRunTool()
{
    if (m_runParameters.isSnapshot && !m_runParameters.coreFile.isEmpty())
        QFile::remove(m_runParameters.coreFile);

    delete m_engine2;
    m_engine2 = nullptr;
    delete m_engine;
    m_engine = nullptr;

    delete d;
}

void DebuggerRunTool::showMessage(const QString &msg, int channel, int timeout)
{
    if (channel == ConsoleOutput)
        debuggerConsole()->printItem(ConsoleItem::DefaultType, msg);

    m_engine->showMessage(msg, channel, timeout);
    if (m_engine2)
        m_engine->showMessage(msg, channel, timeout);
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
    : ChannelProvider(runControl, 2)
{
    setId("GdbServerPortsGatherer");
    m_device = runControl->device();
}

GdbServerPortsGatherer::~GdbServerPortsGatherer() = default;

Port GdbServerPortsGatherer::gdbServerPort() const
{
    QUrl url = channel(0);
    return Port(url.port());
}

QUrl GdbServerPortsGatherer::gdbServer() const
{
    return channel(0);
}

Port GdbServerPortsGatherer::qmlServerPort() const
{
    QUrl url = channel(1);
    return Port(url.port());
}

QUrl GdbServerPortsGatherer::qmlServer() const
{
    return channel(1);
}

void GdbServerPortsGatherer::setDevice(IDevice::ConstPtr device)
{
    m_device = device;
}

// GdbServerRunner

GdbServerRunner::GdbServerRunner(RunControl *runControl, GdbServerPortsGatherer *portsGatherer)
   : SimpleTargetRunner(runControl), m_portsGatherer(portsGatherer)
{
    setId("GdbServerRunner");
    m_runnable = runControl->runnable();
    addStartDependency(m_portsGatherer);
}

GdbServerRunner::~GdbServerRunner() = default;

void GdbServerRunner::setRunnable(const Runnable &runnable)
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

    Runnable gdbserver;
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
    SimpleTargetRunner::start();
}

} // namespace Debugger
