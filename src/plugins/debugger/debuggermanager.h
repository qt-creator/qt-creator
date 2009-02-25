/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

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
class SourceFilesWindow;
class WatchData;
class BreakpointData;


// Note: the Debugger process itself is referred to as 'Debugger',
// whereas the debugged process is referred to as 'Inferior' or 'Debuggee'.

//     DebuggerProcessNotReady
//          |
//     DebuggerProcessStartingUp
//          | <-------------------------------------.
//     DebuggerInferiorRunningRequested             |
//          |                                       | 
//     DebuggerInferiorRunning                      | 
//          |                                       |  
//     DebuggerInferiorStopRequested                |
//          |                                       |
//     DebuggerInferiorStopped                      |
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

    DebuggerInferiorRunningRequested, // Debuggee requested to run
    DebuggerInferiorRunning,          // Debuggee running
    DebuggerInferiorStopRequested,    // Debuggee running, stop requested
    DebuggerInferiorStopped,          // Debuggee stopped
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
    virtual void notifyInferiorStopRequested() = 0;
    virtual void notifyInferiorStopped() = 0; 
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
    virtual SourceFilesWindow *sourceFileWindow() = 0;

    virtual void showApplicationOutput(const QString &data) = 0;
    virtual bool skipKnownFrames() const = 0;
    virtual bool debugDumpers() const = 0;
    virtual bool useCustomDumpers() const = 0;
    
    virtual bool wantsAllPluginBreakpoints() const = 0;
    virtual bool wantsSelectedPluginBreakpoints() const = 0;
    virtual bool wantsNoPluginBreakpoints() const = 0;
    virtual QString selectedPluginBreakpointsPattern() const = 0;

    virtual void reloadDisassembler() = 0;
    virtual void reloadModules() = 0;
    virtual void reloadSourceFiles() = 0;
    virtual void reloadRegisters() = 0;
};


//
// DebuggerSettings
//

class DebuggerSettings
{
public:
    DebuggerSettings();

public:
    QString m_gdbCmd;
    QString m_gdbEnv;
    bool m_autoRun;
    bool m_autoQuit;

    bool m_useCustomDumpers;
    bool m_skipKnownFrames;
    bool m_debugDumpers;
    bool m_useFastStart;
    bool m_useToolTips;

    QString m_scriptFile;

    bool m_pluginAllBreakpoints;
    bool m_pluginSelectedBreakpoints;
    bool m_pluginNoBreakpoints;
    QString m_pluginSelectedBreakpointsPattern;
};

//
// DebuggerManager
//

class DebuggerManager : public QObject,
    public IDebuggerManagerAccessForEngines
{
    Q_OBJECT

public:
    DebuggerManager();
    ~DebuggerManager();

    IDebuggerManagerAccessForEngines *engineInterface();
    QMainWindow *mainWindow() const { return m_mainWindow; }
    QLabel *statusLabel() const { return m_statusLabel; }
    DebuggerSettings *settings() { return &m_settings; }

    enum StartMode { StartInternal, StartExternal, AttachExternal };
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

    void setUseCustomDumpers(bool on);
    void setDebugDumpers(bool on);
    void setSkipKnownFrames(bool on);

private slots:
    void showDebuggerOutput(const QString &prefix, const QString &msg);
    void showDebuggerInput(const QString &prefix, const QString &msg);
    void showApplicationOutput(const QString &data);

    void reloadDisassembler();
    void disassemblerDockToggled(bool on);

    void reloadSourceFiles();
    void sourceFilesDockToggled(bool on);

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
    SourceFilesWindow *sourceFileWindow() { return m_sourceFilesWindow; }

    bool skipKnownFrames() const;
    bool debugDumpers() const;
    bool useCustomDumpers() const;
    bool wantsAllPluginBreakpoints() const
        { return m_settings.m_pluginAllBreakpoints; }
    bool wantsSelectedPluginBreakpoints() const
        { return m_settings.m_pluginSelectedBreakpoints; }
    bool wantsNoPluginBreakpoints() const
        { return m_settings.m_pluginNoBreakpoints; }
    QString selectedPluginBreakpointsPattern() const
        { return m_settings.m_pluginSelectedBreakpointsPattern; }

    void notifyInferiorStopped();
    void notifyInferiorRunningRequested();
    void notifyInferiorStopRequested();
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
    void applicationOutputAvailable(const QString &output);

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
    QDockWidget *m_sourceFilesDock;
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
    SourceFilesWindow *m_sourceFilesWindow;

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
    DebuggerSettings m_settings;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DEBUGGERMANAGER_H
