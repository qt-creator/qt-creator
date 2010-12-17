/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_CDBENGINEPRIVATE_H
#define DEBUGGER_CDBENGINEPRIVATE_H

#include "coreengine.h"
#include "debuggerconstants.h"
#include "cdboptions.h"
#include "cdbdumperhelper.h"
#include "stackhandler.h"

#include <utils/consoleprocess.h>

#include <QtCore/QSharedPointer>
#include <QtCore/QMap>

namespace Debugger {
namespace Internal {

class WatchHandler;
class CdbStackTraceContext;
class CdbSymbolGroupContext;

class CdbEnginePrivate : public CdbCore::CoreEngine
{
    Q_OBJECT
public:

    typedef QMap<QString, QString> EditorToolTipCache;

    enum HandleBreakEventMode { // Special modes for break event handler.
        BreakEventHandle,
        BreakEventIgnoreOnce,
        BreakEventSyncBreakPoints,
    };

    enum StoppedReason {
        StoppedCrash,
        StoppedBreakpoint,
        StoppedOther
    };

    explicit CdbEnginePrivate(CdbEngine* engine);
    ~CdbEnginePrivate();
    bool init(QString *errorMessage);

    void checkVersion();
    void processCreatedAttached(ULONG64 processHandle, ULONG64 initialThreadHandle);
    void setDebuggeeHandles(HANDLE hDebuggeeProcess,  HANDLE hDebuggeeThread);

    bool isDebuggeeRunning() const { return isWatchTimerRunning(); }
    ULONG updateThreadList(const QString &currentThreadState = QString());
    bool setCDBThreadId(unsigned long threadId, QString *errorMessage);
    void updateStackTrace();
    void updateModules();

    void handleBreakpointEvent(PDEBUG_BREAKPOINT2 pBP);
    void cleanStackTrace();
    void clearForRun();
    void handleModuleLoad(quint64 offset, const QString &);
    void handleModuleUnload(const QString &name);
    CdbSymbolGroupContext *getSymbolGroupContext(int frameIndex, QString *errorMessage) const;
    void clearDisplay();

    bool interruptInterferiorProcess(QString *errorMessage);

    bool continueInferiorProcess(QString *errorMessage = 0);
    bool continueInferior(QString *errorMessage);
    bool executeContinueCommand(const QString &command);

    void notifyException(long code, bool fatal, const QString &message);


    bool endInferior(bool detachOnly /* = false */, QString *errorMessage);

    void endDebugging(bool detachOnly = false);

    void updateCodeLevel();

    QString stoppedMessage(const StackFrame *topFrame = 0) const;

private slots:
    void handleDebugEvent();
    void slotModulesLoaded();

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

    CdbEngine *m_engine;
    CdbStackTraceContext *m_currentStackTrace;
    EditorToolTipCache m_editorToolTipCache;

    bool m_firstActivatedFrame;
    bool m_inferiorStartupComplete;

    DebuggerStartMode m_mode;
    Utils::ConsoleProcess m_consoleStubProc;

    StoppedReason m_stoppedReason;
    QString m_stoppedMessage;
};

enum { messageTimeOut = 5000 };

enum { debugCDB = 0 };
enum { debugCDBExecution = 0 };
enum { debugCDBWatchHandling = 0 };
enum { debugToolTips = 0 };
enum { debugBreakpoints = 0 };

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CDBENGINEPRIVATE_H

