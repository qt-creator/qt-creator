#pragma once

#include "windbgeventcallback.h"

#include <QObject>
#include <QVector>

#define DBGHELP_TRANSLATE_TCHAR
#include <Dbghelp.h>

class WinDbgThread;

class Debugger : public QObject
{
    Q_OBJECT
public:
    Debugger(QObject* parent = 0);
    ~Debugger();

    void openProcess(const QString& filename);
    void closeProcess();
    void breakAtCurrentPosition();
    void continueProcess();

    struct StackFrame
    {
        QString symbol;
        QString filename;
        uint    line;
    };

    typedef QVector<StackFrame> StackTrace;
    StackTrace stackTrace() { return m_currentStackTrace; }

signals:
    void debugOutput(const QString&);
    void debuggeePaused();

protected:
    void timerEvent(QTimerEvent*);

private:
    void handleDebugEvent();
    void handleCreateProcessEvent(DEBUG_EVENT* e);
    void handleExceptionEvent(DEBUG_EVENT* e);
    void handleOutputDebugStringEvent(DEBUG_EVENT* e);

private:
    HANDLE           m_hDebuggeeProcess;
    HANDLE           m_hDebuggeeThread;
    HANDLE           m_hDebuggeeImage;
    StackTrace       m_currentStackTrace;
    //DWORD64 m_dwModuleBaseAddress;

    int                   m_watchTimer;
    IDebugClient5*        m_pDebugClient;
    IDebugControl4*       m_pDebugControl;
    IDebugSystemObjects4* m_pDebugSystemObjects;
    IDebugSymbols3*       m_pDebugSymbols;
    IDebugRegisters2*     m_pDebugRegisters;
    WinDbgEventCallback   m_callbackEvent;

    //struct ThreadInfo
    //{
    //    ULONG64 handle, dataOffset, startOffset;
    //};

    //QVector<ThreadInfo>   m_threadlist;

    friend class WinDbgEventCallback;
};
