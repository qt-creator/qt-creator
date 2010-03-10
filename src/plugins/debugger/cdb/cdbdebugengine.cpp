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

#include "cdbdebugengine.h"
#include "cdbdebugengine_p.h"
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
#include "debuggeragents.h"
#include "debuggeruiswitcher.h"
#include "debuggermainwindow.h"

#include "debuggeractions.h"
#include "debuggermanager.h"
#include "breakhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "registerhandler.h"
#include "moduleshandler.h"
#include "watchutils.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <utils/winutils.h>
#include <utils/consoleprocess.h>
#include <utils/fancymainwindow.h>
#include <texteditor/itexteditor.h>

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

static const char *localSymbolRootC = "local";

namespace Debugger {
namespace Internal {

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
   return CdbDebugEngine::tr("The function \"%1()\" failed: %2").arg(function, why);
}

// ----- Engine helpers

// --- CdbDebugEnginePrivate

CdbDebugEnginePrivate::CdbDebugEnginePrivate(DebuggerManager *manager,
                                             const QSharedPointer<CdbOptions> &options,
                                             CdbDebugEngine* engine) :
    m_options(options),
    m_hDebuggeeProcess(0),
    m_hDebuggeeThread(0),
    m_breakEventMode(BreakEventHandle),
    m_dumper(new CdbDumperHelper(manager, this)),
    m_currentThreadId(-1),
    m_eventThreadId(-1),
    m_interruptArticifialThreadId(-1),
    m_ignoreInitialBreakPoint(false),
    m_interrupted(false),
    m_engine(engine),
    m_currentStackTrace(0),
    m_firstActivatedFrame(true),
    m_inferiorStartupComplete(false),
    m_mode(AttachCore)
{
    connect(this, SIGNAL(watchTimerDebugEvent()), this, SLOT(handleDebugEvent()));
}

bool CdbDebugEnginePrivate::init(QString *errorMessage)
{
    enum {  bufLen = 10240 };

    if (!CdbCore::CoreEngine::init(m_options->path, errorMessage))
        return false;
    CdbDebugOutput *output = new CdbDebugOutput;
    setDebugOutput(DebugOutputBasePtr(output));
    connect(output, SIGNAL(debuggerOutput(int,QString)),
            manager(), SLOT(showDebuggerOutput(int,QString)));
    connect(output, SIGNAL(debuggerInputPrompt(int,QString)),
            manager(), SLOT(showDebuggerInput(int,QString)));
    connect(output, SIGNAL(debuggeeOutput(QString)),
            manager(), SLOT(showApplicationOutput(QString)));
    connect(output, SIGNAL(debuggeeInputPrompt(QString)),
            manager(), SLOT(showApplicationOutput(QString)));

    setDebugEventCallback(DebugEventCallbackBasePtr(new CdbDebugEventCallback(m_engine)));
    updateCodeLevel();

    return true;
}

IDebuggerEngine *CdbDebugEngine::create(Debugger::DebuggerManager *manager,
                                        const QSharedPointer<CdbOptions> &options,
                                        QString *errorMessage)
{
    CdbDebugEngine *rc = new CdbDebugEngine(manager, options);
    if (rc->m_d->init(errorMessage)) {
        rc->syncDebuggerPaths();
        return rc;
    }
    delete rc;
    return 0;
}

void  CdbDebugEnginePrivate::updateCodeLevel()
{
    const CdbCore::CoreEngine::CodeLevel cl = theDebuggerBoolSetting(OperateByInstruction) ?
                                              CdbCore::CoreEngine::CodeLevelAssembly : CdbCore::CoreEngine::CodeLevelSource;
    setCodeLevel(cl);
}

CdbDebugEnginePrivate::~CdbDebugEnginePrivate()
{
    cleanStackTrace();
}

DebuggerManager *CdbDebugEnginePrivate::manager() const
{
    return m_engine->manager();
}

void CdbDebugEnginePrivate::clearForRun()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    m_breakEventMode = BreakEventHandle;
    m_eventThreadId = m_interruptArticifialThreadId = -1;
    m_interrupted = false;
    cleanStackTrace();
}

void CdbDebugEnginePrivate::cleanStackTrace()
{
    if (m_currentStackTrace) {
        delete m_currentStackTrace;
        m_currentStackTrace = 0;
    }
    m_firstActivatedFrame = false;
    m_editorToolTipCache.clear();
}

CdbDebugEngine::CdbDebugEngine(DebuggerManager *manager, const QSharedPointer<CdbOptions> &options) :
    IDebuggerEngine(manager),
    m_d(new CdbDebugEnginePrivate(manager, options, this))
{
    m_d->m_consoleStubProc.setMode(Utils::ConsoleProcess::Suspend);
    connect(&m_d->m_consoleStubProc, SIGNAL(processError(QString)),
            this, SLOT(slotConsoleStubError(QString)));
    connect(&m_d->m_consoleStubProc, SIGNAL(processStarted()),
            this, SLOT(slotConsoleStubStarted()));
    connect(&m_d->m_consoleStubProc, SIGNAL(wrapperStopped()),
            this, SLOT(slotConsoleStubTerminated()));
}

CdbDebugEngine::~CdbDebugEngine()
{
    delete m_d;
}

void CdbDebugEngine::setState(DebuggerState state, const char *func, int line)
{
    if (debugCDB)
        qDebug() << "setState(" << state << ") at " << func << ':' << line;
    IDebuggerEngine::setState(state);
}

void CdbDebugEngine::shutdown()
{
    exitDebugger();
}

QString CdbDebugEngine::editorToolTip(const QString &exp, const QString &function)
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
            qDebug() << "CdbDebugEngine::editorToolTip" << exp << function << frameIndex;
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

void CdbDebugEngine::setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos)
{
    typedef CdbDebugEnginePrivate::EditorToolTipCache EditorToolTipCache;
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

void CdbDebugEnginePrivate::clearDisplay()
{
    manager()->threadsHandler()->removeAll();
    manager()->modulesHandler()->removeAll();
    manager()->registerHandler()->removeAll();
}

void CdbDebugEnginePrivate::checkVersion()
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
        manager()->showDebuggerOutput(LogMisc, CdbDebugEngine::tr("Version: %1").arg(version));
        if (version.toDouble() <  minVersion) {
            const QString msg = CdbDebugEngine::tr(
                    "<html>The installed version of the <i>Debugging Tools for Windows</i> (%1) "
                    "is rather old. Upgrading to version %2 is recommended "
                    "for the proper display of Qt's data types.</html>").arg(version).arg(minVersion);
            Core::ICore::instance()->showWarningWithOptions(CdbDebugEngine::tr("Debugger"), msg, QString(),
                                                            QLatin1String(Constants::DEBUGGER_SETTINGS_CATEGORY),
                                                            CdbOptionsPage::settingsId());
        }
    }
}

void CdbDebugEngine::startDebugger(const QSharedPointer<DebuggerStartParameters> &sp)
{
    if (debugCDBExecution)
        qDebug() << "startDebugger" << *sp;
    CdbCore::BreakPoint::clearNormalizeFileNameCache();
    setState(AdapterStarting, Q_FUNC_INFO, __LINE__);
    m_d->checkVersion();
    if (m_d->m_hDebuggeeProcess) {
        warning(QLatin1String("Internal error: Attempt to start debugger while another process is being debugged."));
        setState(AdapterStartFailed, Q_FUNC_INFO, __LINE__);
        emit startFailed();
        return;
    }
    switch (sp->startMode) {
    case AttachCore:
    case StartRemote:
        warning(QLatin1String("Internal error: Mode not supported."));
        setState(AdapterStartFailed, Q_FUNC_INFO, __LINE__);
        emit startFailed();
        break;
    default:
        break;
    }
    m_d->m_mode = sp->startMode;
    m_d->clearDisplay();
    m_d->m_inferiorStartupComplete = false;
    setState(AdapterStarted, Q_FUNC_INFO, __LINE__);

    m_d->setVerboseSymbolLoading(m_d->m_options->verboseSymbolLoading);
    // Figure out dumper. @TODO: same in gdb...
    const QString dumperLibName = QDir::toNativeSeparators(manager()->qtDumperLibraryName());
    bool dumperEnabled = m_d->m_mode != AttachCore
                         && m_d->m_mode != AttachCrashedExternal
                         && manager()->qtDumperLibraryEnabled();
    if (dumperEnabled) {
        const QFileInfo fi(dumperLibName);
        if (!fi.isFile()) {
            const QStringList &locations = manager()->qtDumperLibraryLocations();
            const QString loc = locations.join(QLatin1String(", "));
            const QString msg = tr("The dumper library was not found at %1.").arg(loc);
            manager()->showQtDumperLibraryWarning(msg);
            dumperEnabled = false;
        }
    }
    m_d->m_dumper->reset(dumperLibName, dumperEnabled);

    setState(InferiorStarting, Q_FUNC_INFO, __LINE__);
    manager()->showStatusMessage("Starting Debugger", messageTimeOut);

    QString errorMessage;
    bool rc = false;
    bool needWatchTimer = false;
    m_d->clearForRun();
    m_d->updateCodeLevel();
    m_d->m_ignoreInitialBreakPoint = false;
    switch (m_d->m_mode) {
    case AttachExternal:
    case AttachCrashedExternal:
        rc = startAttachDebugger(sp->attachPID, m_d->m_mode, &errorMessage);
        needWatchTimer = true; // Fetch away module load, etc. even if crashed
        break;
    case StartInternal:
    case StartExternal:
        if (sp->useTerminal) {
            // Attaching to console processes triggers an initial breakpoint, which we do not want
            m_d->m_ignoreInitialBreakPoint = true;
            // Launch console stub and wait for its startup
            m_d->m_consoleStubProc.stop(); // We leave the console open, so recycle it now.
            m_d->m_consoleStubProc.setWorkingDirectory(sp->workingDir);
            m_d->m_consoleStubProc.setEnvironment(sp->environment);
            rc = m_d->m_consoleStubProc.start(sp->executable, sp->processArgs);
            if (!rc)
                errorMessage = tr("The console stub process was unable to start '%1'.").arg(sp->executable);
            // continues in slotConsoleStubStarted()...
        } else {
            needWatchTimer = true;
            rc = m_d->startDebuggerWithExecutable(sp->workingDir,
                                                  sp->executable,
                                                  sp->processArgs,
                                                  sp->environment,
                                                  &errorMessage);
        }
        break;
    case AttachCore:
        errorMessage = tr("Attaching to core files is not supported!");
        break;
    }
    if (rc) {
        if (needWatchTimer)
            m_d->startWatchTimer();
            emit startSuccessful();
    } else {
        warning(errorMessage);
        setState(InferiorStartFailed, Q_FUNC_INFO, __LINE__);
        emit startFailed();
    }
}

bool CdbDebugEngine::startAttachDebugger(qint64 pid, DebuggerStartMode sm, QString *errorMessage)
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

void CdbDebugEnginePrivate::processCreatedAttached(ULONG64 processHandle, ULONG64 initialThreadHandle)
{
    m_engine->setState(InferiorRunningRequested, Q_FUNC_INFO, __LINE__);
    setDebuggeeHandles(reinterpret_cast<HANDLE>(processHandle), reinterpret_cast<HANDLE>(initialThreadHandle));
    ULONG currentThreadId;
    if (SUCCEEDED(interfaces().debugSystemObjects->GetThreadIdByHandle(initialThreadHandle, &currentThreadId))) {
        m_currentThreadId = currentThreadId;
    } else {
        m_currentThreadId = 0;
    }
    // Clear any saved breakpoints and set initial breakpoints
    m_engine->executeDebuggerCommand(QLatin1String("bc"));
    if (manager()->breakHandler()->hasPendingBreakpoints()) {
        if (debugCDBExecution)
            qDebug() << "processCreatedAttached: Syncing breakpoints";
        m_engine->attemptBreakpointSynchronization();
    }
    // Attaching to crashed: This handshake (signalling an event) is required for
    // the exception to be delivered to the debugger
    if (m_mode == AttachCrashedExternal) {
        const QString crashParameter = manager()->startParameters()->crashParameter;
        if (!crashParameter.isEmpty()) {
            ULONG64 evtNr = crashParameter.toULongLong();
            const HRESULT hr = interfaces().debugControl->SetNotifyEventHandle(evtNr);
            // Unless QtCreator is spawned by the debugger and inherits the handles,
            // the event handling does not work reliably
            // (that is, the crash event is not delivered).
            if (SUCCEEDED(hr)) {
                QTimer::singleShot(0, m_engine, SLOT(slotBreakAttachToCrashed()));
            } else {
                m_engine->warning(QString::fromLatin1("Handshake failed on event #%1: %2").arg(evtNr).arg(CdbCore::msgComFailed("SetNotifyEventHandle", hr)));
            }
        }
    }
    m_engine->setState(InferiorRunning, Q_FUNC_INFO, __LINE__);
    if (debugCDBExecution)
        qDebug() << "<processCreatedAttached";
}

void CdbDebugEngine::processTerminated(unsigned long exitCode)
{
    manager()->showDebuggerOutput(LogMisc, tr("The process exited with exit code %1.").arg(exitCode));
    if (state() != InferiorStopping)
        setState(InferiorStopping, Q_FUNC_INFO, __LINE__);
    setState(InferiorStopped, Q_FUNC_INFO, __LINE__);
    setState(InferiorShuttingDown, Q_FUNC_INFO, __LINE__);
    m_d->setDebuggeeHandles(0, 0);
    m_d->clearForRun();
    setState(InferiorShutDown, Q_FUNC_INFO, __LINE__);
    // Avoid calls from event handler.
    QTimer::singleShot(0, manager(), SLOT(exitDebugger()));
}

bool CdbDebugEnginePrivate::endInferior(EndInferiorAction action, QString *errorMessage)
{
    // Process must be stopped in order to terminate
    m_engine->setState(InferiorShuttingDown, Q_FUNC_INFO, __LINE__); // pretend it is shutdown
    const bool wasRunning = isDebuggeeRunning();
    if (wasRunning) {
        interruptInterferiorProcess(errorMessage);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    bool success = false;
    switch (action) {
    case DetachInferior:
            if (detachCurrentProcess(errorMessage))
                success = true;
            break;
    case TerminateInferior:
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
            break;
    }
    // Perform cleanup even when failed..no point clinging to the process
    setDebuggeeHandles(0, 0);
    killWatchTimer();
    m_engine->setState(success ? InferiorShutDown : InferiorShutdownFailed, Q_FUNC_INFO, __LINE__);
    return success;
}

// End debugging. Note that this can invoked via user action
// or the processTerminated() event handler, in which case it
// must not kill the process again.
void CdbDebugEnginePrivate::endDebugging(EndDebuggingMode em)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << em;

    const DebuggerState oldState = m_engine->state();
    if (oldState == DebuggerNotReady || m_mode == AttachCore)
        return;
    // Do we need to stop the process?
    QString errorMessage;
    if (oldState != InferiorShutDown && m_hDebuggeeProcess) {
        EndInferiorAction action;
        switch (em) {
        case EndDebuggingAuto:
            action = (m_mode == AttachExternal || m_mode == AttachCrashedExternal) ?
                     DetachInferior : TerminateInferior;
            break;
        case EndDebuggingDetach:
            action = DetachInferior;
            break;
        case EndDebuggingTerminate:
            action = TerminateInferior;
            break;
        }
        if (debugCDB)
            qDebug() << Q_FUNC_INFO << action;
        // Need a stopped debuggee to act
        if (!endInferior(action, &errorMessage)) {
            errorMessage = QString::fromLatin1("Unable to detach from/end the debuggee: %1").arg(errorMessage);
            manager()->showDebuggerOutput(LogError, errorMessage);
        }
        errorMessage.clear();
    }
    // Clean up resources (open files, etc.)
    m_engine->setState(EngineShuttingDown, Q_FUNC_INFO, __LINE__);
    clearForRun();
    const bool endedCleanly = endSession(&errorMessage);
    m_engine->setState(DebuggerNotReady, Q_FUNC_INFO, __LINE__);
    if (!endedCleanly) {
        errorMessage = QString::fromLatin1("There were errors trying to end debugging:\n%1").arg(errorMessage);
        manager()->showDebuggerOutput(LogError, errorMessage);
    }
}

void CdbDebugEngine::exitDebugger()
{
    m_d->endDebugging();
}

void CdbDebugEngine::detachDebugger()
{
    m_d->endDebugging(CdbDebugEnginePrivate::EndDebuggingDetach);
}

CdbSymbolGroupContext *CdbDebugEnginePrivate::getSymbolGroupContext(int frameIndex, QString *errorMessage) const
{
    if (!m_currentStackTrace) {
        *errorMessage = QLatin1String(msgNoStackTraceC);
        return 0;
    }
    if (CdbSymbolGroupContext *sg = m_currentStackTrace->cdbSymbolGroupContextAt(frameIndex, errorMessage))
        return sg;
    return 0;
}

void CdbDebugEngine::evaluateWatcher(WatchData *wd)
{
    if (debugCDBWatchHandling)
        qDebug() << Q_FUNC_INFO << wd->exp;
    QString errorMessage;
    QString value;
    QString type;
    if (m_d->evaluateExpression(wd->exp, &value, &type, &errorMessage)) {
        wd->setValue(value);
        wd->setType(type);
    } else {
        wd->setValue(errorMessage);
        wd->setTypeUnneeded();
    }
    wd->setHasChildren(false);
}

void CdbDebugEngine::updateWatchData(const WatchData &incomplete)
{
    // Watch item was edited while running
    if (m_d->isDebuggeeRunning())
        return;

    if (debugCDBWatchHandling)
        qDebug() << Q_FUNC_INFO << "\n    " << incomplete.toString();

    WatchHandler *watchHandler = manager()->watchHandler();
    if (incomplete.iname.startsWith("watch.")) {
        WatchData watchData = incomplete;
        evaluateWatcher(&watchData);
        watchHandler->insertData(watchData);
        return;
    }

    const int frameIndex = manager()->stackHandler()->currentIndex();

    bool success = false;
    QString errorMessage;
    do {
        CdbSymbolGroupContext *sg = m_d->m_currentStackTrace->cdbSymbolGroupContextAt(frameIndex, &errorMessage);
        if (!sg)
            break;
        if (!sg->completeData(incomplete, watchHandler, &errorMessage))
            break;
        success = true;
    } while (false);
    if (!success)
        warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
    if (debugCDBWatchHandling > 1)
        qDebug() << *manager()->watchHandler()->model(LocalsWatch);
}

// Continue inferior with a debugger command, such as "p", "pt"
// or its thread variations
bool CdbDebugEnginePrivate::executeContinueCommand(const QString &command)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << command;
    clearForRun();
    updateCodeLevel(); // Step by instruction
    m_engine->setState(InferiorRunningRequested, Q_FUNC_INFO, __LINE__);
    manager()->showDebuggerOutput(LogMisc, CdbDebugEngine::tr("Continuing with '%1'...").arg(command));
    QString errorMessage;
    const bool success = executeDebuggerCommand(command, &errorMessage);
    if (success) {
        m_engine->setState(InferiorRunning, Q_FUNC_INFO, __LINE__);
        startWatchTimer();
    } else {
        m_engine->setState(InferiorStopped, Q_FUNC_INFO, __LINE__);
        m_engine->warning(CdbDebugEngine::tr("Unable to continue: %1").arg(errorMessage));
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

bool CdbDebugEngine::step(unsigned long executionStatus)
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
                            || manager()->threadsHandler()->threads().size() == 1;
    m_d->clearForRun(); // clears thread ids
    m_d->updateCodeLevel(); // Step by instruction or source line
    setState(InferiorRunningRequested, Q_FUNC_INFO, __LINE__);
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
        manager()->showDebuggerOutput(tr("Stepping %1").arg(command));
        const HRESULT hr = m_d->interfaces().debugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT, command.toLatin1().constData(), DEBUG_EXECUTE_ECHO);
        success = SUCCEEDED(hr);
        if (!success)
            warning(msgStepFailed(executionStatus, m_d->m_currentThreadId, msgDebuggerCommandFailed(command, hr)));
    }
    if (success) {
        // Oddity: Step into will first break at the calling function. Ignore
        if (executionStatus == DEBUG_STATUS_STEP_INTO || executionStatus == DEBUG_STATUS_REVERSE_STEP_INTO)
            m_d->m_breakEventMode = CdbDebugEnginePrivate::BreakEventIgnoreOnce;
        m_d->startWatchTimer();
        setState(InferiorRunning, Q_FUNC_INFO, __LINE__);
    } else {
        setState(InferiorStopped, Q_FUNC_INFO, __LINE__);
    }
    if (debugCDBExecution)
        qDebug() << "<step samethread" << sameThread << "succeeded" << success;
    return success;
}

void CdbDebugEngine::executeStep()
{
    step(manager()->isReverseDebugging() ? DEBUG_STATUS_REVERSE_STEP_INTO : DEBUG_STATUS_STEP_INTO);
}

void CdbDebugEngine::executeNext()
{
    step(manager()->isReverseDebugging() ? DEBUG_STATUS_REVERSE_STEP_OVER : DEBUG_STATUS_STEP_OVER);
}

void CdbDebugEngine::executeStepI()
{
    executeStep(); // Step into by instruction (figured out by step)
}

void CdbDebugEngine::executeNextI()
{
    executeNext(); // Step over by instruction (figured out by step)
}

void CdbDebugEngine::executeStepOut()
{
    if (!manager()->isReverseDebugging())
        step(CdbExtendedExecutionStatusStepOut);
}

void CdbDebugEngine::continueInferior()
{
    QString errorMessage;
    if  (!m_d->continueInferior(&errorMessage))
        warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
}

// Continue process without notifications
bool CdbDebugEnginePrivate::continueInferiorProcess(QString *errorMessagePtr /* = 0 */)
{
    if (debugCDBExecution)
        qDebug() << "continueInferiorProcess";
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
bool CdbDebugEnginePrivate::continueInferior(QString *errorMessage)
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
    m_engine->setState(InferiorRunningRequested, Q_FUNC_INFO, __LINE__);
    bool success = false;
    do {
        clearForRun();
        updateCodeLevel();
        killWatchTimer();
        manager()->resetLocation();
        manager()->showStatusMessage(CdbDebugEngine::tr("Running requested..."), messageTimeOut);

        if (!continueInferiorProcess(errorMessage))
            break;

        startWatchTimer();
        success = true;
    } while (false);
    if (success) {
        m_engine->setState(InferiorRunning, Q_FUNC_INFO, __LINE__);
    } else {
        m_engine->setState(InferiorStopped, Q_FUNC_INFO, __LINE__); // No RunningRequestFailed?
    }
    return true;
}

bool CdbDebugEnginePrivate::interruptInterferiorProcess(QString *errorMessage)
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

void CdbDebugEngine::slotBreakAttachToCrashed()
{
    // Force a break when attaching to crashed process (if Creator was not spawned
    // from handler).
    if (state() != InferiorStopped) {
        manager()->showDebuggerOutput(LogMisc, QLatin1String("Forcing break..."));
        m_d->m_dumper->disable();
        interruptInferior();
    }
}

void CdbDebugEngine::interruptInferior()
{
    if (!m_d->m_hDebuggeeProcess || !m_d->isDebuggeeRunning())
        return;

    QString errorMessage;
    setState(InferiorStopping, Q_FUNC_INFO, __LINE__);
    if (!m_d->interruptInterferiorProcess(&errorMessage)) {
        setState(InferiorStopFailed, Q_FUNC_INFO, __LINE__);
        warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
    }
}

void CdbDebugEngine::executeRunToLine(const QString &fileName, int lineNumber)
{
    manager()->showDebuggerOutput(LogMisc, tr("Running up to %1:%2...").arg(fileName).arg(lineNumber));
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

void CdbDebugEngine::executeRunToFunction(const QString &functionName)
{
    manager()->showDebuggerOutput(LogMisc, tr("Running up to function '%1()'...").arg(functionName));
    QString errorMessage;
    CdbCore::BreakPoint tempBreakPoint;
    tempBreakPoint.funcName = functionName;
    tempBreakPoint.oneShot = true;
    const bool ok = tempBreakPoint.add(m_d->interfaces().debugControl, &errorMessage)
                    && m_d->continueInferior(&errorMessage);
    if (!ok)
        warning(errorMessage);
}

void CdbDebugEngine::executeJumpToLine(const QString & /* fileName */, int /*lineNumber*/)
{
    warning(tr("Jump to line is not implemented"));
}

void CdbDebugEngine::assignValueInDebugger(const QString &expr, const QString &value)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << expr << value;
    const int frameIndex = manager()->stackHandler()->currentIndex();
    QString errorMessage;
    bool success = false;
    do {
        QString newValue;
        CdbSymbolGroupContext *sg = m_d->getSymbolGroupContext(frameIndex, &errorMessage);
        if (!sg)
            break;
        if (!sg->assignValue(expr, value, &newValue, &errorMessage))
            break;
        // Update view
        WatchHandler *watchHandler = manager()->watchHandler();
        if (WatchData *fwd = watchHandler->findItem(expr.toLatin1())) {
            fwd->setValue(newValue);
            watchHandler->insertData(*fwd);
            watchHandler->updateWatchers();
        }
        success = true;
    } while (false);
    if (!success) {
        const QString msg = tr("Unable to assign the value '%1' to '%2': %3").arg(value, expr, errorMessage);
        warning(msg);
    }
}

void CdbDebugEngine::executeDebuggerCommand(const QString &command)
{
    QString errorMessage;
    if (!m_d->executeDebuggerCommand(command, &errorMessage))
        warning(errorMessage);
}

void CdbDebugEngine::activateFrame(int frameIndex)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << frameIndex;

    if (state() != InferiorStopped) {
        qWarning("WARNING %s: invoked while debuggee is running\n", Q_FUNC_INFO);
        return;
    }

    QString errorMessage;
    bool success = false;
    StackHandler *stackHandler = manager()->stackHandler();
    do {
        WatchHandler *watchHandler = manager()->watchHandler();
        const int oldIndex = stackHandler->currentIndex();
        if (frameIndex >= stackHandler->stackSize()) {
            errorMessage = msgStackIndexOutOfRange(frameIndex, stackHandler->stackSize());
            break;
        }

        if (oldIndex != frameIndex)
            stackHandler->setCurrentIndex(frameIndex);

        const StackFrame &frame = stackHandler->currentFrame();
        if (!frame.isUsable()) {
            // Clean out model
            watchHandler->beginCycle();
            watchHandler->endCycle();
            errorMessage = QString::fromLatin1("%1: file %2 unusable.").
                           arg(QLatin1String(Q_FUNC_INFO), frame.file);
            break;
        }

        manager()->gotoLocation(frame, true);

        if (oldIndex != frameIndex || m_d->m_firstActivatedFrame) {
            watchHandler->beginCycle();
            if (CdbSymbolGroupContext *sgc = m_d->getSymbolGroupContext(frameIndex, &errorMessage))
                success = sgc->populateModelInitially(watchHandler, &errorMessage);
            watchHandler->endCycle();
        } else {
            success = true;
        }
    } while (false);
    if (!success) {
        const QString msg = QString::fromLatin1("Internal error: activateFrame() failed for frame #%1 of %2, thread %3: %4").
                            arg(frameIndex).arg(stackHandler->stackSize()).arg(m_d->m_currentThreadId).arg(errorMessage);
        warning(msg);
    }
    m_d->m_firstActivatedFrame = false;
}

void CdbDebugEngine::selectThread(int index)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << index;

    //reset location arrow
    manager()->resetLocation();

    ThreadsHandler *threadsHandler = manager()->threadsHandler();
    threadsHandler->setCurrentThread(index);
    const int newThreadId = threadsHandler->threads().at(index).id;
    if (newThreadId != m_d->m_currentThreadId) {
        m_d->m_currentThreadId = threadsHandler->threads().at(index).id;
        m_d->updateStackTrace();
    }
}

void CdbDebugEngine::attemptBreakpointSynchronization()
{
    if (!m_d->m_hDebuggeeProcess) // Sometimes called from the breakpoint Window
        return;
    QString errorMessage;
    if (!m_d->attemptBreakpointSynchronization(&errorMessage))
        warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
}

bool CdbDebugEnginePrivate::attemptBreakpointSynchronization(QString *errorMessage)
{
    if (!m_hDebuggeeProcess) {
        *errorMessage = QLatin1String("attemptBreakpointSynchronization() called while debugger is not running");
        return false;
    }
    // This is called from
    // 1) CreateProcessEvent with the halted engine
    // 2) from the break handler, potentially while the debuggee is running
    // If the debuggee is running (for which the execution status is
    // no reliable indicator), we temporarily halt and have ourselves
    // called again from the debug event handler.

    ULONG dummy;
    const bool wasRunning = !CdbCore::BreakPoint::getBreakPointCount(interfaces().debugControl, &dummy);
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << "\n  Running=" << wasRunning;

    if (wasRunning) {
        const HandleBreakEventMode oldMode = m_breakEventMode;
        m_breakEventMode = BreakEventSyncBreakPoints;
        if (!interruptInterferiorProcess(errorMessage)) {
            m_breakEventMode = oldMode;
            return false;
        }
        return true;
    }

    QStringList warnings;
    const bool ok = synchronizeBreakPoints(interfaces().debugControl,
                                           interfaces().debugSymbols,
                                           manager()->breakHandler(),
                                           errorMessage, &warnings);
    if (const int warningsCount = warnings.size())
        for (int w = 0; w < warningsCount; w++)
            m_engine->warning(warnings.at(w));
    return ok;
}

void CdbDebugEngine::fetchDisassembler(DisassemblerViewAgent *agent)
{
    StackFrame frame = agent->frame();
    enum { ContextLines = 40 };
    bool ok = false;
    QString errorMessage;
    do {
        // get address
        QString address;
        if (!frame.file.isEmpty())
            address = frame.address;
        if (address.isEmpty())
            address = agent->address();
        if (debugCDB)
            qDebug() << "fetchDisassembler" << address << " Agent: " << agent->address()
            << " Frame" << frame.file << frame.line << frame.address;
        if (address.isEmpty()) { // Clear window
            agent->setContents(QString());
            ok = true;
            break;
        }
        if (address.startsWith(QLatin1String("0x")))
            address.remove(0, 2);
        const int addressFieldWith = address.size(); // For the Marker

        const ULONG64 offset = address.toULongLong(&ok, 16);
        if (!ok) {
            errorMessage = QString::fromLatin1("Internal error: Invalid address for disassembly: '%1'.").arg(agent->address());
            break;
        }
        QString disassembly;
        QApplication::setOverrideCursor(Qt::WaitCursor);
        ok = dissassemble(m_d, offset, ContextLines, ContextLines, addressFieldWith, QTextStream(&disassembly), &errorMessage);
        QApplication::restoreOverrideCursor();
        if (!ok)
            break;
        agent->setContents(disassembly);

    } while (false);

    if (!ok) {
        agent->setContents(QString());
        warning(errorMessage);
    }
}

void CdbDebugEngine::fetchMemory(MemoryViewAgent *agent, QObject *token, quint64 addr, quint64 length)
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

void CdbDebugEngine::reloadModules()
{
}

void CdbDebugEngine::loadSymbols(const QString &moduleName)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << moduleName;
}

void CdbDebugEngine::loadAllSymbols()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
}

QList<Symbol> CdbDebugEngine::moduleSymbols(const QString &moduleName)
{
    QList<Symbol> rc;
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
    return rc;
}

void CdbDebugEngine::reloadRegisters()
{
    if (state() != InferiorStopped)
        return;
    const int intBase = 10;
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << intBase;

    QString errorMessage;
    const Registers registers = getRegisters(m_d->interfaces().debugControl, m_d->interfaces().debugRegisters, &errorMessage, intBase);
    if (registers.isEmpty() && !errorMessage.isEmpty())
        warning(msgFunctionFailed("reloadRegisters" , errorMessage));
    manager()->registerHandler()->setRegisters(registers);
}

void CdbDebugEngine::slotConsoleStubStarted()
{
    const qint64 appPid = m_d->m_consoleStubProc.applicationPID();
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << appPid;
    // Attach to console process
    QString errorMessage;
    if (startAttachDebugger(appPid, AttachExternal, &errorMessage)) {
        m_d->startWatchTimer();
        manager()->notifyInferiorPidChanged(appPid);
    } else {
        QMessageBox::critical(DebuggerUISwitcher::instance()->mainWindow(), tr("Debugger Error"), errorMessage);
    }
}

void CdbDebugEngine::slotConsoleStubError(const QString &msg)
{
    QMessageBox::critical(DebuggerUISwitcher::instance()->mainWindow(), tr("Debugger Error"), msg);
}

void CdbDebugEngine::slotConsoleStubTerminated()
{
    exitDebugger();
}

void CdbDebugEngine::warning(const QString &w)
{
    manager()->showDebuggerOutput(LogWarning, w);
    qWarning("%s\n", qPrintable(w));
}

void CdbDebugEnginePrivate::notifyException(long code, bool fatal)
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
            manager()->showDebuggerOutput(LogMisc, CdbDebugEngine::tr("Ignoring initial breakpoint..."));
            m_breakEventMode = BreakEventIgnoreOnce;
        }
        break;
    }
    // Cannot go over crash point to execute calls.
    if (fatal)
        m_dumper->disable();
}

static int threadIndexById(const ThreadsHandler *threadsHandler, int id)
{
    const QList<ThreadData> threads = threadsHandler->threads();
    const int count = threads.count();
    for (int i = 0; i < count; i++)
        if (threads.at(i).id == id)
            return i;
    return -1;
}

void CdbDebugEnginePrivate::handleDebugEvent()
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
        // If this is triggered by breakpoint/crash: Set state to stopping
        // to avoid warnings as opposed to interrupt inferior
        if (m_engine->state() != InferiorStopping)
            m_engine->setState(InferiorStopping, Q_FUNC_INFO, __LINE__);
        m_engine->setState(InferiorStopped, Q_FUNC_INFO, __LINE__);
        m_eventThreadId = updateThreadList();
        m_interruptArticifialThreadId = m_interrupted ? m_eventThreadId : -1;
        // Get thread to stop and its index. If avoidable, do not use
        // the artifical thread that is created when interrupting,
        // use the oldest thread 0 instead.
        ThreadsHandler *threadsHandler = manager()->threadsHandler();
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
                            CdbDebugEngine::tr("Interrupted in thread %1, current thread: %2").arg(m_interruptArticifialThreadId).arg(m_currentThreadId) :
                            CdbDebugEngine::tr("Stopped, current thread: %1").arg(m_currentThreadId);
        manager()->showDebuggerOutput(LogMisc, msg);
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
            // Temp stop to sync breakpoints
            QString errorMessage;
            attemptBreakpointSynchronization(&errorMessage);
            startWatchTimer();
            continueInferiorProcess(&errorMessage);
            if (!errorMessage.isEmpty())
                m_engine->warning(QString::fromLatin1("In handleDebugEvent: %1").arg(errorMessage));
    }
        break;
    }
}

void CdbDebugEnginePrivate::setDebuggeeHandles(HANDLE hDebuggeeProcess,  HANDLE hDebuggeeThread)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << hDebuggeeProcess << hDebuggeeThread;
    m_hDebuggeeProcess = hDebuggeeProcess;
    m_hDebuggeeThread = hDebuggeeThread;
}

// Set thread in CDB engine
bool CdbDebugEnginePrivate::setCDBThreadId(unsigned long threadId, QString *errorMessage)
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
    const QString msg = CdbDebugEngine::tr("Changing threads: %1 -> %2").arg(currentThreadId).arg(threadId);
    m_engine->showStatusMessage(msg, 500);
    return true;
}

ULONG CdbDebugEnginePrivate::updateThreadList()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << m_hDebuggeeProcess;

    QList<ThreadData> threads;
    ULONG currentThreadId;
    QString errorMessage;
    // When interrupting, an artifical thread with a breakpoint is created.
    if (!CdbStackTraceContext::getThreads(interfaces(), &threads, &currentThreadId, &errorMessage))
        m_engine->warning(errorMessage);
    manager()->threadsHandler()->setThreads(threads);
    return currentThreadId;
}

// Figure out the thread to run the dumpers in (see notes on.
// CdbDumperHelper). Avoid the artifical threads created by interrupt
// and threads that are in waitFor().
// A stricter version could only use the thread if it is the event
// thread of a step or breakpoint hit (see CdbDebugEnginePrivate::m_interrupted).

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

void CdbDebugEnginePrivate::updateStackTrace()
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
    // Visibly warn the users about missing top frames/all frames, as they otherwise
    // might think stepping is broken.
    if (!stackFrames.at(0).isUsable()) {
        const QString topFunction = count ? stackFrames.at(0).function : QString();
        const QString msg = current >= 0 ?
                            CdbDebugEngine::tr("Thread %1: Missing debug information for top stack frame (%2).").arg(m_currentThreadId).arg(topFunction) :
                            CdbDebugEngine::tr("Thread %1: No debug information available (%2).").arg(m_currentThreadId).arg(topFunction);
        m_engine->warning(msg);
    }
    // Set up dumper with a thread (or invalid)
    const unsigned long dumperThread = dumperThreadId(stackFrames, m_currentThreadId);
    if (debugCDBExecution)
        qDebug() << "updateStackTrace() current: " << m_currentThreadId << " dumper=" << dumperThread;
    m_dumper->setDumperCallThread(dumperThread);
    // Display frames
    manager()->stackHandler()->setFrames(stackFrames);
    m_firstActivatedFrame = true;
    if (current >= 0) {
        manager()->stackHandler()->setCurrentIndex(current);
        m_engine->activateFrame(current);
    } else {
        // Clean out variables
        manager()->watchHandler()->beginCycle();
        manager()->watchHandler()->endCycle();
    }
    manager()->watchHandler()->updateWatchers();
}

void CdbDebugEnginePrivate::updateModules()
{
    QList<Module> modules;
    QString errorMessage;
    if (!getModuleList(interfaces().debugSymbols, &modules, &errorMessage))
        m_engine->warning(msgFunctionFailed(Q_FUNC_INFO, errorMessage));
    manager()->modulesHandler()->setModules(modules);
}

static const char *dumperPrefixC = "dumper";

void CdbDebugEnginePrivate::handleModuleLoad(const QString &name)
{
    if (debugCDB>2)
        qDebug() << Q_FUNC_INFO << "\n    " << name;
    m_dumper->moduleLoadHook(name, m_hDebuggeeProcess);
    updateModules();
}

void CdbDebugEnginePrivate::handleBreakpointEvent(PDEBUG_BREAKPOINT2 pBP)
{
    Q_UNUSED(pBP)
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
}

void CdbDebugEngine::reloadSourceFiles()
{
}

void CdbDebugEngine::syncDebuggerPaths()
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

unsigned CdbDebugEngine::debuggerCapabilities() const
{
    return DisassemblerCapability | RegisterCapability | ShowMemoryCapability;
}

// Accessed by DebuggerManager
IDebuggerEngine *createWinEngine(DebuggerManager *parent,
                                 bool cmdLineEnabled,
                                 QList<Core::IOptionsPage*> *opts)
{
    // Create options page
    QSharedPointer<CdbOptions> options(new CdbOptions);
    options->fromSettings(Core::ICore::instance()->settings());
    CdbOptionsPage *optionsPage = new CdbOptionsPage(options);
    opts->push_back(optionsPage);
    if (!cmdLineEnabled || !options->enabled)
        return 0;
    // Create engine
    QString errorMessage;
    IDebuggerEngine *engine = CdbDebugEngine::create(parent, options, &errorMessage);
    if (!engine) {
        optionsPage->setFailureMessage(errorMessage);
        qWarning("%s\n" ,qPrintable(errorMessage));
    }
    QObject::connect(optionsPage, SIGNAL(debuggerPathsChanged()), engine, SLOT(syncDebuggerPaths()));
    return engine;
}

} // namespace Internal
} // namespace Debugger

