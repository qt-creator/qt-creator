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

#include "debuggermainwindow.h"
#include "debuggerruncontrol.h"
#include "terminal.h"

#include "analyzer/analyzermanager.h"
#include "console/console.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggerkitinformation.h"
#include "debuggerplugin.h"
#include "debuggerrunconfigurationaspect.h"
#include "breakhandler.h"
#include "enginemanager.h"
#include "shared/peutils.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/deviceprocessesdialog.h>
#include <projectexplorer/devicesupport/idevice.h>
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
DebuggerEngine *createUvscEngine();

static QString noEngineMessage()
{
   return DebuggerPlugin::tr("Unable to create a debugging engine.");
}

static QString noDebuggerInKitMessage()
{
   return DebuggerPlugin::tr("The kit does not have a debugger set.");
}

class CoreUnpacker final : public RunWorker
{
public:
    CoreUnpacker(RunControl *runControl, const FilePath &coreFilePath)
        : RunWorker(runControl), m_coreFilePath(coreFilePath)
    {}

    FilePath coreFileName() const { return m_tempCoreFilePath; }

private:
    ~CoreUnpacker() final
    {
        if (m_tempCoreFile.isOpen())
            m_tempCoreFile.close();

        m_tempCoreFilePath.removeFile();
    }

    void start() final
    {
        {
            Utils::TemporaryFile tmp("tmpcore-XXXXXX");
            tmp.open();
            m_tempCoreFilePath = FilePath::fromString(tmp.fileName());
        }

        m_coreUnpackProcess.setWorkingDirectory(TemporaryDirectory::masterDirectoryFilePath());
        connect(&m_coreUnpackProcess, &QtcProcess::done, this, [this] {
            if (m_coreUnpackProcess.error() == QProcess::UnknownError) {
                reportStopped();
                return;
            }
            reportFailure("Error unpacking " + m_coreFilePath.toUserOutput());
        });

        const QString msg = DebuggerRunTool::tr("Unpacking core file to %1");
        appendMessage(msg.arg(m_tempCoreFilePath.toUserOutput()), LogMessageFormat);

        if (m_coreFilePath.endsWith(".lzo")) {
            m_coreUnpackProcess.setCommand({"lzop", {"-o", m_tempCoreFilePath.path(),
                                                     "-x", m_coreFilePath.path()}});
            reportStarted();
            m_coreUnpackProcess.start();
            return;
        }

        if (m_coreFilePath.endsWith(".gz")) {
            appendMessage(msg.arg(m_tempCoreFilePath.toUserOutput()), LogMessageFormat);
            m_tempCoreFile.setFileName(m_tempCoreFilePath.path());
            m_tempCoreFile.open(QFile::WriteOnly);
            connect(&m_coreUnpackProcess, &QtcProcess::readyReadStandardOutput, this, [this] {
                m_tempCoreFile.write(m_coreUnpackProcess.readAllStandardOutput());
            });
            m_coreUnpackProcess.setCommand({"gzip", {"-c", "-d", m_coreFilePath.path()}});
            reportStarted();
            m_coreUnpackProcess.start();
            return;
        }

        QTC_CHECK(false);
        reportFailure("Unknown file extension in " + m_coreFilePath.toUserOutput());
    }

    QFile m_tempCoreFile;
    FilePath m_coreFilePath;
    FilePath m_tempCoreFilePath;
    QtcProcess m_coreUnpackProcess;
};

class DebuggerRunToolPrivate
{
public:
    bool useTerminal = false;
    QPointer<CoreUnpacker> coreUnpacker;
    QPointer<DebugServerPortsGatherer> portsGatherer;
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

void DebuggerRunTool::setSysRoot(const Utils::FilePath &sysRoot)
{
    m_runParameters.sysRoot = sysRoot;
}

void DebuggerRunTool::setSymbolFile(const FilePath &symbolFile)
{
    if (symbolFile.isEmpty())
        reportFailure(tr("Cannot debug: Local executable is not set."));
    m_runParameters.symbolFile = symbolFile;
}

void DebuggerRunTool::setLldbPlatform(const QString &platform)
{
    m_runParameters.platform = platform;
}

void DebuggerRunTool::setRemoteChannel(const QString &channel)
{
    m_runParameters.remoteChannel = channel;
}

void DebuggerRunTool::setRemoteChannel(const QUrl &url)
{
    m_runParameters.remoteChannel = QString("%1:%2").arg(url.host()).arg(url.port());
}

QString DebuggerRunTool::remoteChannel() const
{
    return m_runParameters.remoteChannel;
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
            && debuggerSettings()->useCdbConsole.value();

    if (on && !d->terminalRunner && !useCdbConsole) {
        d->terminalRunner =
            new TerminalRunner(runControl(), [this] { return m_runParameters.inferior; });
        addStartDependency(d->terminalRunner);
    }
    if (!on && d->terminalRunner) {
        QTC_CHECK(false); // User code can only switch from no terminal to one terminal.
    }
}

void DebuggerRunTool::setRunAsRoot(bool on)
{
    m_runParameters.runAsRoot = on;
}

void DebuggerRunTool::setCommandsAfterConnect(const QString &commands)
{
    m_runParameters.commandsAfterConnect = commands;
}

void DebuggerRunTool::setCommandsForReset(const QString &commands)
{
    m_runParameters.commandsForReset = commands;
}
 void DebuggerRunTool::setDebugInfoLocation(const FilePath &debugInfoLocation)
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

void DebuggerRunTool::setOverrideStartScript(const FilePath &script)
{
    m_runParameters.overrideStartScript = script;
}

void DebuggerRunTool::setAbi(const Abi &abi)
{
    m_runParameters.toolChainAbi = abi;
}

void DebuggerRunTool::setInferior(const Runnable &runnable)
{
    m_runParameters.inferior = runnable;
}

void DebuggerRunTool::setInferiorExecutable(const FilePath &executable)
{
    m_runParameters.inferior.command.setExecutable(executable);
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

void DebuggerRunTool::setCoreFilePath(const FilePath &coreFile, bool isSnapshot)
{
    if (coreFile.endsWith(".gz") || coreFile.endsWith(".lzo")) {
        d->coreUnpacker = new CoreUnpacker(runControl(), coreFile);
        addStartDependency(d->coreUnpacker);
    }

    m_runParameters.coreFile = coreFile;
    m_runParameters.isSnapshot = isSnapshot;
}

void DebuggerRunTool::addQmlServerInferiorCommandLineArgumentIfNeeded()
{
    d->addQmlServerInferiorCommandLineArgumentIfNeeded = true;
}

void DebuggerRunTool::modifyDebuggerEnvironment(const EnvironmentItems &items)
{
    m_runParameters.debugger.environment.modify(items);
}

void DebuggerRunTool::setCrashParameter(const QString &event)
{
    m_runParameters.crashParameter = event;
}

void DebuggerRunTool::addExpectedSignal(const QString &signal)
{
    m_runParameters.expectedSignals.append(signal);
}

void DebuggerRunTool::addSearchDirectory(const Utils::FilePath &dir)
{
    m_runParameters.additionalSearchDirectories.append(dir);
}

void DebuggerRunTool::start()
{
    TaskHub::clearTasks(Constants::TASK_CATEGORY_DEBUGGER_DEBUGINFO);
    TaskHub::clearTasks(Constants::TASK_CATEGORY_DEBUGGER_RUNTIME);

    if (d->portsGatherer) {
        setRemoteChannel(d->portsGatherer->gdbServer());
        setQmlServer(d->portsGatherer->qmlServer());
        if (d->addQmlServerInferiorCommandLineArgumentIfNeeded
                && m_runParameters.isQmlDebugging
                && m_runParameters.isCppDebugging()) {

            int qmlServerPort = m_runParameters.qmlServer.port();
            QTC_ASSERT(qmlServerPort > 0, reportFailure(); return);
            QString mode = QString("port:%1").arg(qmlServerPort);

            CommandLine cmd{m_runParameters.inferior.command.executable()};
            cmd.addArg(qmlDebugCommandLineArguments(QmlDebug::QmlDebuggerServices, mode, true));
            cmd.addArgs(m_runParameters.inferior.command.arguments(), CommandLine::Raw);

            m_runParameters.inferior.command = cmd;
        }
    }

    // User canceled input dialog asking for executable when working on library project.
    if (m_runParameters.startMode == StartInternal
            && m_runParameters.inferior.command.isEmpty()
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
            && Utils::is64BitWindowsBinary(m_runParameters.inferior.command.executable())
            && !Utils::is64BitWindowsBinary(m_runParameters.debugger.command.executable())) {
        reportFailure(
            DebuggerPlugin::tr(
                "%1 is a 64 bit executable which can not be debugged by a 32 bit Debugger.\n"
                "Please select a 64 bit Debugger in the kit settings for this kit.")
                .arg(m_runParameters.inferior.command.executable().toUserOutput()));
        return;
    }

    Utils::globalMacroExpander()->registerFileVariables(
                "DebuggedExecutable", tr("Debugged executable"),
                [this] { return m_runParameters.inferior.command.executable(); }
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
            case UvscEngineType:
                m_engine = createUvscEngine();
                break;
            default:
                if (!m_runParameters.isQmlDebugging) {
                    reportFailure(noEngineMessage() + '\n' +
                        DebuggerPlugin::tr("Specify Debugger settings in Projects > Run."));
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
        QString msg = noEngineMessage();
        if (!DebuggerKitAspect::debugger(runControl()->kit()))
            msg += '\n' + noDebuggerInKitMessage();
        reportFailure(msg);
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
        auto rc = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        rc->copyFromRunControl(runControl());
        auto name = QString(tr("%1 - Snapshot %2").arg(runControl()->displayName()).arg(++d->snapshotCounter));
        auto debugger = new DebuggerRunTool(rc);
        debugger->setStartMode(AttachToCore);
        debugger->setRunControlName(name);
        debugger->setCoreFilePath(FilePath::fromString(coreFile), true);
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
        bool hasQmlBreakpoints = false;
        for (const GlobalBreakpoint &gbp : BreakpointManager::globalBreakpoints()) {
            if (gbp->isEnabled()) {
                const BreakpointParameters &bp = gbp->requestedParameters();
                hasQmlBreakpoints = hasQmlBreakpoints || bp.isQmlFileAndLineBreakpoint();
                if (!m_engine->acceptsBreakpoint(bp)) {
                    if (!m_engine2 || !m_engine2->acceptsBreakpoint(bp))
                        unhandledIds.append(gbp->displayName());
                }
            }
        }
        if (!unhandledIds.isEmpty()) {
            QString warningMessage =
                    DebuggerPlugin::tr("Some breakpoints cannot be handled by the debugger "
                                       "languages currently active, and will be ignored.<p>"
                                       "Affected are breakpoints %1")
                    .arg(unhandledIds.join(", "));

            if (hasQmlBreakpoints) {
                warningMessage += "<p>" +
                    DebuggerPlugin::tr("QML debugging needs to be enabled both in the Build "
                                       "and the Run settings.");
            }

            showMessage(warningMessage, LogWarning);

            static bool checked = true;
            if (checked)
                CheckableMessageBox::information(Core::ICore::dialogParent(),
                                                 tr("Debugger"),
                                                 warningMessage,
                                                 tr("&Show this message again."),
                                                 &checked, QDialogButtonBox::Ok);
        }
    }

    appendMessage(tr("Debugging %1 ...").arg(m_runParameters.inferior.command.toUserOutput()),
                  NormalMessageFormat);
    QString debuggerName = m_engine->objectName();
    if (m_engine2)
        debuggerName += ' ' + m_engine2->objectName();

    const QString message = tr("Starting debugger \"%1\" for ABI \"%2\"...")
            .arg(debuggerName).arg(m_runParameters.toolChainAbi.toString());
    DebuggerMainWindow::showStatusMessage(message, 10000);

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
        QString cmd = m_runParameters.inferior.command.toUserOutput();
        QString msg = engine->runParameters().exitCode // Main engine.
            ? tr("Debugging of %1 has finished with exit code %2.")
                .arg(cmd).arg(engine->runParameters().exitCode.value())
            : tr("Debugging of %1 has finished.").arg(cmd);
        appendMessage(msg, NormalMessageFormat);
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
    d->portsGatherer = new DebugServerPortsGatherer(runControl());
    d->portsGatherer->setUseGdbServer(useCpp);
    d->portsGatherer->setUseQmlServer(useQml);
    addStartDependency(d->portsGatherer);
}

DebugServerPortsGatherer *DebuggerRunTool::portsGatherer() const
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
        rp.symbolFile = rp.inferior.command.executable();

    // Copy over DYLD_IMAGE_SUFFIX etc
    for (const auto &var :
         QStringList({"DYLD_IMAGE_SUFFIX", "DYLD_LIBRARY_PATH", "DYLD_FRAMEWORK_PATH"}))
        if (rp.inferior.environment.hasKey(var))
            rp.debugger.environment.set(var, rp.inferior.environment.expandedValueForKey(var));

    // validate debugger if C++ debugging is enabled
    if (!rp.validationErrors.isEmpty()) {
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

    if (!debuggerSettings()->autoEnrichParameters.value()) {
        const FilePath sysroot = rp.sysRoot;
        if (rp.debugInfoLocation.isEmpty())
            rp.debugInfoLocation = sysroot / "/usr/lib/debug";
        if (rp.debugSourceLocation.isEmpty()) {
            QString base = sysroot.toString() + "/usr/src/debug/";
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
        if (rp.startMode != AttachToLocalProcess && rp.startMode != AttachToCrashedProcess) {
            QString qmlarg = rp.isCppDebugging() && rp.nativeMixedEnabled
                    ? QmlDebug::qmlDebugNativeArguments(service, false)
                    : QmlDebug::qmlDebugTcpArguments(service, rp.qmlServer);
            rp.inferior.command.addArg(qmlarg);
        }
    }

    if (rp.startMode == NoStartMode)
        rp.startMode = StartInternal;

    if (breakOnMainNextTime) {
        rp.breakOnMain = true;
        breakOnMainNextTime = false;
    }

    if (HostOsInfo::isWindowsHost()) {
        ProcessArgs::SplitError perr;
        rp.inferior.command.setArguments(
                ProcessArgs::prepareArgs(rp.inferior.command.arguments(), &perr,
                                         HostOsInfo::hostOs(), nullptr,
                                         &rp.inferior.workingDirectory).toWindowsArgs());
        if (perr != ProcessArgs::SplitOk) {
            // perr == BadQuoting is never returned on Windows
            // FIXME? QTCREATORBUG-2809
            reportFailure(DebuggerPlugin::tr("Debugging complex command lines "
                                             "is currently not supported on Windows."));
            return false;
        }
    }

    if (rp.isNativeMixedDebugging())
        rp.inferior.environment.set("QV4_FORCE_INTERPRETER", "1");

    if (debuggerSettings()->forceLoggingToConsole.value())
        rp.inferior.environment.set("QT_LOGGING_TO_CONSOLE", "1");

    return true;
}

Internal::TerminalRunner *DebuggerRunTool::terminalRunner() const
{
    return d->terminalRunner;
}

DebuggerEngineType DebuggerRunTool::cppEngineType() const
{
    return m_runParameters.cppEngineType;
}

DebuggerRunTool::DebuggerRunTool(RunControl *runControl, AllowTerminal allowTerminal)
    : RunWorker(runControl), d(new DebuggerRunToolPrivate)
{
    setId("DebuggerRunTool");

    static int toolRunCount = 0;

    // Reset once all are gone.
    if (EngineManager::engines().isEmpty())
        toolRunCount = 0;

    d->runId = QString::number(++toolRunCount);

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

    m_runParameters.displayName = runControl->displayName();

    if (auto symbolsAspect = runControl->aspect<SymbolFileAspect>())
        m_runParameters.symbolFile = symbolsAspect->filePath;
    if (auto terminalAspect = runControl->aspect<TerminalAspect>())
        m_runParameters.useTerminal = terminalAspect->useTerminal;
    if (auto runAsRootAspect = runControl->aspect<RunAsRootAspect>())
        m_runParameters.runAsRoot = runAsRootAspect->value;

    Kit *kit = runControl->kit();
    QTC_ASSERT(kit, return);

    m_runParameters.sysRoot = SysRootKitAspect::sysRoot(kit);
    m_runParameters.macroExpander = runControl->macroExpander();
    m_runParameters.debugger = DebuggerKitAspect::runnable(kit);
    m_runParameters.cppEngineType = DebuggerKitAspect::engineType(kit);

    if (QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(kit))
        m_runParameters.qtPackageSourceLocation = qtVersion->qtPackageSourcePath().toString();

    if (auto aspect = runControl->aspect<DebuggerRunConfigurationAspect>()) {
        if (!aspect->useCppDebugger)
            m_runParameters.cppEngineType = NoEngineType;
        m_runParameters.isQmlDebugging = aspect->useQmlDebugger;
        m_runParameters.multiProcess = aspect->useMultiProcess;
        m_runParameters.additionalStartupCommands = aspect->overrideStartup;

        if (aspect->useCppDebugger) {
            if (DebuggerKitAspect::debugger(kit)) {
                const Tasks tasks = DebuggerKitAspect::validateDebugger(kit);
                for (const Task &t : tasks) {
                    if (t.type != Task::Warning)
                        m_runParameters.validationErrors.append(t.description());
                }
            } else {
                m_runParameters.validationErrors.append(noDebuggerInKitMessage());
            }
        }
    }

    Runnable inferior = runnable();
    const FilePath &debuggerExecutable = m_runParameters.debugger.command.executable();
    inferior.command.setExecutable(inferior.command.executable().onDevice(debuggerExecutable));
    inferior.workingDirectory = inferior.workingDirectory.onDevice(debuggerExecutable);
    // Normalize to work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch'...)
    inferior.workingDirectory = inferior.workingDirectory.normalizedPathName();
    m_runParameters.inferior = inferior;

    setUseTerminal(allowTerminal == DoAllowTerminal && m_runParameters.useTerminal);

    const QByteArray envBinary = qgetenv("QTC_DEBUGGER_PATH");
    if (!envBinary.isEmpty())
        m_runParameters.debugger.command.setExecutable(FilePath::fromString(QString::fromLocal8Bit(envBinary)));

    if (Project *project = runControl->project()) {
        m_runParameters.projectSourceDirectory = project->projectDirectory();
        m_runParameters.projectSourceFiles = project->files(Project::SourceFiles);
    }

    m_runParameters.toolChainAbi = ToolChainKitAspect::targetAbi(kit);

    bool ok = false;
    const int nativeMixedOverride = qEnvironmentVariableIntValue("QTC_DEBUGGER_NATIVE_MIXED", &ok);
    if (ok)
        m_runParameters.nativeMixedEnabled = bool(nativeMixedOverride);


    if (auto interpreterAspect = runControl->aspect<InterpreterAspect>()) {
        if (auto mainScriptAspect = runControl->aspect<MainScriptAspect>()) {
            const FilePath mainScript = mainScriptAspect->filePath;
            const FilePath interpreter = interpreterAspect->interpreter.command;
            if (!interpreter.isEmpty() && mainScript.endsWith(".py")) {
                m_runParameters.mainScript = mainScript;
                m_runParameters.interpreter = interpreter;
                if (auto args = runControl->aspect<ArgumentsAspect>())
                    m_runParameters.inferior.command.addArgs(args->arguments, CommandLine::Raw);
                m_engine = createPdbEngine();
            }
        }
    }

    m_runParameters.dumperPath = Core::ICore::resourcePath("debugger/");
    if (QtSupport::QtVersion *baseQtVersion = QtSupport::QtKitAspect::qtVersion(kit)) {
        QtSupport::QtVersionNumber qtVersion = baseQtVersion->qtVersion();
        m_runParameters.fallbackQtVersion = 0x10000 * int(qtVersion.majorVersion)
                                            + 0x100 * int(qtVersion.minorVersion)
                                            + int(qtVersion.patchVersion);
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
        m_runParameters.coreFile.removeFile();

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

    QTC_ASSERT(m_engine, qDebug() << msg; return);

    m_engine->showMessage(msg, channel, timeout);
    if (m_engine2)
        m_engine->showMessage(msg, channel, timeout);
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

////////////////////////////////////////////////////////////////////////
//
// Externally visible helper.
//
////////////////////////////////////////////////////////////////////////

// GdbServerPortGatherer

DebugServerPortsGatherer::DebugServerPortsGatherer(RunControl *runControl)
    : ChannelProvider(runControl, 2)
{
    setId("DebugServerPortsGatherer");
}

DebugServerPortsGatherer::~DebugServerPortsGatherer() = default;

QUrl DebugServerPortsGatherer::gdbServer() const
{
    return channel(0);
}

QUrl DebugServerPortsGatherer::qmlServer() const
{
    return channel(1);
}

// DebugServerRunner

DebugServerRunner::DebugServerRunner(RunControl *runControl, DebugServerPortsGatherer *portsGatherer)
   : SimpleTargetRunner(runControl)
{
    setId("DebugServerRunner");
    const Runnable mainRunnable = runControl->runnable();
    addStartDependency(portsGatherer);

    QTC_ASSERT(portsGatherer, reportFailure(); return);

    setStarter([this, runControl, mainRunnable, portsGatherer] {
        QTC_ASSERT(portsGatherer, reportFailure(); return);

        Runnable debugServer;
        debugServer.environment = mainRunnable.environment;
        debugServer.workingDirectory = mainRunnable.workingDirectory;

        QStringList args = ProcessArgs::splitArgs(mainRunnable.command.arguments(), OsTypeLinux);

        const bool isQmlDebugging = portsGatherer->useQmlServer();
        const bool isCppDebugging = portsGatherer->useGdbServer();

        if (isQmlDebugging) {
            args.prepend(QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices,
                                                        portsGatherer->qmlServer()));
        }
        if (isQmlDebugging && !isCppDebugging) {
            debugServer.command.setExecutable(mainRunnable.command.executable()); // FIXME: Case should not happen?
        } else {
            debugServer.command.setExecutable(runControl->device()->debugServerPath());
            if (debugServer.command.isEmpty())
                debugServer.command.setExecutable("gdbserver");
            args.clear();
            if (debugServer.command.executable().toString().contains("lldb-server")) {
                args.append("platform");
                args.append("--listen");
                args.append(QString("*:%1").arg(portsGatherer->gdbServer().port()));
                args.append("--server");
            } else {
                // Something resembling gdbserver
                if (m_useMulti)
                    args.append("--multi");
                if (m_pid.isValid())
                    args.append("--attach");
                args.append(QString(":%1").arg(portsGatherer->gdbServer().port()));
                if (m_pid.isValid())
                    args.append(QString::number(m_pid.pid()));
            }
        }
        debugServer.command.setArguments(ProcessArgs::joinArgs(args, OsTypeLinux));
        debugServer.device = runControl->device();
        doStart(debugServer);
    });
}

DebugServerRunner::~DebugServerRunner() = default;

void DebugServerRunner::setUseMulti(bool on)
{
    m_useMulti = on;
}

void DebugServerRunner::setAttachPid(ProcessHandle pid)
{
    m_pid = pid;
}

} // namespace Debugger
