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

#ifndef DEBUGGER_GDBENGINE_H
#define DEBUGGER_GDBENGINE_H

#include "idebuggerengine.h"
#include "gdbmi.h"
#include "outputcollector.h"

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QPoint>
#include <QtCore/QTextCodec>
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

struct GdbCookie
{
    GdbCookie() : type(0), synchronized(false) {}

    QString command;
    int type;
    bool synchronized;
    QVariant cookie;
};

enum DataDumperState
{
    DataDumperUninitialized,
    DataDumperLoadTried,
    DataDumperAvailable,
    DataDumperUnavailable,
};


class GdbEngine : public IDebuggerEngine
{
    Q_OBJECT

public:
    GdbEngine(DebuggerManager *parent);
    ~GdbEngine();

signals:
    void gdbInputAvailable(const QString &prefix, const QString &msg);
    void gdbOutputAvailable(const QString &prefix, const QString &msg);
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
    void setToolTipExpression(const QPoint &pos, const QString &exp);
    bool startDebugger();
    void exitDebugger();

    void continueInferior();
    void interruptInferior();

    void runToLineExec(const QString &fileName, int lineNumber);
    void runToFunctionExec(const QString &functionName);
    void jumpToLineExec(const QString &fileName, int lineNumber);

    void activateFrame(int index);
    void selectThread(int index);

    Q_SLOT void attemptBreakpointSynchronization();

    void loadSessionData() {}
    void saveSessionData() {}

    void assignValueInDebugger(const QString &expr, const QString &value);
    void executeDebuggerCommand(const QString & command);

    void loadSymbols(const QString &moduleName);
    void loadAllSymbols();

    void setDebugDumpers(bool on);
    void setUseCustomDumpers(bool on);

    //
    // Own stuff
    //
    int currentFrame() const;
    QString currentWorkingDirectory() const { return m_pwd; }

    bool supportsThreads() const;

    void initializeConnections();
    void initializeVariables();
    void queryFullName(const QString &fileName, QString *fullName);
    QString fullName(const QString &fileName);
    QString shortName(const QString &fullName);
    // get one usable name out of these, try full names first
    QString fullName(const QStringList &candidates);

    void handleResult(const GdbResultRecord &, int type, const QVariant &);

    // type and cookie are sender-internal data, opaque for the "event
    // queue". resultNeeded == true increments m_pendingResults on
    // send and decrements on receipt, effectively preventing 
    // watch model updates before everything is finished.
    enum StopNeeded { DoesNotNeedStop, NeedsStop };
    enum Synchronization { NotSynchronized, Synchronized };
    void sendCommand(const QString &command,
        int type = 0, const QVariant &cookie = QVariant(),
        StopNeeded needStop = DoesNotNeedStop,
        Synchronization synchronized = NotSynchronized);
    void sendSynchronizedCommand(const QString & command,
        int type = 0, const QVariant &cookie = QVariant(),
        StopNeeded needStop = DoesNotNeedStop);

    void setTokenBarrier();

    void updateLocals();

private slots:
    void gdbProcError(QProcess::ProcessError error);
    void readGdbStandardOutput();
    void readGdbStandardError();
    void readDebugeeOutput(const QByteArray &data);

private:
    int terminationIndex(const QByteArray &buffer, int &length);
    void handleResponse(const QByteArray &buff);
    void handleStart(const GdbResultRecord &response);
    void handleAttach();
    void handleAqcuiredInferior();
    void handleAsyncOutput2(const GdbMi &data);
    void handleAsyncOutput(const GdbMi &data);
    void handleResultRecord(const GdbResultRecord &response);
    void handleFileExecAndSymbols(const GdbResultRecord &response);
    void handleExecRun(const GdbResultRecord &response);
    void handleExecJumpToLine(const GdbResultRecord &response);
    void handleExecRunToFunction(const GdbResultRecord &response);
    void handleInfoShared(const GdbResultRecord &response);
    void handleInfoProc(const GdbResultRecord &response);
    void handleInfoThreads(const GdbResultRecord &response);
    void handleShowVersion(const GdbResultRecord &response);
    void handleQueryPwd(const GdbResultRecord &response);
    void handleQuerySources(const GdbResultRecord &response);
    void debugMessage(const QString &msg);

    OutputCollector m_outputCollector;
    QTextCodec *m_outputCodec;
    QTextCodec::ConverterState m_outputCodecState;

    QByteArray m_inbuffer;

    QProcess m_gdbProc;

    QHash<int, GdbCookie> m_cookieForToken;
    QHash<int, QByteArray> m_customOutputForToken;

    QByteArray m_pendingConsoleStreamOutput;
    QByteArray m_pendingTargetStreamOutput;
    QByteArray m_pendingLogStreamOutput;
    QString m_pwd;

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
    void handleBreakList(const GdbResultRecord &record);
    void handleBreakList(const GdbMi &table);
    void handleBreakIgnore(const GdbResultRecord &record, int index);
    void handleBreakInsert(const GdbResultRecord &record, int index);
    void handleBreakInsert1(const GdbResultRecord &record, int index);
    void handleBreakCondition(const GdbResultRecord &record, int index);
    void handleBreakInfo(const GdbResultRecord &record, int index);
    void extractDataFromInfoBreak(const QString &output, BreakpointData *data);
    void breakpointDataFromOutput(BreakpointData *data, const GdbMi &bkpt);
    void sendInsertBreakpoint(int index);


    //
    // Disassembler specific stuff
    //
    void handleDisassemblerList(const GdbResultRecord &record,
        const QString &cookie);
    void reloadDisassembler();
    QString m_address;


    //
    // Modules specific stuff
    //
    void reloadModules();
    void handleModulesList(const GdbResultRecord &record);


    //
    // Register specific stuff
    // 
    void reloadRegisters();
    void handleRegisterListNames(const GdbResultRecord &record);
    void handleRegisterListValues(const GdbResultRecord &record);

    //
    // Source file specific stuff
    // 
    void reloadSourceFiles();

    //
    // Stack specific stuff
    // 
    void handleStackListFrames(const GdbResultRecord &record);
    void handleStackSelectThread(const GdbResultRecord &record, int cookie);
    void handleStackListThreads(const GdbResultRecord &record, int cookie);


    //
    // Tooltip specific stuff
    // 
    void sendToolTipCommand(const QString &command, const QString &cookie);


    //
    // Watch specific stuff
    //
    // FIXME: BaseClass. called to improve situation for a watch item
    void updateSubItem(const WatchData &data);

    void updateWatchModel();
    Q_SLOT void updateWatchModel2();

    void insertData(const WatchData &data);
    void sendWatchParameters(const QByteArray &params0);
    void createGdbVariable(const WatchData &data);

    void handleTypeContents(const QString &output);
    void maybeHandleInferiorPidChanged(const QString &pid);

    void tryLoadCustomDumpers();
    void runCustomDumper(const WatchData &data, bool dumpChildren);
    bool isCustomValueDumperAvailable(const QString &type) const;

    void handleVarListChildren(const GdbResultRecord &record,
        const WatchData &cookie);
    void handleVarCreate(const GdbResultRecord &record,
        const WatchData &cookie);
    void handleVarAssign();
    void handleEvaluateExpression(const GdbResultRecord &record,
        const WatchData &cookie);
    void handleToolTip(const GdbResultRecord &record,
        const QString &cookie);
    void handleDumpCustomValue1(const GdbResultRecord &record,
        const WatchData &cookie);
    void handleQueryDataDumper1(const GdbResultRecord &record);
    void handleQueryDataDumper2(const GdbResultRecord &record);
    void handleDumpCustomValue2(const GdbResultRecord &record,
        const WatchData &cookie);
    void handleDumpCustomEditValue(const GdbResultRecord &record);
    void handleDumpCustomSetup(const GdbResultRecord &record);
    void handleStackListLocals(const GdbResultRecord &record);
    void handleStackListArguments(const GdbResultRecord &record);
    void handleVarListChildrenHelper(const GdbMi &child,
        const WatchData &parent);
    void setWatchDataType(WatchData &data, const GdbMi &mi);
    void setLocals(const QList<GdbMi> &locals);

    QString m_editedData;
    int m_pendingRequests;

    QStringList m_availableSimpleDumpers;
    QString m_namespace; // namespace used in "namespaced Qt";
    int m_qtVersion; // Qt version used in the debugged program
    
    DataDumperState m_dataDumperState; // state of qt creator dumpers
    QList<GdbMi> m_currentFunctionArgs;
    QString m_currentFrame;
    QMap<QString, QString> m_varToType;

    bool m_waitingForBreakpointSynchronizationToContinue;
    bool m_waitingForFirstBreakpointToBeHit;
    bool m_modulesListOutdated;

    QList<GdbCookie> m_commandsToRunOnTemporaryBreak;

    DebuggerManager *q;
    IDebuggerManagerAccessForEngines *qq;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_GDBENGINE_H
