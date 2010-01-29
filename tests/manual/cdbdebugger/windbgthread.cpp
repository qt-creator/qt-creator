#include "windbgthread.h"
#include <QDebug>

#define DBGHELP_TRANSLATE_TCHAR
#include <Dbghelp.h>

WinDbgThread::WinDbgThread(QObject* parent)
:   QThread(parent),
    m_state(Idle)
{
}

WinDbgThread::~WinDbgThread()
{
    stopProcess();
}

void WinDbgThread::startProcess(const QString& filename)
{
    stopProcess();
    m_bOwnsProcess = true;
    m_processFileName = filename;
    m_pi.dwProcessId = 0;
    QThread::start();
}

void WinDbgThread::stopProcess()
{
    if (!QThread::isRunning())
        return;

    switch (m_state)
    {
    case ProcessRunning:
        if (m_bOwnsProcess) {
            m_bOwnsProcess = false; // don't terminate in the loop again
            TerminateProcess(m_pi.hProcess, 0);
        }
        // don't break here
    case ProcessPaused:
        m_bAbortEventPollingLoop = true;
        resume();
        break;
    }
    QThread::wait(5000);
    if (QThread::isRunning()) {
        qWarning("WinDbgThread still running... terminating!");
        QThread::terminate();
    }
}

void WinDbgThread::attachToProcess(DWORD processId)
{
    m_bOwnsProcess = false;
    m_processFileName = QString();
    m_pi.dwProcessId = processId;
    QThread::start();
}

void WinDbgThread::run()
{
    qDebug() << "WinDbgThread started";
    // start process or attach process
    if (m_bOwnsProcess) {
        // create new process
        internalStartProcess();
    } else {
        // attach to process
        qWarning("attach to process not yet implemented");
        return;
    }

    m_hThisThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, GetCurrentThreadId());
    if (!m_hThisThread) {
        qWarning("WinDbgThread: can't open thread handle");
        return;
    }

    DEBUG_EVENT debugEvent;
    m_bAbortEventPollingLoop = false;
    while (WaitForDebugEvent(&debugEvent, INFINITE)) {
        setState(ProcessPaused);
        emit debugEventOccured(&debugEvent);
        suspend();
        if (m_bAbortEventPollingLoop)
            break;
        ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE);
        setState(ProcessRunning);
    }

    setState(Idle);
    if (m_bOwnsProcess) {
        TerminateProcess(m_pi.hProcess, 0);
    }
    CloseHandle(m_pi.hProcess);
    CloseHandle(m_pi.hThread);
    CloseHandle(m_hThisThread);

    qDebug() << "WinDbgThread finished";
}

void WinDbgThread::continueProcess()
{
    if (m_state == ProcessPaused)
        resume();
}

void WinDbgThread::pauseProcess()
{
    if (m_state == ProcessRunning)
        DebugBreakProcess(m_pi.hProcess);
}

void WinDbgThread::internalStartProcess()
{
    BOOL bSuccess;

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.wShowWindow = TRUE;

    ZeroMemory(&m_pi, sizeof(m_pi));

    DWORD dwCreationFlags = DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS;
    bSuccess = CreateProcess(m_processFileName.utf16(), NULL, NULL, NULL, FALSE,
                             dwCreationFlags,
                             NULL, NULL, &si, &m_pi
                            );

    if (bSuccess)
        setState(ProcessRunning);
    else
        setState(Idle);
}

void WinDbgThread::setState(State s)
{
    m_state = s;
}

void WinDbgThread::suspend()
{
    SuspendThread(m_hThisThread);
}

void WinDbgThread::resume()
{
    ResumeThread(m_hThisThread);
}
