/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "cdbdebugengine.h"
#include "cdbdebugengine_p.h"

#include "debuggermanager.h"
#include "breakhandler.h"
#include "stackhandler.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QTimerEvent>
#include <QFileInfo>

#define DBGHELP_TRANSLATE_TCHAR
#include <inc/Dbghelp.h>

using namespace Debugger;
using namespace Debugger::Internal;

CdbDebugEnginePrivate::CdbDebugEnginePrivate(DebuggerManager *parent, CdbDebugEngine* engine) :
    m_hDebuggeeProcess(0),
    m_hDebuggeeThread(0),
    m_bIgnoreNextDebugEvent(false),
    m_watchTimer(-1),
    m_debugEventCallBack(engine),
    m_debugOutputCallBack(engine),
    m_engine(engine)
{
    q = parent;
    qq = parent->engineInterface();

    HRESULT hr;
    hr = DebugCreate( __uuidof(IDebugClient5), reinterpret_cast<void**>(&m_pDebugClient));
    if (FAILED(hr)) m_pDebugClient = 0;
    hr = DebugCreate( __uuidof(IDebugControl4), reinterpret_cast<void**>(&m_pDebugControl));
    if (FAILED(hr)) m_pDebugControl = 0;
    hr = DebugCreate( __uuidof(IDebugSystemObjects4), reinterpret_cast<void**>(&m_pDebugSystemObjects));
    if (FAILED(hr)) m_pDebugSystemObjects = 0;
    hr = DebugCreate( __uuidof(IDebugSymbols3), reinterpret_cast<void**>(&m_pDebugSymbols));
    if (FAILED(hr)) m_pDebugSymbols = 0;
    hr = DebugCreate( __uuidof(IDebugRegisters2), reinterpret_cast<void**>(&m_pDebugRegisters));
    if (FAILED(hr)) m_pDebugRegisters = 0;

    if (m_pDebugControl) {
        m_pDebugControl->SetCodeLevel(DEBUG_LEVEL_SOURCE);
    }
    if (m_pDebugClient) {
        m_pDebugClient->SetOutputCallbacks(&m_debugOutputCallBack);
        m_pDebugClient->SetEventCallbacks(&m_debugEventCallBack);
    }
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

CdbDebugEngine::CdbDebugEngine(DebuggerManager *parent)
  : IDebuggerEngine(parent),
    m_d(new CdbDebugEnginePrivate(parent, this))

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
    m_d->q->showStatusMessage("Starting Debugger", -1);

    //if (!q->m_workingDir.isEmpty())
    //    m_gdbProc.setWorkingDirectory(q->m_workingDir);
    //if (!q->m_environment.isEmpty())
    //    m_gdbProc.setEnvironment(q->m_environment);

    DEBUG_CREATE_PROCESS_OPTIONS dbgopts;
    memset(&dbgopts, 0, sizeof(dbgopts));
    dbgopts.CreateFlags = DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS;

    HRESULT hr;

    QString filename(m_d->q->m_executable);
    QFileInfo fi(filename);
    m_d->m_pDebugSymbols->AppendImagePathWide(fi.absolutePath().replace('/','\\').utf16());
    //m_pDebugSymbols->SetSymbolOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_DEBUG | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_AUTO_PUBLICS);
    m_d->m_pDebugSymbols->SetSymbolOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_AUTO_PUBLICS);
    //m_pDebugSymbols->AddSymbolOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_DEBUG | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_AUTO_PUBLICS | SYMOPT_NO_IMAGE_SEARCH);

    if (m_d->q->startMode() == DebuggerManager::AttachExternal) {
        qWarning("CdbDebugEngine: attach to process not yet implemented!");
    } else {
        hr = m_d->m_pDebugClient->CreateProcess2Wide(NULL,
                                                const_cast<PWSTR>(filename.utf16()),
                                                &dbgopts,
                                                sizeof(dbgopts),
                                                NULL,  // TODO: think about the initial directory
                                                NULL); // TODO: think about setting the environment
        if (FAILED(hr)) {
            //qWarning("CreateProcess2Wide failed");
            m_d->qq->notifyInferiorExited();
            return false;
        }
    }

    m_d->q->showStatusMessage(tr("Debugger Running"), -1);
    startWatchTimer();
    return true;
}

void CdbDebugEngine::exitDebugger()
{
    m_d->m_pDebugClient->TerminateCurrentProcess();
    killWatchTimer();
}

void CdbDebugEngine::updateWatchModel()
{
}

void CdbDebugEngine::stepExec()
{
    //qDebug() << "CdbDebugEngine::stepExec()";
    //m_pDebugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT, "p", 0);
    HRESULT hr;
    hr = m_d->m_pDebugControl->SetExecutionStatus(DEBUG_STATUS_STEP_INTO);
    m_d->m_bIgnoreNextDebugEvent = true;
    startWatchTimer();
}

void CdbDebugEngine::stepOutExec()
{
    //qDebug() << "CdbDebugEngine::stepOutExec()";
    StackHandler* sh = m_d->qq->stackHandler();
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
    //qDebug() << "CdbDebugEngine::nextExec()";
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
    m_d->m_pDebugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT, "p", 0);
    startWatchTimer();
}

void CdbDebugEngine::continueInferior()
{
    killWatchTimer();
    m_d->q->resetLocation();

    ULONG executionStatus;
    HRESULT hr = m_d->m_pDebugControl->GetExecutionStatus(&executionStatus);
    if (SUCCEEDED(hr) && executionStatus != DEBUG_STATUS_GO)
        m_d->m_pDebugControl->SetExecutionStatus(DEBUG_STATUS_GO);

    startWatchTimer();
    m_d->qq->notifyInferiorRunning();
}

void CdbDebugEngine::interruptInferior()
{
    //TODO: better use IDebugControl::SetInterrupt?
    if (!m_d->m_hDebuggeeProcess)
        return;
    if (!DebugBreakProcess(m_d->m_hDebuggeeProcess)) {
        qWarning("DebugBreakProcess failed.");
        return;
    }
    m_d->qq->notifyInferiorStopped();
}

void CdbDebugEngine::runToLineExec(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName)
    Q_UNUSED(lineNumber)
}

void CdbDebugEngine::runToFunctionExec(const QString &functionName)
{
    Q_UNUSED(functionName)
}

void CdbDebugEngine::jumpToLineExec(const QString &fileName, int lineNumber)
{
    Q_UNUSED(fileName)
    Q_UNUSED(lineNumber)
}

void CdbDebugEngine::assignValueInDebugger(const QString &expr, const QString &value)
{
    Q_UNUSED(expr)
    Q_UNUSED(value)
}

void CdbDebugEngine::executeDebuggerCommand(const QString &/*command*/)
{
}

void CdbDebugEngine::activateFrame(int frameIndex)
{
    if (m_d->q->status() != DebuggerInferiorStopped)
        return;

    StackHandler *stackHandler = m_d->qq->stackHandler();
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
        m_d->q->gotoLocation(frame.file, frame.line, true);
    else
        qDebug() << "FULL NAME NOT USABLE: " << frame.file;
}

void CdbDebugEngine::selectThread(int index)
{
    //reset location arrow
    m_d->q->resetLocation();

    ThreadsHandler *threadsHandler = m_d->qq->threadsHandler();
    threadsHandler->setCurrentThread(index);
    m_d->m_currentThreadId = index;
    m_d->updateStackTrace();
}

void CdbDebugEngine::attemptBreakpointSynchronization()
{
    BreakHandler *handler = m_d->qq->breakHandler();
    //qDebug() << "attemptBreakpointSynchronization";
    for (int i=0; i < handler->size(); ++i) {
        BreakpointData* breakpoint = handler->at(i);
        if (breakpoint->pending) {
            IDebugBreakpoint2* pBP = 0;
            HRESULT hr = m_d->m_pDebugControl->AddBreakpoint2(DEBUG_BREAKPOINT_CODE, DEBUG_ANY_ID, &pBP);
            if (FAILED(hr) || !pBP) {
                qWarning("m_pDebugControl->AddBreakpoint2 failed");
                continue;
            }

            QString str = '`' + breakpoint->fileName + ':' + breakpoint->lineNumber + '`';
            hr = pBP->SetOffsetExpressionWide(str.utf16());
            if (FAILED(hr)) {
                qWarning("SetOffsetExpressionWide failed");
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
    Q_UNUSED(moduleName)
}

void CdbDebugEngine::loadAllSymbols()
{
}

void CdbDebugEngine::reloadRegisters()
{
}

void CdbDebugEngine::timerEvent(QTimerEvent* te)
{
    if (te->timerId() != m_d->m_watchTimer)
        return;

    HRESULT hr;
    hr = m_d->m_pDebugControl->WaitForEvent(0, 1);
    switch (hr) {
        case S_OK:
            //qDebug() << "WaitForEvent S_OK";
            killWatchTimer();
            m_d->handleDebugEvent();
            break;
        case S_FALSE:
            //qDebug() << "S_FALSE";
            break;
        case E_PENDING:
            qDebug() << "E_PENDING";
            break;
        case E_UNEXPECTED:
            killWatchTimer();
            break;
        case E_FAIL:
            qDebug() << "E_FAIL";
            break;
    }
}

void CdbDebugEnginePrivate::handleDebugEvent()
{
    if (m_bIgnoreNextDebugEvent) {
        m_engine->startWatchTimer();
        m_bIgnoreNextDebugEvent = false;
    } else {
        qq->notifyInferiorStopped();
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
    ThreadsHandler* th = qq->threadsHandler();
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
        frame.address = QString("0x%1").arg(frames[i].InstructionOffset, 0, 16);

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

    qq->stackHandler()->setFrames(stackFrames);

    // find the first usable frame and select it
    for (int i=0; i < stackFrames.count(); ++i) {
        const StackFrame &frame = stackFrames.at(i);
        bool usable = !frame.file.isEmpty() && QFileInfo(frame.file).isReadable();
        if (usable) {
            qq->stackHandler()->setCurrentIndex(i);
            q->gotoLocation(frame.file, frame.line, true);
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
    qq->showApplicationOutput(QString::fromLocal8Bit(szOutputString));
}

void CdbDebugEnginePrivate::handleBreakpointEvent(PDEBUG_BREAKPOINT pBP)
{
    Q_UNUSED(pBP)
    qDebug() << "CdbDebugEngine::handleBreakpointEvent()";
}

IDebuggerEngine *createWinEngine(DebuggerManager *parent)
{
    return new CdbDebugEngine(parent);
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
