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

#include "debuggermanager.h"
#include "breakhandler.h"
#include "stackhandler.h"

#include <utils/qtcassert.h>
#include <utils/consoleprocess.h>

#include <QtCore/QDebug>
#include <QtCore/QTimerEvent>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QLibrary>

#define DBGHELP_TRANSLATE_TCHAR
#include <inc/Dbghelp.h>

static const char *dbgEngineDllC = "dbgeng";
static const char *debugCreateFuncC = "DebugCreate";

namespace Debugger {
namespace Internal {

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

CdbDebugEnginePrivate::CdbDebugEnginePrivate(const DebuggerEngineLibrary &lib, DebuggerManager *parent, CdbDebugEngine* engine) :
    m_hDebuggeeProcess(0),
    m_hDebuggeeThread(0),
    m_bIgnoreNextDebugEvent(false),
    m_watchTimer(-1),
    m_debugEventCallBack(engine),
    m_debugOutputCallBack(engine),
    m_engine(engine),
    m_debuggerManager(parent),
    m_debuggerManagerAccess(parent->engineInterface())
{
    HRESULT hr;
    hr = lib.debugCreate( __uuidof(IDebugClient5), reinterpret_cast<void**>(&m_pDebugClient));
    if (FAILED(hr)) {
        m_pDebugClient = 0;
    } else {
        m_pDebugClient->SetOutputCallbacks(&m_debugOutputCallBack);
        m_pDebugClient->SetEventCallbacks(&m_debugEventCallBack);
    }

    hr = lib.debugCreate( __uuidof(IDebugControl4), reinterpret_cast<void**>(&m_pDebugControl));
    if (FAILED(hr)) {
        m_pDebugControl = 0;
    } else {
        m_pDebugControl->SetCodeLevel(DEBUG_LEVEL_SOURCE);
    }

    hr = lib.debugCreate( __uuidof(IDebugSystemObjects4), reinterpret_cast<void**>(&m_pDebugSystemObjects));
    if (FAILED(hr)) m_pDebugSystemObjects = 0;

    hr = lib.debugCreate( __uuidof(IDebugSymbols3), reinterpret_cast<void**>(&m_pDebugSymbols));
    if (FAILED(hr)) m_pDebugSymbols = 0;

    hr = lib.debugCreate( __uuidof(IDebugRegisters2), reinterpret_cast<void**>(&m_pDebugRegisters));
    if (FAILED(hr)) m_pDebugRegisters = 0;
}

IDebuggerEngine *CdbDebugEngine::create(DebuggerManager *parent)
{
    DebuggerEngineLibrary lib;
    QString errorMessage;
    if (!lib.init(&errorMessage)) {
        qWarning("%s", qPrintable(errorMessage));
        return 0;
    }
    return new CdbDebugEngine(lib, parent);
}

CdbDebugEnginePrivate::~CdbDebugEnginePrivate()
{
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

CdbDebugEngine::CdbDebugEngine(const DebuggerEngineLibrary &lib, DebuggerManager *parent) :
    IDebuggerEngine(parent),
    m_d(new CdbDebugEnginePrivate(lib, parent, this))
{
}

CdbDebugEngine::~CdbDebugEngine()
{
    delete m_d;
}

void CdbDebugEngine::startWatchTimer()
{
    if (m_d->m_watchTimer == -1)
        m_d->m_watchTimer = startTimer(0);
}

void CdbDebugEngine::killWatchTimer()
{
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

    //if (!q->m_workingDir.isEmpty())
    //    m_gdbProc.setWorkingDirectory(q->m_workingDir);
    //if (!q->m_environment.isEmpty())
    //    m_gdbProc.setEnvironment(q->m_environment);

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

    if (m_d->m_debuggerManager->startMode() == AttachExternal) {
        qWarning("CdbDebugEngine: attach to process not yet implemented!");
        return false;
    } else {
        QString cmd = Core::Utils::AbstractProcess::createWinCommandline(filename, m_d->m_debuggerManager->m_processArgs);
        PCWSTR env = 0;
        QByteArray envData;
        if (!m_d->m_debuggerManager->m_environment.empty()) {
            envData = Core::Utils::AbstractProcess::createWinEnvironment(Core::Utils::AbstractProcess::fixWinEnvironment(m_d->m_debuggerManager->m_environment));
            env = reinterpret_cast<PCWSTR>(envData.data());
        }
        HRESULT hr = m_d->m_pDebugClient->CreateProcess2Wide(NULL,
                                                const_cast<PWSTR>(cmd.utf16()),
                                                &dbgopts,
                                                sizeof(dbgopts),
                                                m_d->m_debuggerManager->m_workingDir.utf16(),
                                                env);
        if (FAILED(hr)) {
            //qWarning("CreateProcess2Wide failed");
            m_d->m_debuggerManagerAccess->notifyInferiorExited();
            return false;
        }
    }

    m_d->m_debuggerManager->showStatusMessage(tr("Debugger Running"), -1);
    startWatchTimer();
    return true;
}

void CdbDebugEngine::exitDebugger()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    m_d->m_pDebugClient->TerminateCurrentProcess();
    killWatchTimer();
}

void CdbDebugEngine::updateWatchModel()
{
}

void CdbDebugEngine::stepExec()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    //m_pDebugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT, "p", 0);
    HRESULT hr;
    hr = m_d->m_pDebugControl->SetExecutionStatus(DEBUG_STATUS_STEP_INTO);
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

    HRESULT hr;
    hr = m_d->m_pDebugControl->SetExecutionStatus(DEBUG_STATUS_STEP_OVER);
    startWatchTimer();
}

void CdbDebugEngine::stepIExec()
{
    qWarning("CdbDebugEngine::stepIExec() not implemented");
}

void CdbDebugEngine::nextIExec()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    m_d->m_pDebugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT, "p", 0);
    startWatchTimer();
}

void CdbDebugEngine::continueInferior()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    killWatchTimer();
    m_d->m_debuggerManager->resetLocation();

    ULONG executionStatus;
    HRESULT hr = m_d->m_pDebugControl->GetExecutionStatus(&executionStatus);
    if (SUCCEEDED(hr) && executionStatus != DEBUG_STATUS_GO)
        m_d->m_pDebugControl->SetExecutionStatus(DEBUG_STATUS_GO);

    startWatchTimer();
    m_d->m_debuggerManagerAccess->notifyInferiorRunning();
}

void CdbDebugEngine::interruptInferior()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

    //TODO: better use IDebugControl::SetInterrupt?
    if (!m_d->m_hDebuggeeProcess)
        return;
    if (!DebugBreakProcess(m_d->m_hDebuggeeProcess)) {
        qWarning("DebugBreakProcess failed.");
        return;
    }
    m_d->m_debuggerManagerAccess->notifyInferiorStopped();
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

    if (m_d->m_debuggerManager->status() != DebuggerInferiorStopped)
        return;

    StackHandler *stackHandler = m_d->m_debuggerManagerAccess->stackHandler();
    const int oldIndex = stackHandler->currentIndex();
    //qDebug() << "ACTIVATE FRAME: " << frameIndex << oldIndex
    //    << stackHandler->currentIndex();

    QTC_ASSERT(frameIndex < stackHandler->stackSize(), return);

    if (oldIndex != frameIndex) {
        stackHandler->setCurrentIndex(frameIndex);
        //updateLocals();
    }

    const StackFrame &frame = stackHandler->currentFrame();

    const bool usable = !frame.file.isEmpty() && QFileInfo(frame.file).isReadable();
    if (usable)
        m_d->m_debuggerManager->gotoLocation(frame.file, frame.line, true);
    else
        qDebug() << "FULL NAME NOT USABLE: " << frame.file;
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

    if (debugCDB > 1)
        qDebug() << Q_FUNC_INFO;

    HRESULT hr;
    hr = m_d->m_pDebugControl->WaitForEvent(0, 1);
    switch (hr) {
        case S_OK:
            if (debugCDB > 1)
                qDebug() << "WaitForEvent S_OK";

            killWatchTimer();
            m_d->handleDebugEvent();
            break;
        case S_FALSE:
            if (debugCDB > 1)
                qDebug() << "WaitForEvent S_FALSE";
            break;
        case E_PENDING:
            if (debugCDB > 1)
                qDebug() << "WaitForEvent E_PENDING";
            break;
        case E_UNEXPECTED:
            if (debugCDB > 1)
                qDebug() << "WaitForEvent E_UNEXPECTED";
            killWatchTimer();
            break;
        case E_FAIL:
            if (debugCDB > 1)
                qDebug() << "WaitForEvent E_FAIL";
            break;
    }
}

void CdbDebugEnginePrivate::handleDebugEvent()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

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

void CdbDebugEnginePrivate::updateThreadList()
{
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;

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

    //qDebug() << "updateStackTrace()";
    HRESULT hr = m_pDebugSystemObjects->SetCurrentThreadId(m_currentThreadId);

    //ULONG64 frameOffset, instructionOffset, stackOffset;
    //if (FAILED(m_pDebugRegisters->GetFrameOffset2(DEBUG_REGSRC_DEBUGGEE, &frameOffset)) ||
    //    FAILED(m_pDebugRegisters->GetInstructionOffset2(DEBUG_REGSRC_DEBUGGEE, &instructionOffset)) ||
    //    FAILED(m_pDebugRegisters->GetStackOffset2(DEBUG_REGSRC_DEBUGGEE, &stackOffset)))
    //{
    //    frameOffset = instructionOffset = stackOffset = 0;
    //}
    //frameOffset = instructionOffset = stackOffset = 0;

    const ULONG numFrames = 100;
    ULONG numFramesFilled = 0;
    DEBUG_STACK_FRAME frames[numFrames];
    //hr = m_pDebugControl->GetStackTrace(frameOffset, stackOffset, instructionOffset, frames, numFrames, &numFramesFilled);
    hr = m_pDebugControl->GetStackTrace(0, 0, 0, frames, numFrames, &numFramesFilled);
    if (FAILED(hr))
        qDebug() << "GetStackTrace failed";

    QList<StackFrame> stackFrames;

    WCHAR wszBuf[MAX_PATH];
    for (ULONG i=0; i < numFramesFilled; ++i) {
        StackFrame frame;
        frame.line = 0;
        frame.level = i;
        frame.address = QString::fromLatin1("0x%1").arg(frames[i].InstructionOffset, 0, 16);

        m_pDebugSymbols->GetNameByOffsetWide(frames[i].InstructionOffset, wszBuf, MAX_PATH, 0, 0);
        frame.function = QString::fromUtf16(wszBuf);

        ULONG ulLine;
        ULONG ulFileNameSize;
        ULONG64 ul64Displacement;
        hr = m_pDebugSymbols->GetLineByOffsetWide(frames[i].InstructionOffset, &ulLine, wszBuf, MAX_PATH, &ulFileNameSize, &ul64Displacement);
        if (SUCCEEDED(hr)) {
            frame.line = ulLine;
            frame.file = QString::fromUtf16(wszBuf, ulFileNameSize);
        }
        stackFrames.append(frame);
    }

    m_debuggerManagerAccess->stackHandler()->setFrames(stackFrames);

    // find the first usable frame and select it
    for (int i=0; i < stackFrames.count(); ++i) {
        const StackFrame &frame = stackFrames.at(i);
        const bool usable = !frame.file.isEmpty() && QFileInfo(frame.file).isReadable();
        if (usable) {
            m_debuggerManagerAccess->stackHandler()->setCurrentIndex(i);
            m_debuggerManager->gotoLocation(frame.file, frame.line, true);
            break;
        }
    }

    //m_pDebugSymbols->GetImagePathWide(wszBuf, buflen, 0);
    //qDebug() << "ImagePath" << QString::fromUtf16(wszBuf);
    //m_pDebugSymbols->GetSymbolPathWide(wszBuf, buflen, 0);
    //qDebug() << "SymbolPath" << QString::fromUtf16(wszBuf);

    //m_pDebugControl->OutputStackTrace(DEBUG_OUTCTL_THIS_CLIENT, 0, 2, DEBUG_STACK_FRAME_ADDRESSES | DEBUG_STACK_COLUMN_NAMES | DEBUG_STACK_FRAME_NUMBERS);
    //m_pDebugControl->OutputStackTrace(DEBUG_OUTCTL_THIS_CLIENT, frames, numFramesFilled, DEBUG_STACK_SOURCE_LINE);
}

void CdbDebugEnginePrivate::handleDebugOutput(const char* szOutputString)
{
    m_debuggerManagerAccess->showApplicationOutput(QString::fromLocal8Bit(szOutputString));
}

void CdbDebugEnginePrivate::handleBreakpointEvent(PDEBUG_BREAKPOINT pBP)
{
    Q_UNUSED(pBP)
    if (debugCDB)
        qDebug() << Q_FUNC_INFO;
}

void CdbDebugEngine::setDebugDumpers(bool on)
{
    Q_UNUSED(on)
}

void CdbDebugEngine::setUseCustomDumpers(bool on)
{
    Q_UNUSED(on)
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

