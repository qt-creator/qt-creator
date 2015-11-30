/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_DEBUGGERENGINE_H
#define DEBUGGER_DEBUGGERENGINE_H

#include "debugger_global.h"
#include "debuggerconstants.h"
#include "debuggerprotocol.h"
#include "debuggerstartparameters.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <texteditor/textmark.h>

#include <QObject>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QDebug;
class QPoint;
class QAbstractItemModel;
QT_END_NAMESPACE

namespace Core { class IOptionsPage; }

namespace Debugger {

class DebuggerRunControl;
class RemoteSetupResult;

DEBUGGER_EXPORT QDebug operator<<(QDebug str, DebuggerState state);

namespace Internal {

class DebuggerEnginePrivate;
class DebuggerPluginPrivate;
class DisassemblerAgent;
class MemoryAgent;
class WatchData;
class WatchItem;
class BreakHandler;
class LocationMark;
class ModulesHandler;
class RegisterHandler;
class StackHandler;
class StackFrame;
class SourceFilesHandler;
class ThreadsHandler;
class WatchHandler;
class Breakpoint;
class QmlAdapter;
class QmlCppEngine;
class DebuggerToolTipContext;
class MemoryViewSetupData;
class Terminal;
class ThreadId;

class DebuggerRunParameters : public DebuggerStartParameters
{
public:
    DebuggerRunParameters() {}

    DebuggerEngineType masterEngineType = NoEngineType;
    DebuggerEngineType cppEngineType = NoEngineType;

    DebuggerLanguages languages = AnyLanguage;
    bool breakOnMain = false;
    bool multiProcess = false; // Whether to set detach-on-fork off.

    QString debuggerCommand;
    QString coreFile;
    QString overrideStartScript; // Used in attach to core and remote debugging
    QString startMessage; // First status message shown.
    QString debugInfoLocation; // Gdb "set-debug-file-directory".
    QStringList debugSourceLocation; // Gdb "directory"
    QString serverStartScript;
    ProjectExplorer::IDevice::ConstPtr device;
    QString sysRoot;
    bool isSnapshot = false; // Set if created internally.
    ProjectExplorer::Abi toolChainAbi;

    QString projectSourceDirectory;
    QStringList projectSourceFiles;

    // Used by Script debugging
    QString interpreter;
    QString mainScript;

    // Used by AttachCrashedExternal.
    QString crashParameter;

    bool nativeMixedEnabled = false;

    // For Debugger testing.
    int testCase = 0;
};

class UpdateParameters
{
public:
    UpdateParameters() {}

    QList<QByteArray> partialVariables() const
    {
        QList<QByteArray> result;
        if (!partialVariable.isEmpty())
            result.append(partialVariable);
        return result;
    }

    QByteArray partialVariable;
};

class Location
{
public:
    Location() { init(); }
    Location(quint64 address) { init(); m_address = address; }
    Location(const QString &file) { init(); m_fileName = file; }
    Location(const QString &file, int line, bool marker = true)
        { init(); m_lineNumber = line; m_fileName = file; m_needsMarker = marker; }
    Location(const StackFrame &frame, bool marker = true);
    QString fileName() const { return m_fileName; }
    QString functionName() const { return m_functionName; }
    QString from() const { return m_from; }
    int lineNumber() const { return m_lineNumber; }
    void setNeedsRaise(bool on) { m_needsRaise = on; }
    void setNeedsMarker(bool on) { m_needsMarker = on; }
    void setFileName(const QString &fileName) { m_fileName = fileName; }
    void setUseAssembler(bool on) { m_hasDebugInfo = !on; }
    bool needsRaise() const { return m_needsRaise; }
    bool needsMarker() const { return m_needsMarker; }
    bool hasDebugInfo() const { return m_hasDebugInfo; }
    bool canBeDisassembled() const
        { return m_address != quint64(-1) || !m_functionName.isEmpty(); }
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
    QString m_from;
    quint64 m_address;
};

enum LocationType { UnknownLocation, LocationByFile, LocationByAddress };

class ContextData
{
public:
    bool isValid() const { return type != UnknownLocation; }

public:
    LocationType type = UnknownLocation;
    QString fileName;
    int lineNumber = 0;
    quint64 address = 0;
};

class DebuggerEngine : public QObject
{
    Q_OBJECT

public:
    explicit DebuggerEngine(const DebuggerRunParameters &sp);
    virtual ~DebuggerEngine();

    const DebuggerRunParameters &runParameters() const;
    DebuggerRunParameters &runParameters();

    virtual bool canHandleToolTip(const DebuggerToolTipContext &) const;
    virtual void expandItem(const QByteArray &iname); // Called when item in tree gets expanded.
    virtual void updateItem(const QByteArray &iname); // Called for fresh watch items.
    virtual void selectWatchData(const QByteArray &iname);

    virtual void startDebugger(DebuggerRunControl *runControl);

    virtual void watchPoint(const QPoint &);

    enum MemoryViewFlags
    {
        MemoryReadOnly = 0x1,      //!< Read-only.
        MemoryTrackRegister = 0x2, //!< Address parameter is register number to track
        MemoryView = 0x4           //!< Open a separate view (using the pos-parameter).
    };

    virtual void runCommand(const DebuggerCommand &cmd);
    virtual void openMemoryView(const MemoryViewSetupData &data);
    virtual void fetchMemory(Internal::MemoryAgent *, QObject *,
                             quint64 addr, quint64 length);
    virtual void changeMemory(Internal::MemoryAgent *, QObject *,
                              quint64 addr, const QByteArray &data);
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
    virtual void requestModuleSections(const QString &moduleName);

    virtual void reloadRegisters();
    virtual void reloadSourceFiles();
    virtual void reloadFullStack();
    virtual void loadAdditionalQmlStack();
    virtual void reloadDebuggingHelpers();

    virtual void setRegisterValue(const QByteArray &name, const QString &value);
    virtual void addOptionPages(QList<Core::IOptionsPage*> *) const;
    virtual bool hasCapability(unsigned cap) const = 0;
    virtual void debugLastCommand() {}

    virtual bool isSynchronous() const;
    virtual QByteArray qtNamespace() const;
    void setQtNamespace(const QByteArray &ns);

    virtual void createSnapshot();
    virtual void updateAll();
    virtual void updateLocals();

    virtual bool stateAcceptsBreakpointChanges() const { return true; }
    virtual void attemptBreakpointSynchronization();
    virtual bool acceptsBreakpoint(Breakpoint bp) const = 0;
    virtual void insertBreakpoint(Breakpoint bp);  // FIXME: make pure
    virtual void removeBreakpoint(Breakpoint bp);  // FIXME: make pure
    virtual void changeBreakpoint(Breakpoint bp);  // FIXME: make pure

    virtual bool acceptsDebuggerCommands() const { return true; }
    virtual void executeDebuggerCommand(const QString &command, DebuggerLanguages languages);

    virtual void assignValueInDebugger(WatchItem *item,
        const QString &expr, const QVariant &value);
    virtual void selectThread(Internal::ThreadId threadId) = 0;

    virtual Internal::ModulesHandler *modulesHandler() const;
    virtual Internal::RegisterHandler *registerHandler() const;
    virtual Internal::StackHandler *stackHandler() const;
    virtual Internal::ThreadsHandler *threadsHandler() const;
    virtual Internal::WatchHandler *watchHandler() const;
    virtual Internal::SourceFilesHandler *sourceFilesHandler() const;
    virtual Internal::BreakHandler *breakHandler() const;

    virtual QAbstractItemModel *modulesModel() const;
    virtual QAbstractItemModel *registerModel() const;
    virtual QAbstractItemModel *stackModel() const;
    virtual QAbstractItemModel *threadsModel() const;
    virtual QAbstractItemModel *watchModel() const;
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

    static const char *stateName(int s);

    void notifyInferiorPid(qint64 pid);
    qint64 inferiorPid() const;
    bool isReverseDebugging() const;
    void handleCommand(int role, const QVariant &value);

    // Convenience
    Q_SLOT virtual void showMessage(const QString &msg, int channel = LogDebug,
        int timeout = -1) const;
    Q_SLOT void showStatusMessage(const QString &msg, int timeout = -1) const;

    virtual void resetLocation();
    virtual void gotoLocation(const Internal::Location &location);
    Q_SLOT virtual void quitDebugger(); // called by DebuggerRunControl
    virtual void abortDebugger(); // called by DebuggerPlugin

    virtual void updateViews();
    bool isSlaveEngine() const;
    bool isMasterEngine() const;
    DebuggerEngine *masterEngine() const;
    virtual DebuggerEngine *cppEngine() { return 0; }

    virtual bool canDisplayTooltip() const;

    virtual void notifyInferiorIll();

    QString toFileInProject(const QUrl &fileUrl);
    void updateBreakpointMarker(const Breakpoint &bp);
    void removeBreakpointMarker(const Breakpoint &bp);

signals:
    void stateChanged(Debugger::DebuggerState state);
    // A new stack frame is on display including locals.
    void stackFrameCompleted();
    /*
     * For "external" clients of a debugger run control that needs to do
     * further setup before the debugger is started (e.g. RemoteLinux).
     * Afterwards, notifyEngineRemoteSetupFinished
     * must be called to continue or abort debugging.
     * This signal is only emitted if the start parameters indicate that
     * a server start script should be used, but none is given.
     */
    void requestRemoteSetup();
    void aboutToNotifyInferiorSetupOk();

protected:
    // The base notify*() function implementation should be sufficient
    // in most cases, but engines are free to override them to do some
    // engine specific cleanup like stopping timers etc.
    virtual void notifyEngineSetupOk();
    virtual void notifyEngineSetupFailed();
    virtual void notifyEngineRunFailed();

    virtual void notifyEngineRequestRemoteSetup();
    public:
    virtual void notifyEngineRemoteServerRunning(const QByteArray &, int pid);
    virtual void notifyEngineRemoteSetupFinished(const RemoteSetupResult &result);

    protected:
    virtual void notifyInferiorSetupOk();
    virtual void notifyInferiorSetupFailed();

    virtual void notifyEngineRunAndInferiorRunOk();
    virtual void notifyEngineRunAndInferiorStopOk();
    virtual void notifyEngineRunOkAndInferiorUnrunnable(); // Called by CoreAdapter.

    // Use notifyInferiorRunRequested() plus notifyInferiorRunOk() instead.
    //virtual void notifyInferiorSpontaneousRun();

    virtual void notifyInferiorRunRequested();
    virtual void notifyInferiorRunOk();
    virtual void notifyInferiorRunFailed();

    virtual void notifyInferiorStopOk();
    virtual void notifyInferiorSpontaneousStop();
    virtual void notifyInferiorStopFailed();

    public:
    virtual void notifyInferiorExited();
    void notifyDebuggerProcessFinished(int exitCode, QProcess::ExitStatus exitStatus,
                                       const QString &backendName);

protected:
    virtual void notifyInferiorShutdownOk();
    virtual void notifyInferiorShutdownFailed();

    virtual void notifyEngineSpontaneousShutdown();
    virtual void notifyEngineShutdownOk();
    virtual void notifyEngineShutdownFailed();

    virtual void notifyEngineIll();

    virtual void setupEngine() = 0;
    virtual void setupInferior() = 0;
    virtual void runEngine() = 0;
    virtual void shutdownInferior() = 0;
    virtual void shutdownEngine() = 0;
    virtual void resetInferior() {}

    virtual void detachDebugger();
    virtual void exitDebugger();
    virtual void executeStep();
    virtual void executeStepOut();
    virtual void executeNext();
    virtual void executeStepI();
    virtual void executeNextI();
    virtual void executeReturn();

    virtual void continueInferior();
    virtual void interruptInferior();
    virtual void requestInterruptInferior();

    virtual void executeRunToLine(const Internal::ContextData &data);
    virtual void executeRunToFunction(const QString &functionName);
    virtual void executeJumpToLine(const Internal::ContextData &data);

    virtual void frameUp();
    virtual void frameDown();

    virtual void doUpdateLocals(const UpdateParameters &params);

    void setTargetState(DebuggerState state);
    void setMasterEngine(DebuggerEngine *masterEngine);

    DebuggerRunControl *runControl() const;
    Terminal *terminal() const;

    static QString msgStopped(const QString &reason = QString());
    static QString msgStoppedBySignal(const QString &meaning, const QString &name);
    static QString msgStoppedByException(const QString &description,
        const QString &threadId);
    static QString msgInterrupted();
    void showStoppedBySignalMessageBox(const QString meaning, QString name);
    void showStoppedByExceptionMessageBox(const QString &description);

    bool isStateDebugging() const;
    void setStateDebugging(bool on);

    static void validateExecutable(DebuggerRunParameters *sp);

    virtual void setupSlaveInferior();
    virtual void setupSlaveEngine();
    virtual void runSlaveEngine();
    virtual void shutdownSlaveEngine();

    virtual void slaveEngineStateChanged(DebuggerEngine *engine,
        DebuggerState state);

    void updateLocalsView(const GdbMi &all);
    void checkState(DebuggerState state, const char *file, int line);
    bool isNativeMixedEnabled() const;
    bool isNativeMixedActive() const;
    bool isNativeMixedActiveFrame() const;

private:
    // Wrapper engine needs access to state of its subengines.
    friend class QmlCppEngine;
    friend class DebuggerPluginPrivate;
    friend class QmlAdapter;

    virtual void setState(DebuggerState state, bool forced = false);

    friend class DebuggerEnginePrivate;
    friend class LocationMark;
    DebuggerEnginePrivate *d;
};

class LocationMark : public TextEditor::TextMark
{
public:
    LocationMark(DebuggerEngine *engine, const QString &file, int line);
    void removedFromEditor() override { updateLineNumber(0); }

private:
    bool isDraggable() const override;
    void dragToLine(int line) override;

    QPointer<DebuggerEngine> m_engine;
};

DebuggerEngine *createEngine(DebuggerEngineType et, const DebuggerRunParameters &rp, QStringList *errors);

DebuggerRunControl *createAndScheduleRun(const DebuggerRunParameters &rp, const ProjectExplorer::Kit *kit);

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::UpdateParameters)
Q_DECLARE_METATYPE(Debugger::Internal::ContextData)

#endif // DEBUGGER_DEBUGGERENGINE_H
