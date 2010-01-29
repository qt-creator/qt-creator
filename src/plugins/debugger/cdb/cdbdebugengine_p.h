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

#include "coreengine.h"
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

class CdbDebugEnginePrivate : public CdbCore::CoreEngine
{
    Q_OBJECT
public:

    typedef QMap<QString, QString> EditorToolTipCache;

    enum HandleBreakEventMode { // Special modes for break event handler.
        BreakEventHandle,
        BreakEventIgnoreOnce,
        BreakEventSyncBreakPoints,
    };

    explicit CdbDebugEnginePrivate(DebuggerManager *parent,
                                   const QSharedPointer<CdbOptions> &options,
                                   CdbDebugEngine* engine);
    ~CdbDebugEnginePrivate();
    bool init(QString *errorMessage);

    void checkVersion();
    void processCreatedAttached(ULONG64 processHandle, ULONG64 initialThreadHandle);
    void setDebuggeeHandles(HANDLE hDebuggeeProcess,  HANDLE hDebuggeeThread);

    bool isDebuggeeRunning() const { return isWatchTimerRunning(); }
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
    void notifyException(long code, bool fatal);

    enum EndInferiorAction { DetachInferior, TerminateInferior };
    bool endInferior(EndInferiorAction a, QString *errorMessage);

    enum EndDebuggingMode { EndDebuggingDetach, EndDebuggingTerminate, EndDebuggingAuto };
    void endDebugging(EndDebuggingMode em = EndDebuggingAuto);

    void updateCodeLevel();

public slots:
    void handleDebugEvent();

public:
    const QSharedPointer<CdbOptions>  m_options;
    HANDLE                  m_hDebuggeeProcess;
    HANDLE                  m_hDebuggeeThread;
    bool                    m_interrupted;
    int                     m_currentThreadId;
    int                     m_eventThreadId;
    int                     m_interruptArticifialThreadId;
    bool                    m_ignoreInitialBreakPoint;
    HandleBreakEventMode    m_breakEventMode;

    QSharedPointer<CdbDumperHelper> m_dumper;

    CdbDebugEngine *m_engine;
    inline DebuggerManager *manager() const;
    CdbStackTraceContext *m_currentStackTrace;
    EditorToolTipCache m_editorToolTipCache;

    bool m_firstActivatedFrame;
    bool m_inferiorStartupComplete;

    DebuggerStartMode m_mode;
    Utils::ConsoleProcess m_consoleStubProc;
};

enum { messageTimeOut = 5000 };

enum { debugCDB = 0 };
enum { debugCDBExecution = 0 };
enum { debugCDBWatchHandling = 0 };

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CDBENGINEPRIVATE_H

