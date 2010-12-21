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

#ifndef DEBUGGER_DEBUGGERENGINE_H
#define DEBUGGER_DEBUGGERENGINE_H

#include "debugger_global.h"
#include "debuggerconstants.h"
#include "moduleshandler.h" // For 'Symbols'
#include "breakpoint.h" // For 'BreakpointId'
#include "stackframe.h"

#include <coreplugin/ssh/sshconnection.h>

#include <utils/environment.h>

#include <QtCore/QObject>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QDebug;
class QPoint;
class QMessageBox;
QT_END_NAMESPACE

namespace TextEditor {
class ITextEditor;
}

namespace Core {
class IOptionsPage;
}

namespace Debugger {

class DebuggerEnginePrivate;
class DebuggerRunControl;

class DEBUGGER_EXPORT DebuggerStartParameters
{
public:
    DebuggerStartParameters();
    QString toolChainName() const;

    QString executable;
    QString displayName;
    QString coreFile;
    bool isSnapshot; // set if created internally
    QString processArgs;
    Utils::Environment environment;
    QString workingDirectory;
    qint64 attachPID;
    bool useTerminal;

    // Used by AttachCrashedExternal.
    QString crashParameter;

    // Used by Qml debugging.
    QString qmlServerAddress;
    quint16 qmlServerPort;
    QString projectBuildDir;
    QString projectDir;

    // Used by combined cpp+qml debugging.
    DebuggerEngineType cppEngineType;

    // Used by remote debugging.
    QString remoteChannel;
    QString remoteArchitecture;
    QString gnuTarget;
    QString symbolFileName;
    bool useServerStartScript;
    QString serverStartScript;
    QString sysRoot;
    QByteArray remoteDumperLib;
    QByteArray remoteSourcesDir;
    QString remoteMountPoint;
    QString localMountDir;
    Core::SshConnectionParameters connParams;

    QString debuggerCommand;
    int toolChainType;
    QString qtInstallPath;

    QString dumperLibrary;
    QStringList dumperLibraryLocations;
    DebuggerStartMode startMode;

    // for SymbianOS debugging
    quint32 executableUid;
};

DEBUGGER_EXPORT QDebug operator<<(QDebug str, const DebuggerStartParameters &);
DEBUGGER_EXPORT QDebug operator<<(QDebug str, DebuggerState state);

namespace Internal {

class DebuggerPluginPrivate;
class DisassemblerAgent;
class MemoryAgent;
class WatchData;
class BreakHandler;
class ModulesHandler;
class RegisterHandler;
class StackHandler;
class StackFrame;
class SourceFilesHandler;
class ThreadsHandler;
class WatchHandler;
class BreakpointParameters;
class QmlCppEngine;

struct WatchUpdateFlags
{
    WatchUpdateFlags() : tryIncremental(false) {}
    bool tryIncremental;
};

class Location
{
public:
    Location() { init(); }
    Location(quint64 address) { init(); m_address = address; }
    Location(const QString &file) { init(); m_fileName = file; }
    Location(const QString &file, int line, bool marker = true)
        { init(); m_lineNumber = line; m_fileName = file; m_needsMarker = marker; }
    Location(const StackFrame &frame, bool marker = true) //: m_frame(frame)
        { init(); m_fileName = frame.file; m_lineNumber = frame.line;
          m_needsMarker = marker; m_functionName = frame.function;
          m_hasDebugInfo = frame.isUsable(); m_address = frame.address; }
    QString fileName() const { return m_fileName; }
    QString functionName() const { return m_functionName; }
    int lineNumber() const { return m_lineNumber; }
    void setNeedsRaise(bool on) { m_needsRaise = on; }
    void setNeedsMarker(bool on) { m_needsMarker = on; }
    void setFileName(const QString &fileName) { m_fileName = fileName; }
    bool needsRaise() const { return m_needsRaise; }
    bool needsMarker() const { return m_needsMarker; }
    bool hasDebugInfo() const { return m_hasDebugInfo; }
    quint64 address() const { return m_address; }

private:
    void init() { m_needsMarker = false; m_needsRaise = true; m_lineNumber = -1;
        m_address = 0; m_hasDebugInfo = true; }
    bool m_needsMarker;
    bool m_needsRaise;
    bool m_hasDebugInfo;
    int m_lineNumber;
    QString m_fileName;
    QString m_functionName;
    quint64 m_address;
};


} // namespace Internal


// FIXME: DEBUGGER_EXPORT?
class DEBUGGER_EXPORT DebuggerEngine : public QObject
{
    Q_OBJECT

public:
    explicit DebuggerEngine(const DebuggerStartParameters &sp);
    virtual ~DebuggerEngine();

    typedef Internal::BreakpointId BreakpointId;
    virtual void setToolTipExpression(const QPoint & mousePos,
        TextEditor::ITextEditor *editor, int cursorPos);

    virtual void updateWatchData(const Internal::WatchData &data,
        const Internal::WatchUpdateFlags & flags = Internal::WatchUpdateFlags());
    void startDebugger(DebuggerRunControl *runControl);

    virtual void watchPoint(const QPoint &);
    virtual void openMemoryView(quint64 addr);
    virtual void fetchMemory(Internal::MemoryAgent *, QObject *,
                             quint64 addr, quint64 length);
    virtual void updateMemoryViews();
    virtual void openDisassemblerView(const Internal::Location &location);
    virtual void fetchDisassembler(Internal::DisassemblerAgent *);
    virtual void activateFrame(int index);

    virtual void reloadModules();
    virtual void examineModules();
    virtual void loadSymbols(const QString &moduleName);
    virtual void loadSymbolsForStack();
    virtual void loadAllSymbols();
    virtual void requestModuleSymbols(const QString &moduleName);

    virtual void reloadRegisters();
    virtual void reloadSourceFiles();
    virtual void reloadFullStack();

    virtual void setRegisterValue(int regnr, const QString &value);
    virtual void addOptionPages(QList<Core::IOptionsPage*> *) const;
    virtual unsigned debuggerCapabilities() const;

    virtual bool isSynchronous() const;
    virtual QByteArray qtNamespace() const;

    virtual void createSnapshot();
    virtual void updateAll();


    virtual bool stateAcceptsBreakpointChanges() const { return true; }
    virtual void attemptBreakpointSynchronization();
    virtual bool acceptsBreakpoint(BreakpointId id) const = 0;
    virtual void insertBreakpoint(BreakpointId id);  // FIXME: make pure
    virtual void removeBreakpoint(BreakpointId id);  // FIXME: make pure
    virtual void changeBreakpoint(BreakpointId id);  // FIXME: make pure

    virtual void assignValueInDebugger(const Internal::WatchData *data,
        const QString &expr, const QVariant &value);
    virtual void removeTooltip();
    virtual void selectThread(int index);

    virtual void handleRemoteSetupDone(int gdbServerPort, int qmlPort);
    virtual void handleRemoteSetupFailed(const QString &message);

protected:
    friend class Internal::DebuggerPluginPrivate;
    virtual void detachDebugger();
    virtual void exitDebugger();
    virtual void executeStep();
    virtual void executeStepOut() ;
    virtual void executeNext();
    virtual void executeStepI();
    virtual void executeNextI();
    virtual void executeReturn();

    virtual void continueInferior();
    virtual void interruptInferior();
    virtual void requestInterruptInferior();

    virtual void executeRunToLine(const QString &fileName, int lineNumber);
    virtual void executeRunToFunction(const QString &functionName);
    virtual void executeJumpToLine(const QString &fileName, int lineNumber);
    virtual void executeDebuggerCommand(const QString &command);

    virtual void frameUp();
    virtual void frameDown();

public:
    const DebuggerStartParameters &startParameters() const;
    DebuggerStartParameters &startParameters();

    Internal::ModulesHandler *modulesHandler() const;
    Internal::RegisterHandler *registerHandler() const;
    Internal::StackHandler *stackHandler() const;
    Internal::ThreadsHandler *threadsHandler() const;
    Internal::WatchHandler *watchHandler() const;
    Internal::SourceFilesHandler *sourceFilesHandler() const;
    Internal::BreakHandler *breakHandler() const;

    virtual QAbstractItemModel *modulesModel() const;
    virtual QAbstractItemModel *registerModel() const;
    virtual QAbstractItemModel *stackModel() const;
    virtual QAbstractItemModel *threadsModel() const;
    virtual QAbstractItemModel *localsModel() const;
    virtual QAbstractItemModel *watchersModel() const;
    virtual QAbstractItemModel *returnModel() const;
    virtual QAbstractItemModel *sourceFilesModel() const;

    void progressPing();
    void handleFinished();
    void handleStartFailed();
    bool debuggerActionsEnabled() const;
    static bool debuggerActionsEnabled(DebuggerState state);

    DebuggerState state() const;
    DebuggerState lastGoodState() const;
    DebuggerState targetState() const;
    bool isDying() const;

    // Dumper stuff (common to cdb and gdb).
    bool qtDumperLibraryEnabled() const;
    QString qtDumperLibraryName() const;
    QStringList qtDumperLibraryLocations() const;
    void showQtDumperLibraryWarning(const QString &details);

    static const char *stateName(int s);

    void notifyInferiorPid(qint64 pid);
    qint64 inferiorPid() const;
    bool isReverseDebugging() const;
    void handleCommand(int role, const QVariant &value);

    // Convenience
    Q_SLOT void showMessage(const QString &msg, int channel = LogDebug,
        int timeout = -1) const;
    Q_SLOT void showStatusMessage(const QString &msg, int timeout = -1) const;

    void resetLocation();
    virtual void gotoLocation(const Internal::Location &location);
    virtual void quitDebugger(); // called by DebuggerRunControl

    virtual void updateViews();
    bool isSlaveEngine() const;

signals:
    void stateChanged(const DebuggerState &state);
    void updateViewsRequested();
    /*
     * For "external" clients of a debugger run control that needs to do
     * further setup before the debugger is started (e.g. Maemo).
     * Afterwards, handleSetupDone() or handleSetupFailed() must be called
     * to continue or abort debugging, respectively.
     * This signal is only emitted if the start parameters indicate that
     * a server start script should be used, but none is given.
     */
    void requestRemoteSetup();

protected:
    // The base notify*() function implementation should be sufficient
    // in most cases, but engines are free to override them to do some
    // engine specific cleanup like stopping timers etc.
    virtual void notifyEngineSetupOk();
    virtual void notifyEngineSetupFailed();
    virtual void notifyEngineRunFailed();

    virtual void notifyInferiorSetupOk();
    virtual void notifyInferiorSetupFailed();

    virtual void notifyEngineRunAndInferiorRunOk();
    virtual void notifyEngineRunAndInferiorStopOk();
    virtual void notifyInferiorUnrunnable(); // Called by CoreAdapter.

    // Use notifyInferiorRunRequested() plus notifyInferiorRunOk() instead.
    //virtual void notifyInferiorSpontaneousRun();

    virtual void notifyInferiorRunRequested();
    virtual void notifyInferiorRunOk();
    virtual void notifyInferiorRunFailed();

    virtual void notifyInferiorStopOk();
    virtual void notifyInferiorSpontaneousStop();
    virtual void notifyInferiorStopFailed();
    virtual void notifyInferiorExited();

    virtual void notifyInferiorShutdownOk();
    virtual void notifyInferiorShutdownFailed();

    virtual void notifyEngineSpontaneousShutdown();
    virtual void notifyEngineShutdownOk();
    virtual void notifyEngineShutdownFailed();

    virtual void notifyInferiorIll();
    virtual void notifyEngineIll();

    virtual void setupEngine() = 0;
    virtual void setupInferior() = 0;
    virtual void runEngine() = 0;
    virtual void shutdownInferior() = 0;
    virtual void shutdownEngine() = 0;

    DebuggerRunControl *runControl() const; // FIXME: Protect.

protected:
    static QString msgWatchpointTriggered(BreakpointId id,
        int number, quint64 address);
    static QString msgWatchpointTriggered(BreakpointId id,
        int number, quint64 address, const QString &threadId);
    static QString msgBreakpointTriggered(BreakpointId id,
        int number, const QString &threadId);
    static QString msgStopped(const QString &reason = QString());
    static QString msgStoppedBySignal(const QString &meaning, const QString &name);
    static QString msgStoppedByException(const QString &description,
        const QString &threadId);
    static QString msgInterrupted();
    void showStoppedBySignalMessageBox(const QString meaning, QString name);
    void showStoppedByExceptionMessageBox(const QString &description);

    static bool isCppBreakpoint(const Internal::BreakpointParameters &p);

private:
    // Wrapper engine needs access to state of its subengines.
    friend class Internal::QmlCppEngine;
    void setState(DebuggerState state, bool forced = false);
    void setSlaveEngine(bool value);

    friend class DebuggerEnginePrivate;
    DebuggerEnginePrivate *d;
};

} // namespace Debugger

#endif // DEBUGGER_DEBUGGERENGINE_H
