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

#include "cdbengine.h"
#include "cdbengine_p.h"
#include "cdbdebugoutput.h"
#include "cdbdebugeventcallback.h"
#include "cdbstacktracecontext.h"
#include "cdbsymbolgroupcontext.h"
#include "cdbbreakpoint.h"
#include "cdbmodules.h"
#include "cdbassembler.h"
#include "cdboptionspage.h"
#include "cdboptions.h"
#include "cdbexceptionutils.h"
#include "cdbsymbolpathlisteditor.h"
#include "dbgwinutils.h"
#include "debuggercore.h"
#include "disassembleragent.h"
#include "memoryagent.h"

#include "debuggeractions.h"
#include "breakhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "threadshandler.h"
#include "registerhandler.h"
#include "moduleshandler.h"
#include "watchutils.h"
#include "corebreakpoint.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/winutils.h>
#include <utils/consoleprocess.h>
#include <utils/fancymainwindow.h>
#include <texteditor/itexteditor.h>
#include <utils/savedaction.h>
#include <utils/checkablemessagebox.h>
#include <projectexplorer/toolchain.h>

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QTimerEvent>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QLibrary>
#include <QtCore/QCoreApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>
#include <QtGui/QApplication>
#include <QtGui/QToolTip>

#define DBGHELP_TRANSLATE_TCHAR
#include <inc/Dbghelp.h>

static const char localSymbolRootC[] = "local";

#if 0
#  define STATE_DEBUG(func, line, notifyFunc) qDebug("%s at %s:%d", notifyFunc, func, line);
#else
#  define STATE_DEBUG(func, line, notifyFunc)
#endif

namespace Debugger {
namespace Internal {

CdbOptionsPage *theOptionsPage = 0;
typedef QList<WatchData> WatchList;

// ----- Message helpers

static QString msgStackIndexOutOfRange(int idx, int size)
{
    return QString::fromLatin1("Frame index %1 out of range (%2).").arg(idx).arg(size);
}

QString msgDebuggerCommandFailed(const QString &command, HRESULT hr)
{
    return QString::fromLatin1("Unable to execute '%1': %2").arg(command, CdbCore::msgDebugEngineComResult(hr));
}

static const char *msgNoStackTraceC = "Internal error: no stack trace present.";

// Format function failure message. Pass in Q_FUNC_INFO
static QString msgFunctionFailed(const char *func, const QString &why)
{
    // Strip a "cdecl_ int namespace1::class::foo(int bar)" as
    // returned by Q_FUNC_INFO down to "foo"
    QString function = QLatin1String(func);
    const int firstParentPos = function.indexOf(QLatin1Char('('));
    if (firstParentPos != -1)
        function.truncate(firstParentPos);
    const int classSepPos = function.lastIndexOf(QLatin1String("::"));
    if (classSepPos != -1)
        function.remove(0, classSepPos + 2);
   //: Function call failed
   return CdbEngine::tr("The function \"%1()\" failed: %2").arg(function, why);
}

// ----- Engine helpers

// --- CdbEnginePrivate

CdbEnginePrivate::CdbEnginePrivate(CdbEngine *engine) :
    m_options(theOptionsPage->options()),
    m_hDebuggeeProcess(0),
    m_hDebuggeeThread(0),
    m_breakEventMode(BreakEventHandle),
    m_dumper(new CdbDumperHelper(engine, this)),
    m_currentThreadId(-1),
    m_eventThreadId(-1),
    m_interruptArticifialThreadId(-1),
    m_ignoreInitialBreakPoint(false),
    m_interrupted(false),
    m_engine(engine),
    m_currentStackTrace(0),
    m_firstActivatedFrame(true),
    m_inferiorStartupComplete(false),
    m_mode(AttachCore),
    m_stoppedReason(StoppedOther)
{
    connect(this, SIGNAL(watchTimerDebugEvent()), this, SLOT(handleDebugEvent()));
    connect(this, SIGNAL(modulesLoaded()), this, SLOT(slotModulesLoaded()));
}

bool CdbEnginePrivate::init(QString *errorMessage)
{
    enum {  bufLen = 10240 };

    if (!CdbCore::CoreEngine::init(m_options->path, errorMessage))
        return false;
    CdbDebugOutput *output = new CdbDebugOutput;
    connect(output, SIGNAL(showMessage(QString,int,int)),
            m_engine, SLOT(showMessage(QString,int,int)),
            Qt::QueuedConnection); // Multithread invocation when loading dumpers.
    setDebugOutput(DebugOutputBasePtr(output));
    setDebugEventCallback(DebugEventCallbackBasePtr(new CdbDebugEventCallback(m_engine)));
    updateCodeLevel();

    return true;
}

DebuggerEngine *CdbEngine::create(const DebuggerStartParameters &sp,
                                       QString *errorMessage)
{
    if (!CdbCore::CoreEngine::interfacesAvailable()) {
        *errorMessage = CdbEngine::tr("An instance of the CDB engine is still running; cannot create an a new instance.");
        return 0;
    }
    CdbEngine *rc = new CdbEngine(sp);
    if (rc->m_d->init(errorMessage)) {
        rc->syncDebuggerPaths();
        return rc;
    }
    delete rc;
    return 0;
}

void  CdbEnginePrivate::updateCodeLevel()
{
    const CdbCore::CoreEngine::CodeLevel cl = debuggerCore()->boolSetting(OperateByInstruction) ?
                                              CdbCore::CoreEngine::CodeLevelAssembly : CdbCore::CoreEngine::CodeLevelSource;
    setCodeLevel(cl);
}

CdbEnginePrivate::~CdbEnginePrivate()
{
    cleanStackTrace();
}

void CdbEnginePrivate::clearForRun()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    m_breakEventMode = BreakEventHandle;
    m_eventThreadId = m_interruptArticifialThreadId = -1;
    m_interrupted = false;
    cleanStackTrace();
    m_stoppedReason = StoppedOther;
    m_stoppedMessage.clear();
    m_engine->threadsHandler()->notifyRunning();
}

void CdbEnginePrivate::cleanStackTrace()
{
    if (m_currentStackTrace) {
        delete m_currentStackTrace;
        m_currentStackTrace = 0;
    }
    m_firstActivatedFrame = false;
    m_editorToolTipCache.clear();
}

CdbEngine::CdbEngine(const DebuggerStartParameters &startParameters) :
    DebuggerEngine(startParameters),
    m_d(new CdbEnginePrivate(this))
{
    setObjectName(QLatin1String("CdbEngine"));
    m_d->m_consoleStubProc.setMode(Utils::ConsoleProcess::Suspend);
    connect(&m_d->m_consoleStubProc, SIGNAL(processMessage(QString,bool)),
            this, SLOT(slotConsoleStubMessage(QString, bool)));
    connect(&m_d->m_consoleStubProc, SIGNAL(processStarted()),
            this, SLOT(slotConsoleStubStarted()));
    connect(&m_d->m_consoleStubProc, SIGNAL(wrapperStopped()),
            this, SLOT(slotConsoleStubTerminated()));
}

CdbEngine::~CdbEngine()
{
    delete m_d;
}

void CdbEngine::shutdownInferior()
{
    QString errorMessage;
    if (!m_d->endInferior(false, &errorMessage))
        showMessage(errorMessage, LogError);
}

void CdbEngine::shutdownEngine()
{
    m_d->endDebugging();
}

QString CdbEngine::editorToolTip(const QString &exp, const QString &function)
{
    // Figure the editor tooltip. Ask the frame context of the
    // function if it is a local variable it knows. If that is not
    // the case, try to evaluate via debugger
    QString errorMessage;
    QString rc;
    // Find the frame of the function if there is any
    CdbSymbolGroupContext *frame = 0;
    if (m_d->m_currentStackTrace &&  !function.isEmpty()) {
        const int frameIndex = m_d->m_currentStackTrace->indexOf(function);
        if (debugToolTips)
            qDebug() << "CdbEngine::editorToolTip" << exp << function << frameIndex;
        if (frameIndex != -1)
            frame = m_d->m_currentStackTrace->cdbSymbolGroupContextAt(frameIndex, &errorMessage);
    }
    if (frame && frame->editorToolTip(QLatin1String("local.") + exp, &rc, &errorMessage))
        return rc;
    // No function/symbol context found, try to evaluate in current context.
    // Do not append type as this will mostly be 'long long' for integers, etc.
    QString type;
    if (debugToolTips)
        qDebug() << "Defaulting to expression";
    if (!m_d->evaluateExpression(exp, &rc, &type, &errorMessage))
        return QString();
    return rc;
}

void CdbEngine::setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos)
{
    typedef CdbEnginePrivate::EditorToolTipCache EditorToolTipCache;
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << '\n' << cursorPos;
    // Need a stopped debuggee and a cpp file
    if (!m_d->m_hDebuggeeProcess || m_d->isDebuggeeRunning())
        return;
    if (!isCppEditor(editor))
        return;
    // Determine expression and function
    QString toolTip;
    do {
        int line;
        int column;
        QString function;
        const QString exp = cppExpressionAt(editor, cursorPos, &line, &column, &function);
        if (function.isEmpty() || exp.isEmpty())
            break;
        // Check cache (key containing function) or try to figure out expression
        QString cacheKey = function;
        cacheKey += QLatin1Char('@');
        cacheKey += exp;
        const EditorToolTipCache::const_iterator cit = m_d->m_editorToolTipCache.constFind(cacheKey);
        if (cit != m_d->m_editorToolTipCache.constEnd()) {
            toolTip = cit.value();
        } else {
            toolTip = editorToolTip(exp, function);
            if (!toolTip.isEmpty())
                m_d->m_editorToolTipCache.insert(cacheKey, toolTip);
        }
    } while (false);
    // Display
    QToolTip::hideText();
    if (!toolTip.isEmpty())
        QToolTip::showText(mousePos, toolTip);
}

void CdbEnginePrivate::clearDisplay()
{
    m_engine->threadsHandler()->removeAll();
    m_engine->modulesHandler()->removeAll();
    m_engine->registerHandler()->removeAll();
}

void CdbEnginePrivate::checkVersion()
{
    static bool versionNotChecked = true;
    // Check for version 6.11 (extended expression syntax)
    if (versionNotChecked) {
        versionNotChecked = false;
        // Get engine DLL version
        QString errorMessage;
        const QString version = Utils::winGetDLLVersion(Utils::WinDLLProductVersion, dbengDLL(), &errorMessage);
        if (version.isEmpty()) {
            qWarning("%s\n", qPrintable(errorMessage));
            return;
        }
        // Compare
        const double minVersion = 6.11;
        m_engine->showMessage(CdbEngine::tr("Version: %1").arg(version));
        if (version.toDouble() <  minVersion) {
            const QString msg = CdbEngine::tr(
                    "<html>The installed version of the <i>Debugging Tools for Windows</i> (%1) "
                    "is rather old. Upgrading to version %2 is recommended "
                    "for the proper display of Qt's data types.</html>").arg(version).arg(minVersion);
            Core::ICore::instance()->showWarningWithOptions(CdbEngine::tr("Debugger"), msg, QString(),
                                                            QLatin1String(Constants::DEBUGGER_SETTINGS_CATEGORY),
                                                            CdbOptionsPage::settingsId());
        }
    }
}

void CdbEngine::setupEngine()
{
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    const DebuggerStartParameters &sp = startParameters();
    if (debugCDBExecution)
        qDebug("setupEngine");
    CdbCore::BreakPoint::clearNormalizeFileNameCache();
    // Nag to add symbol server
    if (CdbSymbolPathListEditor::promptToAddSymbolServer(CdbOptions::settingsGroup(),
                                                         &(m_d->m_options->symbolPaths))) {
        m_d->m_options->toSettings(Core::ICore::instance()->settings());
        syncDebuggerPaths();
    }
    m_d->checkVersion();
    if (m_d->m_hDebuggeeProcess) {
        warning(QLatin1String("Internal error: Attempt to start debugger while another process is being debugged."));
        notifyEngineSetupFailed();
        return;
    }
    switch (sp.startMode) {
    case AttachCore:
    case AttachToRemote:
        warning(QLatin1String("Internal error: Mode not supported."));
        notifyEngineSetupFailed();
        return;
    default:
        break;
    }
    m_d->m_mode = sp.startMode;
    m_d->clearDisplay();
    m_d->m_inferiorStartupComplete = false;
    // Options
    QString errorMessage;
    if (!m_d->setBreakOnThrow(m_d->m_options->breakOnException, &errorMessage))
        showMessage(errorMessage, LogWarning);
    m_d->setVerboseSymbolLoading(m_d->m_options->verboseSymbolLoading);
    // Figure out dumper. @TODO: same in gdb...
    const QString dumperLibName = QDir::toNativeSeparators(qtDumperLibraryName());
    bool dumperEnabled = m_d->m_mode != AttachCore
                         && m_d->m_mode != AttachCrashedExternal
                         && qtDumperLibraryEnabled();
    if (dumperEnabled) {
        const QFileInfo fi(dumperLibName);
        if (!fi.isFile()) {
            const QStringList &locations = qtDumperLibraryLocations();
            const QString loc = locations.join(QLatin1String(", "));
            const QString msg = tr("The dumper library was not found at %1.").arg(loc);
            showQtDumperLibraryWarning(msg);
            dumperEnabled = false;
        }
    }
    m_d->m_dumper->reset(dumperLibName, dumperEnabled);
    STATE_DEBUG(Q_FUNC_INFO, __LINE__,  "notifyEngineSetupOk");
    notifyEngineSetupOk();
}

void CdbEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    STATE_DEBUG(Q_FUNC_INFO, __LINE__,  "notifyInferiorSetupOk");
    notifyInferiorSetupOk();
}

void CdbEngine::runEngine()
{
    if (debugCDBExecution)
        qDebug("runEngine");
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    const DebuggerStartParameters &sp = startParameters();
    bool rc = false;
    bool needWatchTimer = false;
    QString errorMessage;
    m_d->clearForRun();
    m_d->updateCodeLevel();
    m_d->m_ignoreInitialBreakPoint = false;
    switch (m_d->m_mode) {
    case AttachExternal:
    case AttachCrashedExternal:
        rc = startAttachDebugger(sp.attachPID, m_d->m_mode, &errorMessage);
        needWatchTimer = true; // Fetch away module load, etc. even if crashed
        break;
    case StartInternal:
    case StartExternal: {
        Utils::QtcProcess::SplitError perr;
        QString pargs = Utils::QtcProcess::prepareArgs(sp.processArgs, &perr,
                                                       &sp.environment, &sp.workingDirectory);
        if (perr != Utils::QtcProcess::SplitOk) {
            // perr == BadQuoting is never returned on Windows
            // FIXME? QTCREATORBUG-2809
            errorMessage = QApplication::translate("DebuggerEngine", // Same message in GdbEngine
                    "Debugging complex command lines is currently not supported under Windows");
            break;
        }
        if (sp.useTerminal) {
            // Attaching to console processes triggers an initial breakpoint, which we do not want
            m_d->m_ignoreInitialBreakPoint = true;
            // Launch console stub and wait for its startup
            m_d->m_consoleStubProc.stop(); // We leave the console open, so recycle it now.
            m_d->m_consoleStubProc.setWorkingDirectory(sp.workingDirectory);
            m_d->m_consoleStubProc.setEnvironment(sp.environment);
            rc = m_d->m_consoleStubProc.start(sp.executable, pargs);
            if (!rc)
                errorMessage = tr("The console stub process was unable to start '%1'.").arg(sp.executable);
            // continues in slotConsoleStubStarted()...
        } else {
            needWatchTimer = true;
            rc = m_d->startDebuggerWithExecutable(sp.workingDirectory,
                                                  sp.executable,
                                                  pargs,
                                                  sp.environment.toStringList(),
                                                  &errorMessage);
        }
        break; }
    case AttachCore:
        errorMessage = tr("Attaching to core files is not supported.");
        break;
    }
    if (rc) {
        if (needWatchTimer)
            m_d->startWatchTimer();
        // Continues processCreatedAttached().
    } else {
        warning(errorMessage);
        STATE_DEBUG(Q_FUNC_INFO, __LINE__,  "notifyEngineRunFailed");
        notifyEngineRunFailed();
    }
}

bool CdbEngine::startAttachDebugger(qint64 pid, DebuggerStartMode sm, QString *errorMessage)
{
    // Need to attach invasively, otherwise, no notification signals
    // for for CreateProcess/ExitProcess occur.
    // Initial breakpoint occur:
    // 1) Desired: When attaching to a crashed process
    // 2) Undesired: When starting up a console process, in conjunction
    //    with the 32bit Wow-engine
    //  As of version 6.11, the flag only affects 1). 2) Still needs to be suppressed
    // by lookup at the state of the application (startup trap). However,
    // there is no startup trap when attaching to a process that has been
    // running for a while. (see notifyException).
    const bool suppressInitialBreakPoint = sm != AttachCrashedExternal;
    return m_d->startAttachDebugger(pid, suppressInitialBreakPoint, errorMessage);
}

void CdbEnginePrivate::processCreatedAttached(ULONG64 processHandle, ULONG64 initialThreadHandle)
{
    if (debugCDBExecution)
        qDebug(">processCreatedAttached");
    STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyEngineRunAndInferiorStopOk");
    m_engine->notifyEngineRunAndInferiorStopOk();
    setDebuggeeHandles(reinterpret_cast<HANDLE>(processHandle), reinterpret_cast<HANDLE>(initialThreadHandle));
    ULONG currentThreadId;
    if (SUCCEEDED(interfaces().debugSystemObjects->GetThreadIdByHandle(initialThreadHandle, &currentThreadId))) {
        m_currentThreadId = currentThreadId;
    } else {
        m_currentThreadId = 0;
    }
    // Clear any saved breakpoints and set initial breakpoints
    m_engine->executeDebuggerCommand(QLatin1String("bc"));
    m_engine->attemptBreakpointSynchronization();
    // Attaching to crashed: This handshake (signalling an event) is required for
    // the exception to be delivered to the debugger
    // Also, see special handling in slotModulesLoaded().
    if (m_mode == AttachCrashedExternal) {
        const QString crashParameter = m_engine->startParameters().crashParameter;
        if (!crashParameter.isEmpty()) {
            ULONG64 evtNr = crashParameter.toULongLong();
            const HRESULT hr = interfaces().debugControl->SetNotifyEventHandle(evtNr);
            if (FAILED(hr))
                m_engine->warning(QString::fromLatin1("Handshake failed on event #%1: %2").arg(evtNr).arg(CdbCore::msgComFailed("SetNotifyEventHandle", hr)));
        }
    }
    STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyInferiorRunRequested()/notifyInferiorRunOk()");
    m_engine->notifyInferiorRunRequested();
    m_engine->notifyInferiorRunOk();
    if (debugCDBExecution)
        qDebug("<processCreatedAttached");
}

void CdbEngine::processTerminated(unsigned long exitCode)
{
    if (debugCDBExecution)
        qDebug("processTerminated: %lu", exitCode);
    showMessage(tr("The process exited with exit code %1.").arg(exitCode));
    m_d->setDebuggeeHandles(0, 0);
    m_d->clearForRun();
    STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyInferiorExited");
    notifyInferiorExited();
}

bool CdbEnginePrivate::endInferior(bool detachOnly, QString *errorMessage)
{
    QTC_ASSERT(hasInterfaces(), return true; )
    // Prevent repeated invocation.
    const bool hasHandles = m_hDebuggeeProcess != NULL;
    if (debugCDBExecution)
        qDebug("endInferior detach=%d, %s handles=%d", detachOnly, DebuggerEngine::stateName(m_engine->state()), hasHandles);
    if (!hasHandles)
        return true;
    // Are we running
    switch (m_engine->state()) {
    case InferiorRunRequested:
    case InferiorRunOk:
    case InferiorRunFailed:
    case InferiorStopRequested:
    case InferiorStopOk:
    case InferiorStopFailed:
    case InferiorShutdownRequested:
    case EngineShutdownRequested: // Forwarded when choosing 'Abort...' an attached process.
        break;
    default:
        return true;
    }
    // Detach or terminate?
    if (!detachOnly && (m_mode == AttachExternal || m_mode == AttachCrashedExternal))
        detachOnly = true;
    // Process must be stopped in order to terminate
    const bool wasRunning = isDebuggeeRunning();
    if (debugCDBExecution)
        qDebug("endInferior detach=%d, running=%d", detachOnly, wasRunning);
    if (wasRunning) {
        interruptInterferiorProcess(errorMessage);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    bool success = false;
    if (detachOnly) {
        if (detachCurrentProcess(errorMessage))
            success = true;
    } else {
        // Stop debuggee. Weird exit logic.
        do {
            // The exit process event handler will not be called.
            terminateCurrentProcess(errorMessage);
            if (wasRunning) {
                success = true;
                break;
            }
            if (terminateProcesses(errorMessage))
                success = true;
        } while (false);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    // Perform cleanup even when failed..no point clinging to the process
    setDebuggeeHandles(0, 0);
    killWatchTimer();
    clearForRun();
    if (success) {
        STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyInferiorShutdownOk");
        m_engine->notifyInferiorShutdownOk();
    } else {
        *errorMessage = QString::fromLatin1("Unable to detach from/end the debuggee: %1").arg(*errorMessage);
        STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyInferiorShutdownFailed");
        m_engine->notifyInferiorShutdownFailed();
    }
    return success;
}

// End debugging. Note that this can invoked via user action
// or the processTerminated() event handler, in which case it
// must not kill the process again.
void CdbEnginePrivate::endDebugging(bool detachOnly)
{
    if (debugCDBExecution)
        qDebug("endDebugging() detach=%d, state=%s", detachOnly, DebuggerEngine::stateName(m_engine->state()));

    QTC_ASSERT(hasInterfaces(), return; )

    switch (m_engine->state()) {
    case DebuggerNotReady:
    case EngineShutdownOk:
    case EngineShutdownFailed:
    case DebuggerFinished:
        return;
    default:
        break;
    }
    // Do we need to stop the process?
    QString errorMessage;
    if (!endInferior(detachOnly, &errorMessage)) {
        m_engine->showMessage(errorMessage, LogError);
        errorMessage.clear();
    }
    if (endSession(&errorMessage)) {
        STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyEngineShutdownOk");
        m_engine->notifyEngineShutdownOk();
    } else {
        STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyEngineShutdownFailed");
        errorMessage = QString::fromLatin1("There were errors trying to end debugging:\n%1").arg(errorMessage);
        m_engine->showMessage(errorMessage, LogError);
        m_engine->notifyEngineShutdownFailed();
    }
    // At this point release interfaces as we might be kept around by the run control.
    releaseInterfaces();
}

void CdbEngine::detachDebugger()
{
    m_d->endDebugging(true);
}

CdbSymbolGroupContext *CdbEnginePrivate::getSymbolGroupContext(int frameIndex, QString *errorMessage) const
{
    if (!m_currentStackTrace) {
        *errorMessage = QLatin1String(msgNoStackTraceC);
        return 0;
    }
    if (CdbSymbolGroupContext *sg = m_currentStackTrace->cdbSymbolGroupContextAt(frameIndex, errorMessage))
        return sg;
    return 0;
}

void CdbEngine::evaluateWatcher(WatchData *wd)
{
    if (debugCDBWatchHandling)
        qDebug() << Q_FUNC_INFO << wd->exp;
    QString errorMessage;
    QString value;
    QString type;
    QString exp = wd->exp;
    // Remove locals watch prefix.
    if (exp.startsWith(QLatin1String("local.")))
        exp.remove(0, 6);
    if (m_d->evaluateExpression(exp, &value, &type, &errorMessage)) {
        wd->setValue(value);
        wd->setType(type.toUtf8());
    } else {
        wd->setValue(errorMessage);
        wd->setTypeUnneeded();
    }
    wd->setHasChildren(false);
}

void CdbEngine::updateWatchData(const WatchData &incomplete, const WatchUpdateFlags & /* flags */)
{
    // Watch item was edited while running
    if (m_d->isDebuggeeRunning())
        return;

    if (debugCDBWatchHandling)
        qDebug() << Q_FUNC_INFO << "\n    " << incomplete.toString();

    if (incomplete.iname.startsWith("watch.")) {
        WatchData watchData = incomplete;
        watchData.setError(tr("<not supported>"));
        watchHandler()->insertData(watchData);
        return;
    }

    const int frameIndex = stackHandler()->currentIndex();

    bool success = false;
    QString errorMessage;
    do {
        CdbSymbolGroupContext *sg = m_d->m_currentStackTrace->cdbSymbolGroupContextAt(frameIndex, &errorMessage);
        if (!sg)
            break;
        if (!sg->completeData(incomplete, watchHandler(), &errorMessage))
            break;
        success = true;
    } while (false);
    if (!success)
        warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
    if (debugCDBWatchHandling > 1)
        qDebug() << *watchHandler()->model(LocalsWatch);
}

// Continue inferior with a debugger command, such as "p", "pt"
// or its thread variations
bool CdbEnginePrivate::executeContinueCommand(const QString &command)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << command;
    clearForRun();
    updateCodeLevel(); // Step by instruction
    STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyInferiorRunRequested");
    m_engine->notifyInferiorRunRequested();
    m_engine->showMessage(CdbEngine::tr("Continuing with '%1'...").arg(command));
    QString errorMessage;
    const bool success = executeDebuggerCommand(command, &errorMessage);
    if (success) {
        STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyInferiorRunOk");
        m_engine->notifyInferiorRunOk();
        startWatchTimer();
    } else {
        STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyInferiorRunFailed");
        m_engine->notifyInferiorRunFailed();
        m_engine->warning(CdbEngine::tr("Unable to continue: %1").arg(errorMessage));
    }
    return success;
}

static inline QString msgStepFailed(unsigned long executionStatus, int threadId, const QString &why)
{
    if (executionStatus ==  DEBUG_STATUS_STEP_OVER)
        return QString::fromLatin1("Thread %1: Unable to step over: %2").arg(threadId).arg(why);
    return QString::fromLatin1("Thread %1: Unable to step into: %2").arg(threadId).arg(why);
}

// Step out has to be done via executing 'gu'. TODO: Remove once it is
// accessible via normal API for SetExecutionStatus().

enum { CdbExtendedExecutionStatusStepOut = 7452347 };

// Step with  DEBUG_STATUS_STEP_OVER ('p'-command),
// DEBUG_STATUS_STEP_INTO ('t'-trace-command) or
// CdbExtendedExecutionStatusStepOut ("gu"-command)
// its reverse equivalents in the case of single threads.

bool CdbEngine::step(unsigned long executionStatus)
{
    if (debugCDBExecution)
        qDebug() << ">step" << executionStatus << "curr " << m_d->m_currentThreadId << " evt " << m_d->m_eventThreadId;

    // State of reverse stepping as of 10/2009 (Debugging tools 6.11@404):
    // The constants exist, but invoking the calls leads to E_NOINTERFACE.
    // Also there is no CDB command for it.
    if (executionStatus == DEBUG_STATUS_REVERSE_STEP_OVER || executionStatus == DEBUG_STATUS_REVERSE_STEP_INTO) {
        warning(tr("Reverse stepping is not implemented."));
        return false;
    }

    // Do not step the artifical thread created to interrupt the debuggee.
    if (m_d->m_interrupted && m_d->m_currentThreadId == m_d->m_interruptArticifialThreadId) {
        warning(tr("Thread %1 cannot be stepped.").arg(m_d->m_currentThreadId));
        return false;
    }

    // SetExecutionStatus() continues the thread that triggered the
    // stop event (~# p). This can be confusing if the user is looking
    // at the stack trace of another thread and wants to step that one. If that
    // is the case, explicitly tell it to step the current thread using a command.
    const int triggeringEventThread = m_d->m_eventThreadId;
    const bool sameThread = triggeringEventThread == -1
                            || m_d->m_currentThreadId == triggeringEventThread
                            || threadsHandler()->threads().size() == 1;
    m_d->clearForRun(); // clears thread ids
    m_d->updateCodeLevel(); // Step by instruction or source line
    STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyInferiorRunRequested");
    notifyInferiorRunRequested();
    bool success = false;
    if (sameThread && executionStatus != CdbExtendedExecutionStatusStepOut) { // Step event-triggering thread, use fast API
        const HRESULT hr = m_d->interfaces().debugControl->SetExecutionStatus(executionStatus);
        success = SUCCEEDED(hr);
        if (!success)
            warning(msgStepFailed(executionStatus, m_d->m_currentThreadId, CdbCore::msgComFailed("SetExecutionStatus", hr)));
    } else {
        // Need to use a command to explicitly specify the current thread
        QString command;
        QTextStream str(&command);
        str << '~' << m_d->m_currentThreadId << ' ';
        switch (executionStatus) {
        case DEBUG_STATUS_STEP_OVER:
            str << 'p';
            break;
        case DEBUG_STATUS_STEP_INTO:
            str << 't';
            break;
        case CdbExtendedExecutionStatusStepOut:
            str << "gu";
            break;
        }
        showMessage(tr("Stepping %1").arg(command));
        const HRESULT hr = m_d->interfaces().debugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT, command.toLatin1().constData(), DEBUG_EXECUTE_ECHO);
        success = SUCCEEDED(hr);
        if (!success)
            warning(msgStepFailed(executionStatus, m_d->m_currentThreadId, msgDebuggerCommandFailed(command, hr)));
    }
    if (success) {
        // Oddity: Step into will first break at the calling function. Ignore
        if (executionStatus == DEBUG_STATUS_STEP_INTO || executionStatus == DEBUG_STATUS_REVERSE_STEP_INTO)
            m_d->m_breakEventMode = CdbEnginePrivate::BreakEventIgnoreOnce;
        m_d->startWatchTimer();
        STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyInferiorRunOk");
        notifyInferiorRunOk();
    } else {
        STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyInferiorRunFailed");
        notifyInferiorRunFailed();
    }
    if (debugCDBExecution)
        qDebug() << "<step samethread" << sameThread << "succeeded" << success;
    return success;
}

void CdbEngine::executeStep()
{
    step(isReverseDebugging() ? DEBUG_STATUS_REVERSE_STEP_INTO : DEBUG_STATUS_STEP_INTO);
}

void CdbEngine::executeNext()
{
    step(isReverseDebugging() ? DEBUG_STATUS_REVERSE_STEP_OVER : DEBUG_STATUS_STEP_OVER);
}

void CdbEngine::executeStepI()
{
    executeStep(); // Step into by instruction (figured out by step)
}

void CdbEngine::executeNextI()
{
    executeNext(); // Step over by instruction (figured out by step)
}

void CdbEngine::executeStepOut()
{
    if (!isReverseDebugging())
        step(CdbExtendedExecutionStatusStepOut);
}

void CdbEngine::continueInferior()
{
    QString errorMessage;
    if  (!m_d->continueInferior(&errorMessage))
        warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
}

// Continue process without notifications
bool CdbEnginePrivate::continueInferiorProcess(QString *errorMessagePtr /* = 0 */)
{
    if (debugCDBExecution)
        qDebug("continueInferiorProcess");
    const HRESULT hr = interfaces().debugControl->SetExecutionStatus(DEBUG_STATUS_GO);
    if (FAILED(hr)) {
        const QString errorMessage = CdbCore::msgComFailed("SetExecutionStatus", hr);
        if (errorMessagePtr) {
            *errorMessagePtr = errorMessage;
        } else {
            m_engine->warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
        }
        return false;
    }
    return  true;
}

// Continue process with notifications
bool CdbEnginePrivate::continueInferior(QString *errorMessage)
{
    // Check state: Are we running?
    const ULONG ex = executionStatus();
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << "\n    ex=" << ex;

    if (ex == DEBUG_STATUS_GO) {
        m_engine->warning(QLatin1String("continueInferior() called while debuggee is running."));
        return true;
    }
    // Request continue
    STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyInferiorRunRequested");
    m_engine->notifyInferiorRunRequested();
    bool success = false;
    do {
        clearForRun();
        updateCodeLevel();
        killWatchTimer();
        m_engine->resetLocation();
        m_engine->showStatusMessage(CdbEngine::tr("Running requested..."), messageTimeOut);

        if (!continueInferiorProcess(errorMessage))
            break;

        startWatchTimer();
        success = true;
    } while (false);
    if (success) {
        STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyInferiorRunOk");
        m_engine->notifyInferiorRunOk();
    } else {
        STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyInferiorRunFailed");
        m_engine->notifyInferiorRunFailed();
    }
    return true;
}

bool CdbEnginePrivate::interruptInterferiorProcess(QString *errorMessage)
{

    // Interrupt the interferior process without notifications
    // Could use setInterrupt, but that does not work.
    if (debugCDBExecution) {
        qDebug() << "interruptInterferiorProcess  ex=" << executionStatus();
    }
    const bool rc = debugBreakProcess(m_hDebuggeeProcess, errorMessage);
    if (rc)
        m_interrupted = true;
    return rc;
}

void CdbEnginePrivate::slotModulesLoaded()
{
    // Attaching to crashed windows processes: Unless QtCreator is
    // spawned by the debug handler and inherits the handles,
    // the event handling does not work reliably (that is, the crash
    // event is not delivered). In that case, force a break
    if (m_mode == AttachCrashedExternal && m_engine->state() != InferiorStopOk)
        QTimer::singleShot(10, m_engine, SLOT(slotBreakAttachToCrashed()));
}

void CdbEngine::slotBreakAttachToCrashed()
{
    // Force a break when attaching to crashed process (if Creator was not spawned
    // from handler).
    if (state() != InferiorStopOk) {
        showMessage(QLatin1String("Forcing break..."));
        m_d->m_dumper->disable();
        interruptInferior();
    }
}

void CdbEngine::interruptInferior()
{
    if (!m_d->m_hDebuggeeProcess || !m_d->isDebuggeeRunning())
        return;

    QString errorMessage;
    if (!m_d->interruptInterferiorProcess(&errorMessage)) {
        STATE_DEBUG(Q_FUNC_INFO, __LINE__, "notifyInferiorStopFailed");
        notifyInferiorStopFailed();
        warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
    }
}

void CdbEngine::executeRunToLine(const QString &fileName, int lineNumber)
{
    showMessage(tr("Running up to %1:%2...").arg(fileName).arg(lineNumber));
    QString errorMessage;
    CdbCore::BreakPoint tempBreakPoint;
    tempBreakPoint.fileName = fileName;
    tempBreakPoint.lineNumber = lineNumber;
    tempBreakPoint.oneShot = true;
    const bool ok = tempBreakPoint.add(m_d->interfaces().debugControl, &errorMessage)
                    && m_d->continueInferior(&errorMessage);
    if (!ok)
        warning(errorMessage);
}

void CdbEngine::executeRunToFunction(const QString &functionName)
{
    showMessage(tr("Running up to function '%1()'...").arg(functionName));
    QString errorMessage;
    CdbCore::BreakPoint tempBreakPoint;
    tempBreakPoint.funcName = functionName;
    tempBreakPoint.oneShot = true;
    const bool ok = tempBreakPoint.add(m_d->interfaces().debugControl, &errorMessage)
                    && m_d->continueInferior(&errorMessage);
    if (!ok)
        warning(errorMessage);
}

void CdbEngine::setRegisterValue(int regnr, const QString &valueIn)
{
    bool success = false;
    QString errorMessage;
    do {
        const quint64 value = valueIn.toULongLong(&success, 0);
        if (!success) {
            errorMessage = tr("Invalid register value '%1'").arg(valueIn);
            break;
        }
        if (!setRegisterValueU64(m_d->interfaces().debugRegisters, regnr, value, &errorMessage))
            break;
        showMessage(QString::fromLatin1("Setting register %1 to 0x%2...").
                    arg(regnr).arg(value, 0, 16));
        reloadRegisters();
        success =true;
    } while (false);
    if (!success)
        warning(tr("Cannot set register %1 to '%2': %3").
                arg(regnr).arg(valueIn).arg(errorMessage));
}

void CdbEngine::executeJumpToLine(const QString &fileName, int lineNumber)
{
    // 'Jump' to line by manipulating the program counter register 'rip'.
    bool success = false;
    QString errorMessage;
    do {
        const quint64 address = m_d->getSourceLineAddress(fileName, lineNumber, &errorMessage);
        if (address == 0)
            break;

        if (!setRegisterValueU64(m_d->interfaces().debugControl,
                                 m_d->interfaces().debugRegisters,
                                 QLatin1String("rip"), address, &errorMessage))
            break;
        showMessage(QString::fromLatin1("Jumping to %1:%2 (0x%3)...").
                    arg(QDir::toNativeSeparators(fileName)).arg(lineNumber).arg(address, 0, 16));

        StackFrame frame;
        frame.file = fileName;
        frame.line = lineNumber;
        gotoLocation(frame, true);
        success = true;
    } while (false);
    if (!success)
        warning(tr("Cannot jump to %1:%2: %3").arg(fileName).arg(lineNumber).arg(errorMessage));
}

void CdbEngine::assignValueInDebugger(const WatchData *w, const QString &expr, const QVariant &valueV)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << w->iname << expr << valueV;
    const int frameIndex = stackHandler()->currentIndex();
    QString errorMessage;
    bool success = false;
    QApplication::setOverrideCursor(Qt::BusyCursor);
    const QString iname = QLatin1String(w->iname);
    const QString type = QLatin1String(w->type);
    const QString newValue = valueV.toString();
    //: Arguments: New value, name, (type)
    showMessage(tr("Assigning '%1' to '%2' (%3)...").arg(newValue, iname, type), LogMisc);
    do {
        // Value must be scalar
        const QVariant::Type type = valueV.type();
        if (type != QVariant::Double && type != QVariant::Bool
            && type != QVariant::Int && type != QVariant::LongLong
            && type != QVariant::UInt&& type != QVariant::ULongLong) {
            errorMessage = tr("Can assign only scalar values.");
            break;
        }
        // Check the assigneable type
        const bool isInt = isIntType(w->type);
        const bool isFloat = !isInt && isFloatType(w->type);
        const bool isPointer = !isInt && !isFloat && isPointerType(w->type);
        if (!isInt && !isFloat & !isPointer) {
            errorMessage = tr("Cannot assign values of type '%1'. Only POD-types can be assigned.").arg(type);
            break;
        }
        CdbSymbolGroupContext *sg = m_d->getSymbolGroupContext(frameIndex, &errorMessage);
        if (!sg)
            break;
        QString newValueObtained;
        if (!sg->assignValue(w->iname, newValue, &newValueObtained, &errorMessage))
            break;
        // Fix the crappy values returned by the symbol group (0n<Decimal>, etc).
        // Return pointers as hex
        if (isInt || isPointer) {
            const QVariant v = CdbCore::SymbolGroupContext::getIntValue(newValueObtained);
            if (v.isValid())
                newValueObtained = isPointer ?
                                   (QLatin1String("0x") + QString::number(v.toULongLong(), 16)):
                                   v.toString();
        }
        // Update view
        if (const WatchData *fwd = watchHandler()->findItem(w->iname)) {
            WatchData modified = *fwd;
            modified.setValue(newValueObtained);
            watchHandler()->insertData(modified);
            watchHandler()->updateWatchers();
        }
        success = true;
    } while (false);
    QApplication::restoreOverrideCursor();
    if (!success) {
        //: Arguments: New value, name, (type): Error
        const QString msg = tr("Unable to assign the value '%1' to '%2' (%3): %4").
                arg(newValue, expr, type, errorMessage);
        warning(msg);
    }
}

void CdbEngine::executeDebuggerCommand(const QString &command)
{
    QString errorMessage;
    if (!m_d->executeDebuggerCommand(command, &errorMessage))
        warning(errorMessage);
}

void CdbEngine::activateFrame(int frameIndex)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << frameIndex;

    switch (state()) {
    case InferiorStopOk:
    case InferiorShutdownRequested:
        break;
    default:
        qWarning("WARNING %s: invoked in invalid state %s\n",
                 Q_FUNC_INFO, DebuggerEngine::stateName(state()));
        return;
    }

    QString errorMessage;
    bool success = false;
    do {
        const int oldIndex = stackHandler()->currentIndex();
        if (frameIndex >= stackHandler()->stackSize()) {
            errorMessage = msgStackIndexOutOfRange(frameIndex, stackHandler()->stackSize());
            break;
        }

        if (oldIndex != frameIndex)
            stackHandler()->setCurrentIndex(frameIndex);

        const StackFrame &frame = stackHandler()->currentFrame();

        const bool showAssembler = !frame.isUsable();
        if (showAssembler) { // Assembly code: Clean out model and force instruction mode.
            watchHandler()->beginCycle();
            watchHandler()->endCycle();
            QAction *assemblerAction = debuggerCore()->action(OperateByInstruction);
            if (!assemblerAction->isChecked())
                assemblerAction->trigger();
            success = true;
            break;
        }

        gotoLocation(frame, true);

        if (oldIndex != frameIndex || m_d->m_firstActivatedFrame) {
            watchHandler()->beginCycle();
            if (CdbSymbolGroupContext *sgc = m_d->getSymbolGroupContext(frameIndex, &errorMessage))
                success = sgc->populateModelInitially(watchHandler(), &errorMessage);
            watchHandler()->endCycle();
        } else {
            success = true;
        }
    } while (false);

    if (!success) {
        const QString msg = QString::fromLatin1("Internal error: activateFrame() failed for frame #%1 of %2, thread %3: %4").
                            arg(frameIndex).arg(stackHandler()->stackSize()).
                            arg(m_d->m_currentThreadId).arg(errorMessage);
        warning(msg);
    }
    m_d->m_firstActivatedFrame = false;
}

void CdbEngine::selectThread(int index)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << index;

    // Reset location arrow.
    resetLocation();

    threadsHandler()->setCurrentThread(index);
    const int newThreadId = threadsHandler()->threads().at(index).id;
    if (newThreadId != m_d->m_currentThreadId) {
        m_d->m_currentThreadId = threadsHandler()->threads().at(index).id;
        m_d->updateStackTrace();
    }
}

bool CdbEngine::stateAcceptsBreakpointChanges() const
{
    switch (state()) {
    case InferiorRunOk:
    case InferiorStopOk:
    return true;
    default:
        break;
    }
    return false;
}

bool CdbEngine::acceptsBreakpoint(BreakpointId id) const
{
    return DebuggerEngine::isCppBreakpoint(breakHandler()->breakpointData(id));
}

void CdbEngine::attemptBreakpointSynchronization()
{
    if (!m_d->m_hDebuggeeProcess) // Sometimes called from the breakpoint Window
        return;
    QString errorMessage;
    if (!attemptBreakpointSynchronizationI(&errorMessage))
        warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
}

// Figure out what kind of changes are required to synchronize
enum BreakPointSyncType {
    BreakpointsUnchanged, BreakpointsAdded, BreakpointsRemovedChanged
};

static inline BreakPointSyncType breakPointSyncType(const BreakHandler *handler, const BreakpointIds ids)
{
    bool added = false;
    foreach (BreakpointId id, ids) {
        switch (handler->state(id)) {
        case BreakpointInsertRequested:
            added = true;
            break;
        case BreakpointChangeRequested:
        case BreakpointRemoveRequested:
            return BreakpointsRemovedChanged;
        default:
            break;
        }
    }
    return added ? BreakpointsAdded : BreakpointsUnchanged;
}

bool CdbEngine::attemptBreakpointSynchronizationI(QString *errorMessage)
{

    if (!m_d->m_hDebuggeeProcess) {
        *errorMessage = QLatin1String("attemptBreakpointSynchronization() called while debugger is not running");
        return false;
    }
    // Might be called nested while attempting to stop.
    if (m_d->m_breakEventMode == CdbEnginePrivate::BreakEventSyncBreakPoints) {
        *errorMessage = QLatin1String("Nested invocation of attemptBreakpointSynchronization.");
        return false;
    }

    // Check if there is anything to be done at all.
    BreakHandler *handler = breakHandler();
    // Take ownership of the breakpoint. Requests insertion. TODO: Cpp only?
    foreach (BreakpointId id, handler->unclaimedBreakpointIds())
        if (acceptsBreakpoint(id))
            handler->setEngine(id, this);

    // Find out if there is a need to synchronize again
    const BreakpointIds ids = handler->engineBreakpointIds(this);
    const BreakPointSyncType syncType = breakPointSyncType(handler, ids);
    if (debugBreakpoints)
        qDebug("attemptBreakpointSynchronizationI %d breakpoints, syncType=%d", ids.size(), syncType);
    if (syncType == BreakpointsUnchanged)
        return true;

    // This is called from
    // 1) CreateProcessEvent with the halted engine
    // 2) from the break handler, potentially while the debuggee is running
    // If the debuggee is running (for which the execution status is
    // no reliable indicator), we temporarily halt and have ourselves
    // called again from the debug event handler.

    CIDebugControl *control = m_d->interfaces().debugControl;
    CIDebugSymbols *symbols = m_d->interfaces().debugSymbols;

    ULONG dummy;
    const bool wasRunning = !CdbCore::BreakPoint::getBreakPointCount(control, &dummy);
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << "\n  Running=" << wasRunning;

    if (wasRunning) {
        const CdbEnginePrivate::HandleBreakEventMode oldMode = m_d->m_breakEventMode;
        m_d->m_breakEventMode = CdbEnginePrivate::BreakEventSyncBreakPoints;
        if (!m_d->interruptInterferiorProcess(errorMessage)) {
            m_d->m_breakEventMode = oldMode;
            return false;
        }
        return true;
    }

    // If there are changes/removals, delete all breakpoints and re-insert
    // all enabled breakpoints. This is the simplest way to apply changes
    // since CDB ids shift when removing breakpoints and there is no
    // easy way to re-match them.
    if (syncType == BreakpointsRemovedChanged && !deleteCdbBreakpoints(control, errorMessage))
        return false;

    foreach (BreakpointId id, ids) {
        BreakpointResponse response;
        const BreakpointParameters &data = handler->breakpointData(id);
        errorMessage->clear();
        switch (handler->state(id)) {
        case BreakpointInsertRequested:
            handler->notifyBreakpointInsertProceeding(id);
            if (addCdbBreakpoint(control, symbols, data, &response, errorMessage)) {
                handler->notifyBreakpointInsertOk(id);
                handler->setResponse(id, response);
            } else {
                handler->notifyBreakpointInsertOk(id);
                showMessage(*errorMessage, LogError);
            }
            break;
        case BreakpointChangeRequested:
            // Skip disabled breakpoints, else add.
            handler->notifyBreakpointChangeProceeding(id);
            if (data.enabled) {
                if (addCdbBreakpoint(control, symbols, data, &response, errorMessage)) {
                    handler->notifyBreakpointChangeOk(id);
                    handler->setResponse(id, response);
                } else {
                    handler->notifyBreakpointChangeFailed(id);
                    showMessage(*errorMessage, LogError);
                }
            } else {
                handler->notifyBreakpointChangeOk(id);
            }
            break;
        case BreakpointRemoveRequested:
            handler->notifyBreakpointRemoveProceeding(id);
            handler->notifyBreakpointRemoveOk(id);
            break;
        case BreakpointInserted:
            // Existing breakpoints were deleted due to change/removal, re-set
            if (syncType == BreakpointsRemovedChanged
                && !addCdbBreakpoint(control, symbols, handler->breakpointData(id), &response, errorMessage))
                showMessage(*errorMessage, LogError);
            break;
        default:
            break;
        }
    }
    if (debugBreakpoints)
        debugCdbBreakpoints(control);
    return true;
}

void CdbEngine::fetchDisassembler(DisassemblerViewAgent *agent)
{
    enum { ContextLines = 40 };
    QString errorMessage;
    const quint64 address = agent->address();
    if (debugCDB)
        qDebug() << "fetchDisassembler" << address << " Agent: " << agent->address();

    DisassemblerLines lines;
    if (address == 0) { // Clear window
        agent->setContents(lines);
        return;
    }
    QString disassembly;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool ok = disassemble(m_d, address, ContextLines, ContextLines, QTextStream(&disassembly), &errorMessage);
    QApplication::restoreOverrideCursor();
    if (ok) {
        foreach(const QString &line, disassembly.remove(QLatin1Char('\r')).split(QLatin1Char('\n')))
            lines.appendLine(DisassemblerLine(line));
    } else {
        warning(errorMessage);
    }
    agent->setContents(lines);
}

void CdbEngine::fetchMemory(MemoryViewAgent *agent, QObject *token, quint64 addr, quint64 length)
{
    if (!m_d->m_hDebuggeeProcess && !length)
        return;
    ULONG received;
    QByteArray data(length, '\0');
    const HRESULT hr = m_d->interfaces().debugDataSpaces->ReadVirtual(addr, data.data(), length, &received);
    if (FAILED(hr)) {
        warning(tr("Unable to retrieve %1 bytes of memory at 0x%2: %3").
                arg(length).arg(addr, 0, 16).arg(CdbCore::msgComFailed("ReadVirtual", hr)));
        return;
    }
    if (received < length)
        data.truncate(received);
    agent->addLazyData(token, addr, data);
}

void CdbEngine::reloadModules()
{
}

void CdbEngine::loadSymbols(const QString &moduleName)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << moduleName;
}

void CdbEngine::loadAllSymbols()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
}

void CdbEngine::requestModuleSymbols(const QString &moduleName)
{
    Symbols rc;
    QString errorMessage;
    bool success = false;
    do {
        if (m_d->isDebuggeeRunning()) {
            errorMessage = tr("Cannot retrieve symbols while the debuggee is running.");
            break;
        }
        if (!getModuleSymbols(m_d->interfaces().debugSymbols, moduleName, &rc, &errorMessage))
            break;
        success = true;
    } while (false);
    if (!success)
        warning(errorMessage);
    debuggerCore()->showModuleSymbols(moduleName, rc);
}

void CdbEngine::reloadRegisters()
{
    if (state() != InferiorStopOk)
        return;
    const int intBase = 10;
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << intBase;

    QString errorMessage;
    const Registers registers = getRegisters(m_d->interfaces().debugControl,
        m_d->interfaces().debugRegisters, &errorMessage, intBase);
    if (registers.isEmpty() && !errorMessage.isEmpty())
        warning(msgFunctionFailed("reloadRegisters" , errorMessage));
    registerHandler()->setRegisters(registers);
}

void CdbEngine::slotConsoleStubStarted()
{
    const qint64 appPid = m_d->m_consoleStubProc.applicationPID();
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << appPid;
    // Attach to console process.
    QString errorMessage;
    if (startAttachDebugger(appPid, AttachExternal, &errorMessage)) {
        m_d->startWatchTimer();
        notifyInferiorPid(appPid);
    } else {
        QMessageBox::critical(debuggerCore()->mainWindow(), tr("Debugger Error"), errorMessage);
    }
}

void CdbEngine::slotConsoleStubMessage(const QString &msg, bool)
{
    QMessageBox::critical(debuggerCore()->mainWindow(), tr("Debugger Error"), msg);
}

void CdbEngine::slotConsoleStubTerminated()
{
    shutdownEngine();
}

void CdbEngine::warning(const QString &msg)
{
    showMessage(msg, LogWarning);
    qWarning("%s\n", qPrintable(msg));
}

void CdbEnginePrivate::notifyException(long code, bool fatal, const QString &message)
{
    if (debugCDBExecution)
        qDebug() << "notifyException code" << code << " fatal=" << fatal;
    // Suppress the initial breakpoint that occurs when
    // attaching to a console (If a breakpoint is encountered before startup
    // is complete, see startAttachDebugger()).
    switch (code) {
    case winExceptionStartupCompleteTrap:
        m_inferiorStartupComplete = true;
        break;
    case EXCEPTION_BREAKPOINT:
        if (m_ignoreInitialBreakPoint && !m_inferiorStartupComplete && m_breakEventMode == BreakEventHandle) {
            m_engine->showMessage(CdbEngine::tr("Ignoring initial breakpoint..."));
            m_breakEventMode = BreakEventIgnoreOnce;
        }
        break;
    }
    // Cannot go over crash point to execute calls.
    if (fatal) {
        m_dumper->disable();
        m_stoppedReason = StoppedCrash;
        m_stoppedMessage = message;
    }
}

static int threadIndexById(const ThreadsHandler *threadsHandler, int id)
{
    const Threads threads = threadsHandler->threads();
    const int count = threads.count();
    for (int i = 0; i < count; i++)
        if (threads.at(i).id == id)
            return i;
    return -1;
}

void CdbEnginePrivate::handleDebugEvent()
{
    if (debugCDBExecution)
        qDebug() << "handleDebugEvent mode " << m_breakEventMode
                << CdbCore::msgExecutionStatusString(executionStatus()) << " interrupt" << m_interrupted
                << " startupcomplete" << m_inferiorStartupComplete;
    // restore mode and do special handling
    const HandleBreakEventMode mode = m_breakEventMode;
    m_breakEventMode = BreakEventHandle;

    switch (mode) {
    case BreakEventHandle: {
        if (m_interrupted) {
            STATE_DEBUG(Q_FUNC_INFO, __LINE__, "BreakEventHandle / notifyInferiorStopOk");
            m_engine->notifyInferiorStopOk();
        } else {
            STATE_DEBUG(Q_FUNC_INFO, __LINE__, "BreakEventHandle / notifyInferiorSpontaneousStop");
            m_engine->notifyInferiorSpontaneousStop();
        }
        // If this is triggered by breakpoint/crash: Set state to stopping
        // to avoid warnings as opposed to interrupt inferior
        // Indicate artifical thread that is created when interrupting as such,
        // else use stop message with cleaned newlines and blanks.
        const QString currentThreadState =
                m_interrupted ? CdbEngine::tr("<interrupt thread>") :
                (m_stoppedReason == StoppedBreakpoint ? CdbEngine::tr("Breakpoint") :
                                                        m_stoppedMessage.simplified() );
        m_eventThreadId = updateThreadList(currentThreadState);
        m_interruptArticifialThreadId = m_interrupted ? m_eventThreadId : -1;
        // Get thread to stop and its index. If avoidable, do not use
        // the artifical thread that is created when interrupting,
        // use the oldest thread 0 instead.
        ThreadsHandler *threadsHandler = m_engine->threadsHandler();
        m_currentThreadId = m_interrupted ? 0 : m_eventThreadId;
        int currentThreadIndex = -1;
        m_currentThreadId = -1;
        if (m_interrupted) {
            m_currentThreadId = 0;
            currentThreadIndex = threadIndexById(threadsHandler, m_currentThreadId);
        }
        if (!m_interrupted || currentThreadIndex == -1) {
            m_currentThreadId = m_eventThreadId;
            currentThreadIndex = threadIndexById(threadsHandler, m_currentThreadId);
        }
        const QString msg = m_interrupted ?
                            CdbEngine::tr("Interrupted in thread %1, current thread: %2").arg(m_interruptArticifialThreadId).arg(m_currentThreadId) :
                            CdbEngine::tr("Stopped, current thread: %1").arg(m_currentThreadId);
        m_engine->showMessage(msg);
        const int threadIndex = threadIndexById(threadsHandler, m_currentThreadId);
        if (threadIndex != -1)
            threadsHandler->setCurrentThread(threadIndex);
        updateStackTrace();
    }
        break;
    case BreakEventIgnoreOnce:
        startWatchTimer();
        m_interrupted = false;
        break;
    case BreakEventSyncBreakPoints: {
            m_interrupted = false;
            // Temp stop to sync breakpoints (without invoking states).
            // Triggered when the users changes breakpoints while running.
            QString errorMessage;
            m_engine->attemptBreakpointSynchronizationI(&errorMessage);
            startWatchTimer();
            if (!continueInferiorProcess(&errorMessage)) {
                STATE_DEBUG(Q_FUNC_INFO, __LINE__, "BreakEventSyncBreakPoints / notifyInferiorSpontaneousStop");
                m_engine->notifyInferiorSpontaneousStop();
            }
            if (!errorMessage.isEmpty())
                m_engine->warning(QString::fromLatin1("In handleDebugEvent: %1").arg(errorMessage));
    }
        break;
    }
}

void CdbEnginePrivate::setDebuggeeHandles(HANDLE hDebuggeeProcess,  HANDLE hDebuggeeThread)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << hDebuggeeProcess << hDebuggeeThread;
    m_hDebuggeeProcess = hDebuggeeProcess;
    m_hDebuggeeThread = hDebuggeeThread;
}

// Set thread in CDB engine
bool CdbEnginePrivate::setCDBThreadId(unsigned long threadId, QString *errorMessage)
{
    ULONG currentThreadId;
    HRESULT hr = interfaces().debugSystemObjects->GetCurrentThreadId(&currentThreadId);
    if (FAILED(hr)) {
        *errorMessage = CdbCore::msgComFailed("GetCurrentThreadId", hr);
        return false;
    }
    if (currentThreadId == threadId)
        return true;
    hr = interfaces().debugSystemObjects->SetCurrentThreadId(threadId);
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Failed to change to from thread %1 to %2: SetCurrentThreadId() failed: %3").
                        arg(currentThreadId).arg(threadId).arg(CdbCore::msgDebugEngineComResult(hr));
        return false;
    }
    const QString msg = CdbEngine::tr("Changing threads: %1 -> %2").arg(currentThreadId).arg(threadId);
    m_engine->showStatusMessage(msg, 500);
    return true;
}

ULONG CdbEnginePrivate::updateThreadList(const QString &currentThreadState)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << m_hDebuggeeProcess;

    Threads threads;
    ULONG currentThreadId;
    QString errorMessage;
    // When interrupting, an artifical thread with a breakpoint is created.
    const bool stopped = m_engine->state() == InferiorStopOk;
    if (!CdbStackTraceContext::getThreads(interfaces(),
                                          stopped,
                                          &threads, &currentThreadId,
                                          &errorMessage))
        m_engine->warning(errorMessage);
    // Indicate states 'stopped' or current thread state.
    // Do not indicate 'running' since we can't know if it is suspended.
    if (stopped) {
        const QString state = CdbEngine::tr("stopped");
        const bool hasCurrentState = !currentThreadState.isEmpty();
        const int count = threads.size();
        for (int i= 0; i < count; i++) {
            threads[i].state = hasCurrentState && threads.at(i).id == currentThreadId ?
                               currentThreadState : state;
        }
    }
    m_engine->threadsHandler()->setThreads(threads);
    return currentThreadId;
}

// Figure out the thread to run the dumpers in (see notes on.
// CdbDumperHelper). Avoid the artifical threads created by interrupt
// and threads that are in waitFor().
// A stricter version could only use the thread if it is the event
// thread of a step or breakpoint hit (see CdbEnginePrivate::m_interrupted).

static inline unsigned long dumperThreadId(const QList<StackFrame> &frames,
                                           unsigned long currentThread)
{
    if (frames.empty())
        return CdbDumperHelper::InvalidDumperCallThread;
    switch (CdbCore::StackTraceContext::specialFunction(frames.at(0).from, frames.at(0).function)) {
    case CdbCore::StackTraceContext::BreakPointFunction:
    case CdbCore::StackTraceContext::WaitFunction:
        return CdbDumperHelper::InvalidDumperCallThread;
    default:
        break;
    }
    // Check remaining frames for wait
    const int waitCheckDepth = qMin(frames.size(), 5);
    for (int f = 1; f < waitCheckDepth; f++) {
        if (CdbCore::StackTraceContext::specialFunction(frames.at(f).from, frames.at(f).function)
            == CdbCore::StackTraceContext::WaitFunction)
            return CdbDumperHelper::InvalidDumperCallThread;
    }
    return currentThread;
}

// Format stop message with all available information.
QString CdbEnginePrivate::stoppedMessage(const StackFrame *topFrame /*  = 0 */) const
{
    QString msg;
    if (topFrame) {
        if (topFrame->isUsable()) {
            // Stopped at basename:line
            const int lastSlashPos = topFrame->file.lastIndexOf(QLatin1Char('/'));
            const QString file = lastSlashPos == -1 ? topFrame->file : topFrame->file.mid(lastSlashPos + 1);
            msg = CdbEngine::tr("Stopped at %1:%2 in thread %3.").
                  arg(file).arg(topFrame->line).arg(m_currentThreadId);
        } else {
            // Somewhere in assembly code.
            if (topFrame->function.isEmpty()) {
                msg = CdbEngine::tr("Stopped at %1 in thread %2 (missing debug information).").
                      arg(topFrame->address).arg(m_currentThreadId);
            } else {
                msg = CdbEngine::tr("Stopped at %1 (%2) in thread %3 (missing debug information).").
                      arg(topFrame->address).arg(topFrame->function).arg(m_currentThreadId);
            }
        } // isUsable
    } else {
        msg = CdbEngine::tr("Stopped in thread %1 (missing debug information).").arg(m_currentThreadId);

    }
    if (!m_stoppedMessage.isEmpty()) {
        msg += QLatin1Char(' ');
        msg += m_stoppedMessage;
    }
    return msg;
}

void CdbEnginePrivate::updateStackTrace()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
    // Create a new context
    cleanStackTrace();
    QString errorMessage;
    m_engine->reloadRegisters();
    if (!setCDBThreadId(m_currentThreadId, &errorMessage)) {
        m_engine->warning(errorMessage);
        return;
    }
    m_currentStackTrace =
            CdbStackTraceContext::create(m_dumper, &errorMessage);
    if (!m_currentStackTrace) {
        m_engine->warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
        return;
    }
    // Disassembling slows things down a bit. Assembler is still available via menu.
#if 0
    m_engine->reloadDisassembler(); // requires stack trace
#endif
    const QList<StackFrame> stackFrames = m_currentStackTrace->stackFrames();
    // find the first usable frame and select it
    int current = -1;
    const int count = stackFrames.count();
    for (int i=0; i < count; ++i)
        if (stackFrames.at(i).isUsable()) {
            current = i;
            break;
        }
    // Format stop message.
    const QString stopMessage = stoppedMessage(stackFrames.isEmpty() ? static_cast<const StackFrame *>(0) : &stackFrames.front());
    // Set up dumper with a thread (or invalid)
    const unsigned long dumperThread = dumperThreadId(stackFrames, m_currentThreadId);
    if (debugCDBExecution)
        qDebug() << "updateStackTrace() current: " << m_currentThreadId << " dumper=" << dumperThread;
    m_dumper->setDumperCallThread(dumperThread);
    // Display frames
    m_engine->stackHandler()->setFrames(stackFrames);
    m_firstActivatedFrame = true;
    if (current >= 0) {
        m_engine->stackHandler()->setCurrentIndex(current);
        m_engine->activateFrame(current);
    } else {
        // Clean out variables
        m_engine->watchHandler()->beginCycle();
        m_engine->watchHandler()->endCycle();
    }
    m_engine->watchHandler()->updateWatchers();
    // Show message after a lengthy dumper initialization
    m_engine->showMessage(stopMessage, StatusBar, 15000);
}

void CdbEnginePrivate::updateModules()
{
    Modules modules;
    QString errorMessage;
    if (!getModuleList(interfaces().debugSymbols, &modules, &errorMessage))
        m_engine->warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
    m_engine->modulesHandler()->setModules(modules);
}

static const char *dumperPrefixC = "dumper";

void CdbEnginePrivate::handleModuleLoad(quint64 offset, const QString &name)
{
    if (debugCDB>2)
        qDebug() << Q_FUNC_INFO << "\n    " << offset << name;
    Module module;
    // Determine module parameters by offset. The callback has almost all the
    // parameters we need with the exception of 'symbolsRead'. Retrieve the
    // parameters by offset as to avoid a hack like 'check last module'.
    QString errorMessage;
    if (getModuleByOffset(interfaces().debugSymbols, offset, &module, &errorMessage)) {
        m_engine->modulesHandler()->addModule(module);
    } else {
        m_engine->warning(errorMessage);
    }
    m_dumper->moduleLoadHook(name, m_hDebuggeeProcess);
}

void CdbEnginePrivate::handleModuleUnload(const QString &imageName)
{
    m_engine->modulesHandler()->removeModule(imageName);
}

void CdbEnginePrivate::handleBreakpointEvent(PDEBUG_BREAKPOINT2 pBP)
{
    Q_UNUSED(pBP)
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
    m_stoppedReason = StoppedBreakpoint;
    CdbCore::BreakPoint breakpoint;
    // Format message unless it is a temporary step-out breakpoint with empty expression.
    QString expression;
    if (breakpoint.retrieve(pBP, &expression)) {
        expression = breakpoint.expression();
    } else {
        expression.clear();
    }
    if (!expression.isEmpty())
        m_stoppedMessage = breakpoint.type == CdbCore::BreakPoint::Code ?
                           CdbEngine::tr("Breakpoint: %1").arg(expression) :
                           CdbEngine::tr("Watchpoint: %1").arg(expression);
}

void CdbEngine::reloadSourceFiles()
{
}

void CdbEngine::syncDebuggerPaths()
{
     if (debugCDB)
        qDebug() << Q_FUNC_INFO << m_d->m_options->symbolPaths << m_d->m_options->sourcePaths;
    QString errorMessage;
    if (!m_d->setSourcePaths(m_d->m_options->sourcePaths, &errorMessage)
        || !m_d->setSymbolPaths(m_d->m_options->symbolPaths, &errorMessage)) {
        errorMessage = QString::fromLatin1("Unable to set the debugger paths: %1").arg(errorMessage);
        warning(errorMessage);
    }
}

unsigned CdbEngine::debuggerCapabilities() const
{
    return DisassemblerCapability | RegisterCapability | ShowMemoryCapability
           |WatchpointCapability|JumpToLineCapability
           |BreakOnThrowAndCatchCapability; // Sort-of: Can break on throw().
}

// Accessed by RunControlFactory
bool isCdbEngineEnabled()
{
    return theOptionsPage && theOptionsPage->options()->enabled;
}

// Accessed by RunControlFactory
DebuggerEngine *createCdbEngine(const DebuggerStartParameters &sp, QString *errorMessage)
{
    // Create engine
    DebuggerEngine *engine = CdbEngine::create(sp, errorMessage);
    if (engine) {
        QObject::connect(theOptionsPage, SIGNAL(debuggerPathsChanged()), engine, SLOT(syncDebuggerPaths()));
    } else {
        theOptionsPage->setFailureMessage(*errorMessage);
        qWarning("%s\n" ,qPrintable(*errorMessage));
    }
    return engine;
}

void addCdbOptionPages(QList<Core::IOptionsPage *> *opts)
{
    // FIXME: HACK (global variable)
    theOptionsPage = new CdbOptionsPage;
    opts->push_back(theOptionsPage);
}

bool checkCdbConfiguration(int toolChainI, QString *errorMsg, QString *settingsPage)
{
    const ProjectExplorer::ToolChainType toolChain = static_cast<ProjectExplorer::ToolChainType>(toolChainI);
    switch (toolChain) {
    case ProjectExplorer::ToolChain_MinGW: // Do our best
    case ProjectExplorer::ToolChain_MSVC:
    case ProjectExplorer::ToolChain_WINCE:
    case ProjectExplorer::ToolChain_OTHER:
    case ProjectExplorer::ToolChain_UNKNOWN:
    case ProjectExplorer::ToolChain_INVALID:
        break;
    default:
        //: %1 is something like "GCCE" or "Intel C++ Compiler (Linux)" (see ToolChain context)
        *errorMsg = CdbEngine::tr("The CDB debug engine does not support the %1 toolchain.").
                    arg(ProjectExplorer::ToolChain::toolChainName(toolChain));
        *settingsPage = CdbOptionsPage::settingsId();
        return false;
    }
    return true;
}

} // namespace Internal
} // namespace Debugger

