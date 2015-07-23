/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggerruncontrol.h"

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
#include <projectexplorer/localapplicationrunconfiguration.h> // For LocalApplication*
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/checkablemessagebox.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <coreplugin/icore.h>

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
    setIcon(QLatin1String(ProjectExplorer::Constants::ICON_DEBUG_SMALL));
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

void DebuggerRunControl::start()
{
    TaskHub::clearTasks(Debugger::Constants::TASK_CATEGORY_DEBUGGER_DEBUGINFO);
    TaskHub::clearTasks(Debugger::Constants::TASK_CATEGORY_DEBUGGER_RUNTIME);

    QTC_ASSERT(m_engine, return);
    // User canceled input dialog asking for executable when working on library project.
    if (m_engine->runParameters().startMode == StartInternal
        && m_engine->runParameters().executable.isEmpty()) {
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
    return showPromptToStopDialog(tr("Close Debugging Session"), question,
                                  QString(), QString(), optionalPrompt);
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

    // Scratch data.
    const Kit *m_kit = 0;
    const RunConfiguration *m_runConfig = 0;
    DebuggerRunConfigurationAspect *m_debuggerAspect = 0;
    Target *m_target = 0;
    Project *m_project = 0;

    QStringList m_errors;
    DebuggerRunControl *m_runControl = 0;
};

void DebuggerRunControlCreator::initialize(const DebuggerStartParameters &sp)
{
    m_rp.DebuggerStartParameters::operator=(sp);
}

void DebuggerRunControlCreator::enrich(const RunConfiguration *runConfig, const Kit *kit)
{
    QTC_ASSERT(!m_kit, return);
    QTC_ASSERT(!m_runConfig, return);

    // Find RunConfiguration.
    if (!m_runConfig)
        m_runConfig = runConfig;

    // Extract as much as possible from available RunConfiguration.
    if (auto localRc = qobject_cast<const LocalApplicationRunConfiguration *>(m_runConfig)) {
        m_rp.executable = localRc->executable();
        m_rp.processArgs = localRc->commandLineArguments();
        m_rp.useTerminal = localRc->runMode() == ApplicationLauncher::Console;
        // Normalize to work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch'...)
        m_rp.workingDirectory = FileUtils::normalizePathName(localRc->workingDirectory());
    }

    // Find a Kit and Target. Either could be missing.
    if (m_runConfig)
        m_target = m_runConfig->target();

    if (!m_kit)
        m_kit = kit;

    if (!m_kit) {
        if (m_target)
            m_kit = m_target->kit();
    }

    // We might get an executable from a local PID.
    if (m_rp.executable.isEmpty() && m_rp.attachPID != InvalidPid) {
        foreach (const DeviceProcessItem &p, DeviceProcessList::localProcesses())
            if (p.pid == m_rp.attachPID)
                m_rp.executable = p.exe;
    }

    if (!m_kit) {
        // This code can only be reached when starting via the command line
        // (-debug pid or executable) or attaching from runconfiguration
        // without specifying a kit. Try to find a kit via ABI.
        QList<Abi> abis;
        if (m_rp.toolChainAbi.isValid()) {
            abis.push_back(m_rp.toolChainAbi);
        } else if (!m_rp.executable.isEmpty()) {
            abis = Abi::abisOfBinary(FileName::fromString(m_rp.executable));
        }

        if (!abis.isEmpty()) {
            // Try exact abis.
            m_kit = KitManager::find(std::function<bool (const Kit *)>([abis](const Kit *k) -> bool {
                if (const ToolChain *tc = ToolChainKitInformation::toolChain(k))
                    return abis.contains(tc->targetAbi()) && DebuggerKitInformation::isValidDebugger(k);
                return false;
            }));
            if (!m_kit) {
                // Or something compatible.
                m_kit = KitManager::find(std::function<bool (const Kit *)>([abis](const Kit *k) -> bool {
                    if (const ToolChain *tc = ToolChainKitInformation::toolChain(k))
                        foreach (const Abi &a, abis)
                            if (a.isCompatibleWith(tc->targetAbi()) && DebuggerKitInformation::isValidDebugger(k))
                                return true;
                    return false;
                }));
            }
        }
    }

    if (!m_kit)
        m_kit = KitManager::defaultKit();

    // We really should have a kit now.
    if (!m_kit) {
        m_errors.append(DebuggerKitInformation::tr("No kit found."));
        return;
    }

    if (m_runConfig) {
        if (auto envAspect = m_runConfig->extraAspect<EnvironmentAspect>())
            m_rp.environment = envAspect->environment();
    }

    if (ToolChain *tc = ToolChainKitInformation::toolChain(m_kit))
        m_rp.toolChainAbi = tc->targetAbi();

    if (m_target)
        m_project = m_target->project();

    if (m_project && m_rp.projectSourceDirectory.isEmpty())
        m_rp.projectSourceDirectory = m_project->projectDirectory().toString();

    if (m_project && m_rp.projectSourceFiles.isEmpty())
        m_rp.projectSourceFiles = m_project->files(Project::ExcludeGeneratedFiles);

    // validate debugger if C++ debugging is enabled
    if (m_rp.languages & CppLanguage) {
        const QList<Task> tasks = DebuggerKitInformation::validateDebugger(m_kit);
        if (!tasks.isEmpty()) {
            foreach (const Task &t, tasks)
                m_errors.append(t.description);
            return;
        }
    }

    m_rp.cppEngineType = DebuggerKitInformation::engineType(m_kit);
    m_rp.sysRoot = SysRootKitInformation::sysRoot(m_kit).toString();
    m_rp.debuggerCommand = DebuggerKitInformation::debuggerCommand(m_kit).toString();
    m_rp.device = DeviceKitInformation::device(m_kit);

    if (m_project) {
        m_rp.projectSourceDirectory = m_project->projectDirectory().toString();
        m_rp.projectSourceFiles = m_project->files(Project::ExcludeGeneratedFiles);
    }

    if (m_runConfig)
        m_debuggerAspect = m_runConfig->extraAspect<DebuggerRunConfigurationAspect>();

    if (m_debuggerAspect) {
        m_rp.multiProcess = m_debuggerAspect->useMultiProcess();

        if (m_debuggerAspect->useCppDebugger())
            m_rp.languages |= CppLanguage;

        if (m_debuggerAspect->useQmlDebugger()) {
            m_rp.languages |= QmlLanguage;
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
                if (!m_rp.environment.hasKey(optimizerKey))
                    m_rp.environment.set(optimizerKey, _("1"));

                QtcProcess::addArg(&m_rp.processArgs, QString::fromLatin1("-qmljsdebugger=port:%1,block").arg(m_rp.qmlServerPort));
            }
        }
    }

    if (m_rp.displayName.isEmpty() && m_runConfig)
        m_rp.displayName = m_runConfig->displayName();

    if (m_rp.masterEngineType == NoEngineType) {
        if (m_rp.executable.endsWith(_(".py"))
            || m_rp.executable == _("/usr/bin/python")
            || m_rp.executable == _("/usr/bin/python3")) {
                m_rp.masterEngineType = PdbEngineType;
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

    if (m_rp.masterEngineType == NoEngineType && m_debuggerAspect) {
        const bool useCppDebugger = m_debuggerAspect->useCppDebugger() && (m_rp.languages & CppLanguage);
        const bool useQmlDebugger = m_debuggerAspect->useQmlDebugger() && (m_rp.languages & QmlLanguage);

        if (useQmlDebugger) {
            if (useCppDebugger)
                m_rp.masterEngineType = QmlCppEngineType;
            else
                m_rp.masterEngineType = QmlEngineType;
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
        creator.enrich(runConfig, 0);
        creator.createRunControl(mode);
        if (errorMessage)
            *errorMessage = creator.fullError();
        return creator.m_runControl;
    }

    bool canRun(RunConfiguration *runConfig, Core::Id mode) const override
    {
        return (mode == DebugRunMode || mode == DebugRunModeWithBreakOnMain)
                && qobject_cast<LocalApplicationRunConfiguration *>(runConfig);
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
    DebuggerRunControlCreator creator;
    creator.initialize(sp);
    creator.enrich(runConfig, 0);
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
