#include "debugger.h"
#include "windbgthread.h"
#include "outputcallback.h"
#include "windbgeventcallback.h"

#include <QFileInfo>
#include <QTimerEvent>
#include <QDebug>

Debugger::Debugger(QObject* parent)
:   QObject(parent),
    m_callbackEvent(this),
    m_watchTimer(-1),
    m_hDebuggeeProcess(0)
{
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

    m_pDebugClient->SetOutputCallbacks(&g_outputCallbacks);
    m_pDebugClient->SetEventCallbacks(&m_callbackEvent);
}

Debugger::~Debugger()
{
    killTimer(m_watchTimer);
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

void Debugger::timerEvent(QTimerEvent* te)
{
    if (te->timerId() != m_watchTimer)
        return;

    HRESULT hr;
    hr = m_pDebugControl->WaitForEvent(0, 1);
    switch (hr) {
        case S_OK:
            //qDebug() << "S_OK";
            //hr = m_pDebugControl->SetExecutionStatus(DEBUG_STATUS_BREAK);
            killTimer(m_watchTimer);
            m_watchTimer = -1;
            handleDebugEvent();
            break;
        case S_FALSE:
            //qDebug() << "S_FALSE";
            break;
        case E_PENDING:
            qDebug() << "S_PENDING";
            break;
        case E_UNEXPECTED:
            killTimer(m_watchTimer);
            m_watchTimer = -1;
            break;
        case E_FAIL:
            qDebug() << "E_FAIL";
            break;
        default:
            qDebug() << "asser welljuh";
    }
}

void Debugger::openProcess(const QString& filename)
{
    DEBUG_CREATE_PROCESS_OPTIONS dbgopts;
    memset(&dbgopts, 0, sizeof(dbgopts));
    dbgopts.CreateFlags = DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS;

    HRESULT hr;

    QFileInfo fi(filename);
    m_pDebugSymbols->AppendImagePathWide(fi.absolutePath().replace('/','\\').utf16());
    //m_pDebugSymbols->AppendSymbolPathWide(fi.absolutePath().replace('/','\\').utf16());
    //m_pDebugSymbols->AppendSymbolPathWide(L"D:\\dev\\qt\\4.4.3\\lib");
    m_pDebugSymbols->SetSymbolOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_DEBUG | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_AUTO_PUBLICS);
    //m_pDebugSymbols->AddSymbolOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_DEBUG | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_AUTO_PUBLICS | SYMOPT_NO_IMAGE_SEARCH);

    hr = m_pDebugClient->CreateProcess2Wide(NULL,
                                            const_cast<PWSTR>(filename.utf16()),
                                            &dbgopts,
                                            sizeof(dbgopts),
                                            NULL,  // TODO: think about the initial directory
                                            NULL); // TODO: think about setting the environment
    if (FAILED(hr)) {
        qWarning("CreateProcess2Wide failed");
        return;
    }

    m_watchTimer = startTimer(0);
}

void Debugger::closeProcess()
{
    m_pDebugClient->TerminateCurrentProcess();
}

void Debugger::breakAtCurrentPosition()
{
    if (!m_hDebuggeeProcess)
        return;
    if (!DebugBreakProcess(m_hDebuggeeProcess))
        qWarning("DebugBreakProcess failed.");
}

void Debugger::continueProcess()
{
    m_watchTimer = startTimer(0);
}

void Debugger::handleDebugEvent()
{
    HRESULT hr;

    ULONG numberOfThreads;
    hr = m_pDebugSystemObjects->GetNumberThreads(&numberOfThreads);
    const ULONG maxThreadIds = 200;
    ULONG threadIds[maxThreadIds];
    ULONG biggestThreadId = qMin(maxThreadIds, numberOfThreads - 1);
    hr = m_pDebugSystemObjects->GetThreadIdsByIndex(0, biggestThreadId, threadIds, 0);
    for (ULONG threadId = 0; threadId <= biggestThreadId; ++threadId) {
        qDebug() << "dumping stack for thread" << threadId;

        m_pDebugSystemObjects->SetCurrentThreadId(threadId);

        ULONG64 frameOffset, instructionOffset, stackOffset;
        if (FAILED(m_pDebugRegisters->GetFrameOffset2(DEBUG_REGSRC_DEBUGGEE, &frameOffset)) ||
            FAILED(m_pDebugRegisters->GetInstructionOffset2(DEBUG_REGSRC_DEBUGGEE, &instructionOffset)) ||
            FAILED(m_pDebugRegisters->GetStackOffset2(DEBUG_REGSRC_DEBUGGEE, &stackOffset)))
        {
            frameOffset = instructionOffset = stackOffset = 0;
        }
        //frameOffset = instructionOffset = stackOffset = 0;

        const ULONG numFrames = 100;
        ULONG numFramesFilled = 0;
        DEBUG_STACK_FRAME frames[numFrames];
        hr = m_pDebugControl->GetStackTrace(frameOffset, stackOffset, instructionOffset, frames, numFrames, &numFramesFilled);
        if (FAILED(hr))
            qDebug() << "GetStackTrace failed";

        const size_t buflen = 1024;
        WCHAR wszBuf[buflen];
        for (ULONG i=0; i < numFramesFilled; ++i) {
            m_pDebugSymbols->GetNameByOffsetWide(frames[i].InstructionOffset, wszBuf, buflen, 0, 0);
            qDebug() << QString::fromUtf16(wszBuf);
        }

        //m_pDebugSymbols->GetImagePathWide(wszBuf, buflen, 0);
        //qDebug() << "ImagePath" << QString::fromUtf16(wszBuf);
        //m_pDebugSymbols->GetSymbolPathWide(wszBuf, buflen, 0);
        //qDebug() << "SymbolPath" << QString::fromUtf16(wszBuf);

        //m_pDebugControl->OutputStackTrace(DEBUG_OUTCTL_THIS_CLIENT, 0, 2, DEBUG_STACK_FRAME_ADDRESSES | DEBUG_STACK_COLUMN_NAMES | DEBUG_STACK_FRAME_NUMBERS);
        //m_pDebugControl->OutputStackTrace(DEBUG_OUTCTL_THIS_CLIENT, frames, numFramesFilled, DEBUG_STACK_SOURCE_LINE);
    }
}

void Debugger::handleCreateProcessEvent(DEBUG_EVENT* e)
{
    //qDebug() << "CREATE_PROCESS_DEBUG_EVENT";
    //m_hDebuggeeProcess = e->u.CreateProcessInfo.hProcess;
    //m_hDebuggeeThread  = e->u.CreateProcessInfo.hThread;
    //m_hDebuggeeImage   = e->u.CreateProcessInfo.hFile;

    //QFileInfo fi(m_pDbgProcess->processFileName());
    //BOOL bSuccess;
    //bSuccess = SymInitialize(m_hDebuggeeProcess, fi.absolutePath().utf16(), FALSE);
    //if (!bSuccess)
    //    qWarning("SymInitialize failed");
    //else {
    //    SymSetOptions(SYMOPT_DEBUG | SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS);
    //    if (!SymLoadModule64(m_hDebuggeeProcess, m_hDebuggeeImage, NULL, NULL, NULL, NULL))
    //        qDebug() << "SymLoadModule64 failed w/ error code" << GetLastError();
    //}
}

void Debugger::handleExceptionEvent(DEBUG_EVENT* e)
{
    //BOOL bSuccess;
    //SuspendThread(m_hDebuggeeThread);

    //CONTEXT context;
    //memset(&context, 0, sizeof(context));
    //context.ContextFlags = CONTEXT_ALL;
    //bSuccess = GetThreadContext(m_hDebuggeeThread, &context);
    //if (!bSuccess)
    //    qDebug() << "GetThreadContext failed w/ error code" << GetLastError();
    //ResumeThread(m_hDebuggeeThread);

    //STACKFRAME64 stackFrame;
    //stackFrame.AddrPC.Offset = context.Eip;
    //stackFrame.AddrPC.Mode = AddrModeFlat;
    //stackFrame.AddrFrame.Offset = context.Ebp;
    //stackFrame.AddrFrame.Mode = AddrModeFlat;
    //stackFrame.AddrStack.Offset = context.Esp;
    //stackFrame.AddrStack.Mode = AddrModeFlat;
    //m_currentStackTrace.clear();

    //do {
    //    StackFrame sf;
    //    bSuccess = StackWalk64(IMAGE_FILE_MACHINE_I386, m_hDebuggeeProcess, m_hDebuggeeThread, &stackFrame,
    //                           &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL);
    //    if (bSuccess) {
    //        qDebug() << "StackWalk";
    //        IMAGEHLP_MODULE64 moduleInfo;
    //        moduleInfo.SizeOfStruct = sizeof(moduleInfo);
    //        if (SymGetModuleInfo64(m_hDebuggeeProcess, stackFrame.AddrPC.Offset, &moduleInfo))
    //            qDebug() << "SymGetModuleInfo64 success!";
    //        else
    //            qDebug() << "SymGetModuleInfo64 failed w/ error code" << GetLastError();
    //    }

    //    if (stackFrame.AddrPC.Offset) {
    //        DWORD64 dwDisplacement;
    //        const size_t bufferSize = 200;
    //        class MySymbol : public IMAGEHLP_SYMBOL64
    //        {
    //        public:
    //        private:
    //            char buffer[bufferSize];
    //        };
    //        MySymbol img;
    //        ZeroMemory(&img, sizeof(img));
    //        img.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
    //        img.MaxNameLength = bufferSize;

    //        BOOL bSuccess;
    //        bSuccess = SymGetSymFromAddr64(m_hDebuggeeProcess,
    //                                       stackFrame.AddrPC.Offset,
    //                                       &dwDisplacement,
    //                                       &img);
    //        if (bSuccess) {
    //            qDebug() << "SymGetSymFromAddr64:" << img.Name;
    //            sf.symbol = QString::fromLocal8Bit(img.Name);
    //        }
    //        else
    //            qDebug() << "SymGetSymFromAddr64 failed w/ error code" << GetLastError();
    //    }

    //    if (stackFrame.AddrPC.Offset) {
    //        DWORD dwDisplacement;
    //        IMAGEHLP_LINE64 line;
    //        ZeroMemory(&line, sizeof(line));
    //        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    //        BOOL bSuccess;
    //        bSuccess = SymGetLineFromAddr64(m_hDebuggeeProcess,
    //                                        stackFrame.AddrPC.Offset,
    //                                        &dwDisplacement,
    //                                        &line);
    //        if (bSuccess) {
    //            //qDebug() << "SymGetLineFromAddr64:" << QString::fromUtf16((ushort*)line.FileName) << line.LineNumber;
    //            sf.filename = QString::fromUtf16((ushort*)line.FileName);
    //            sf.line = line.LineNumber;
    //        } else
    //            qDebug() << "SymGetLineFromAddr64 failed w/ error code" << GetLastError();

    //        m_currentStackTrace.append(sf);
    //    }
    //} while (bSuccess);

    //emit debuggeePaused();
}

void Debugger::handleOutputDebugStringEvent(DEBUG_EVENT* e)
{
    //qDebug() << "OUTPUT_DEBUG_STRING_EVENT";
    //BOOL bSuccess;
    //SIZE_T nNumberOfBytesRead;
    //void* buffer;
    //QString result;
    //if (e->u.DebugString.fUnicode) {
    //    buffer = malloc(e->u.DebugString.nDebugStringLength * sizeof(WCHAR));
    //} else {
    //    buffer = malloc(e->u.DebugString.nDebugStringLength * sizeof(char));
    //}

    //bSuccess = ReadProcessMemory(m_hDebuggeeProcess, e->u.DebugString.lpDebugStringData,
    //                             buffer, e->u.DebugString.nDebugStringLength, &nNumberOfBytesRead);
    //if (bSuccess) {
    //    if (e->u.DebugString.fUnicode)
    //        result = QString::fromUtf16(reinterpret_cast<ushort*>(buffer), nNumberOfBytesRead);
    //    else
    //        result = QString::fromLocal8Bit(reinterpret_cast<char*>(buffer), nNumberOfBytesRead);
    //    emit debugOutput(result);
    //}
    //free(buffer);
}
