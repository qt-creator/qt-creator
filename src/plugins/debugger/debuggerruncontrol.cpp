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
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerkitinformation.h"
#include "debuggerplugin.h"
#include "debuggerrunconfigurationaspect.h"
#include "debuggerstartparameters.h"
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
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/checkablemessagebox.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

#include <qtsupport/qtkitinformation.h>

#include <QTcpServer>

using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace Utils;

enum { debug = 0 };

namespace Debugger {
namespace Internal {

DebuggerEngine *createCdbEngine(const DebuggerRunParameters &rp, QStringList *error);
const auto *DebugRunMode = ProjectExplorer::Constants::DEBUG_RUN_MODE;
const auto *DebugRunModeWithBreakOnMain = ProjectExplorer::Constants::DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN;

DebuggerEngine *createGdbEngine(const DebuggerRunParameters &rp);
DebuggerEngine *createPdbEngine(const DebuggerRunParameters &rp);
DebuggerEngine *createQmlEngine(const DebuggerRunParameters &rp);
DebuggerEngine *createQmlCppEngine(const DebuggerRunParameters &rp, QStringList *error);
DebuggerEngine *createLldbEngine(const DebuggerRunParameters &rp);

} // namespace Internal


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

DebuggerRunControl *createHelper(RunConfiguration *runConfig, Internal::DebuggerEngine *engine)
{
    return new DebuggerRunControl(runConfig, engine);
}

DebuggerRunControl::DebuggerRunControl(RunConfiguration *runConfig, DebuggerEngine *engine)
    : RunControl(runConfig, DebugRunMode),
      m_engine(engine),
      m_running(false)
{
    setIcon(ProjectExplorer::Icons::DEBUG_START_SMALL_TOOLBAR);
    connect(this, &RunControl::finished, this, &DebuggerRunControl::handleFinished);

    connect(engine, &DebuggerEngine::requestRemoteSetup,
            this, &DebuggerRunControl::requestRemoteSetup);
    connect(engine, &DebuggerEngine::stateChanged,
            this, &DebuggerRunControl::stateChanged);
    connect(engine, &DebuggerEngine::aboutToNotifyInferiorSetupOk,
            this, &DebuggerRunControl::aboutToNotifyInferiorSetupOk);
}

DebuggerRunControl::~DebuggerRunControl()
{
    disconnect();
    if (m_outputProcessor) {
        delete m_outputProcessor;
        m_outputProcessor = 0;
    }
    if (m_engine) {
        DebuggerEngine *engine = m_engine;
        m_engine = 0;
        engine->disconnect();
        delete engine;
    }
}

QString DebuggerRunControl::displayName() const
{
    QTC_ASSERT(m_engine, return QString());
    return m_engine->runParameters().displayName;
}

bool DebuggerRunControl::supportsReRunning() const
{
    // QML and/or mixed are not prepared for it.
    return m_engine && !(m_engine->runParameters().languages & QmlLanguage);
}

static OutputFormat outputFormatForChannelType(int channel)
{
    switch (channel) {
    case AppOutput: return StdOutFormatSameLine;
    case AppError: return StdErrFormatSameLine;
    case AppStuff: return DebugFormat;
    default: return NumberOfFormats;
    }
}

void DebuggerRunControl::handleApplicationOutput(const QString &msg, int channel)
{
    OutputFormat format = outputFormatForChannelType(channel);
    QTC_ASSERT(format != NumberOfFormats, return);
    if (m_outputProcessor) {
        if (m_outputProcessor->logToAppOutputPane)
            appendMessage(msg, format);
        if (m_outputProcessor->process) {
            m_outputProcessor->process(msg, channel == AppError ? OutputProcessor::StandardError
                                                                : OutputProcessor::StandardOut);
        }
    } else {
        appendMessage(msg, format);
    }
}

void DebuggerRunControl::start()
{
    Debugger::Internal::saveModeToRestore();
    Debugger::selectPerspective(Debugger::Constants::CppPerspectiveId);
    TaskHub::clearTasks(Debugger::Constants::TASK_CATEGORY_DEBUGGER_DEBUGINFO);
    TaskHub::clearTasks(Debugger::Constants::TASK_CATEGORY_DEBUGGER_RUNTIME);

    QTC_ASSERT(m_engine, return);
    // User canceled input dialog asking for executable when working on library project.
    if (m_engine->runParameters().startMode == StartInternal
            && m_engine->runParameters().inferior.executable.isEmpty()
            && m_engine->runParameters().interpreter.isEmpty()) {
        appendMessage(tr("No executable specified.") + QLatin1Char('\n'), ErrorMessageFormat);
        emit started();
        emit finished();
        return;
    }

    if (m_engine->runParameters().startMode == StartInternal) {
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

    Internal::runControlStarted(m_engine);

    // We might get a synchronous startFailed() notification on Windows,
    // when launching the process fails. Emit a proper finished() sequence.
    emit started();
    m_running = true;

    m_engine->startDebugger(this);

    if (m_running)
        appendMessage(tr("Debugging starts") + QLatin1Char('\n'), NormalMessageFormat);
}

void DebuggerRunControl::startFailed()
{
    appendMessage(tr("Debugging has failed") + QLatin1Char('\n'), NormalMessageFormat);
    m_running = false;
    emit finished();
    m_engine->handleStartFailed();
}

void DebuggerRunControl::notifyEngineRemoteServerRunning(const QByteArray &msg, int pid)
{
    m_engine->notifyEngineRemoteServerRunning(QString::fromUtf8(msg), pid);
}

void DebuggerRunControl::notifyEngineRemoteSetupFinished(const RemoteSetupResult &result)
{
    m_engine->notifyEngineRemoteSetupFinished(result);
}

void DebuggerRunControl::handleFinished()
{
    appendMessage(tr("Debugging has finished") + QLatin1Char('\n'), NormalMessageFormat);
    if (m_engine)
        m_engine->handleFinished();
    Internal::runControlFinished(m_engine);
}

bool DebuggerRunControl::promptToStop(bool *optionalPrompt) const
{
    QTC_ASSERT(isRunning(), return true);

    if (optionalPrompt && !*optionalPrompt)
        return true;

    const QString question = tr("A debugging session is still in progress. "
            "Terminating the session in the current"
            " state can leave the target in an inconsistent state."
            " Would you still like to terminate it?");
    bool result = showPromptToStopDialog(tr("Close Debugging Session"), question,
                                         QString(), QString(), optionalPrompt);
    if (result)
        disconnect(this);
    return result;
}

RunControl::StopResult DebuggerRunControl::stop()
{
    QTC_ASSERT(m_engine, return StoppedSynchronously);
    m_engine->quitDebugger();
    return AsynchronousStop;
}

void DebuggerRunControl::debuggingFinished()
{
    m_running = false;
    emit finished();
}

void DebuggerRunControl::showMessage(const QString &msg, int channel)
{
    m_engine->showMessage(msg, channel);
}

bool DebuggerRunControl::isRunning() const
{
    return m_running;
}

DebuggerStartParameters &DebuggerRunControl::startParameters()
{
    return m_engine->runParameters();
}

void DebuggerRunControl::setOutputProcessor(OutputProcessor *processor)
{
    delete m_outputProcessor;
    m_outputProcessor = processor;
}

void DebuggerRunControl::notifyInferiorIll()
{
    m_engine->notifyInferiorIll();
}

void DebuggerRunControl::notifyInferiorExited()
{
    m_engine->notifyInferiorExited();
}

void DebuggerRunControl::quitDebugger()
{
    m_engine->quitDebugger();
}

void DebuggerRunControl::abortDebugger()
{
    m_engine->abortDebugger();
}

///////////////////////////////////////////////////////////////////////
//
// DebuggerRunControlCreator
//
///////////////////////////////////////////////////////////////////////


namespace Internal {

// Re-used for Combined C++/QML engine.
DebuggerEngine *createEngine(DebuggerEngineType et, const DebuggerRunParameters &rp, QStringList *errors)
{
    switch (et) {
    case GdbEngineType:
        return createGdbEngine(rp);
    case CdbEngineType:
        return createCdbEngine(rp, errors);
    case PdbEngineType:
        return createPdbEngine(rp);
    case QmlEngineType:
        return createQmlEngine(rp);
    case LldbEngineType:
        return createLldbEngine(rp);
    case QmlCppEngineType:
        return createQmlCppEngine(rp, errors);
    default:
        if (errors)
            errors->append(DebuggerPlugin::tr("Unknown debugger type \"%1\"")
                           .arg(engineTypeName(et)));
    }
    return 0;
}

static DebuggerRunControl *doCreate(DebuggerRunParameters rp, RunConfiguration *runConfig,
                                    const Kit *kit, Core::Id runMode, QStringList *errors)
{
    QTC_ASSERT(kit, return 0);

    // Extract as much as possible from available RunConfiguration.
    if (runConfig && runConfig->runnable().is<StandardRunnable>()) {
        rp.inferior = runConfig->runnable().as<StandardRunnable>();
        rp.useTerminal = rp.inferior.runMode == ApplicationLauncher::Console;
        // Normalize to work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch'...)
        rp.inferior.workingDirectory = FileUtils::normalizePathName(rp.inferior.workingDirectory);
    }

    // We might get an executable from a local PID.
    if (rp.inferior.executable.isEmpty() && rp.attachPID != InvalidPid) {
        foreach (const DeviceProcessItem &p, DeviceProcessList::localProcesses())
            if (p.pid == rp.attachPID)
                rp.inferior.executable = p.exe;
    }

    rp.macroExpander = kit->macroExpander();
    if (rp.symbolFile.isEmpty())
        rp.symbolFile = rp.inferior.executable;

    rp.debugger = DebuggerKitInformation::runnable(kit);
    const QByteArray envBinary = qgetenv("QTC_DEBUGGER_PATH");
    if (!envBinary.isEmpty())
        rp.debugger.executable = QString::fromLocal8Bit(envBinary);

    if (runConfig) {
        if (auto envAspect = runConfig->extraAspect<EnvironmentAspect>()) {
            rp.inferior.environment = envAspect->environment(); // Correct.
            rp.stubEnvironment = rp.inferior.environment; // FIXME: Wrong, but contains DYLD_IMAGE_SUFFIX

            // Copy over DYLD_IMAGE_SUFFIX etc
            for (auto var : QStringList({"DYLD_IMAGE_SUFFIX", "DYLD_LIBRARY_PATH", "DYLD_FRAMEWORK_PATH"}))
                if (rp.inferior.environment.hasKey(var))
                    rp.debugger.environment.set(var, rp.inferior.environment.value(var));
        }
        if (Project *project = runConfig->target()->project()) {
            rp.projectSourceDirectory = project->projectDirectory().toString();
            rp.projectSourceFiles = project->files(Project::SourceFiles);
        }
    } else {
        // "special" starts like Start and Debug External Application.
        rp.inferior.environment = Environment::systemEnvironment();
        rp.inferior.environment.modify(EnvironmentKitInformation::environmentChanges(kit));
    }

    rp.toolChainAbi = ToolChainKitInformation::targetAbi(kit);

    if (false) {
        const QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
        rp.nativeMixedEnabled = version && version->qtVersion() >= QtSupport::QtVersionNumber(5, 7, 0);
    }

    bool ok = false;
    int nativeMixedOverride = qgetenv("QTC_DEBUGGER_NATIVE_MIXED").toInt(&ok);
    if (ok)
        rp.nativeMixedEnabled = bool(nativeMixedOverride);

    rp.cppEngineType = DebuggerKitInformation::engineType(kit);
    if (rp.sysRoot.isEmpty())
        rp.sysRoot = SysRootKitInformation::sysRoot(kit).toString();
    rp.device = DeviceKitInformation::device(kit);

    if (rp.displayName.isEmpty() && runConfig)
        rp.displayName = runConfig->displayName();

    if (runConfig && runConfig->property("supportsDebugger").toBool()) {
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

    DebuggerRunConfigurationAspect *debuggerAspect = 0;
    if (runConfig)
        debuggerAspect = runConfig->extraAspect<DebuggerRunConfigurationAspect>();

    if (debuggerAspect)
        rp.multiProcess = debuggerAspect->useMultiProcess();

    if (debuggerAspect) {
        rp.languages = NoLanguage;
        if (debuggerAspect->useCppDebugger())
            rp.languages |= CppLanguage;
        if (debuggerAspect->useQmlDebugger())
            rp.languages |= QmlLanguage;
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
                errors->append(t.description);
            }
            if (!errors->isEmpty())
                return 0;
        }
    }

    if (rp.languages & QmlLanguage) {
        if (rp.device && rp.device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
            if (rp.qmlServer.host.isEmpty() || !rp.qmlServer.port.isValid()) {
                QTcpServer server;
                const bool canListen = server.listen(QHostAddress::LocalHost)
                        || server.listen(QHostAddress::LocalHostIPv6);
                if (!canListen) {
                    errors->append(DebuggerPlugin::tr("Not enough free ports for QML debugging.") + ' ');
                    return 0;
                }
                TcpServerConnection conn;
                conn.host = server.serverAddress().toString();
                conn.port = Utils::Port(server.serverPort());
                rp.qmlServer = conn;
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
                QtcProcess::addArg(&rp.inferior.commandLineArguments,
                                   (rp.languages & CppLanguage) && rp.nativeMixedEnabled ?
                                       QmlDebug::qmlDebugNativeArguments(service, false) :
                                       QmlDebug::qmlDebugTcpArguments(service, rp.qmlServer.port));
            }
        }
    }

    if (rp.masterEngineType == NoEngineType)
        rp.masterEngineType = rp.cppEngineType;

    if (rp.device && rp.connParams.port == 0)
        rp.connParams = rp.device->sshParameters();

    // Could have been set from command line.
    if (rp.remoteChannel.isEmpty())
        rp.remoteChannel = rp.connParams.host + ':' + QString::number(rp.connParams.port);

    if (rp.startMode == NoStartMode)
        rp.startMode = StartInternal;


    if (runMode == DebugRunModeWithBreakOnMain)
        rp.breakOnMain = true;

    DebuggerEngine *engine = createEngine(rp.masterEngineType, rp, errors);
    if (!engine) {
        errors->append(DebuggerPlugin::tr("Unable to create a debugger engine of the type \"%1\"").
                        arg(engineTypeName(rp.masterEngineType)));
        rp.startMode = NoStartMode;
        return 0;
    }

    DebuggerRunControl *runControl = createHelper(runConfig, engine);

    if (!runControl)
        rp.startMode = NoStartMode;

    return runControl;
}

////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControlFactory
//
////////////////////////////////////////////////////////////////////////

static bool isDebuggableScript(RunConfiguration *runConfig)
{
    QString mainScript = runConfig->property("mainScript").toString();
    return mainScript.endsWith(".py"); // Only Python for now.
}

class DebuggerRunControlFactory : public IRunControlFactory
{
public:
    explicit DebuggerRunControlFactory(QObject *parent)
        : IRunControlFactory(parent)
    {}

    RunControl *create(RunConfiguration *runConfig,
                       Core::Id mode, QString *errorMessage) override
    {
        QTC_ASSERT(runConfig, return 0);
        QTC_ASSERT(mode == DebugRunMode || mode == DebugRunModeWithBreakOnMain, return 0);

        // We cover only local setup here. Remote setups are handled by the
        // RunControl factories in the target specific plugins.
        QStringList errors;
        DebuggerRunControl *runControl = doCreate(DebuggerRunParameters(), runConfig,
                                                  runConfig->target()->kit(), mode, &errors);
        if (errorMessage)
            *errorMessage = errors.join('\n');
        return runControl;
    }

    bool canRun(RunConfiguration *runConfig, Core::Id mode) const override
    {
        if (!(mode == DebugRunMode || mode == DebugRunModeWithBreakOnMain))
            return false;

        Runnable runnable = runConfig->runnable();
        if (runnable.is<StandardRunnable>()) {
            IDevice::ConstPtr device = runnable.as<StandardRunnable>().device;
            if (device && device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
                return true;
        }

        return DeviceTypeKitInformation::deviceTypeId(runConfig->target()->kit())
                    == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE
                 || isDebuggableScript(runConfig);
    }

    IRunConfigurationAspect *createRunConfigurationAspect(RunConfiguration *rc) override
    {
        return new DebuggerRunConfigurationAspect(rc);
    }
};

QObject *createDebuggerRunControlFactory(QObject *parent)
{
    return new DebuggerRunControlFactory(parent);
}

////////////////////////////////////////////////////////////////////////
//
// Externally visible helper.
//
////////////////////////////////////////////////////////////////////////

/**
 * Used for direct "special" starts from actions in the debugger plugin.
 */
DebuggerRunControl *createAndScheduleRun(const DebuggerRunParameters &rp, const Kit *kit)
{
    QTC_ASSERT(kit, return 0); // Caller needs to look for a suitable kit.
    QStringList errors;
    DebuggerRunControl *runControl = doCreate(rp, 0, kit, DebugRunMode, &errors);
    if (!runControl) {
        ProjectExplorerPlugin::showRunErrorMessage(errors.join('\n'));
        return 0;
    }
    Internal::showMessage(rp.startMessage, 0);
    ProjectExplorerPlugin::startRunControl(runControl, DebugRunMode);
    return runControl; // Only used for tests.
}

} // namespace Internal

/**
 * Main entry point for target plugins.
 */
DebuggerRunControl *createDebuggerRunControl(const DebuggerStartParameters &sp,
                                             RunConfiguration *runConfig,
                                             QString *errorMessage,
                                             Core::Id runMode)
{
    QTC_ASSERT(runConfig, return 0);
    QStringList errors;
    DebuggerRunControl *runControl = doCreate(sp, runConfig, runConfig->target()->kit(), runMode, &errors);
    QString msg = errors.join('\n');
    if (errorMessage)
        *errorMessage = msg;
    if (!runControl) {
        Core::ICore::showWarningWithOptions(DebuggerRunControl::tr("Debugger"), msg);
        return 0;
    }
    return runControl;
}

} // namespace Debugger
