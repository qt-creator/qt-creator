#ifndef __MSVCDEBUGENGINE_H__
#define __MSVCDEBUGENGINE_H__

#include "idebuggerengine.h"
#include "msvcdebugeventcallback.h"
#include "msvcdebugoutput.h"
#include <windows.h>

namespace Debugger {
namespace Internal {

class DebuggerManager;
class IDebuggerManagerAccessForEngines;

class MSVCDebugEngine : public IDebuggerEngine
{
    Q_OBJECT
public:
    MSVCDebugEngine(DebuggerManager *parent);
    ~MSVCDebugEngine();

    virtual void shutdown();
    virtual void setToolTipExpression(const QPoint &pos, const QString &exp);
    virtual bool startDebugger();
    virtual void exitDebugger();
    virtual void updateWatchModel();

    virtual void stepExec();
    virtual void stepOutExec();
    virtual void nextExec();
    virtual void stepIExec();
    virtual void nextIExec();
    
    virtual void continueInferior();
    virtual void runInferior();
    virtual void interruptInferior();

    virtual void runToLineExec(const QString &fileName, int lineNumber);
    virtual void runToFunctionExec(const QString &functionName);
    virtual void jumpToLineExec(const QString &fileName, int lineNumber);
    virtual void assignValueInDebugger(const QString &expr, const QString &value);
    virtual void executeDebuggerCommand(const QString &command);

    virtual void activateFrame(int index);
    virtual void selectThread(int index);

    virtual void attemptBreakpointSynchronization();

    virtual void loadSessionData();
    virtual void saveSessionData();

    virtual void reloadDisassembler();

    virtual void reloadModules();
    virtual void loadSymbols(const QString &moduleName);
    virtual void loadAllSymbols();

    virtual void reloadRegisters();

protected:
    void timerEvent(QTimerEvent*);

private:
    void startWatchTimer();
    void killWatchTimer();
    bool isDebuggeeRunning() { return m_watchTimer != -1; }
    void handleDebugEvent();
    void updateThreadList();
    void updateStackTrace();
    void handleDebugOutput(const char* szOutputString);
    void handleBreakpointEvent(PDEBUG_BREAKPOINT pBP);

private:
    HANDLE                  m_hDebuggeeProcess;
    HANDLE                  m_hDebuggeeThread;
    int                     m_currentThreadId;
    bool                    m_bIgnoreNextDebugEvent;

    int                     m_watchTimer;
    IDebugClient5*          m_pDebugClient;
    IDebugControl4*         m_pDebugControl;
    IDebugSystemObjects4*   m_pDebugSystemObjects;
    IDebugSymbols3*         m_pDebugSymbols;
    IDebugRegisters2*       m_pDebugRegisters;
    MSVCDebugEventCallback  m_debugEventCallBack;
    MSVCDebugOutput         m_debugOutputCallBack;

    DebuggerManager *q;
    IDebuggerManagerAccessForEngines *qq;

    friend class MSVCDebugEventCallback;
    friend class MSVCDebugOutput;
};

} // namespace Internal
} // namespace Debugger

#endif
