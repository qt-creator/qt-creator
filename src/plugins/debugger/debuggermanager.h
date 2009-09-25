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

#ifndef DEBUGGER_DEBUGGERMANAGER_H
#define DEBUGGER_DEBUGGERMANAGER_H

#include "debuggerconstants.h"

#include <utils/fancymainwindow.h>

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QAction;
class QAbstractItemModel;
class QDockWidget;
class QLabel;
class QMessageBox;
class QModelIndex;
class QPoint;
class QTimer;
class QWidget;
class QDebug;
QT_END_NAMESPACE

namespace Core {
class IOptionsPage;
} // namespace Core

namespace TextEditor {
class ITextEditor;
}

namespace Debugger {
namespace Internal {

typedef QLatin1Char _c;
typedef QLatin1String __;
inline QString _(const char *s) { return QString::fromLatin1(s); }
inline QString _(const QByteArray &ba) { return QString::fromLatin1(ba, ba.size()); }

class DebuggerOutputWindow;
class DebuggerRunControl;
class DebuggerPlugin;
class DebugMode;

class BreakHandler;
class BreakpointData;
class ModulesHandler;
class RegisterHandler;
class SourceFilesWindow;
struct StackFrame;
class StackHandler;
class Symbol;
class ThreadsHandler;
class WatchData;
class WatchHandler;

class DebuggerStartParameters
{
public:
    DebuggerStartParameters();
    void clear();

    QString executable;
    QString coreFile;
    QStringList processArgs;
    QStringList environment;
    QString workingDir;
    QString buildDir;
    qint64 attachPID;
    bool useTerminal;
    QString crashParameter; // for AttachCrashedExternal
    // for remote debugging
    QString remoteChannel;
    QString remoteArchitecture;
    QString serverStartScript;
    int toolChainType;

    QString dumperLibrary;
    QStringList dumperLibraryLocations;
    DebuggerStartMode startMode;
};

typedef QSharedPointer<DebuggerStartParameters> DebuggerStartParametersPtr;
QDebug operator<<(QDebug str, const DebuggerStartParameters &);

class IDebuggerEngine;
class GdbEngine;
class ScriptEngine;
class CdbDebugEngine;
struct CdbDebugEnginePrivate;

// Flags for initialization
enum DebuggerEngineTypeFlags
{
    GdbEngineType     = 0x01,
    ScriptEngineType  = 0x02,
    CdbEngineType     = 0x04,
    TcfEngineType     = 0x08,
    AllEngineTypes = GdbEngineType
        | ScriptEngineType 
        | CdbEngineType 
        | TcfEngineType
};

QDebug operator<<(QDebug d, DebuggerState state);

//
// DebuggerManager
//

class DebuggerManager : public QObject
{
    Q_OBJECT

public:
    DebuggerManager();
    ~DebuggerManager();

    friend class CdbDebugEngine;
    friend class CdbDebugEventCallback;
    friend class CdbDumperHelper;
    friend class CdbExceptionLoggerEventCallback;
    friend class GdbEngine;
    friend class ScriptEngine;
    friend class TcfEngine;
    friend struct CdbDebugEnginePrivate;

    QList<Core::IOptionsPage*> initializeEngines(unsigned enabledTypeFlags);

    Core::Utils::FancyMainWindow *mainWindow() const { return m_mainWindow; }
    QLabel *statusLabel() const { return m_statusLabel; }
    IDebuggerEngine *currentEngine() const { return m_engine; }

    virtual DebuggerStartParametersPtr startParameters() const;
    virtual qint64 inferiorPid() const;

    void showMessageBox(int icon, const QString &title, const QString &text);

public slots:
    void startNewDebugger(const DebuggerStartParametersPtr &sp);
    void exitDebugger();

    void setSimpleDockWidgetArrangement();

    void setBusyCursor(bool on);
    void queryCurrentTextEditor(QString *fileName, int *lineNumber, QObject **ed);

    void gotoLocation(const StackFrame &frame, bool setLocationMarker);
    void fileOpen(const QString &file);
    void resetLocation();

    void interruptDebuggingRequest();

    void jumpToLineExec();
    void runToLineExec();
    void runToFunctionExec();
    void toggleBreakpoint();
    void breakByFunction(const QString &functionName);
    void breakByFunctionMain();
    void setBreakpoint(const QString &fileName, int lineNumber);
    void activateFrame(int index);
    void selectThread(int index);

    void stepExec();
    void stepOutExec();
    void nextExec();
    void continueExec();
    void detachDebugger();

    void addToWatchWindow();
    void updateWatchData(const WatchData &data);

    void sessionLoaded();
    void aboutToUnloadSession();
    void aboutToSaveSession();
    QVariant sessionValue(const QString &name);
    void setSessionValue(const QString &name, const QVariant &value);

    void assignValueInDebugger();
    void assignValueInDebugger(const QString &expr, const QString &value);

    void executeDebuggerCommand();
    void executeDebuggerCommand(const QString &command);

    void watchPoint();
    void setRegisterValue(int nr, const QString &value);

    void showStatusMessage(const QString &msg, int timeout = -1); // -1 forever

private slots:
    void showDebuggerOutput(const QString &msg)
        { showDebuggerOutput(LogDebug, msg); }
    void showDebuggerOutput(int channel, const QString &msg);
    void showDebuggerInput(int channel, const QString &msg);
    void showApplicationOutput(const QString &data);

    void reloadSourceFiles();
    void sourceFilesDockToggled(bool on);

    void reloadModules();
    void modulesDockToggled(bool on);
    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();

    void reloadRegisters();
    void registerDockToggled(bool on);
    void clearStatusMessage();
    void attemptBreakpointSynchronization();
    void reloadFullStack();
    void stepByInstructionTriggered();
    void startFailed();

private:
    ModulesHandler *modulesHandler() { return m_modulesHandler; }
    BreakHandler *breakHandler() { return m_breakHandler; }
    RegisterHandler *registerHandler() { return m_registerHandler; }
    StackHandler *stackHandler() { return m_stackHandler; }
    ThreadsHandler *threadsHandler() { return m_threadsHandler; }
    WatchHandler *watchHandler() { return m_watchHandler; }
    SourceFilesWindow *sourceFileWindow() { return m_sourceFilesWindow; }
    QWidget *threadsWindow() const { return m_threadsWindow; }

    void notifyInferiorStopped();
    void notifyInferiorRunning();
    void notifyInferiorExited();
    void notifyInferiorPidChanged(qint64);
    void notifyEngineFinished();

    void cleanupViews();

    DebuggerState state() const;
    void setState(DebuggerState state);
    DebuggerState m_state;

    //
    // internal implementation
    //
    bool qtDumperLibraryEnabled() const;
    QString qtDumperLibraryName() const;
    QStringList qtDumperLibraryLocations() const;
    void showQtDumperLibraryWarning(const QString &details = QString());
    bool isReverseDebugging() const;
    QAbstractItemModel *threadsModel();

    Q_SLOT void loadSessionData();
    Q_SLOT void saveSessionData();
    Q_SLOT void dumpLog();

public:
    // stuff in this block should be made private by moving it to
    // one of the interfaces
    int status() const { return m_status; }
    QList<Symbol> moduleSymbols(const QString &moduleName);

signals:
    void debuggingFinished();
    void inferiorPidChanged(qint64 pid);
    void stateChanged(int newstatus);
    void debugModeRequested();
    void previousModeRequested();
    void statusMessageRequested(const QString &msg, int timeout); // -1 for 'forever'
    void gotoLocationRequested(const StackFrame &frame, bool setLocationMarker);
    void resetLocationRequested();
    void currentTextEditorRequested(QString *fileName, int *lineNumber, QObject **ob);
    void sessionValueRequested(const QString &name, QVariant *value);
    void setSessionValueRequested(const QString &name, const QVariant &value);
    void configValueRequested(const QString &name, QVariant *value);
    void setConfigValueRequested(const QString &name, const QVariant &value);
    void applicationOutputAvailable(const QString &output);

private:
    void init();
    void runTest(const QString &fileName);
    Q_SLOT void createNewDock(QWidget *widget);

    void shutdown();

    void toggleBreakpoint(const QString &fileName, int lineNumber);
    void toggleBreakpointEnabled(const QString &fileName, int lineNumber);
    BreakpointData *findBreakpoint(const QString &fileName, int lineNumber);
    void setToolTipExpression(const QPoint &mousePos,
        TextEditor::ITextEditor *editor, int cursorPos);

    // FIXME: Remove engine-specific state
    DebuggerStartParametersPtr m_startParameters;
    qint64 m_inferiorPid;


    /// Views
    Core::Utils::FancyMainWindow *m_mainWindow;
    QLabel *m_statusLabel;
    QDockWidget *m_breakDock;
    QDockWidget *m_modulesDock;
    QDockWidget *m_outputDock;
    QDockWidget *m_registerDock;
    QDockWidget *m_stackDock;
    QDockWidget *m_sourceFilesDock;
    QDockWidget *m_threadsDock;
    QDockWidget *m_watchDock;

    BreakHandler *m_breakHandler;
    ModulesHandler *m_modulesHandler;
    RegisterHandler *m_registerHandler;
    StackHandler *m_stackHandler;
    ThreadsHandler *m_threadsHandler;
    WatchHandler *m_watchHandler;
    SourceFilesWindow *m_sourceFilesWindow;

    /// Actions
    friend class DebuggerPlugin;
    friend class IDebuggerEngine;
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
    QAction *m_sepAction;
    QAction *m_reverseDirectionAction;

    QWidget *m_breakWindow;
    QWidget *m_localsWindow;
    QWidget *m_registerWindow;
    QWidget *m_modulesWindow;
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
