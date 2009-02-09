#include "msvcdebugengine.h"

#include "assert.h"
#include "debuggermanager.h"
#include "breakhandler.h"
#include "stackhandler.h"

#include <QDebug>
#include <QTimerEvent>
#include <QFileInfo>

#define DBGHELP_TRANSLATE_TCHAR
#include <Dbghelp.h>

using namespace Debugger;
using namespace Debugger::Internal;


MSVCDebugEngine::MSVCDebugEngine(DebuggerManager *parent)
:   IDebuggerEngine(parent),
    m_hDebuggeeProcess(0),
    m_hDebuggeeThread(0),
    //m_hDebuggeeImage(0),
    m_bIgnoreNextDebugEvent(false),
    m_watchTimer(-1),
    m_debugEventCallBack(this),
    m_debugOutputCallBack(this)
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

MSVCDebugEngine::~MSVCDebugEngine()
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

void MSVCDebugEngine::startWatchTimer()
{
    if (m_watchTimer == -1)
        m_watchTimer = startTimer(0);
}

void MSVCDebugEngine::killWatchTimer()
{
    if (m_watchTimer != -1) {
        killTimer(m_watchTimer);
        m_watchTimer = -1;
    }
}

void MSVCDebugEngine::shutdown()
{
    exitDebugger();
}

void MSVCDebugEngine::setToolTipExpression(const QPoint &pos, const QString &exp)
{
}

bool MSVCDebugEngine::startDebugger()
{
    q->showStatusMessage("Starting Debugger", -1);

    //if (!q->m_workingDir.isEmpty())
    //    m_gdbProc.setWorkingDirectory(q->m_workingDir);
    //if (!q->m_environment.isEmpty())
    //    m_gdbProc.setEnvironment(q->m_environment);

    DEBUG_CREATE_PROCESS_OPTIONS dbgopts;
    memset(&dbgopts, 0, sizeof(dbgopts));
    dbgopts.CreateFlags = DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS;

    HRESULT hr;

    QString filename(q->m_executable);
    QFileInfo fi(filename);
    m_pDebugSymbols->AppendImagePathWide(fi.absolutePath().replace('/','\\').utf16());
    //m_pDebugSymbols->SetSymbolOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_DEBUG | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_AUTO_PUBLICS);
    m_pDebugSymbols->SetSymbolOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_AUTO_PUBLICS);
    //m_pDebugSymbols->AddSymbolOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_DEBUG | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_AUTO_PUBLICS | SYMOPT_NO_IMAGE_SEARCH);

    if (q->startMode() == q->attachExternal) {
        qWarning("MSVCDebugEngine: attach to process not yet implemented!");
    } else {
        hr = m_pDebugClient->CreateProcess2Wide(NULL,
                                                const_cast<PWSTR>(filename.utf16()),
                                                &dbgopts,
                                                sizeof(dbgopts),
                                                NULL,  // TODO: think about the initial directory
                                                NULL); // TODO: think about setting the environment
        if (FAILED(hr)) {
            //qWarning("CreateProcess2Wide failed");
            qq->notifyInferiorExited();
            return false;
        }
    }

    q->showStatusMessage(tr("Debugger Running"), -1);
    startWatchTimer();
    return true;
}

void MSVCDebugEngine::exitDebugger()
{
    m_pDebugClient->TerminateCurrentProcess();
    killWatchTimer();
}

void MSVCDebugEngine::updateWatchModel()
{
}

void MSVCDebugEngine::stepExec()
{
    //qDebug() << "MSVCDebugEngine::stepExec()";
    //m_pDebugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT, "p", 0);
    HRESULT hr;
    hr = m_pDebugControl->SetExecutionStatus(DEBUG_STATUS_STEP_INTO);
    m_bIgnoreNextDebugEvent = true;
    startWatchTimer();
}

void MSVCDebugEngine::stepOutExec()
{
    //qDebug() << "MSVCDebugEngine::stepOutExec()";
    StackHandler* sh = qq->stackHandler();
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
    HRESULT hr = m_pDebugControl->AddBreakpoint2(DEBUG_BREAKPOINT_CODE, DEBUG_ANY_ID, &pBP);
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

void MSVCDebugEngine::nextExec()
{
    //qDebug() << "MSVCDebugEngine::nextExec()";
    HRESULT hr;
    hr = m_pDebugControl->SetExecutionStatus(DEBUG_STATUS_STEP_OVER);
    startWatchTimer();
}

void MSVCDebugEngine::stepIExec()
{
    qWarning("MSVCDebugEngine::stepIExec() not implemented");
}

void MSVCDebugEngine::nextIExec()
{
    m_pDebugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT, "p", 0);
    startWatchTimer();
}

void MSVCDebugEngine::continueInferior()
{
    killWatchTimer();
    q->resetLocation();

    ULONG executionStatus;
    HRESULT hr = m_pDebugControl->GetExecutionStatus(&executionStatus);
    if (SUCCEEDED(hr) && executionStatus != DEBUG_STATUS_GO)
        m_pDebugControl->SetExecutionStatus(DEBUG_STATUS_GO);

    startWatchTimer();
    qq->notifyInferiorRunning();
}

void MSVCDebugEngine::runInferior()
{
    continueInferior();
}

void MSVCDebugEngine::interruptInferior()
{
    //TODO: better use IDebugControl::SetInterrupt?
    if (!m_hDebuggeeProcess)
        return;
    if (!DebugBreakProcess(m_hDebuggeeProcess)) {
        qWarning("DebugBreakProcess failed.");
        return;
    }
    qq->notifyInferiorStopped();
}

void MSVCDebugEngine::runToLineExec(const QString &fileName, int lineNumber)
{
}

void MSVCDebugEngine::runToFunctionExec(const QString &functionName)
{
}

void MSVCDebugEngine::jumpToLineExec(const QString &fileName, int lineNumber)
{
}

void MSVCDebugEngine::assignValueInDebugger(const QString &expr, const QString &value)
{
}

void MSVCDebugEngine::executeDebuggerCommand(const QString &/*command*/)
{
}

void MSVCDebugEngine::activateFrame(int frameIndex)
{
    if (q->status() != DebuggerInferiorStopped)
        return;

    StackHandler *stackHandler = qq->stackHandler();
    int oldIndex = stackHandler->currentIndex();
    //qDebug() << "ACTIVATE FRAME: " << frameIndex << oldIndex
    //    << stackHandler->currentIndex();

    QWB_ASSERT(frameIndex < stackHandler->stackSize(), return);

    if (oldIndex != frameIndex) {
        stackHandler->setCurrentIndex(frameIndex);
        //updateLocals();
    }

    const StackFrame &frame = stackHandler->currentFrame();

    bool usable = !frame.file.isEmpty() && QFileInfo(frame.file).isReadable();
    if (usable)
        q->gotoLocation(frame.file, frame.line, true);
    else
        qDebug() << "FULL NAME NOT USABLE: " << frame.file;
}

void MSVCDebugEngine::selectThread(int index)
{
    //reset location arrow
    q->resetLocation();

    ThreadsHandler *threadsHandler = qq->threadsHandler();
    threadsHandler->setCurrentThread(index);
    m_currentThreadId = index;
    updateStackTrace();
}

void MSVCDebugEngine::attemptBreakpointSynchronization()
{
    BreakHandler *handler = qq->breakHandler();
    //qDebug() << "attemptBreakpointSynchronization";
    for (int i=0; i < handler->size(); ++i) {
        BreakpointData* breakpoint = handler->at(i);
        if (breakpoint->pending) {
            IDebugBreakpoint2* pBP = 0;
            HRESULT hr = m_pDebugControl->AddBreakpoint2(DEBUG_BREAKPOINT_CODE, DEBUG_ANY_ID, &pBP);
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

void MSVCDebugEngine::loadSessionData()
{
}

void MSVCDebugEngine::saveSessionData()
{
}

void MSVCDebugEngine::reloadDisassembler()
{
}

void MSVCDebugEngine::reloadModules()
{
}

void MSVCDebugEngine::loadSymbols(const QString &moduleName)
{
}

void MSVCDebugEngine::loadAllSymbols()
{
}

void MSVCDebugEngine::reloadRegisters()
{
}

void MSVCDebugEngine::timerEvent(QTimerEvent* te)
{
    if (te->timerId() != m_watchTimer)
        return;

    HRESULT hr;
    hr = m_pDebugControl->WaitForEvent(0, 1);
    switch (hr) {
        case S_OK:
            //qDebug() << "WaitForEvent S_OK";
            killWatchTimer();
            handleDebugEvent();
            break;
        case S_FALSE:
            //qDebug() << "S_FALSE";
            break;
        case E_PENDING:
            qDebug() << "S_PENDING";
            break;
        case E_UNEXPECTED:
            killWatchTimer();
            break;
        case E_FAIL:
            qDebug() << "E_FAIL";
            break;
        //default:
        //    qDebug() << "asser welljuh, schuddnt heppn";
    }
}

void MSVCDebugEngine::handleDebugEvent()
{
    if (m_bIgnoreNextDebugEvent) {
        startWatchTimer();
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

void MSVCDebugEngine::updateThreadList()
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

void MSVCDebugEngine::updateStackTrace()
{
    //qDebug() << "updateStackTrace()";
    HRESULT hr;
    hr = m_pDebugSystemObjects->SetCurrentThreadId(m_currentThreadId);

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

void MSVCDebugEngine::handleDebugOutput(const char* szOutputString)
{
    qq->showApplicationOutput("app-dbgoutput", QString::fromLocal8Bit(szOutputString));
}

void MSVCDebugEngine::handleBreakpointEvent(PDEBUG_BREAKPOINT pBP)
{
    qDebug() << "MSVCDebugEngine::handleBreakpointEvent()";
}

IDebuggerEngine *createWinEngine(DebuggerManager *parent)
{
    return new MSVCDebugEngine(parent);
}
