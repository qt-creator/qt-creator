/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DEBUGGER_CDBENGINEPRIVATE_H
#define DEBUGGER_CDBENGINEPRIVATE_H

#include "cdbdebugeventcallback.h"
#include "cdbdebugoutput.h"
#include "cdboptions.h"
#include "cdbdumperhelper.h"
#include "stackhandler.h"
#include "debuggermanager.h"

#include <utils/consoleprocess.h>
#include <QtCore/QSharedPointer>
#include <QtCore/QMap>

namespace Debugger {
class DebuggerManager;

namespace Internal {

class WatchHandler;
class CdbStackFrameContext;
class CdbStackTraceContext;

// Thin wrapper around the 'DBEng' debugger engine shared library
// which is loaded at runtime.

class DebuggerEngineLibrary
{
public:
    DebuggerEngineLibrary();
    bool init(const QString &path, QString *errorMessage);

    inline HRESULT debugCreate(REFIID interfaceId, PVOID *interfaceHandle) const
        { return m_debugCreate(interfaceId, interfaceHandle); }

private:
    // The exported functions of the library
    typedef HRESULT (STDAPICALLTYPE *DebugCreateFunction)(REFIID, PVOID *);

    DebugCreateFunction m_debugCreate;
};

// A class that sets an expression syntax on the debug control while in scope.
// Can be nested as it checks for the old value.
class SyntaxSetter {
    Q_DISABLE_COPY(SyntaxSetter)
public:
    explicit inline SyntaxSetter(CIDebugControl *ctl, ULONG desiredSyntax);
    inline  ~SyntaxSetter();
private:
    const ULONG m_desiredSyntax;
    CIDebugControl *m_ctl;
    ULONG m_oldSyntax;
};

// helper struct to pass interfaces around
struct CdbComInterfaces
{
    CdbComInterfaces();
    CIDebugClient*          debugClient;
    CIDebugControl*         debugControl;
    CIDebugSystemObjects*   debugSystemObjects;
    CIDebugSymbols*         debugSymbols;
    CIDebugRegisters*       debugRegisters;
    CIDebugDataSpaces*      debugDataSpaces;
};

struct CdbDebugEnginePrivate
{
    typedef QMap<QString, QString> EditorToolTipCache;

    enum HandleBreakEventMode { // Special modes for break event handler.
        BreakEventHandle,
        BreakEventIgnoreOnce,
        BreakEventSyncBreakPoints,
    };

    explicit CdbDebugEnginePrivate(DebuggerManager *parent,
                                   const QSharedPointer<CdbOptions> &options,
                                   CdbDebugEngine* engine);
    bool init(QString *errorMessage);
    ~CdbDebugEnginePrivate();

    void processCreatedAttached(ULONG64 processHandle, ULONG64 initialThreadHandle);
    void setDebuggeeHandles(HANDLE hDebuggeeProcess,  HANDLE hDebuggeeThread);

    bool isDebuggeeRunning() const { return m_watchTimer != -1; }
    void handleDebugEvent();
    ULONG updateThreadList();
    bool setCDBThreadId(unsigned long threadId, QString *errorMessage);
    void updateStackTrace();
    void updateModules();

    void handleBreakpointEvent(PDEBUG_BREAKPOINT2 pBP);
    void cleanStackTrace();
    void clearForRun();
    void handleModuleLoad(const QString &);
    CdbStackFrameContext *getStackFrameContext(int frameIndex, QString *errorMessage) const;
    void clearDisplay();

    bool interruptInterferiorProcess(QString *errorMessage);

    bool continueInferiorProcess(QString *errorMessage = 0);
    bool continueInferior(QString *errorMessage);
    bool executeContinueCommand(const QString &command);

    bool attemptBreakpointSynchronization(QString *errorMessage);
    void notifyCrashed();

    enum EndInferiorAction { DetachInferior, TerminateInferior };
    bool endInferior(EndInferiorAction a, QString *errorMessage);

    enum EndDebuggingMode { EndDebuggingDetach, EndDebuggingTerminate, EndDebuggingAuto };
    void endDebugging(EndDebuggingMode em = EndDebuggingAuto);

    static bool executeDebuggerCommand(CIDebugControl *ctrl, const QString &command, QString *errorMessage);
    static bool evaluateExpression(CIDebugControl *ctrl, const QString &expression, DEBUG_VALUE *v, QString *errorMessage);

    QStringList sourcePaths() const;
    bool setSourcePaths(const QStringList &s, QString *errorMessage);

    QStringList symbolPaths() const;
    bool setSymbolPaths(const QStringList &s, QString *errorMessage);

    bool setCodeLevel();

    const QSharedPointer<CdbOptions>  m_options;
    HANDLE                  m_hDebuggeeProcess;
    HANDLE                  m_hDebuggeeThread;
    bool                    m_interrupted;
    int                     m_currentThreadId;
    int                     m_eventThreadId;
    HandleBreakEventMode    m_breakEventMode;

    int                     m_watchTimer;
    CdbComInterfaces        m_cif;
    CdbDebugEventCallback   m_debugEventCallBack;
    CdbDebugOutput          m_debugOutputCallBack;    
    QSharedPointer<CdbDumperHelper> m_dumper;
    QString                 m_baseImagePath;

    CdbDebugEngine *m_engine;
    inline DebuggerManager *manager() const;
    CdbStackTraceContext *m_currentStackTrace;
    EditorToolTipCache m_editorToolTipCache;

    bool m_firstActivatedFrame;

    DebuggerStartMode m_mode;
    Utils::ConsoleProcess m_consoleStubProc;
};

// helper functions

bool getExecutionStatus(CIDebugControl *ctl, ULONG *executionStatus, QString *errorMessage = 0);
const char *executionStatusString(ULONG executionStatus);
const char *executionStatusString(CIDebugControl *ctl);

// Message
QString msgDebugEngineComResult(HRESULT hr);
QString msgComFailed(const char *func, HRESULT hr);

enum { debugCDB = 0 };
enum { debugCDBWatchHandling = 0 };

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CDBENGINEPRIVATE_H

