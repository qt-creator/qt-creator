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
    typedef QSharedPointer<CdbCommand> CdbCommandPtr;
    typedef std::function<void(const CdbResponse &)> CommandHandler;

    CdbEngine(const DebuggerRunParameters &sp);
    ~CdbEngine();

    // Factory function that returns 0 if the debug engine library cannot be found.

    bool canHandleToolTip(const DebuggerToolTipContext &context) const override;

    DebuggerEngine *cppEngine() override { return this; }

    void setupEngine() override;
    void setupInferior() override;
    void runEngine() override;
    void shutdownInferior() override;
    void shutdownEngine() override;
    void abortDebugger() override;
    void detachDebugger() override;
    bool hasCapability(unsigned cap) const override;
    void watchPoint(const QPoint &) override;
    void setRegisterValue(const QByteArray &name, const QString &value) override;

    void executeStep() override;
    void executeStepOut() override;
    void executeNext() override;
    void executeStepI() override;
    void executeNextI() override;

    void continueInferior() override;
    void interruptInferior() override;

    void executeRunToLine(const ContextData &data) override;
    void executeRunToFunction(const QString &functionName) override;
    void executeJumpToLine(const ContextData &data) override;
    void assignValueInDebugger(WatchItem *w, const QString &expr, const QVariant &value) override;
    void executeDebuggerCommand(const QString &command, DebuggerLanguages languages) override;

    void activateFrame(int index) override;
    void selectThread(ThreadId threadId) override;

    bool stateAcceptsBreakpointChanges() const override;
    bool acceptsBreakpoint(Breakpoint bp) const override;
    void attemptBreakpointSynchronization() override;

    void fetchDisassembler(DisassemblerAgent *agent) override;
    void fetchMemory(MemoryAgent *, QObject *, quint64 addr, quint64 length) override;
    void changeMemory(Internal::MemoryAgent *, QObject *, quint64 addr,
                      const QByteArray &data) override;

    void reloadModules() override;
    void loadSymbols(const QString &moduleName) override;
    void loadAllSymbols() override;
    void requestModuleSymbols(const QString &moduleName) override;

    void reloadRegisters() override;
    void reloadSourceFiles() override;
    void reloadFullStack() override;
    void loadAdditionalQmlStack() override;
    void listBreakpoints();

    static QString extensionLibraryName(bool is64Bit);

private slots:
    void readyReadStandardOut();
    void readyReadStandardError();
    void processError();
    void processFinished();
    void postCommand(const QByteArray &cmd);
    void postBuiltinCommand(const QByteArray &cmd,
                            CommandHandler handler);

    void postExtensionCommand(const QByteArray &cmd,
                              const QByteArray &arguments,
                              CommandHandler handler);

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


    bool startConsole(const DebuggerRunParameters &sp, QString *errorMessage);
    void init();
    unsigned examineStopReason(const GdbMi &stopReason, QString *message,
                               QString *exceptionBoxMessage,
                               bool conditionalBreakPointTriggered = false);
    void processStop(const GdbMi &stopReason, bool conditionalBreakPointTriggered = false);
    bool commandsPending() const;
    void handleExtensionMessage(char t, int token, const QByteArray &what, const QByteArray &message);
    bool doSetupEngine(QString *errorMessage);
    bool launchCDB(const DebuggerRunParameters &sp, QString *errorMessage);
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
    void handleStackTrace(const CdbResponse &);
    void handleRegisters(const CdbResponse &);
    void handleDisassembler(const CdbResponse &, DisassemblerAgent *agent);
    void handleJumpToLineAddressResolution(const CdbResponse &response, const ContextData &context);
    void handleExpression(const CdbResponse &command, BreakpointModelId id, const GdbMi &stopReason);
    void handleResolveSymbol(const CdbResponse &command, const QString &symbol, DisassemblerAgent *agent);
    void handleResolveSymbolHelper(const QList<quint64> &addresses, DisassemblerAgent *agent);
    void handleBreakInsert(const CdbResponse &response, const BreakpointModelId &bpId);
    void handleCheckWow64(const CdbResponse &response, const GdbMi &stack);
    void ensureUsing32BitStackInWow64(const CdbResponse &response, const GdbMi &stack);
    void handleSwitchWow64Stack(const CdbResponse &response);
    void jumpToAddress(quint64 address);
    void handleCreateFullBackTrace(const CdbResponse &response);

    // Extension commands
    void handleThreads(const CdbResponse &response);
    void handlePid(const CdbResponse &response);
    void handleLocals(const CdbResponse &response, bool partialUpdate);
    void handleExpandLocals(const CdbResponse &response);
    void handleRegistersExt(const CdbResponse &response);
    void handleModules(const CdbResponse &response);
    void handleMemory(const CdbResponse &response, const MemoryViewCookie &memViewCookie);
    void handleWidgetAt(const CdbResponse &response);
    void handleBreakPoints(const CdbResponse &response);
    void handleBreakPoints(const GdbMi &value);
    void handleAdditionalQmlStack(const CdbResponse &response);
    NormalizedSourceFileName sourceMapNormalizeFileNameFromDebugger(const QString &f);
    void doUpdateLocals(const UpdateParameters &params) override;
    void updateAll() override;
    int elapsedLogTime() const;
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
    QByteArray m_currentBuiltinCommandReply;
    QList<CdbCommandPtr> m_extensionCommandQueue;
    QMap<QString, NormalizedSourceFileName> m_normalizedFileCache;
    const QByteArray m_extensionCommandPrefixBA; //!< Library name used as prefix
    bool m_operateByInstructionPending; //!< Creator operate by instruction action changed.
    bool m_operateByInstruction;
    bool m_verboseLogPending; //!< Creator verbose log action changed.
    bool m_verboseLog;
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
