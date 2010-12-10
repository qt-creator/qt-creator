/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "debuggerrunner.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggermainwindow.h"
#include "debuggerplugin.h"
#include "debuggerstringutils.h"
#include "gdb/gdboptionspage.h"
#include "lldb/lldbenginehost.h"

#ifdef Q_OS_WIN
#  include "peutils.h"
#endif

#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/project.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/applicationrunconfiguration.h> // For LocalApplication*

#include <utils/synchronousprocess.h>
#include <utils/qtcassert.h>
#include <utils/fancymainwindow.h>
#include <coreplugin/icore.h>

#include <QtCore/QDir>
#include <QtGui/QMessageBox>

using namespace ProjectExplorer;
using namespace Debugger::Internal;

namespace Debugger {

namespace Cdb {
DebuggerEngine *createCdbEngine(const DebuggerStartParameters &, QString *error);
bool isCdbEngineEnabled(); // Check the configuration page
}

namespace Internal {

DebuggerEngine *createGdbEngine(const DebuggerStartParameters &);
DebuggerEngine *createScriptEngine(const DebuggerStartParameters &);
DebuggerEngine *createPdbEngine(const DebuggerStartParameters &);
DebuggerEngine *createTcfEngine(const DebuggerStartParameters &);
DebuggerEngine *createQmlEngine(const DebuggerStartParameters &);
DebuggerEngine *createQmlCppEngine(const DebuggerStartParameters &);
DebuggerEngine *createLldbEngine(const DebuggerStartParameters &);

extern QString msgNoBinaryForToolChain(int tc);

// FIXME: Outdated?
// The createCdbEngine function takes a list of options pages it can add to.
// This allows for having a "enabled" toggle on the page independently
// of the engine. That's good for not enabling the related ActiveX control
// unnecessarily.

#ifdef CDB_ENABLED

DebuggerEngine *createCdbEngine(const DebuggerStartParameters &, QString *error);
bool checkCdbConfiguration(int toolChain, QString *errorMsg, QString *settingsPage);
bool isCdbEngineEnabled(); // Check the configuration page

#else

DebuggerEngine *createCdbEngine(const DebuggerStartParameters &, QString *)
{
    return 0;
}

bool checkCdbConfiguration(int, QString *, QString *)
{
    return false;
}

#endif

////////////////////////////////////////////////////////////////////////
//
// DebuggerRunControlFactory
//
////////////////////////////////////////////////////////////////////////

static QString msgEngineNotAvailable(const char *engine)
{
    return DebuggerPlugin::tr("The application requires the debugger engine '%1', "
        "which is disabled.").arg(QLatin1String(engine));
}

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
    return tr("Debug");
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
    sp.toolChainType = rc->toolChainType();
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

        sp.processArgs.append(QLatin1String("-qmljsdebugger=port:")
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

DebuggerRunControl *DebuggerRunControlFactory::create
    (const DebuggerStartParameters &sp, RunConfiguration *runConfiguration)
{
    DebuggerRunControl *runControl =
        new DebuggerRunControl(runConfiguration, m_enabledEngines, sp);
    if (runControl->engine())
        return runControl;
    qDebug() << "FAILED TO CREATE ENGINE";
    delete runControl;
    return 0;
}

QWidget *DebuggerRunControlFactory::createConfigurationWidget
    (RunConfiguration *runConfiguration)
{
    // NBS TODO: Add GDB-specific configuration widget
    Q_UNUSED(runConfiguration)
    return 0;
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
        RunConfiguration *runConfiguration, unsigned enabledEngines);
    unsigned enabledEngines() const;

    DebuggerEngineType engineForExecutable(unsigned enabledEngineTypes,
        const QString &executable);
    DebuggerEngineType engineForMode(unsigned enabledEngineTypes,
        DebuggerStartMode mode);

public:
    DebuggerRunControl *q;
    DebuggerEngine *m_engine;
    const QWeakPointer<RunConfiguration> m_myRunConfiguration;
    bool m_running;
    const unsigned m_cmdLineEnabledEngines;
    QString m_errorMessage;
    QString m_settingsIdHint;
};

unsigned DebuggerRunControlPrivate::enabledEngines() const
{
    unsigned rc = m_cmdLineEnabledEngines;
#ifdef CDB_ENABLED
    if (!isCdbEngineEnabled() && !Cdb::isCdbEngineEnabled())
        rc &= ~CdbEngineType;
#endif
    return rc;
}

DebuggerRunControlPrivate::DebuggerRunControlPrivate(DebuggerRunControl *parent,
        RunConfiguration *runConfiguration, unsigned enabledEngines)
    : q(parent)
    , m_engine(0)
    , m_myRunConfiguration(runConfiguration)
    , m_running(false)
    , m_cmdLineEnabledEngines(enabledEngines)
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
   if (DebuggerRunControl::checkDebugConfiguration(ProjectExplorer::ToolChain_MSVC,
            &m_errorMessage, 0, &m_settingsIdHint)) {
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

static DebuggerEngineType engineForToolChain(int toolChainType)
{
    switch (toolChainType) {
        case ProjectExplorer::ToolChain_LINUX_ICC:
        case ProjectExplorer::ToolChain_MinGW:
        case ProjectExplorer::ToolChain_GCC:
        case ProjectExplorer::ToolChain_WINSCW: // S60
        case ProjectExplorer::ToolChain_GCCE:
        case ProjectExplorer::ToolChain_RVCT2_ARMV5:
        case ProjectExplorer::ToolChain_RVCT2_ARMV6:
        case ProjectExplorer::ToolChain_RVCT_ARMV5_GNUPOC:
        case ProjectExplorer::ToolChain_GCCE_GNUPOC:
        case ProjectExplorer::ToolChain_GCC_MAEMO:
#ifdef WITH_LLDB
            // lldb override
            if (Core::ICore::instance()->settings()->value("LLDB/enabled").toBool())
                return LldbEngineType;
#endif
            return GdbEngineType;


        case ProjectExplorer::ToolChain_MSVC:
        case ProjectExplorer::ToolChain_WINCE:
            return CdbEngineType;

        case ProjectExplorer::ToolChain_OTHER:
        case ProjectExplorer::ToolChain_UNKNOWN:
        case ProjectExplorer::ToolChain_INVALID:
        default:
            break;
    }
    return NoEngineType;
}

DebuggerRunControl::DebuggerRunControl(RunConfiguration *runConfiguration,
        unsigned enabledEngines, const DebuggerStartParameters &startParams)
    : RunControl(runConfiguration, Constants::DEBUGMODE),
      d(new DebuggerRunControlPrivate(this, runConfiguration, enabledEngines))
{
    connect(this, SIGNAL(finished()), SLOT(handleFinished()));

    // Figure out engine according to toolchain, executable, attach or default.
    DebuggerEngineType engineType = NoEngineType;
    DebuggerLanguages activeLangs = debuggerCore()->activeLanguages();
    DebuggerStartParameters sp = startParams;
    const unsigned enabledEngineTypes = d->enabledEngines();
    if (sp.executable.endsWith(_(".js")))
        engineType = ScriptEngineType;
    else if (sp.executable.endsWith(_(".py")))
        engineType = PdbEngineType;
    else {
        engineType = engineForToolChain(sp.toolChainType);
        if (engineType == CdbEngineType && !(enabledEngineTypes & CdbEngineType)) {
            d->m_errorMessage = msgEngineNotAvailable("Cdb Engine");
            engineType = NoEngineType;
        }
    }

    // Fixme: unclean ipc override. Someone please have a better idea
    if (sp.startMode == StartRemoteEngine)
        // for now thats the only supported ipc engine
        engineType = LldbEngineType;

    // Fixme: 1 of 3 testing hacks.
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
            d->m_engine = createGdbEngine(sp);
            break;
        case ScriptEngineType:
            d->m_engine = createScriptEngine(sp);
            break;
        case CdbEngineType:
            // Try new engine, fall back to old.
            if (Cdb::isCdbEngineEnabled())
                d->m_engine = Cdb::createCdbEngine(sp, &d->m_errorMessage);
            else
                d->m_engine = Internal::createCdbEngine(sp, &d->m_errorMessage);
            break;
        case PdbEngineType:
            d->m_engine = createPdbEngine(sp);
            break;
        case TcfEngineType:
            d->m_engine = createTcfEngine(sp);
            break;
        case QmlEngineType:
            d->m_engine = createQmlEngine(sp);
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

    if (!d->m_engine) {
        // Could not find anything suitable.
        debuggingFinished();
        // Create Message box with possibility to go to settings.
        const QString msg = tr("Cannot debug '%1' (tool chain: '%2'): %3")
            .arg(sp.executable, sp.toolChainName(), d->m_errorMessage);
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
    d->m_engine->startParameters().environment = env;
}

bool DebuggerRunControl::checkDebugConfiguration(int toolChain,
                                              QString *errorMessage,
                                              QString *settingsCategory /* = 0 */,
                                              QString *settingsPage /* = 0 */)
{
    errorMessage->clear();
    if (settingsCategory)
        settingsCategory->clear();
    if (settingsPage)
        settingsPage->clear();

    bool success = true;

    if (!(debuggerCore()->activeLanguages() & CppLanguage))
        return success;

    switch(toolChain) {
    case ProjectExplorer::ToolChain_GCC:
    case ProjectExplorer::ToolChain_LINUX_ICC:
    case ProjectExplorer::ToolChain_MinGW:
    case ProjectExplorer::ToolChain_WINCE: // S60
    case ProjectExplorer::ToolChain_WINSCW:
    case ProjectExplorer::ToolChain_GCCE:
    case ProjectExplorer::ToolChain_RVCT2_ARMV5:
    case ProjectExplorer::ToolChain_RVCT2_ARMV6:
        if (debuggerCore()->gdbBinaryForToolChain(toolChain).isEmpty()) {
            *errorMessage = msgNoBinaryForToolChain(toolChain);
            *settingsPage = GdbOptionsPage::settingsId();
            *errorMessage += msgEngineNotAvailable("Gdb");
            success = false;
        } else {
            success = true;
        }
        break;
    case ProjectExplorer::ToolChain_MSVC:
        success = checkCdbConfiguration(toolChain, errorMessage, settingsPage);
        if (!success) {
            *errorMessage += msgEngineNotAvailable("Cdb");
            if (settingsPage)
                *settingsPage = QLatin1String("Cdb");
        }
        break;
    }
    if (!success && settingsCategory && settingsPage && !settingsPage->isEmpty())
        *settingsCategory = QLatin1String(Constants::DEBUGGER_SETTINGS_CATEGORY);

    return success;
}

void DebuggerRunControl::start()
{
    QTC_ASSERT(d->m_engine, return);
    const DebuggerStartParameters &sp = d->m_engine->startParameters();

    QString errorMessage;
    QString settingsCategory;
    QString settingsPage;

    if (!checkDebugConfiguration(sp.toolChainType,
            &errorMessage, &settingsCategory, &settingsPage)) {
        emit appendMessage(this, errorMessage, true);
        emit finished();
        Core::ICore::instance()->showWarningWithOptions(tr("Debugger"),
            errorMessage, QString(), settingsCategory, settingsPage);
        return;
    }

    debuggerCore()->runControlStarted(engine());

    // We might get a synchronous startFailed() notification on Windows,
    // when launching the process fails. Emit a proper finished() sequence.
    emit started();
    d->m_running = true;

    engine()->startDebugger(this);

    if (d->m_running) {
        emit addToOutputWindowInline(this, tr("Debugging starts"), false);
        emit addToOutputWindowInline(this, "\n", false);
    }
}

void DebuggerRunControl::startFailed()
{
    emit addToOutputWindowInline(this, tr("Debugging has failed"), false);
    d->m_running = false;
    emit finished();
    engine()->handleStartFailed();
}

void DebuggerRunControl::handleFinished()
{
    emit addToOutputWindowInline(this, tr("Debugging has finished"), false);
    if (engine())
        engine()->handleFinished();
    debuggerCore()->runControlFinished(engine());
}

void DebuggerRunControl::showMessage(const QString &msg, int channel)
{
    switch (channel) {
        case AppOutput:
            emit addToOutputWindowInline(this, msg, false);
            break;
        case AppError:
            emit addToOutputWindowInline(this, msg, true);
            break;
        case AppStuff:
            emit appendMessage(this, msg, true);
            break;
    }
}

bool DebuggerRunControl::aboutToStop() const
{
    QTC_ASSERT(isRunning(), return true;)

    const QString question = tr("A debugging session is still in progress. "
            "Terminating the session in the current"
            " state can leave the target in an inconsistent state."
            " Would you still like to terminate it?");

    const QMessageBox::StandardButton answer =
            QMessageBox::question(debuggerCore()->mainWindow(),
                                  tr("Close Debugging Session"), question,
                                  QMessageBox::Yes|QMessageBox::No);
    return answer == QMessageBox::Yes;
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

} // namespace Debugger
