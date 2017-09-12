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

DebuggerEngine *createCdbEngine(QStringList *error, DebuggerStartMode sm);
DebuggerEngine *createGdbEngine(bool useTerminal, DebuggerStartMode sm);
DebuggerEngine *createPdbEngine();
DebuggerEngine *createQmlEngine(bool useTerminal);
DebuggerEngine *createQmlCppEngine(DebuggerEngine *cppEngine, bool useTerminal);
DebuggerEngine *createLldbEngine();

static bool fixupParameters(DebuggerRunParameters &rp, RunControl *runControl, QStringList &m_errors);

class LocalProcessRunner : public RunWorker
{
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

} // namespace Internal


static bool breakOnMainNextTime = false;

void DebuggerRunTool::setBreakOnMainNextTime()
{
    breakOnMainNextTime = true;
}

void DebuggerRunTool::setStartMode(DebuggerStartMode startMode)
{
    m_runParameters.startMode = startMode;
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
    m_runParameters.useTerminal = on;
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
    m_runParameters.serverStartScript = serverStartScript;
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

void DebuggerRunTool::setNeedFixup(bool on)
{
    m_runParameters.needFixup = on;
}

void DebuggerRunTool::setTestCase(int testCase)
{
    m_runParameters.testCase = testCase;
}

void DebuggerRunTool::setOverrideStartScript(const QString &script)
{
    m_runParameters.overrideStartScript = script;
}

void DebuggerRunTool::setToolChainAbi(const Abi &abi)
{
    m_runParameters.toolChainAbi = abi;
}

void DebuggerRunTool::setInferior(const Runnable &runnable)
{
    QTC_ASSERT(runnable.is<StandardRunnable>(), reportFailure(); return);
    m_runParameters.inferior = runnable.as<StandardRunnable>();
}

void DebuggerRunTool::setInferiorExecutable(const QString &executable)
{
    m_runParameters.inferior.executable = executable;
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
    if (isQmlDebugging() && isCppDebugging()) {
        using namespace QmlDebug;
        int qmlServerPort = m_runParameters.qmlServer.port();
        QTC_ASSERT(qmlServerPort > 0, reportFailure(); return);
        QString mode = QString("port:%1").arg(qmlServerPort);
        QString qmlServerArg = qmlDebugCommandLineArguments(QmlDebuggerServices, mode, true);
        prependInferiorCommandLineArgument(qmlServerArg);
    }
}

void DebuggerRunTool::setMasterEngineType(DebuggerEngineType engineType)
{
    m_runParameters.masterEngineType = engineType;
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

static QLatin1String engineTypeName(DebuggerEngineType et)
{
    switch (et) {
    case Debugger::NoEngineType:
        break;
    case Debugger::GdbEngineType:
        return QLatin1String("Gdb engine");
    case Debugger::CdbEngineType:
        return QLatin1String("Cdb engine");
    case Debugger::PdbEngineType:
        return QLatin1String("Pdb engine");
    case Debugger::QmlEngineType:
        return QLatin1String("QML engine");
    case Debugger::QmlCppEngineType:
        return QLatin1String("QML C++ engine");
    case Debugger::LldbEngineType:
        return QLatin1String("LLDB command line engine");
    case Debugger::AllEngineTypes:
        break;
    }
    return QLatin1String("No engine");
}

void DebuggerRunTool::start()
{
    Debugger::Internal::saveModeToRestore();
    Debugger::selectPerspective(Debugger::Constants::CppPerspectiveId);
    TaskHub::clearTasks(Debugger::Constants::TASK_CATEGORY_DEBUGGER_DEBUGINFO);
    TaskHub::clearTasks(Debugger::Constants::TASK_CATEGORY_DEBUGGER_RUNTIME);

    // User canceled input dialog asking for executable when working on library project.
    if (m_runParameters.startMode == StartInternal
            && m_runParameters.inferior.executable.isEmpty()
            && m_runParameters.interpreter.isEmpty()) {
        reportFailure(tr("No executable specified."));
        return;
    }

    // QML and/or mixed are not prepared for it.
    setSupportsReRunning(!(m_runParameters.languages & QmlLanguage));

    // FIXME: Disabled due to Android. Make Android device report available ports instead.
//    int portsUsed = portsUsedByDebugger();
//    if (portsUsed > device()->freePorts().count()) {
//        reportFailure(tr("Cannot debug: Not enough free ports available."));
//        return;
//    }

    if (!Internal::fixupParameters(m_runParameters, runControl(), m_errors)) {
        reportFailure(m_errors.join('\n'));
        return;
    }

    Utils::globalMacroExpander()->registerFileVariables(
                "DebuggedExecutable", tr("Debugged executable"),
                [this] { return m_runParameters.inferior.executable; }
    );

    runControl()->setDisplayName(m_runParameters.displayName);

    DebuggerEngine *cppEngine = nullptr;
    switch (m_runParameters.cppEngineType) {
        case GdbEngineType:
            cppEngine = createGdbEngine(m_runParameters.useTerminal, m_runParameters.startMode);
            break;
        case CdbEngineType:
            cppEngine = createCdbEngine(&m_errors, m_runParameters.startMode);
            break;
        case LldbEngineType:
            cppEngine = createLldbEngine();
            break;
        case PdbEngineType: // FIXME: Yes, Python counts as C++...
            cppEngine = createPdbEngine();
            break;
        default:
            QTC_CHECK(false);
            break;
    }

    switch (m_runParameters.masterEngineType) {
        case QmlEngineType:
            m_engine = createQmlEngine(m_runParameters.useTerminal);
            break;
        case QmlCppEngineType:
            if (cppEngine)
                m_engine = createQmlCppEngine(cppEngine, m_runParameters.useTerminal);
            break;
        default:
            m_engine = cppEngine;
            break;
    }

    if (!m_engine) {
        m_errors.append(DebuggerPlugin::tr("Unable to create a debugging engine of the type \"%1\"").
                       arg(engineTypeName(m_runParameters.masterEngineType)));
        reportFailure(m_errors.join('\n'));
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

void DebuggerRunTool::startFailed()
{
    appendMessage(tr("Debugging has failed"), NormalMessageFormat);
    m_engine->handleStartFailed();
}

void DebuggerRunTool::stop()
{
    m_isDying = true;
    QTC_ASSERT(m_engine, reportStopped(); return);
    m_engine->quitDebugger();
}

void DebuggerRunTool::debuggingFinished()
{
    appendMessage(tr("Debugging has finished"), NormalMessageFormat);
    Internal::runControlFinished(this);
    reportStopped();
}

DebuggerRunParameters &DebuggerRunTool::runParameters()
{
    return m_runParameters;
}

const DebuggerRunParameters &DebuggerRunTool::runParameters() const
{
    return m_runParameters;
}

int DebuggerRunTool::portsUsedByDebugger() const
{
    return isCppDebugging() + isQmlDebugging();
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

///////////////////////////////////////////////////////////////////////
//
// DebuggerRunControlCreator
//
///////////////////////////////////////////////////////////////////////

namespace Internal {

static bool fixupParameters(DebuggerRunParameters &rp, RunControl *runControl, QStringList &m_errors)
{
    RunConfiguration *runConfig = runControl->runConfiguration();
    if (!runConfig)
        return false;

    const Kit *kit = runConfig->target()->kit();
    QTC_ASSERT(kit, return false);

    // Extract as much as possible from available RunConfiguration.
    const Runnable runnable = runConfig->runnable();
    if (rp.needFixup && runnable.is<StandardRunnable>()) {
        // FIXME: Needed for core dump which stores the executable in inferior, but not in runConfig
        // executable.
        const QString prevExecutable = rp.inferior.executable;
        rp.inferior = runnable.as<StandardRunnable>();
        if (rp.inferior.executable.isEmpty())
            rp.inferior.executable = prevExecutable;
        rp.useTerminal = rp.inferior.runMode == ApplicationLauncher::Console;
        // Normalize to work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch'...)
        rp.inferior.workingDirectory = FileUtils::normalizePathName(rp.inferior.workingDirectory);
    }

    // We might get an executable from a local PID.
    if (rp.needFixup && rp.inferior.executable.isEmpty() && rp.attachPID.isValid()) {
        foreach (const DeviceProcessItem &p, DeviceProcessList::localProcesses()) {
            if (p.pid == rp.attachPID.pid()) {
                rp.inferior.executable = p.exe;
                break;
            }
        }
    }

    rp.macroExpander = kit->macroExpander();
    if (rp.symbolFile.isEmpty())
        rp.symbolFile = rp.inferior.executable;

    rp.debugger = DebuggerKitInformation::runnable(kit);
    const QByteArray envBinary = qgetenv("QTC_DEBUGGER_PATH");
    if (!envBinary.isEmpty())
        rp.debugger.executable = QString::fromLocal8Bit(envBinary);

    if (rp.needFixup) {
        if (auto envAspect = runConfig->extraAspect<EnvironmentAspect>()) {
            rp.inferior.environment = envAspect->environment(); // Correct.
            rp.stubEnvironment = rp.inferior.environment; // FIXME: Wrong, but contains DYLD_IMAGE_SUFFIX

            // Copy over DYLD_IMAGE_SUFFIX etc
            for (auto var : QStringList({"DYLD_IMAGE_SUFFIX", "DYLD_LIBRARY_PATH", "DYLD_FRAMEWORK_PATH"}))
                if (rp.inferior.environment.hasKey(var))
                    rp.debugger.environment.set(var, rp.inferior.environment.value(var));
        }
    }
    if (Project *project = runConfig->target()->project()) {
        rp.projectSourceDirectory = project->projectDirectory().toString();
        rp.projectSourceFiles = project->files(Project::SourceFiles);
    }

    rp.toolChainAbi = ToolChainKitInformation::targetAbi(kit);

    bool ok = false;
    int nativeMixedOverride = qgetenv("QTC_DEBUGGER_NATIVE_MIXED").toInt(&ok);
    if (ok)
        rp.nativeMixedEnabled = bool(nativeMixedOverride);

    rp.cppEngineType = DebuggerKitInformation::engineType(kit);
    if (rp.sysRoot.isEmpty())
        rp.sysRoot = SysRootKitInformation::sysRoot(kit).toString();

    if (rp.displayName.isEmpty())
        rp.displayName = runConfig->displayName();

    if (runConfig->property("supportsDebugger").toBool()) {
        QString mainScript = runConfig->property("mainScript").toString();
        QString interpreter = runConfig->property("interpreter").toString();
        if (!interpreter.isEmpty() && mainScript.endsWith(".py")) {
            rp.mainScript = mainScript;
            rp.interpreter = interpreter;
            QString args = runConfig->property("arguments").toString();
            if (!args.isEmpty()) {
                if (!rp.inferior.commandLineArguments.isEmpty())
                    rp.inferior.commandLineArguments.append(QLatin1Char(' '));
                rp.inferior.commandLineArguments.append(args);
            }
            rp.masterEngineType = PdbEngineType;
        }
    }

    if (auto debuggerAspect = runConfig->extraAspect<DebuggerRunConfigurationAspect>()) {
        rp.multiProcess = debuggerAspect->useMultiProcess();
        if (rp.languages == NoLanguage) {
            if (debuggerAspect->useCppDebugger())
                rp.languages |= CppLanguage;
            if (debuggerAspect->useQmlDebugger())
                rp.languages |= QmlLanguage;
        }
    }

    // This can happen e.g. when started from the command line.
    if (rp.languages == NoLanguage)
        rp.languages = CppLanguage;

    // validate debugger if C++ debugging is enabled
    if (rp.languages & CppLanguage) {
        const QList<Task> tasks = DebuggerKitInformation::validateDebugger(kit);
        if (!tasks.isEmpty()) {
            foreach (const Task &t, tasks) {
                if (t.type == Task::Warning)
                    continue;
                m_errors.append(t.description);
            }
            if (!m_errors.isEmpty())
                return false;
        }
    }

    IDevice::ConstPtr device = runControl->device();
    if (rp.languages & QmlLanguage) {
        if (device && device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
            if (rp.qmlServer.host().isEmpty() || rp.qmlServer.port() <= 0) {
                QTcpServer server;
                const bool canListen = server.listen(QHostAddress::LocalHost)
                        || server.listen(QHostAddress::LocalHostIPv6);
                if (!canListen) {
                    m_errors.append(DebuggerPlugin::tr("Not enough free ports for QML debugging.") + ' ');
                    return false;
                }
                rp.qmlServer.setHost(server.serverAddress().toString());
                rp.qmlServer.setPort(server.serverPort());
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

    if (rp.masterEngineType == NoEngineType) {
        if (rp.languages & QmlLanguage) {
            QmlDebug::QmlDebugServicesPreset service;
            if (rp.languages & CppLanguage) {
                if (rp.nativeMixedEnabled) {
                    service = QmlDebug::QmlNativeDebuggerServices;
                } else {
                    rp.masterEngineType = QmlCppEngineType;
                    service = QmlDebug::QmlDebuggerServices;
                }
            } else {
                rp.masterEngineType = QmlEngineType;
                service = QmlDebug::QmlDebuggerServices;
            }
            if (rp.startMode != AttachExternal && rp.startMode != AttachCrashedExternal) {
                QString qmlarg = (rp.languages & CppLanguage) && rp.nativeMixedEnabled
                        ? QmlDebug::qmlDebugNativeArguments(service, false)
                        : QmlDebug::qmlDebugTcpArguments(service, Port(rp.qmlServer.port()));
                QtcProcess::addArg(&rp.inferior.commandLineArguments, qmlarg);
            }
        }
    }

    if (rp.masterEngineType == NoEngineType)
        rp.masterEngineType = rp.cppEngineType;

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
            m_errors.append(DebuggerPlugin::tr("Debugging complex command lines "
                                               "is currently not supported on Windows."));
            return false;
        }
    }

    // FIXME: We can't handle terminals yet.
    if (rp.useTerminal && rp.cppEngineType == LldbEngineType) {
        qWarning("Run in Terminal is not supported yet with the LLDB backend");
        m_errors.append(DebuggerPlugin::tr("Run in Terminal is not supported with the LLDB backend."));
        rp.useTerminal = false;
    }

    return true;
}

} // Internal

static DebuggerRunConfigurationAspect *debuggerAspect(const RunControl *runControl)
{
    return runControl->runConfiguration()->extraAspect<DebuggerRunConfigurationAspect>();
}

static bool cppDebugging(const RunControl *runControl)
{
    auto aspect = debuggerAspect(runControl);
    return aspect ? aspect->useCppDebugger() : true; // For cases like valgrind-with-gdb.
}

static bool qmlDebugging(const RunControl *runControl)
{
    auto aspect = debuggerAspect(runControl);
    return aspect ? aspect->useQmlDebugger() : false; // For cases like valgrind-with-gdb.
}

/// DebuggerRunTool

DebuggerRunTool::DebuggerRunTool(RunControl *runControl)
    : RunWorker(runControl),
      m_isCppDebugging(cppDebugging(runControl)),
      m_isQmlDebugging(qmlDebugging(runControl))
{
    setDisplayName("DebuggerRunTool");
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

    Runnable r = runnable();
    if (r.is<StandardRunnable>())
        m_runParameters.inferior = r.as<StandardRunnable>();
}

void DebuggerRunTool::setRunParameters(const DebuggerRunParameters &rp)
{
    m_runParameters = rp;

    if (!rp.serverStartScript.isEmpty()) {
        // Provide script information about the environment
        StandardRunnable serverStarter;
        serverStarter.executable = rp.serverStartScript;
        QtcProcess::addArg(&serverStarter.commandLineArguments, rp.inferior.executable);
        QtcProcess::addArg(&serverStarter.commandLineArguments, rp.remoteChannel);
        addStartDependency(new LocalProcessRunner(runControl(), serverStarter));
    }
}

DebuggerEngine *DebuggerRunTool::activeEngine() const
{
    return m_engine ? m_engine->activeEngine() : nullptr;
}

class DummyProject : public Project
{
public:
    DummyProject() : Project(QString(""), FileName::fromString("")) {}
};

RunConfiguration *dummyRunConfigForKit(ProjectExplorer::Kit *kit)
{
    QTC_ASSERT(kit, return nullptr); // Caller needs to look for a suitable kit.
    Project *project = SessionManager::startupProject();
    Target *target = project ? project->target(kit) : nullptr;
    if (!target || !target->activeRunConfiguration()) {
        project = new DummyProject; // FIXME: Leaks.
        target = project->createTarget(kit);
    }
    QTC_ASSERT(target, return nullptr);
    auto runConfig = target->activeRunConfiguration();
    return runConfig;
}

DebuggerRunTool *DebuggerRunTool::createFromKit(Kit *kit)
{
    RunConfiguration *runConfig = dummyRunConfigForKit(kit);
    return createFromRunConfiguration(runConfig);
}

void DebuggerRunTool::startRunControl()
{
    ProjectExplorerPlugin::startRunControl(runControl());
}

DebuggerRunTool *DebuggerRunTool::createFromRunConfiguration(RunConfiguration *runConfig)
{
    QTC_ASSERT(runConfig, return nullptr);
    auto runControl = new RunControl(runConfig, ProjectExplorer::Constants::DEBUG_RUN_MODE);
    auto debugger = new DebuggerRunTool(runControl);
    return debugger;
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
}

GdbServerPortsGatherer::~GdbServerPortsGatherer()
{
}

QString GdbServerPortsGatherer::gdbServerChannel() const
{
    const QString host = device()->sshParameters().host;
    return QString("%1:%2").arg(host).arg(m_gdbServerPort.number());
}

QUrl GdbServerPortsGatherer::qmlServer() const
{
    QUrl server = device()->toolControlChannel(IDevice::QmlControlChannel);
    server.setPort(m_qmlServerPort.number());
    return server;
}

void GdbServerPortsGatherer::start()
{
    appendMessage(tr("Checking available ports..."), NormalMessageFormat);
    m_portsGatherer.start(device());
}

void GdbServerPortsGatherer::handlePortListReady()
{
    Utils::PortList portList = device()->freePorts();
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
    reportDone();
}

// GdbServerRunner

GdbServerRunner::GdbServerRunner(RunControl *runControl, GdbServerPortsGatherer *portsGatherer)
   : SimpleTargetRunner(runControl), m_portsGatherer(portsGatherer)
{
    setDisplayName("GdbServerRunner");
}

GdbServerRunner::~GdbServerRunner()
{
}

void GdbServerRunner::start()
{
    QTC_ASSERT(m_portsGatherer, reportFailure(); return);

    StandardRunnable r = runnable().as<StandardRunnable>();
    QStringList args = QtcProcess::splitArgs(r.commandLineArguments, OsTypeLinux);
    QString command;

    const bool isQmlDebugging = m_portsGatherer->useQmlServer();
    const bool isCppDebugging = m_portsGatherer->useGdbServer();

    if (isQmlDebugging) {
        args.prepend(QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlDebuggerServices,
                                                    m_portsGatherer->qmlServerPort()));
    }

    if (isQmlDebugging && !isCppDebugging) {
        command = r.executable;
    } else {
        command = device()->debugServerPath();
        if (command.isEmpty())
            command = "gdbserver";
        args.clear();
        args.append(QString("--multi"));
        args.append(QString(":%1").arg(m_portsGatherer->gdbServerPort().number()));
    }
    r.executable = command;
    r.commandLineArguments = QtcProcess::joinArgs(args, OsTypeLinux);

    setRunnable(r);

    appendMessage(tr("Starting gdbserver..."), NormalMessageFormat);

    SimpleTargetRunner::start();
}

} // namespace Debugger
