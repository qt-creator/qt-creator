/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DEBUGGER_DEBUGGERENGINE_H
#define DEBUGGER_DEBUGGERENGINE_H

#include "debugger_global.h"
#include "debuggerconstants.h"
#include "moduleshandler.h" // For 'Symbols'

#include <coreplugin/ssh/sshconnection.h> 

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QDebug;
class QPoint;
QT_END_NAMESPACE

namespace TextEditor {
class ITextEditor;
}

namespace Core {
class IOptionsPage;
}

namespace Debugger {

class DebuggerRunControl;
class DebuggerPlugin;

class DEBUGGER_EXPORT DebuggerStartParameters
{
public:
    DebuggerStartParameters();
    void clear();

    QString executable;
    QString displayName;
    QString coreFile;
    QStringList processArgs;
    QStringList environment;
    QString workingDirectory;
    qint64 attachPID;
    bool useTerminal;
    bool breakAtMain;
    QString crashParameter; // for AttachCrashedExternal
    // for remote debugging
    QString remoteChannel;
    QString remoteArchitecture;
    QString symbolFileName;
    QString serverStartScript;
    QString sysRoot;
    QString debuggerCommand;
    int toolChainType;
    QByteArray remoteDumperLib;
    QString qtInstallPath;

    QString dumperLibrary;
    QStringList dumperLibraryLocations;
    Core::SshConnectionParameters connParams;
    DebuggerStartMode startMode;
};

DEBUGGER_EXPORT QDebug operator<<(QDebug str, const DebuggerStartParameters &);
DEBUGGER_EXPORT QDebug operator<<(QDebug str, DebuggerState state);

namespace Internal {

class DisassemblerViewAgent;
class MemoryViewAgent;
class Symbol;
class WatchData;
class BreakHandler;
class ModulesHandler;
class RegisterHandler;
class StackHandler;
class StackFrame;
class SnapshotHandler;
class SourceFilesHandler;
class ThreadsHandler;
class WatchHandler;

class DebuggerEnginePrivate;

// FIXME: DEBUGGER_EXPORT?
class DEBUGGER_EXPORT DebuggerEngine : public QObject
{
    Q_OBJECT

public:
    DebuggerEngine(const DebuggerStartParameters &sp);
    virtual ~DebuggerEngine();

    virtual void setToolTipExpression(const QPoint & /* mousePos */,
            TextEditor::ITextEditor * /* editor */, int /* cursorPos */) { }
    void initializeFromTemplate(DebuggerEngine *other);

    virtual void updateWatchData(const WatchData & /* data */) { }
    void startDebugger(DebuggerRunControl *runControl);
    virtual bool isSessionEngine() const { return false; }

    virtual void watchPoint(const QPoint &) {}
    virtual void fetchMemory(MemoryViewAgent *, QObject *,
            quint64 addr, quint64 length);
    virtual void fetchDisassembler(DisassemblerViewAgent *) {}
    virtual void activateFrame(int index) { Q_UNUSED(index); }

    virtual void reloadModules() {}
    virtual void loadSymbols(const QString &moduleName)
        { Q_UNUSED(moduleName); }
    virtual void loadAllSymbols() {}
    virtual void requestModuleSymbols(const QString &moduleName)
        { Q_UNUSED(moduleName); }

    virtual void reloadRegisters() {}
    virtual void reloadSourceFiles() {}
    virtual void reloadFullStack() {}

    virtual void setRegisterValue(int regnr, const QString &value);
    virtual void addOptionPages(QList<Core::IOptionsPage*> *) const {}
    virtual unsigned debuggerCapabilities() const { return 0; }

    virtual bool isSynchroneous() const { return false; }
    virtual QString qtNamespace() const { return QString(); }

    virtual void makeSnapshot() {}
    virtual void activateSnapshot(int index) { Q_UNUSED(index); }

    virtual void attemptBreakpointSynchronization() {}
    virtual void selectThread(int index) { Q_UNUSED(index); }

    virtual void assignValueInDebugger(const QString &expr, const QString &value)
        { Q_UNUSED(expr); Q_UNUSED(value); }

protected:
    virtual void detachDebugger() {}
    virtual void executeStep() {}
    virtual void executeStepOut()  {}
    virtual void executeNext() {}
    virtual void executeStepI() {}
    virtual void executeNextI() {}
    virtual void executeReturn() {}

    virtual void continueInferior() {}
    virtual void interruptInferior() {}

    virtual void executeRunToLine(const QString &fileName, int lineNumber)
        { Q_UNUSED(fileName); Q_UNUSED(lineNumber); }
    virtual void executeRunToFunction(const QString &functionName)
        { Q_UNUSED(functionName); }
    virtual void executeJumpToLine(const QString &fileName, int lineNumber)
        { Q_UNUSED(fileName); Q_UNUSED(lineNumber); }
    virtual void executeDebuggerCommand(const QString &command)
        { Q_UNUSED(command); }

    virtual void frameUp();
    virtual void frameDown();

public:
    DebuggerPlugin *plugin() const;
    const DebuggerStartParameters &startParameters() const;
    DebuggerStartParameters &startParameters();

    ModulesHandler *modulesHandler() const;
    BreakHandler *breakHandler() const;
    RegisterHandler *registerHandler() const;
    StackHandler *stackHandler() const;
    ThreadsHandler *threadsHandler() const;
    WatchHandler *watchHandler() const;
    SnapshotHandler *snapshotHandler() const;
    SourceFilesHandler *sourceFilesHandler() const;

    QAbstractItemModel *commandModel() const;
    QAbstractItemModel *modulesModel() const;
    QAbstractItemModel *breakModel() const;
    QAbstractItemModel *registerModel() const;
    QAbstractItemModel *stackModel() const;
    QAbstractItemModel *threadsModel() const;
    QAbstractItemModel *localsModel() const;
    QAbstractItemModel *watchersModel() const;
    QAbstractItemModel *returnModel() const;
    QAbstractItemModel *snapshotModel() const;
    QAbstractItemModel *sourceFilesModel() const;

    void handleFinished();
    bool debuggerActionsEnabled() const;
    static bool debuggerActionsEnabled(DebuggerState state);
    void showModuleSymbols(const QString &moduleName, const Symbols &symbols);

    void breakByFunction(const QString &functionName);
    void breakByFunctionMain();
    
    void executeStepX();
    void executeStepOutX();
    void executeStepNextX();
    void executeReturnX();

    DebuggerState state() const;
    DebuggerState lastGoodState() const;
    DebuggerState targetState() const;
    bool isDying() const { return targetState() == DebuggerFinished; }

    // Dumper stuff (common to cdb and gdb).
    bool qtDumperLibraryEnabled() const;
    QString qtDumperLibraryName() const;
    QStringList qtDumperLibraryLocations() const;
    void showQtDumperLibraryWarning(const QString &details = QString());

    static const char *stateName(int s);

    void notifyInferiorPid(qint64 pid);
    qint64 inferiorPid() const;
    bool isReverseDebugging() const;
    void handleCommand(int role, const QVariant &value);

    // Convenience
    void showMessage(const QString &msg, int channel = LogDebug, int timeout = -1) const;
    void showStatusMessage(const QString &msg, int timeout = -1) const;

    void resetLocation();
    void openFile(const QString &fileName, int lineNumber = -1);
    void gotoLocation(const QString &fileName, int lineNumber, bool setMarker);
    void gotoLocation(const StackFrame &frame, bool setMarker);
    Q_SLOT void raiseApplication();
    virtual void quitDebugger(); // called by DebuggerRunControl

protected:
    void notifyEngineSetupOk();
    void notifyEngineSetupFailed();
    void notifyEngineRunFailed();

    void notifyInferiorSetupOk();
    void notifyInferiorSetupFailed();

    void notifyEngineRunAndInferiorRunOk();
    void notifyEngineRunAndInferiorStopOk();
    void notifyInferiorUnrunnable(); // Called by CoreAdapter.
    //void notifyInferiorSpontaneousRun();

    void notifyInferiorRunRequested();
    void notifyInferiorRunOk();
    void notifyInferiorRunFailed();

    void notifyInferiorStopOk();
    void notifyInferiorSpontaneousStop();
    void notifyInferiorStopFailed();
    void notifyInferiorExited();

    void notifyInferiorShutdownOk();
    void notifyInferiorShutdownFailed();

    void notifyEngineSpontaneousShutdown();
    void notifyEngineShutdownOk();
    void notifyEngineShutdownFailed();

    void notifyInferiorIll();
    void notifyEngineIll();

    virtual void setupEngine() = 0;
    virtual void setupInferior() = 0;
    virtual void runEngine() = 0;
    virtual void shutdownInferior() = 0;
    virtual void shutdownEngine() = 0;

//private: // FIXME. State transitions 
    void setState(DebuggerState state, bool forced = false);

private:
    void executeRunToLine();
    void executeRunToFunction();
    void executeJumpToLine();
    void addToWatchWindow();

    friend class DebuggerEnginePrivate;
    DebuggerEnginePrivate *d;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DEBUGGERENGINE_H
