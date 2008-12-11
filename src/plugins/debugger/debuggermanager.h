/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef DEBUGGER_DEBUGGERMANAGER_H
#define DEBUGGER_DEBUGGERMANAGER_H

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE
class QAction;
class QAbstractItemModel;
class QDockWidget;
class QLabel;
class QMainWindow;
class QModelIndex;
class QSplitter;
class QTimer;
class QWidget;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class DebuggerOutputWindow;
class DebuggerPlugin;
class DebugMode;

class BreakHandler;
class DisassemblerHandler;
class ModulesHandler;
class RegisterHandler;
class StackHandler;
class ThreadsHandler;
class WatchHandler;
class WatchData;
class BreakpointData;


// Note: the Debugger process itself is referred to as 'Debugger',
// whereas the debugged process is referred to as 'Inferior' or 'Debuggee'.

//     DebuggerProcessNotReady
//          |
//     DebuggerProcessStartingUp
//          |
//     DebuggerReady              [R] [N]
//          | <-------------------------------------.
//     DebuggerInferiorRunningRequested             |
//          |                                       |
//     DebuggerInferiorRunning                      |
//          |                                       |
//     DebuggerInferiorStopRequested                |
//          |                                       |
//     DebuggerInferiorStopped                      |
//          |                                       |
//     DebuggerInferiorUpdating                     |
//          |                                       |
//     DebuggerInferiorUpdateFinishing              |
//          |                                       |
//     DebuggerInferiorReady             [C] [N]    |
//          |                                       |
//          `---------------------------------------'
//
// Allowed actions:
//    [R] :  Run
//    [C] :  Continue
//    [N] :  Step, Next



enum DebuggerStatus
{
    DebuggerProcessNotReady,          // Debugger not started
    DebuggerProcessStartingUp,        // Debugger starting up
    DebuggerProcessReady,             // Debugger started, Inferior not yet
                                      // running or already finished

    DebuggerInferiorRunningRequested, // Debuggee requested to run
    DebuggerInferiorRunning,          // Debuggee running
    DebuggerInferiorStopRequested,    // Debuggee running, stop requested
    DebuggerInferiorStopped,          // Debuggee stopped

    DebuggerInferiorUpdating,         // Debuggee updating data views
    DebuggerInferiorUpdateFinishing,  // Debuggee updating data views aborting
    DebuggerInferiorReady,
};


class IDebuggerEngine;
class GdbEngine;
class ScriptEngine;
class WinEngine;

// The construct below is not nice but enforces a bit of order. The
// DebuggerManager interfaces a lots of thing: The DebuggerPlugin,
// the DebuggerEngines, the RunMode, the handlers and views.
// Instead of making the whole interface public, we split in into
// smaller parts and grant friend access only to the classes that
// need it.


//
// IDebuggerManagerAccessForEngines
//

class IDebuggerManagerAccessForEngines
{
public:
    virtual ~IDebuggerManagerAccessForEngines() {}

private:
    // This is the part of the interface that's exclusively seen by the
    // debugger engines.
    friend class GdbEngine;
    friend class ScriptEngine;
    friend class WinEngine;

    // called from the engines after successful startup
    virtual void notifyStartupFinished() = 0; 
    virtual void notifyInferiorStopped() = 0; 
    virtual void notifyInferiorUpdateFinished() = 0; 
    virtual void notifyInferiorRunningRequested() = 0;
    virtual void notifyInferiorRunning() = 0;
    virtual void notifyInferiorExited() = 0;
    virtual void notifyInferiorPidChanged(int) = 0;

    virtual DisassemblerHandler *disassemblerHandler() = 0;
    virtual ModulesHandler *modulesHandler() = 0;
    virtual BreakHandler *breakHandler() = 0;
    virtual RegisterHandler *registerHandler() = 0;
    virtual StackHandler *stackHandler() = 0;
    virtual ThreadsHandler *threadsHandler() = 0;
    virtual WatchHandler *watchHandler() = 0;

    virtual void showApplicationOutput(const QString &prefix, const QString &data) = 0;
    virtual QAction *useCustomDumpersAction() const = 0;
    virtual QAction *debugDumpersAction() const = 0;
    virtual bool skipKnownFrames() const = 0;
    virtual bool debugDumpers() const = 0;
    virtual bool useCustomDumpers() const = 0;
    virtual bool useFastStart() const = 0;

    virtual void reloadDisassembler() = 0;
    virtual void reloadModules() = 0;
    virtual void reloadRegisters() = 0;
};


//
// IDebuggerManagerAccessForDebugMode
//

class IDebuggerManagerAccessForDebugMode
{
public:
    virtual ~IDebuggerManagerAccessForDebugMode() {}

private:
    friend class DebugMode;

    virtual QWidget *threadsWindow() const = 0;
    virtual QLabel *statusLabel() const = 0;
    virtual QList<QDockWidget*> dockWidgets() const = 0;
    virtual void createDockWidgets() = 0;
};


//
// DebuggerManager
//

class DebuggerManager : public QObject,
    public IDebuggerManagerAccessForEngines,
    public IDebuggerManagerAccessForDebugMode
{
    Q_OBJECT

public:
    DebuggerManager();
    ~DebuggerManager();

    IDebuggerManagerAccessForEngines *engineInterface();
    IDebuggerManagerAccessForDebugMode *debugModeInterface();
    QMainWindow *mainWindow() const { return m_mainWindow; }
    QLabel *statusLabel() const { return m_statusLabel; }

    enum StartMode { startInternal, startExternal, attachExternal };
    enum DebuggerType { GdbDebugger, ScriptDebugger, WinDebugger };

public slots:
    bool startNewDebugger(StartMode mode);
    void exitDebugger(); 

    void setSimpleDockWidgetArrangement();
    void setLocked(bool locked);
    void dockToggled(bool on);

    void setBusyCursor(bool on);
    void queryCurrentTextEditor(QString *fileName, int *lineNumber, QObject **ed);
    void querySessionValue(const QString &name, QVariant *value);
    void setSessionValue(const QString &name, const QVariant &value);
    QVariant configValue(const QString &name);
    void queryConfigValue(const QString &name, QVariant *value);
    void setConfigValue(const QString &name, const QVariant &value);
    QVariant sessionValue(const QString &name);

    void gotoLocation(const QString &file, int line, bool setLocationMarker);
    void resetLocation();

    void interruptDebuggingRequest();
    void startExternalApplication();
    void attachExternalApplication();

    void jumpToLineExec();
    void runToLineExec();
    void runToFunctionExec();
    void toggleBreakpoint();
    void breakByFunction();
    void breakByFunction(const QString &functionName);
    void setBreakpoint(const QString &fileName, int lineNumber);
    void watchExpression(const QString &expression);
    void breakAtMain();
    void activateFrame(int index);
    void selectThread(int index);

    void stepExec();
    void stepOutExec();
    void nextExec();
    void stepIExec();
    void nextIExec();
    void continueExec();

    void addToWatchWindow();
    void updateWatchModel();
    void removeWatchExpression(const QString &iname);
    void expandChildren(const QModelIndex &idx);
    void collapseChildren(const QModelIndex &idx);
    
    void sessionLoaded();
    void aboutToSaveSession();

    void assignValueInDebugger(const QString &expr, const QString &value);
    void executeDebuggerCommand(const QString &command);

    void showStatusMessage(const QString &msg, int timeout = -1); // -1 forever

private slots:
    void showDebuggerOutput(const QString &prefix, const QString &msg);
    void showDebuggerInput(const QString &prefix, const QString &msg);
    void showApplicationOutput(const QString &prefix, const QString &msg);

    void reloadDisassembler();
    void disassemblerDockToggled(bool on);

    void reloadModules();
    void modulesDockToggled(bool on);
    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();

    void reloadRegisters();
    void registerDockToggled(bool on);
    void setStatus(int status);
    void clearStatusMessage();

private:
    //
    // Implementation of IDebuggerManagerAccessForEngines
    // 
    DisassemblerHandler *disassemblerHandler() { return m_disassemblerHandler; }
    ModulesHandler *modulesHandler() { return m_modulesHandler; }
    BreakHandler *breakHandler() { return m_breakHandler; }
    RegisterHandler *registerHandler() { return m_registerHandler; }
    StackHandler *stackHandler() { return m_stackHandler; }
    ThreadsHandler *threadsHandler() { return m_threadsHandler; }
    WatchHandler *watchHandler() { return m_watchHandler; }
    QAction *useCustomDumpersAction() const { return m_useCustomDumpersAction; }
    QAction *useToolTipsAction() const { return m_useToolTipsAction; }
    QAction *debugDumpersAction() const { return m_debugDumpersAction; }
    bool skipKnownFrames() const;
    bool debugDumpers() const;
    bool useCustomDumpers() const;
    bool useFastStart() const;

    void notifyStartupFinished();
    void notifyInferiorStopped();
    void notifyInferiorUpdateFinished();
    void notifyInferiorRunningRequested();
    void notifyInferiorRunning();
    void notifyInferiorExited();
    void notifyInferiorPidChanged(int);

    void cleanupViews(); 

    //
    // Implementation of IDebuggerManagerAccessForDebugMode
    //
    QWidget *threadsWindow() const { return m_threadsWindow; }
    QList<QDockWidget*> dockWidgets() const { return m_dockWidgets; }
    void createDockWidgets();

    //    
    // internal implementation
    //  
    Q_SLOT void loadSessionData();
    Q_SLOT void saveSessionData();
    Q_SLOT void dumpLog();

public:
    // stuff in this block should be made private by moving it to 
    // one of the interfaces
    QAbstractItemModel *threadsModel();
    int status() const { return m_status; }
    StartMode startMode() const { return m_startMode; }

signals:
    void debuggingFinished();
    void inferiorPidChanged(qint64 pid);
    void statusChanged(int newstatus);
    void debugModeRequested();
    void previousModeRequested();
    void statusMessageRequested(const QString &msg, int timeout); // -1 for 'forever'
    void gotoLocationRequested(const QString &file, int line, bool setLocationMarker);
    void resetLocationRequested();
    void currentTextEditorRequested(QString *fileName, int *lineNumber, QObject **ob);
    void currentMainWindowRequested(QWidget **);
    void sessionValueRequested(const QString &name, QVariant *value);
    void setSessionValueRequested(const QString &name, const QVariant &value);
    void configValueRequested(const QString &name, QVariant *value);
    void setConfigValueRequested(const QString &name, const QVariant &value);
    void applicationOutputAvailable(const QString &prefix, const QString &msg);


public:
    // FIXME: make private
    QString m_executable;
    QStringList m_environment;
    QString m_workingDir;
    QString m_buildDir;
    QStringList m_processArgs;
    int m_attachedPID;

private:
    void init();
    void setDebuggerType(DebuggerType type);
    QDockWidget *createDockForWidget(QWidget *widget);

    void shutdown();

    void toggleBreakpoint(const QString &fileName, int lineNumber);
    void setToolTipExpression(const QPoint &pos, const QString &exp0);

    StartMode m_startMode;
    DebuggerType m_debuggerType;

    /// Views
    QMainWindow *m_mainWindow;
    QLabel *m_statusLabel;
    QDockWidget *m_breakDock;
    QDockWidget *m_disassemblerDock;
    QDockWidget *m_modulesDock;
    QDockWidget *m_outputDock;
    QDockWidget *m_registerDock;
    QDockWidget *m_stackDock;
    QDockWidget *m_threadsDock;
    QDockWidget *m_watchDock;
    QList<QDockWidget*> m_dockWidgets;

    BreakHandler *m_breakHandler;
    DisassemblerHandler *m_disassemblerHandler;
    ModulesHandler *m_modulesHandler;
    RegisterHandler *m_registerHandler;
    StackHandler *m_stackHandler;
    ThreadsHandler *m_threadsHandler;
    WatchHandler *m_watchHandler;

    /// Actions
    friend class DebuggerPlugin;
    QAction *m_startExternalAction;
    QAction *m_attachExternalAction;
    QAction *m_continueAction;
    QAction *m_stopAction;
    QAction *m_resetAction; // FIXME: Should not be needed in a stable release
    QAction *m_stepAction;
    QAction *m_stepOutAction;
    QAction *m_runToLineAction;
    QAction *m_runToFunctionAction;
    QAction *m_jumpToLineAction;
    QAction *m_nextAction;
    QAction *m_watchAction;
    QAction *m_breakAction;
    QAction *m_breakByFunctionAction;
    QAction *m_breakAtMainAction;
    QAction *m_sepAction;
    QAction *m_stepIAction;
    QAction *m_nextIAction;
    QAction *m_skipKnownFramesAction;

    QAction *m_debugDumpersAction;
    QAction *m_useCustomDumpersAction;
    QAction *m_useFastStartAction;
    QAction *m_useToolTipsAction;
    QAction *m_dumpLogAction;

    QWidget *m_breakWindow;
    QWidget *m_disassemblerWindow;
    QWidget *m_localsWindow;
    QWidget *m_registerWindow;
    QWidget *m_modulesWindow;
    QWidget *m_tooltipWindow;
    QWidget *m_stackWindow;
    QWidget *m_threadsWindow;
    QWidget *m_watchersWindow;
    DebuggerOutputWindow *m_outputWindow;

    int m_status;
    bool m_busy;
    QTimer *m_statusTimer;
    QString m_lastPermanentStatusMessage;

    IDebuggerEngine *engine();
    IDebuggerEngine *m_engine;
};


} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DEBUGGERMANAGER_H
