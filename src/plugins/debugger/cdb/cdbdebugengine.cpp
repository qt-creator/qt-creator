/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cdbdebugengine.h"
#include "cdbdebugengine_p.h"
#include "cdbsymbolgroupcontext.h"
#include "cdbstacktracecontext.h"

#include "debuggermanager.h"
#include "breakhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"

#include <utils/qtcassert.h>
#include <utils/winutils.h>
#include <utils/consoleprocess.h>

#include <QtCore/QDebug>
#include <QtCore/QTimerEvent>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QLibrary>
#include <QtCore/QCoreApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>

#define DBGHELP_TRANSLATE_TCHAR
#include <inc/Dbghelp.h>

static const char *dbgEngineDllC = "dbgeng";
static const char *debugCreateFuncC = "DebugCreate";

static const char *localSymbolRootC = "local";

namespace Debugger {
namespace Internal {

QString msgDebugEngineComResult(HRESULT hr)
{
    switch (hr) {
        case S_OK:
        return QLatin1String("S_OK");
        case S_FALSE:
        return QLatin1String("S_FALSE");
        case E_FAIL:
        break;
        case E_INVALIDARG:
        return QLatin1String("E_INVALIDARG");
        case E_NOINTERFACE:
        return QLatin1String("E_NOINTERFACE");
        case E_OUTOFMEMORY:
        return QLatin1String("E_OUTOFMEMORY");
        case E_UNEXPECTED:
        return QLatin1String("E_UNEXPECTED");
        case E_NOTIMPL:
        return QLatin1String("E_NOTIMPL");
    }
    if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
        return QLatin1String("ERROR_ACCESS_DENIED");;
    if (hr == HRESULT_FROM_NT(STATUS_CONTROL_C_EXIT))
        return QLatin1String("STATUS_CONTROL_C_EXIT");
    return Core::Utils::winErrorMessage(HRESULT_CODE(hr));
}

static QString msgStackIndexOutOfRange(int idx, int size)
{
    return QString::fromLatin1("Frame index %1 out of range (%2).").arg(idx).arg(size);
}

QString msgComFailed(const char *func, HRESULT hr)
{
    return QString::fromLatin1("%1 failed: %2").arg(QLatin1String(func), msgDebugEngineComResult(hr));
}

static const char *msgNoStackTraceC = "Internal error: no stack trace present.";

DebuggerEngineLibrary::DebuggerEngineLibrary() :
    m_debugCreate(0)
{
}

bool DebuggerEngineLibrary::init(QString *errorMessage)
{
    // Load
    QLibrary lib(QLatin1String(dbgEngineDllC), 0);

    if (!lib.isLoaded() && !lib.load()) {
        *errorMessage = CdbDebugEngine::tr("Unable to load the debugger engine library '%1': %2").
                        arg(QLatin1String(dbgEngineDllC), lib.errorString());
        return false;
    }
    // Locate symbols
    void *createFunc = lib.resolve(debugCreateFuncC);
    if (!createFunc) {
        *errorMessage = CdbDebugEngine::tr("Unable to resolve '%1' in the debugger engine library '%2'").
                        arg(QLatin1String(debugCreateFuncC), QLatin1String(dbgEngineDllC));
        return false;
    }
    m_debugCreate = static_cast<DebugCreateFunction>(createFunc);
    return true;
}

// --- CdbDebugEnginePrivate

CdbDebugEnginePrivate::CdbDebugEnginePrivate(DebuggerManager *parent, CdbDebugEngine* engine) :
    m_hDebuggeeProcess(0),
    m_hDebuggeeThread(0),
    m_bIgnoreNextDebugEvent(false),
    m_watchTimer(-1),
    m_debugEventCallBack(engine),
    m_debugOutputCallBack(engine),    
    m_pDebugClient(0),
    m_pDebugControl(0),
    m_pDebugSystemObjects(0),
    m_pDebugSymbols(0),
    m_pDebugRegisters(0),
    m_engine(engine),
    m_debuggerManager(parent),
    m_debuggerManagerAccess(parent->engineInterface()),
    m_currentStackTrace(0),
    m_firstActivatedFrame(true),
    m_mode(AttachCore)
{   
}

bool CdbDebugEnginePrivate::init(QString *errorMessage)
{
    // Load the DLL
    DebuggerEngineLibrary lib;
    if (!lib.init(errorMessage))
        return false;

    // Initialize the COM interfaces
    HRESULT hr;
    hr = lib.debugCreate( __uuidof(IDebugClient5), reinterpret_cast<void**>(&m_pDebugClient));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Creation of IDebugClient5 failed: %1").arg(msgDebugEngineComResult(hr));
        return false;
    }

    m_pDebugClient->SetOutputCallbacks(&m_debugOutputCallBack);
    m_pDebugClient->SetEventCallbacks(&m_debugEventCallBack);

    hr = lib.debugCreate( __uuidof(IDebugControl4), reinterpret_cast<void**>(&m_pDebugControl));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Creation of IDebugControl4 failed: %1").arg(msgDebugEngineComResult(hr));
        return false;
    }

    m_pDebugControl->SetCodeLevel(DEBUG_LEVEL_SOURCE);

    hr = lib.debugCreate( __uuidof(IDebugSystemObjects4), reinterpret_cast<void**>(&m_pDebugSystemObjects));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Creation of IDebugSystemObjects4 failed: %1").arg(msgDebugEngineComResult(hr));
        return false;
    }

    hr = lib.debugCreate( __uuidof(IDebugSymbols3), reinterpret_cast<void**>(&m_pDebugSymbols));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Creation of IDebugSymbols3 failed: %1").arg(msgDebugEngineComResult(hr));
        return false;
    }

    hr = lib.debugCreate( __uuidof(IDebugRegisters2), reinterpret_cast<void**>(&m_pDebugRegisters));
    if (FAILED(hr)) {
        *errorMessage = QString::fromLatin1("Creation of IDebugRegisters2 failed: %1").arg(msgDebugEngineComResult(hr));
        return false;    
    }

    return true;
}

IDebuggerEngine *CdbDebugEngine::create(DebuggerManager *parent)
{
    QString errorMessage;
    IDebuggerEngine *rc = 0;
    CdbDebugEngine *e = new CdbDebugEngine(parent);
    if (e->m_d->init(&errorMessage)) {
        rc = e;
    } else {
        delete e;
        qWarning("%s", qPrintable(errorMessage));
    }
    return rc;
}

CdbDebugEnginePrivate::~CdbDebugEnginePrivate()
{
    cleanStackTrace();
    if (m_pDebugClient)
        m_pDebugClient->Release();
    if (m_pDebugControl)
        m_pDebugControl->Release();
    if (m_pDebugSystemObjects)
        m_pDebugSystemObjects->Release();
    if (m_pDebugSymbols)
        m_pDebugSymbols->Release();
    if (m_pDebugRegisters)
        m_pDebugRegisters->Release();
}

void CdbDebugEnginePrivate::cleanStackTrace()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    if (m_currentStackTrace) {
        delete m_currentStackTrace;
        m_currentStackTrace = 0;
    }
}

CdbDebugEngine::CdbDebugEngine(DebuggerManager *parent) :
    IDebuggerEngine(parent),
    m_d(new CdbDebugEnginePrivate(parent, this))
{
    // m_d->m_consoleStubProc.setDebug(true);
    connect(&m_d->m_consoleStubProc, SIGNAL(processError(QString)), this, SLOT(slotConsoleStubError(QString)));
    connect(&m_d->m_consoleStubProc, SIGNAL(processStarted()), this, SLOT(slotConsoleStubStarted()));
    connect(&m_d->m_consoleStubProc, SIGNAL(wrapperStopped()), this, SLOT(slotConsoleStubTerminated()));
}

CdbDebugEngine::~CdbDebugEngine()
{
    delete m_d;
}

void CdbDebugEngine::startWatchTimer()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    if (m_d->m_watchTimer == -1)
        m_d->m_watchTimer = startTimer(0);
}

void CdbDebugEngine::killWatchTimer()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    if (m_d->m_watchTimer != -1) {
        killTimer(m_d->m_watchTimer);
        m_d->m_watchTimer = -1;
    }
}

void CdbDebugEngine::shutdown()
{
    exitDebugger();
}

void CdbDebugEngine::setToolTipExpression(const QPoint & /*pos*/, const QString & /*exp*/)
{
}

bool CdbDebugEngine::startDebugger()
{    
    m_d->m_debuggerManager->showStatusMessage("Starting Debugger", -1);
    QString errorMessage;
    bool rc = false;
    m_d->m_bIgnoreNextDebugEvent = false;
    const DebuggerStartMode mode = m_d->m_debuggerManager->startMode();
    switch (mode) {
    case AttachExternal:
        rc = startAttachDebugger(m_d->m_debuggerManager->m_attachedPID, &errorMessage);
        break;
    case StartInternal:
    case StartExternal:
        if (m_d->m_debuggerManager->m_useTerminal) {
            // Launch console stub and wait for its startup
            m_d->m_consoleStubProc.stop(); // We leave the console open, so recycle it now.
            m_d->m_consoleStubProc.setWorkingDirectory(m_d->m_debuggerManager->m_workingDir);
            m_d->m_consoleStubProc.setEnvironment(m_d->m_debuggerManager->m_environment);
            rc = m_d->m_consoleStubProc.start(m_d->m_debuggerManager->m_executable, m_d->m_debuggerManager->m_processArgs);
            if (!rc)
                errorMessage = tr("The console stub process was unable to start '%1'.").arg(m_d->m_debuggerManager->m_executable);
        } else {
            rc = startDebuggerWithExecutable(mode, &errorMessage);
        }
        break;
    case AttachCore:
        errorMessage = tr("CdbDebugEngine: Attach to core not supported!");
        break;
    }
    if (rc) {
        m_d->m_debuggerManager->showStatusMessage(tr("Debugger Running"), -1);
        startWatchTimer();
    } else {
        qWarning("%s\n", qPrintable(errorMessage));
    }
    return rc;
}

bool CdbDebugEngine::startAttachDebugger(qint64 pid, QString *errorMessage)
{
    // Need to aatrach invasively, otherwise, no notification signals
    // for for CreateProcess/ExitProcess occur.
    const HRESULT hr = m_d->m_pDebugClient->AttachProcess(NULL, pid,
                                                          DEBUG_ATTACH_INVASIVE_RESUME_PROCESS);
    if (debugCDB)
        qDebug() << "Attaching to " << pid << " returns " << hr;
    if (FAILED(hr)) {
        *errorMessage = tr("AttachProcess failed for pid %1: %2").arg(pid).arg(msgDebugEngineComResult(hr));
        return false;
    } else {
        m_d->m_mode = AttachExternal;
    }
    return true;
}

bool CdbDebugEngine::startDebuggerWithExecutable(DebuggerStartMode sm, QString *errorMessage)
{
    m_d->m_debuggerManager->showStatusMessage("Starting Debugger", -1);

    DEBUG_CREATE_PROCESS_OPTIONS dbgopts;
    memset(&dbgopts, 0, sizeof(dbgopts));
    dbgopts.CreateFlags = DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS;

    const QString filename(m_d->m_debuggerManager->m_executable);
    if (debugCDB)
        qDebug() << Q_FUNC_INFO <<filename;

    const QFileInfo fi(filename);
    m_d->m_pDebugSymbols->AppendImagePathWide(QDir::toNativeSeparators(fi.absolutePath()).utf16());
    //m_pDebugSymbols->SetSymbolOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_DEBUG | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_AUTO_PUBLICS);
    m_d->m_pDebugSymbols->SetSymbolOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_AUTO_PUBLICS);
    //m_pDebugSymbols->AddSymbolOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_DEBUG | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_AUTO_PUBLICS | SYMOPT_NO_IMAGE_SEARCH);

    // TODO console
    const QString cmd = Core::Utils::AbstractProcess::createWinCommandline(filename, m_d->m_debuggerManager->m_processArgs);
    if (debugCDB)
        qDebug() << "Starting " << cmd;
    PCWSTR env = 0;
    QByteArray envData;
    if (!m_d->m_debuggerManager->m_environment.empty()) {
        envData = Core::Utils::AbstractProcess::createWinEnvironment(Core::Utils::AbstractProcess::fixWinEnvironment(m_d->m_debuggerManager->m_environment));
        env = reinterpret_cast<PCWSTR>(envData.data());
    }
    const HRESULT hr = m_d->m_pDebugClient->CreateProcess2Wide(NULL,
                                                               const_cast<PWSTR>(cmd.utf16()),
                                                               &dbgopts,
                                                               sizeof(dbgopts),
                                                               m_d->m_debuggerManager->m_workingDir.utf16(),
                                                               env);
    if (FAILED(hr)) {
        *errorMessage = tr("CreateProcess2Wide failed for '%1': %2").arg(cmd, msgDebugEngineComResult(hr));
        m_d->m_debuggerManagerAccess->notifyInferiorExited();
        return false;
    } else {
        m_d->m_mode = sm;
    }
    m_d->m_debuggerManagerAccess->notifyInferiorRunning();
    return true;
}

void CdbDebugEngine::processTerminated(unsigned long exitCode)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << exitCode;

    m_d->cleanStackTrace();
    m_d->setDebuggeeHandles(0, 0);
    m_d->m_debuggerManagerAccess->notifyInferiorExited();
    m_d->m_debuggerManager->exitDebugger();
}

void CdbDebugEngine::exitDebugger()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    if (m_d->m_hDebuggeeProcess) {
        m_d->cleanStackTrace();
        // Terminate or detach if we are running
        HRESULT hr;
        switch (m_d->m_mode) {
        case AttachExternal:            
            if (m_d->isDebuggeeRunning()) { // Process must be stopped in order to detach
                DebugBreakProcess(m_d->m_hDebuggeeProcess);
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            }
            hr = m_d->m_pDebugClient->DetachCurrentProcess();
            if (debugCDB)
                qDebug() << Q_FUNC_INFO << "detached" << msgDebugEngineComResult(hr);
            break;
        case StartExternal:
        case StartInternal:            
            // Terminate and waitr for stop events.
            hr = m_d->m_pDebugClient->TerminateCurrentProcess();
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            if (debugCDB)
                qDebug() << Q_FUNC_INFO << "terminated" << msgDebugEngineComResult(hr);

            break;
        case AttachCore:
            break;
        }
        m_d->setDebuggeeHandles(0, 0);
    }

    killWatchTimer();
}

CdbSymbolGroupContext *CdbDebugEnginePrivate::getStackFrameSymbolGroupContext(int frameIndex, QString *errorMessage) const
{
    if (!m_currentStackTrace) {
        *errorMessage = QLatin1String(msgNoStackTraceC);
        return 0;
    }
    if (CdbSymbolGroupContext *sg = m_currentStackTrace->symbolGroupContextAt(frameIndex, errorMessage))
        return sg;
    return 0;
}

bool CdbDebugEnginePrivate::updateLocals(int frameIndex,
                                         WatchHandler *wh,
                                         QString *errorMessage)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << frameIndex;
    wh->cleanup();

    bool success = false;
    if (CdbSymbolGroupContext *sgc = getStackFrameSymbolGroupContext(frameIndex, errorMessage))
        success = CdbSymbolGroupContext::populateModelInitially(sgc, wh, errorMessage);

    wh->rebuildModel();
    return success;
}

void CdbDebugEngine::updateWatchModel()
{
    const int frameIndex = m_d->m_debuggerManagerAccess->stackHandler()->currentIndex();
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << "fi=" << frameIndex;

    bool success = false;
    QString errorMessage;    
    if (CdbSymbolGroupContext *sg = m_d->m_currentStackTrace->symbolGroupContextAt(frameIndex, &errorMessage))
        success = CdbSymbolGroupContext::completeModel(sg, m_d->m_debuggerManagerAccess->watchHandler(), &errorMessage);

    if (!success)
        qWarning("%s : %s", Q_FUNC_INFO, qPrintable(errorMessage));
}

void CdbDebugEngine::stepExec()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    //m_pDebugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT, "p", 0);
    m_d->cleanStackTrace();
    const HRESULT hr = m_d->m_pDebugControl->SetExecutionStatus(DEBUG_STATUS_STEP_INTO);
    Q_UNUSED(hr)
    m_d->m_bIgnoreNextDebugEvent = true;
    startWatchTimer();
}

void CdbDebugEngine::stepOutExec()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    StackHandler* sh = m_d->m_debuggerManagerAccess->stackHandler();
    const int idx = sh->currentIndex() + 1;
    QList<StackFrame> stackframes = sh->frames();
    if (idx < 0 || idx >= stackframes.size()) {
        qWarning("cannot step out");
        return;
    }

    const StackFrame& frame = stackframes.at(idx);
    bool ok;
    ULONG64 address = frame.address.toULongLong(&ok, 16);
    if (!ok) {
        qWarning("stepOutExec: cannot obtain address from stack frame");
        return;
    }

    IDebugBreakpoint2* pBP;
    HRESULT hr = m_d->m_pDebugControl->AddBreakpoint2(DEBUG_BREAKPOINT_CODE, DEBUG_ANY_ID, &pBP);
    if (FAILED(hr) || !pBP) {
        qWarning("stepOutExec: cannot create temporary breakpoint");
        return;
    }

    pBP->SetOffset(address);

    //QString str = '`' + frame.file + ':' + frame.line + '`';
    //hr = pBP->SetOffsetExpressionWide(str.utf16());
    //if (FAILED(hr)) {
    //    qWarning("SetOffsetExpressionWide failed");
    //    return;
    //}

    pBP->AddFlags(DEBUG_BREAKPOINT_ENABLED);
    pBP->AddFlags(DEBUG_BREAKPOINT_ONE_SHOT);
    //hr = m_pDebugControl->SetExecutionStatus(DEBUG_STATUS_GO);
    continueInferior();
}

void CdbDebugEngine::nextExec()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    m_d->cleanStackTrace();
    const HRESULT hr = m_d->m_pDebugControl->SetExecutionStatus(DEBUG_STATUS_STEP_OVER);
    if (SUCCEEDED(hr)) {
        startWatchTimer();
    } else {
        qWarning("%s failed: %s", Q_FUNC_INFO, qPrintable(msgDebugEngineComResult(hr)));
    }
}

void CdbDebugEngine::stepIExec()
{
    qWarning("CdbDebugEngine::stepIExec() not implemented");
}

void CdbDebugEngine::nextIExec()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    m_d->cleanStackTrace();
    const HRESULT hr = m_d->m_pDebugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT, "p", 0);
    if (SUCCEEDED(hr)) {
        startWatchTimer();
    } else {
        qWarning("%s failed: %s", Q_FUNC_INFO, qPrintable(msgDebugEngineComResult(hr)));
    }
}

void CdbDebugEngine::continueInferior()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    m_d->cleanStackTrace();
    killWatchTimer();
    m_d->m_debuggerManager->resetLocation();

    ULONG executionStatus;
    m_d->m_debuggerManagerAccess->notifyInferiorRunningRequested();
    HRESULT hr = m_d->m_pDebugControl->GetExecutionStatus(&executionStatus);
    if (SUCCEEDED(hr) && executionStatus != DEBUG_STATUS_GO) {
        hr = m_d->m_pDebugControl->SetExecutionStatus(DEBUG_STATUS_GO);
        if (SUCCEEDED(hr)) {
            startWatchTimer();
            m_d->m_debuggerManagerAccess->notifyInferiorRunning();
        } else {
            qWarning("%s failed: %s", Q_FUNC_INFO, qPrintable(msgDebugEngineComResult(hr)));
        }
    }
}

void CdbDebugEngine::interruptInferior()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << m_d->m_hDebuggeeProcess;

    //TODO: better use IDebugControl::SetInterrupt?
    if (!m_d->m_hDebuggeeProcess)
        return;
    if (DebugBreakProcess(m_d->m_hDebuggeeProcess)) {
        m_d->m_debuggerManagerAccess->notifyInferiorStopped();
    } else {
        qWarning("DebugBreakProcess failed: %s", Core::Utils::winErrorMessage(GetLastError()));
    }
}

void CdbDebugEngine::runToLineExec(const QString &fileName, int lineNumber)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << fileName << lineNumber;
}

void CdbDebugEngine::runToFunctionExec(const QString &functionName)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << functionName;
}

void CdbDebugEngine::jumpToLineExec(const QString &fileName, int lineNumber)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << fileName << lineNumber;
}

void CdbDebugEngine::assignValueInDebugger(const QString &expr, const QString &value)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << expr << value;
}

void CdbDebugEngine::executeDebuggerCommand(const QString &command)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << command;
}

void CdbDebugEngine::activateFrame(int frameIndex)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << frameIndex;

    if (m_d->m_debuggerManager->status() != DebuggerInferiorStopped) {
        qWarning("WARNING %s: invoked while debuggee is running\n", Q_FUNC_INFO);
        return;
    }

    QString errorMessage;
    bool success = false;
    do {
        StackHandler *stackHandler = m_d->m_debuggerManagerAccess->stackHandler();
        WatchHandler *watchHandler = m_d->m_debuggerManagerAccess->watchHandler();
        const int oldIndex = stackHandler->currentIndex();
        if (frameIndex >= stackHandler->stackSize()) {
            errorMessage = msgStackIndexOutOfRange(frameIndex, stackHandler->stackSize());
            break;
        }

        if (oldIndex != frameIndex)
            stackHandler->setCurrentIndex(frameIndex);
        if (oldIndex != frameIndex || m_d->m_firstActivatedFrame)
            if (!m_d->updateLocals(frameIndex, watchHandler, &errorMessage))
                break;

        const StackFrame &frame = stackHandler->currentFrame();
        if (!frame.isUsable()) {
            errorMessage = QString::fromLatin1("%1: file %2 unusable.").
                           arg(QLatin1String(Q_FUNC_INFO), frame.file);
            break;
        }
        m_d->m_debuggerManager->gotoLocation(frame.file, frame.line, true);
        success =true;
    } while (false);
    if (!success)
        qWarning("%s", qPrintable(errorMessage));
    m_d->m_firstActivatedFrame = false;
}

void CdbDebugEngine::selectThread(int index)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << index;

    //reset location arrow
    m_d->m_debuggerManager->resetLocation();

    ThreadsHandler *threadsHandler = m_d->m_debuggerManagerAccess->threadsHandler();
    threadsHandler->setCurrentThread(index);
    m_d->m_currentThreadId = index;
    m_d->updateStackTrace();
}

static inline QString breakPointExpression(const QString &fileName, const QString &lineNumber)
{
    QString str;
    str += QLatin1Char('`');
    str += QDir::toNativeSeparators(fileName);
    str += QLatin1Char(':');
    str += lineNumber;
    str += QLatin1Char('`');
    return str;
}

void CdbDebugEngine::attemptBreakpointSynchronization()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    if (!m_d->m_hDebuggeeProcess) {
        qWarning("attemptBreakpointSynchronization() called while debugger is not running");
        return;
    }

    BreakHandler *handler = m_d->m_debuggerManagerAccess->breakHandler();
    for (int i=0; i < handler->size(); ++i) {
        BreakpointData* breakpoint = handler->at(i);
        if (breakpoint->pending) {
            const QString expr = breakPointExpression(breakpoint->fileName,  breakpoint->lineNumber);
            IDebugBreakpoint2* pBP = 0;
            HRESULT hr = m_d->m_pDebugControl->AddBreakpoint2(DEBUG_BREAKPOINT_CODE, DEBUG_ANY_ID, &pBP);
            if (FAILED(hr) || !pBP) {
                qWarning("m_pDebugControl->AddBreakpoint2 %s failed.", qPrintable(expr));
                continue;
            }

            hr = pBP->SetOffsetExpressionWide(expr.utf16());
            if (FAILED(hr)) {
                qWarning("SetOffsetExpressionWide %s failed", qPrintable(expr));
                continue;
            }

            bool ok;
            ULONG ul;
            ul = breakpoint->ignoreCount.toULong(&ok);
            if (ok) pBP->SetPassCount(ul);

            //TODO: handle breakpoint->condition

            pBP->AddFlags(DEBUG_BREAKPOINT_ENABLED);
            //pBP->AddFlags(DEBUG_BREAKPOINT_GO_ONLY);
            breakpoint->pending = false;
        }
    }
}

void CdbDebugEngine::loadSessionData()
{
}

void CdbDebugEngine::saveSessionData()
{
}

void CdbDebugEngine::reloadDisassembler()
{
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

void CdbDebugEngine::reloadRegisters()
{
}

void CdbDebugEngine::timerEvent(QTimerEvent* te)
{
    if (te->timerId() != m_d->m_watchTimer)
        return;

    const HRESULT hr = m_d->m_pDebugControl->WaitForEvent(0, 1);
    if (debugCDB)
        if (debugCDB > 1 || hr != S_FALSE)
            qDebug() << Q_FUNC_INFO << "WaitForEvent" << m_d->m_debuggerManager->status() <<   msgDebugEngineComResult(hr);

    switch (hr) {
        case S_OK:
            killWatchTimer();
            m_d->handleDebugEvent();
            break;
        case S_FALSE:
        case E_PENDING:
        case E_FAIL:
            break;
        case E_UNEXPECTED: // Occurs on ExitProcess.
            killWatchTimer();
            break;        
    }
}

void CdbDebugEngine::slotConsoleStubStarted()
{
    const qint64 appPid = m_d->m_consoleStubProc.applicationPID();
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << appPid;
    // Attach to console process
    QString errorMessage;
    if (startAttachDebugger(appPid, &errorMessage)) {
        m_d->m_debuggerManager->m_attachedPID = appPid;
        m_d->m_debuggerManagerAccess->notifyInferiorPidChanged(appPid);
    } else {
        QMessageBox::critical(m_d->m_debuggerManager->mainWindow(), tr("Debugger Error"), errorMessage);
    }
}

void CdbDebugEngine::slotConsoleStubError(const QString &msg)
{
    QMessageBox::critical(m_d->m_debuggerManager->mainWindow(), tr("Debugger Error"), msg);
}

void CdbDebugEngine::slotConsoleStubTerminated()
{
    exitDebugger();
}

void CdbDebugEnginePrivate::handleDebugEvent()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << m_hDebuggeeProcess;

    if (m_bIgnoreNextDebugEvent) {
        m_engine->startWatchTimer();
        m_bIgnoreNextDebugEvent = false;
    } else {
        m_debuggerManagerAccess->notifyInferiorStopped();
        updateThreadList();
        updateStackTrace();
    }

    //ULONG executionStatus;
    //HRESULT hr = m_pDebugControl->GetExecutionStatus(&executionStatus);
    //if (SUCCEEDED(hr) && executionStatus == DEBUG_STATUS_STEP_INTO) {
    //    // check if stack trace is valid
    //    StackHandler* sh = qq->stackHandler();
    //    QList<StackFrame> frames = sh->frames();
    //    if (frames.size() > 0 && frames.first().file.isEmpty()) {
    //        stepExec();
    //    }
    //}
}

void CdbDebugEnginePrivate::setDebuggeeHandles(HANDLE hDebuggeeProcess,  HANDLE hDebuggeeThread)
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << hDebuggeeProcess << hDebuggeeThread;
    m_hDebuggeeProcess = hDebuggeeProcess;
    m_hDebuggeeThread = hDebuggeeThread;
}

void CdbDebugEnginePrivate::updateThreadList()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO << m_hDebuggeeProcess;

    ThreadsHandler* th = m_debuggerManagerAccess->threadsHandler();
    QList<ThreadData> threads;

    HRESULT hr;
    ULONG numberOfThreads;
    hr = m_pDebugSystemObjects->GetNumberThreads(&numberOfThreads);
    const ULONG maxThreadIds = 256;
    ULONG threadIds[maxThreadIds];
    ULONG biggestThreadId = qMin(maxThreadIds, numberOfThreads - 1);
    hr = m_pDebugSystemObjects->GetThreadIdsByIndex(0, biggestThreadId, threadIds, 0);
    for (ULONG threadId = 0; threadId <= biggestThreadId; ++threadId) {
        ThreadData thread;
        thread.id = threadId;
        threads.append(thread);
    }

    th->setThreads(threads);
}

void CdbDebugEnginePrivate::updateStackTrace()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
    // Create a new context
    cleanStackTrace();
    QString errorMessage;
    m_currentStackTrace =
            CdbStackTraceContext::create(m_pDebugControl, m_pDebugSystemObjects,
                                         m_pDebugSymbols, m_currentThreadId, &errorMessage);
    if (!m_currentStackTrace) {
        qWarning("%s: failed to create trace context: %s", Q_FUNC_INFO, qPrintable(errorMessage));
        return;
    }
    const QList<StackFrame> stackFrames = m_currentStackTrace->frames();
    // find the first usable frame and select it
    int current = -1;
    const int count = stackFrames.count();
    for (int i=0; i < count; ++i)
        if (stackFrames.at(i).isUsable()) {
            current = i;
            break;
        }

    m_debuggerManagerAccess->stackHandler()->setFrames(stackFrames);
    if (current >= 0) {
        m_debuggerManagerAccess->stackHandler()->setCurrentIndex(current);
        m_debuggerManager->gotoLocation(stackFrames.at(current).file,
                                        stackFrames.at(current).line, true);
    }
    m_firstActivatedFrame = true;
}

void CdbDebugEnginePrivate::handleDebugOutput(const char *szOutputString)
{
    if (debugCDB && strstr(szOutputString, "ModLoad:") == 0)
        qDebug() << Q_FUNC_INFO << szOutputString;
    m_debuggerManagerAccess->showApplicationOutput(QString::fromLocal8Bit(szOutputString));
}

void CdbDebugEnginePrivate::handleBreakpointEvent(PDEBUG_BREAKPOINT pBP)
{
    Q_UNUSED(pBP)
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
}

void CdbDebugEngine::reloadSourceFiles()
{
}

} // namespace Internal
} // namespace Debugger

// Accessed by DebuggerManager
Debugger::Internal::IDebuggerEngine *createWinEngine(Debugger::Internal::DebuggerManager *parent)
{
    return Debugger::Internal::CdbDebugEngine::create(parent);
}

