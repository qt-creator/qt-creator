/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "debuggerrunner.h"
#include "debuggerruncontrolfactory.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggermainwindow.h"
#include "debuggerplugin.h"
#include "debuggerstringutils.h"
#include "debuggerstartparameters.h"
#include "lldb/lldbenginehost.h"
#include "debuggertooltipmanager.h"

#ifdef Q_OS_WIN
#  include "peutils.h"
#endif

#include <projectexplorer/abi.h>
#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/outputformat.h>
#include <projectexplorer/applicationrunconfiguration.h> // For LocalApplication*

#include <utils/synchronousprocess.h>
#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>
#include <utils/qtcprocess.h>
#include <coreplugin/icore.h>

#include <QtCore/QDir>
#include <QtGui/QMessageBox>

using namespace ProjectExplorer;
using namespace Debugger::Internal;

namespace Debugger {
namespace Internal {

bool isCdbEngineEnabled(); // Check the configuration page
ConfigurationCheck checkCdbConfiguration(const ProjectExplorer::Abi &);

DebuggerEngine *createCdbEngine(const DebuggerStartParameters &,
    DebuggerEngine *masterEngine, QString *error);
DebuggerEngine *createGdbEngine(const DebuggerStartParameters &,
    DebuggerEngine *masterEngine);
DebuggerEngine *createScriptEngine(const DebuggerStartParameters &);
DebuggerEngine *createPdbEngine(const DebuggerStartParameters &);
DebuggerEngine *createTcfEngine(const DebuggerStartParameters &);
DebuggerEngine *createQmlEngine(const DebuggerStartParameters &,
    DebuggerEngine *masterEngine);
DebuggerEngine *createQmlCppEngine(const DebuggerStartParameters &);
DebuggerEngine *createLldbEngine(const DebuggerStartParameters &);

extern QString msgNoBinaryForToolChain(const ProjectExplorer::Abi &abi);

static QString msgEngineNotAvailable(const char *engine)
{
    return DebuggerPlugin::tr("The application requires the debugger engine '%1', "
        "which is disabled.").arg(QLatin1String(engine));
}

////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControlPrivate
//
////////////////////////////////////////////////////////////////////////

class DebuggerRunControlPrivate
{
public:
    DebuggerRunControlPrivate(DebuggerRunControl *parent,
        RunConfiguration *runConfiguration);

    DebuggerEngineType engineForExecutable(unsigned enabledEngineTypes,
        const QString &executable);
    DebuggerEngineType engineForMode(unsigned enabledEngineTypes,
        DebuggerStartMode mode);

public:
    DebuggerRunControl *q;
    DebuggerEngine *m_engine;
    const QWeakPointer<RunConfiguration> m_myRunConfiguration;
    bool m_running;
    QString m_errorMessage;
    QString m_settingsIdHint;
};

DebuggerRunControlPrivate::DebuggerRunControlPrivate(DebuggerRunControl *parent,
        RunConfiguration *runConfiguration)
    : q(parent)
    , m_engine(0)
    , m_myRunConfiguration(runConfiguration)
    , m_running(false)
{
}

// Figure out the debugger type of an executable. Analyze executable
// unless the toolchain provides a hint.
DebuggerEngineType DebuggerRunControlPrivate::engineForExecutable
    (unsigned enabledEngineTypes, const QString &executable)
{
    if (executable.endsWith(_(".js"))) {
        if (enabledEngineTypes & ScriptEngineType)
            return ScriptEngineType;
        m_errorMessage = msgEngineNotAvailable("Script Engine");
    }

    if (executable.endsWith(_(".py"))) {
        if (enabledEngineTypes & PdbEngineType)
            return PdbEngineType;
        m_errorMessage = msgEngineNotAvailable("Pdb Engine");
    }

#ifdef Q_OS_WIN
    // A remote executable?
    if (!executable.endsWith(_(".exe")))
        return GdbEngineType;

    // If a file has PDB files, it has been compiled by VS.
    QStringList pdbFiles;
    if (!getPDBFiles(executable, &pdbFiles, &m_errorMessage)) {
        qWarning("Cannot determine type of executable %s: %s",
                 qPrintable(executable), qPrintable(m_errorMessage));
        return NoEngineType;
    }
    if (pdbFiles.empty())
        return GdbEngineType;

    // We need the CDB debugger in order to be able to debug VS
    // executables.
    Abi hostAbi = Abi::hostAbi();
    ConfigurationCheck check = checkDebugConfiguration(Abi(hostAbi.architecture(),
                                                           Abi::Windows,
                                                           hostAbi.osFlavor(),
                                                           Abi::Format_PE,
                                                           hostAbi.wordWidth()));
    if (!check) {
        m_errorMessage = check.errorMessage;
        m_settingsIdHint = check.settingsPage;
        if (enabledEngineTypes & CdbEngineType)
            return CdbEngineType;
        m_errorMessage = msgEngineNotAvailable("Cdb Engine");
        return NoEngineType;
    }
#else
    if (enabledEngineTypes & GdbEngineType)
        return GdbEngineType;
    m_errorMessage = msgEngineNotAvailable("Gdb Engine");
#endif

    return NoEngineType;
}

// Debugger type for mode.
DebuggerEngineType DebuggerRunControlPrivate::engineForMode
    (unsigned enabledEngineTypes, DebuggerStartMode startMode)
{
    if (startMode == AttachTcf)
        return TcfEngineType;

#ifdef Q_OS_WIN
    // Preferably Windows debugger for attaching locally.
    if (startMode != AttachToRemote && (enabledEngineTypes & CdbEngineType))
        return CdbEngineType;
    if (startMode == AttachCrashedExternal) {
        m_errorMessage = DebuggerRunControl::tr("There is no debugging engine available for post-mortem debugging.");
        return NoEngineType;
    }
    return GdbEngineType;
#else
    Q_UNUSED(startMode)
    Q_UNUSED(enabledEngineTypes)
    //  >m_errorMessage = msgEngineNotAvailable("Gdb Engine");
    return GdbEngineType;
#endif
}

} // namespace Internal


////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControl
//
////////////////////////////////////////////////////////////////////////

static DebuggerEngineType engineForToolChain(const Abi &toolChain)
{
    if (toolChain.binaryFormat() == Abi::Format_ELF || toolChain.binaryFormat() == Abi::Format_Mach_O
            || (toolChain.binaryFormat() == Abi::Format_PE && toolChain.osFlavor() == Abi::Windows_msys)) {
#ifdef WITH_LLDB
            // lldb override
            if (Core::ICore::instance()->settings()->value("LLDB/enabled").toBool())
                return LldbEngineType;
#endif
            return GdbEngineType;
    } else if (toolChain.binaryFormat() == Abi::Format_PE && toolChain.osFlavor() != Abi::Windows_msys) {
            return CdbEngineType;
    }
    return NoEngineType;
}


unsigned filterEngines(unsigned enabledEngineTypes)
{
#ifdef CDB_ENABLED
    if (!isCdbEngineEnabled() && !Cdb::isCdbEngineEnabled())
       enabledEngineTypes &= ~CdbEngineType;
#endif
    return enabledEngineTypes;
}

DebuggerRunControl::DebuggerRunControl(RunConfiguration *runConfiguration,
        const DebuggerStartParameters &startParams)
    : RunControl(runConfiguration, Constants::DEBUGMODE),
      d(new DebuggerRunControlPrivate(this, runConfiguration))
{
    connect(this, SIGNAL(finished()), SLOT(handleFinished()));

    // Figure out engine according to toolchain, executable, attach or default.
    DebuggerEngineType engineType = NoEngineType;
    DebuggerLanguages activeLangs = debuggerCore()->activeLanguages();
    DebuggerStartParameters sp = startParams;
    unsigned enabledEngineTypes = filterEngines(sp.enabledEngines);

    if (sp.executable.endsWith(_(".js")))
        engineType = ScriptEngineType;
    else if (sp.executable.endsWith(_(".py")))
        engineType = PdbEngineType;
    else {
        engineType = engineForToolChain(sp.toolChainAbi);
        if (engineType == CdbEngineType && !(enabledEngineTypes & CdbEngineType)) {
            d->m_errorMessage = msgEngineNotAvailable("Cdb Engine");
            engineType = NoEngineType;
        }
    }

    // FIXME: Unclean ipc override. Someone please have a better idea.
    if (sp.startMode == StartRemoteEngine)
        // For now thats the only supported IPC engine.
        engineType = LldbEngineType;

    // FIXME: 1 of 3 testing hacks.
    if (sp.processArgs.startsWith(__("@tcf@ ")))
        engineType = GdbEngineType;

    if (engineType == NoEngineType
            && sp.startMode != AttachToRemote
            && !sp.executable.isEmpty())
        engineType = d->engineForExecutable(enabledEngineTypes, sp.executable);

    if (engineType == NoEngineType)
        engineType = d->engineForMode(enabledEngineTypes, sp.startMode);

    if ((engineType != QmlEngineType && engineType != NoEngineType)
        && (activeLangs & QmlLanguage)) {
        if (activeLangs & CppLanguage) {
            sp.cppEngineType = engineType;
            engineType = QmlCppEngineType;
        } else {
            engineType = QmlEngineType;
        }
    }

    // qDebug() << "USING ENGINE : " << engineType;

    switch (engineType) {
        case GdbEngineType:
            d->m_engine = createGdbEngine(sp, 0);
            break;
        case ScriptEngineType:
            d->m_engine = createScriptEngine(sp);
            break;
        case CdbEngineType:
            d->m_engine = createCdbEngine(sp, 0, &d->m_errorMessage);
            break;
        case PdbEngineType:
            d->m_engine = createPdbEngine(sp);
            break;
        case TcfEngineType:
            d->m_engine = createTcfEngine(sp);
            break;
        case QmlEngineType:
            d->m_engine = createQmlEngine(sp, 0);
            break;
        case QmlCppEngineType:
            d->m_engine = createQmlCppEngine(sp);
            break;
        case LldbEngineType:
            d->m_engine = createLldbEngine(sp);
        case NoEngineType:
        case AllEngineTypes:
            break;
    }

    if (d->m_engine) {
        DebuggerToolTipManager::instance()->registerEngine(d->m_engine);
    } else {
        // Could not find anything suitable.
        debuggingFinished();
        // Create Message box with possibility to go to settings.
        const QString msg = tr("Cannot debug '%1' (binary format: '%2'): %3")
            .arg(sp.executable, sp.toolChainAbi.toString(), d->m_errorMessage);
        Core::ICore::instance()->showWarningWithOptions(tr("Warning"),
            msg, QString(), QLatin1String(Constants::DEBUGGER_SETTINGS_CATEGORY),
            d->m_settingsIdHint);
    }
}

DebuggerRunControl::~DebuggerRunControl()
{
    disconnect();
    if (DebuggerEngine *engine = d->m_engine) {
        d->m_engine = 0;
        engine->disconnect();
        delete engine;
    }
}

const DebuggerStartParameters &DebuggerRunControl::startParameters() const
{
    QTC_ASSERT(d->m_engine, return *(new DebuggerStartParameters()));
    return d->m_engine->startParameters();
}

QString DebuggerRunControl::displayName() const
{
    QTC_ASSERT(d->m_engine, return QString());
    return d->m_engine->startParameters().displayName;
}

void DebuggerRunControl::setCustomEnvironment(Utils::Environment env)
{
    QTC_ASSERT(d->m_engine, return);
    d->m_engine->startParameters().environment = env;
}

ConfigurationCheck checkDebugConfiguration(const ProjectExplorer::Abi &abi)
{
    ConfigurationCheck result;

    if (!(debuggerCore()->activeLanguages() & CppLanguage))
        return result;

    if (abi.binaryFormat() == Abi::Format_ELF ||
            abi.binaryFormat() == Abi::Format_Mach_O ||
            (abi.binaryFormat() == Abi::Format_PE && abi.osFlavor() == Abi::Windows_msys)) {
        if (debuggerCore()->debuggerForAbi(abi).isEmpty()) {
            result.errorMessage = msgNoBinaryForToolChain(abi);
            result.errorMessage += QLatin1Char(' ') + msgEngineNotAvailable("Gdb");
            //result.settingsPage = GdbOptionsPage::settingsId();
        }
    } else if (abi.binaryFormat() == Abi::Format_PE && abi.osFlavor() != Abi::Windows_msys) {
        result = checkCdbConfiguration(abi);
        if (!result) {
            result.errorMessage += msgEngineNotAvailable("Cdb");
            result.settingsPage = QLatin1String("Cdb");
        }
    }

    if (!result && !result.settingsPage.isEmpty())
        result.settingsCategory = QLatin1String(Constants::DEBUGGER_SETTINGS_CATEGORY);

    return result;
}

void DebuggerRunControl::start()
{
    QTC_ASSERT(d->m_engine, return);
    debuggerCore()->runControlStarted(d->m_engine);

    // We might get a synchronous startFailed() notification on Windows,
    // when launching the process fails. Emit a proper finished() sequence.
    emit started();
    d->m_running = true;

    d->m_engine->startDebugger(this);

    if (d->m_running)
        appendMessage(tr("Debugging starts"), NormalMessageFormat);
}

void DebuggerRunControl::startFailed()
{
    appendMessage(tr("Debugging has failed"), NormalMessageFormat);
    d->m_running = false;
    emit finished();
    d->m_engine->handleStartFailed();
}

void DebuggerRunControl::handleFinished()
{
    appendMessage(tr("Debugging has finished"), NormalMessageFormat);
    if (d->m_engine)
        d->m_engine->handleFinished();
    debuggerCore()->runControlFinished(d->m_engine);
}

void DebuggerRunControl::showMessage(const QString &msg, int channel)
{
    switch (channel) {
        case AppOutput:
            appendMessage(msg, StdOutFormatSameLine);
            break;
        case AppError:
            appendMessage(msg, StdErrFormatSameLine);
            break;
        case AppStuff:
            appendMessage(msg, NormalMessageFormat);
            break;
    }
}

bool DebuggerRunControl::promptToStop(bool *optionalPrompt) const
{
    QTC_ASSERT(isRunning(), return true;)

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
    QTC_ASSERT(d->m_engine, return StoppedSynchronously);
    d->m_engine->quitDebugger();
    return AsynchronousStop;
}

void DebuggerRunControl::debuggingFinished()
{
    d->m_running = false;
    emit finished();
}

bool DebuggerRunControl::isRunning() const
{
    return d->m_running;
}

DebuggerEngine *DebuggerRunControl::engine()
{
    QTC_ASSERT(d->m_engine, /**/);
    return d->m_engine;
}

RunConfiguration *DebuggerRunControl::runConfiguration() const
{
    return d->m_myRunConfiguration.data();
}


////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControlFactory
//
////////////////////////////////////////////////////////////////////////

// A factory to create DebuggerRunControls
DebuggerRunControlFactory::DebuggerRunControlFactory(QObject *parent,
        unsigned enabledEngines)
    : IRunControlFactory(parent), m_enabledEngines(enabledEngines)
{}

bool DebuggerRunControlFactory::canRun(RunConfiguration *runConfiguration, const QString &mode) const
{
//    return mode == ProjectExplorer::Constants::DEBUGMODE;
    return mode == Constants::DEBUGMODE
            && qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
}

QString DebuggerRunControlFactory::displayName() const
{
    return DebuggerRunControl::tr("Debug");
}

// Find Qt installation by running qmake
static inline QString findQtInstallPath(const QString &qmakePath)
{
    QProcess proc;
    QStringList args;
    args.append(QLatin1String("-query"));
    args.append(QLatin1String("QT_INSTALL_HEADERS"));
    proc.start(qmakePath, args);
    if (!proc.waitForStarted()) {
        qWarning("%s: Cannot start '%s': %s", Q_FUNC_INFO, qPrintable(qmakePath),
           qPrintable(proc.errorString()));
        return QString();
    }
    proc.closeWriteChannel();
    if (!proc.waitForFinished()) {
        Utils::SynchronousProcess::stopProcess(proc);
        qWarning("%s: Timeout running '%s'.", Q_FUNC_INFO, qPrintable(qmakePath));
        return QString();
    }
    if (proc.exitStatus() != QProcess::NormalExit) {
        qWarning("%s: '%s' crashed.", Q_FUNC_INFO, qPrintable(qmakePath));
        return QString();
    }
    const QByteArray ba = proc.readAllStandardOutput().trimmed();
    QDir dir(QString::fromLocal8Bit(ba));
    if (dir.exists() && dir.cdUp())
        return dir.absolutePath();
    return QString();
}

static DebuggerStartParameters localStartParameters(RunConfiguration *runConfiguration)
{
    DebuggerStartParameters sp;
    QTC_ASSERT(runConfiguration, return sp);
    LocalApplicationRunConfiguration *rc =
            qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return sp);

    sp.startMode = StartInternal;
    sp.environment = rc->environment();
    sp.workingDirectory = rc->workingDirectory();
    sp.executable = rc->executable();
    sp.processArgs = rc->commandLineArguments();
    sp.toolChainAbi = rc->abi();
    sp.useTerminal = rc->runMode() == LocalApplicationRunConfiguration::Console;
    sp.dumperLibrary = rc->dumperLibrary();
    sp.dumperLibraryLocations = rc->dumperLibraryLocations();

    if (debuggerCore()->isActiveDebugLanguage(QmlLanguage)) {
        sp.qmlServerAddress = QLatin1String("127.0.0.1");
        sp.qmlServerPort = runConfiguration->qmlDebugServerPort();

        sp.projectDir = runConfiguration->target()->project()->projectDirectory();
        if (runConfiguration->target()->activeBuildConfiguration())
            sp.projectBuildDir = runConfiguration->target()
                ->activeBuildConfiguration()->buildDirectory();

        // Makes sure that all bindings go through the JavaScript engine, so that
        // breakpoints are actually hit!
        if (!sp.environment.hasKey(QLatin1String("QML_DISABLE_OPTIMIZER"))) {
            sp.environment.set(QLatin1String("QML_DISABLE_OPTIMIZER"), QLatin1String("1"));
        }

        Utils::QtcProcess::addArg(&sp.processArgs, QLatin1String("-qmljsdebugger=port:")
                                  + QString::number(sp.qmlServerPort));
    }

    // FIXME: If it's not yet build this will be empty and not filled
    // when rebuild as the runConfiguration is not stored and therefore
    // cannot be used to retrieve the dumper location.
    //qDebug() << "DUMPER: " << sp.dumperLibrary << sp.dumperLibraryLocations;
    sp.displayName = rc->displayName();

    // Find qtInstallPath.
    QString qmakePath = DebuggingHelperLibrary::findSystemQt(rc->environment());
    if (!qmakePath.isEmpty())
        sp.qtInstallPath = findQtInstallPath(qmakePath);
    return sp;
}

RunControl *DebuggerRunControlFactory::create
    (RunConfiguration *runConfiguration, const QString &mode)
{
    QTC_ASSERT(mode == Constants::DEBUGMODE, return 0);
    DebuggerStartParameters sp = localStartParameters(runConfiguration);
    return create(sp, runConfiguration);
}

QWidget *DebuggerRunControlFactory::createConfigurationWidget
    (RunConfiguration *runConfiguration)
{
    // NBS TODO: Add GDB-specific configuration widget
    Q_UNUSED(runConfiguration)
    return 0;
}

DebuggerRunControl *DebuggerRunControlFactory::create
    (const DebuggerStartParameters &sp0, RunConfiguration *runConfiguration)
{
    DebuggerStartParameters sp = sp0;
    sp.enabledEngines = m_enabledEngines;
    ConfigurationCheck check = checkDebugConfiguration(sp.toolChainAbi);

    if (!check) {
        //appendMessage(errorMessage, true);
        Core::ICore::instance()->showWarningWithOptions(DebuggerRunControl::tr("Debugger"),
            check.errorMessage, QString(), check.settingsCategory, check.settingsPage);
        return 0;
    }

    DebuggerRunControl *runControl =
        new DebuggerRunControl(runConfiguration, sp);
    if (runControl->d->m_engine)
        return runControl;
    delete runControl;
    return 0;
}

} // namespace Debugger
