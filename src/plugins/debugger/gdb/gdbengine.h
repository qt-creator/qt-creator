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

#ifndef DEBUGGER_GDBENGINE_H
#define DEBUGGER_GDBENGINE_H

#include "idebuggerengine.h"
#include "gdbmi.h"
#include "outputcollector.h"
#include "watchutils.h"

#include <consoleprocess.h>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QPoint>
#include <QtCore/QTextCodec>
#include <QtCore/QTime>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE
class QAction;
class QAbstractItemModel;
class QWidget;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class DebuggerManager;
class IDebuggerManagerAccessForEngines;
class GdbResultRecord;
class GdbMi;

class WatchData;
class BreakpointData;

enum DebuggingHelperState
{
    DebuggingHelperUninitialized,
    DebuggingHelperLoadTried,
    DebuggingHelperAvailable,
    DebuggingHelperUnavailable,
};


class GdbEngine : public IDebuggerEngine
{
    Q_OBJECT

public:
    GdbEngine(DebuggerManager *parent);
    ~GdbEngine();

signals:
    void gdbInputAvailable(int channel, const QString &msg);
    void gdbOutputAvailable(int channel, const QString &msg);
    void applicationOutputAvailable(const QString &output);

private:
    //
    // IDebuggerEngine implementation
    //
    void stepExec();
    void stepOutExec();
    void nextExec();
    void stepIExec();
    void nextIExec();

    void shutdown();
    void setToolTipExpression(const QPoint &mousePos, TextEditor::ITextEditor *editor, int cursorPos);
    bool startDebugger(const QSharedPointer<DebuggerStartParameters> &sp);
    void exitDebugger();
    void detachDebugger();

    void continueInferior();
    void interruptInferior();

    void runToLineExec(const QString &fileName, int lineNumber);
    void runToFunctionExec(const QString &functionName);
    void jumpToLineExec(const QString &fileName, int lineNumber);

    void activateFrame(int index);
    void selectThread(int index);

    Q_SLOT void attemptBreakpointSynchronization();

    void assignValueInDebugger(const QString &expr, const QString &value);
    void executeDebuggerCommand(const QString & command);
    void watchPoint(const QPoint &);

    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();
    virtual QList<Symbol> moduleSymbols(const QString &moduleName);

    void fetchMemory(MemoryViewAgent *agent, quint64 addr, quint64 length);
    void handleFetchMemory(const GdbResultRecord &record, const QVariant &cookie);

    void fetchDisassembler(DisassemblerViewAgent *agent,
        const StackFrame &frame);
    void fetchDisassemblerByAddress(DisassemblerViewAgent *agent,
        bool useMixedMode);
    void handleFetchDisassemblerByLine(const GdbResultRecord &record,
        const QVariant &cookie);
    void handleFetchDisassemblerByAddress1(const GdbResultRecord &record,
        const QVariant &cookie);
    void handleFetchDisassemblerByAddress0(const GdbResultRecord &record,
        const QVariant &cookie);

    Q_SLOT void setDebugDebuggingHelpers(const QVariant &on);
    Q_SLOT void setUseDebuggingHelpers(const QVariant &on);

    //
    // Own stuff
    //

    int currentFrame() const;

    bool supportsThreads() const;

    void initializeConnections();
    void initializeVariables();
    QString fullName(const QString &fileName);
    // get one usable name out of these, try full names first
    QString fullName(const QStringList &candidates);

    void handleResult(const GdbResultRecord &, int type, const QVariant &);

public: // otherwise the Qt flag macros are unhappy
    enum GdbCommandFlag {
        NoFlags = 0,
        NeedsStop = 1,
        Discardable = 2,
        RebuildModel = 4,
        WatchUpdate = Discardable | RebuildModel,
        EmbedToken = 8
    };
    Q_DECLARE_FLAGS(GdbCommandFlags, GdbCommandFlag)

private:
    typedef void (GdbEngine::*GdbCommandCallback)(const GdbResultRecord &record, const QVariant &cookie);

    struct GdbCommand
    {
        GdbCommand() : flags(0), callback(0), callbackName(0) {}

        int flags;
        GdbCommandCallback callback;
        const char *callbackName;
        QString command;
        QVariant cookie;
        QTime postTime;
    };

    // type and cookie are sender-internal data, opaque for the "event
    // queue". resultNeeded == true increments m_pendingResults on
    // send and decrements on receipt, effectively preventing 
    // watch model updates before everything is finished.
    void flushCommand(GdbCommand &cmd);
    void postCommand(const QString &command,
                     GdbCommandFlags flags,
                     GdbCommandCallback callback = 0,
                     const char *callbackName = 0,
                     const QVariant &cookie = QVariant());
    void postCommand(const QString &command,
                     GdbCommandCallback callback = 0,
                     const char *callbackName = 0,
                     const QVariant &cookie = QVariant());

    void setTokenBarrier();

    void updateLocals();

private slots:
    void gdbProcError(QProcess::ProcessError error);
    void readGdbStandardOutput();
    void readGdbStandardError();
    void readUploadStandardOutput();
    void readUploadStandardError();
    void readDebugeeOutput(const QByteArray &data);
    void stubStarted();
    void stubError(const QString &msg);
    void uploadProcError(QProcess::ProcessError error);

private:
    int terminationIndex(const QByteArray &buffer, int &length);
    void handleResponse(const QByteArray &buff);
    void handleStart(const GdbResultRecord &response, const QVariant &);
    void handleAttach(const GdbResultRecord &, const QVariant &);
    void handleStubAttached(const GdbResultRecord &, const QVariant &);
    void handleAqcuiredInferior();
    void handleAsyncOutput2(const GdbResultRecord &, const QVariant &cookie);
    void handleAsyncOutput2(const GdbMi &data);
    void handleAsyncOutput(const GdbMi &data);
    void handleResultRecord(const GdbResultRecord &response);
    void handleFileExecAndSymbols(const GdbResultRecord &response, const QVariant &);
    void handleExecContinue(const GdbResultRecord &response, const QVariant &);
    void handleExecRun(const GdbResultRecord &response, const QVariant &);
    void handleExecJumpToLine(const GdbResultRecord &response, const QVariant &);
    void handleExecRunToFunction(const GdbResultRecord &response, const QVariant &);
    void handleInfoShared(const GdbResultRecord &response, const QVariant &);
    void handleInfoProc(const GdbResultRecord &response, const QVariant &);
    void handleInfoThreads(const GdbResultRecord &response, const QVariant &);
    void handleShowVersion(const GdbResultRecord &response, const QVariant &);
    void handleQueryPwd(const GdbResultRecord &response, const QVariant &);
    void handleQuerySources(const GdbResultRecord &response, const QVariant &);
    void handleTargetCore(const GdbResultRecord &, const QVariant &);
    void handleExit(const GdbResultRecord &, const QVariant &);
    void handleSetTargetAsync(const GdbResultRecord &, const QVariant &);
    void handleTargetRemote(const GdbResultRecord &, const QVariant &);
    void handleWatchPoint(const GdbResultRecord &, const QVariant &);
    void debugMessage(const QString &msg);
    bool showToolTip();

    const bool m_dumperInjectionLoad;

    OutputCollector m_outputCollector;
    QTextCodec *m_outputCodec;
    QTextCodec::ConverterState m_outputCodecState;

    QByteArray m_inbuffer;

    QProcess m_gdbProc;
    QProcess m_uploadProc;

    Core::Utils::ConsoleProcess m_stubProc;

    QHash<int, GdbCommand> m_cookieForToken;
    QHash<int, QByteArray> m_customOutputForToken;

    QByteArray m_pendingConsoleStreamOutput;
    QByteArray m_pendingTargetStreamOutput;
    QByteArray m_pendingLogStreamOutput;

    // contains the first token number for the current round
    // of evaluation. Responses with older tokens are considers
    // out of date and discarded.
    int m_oldestAcceptableToken;

    int m_gdbVersion; // 6.8.0 is 680
    int m_gdbBuildVersion; // MAC only? 

    // awful hack to keep track of used files
    QMap<QString, QString> m_shortToFullName;
    QMap<QString, QString> m_fullToShortName;

    //
    // Breakpoint specific stuff
    //
    void handleBreakList(const GdbResultRecord &record, const QVariant &);
    void handleBreakList(const GdbMi &table);
    void handleBreakIgnore(const GdbResultRecord &record, const QVariant &cookie);
    void handleBreakInsert(const GdbResultRecord &record, const QVariant &cookie);
    void handleBreakInsert1(const GdbResultRecord &record, const QVariant &cookie);
    void handleBreakCondition(const GdbResultRecord &record, const QVariant &cookie);
    void handleBreakInfo(const GdbResultRecord &record, const QVariant &cookie);
    void extractDataFromInfoBreak(const QString &output, BreakpointData *data);
    void breakpointDataFromOutput(BreakpointData *data, const GdbMi &bkpt);
    void sendInsertBreakpoint(int index);

    //
    // Modules specific stuff
    //
    void reloadModules();
    void handleModulesList(const GdbResultRecord &record, const QVariant &);


    //
    // Register specific stuff
    // 
    Q_SLOT void reloadRegisters();
    void setRegisterValue(int nr, const QString &value);
    void handleRegisterListNames(const GdbResultRecord &record, const QVariant &);
    void handleRegisterListValues(const GdbResultRecord &record, const QVariant &);

    //
    // Source file specific stuff
    // 
    void reloadSourceFiles();

    //
    // Stack specific stuff
    // 
    void handleStackListFrames(const GdbResultRecord &record, const QVariant &cookie);
    void handleStackSelectThread(const GdbResultRecord &, const QVariant &);
    void handleStackListThreads(const GdbResultRecord &record, const QVariant &cookie);
    Q_SLOT void reloadStack();
    Q_SLOT void reloadFullStack();


    //
    // Tooltip specific stuff
    // 
    void sendToolTipCommand(const QString &command, const QString &cookie);


    //
    // Watch specific stuff
    //
    // FIXME: BaseClass. called to improve situation for a watch item
    void updateSubItem(const WatchData &data);

    void updateWatchData(const WatchData &data);
    void rebuildModel();

    void insertData(const WatchData &data);
    void sendWatchParameters(const QByteArray &params0);
    void createGdbVariable(const WatchData &data);

    void maybeHandleInferiorPidChanged(const QString &pid);

    void tryLoadDebuggingHelpers();
    void tryQueryDebuggingHelpers();
    Q_SLOT void recheckDebuggingHelperAvailability();
    void runDebuggingHelper(const WatchData &data, bool dumpChildren);
    void runDirectDebuggingHelper(const WatchData &data, bool dumpChildren);
    bool hasDebuggingHelperForType(const QString &type) const;

    void handleVarListChildren(const GdbResultRecord &record,
        const QVariant &cookie);
    void handleVarCreate(const GdbResultRecord &record,
        const QVariant &cookie);
    void handleVarAssign(const GdbResultRecord &, const QVariant &);
    void handleEvaluateExpression(const GdbResultRecord &record,
        const QVariant &cookie);
    //void handleToolTip(const GdbResultRecord &record,
    //    const QVariant &cookie);
    void handleQueryDebuggingHelper(const GdbResultRecord &record, const QVariant &);
    void handleDebuggingHelperValue1(const GdbResultRecord &record,
        const QVariant &cookie);
    void handleDebuggingHelperValue2(const GdbResultRecord &record,
        const QVariant &cookie);
    void handleDebuggingHelperValue3(const GdbResultRecord &record,
        const QVariant &cookie);
    void handleDebuggingHelperEditValue(const GdbResultRecord &record);
    void handleDebuggingHelperSetup(const GdbResultRecord &record, const QVariant &);
    void handleStackListLocals(const GdbResultRecord &record, const QVariant &);
    void handleStackListArguments(const GdbResultRecord &record, const QVariant &);
    void handleVarListChildrenHelper(const GdbMi &child,
        const WatchData &parent);
    void setWatchDataType(WatchData &data, const GdbMi &mi);
    void setWatchDataDisplayedType(WatchData &data, const GdbMi &mi);
    void setLocals(const QList<GdbMi> &locals);
   
    bool startModeAllowsDumpers() const;
    QString parseDisassembler(const GdbMi &lines);

    int m_pendingRequests;
    QSet<QString> m_processedNames; 

    QtDumperHelper m_dumperHelper;
    
    DebuggingHelperState m_debuggingHelperState;
    QList<GdbMi> m_currentFunctionArgs;
    QString m_currentFrame;
    QMap<QString, QString> m_varToType;

    bool m_autoContinue;
    bool m_waitingForFirstBreakpointToBeHit;
    bool m_modulesListOutdated;

    QList<GdbCommand> m_commandsToRunOnTemporaryBreak;

    DebuggerManager * const q;
    IDebuggerManagerAccessForEngines * const qq;
    // make sure to re-initialize new members in initializeVariables();
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_OPERATORS_FOR_FLAGS(Debugger::Internal::GdbEngine::GdbCommandFlags)

#endif // DEBUGGER_GDBENGINE_H
