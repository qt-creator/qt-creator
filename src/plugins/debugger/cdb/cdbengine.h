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

#ifndef DEBUGGER_CDBENGINE_H
#define DEBUGGER_CDBENGINE_H

#include <debugger/debuggerengine.h>
#include <debugger/breakhandler.h>

#include <projectexplorer/devicesupport/idevice.h>

#include <QSharedPointer>
#include <QProcess>
#include <QMap>
#include <QVariant>
#include <QTime>

#include <functional>

namespace Utils { class ConsoleProcess; }
namespace Debugger {
namespace Internal {

class DisassemblerAgent;
class CdbCommand;
class CdbResponse;
struct MemoryViewCookie;
class ByteArrayInputStream;
class GdbMi;

class CdbEngine : public DebuggerEngine
{
    Q_OBJECT

public:
    enum CommandFlags { QuietCommand = 0x1 };
    // Flag bits for a sequence of commands
    enum CommandSequenceFlags {
        CommandListStack = 0x1,
        CommandListThreads = 0x2,
        CommandListRegisters = 0x4,
        CommandListModules = 0x8,
        CommandListBreakPoints = 0x10
    };

    typedef QSharedPointer<CdbCommand> CdbCommandPtr;
    typedef std::function<void(const CdbResponse &)> CommandHandler;

    CdbEngine(const DebuggerStartParameters &sp);
    ~CdbEngine();

    // Factory function that returns 0 if the debug engine library cannot be found.

    virtual bool setToolTipExpression(const DebuggerToolTipContext &context);

    virtual DebuggerEngine *cppEngine() { return this; }

    virtual void setupEngine();
    virtual void setupInferior();
    virtual void runEngine();
    virtual void shutdownInferior();
    virtual void shutdownEngine();
    virtual void abortDebugger();
    virtual void detachDebugger();
    virtual void updateWatchItem(WatchItem *item);
    virtual bool hasCapability(unsigned cap) const;
    virtual void watchPoint(const QPoint &);
    virtual void setRegisterValue(const QByteArray &name, const QString &value);

    virtual void executeStep();
    virtual void executeStepOut();
    virtual void executeNext();
    virtual void executeStepI();
    virtual void executeNextI();

    virtual void continueInferior();
    virtual void interruptInferior();

    virtual void executeRunToLine(const ContextData &data);
    virtual void executeRunToFunction(const QString &functionName);
    virtual void executeJumpToLine(const ContextData &data);
    virtual void assignValueInDebugger(WatchItem *w, const QString &expr, const QVariant &value);
    virtual void executeDebuggerCommand(const QString &command, DebuggerLanguages languages);

    virtual void activateFrame(int index);
    virtual void selectThread(ThreadId threadId);

    virtual bool stateAcceptsBreakpointChanges() const;
    virtual bool acceptsBreakpoint(Breakpoint bp) const;
    virtual void attemptBreakpointSynchronization();

    virtual void fetchDisassembler(DisassemblerAgent *agent);
    virtual void fetchMemory(MemoryAgent *, QObject *, quint64 addr, quint64 length);
    virtual void changeMemory(Internal::MemoryAgent *, QObject *, quint64 addr, const QByteArray &data);

    virtual void reloadModules();
    virtual void loadSymbols(const QString &moduleName);
    virtual void loadAllSymbols();
    virtual void requestModuleSymbols(const QString &moduleName);

    virtual void reloadRegisters();
    virtual void reloadSourceFiles();
    virtual void reloadFullStack();
    void loadAdditionalQmlStack();

    static QString extensionLibraryName(bool is64Bit);

private slots:
    void readyReadStandardOut();
    void readyReadStandardError();
    void processError();
    void processFinished();
    void postCommand(const QByteArray &cmd, unsigned flags);
    void postBuiltinCommand(const QByteArray &cmd,
                            unsigned flags,
                            CommandHandler handler,
                            unsigned nextCommandFlag = 0);

    void postExtensionCommand(const QByteArray &cmd,
                              const QByteArray &arguments,
                              unsigned flags,
                              CommandHandler handler,
                              unsigned nextCommandFlag = 0);

    void postCommandSequence(unsigned mask);
    void operateByInstructionTriggered(bool);
    void verboseLogTriggered(bool);

    void consoleStubError(const QString &);
    void consoleStubProcessStarted();
    void consoleStubExited();

    void createFullBacktrace();

    void handleDoInterruptInferior(const QString &errorMessage);

private:
    typedef QHash<BreakpointModelId, BreakpointResponse> PendingBreakPointMap;
    typedef QPair<QString, QString> SourcePathMapping;
    struct NormalizedSourceFileName // Struct for caching mapped/normalized source files.
    {
        NormalizedSourceFileName(const QString &fn = QString(), bool e = false) : fileName(fn), exists(e) {}

        QString fileName;
        bool exists;
    };

    enum SpecialStopMode
    {
        NoSpecialStop,
        SpecialStopSynchronizeBreakpoints,
        SpecialStopGetWidgetAt,
        CustomSpecialStop // Associated with m_specialStopData, handleCustomSpecialStop()
    };
    enum ParseStackResultFlags // Flags returned by parseStackTrace
    {
        ParseStackStepInto = 1, // Need to execute a step, hit on a call frame in "Step into"
        ParseStackStepOut = 2, // Need to step out, hit on a frame without debug information
        ParseStackWow64 = 3 // Hit on a frame with 32bit emulation, switch debugger to 32 bit mode
    };


    bool startConsole(const DebuggerStartParameters &sp, QString *errorMessage);
    void init();
    unsigned examineStopReason(const GdbMi &stopReason, QString *message,
                               QString *exceptionBoxMessage,
                               bool conditionalBreakPointTriggered = false);
    void processStop(const GdbMi &stopReason, bool conditionalBreakPointTriggered = false);
    bool commandsPending() const;
    void handleExtensionMessage(char t, int token, const QByteArray &what, const QByteArray &message);
    bool doSetupEngine(QString *errorMessage);
    bool launchCDB(const DebuggerStartParameters &sp, QString *errorMessage);
    void handleSessionAccessible(unsigned long cdbExState);
    void handleSessionInaccessible(unsigned long cdbExState);
    void handleSessionIdle(const QByteArray &message);
    void doInterruptInferior(SpecialStopMode sm);
    void doInterruptInferiorCustomSpecialStop(const QVariant &v);
    void doContinueInferior();
    inline void parseOutputLine(QByteArray line);
    inline bool isCdbProcessRunning() const { return m_process.state() != QProcess::NotRunning; }
    bool canInterruptInferior() const;
    void syncOperateByInstruction(bool operateByInstruction);
    void syncVerboseLog(bool verboseLog);
    void postWidgetAtCommand();
    void handleCustomSpecialStop(const QVariant &v);
    void postFetchMemory(const MemoryViewCookie &c);
    inline void postDisassemblerCommand(quint64 address, DisassemblerAgent *agent);
    void postDisassemblerCommand(quint64 address, quint64 endAddress,
                                 DisassemblerAgent *agent);
    void postResolveSymbol(const QString &module, const QString &function,
                           DisassemblerAgent *agent);
    // Builtin commands
    void dummyHandler(const CdbResponse &);
    void handleStackTrace(const CdbResponse &);
    void handleRegisters(const CdbResponse &);
    void handleDisassembler(const CdbResponse &, DisassemblerAgent *agent);
    void handleJumpToLineAddressResolution(const CdbResponse &response, const ContextData &context);
    void handleExpression(const CdbResponse &command, BreakpointModelId id, const GdbMi &stopReason);
    void handleResolveSymbol(const CdbResponse &command, const QString &symbol, DisassemblerAgent *agent);
    void handleResolveSymbolHelper(const QList<quint64> &addresses, DisassemblerAgent *agent);
    void handleBreakInsert(const CdbResponse &response);
    void handleCheckWow64(const CdbResponse &response, const GdbMi &stack);
    void ensureUsing32BitStackInWow64(const CdbResponse &response, const GdbMi &stack);
    void handleSwitchWow64Stack(const CdbResponse &response);
    void jumpToAddress(quint64 address);
    void handleCreateFullBackTrace(const CdbResponse &response);

    // Extension commands
    void handleThreads(const CdbResponse &response);
    void handlePid(const CdbResponse &response);
    void handleLocals(const CdbResponse &response, bool newFrame);
    void handleAddWatch(const CdbResponse &response, WatchData item);
    void handleExpandLocals(const CdbResponse &response);
    void handleRegistersExt(const CdbResponse &response);
    void handleModules(const CdbResponse &response);
    void handleMemory(const CdbResponse &response, const MemoryViewCookie &memViewCookie);
    void handleWidgetAt(const CdbResponse &response);
    void handleBreakPoints(const CdbResponse &response);
    void handleBreakPoints(const GdbMi &value);
    void handleAdditionalQmlStack(const CdbResponse &response);
    NormalizedSourceFileName sourceMapNormalizeFileNameFromDebugger(const QString &f);
    void updateLocalVariable(const QByteArray &iname);
    void updateLocals(bool forNewStackFrame);
    int elapsedLogTime() const;
    void addLocalsOptions(ByteArrayInputStream &s) const;
    unsigned parseStackTrace(const GdbMi &data, bool sourceStepInto);
    void mergeStartParametersSourcePathMap();

    const QByteArray m_tokenPrefix;

    QProcess m_process;
    QScopedPointer<Utils::ConsoleProcess> m_consoleStub;
    DebuggerStartMode m_effectiveStartMode;
    QByteArray m_outputBuffer;
    //! Debugger accessible (expecting commands)
    bool m_accessible;
    SpecialStopMode m_specialStopMode;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr m_signalOperation;
    int m_nextCommandToken;
    QList<CdbCommandPtr> m_builtinCommandQueue;
    int m_currentBuiltinCommandIndex; //!< Current command whose output is recorded.
    QList<CdbCommandPtr> m_extensionCommandQueue;
    QMap<QString, NormalizedSourceFileName> m_normalizedFileCache;
    const QByteArray m_extensionCommandPrefixBA; //!< Library name used as prefix
    bool m_operateByInstructionPending; //!< Creator operate by instruction action changed.
    bool m_operateByInstruction;
    bool m_verboseLogPending; //!< Creator verbose log action changed.
    bool m_verboseLog;
    bool m_notifyEngineShutdownOnTermination;
    bool m_hasDebuggee;
    enum Wow64State {
        wow64Uninitialized,
        noWow64Stack,
        wow64Stack32Bit,
        wow64Stack64Bit
    } m_wow64State;
    QTime m_logTime;
    mutable int m_elapsedLogTime;
    QByteArray m_extensionMessageBuffer;
    bool m_sourceStepInto;
    int m_watchPointX;
    int m_watchPointY;
    PendingBreakPointMap m_pendingBreakpointMap;
    PendingBreakPointMap m_insertSubBreakpointMap;
    PendingBreakPointMap m_pendingSubBreakpointMap;
    bool m_autoBreakPointCorrection;
    QHash<QString, QString> m_fileNameModuleHash;
    QMultiHash<QString, quint64> m_symbolAddressCache;
    bool m_ignoreCdbOutput;
    QVariantList m_customSpecialStopData;
    QList<SourcePathMapping> m_sourcePathMappings;
    QScopedPointer<GdbMi> m_coreStopReason;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CDBENGINE_H
