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
#include "debuggerstringutils.h"
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


static const char *engineTypeName(DebuggerEngineType et)
{
    switch (et) {
    case Debugger::NoEngineType:
        break;
    case Debugger::GdbEngineType:
        return "Gdb engine";
    case Debugger::CdbEngineType:
        return "Cdb engine";
    case Debugger::PdbEngineType:
        return "Pdb engine";
    case Debugger::QmlEngineType:
        return "QML engine";
    case Debugger::QmlCppEngineType:
        return "QML C++ engine";
    case Debugger::LldbEngineType:
        return "LLDB command line engine";
    case Debugger::AllEngineTypes:
        break;
    }
    return "No engine";
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
    m_engine->notifyEngineRemoteServerRunning(msg, pid);
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

class DebuggerRunControlCreator
{
public:
    DebuggerRunControlCreator() {}

    // Life cycle: Initialize from StartParameters, enrich by automatically
    // detectable pieces, construct an Engine and a RunControl.
    void initialize(const DebuggerStartParameters &sp);
    void enrich(const RunConfiguration *runConfig, const Kit *kit);
    void createRunControl(Core::Id runMode = DebugRunMode);
    QString fullError() const { return m_errors.join(QLatin1Char('\n')); }

    // Result.
    DebuggerRunParameters m_rp;
    QStringList m_errors;
    DebuggerRunControl *m_runControl = 0;

    const RunConfiguration *m_runConfig = 0;
};

void DebuggerRunControlCreator::initialize(const DebuggerStartParameters &sp)
{
    m_rp.DebuggerStartParameters::operator=(sp);
}

void DebuggerRunControlCreator::enrich(const RunConfiguration *runConfig, const Kit *kit)
{
    m_runConfig = runConfig;
    QTC_ASSERT(kit, return);

    Target *target = 0;
    Project *project = 0;

    // Find a Kit and Target. Either could be missing.
    if (m_runConfig)
        target = m_runConfig->target();

    // Extract as much as possible from available RunConfiguration.
    if (m_runConfig && m_runConfig->runnable().is<StandardRunnable>()) {
        m_rp.inferior = m_runConfig->runnable().as<StandardRunnable>();
        m_rp.useTerminal = m_rp.inferior.runMode == ApplicationLauncher::Console;
        // Normalize to work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch'...)
        m_rp.inferior.workingDirectory = FileUtils::normalizePathName(m_rp.inferior.workingDirectory);
    }

    // We might get an executable from a local PID.
    if (m_rp.inferior.executable.isEmpty() && m_rp.attachPID != InvalidPid) {
        foreach (const DeviceProcessItem &p, DeviceProcessList::localProcesses())
            if (p.pid == m_rp.attachPID)
                m_rp.inferior.executable = p.exe;
    }

    if (m_rp.symbolFile.isEmpty())
        m_rp.symbolFile = m_rp.inferior.executable;

    if (m_runConfig) {
        if (auto envAspect = m_runConfig->extraAspect<EnvironmentAspect>()) {
            m_rp.inferior.environment = envAspect->environment(); // Correct.
            m_rp.stubEnvironment = m_rp.inferior.environment; // FIXME: Wrong, but contains DYLD_IMAGE_SUFFIX
            m_rp.debuggerEnvironment = m_rp.inferior.environment; // FIXME: Wrong, but contains DYLD_IMAGE_SUFFIX
        }
    }

    if (ToolChain *tc = ToolChainKitInformation::toolChain(kit))
        m_rp.toolChainAbi = tc->targetAbi();

    if (target)
        project = target->project();

    if (project && m_rp.projectSourceDirectory.isEmpty())
        m_rp.projectSourceDirectory = project->projectDirectory().toString();

    if (project && m_rp.projectSourceFiles.isEmpty())
        m_rp.projectSourceFiles = project->files(Project::SourceFiles);

    if (project && m_rp.projectSourceFiles.isEmpty())
        m_rp.projectSourceFiles = project->files(Project::SourceFiles);

    if (false && project) {
        const QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
        m_rp.nativeMixedEnabled = version && version->qtVersion() >= QtSupport::QtVersionNumber(5, 7, 0);
    }

    bool ok = false;
    int nativeMixedOverride = qgetenv("QTC_DEBUGGER_NATIVE_MIXED").toInt(&ok);
    if (ok)
        m_rp.nativeMixedEnabled = bool(nativeMixedOverride);

    m_rp.cppEngineType = DebuggerKitInformation::engineType(kit);
    m_rp.sysRoot = SysRootKitInformation::sysRoot(kit).toString();
    m_rp.debuggerCommand = DebuggerKitInformation::debuggerCommand(kit).toString();
    m_rp.device = DeviceKitInformation::device(kit);

    if (project) {
        m_rp.projectSourceDirectory = project->projectDirectory().toString();
        m_rp.projectSourceFiles = project->files(Project::SourceFiles);
    }

    if (m_rp.displayName.isEmpty() && m_runConfig)
        m_rp.displayName = m_runConfig->displayName();

    if (m_runConfig && m_runConfig->property("supportsDebugger").toBool()) {
        QString mainScript = m_runConfig->property("mainScript").toString();
        QString interpreter = m_runConfig->property("interpreter").toString();
        if (!interpreter.isEmpty() && mainScript.endsWith(_(".py"))) {
            m_rp.mainScript = mainScript;
            m_rp.interpreter = interpreter;
            QString args = m_runConfig->property("arguments").toString();
            if (!args.isEmpty()) {
                if (!m_rp.inferior.commandLineArguments.isEmpty())
                    m_rp.inferior.commandLineArguments.append(QLatin1Char(' '));
                m_rp.inferior.commandLineArguments.append(args);
            }
            m_rp.masterEngineType = PdbEngineType;
        }
    }

    DebuggerRunConfigurationAspect *debuggerAspect = 0;
    if (m_runConfig)
        debuggerAspect = m_runConfig->extraAspect<DebuggerRunConfigurationAspect>();

    if (debuggerAspect)
        m_rp.multiProcess = debuggerAspect->useMultiProcess();

    if (debuggerAspect) {
        m_rp.languages = NoLanguage;
        if (debuggerAspect->useCppDebugger())
            m_rp.languages |= CppLanguage;
        if (debuggerAspect->useQmlDebugger())
            m_rp.languages |= QmlLanguage;
    }

    // This can happen e.g. when started from the command line.
    if (m_rp.languages == NoLanguage)
        m_rp.languages = CppLanguage;

    // validate debugger if C++ debugging is enabled
    if (m_rp.languages & CppLanguage) {
        const QList<Task> tasks = DebuggerKitInformation::validateDebugger(kit);
        if (!tasks.isEmpty()) {
            foreach (const Task &t, tasks)
                m_errors.append(t.description);
            return;
        }
    }

    if (m_rp.languages & QmlLanguage) {
        if (m_rp.device && m_rp.device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE) {
            QTcpServer server;
            const bool canListen = server.listen(QHostAddress::LocalHost)
                    || server.listen(QHostAddress::LocalHostIPv6);
            if (!canListen) {
                m_errors.append(DebuggerPlugin::tr("Not enough free ports for QML debugging.") + QLatin1Char(' '));
                return;
            }
            m_rp.qmlServerAddress = server.serverAddress().toString();
            m_rp.qmlServerPort = server.serverPort();

            // Makes sure that all bindings go through the JavaScript engine, so that
            // breakpoints are actually hit!
            const QString optimizerKey = _("QML_DISABLE_OPTIMIZER");
            if (!m_rp.inferior.environment.hasKey(optimizerKey))
                m_rp.inferior.environment.set(optimizerKey, _("1"));
        }
    }

    if (!boolSetting(AutoEnrichParameters)) {
        const QString sysroot = m_rp.sysRoot;
        if (m_rp.debugInfoLocation.isEmpty())
            m_rp.debugInfoLocation = sysroot + QLatin1String("/usr/lib/debug");
        if (m_rp.debugSourceLocation.isEmpty()) {
            QString base = sysroot + QLatin1String("/usr/src/debug/");
            m_rp.debugSourceLocation.append(base + QLatin1String("qt5base/src/corelib"));
            m_rp.debugSourceLocation.append(base + QLatin1String("qt5base/src/gui"));
            m_rp.debugSourceLocation.append(base + QLatin1String("qt5base/src/network"));
        }
    }

    if (m_rp.masterEngineType == NoEngineType) {
        if (m_rp.languages & QmlLanguage) {
            QmlDebug::QmlDebugServicesPreset service;
            if (m_rp.languages & CppLanguage) {
                if (m_rp.nativeMixedEnabled) {
                    service = QmlDebug::QmlNativeDebuggerServices;
                } else {
                    m_rp.masterEngineType = QmlCppEngineType;
                    service = QmlDebug::QmlDebuggerServices;
                }
            } else {
                m_rp.masterEngineType = QmlEngineType;
                service = QmlDebug::QmlDebuggerServices;
            }
            if (m_rp.startMode != AttachExternal && m_rp.startMode != AttachCrashedExternal) {
                QtcProcess::addArg(&m_rp.inferior.commandLineArguments,
                                   (m_rp.languages & CppLanguage) && m_rp.nativeMixedEnabled ?
                                       QmlDebug::qmlDebugNativeArguments(service, false) :
                                       QmlDebug::qmlDebugTcpArguments(service, m_rp.qmlServerPort));
            }
        }
    }

    if (m_rp.masterEngineType == NoEngineType)
        m_rp.masterEngineType = m_rp.cppEngineType;

    if (m_rp.device && m_rp.connParams.port == 0)
        m_rp.connParams = m_rp.device->sshParameters();

    // Could have been set from command line.
    if (m_rp.remoteChannel.isEmpty())
        m_rp.remoteChannel = m_rp.connParams.host + QLatin1Char(':') + QString::number(m_rp.connParams.port);

    if (m_rp.startMode == NoStartMode)
        m_rp.startMode = StartInternal;
}

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
            errors->append(DebuggerPlugin::tr("Unknown debugger type \"%1\"").arg(_(engineTypeName(et))));
    }
    return 0;
}

void DebuggerRunControlCreator::createRunControl(Core::Id runMode)
{
    if (runMode == DebugRunModeWithBreakOnMain)
        m_rp.breakOnMain = true;

    DebuggerEngine *engine = createEngine(m_rp.masterEngineType, m_rp, &m_errors);
    if (!engine) {
        m_errors.append(DebuggerPlugin::tr("Unable to create a debugger engine of the type \"%1\"").
                        arg(_(engineTypeName(m_rp.masterEngineType))));
        m_rp.startMode = NoStartMode;
        return;
    }

    m_runControl = new DebuggerRunControl(const_cast<RunConfiguration *>(m_runConfig), engine);

    if (!m_runControl)
        m_rp.startMode = NoStartMode;
}

////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControlFactory
//
////////////////////////////////////////////////////////////////////////

static bool isDebuggableScript(RunConfiguration *runConfig)
{
    QString mainScript = runConfig->property("mainScript").toString();
    return mainScript.endsWith(_(".py")); // Only Python for now.
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
        DebuggerRunControlCreator creator;
        creator.enrich(runConfig, runConfig->target()->kit());
        creator.createRunControl(mode);
        if (errorMessage)
            *errorMessage = creator.fullError();
        return creator.m_runControl;
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
    DebuggerRunControlCreator creator;
    creator.m_rp = rp;
    creator.enrich(0, kit);
    creator.createRunControl(DebugRunMode);
    if (!creator.m_runControl) {
        ProjectExplorerPlugin::showRunErrorMessage(creator.fullError());
        return 0;
    }
    Internal::showMessage(rp.startMessage, 0);
    ProjectExplorerPlugin::startRunControl(creator.m_runControl, DebugRunMode);
    return creator.m_runControl; // Only used for tests.
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
    DebuggerRunControlCreator creator;
    creator.initialize(sp);
    creator.enrich(runConfig, runConfig->target()->kit());
    creator.createRunControl(runMode);
    if (errorMessage)
        *errorMessage = creator.fullError();
    if (!creator.m_runControl) {
        Core::ICore::showWarningWithOptions(DebuggerRunControl::tr("Debugger"), creator.fullError());
        return 0;
    }
    return creator.m_runControl;
}

} // namespace Debugger
