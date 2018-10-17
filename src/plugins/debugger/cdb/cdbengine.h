/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <debugger/debuggerengine.h>
#include <debugger/breakhandler.h>

#include <cplusplus/CppDocument.h>

#include <projectexplorer/devicesupport/idevice.h>

#include <QTime>

namespace Debugger {
namespace Internal {

class CdbCommand;
struct MemoryViewCookie;
class StringInputStream;

class CdbEngine : public CppDebuggerEngine
{
    Q_OBJECT

public:
    using CdbCommandPtr = QSharedPointer<CdbCommand>;
    using CommandHandler = std::function<void (const DebuggerResponse &)>;

    explicit CdbEngine();
    ~CdbEngine() override;

    bool canHandleToolTip(const DebuggerToolTipContext &context) const override;

    void setupEngine() override;
    void runEngine() override;
    void shutdownInferior() override;
    void shutdownEngine() override;
    void abortDebuggerProcess() override;
    void detachDebugger() override;
    bool hasCapability(unsigned cap) const override;
    void watchPoint(const QPoint &) override;
    void setRegisterValue(const QString &name, const QString &value) override;

    void executeStepOver(bool byInstruction) override;
    void executeStepIn(bool byInstruction) override;
    void executeStepOut() override;

    void continueInferior() override;
    void interruptInferior() override;

    void executeRunToLine(const ContextData &data) override;
    void executeRunToFunction(const QString &functionName) override;
    void executeJumpToLine(const ContextData &data) override;
    void assignValueInDebugger(WatchItem *w, const QString &expr, const QVariant &value) override;
    void executeDebuggerCommand(const QString &command) override;

    void activateFrame(int index) override;
    void selectThread(const Thread &thread) override;

    bool stateAcceptsBreakpointChanges() const override;
    bool acceptsBreakpoint(const BreakpointParameters &params) const override;

    void insertBreakpoint(const Breakpoint &bp) override;
    void removeBreakpoint(const Breakpoint &bp) override;
    void updateBreakpoint(const Breakpoint &bp) override;
    void enableSubBreakpoint(const SubBreakpoint &sbp, bool on) override;

    void fetchDisassembler(DisassemblerAgent *agent) override;
    void fetchMemory(MemoryAgent *, quint64 addr, quint64 length) override;
    void changeMemory(MemoryAgent *, quint64 addr, const QByteArray &data) override;

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

private:
    void readyReadStandardOut();
    void readyReadStandardError();
    void processError();
    void processFinished();
    void runCommand(const DebuggerCommand &cmd) override;
    void adjustOperateByInstruction(bool);

    void createFullBacktrace();

    void handleDoInterruptInferior(const QString &errorMessage);

    typedef QPair<QString, QString> SourcePathMapping;
    struct NormalizedSourceFileName // Struct for caching mapped/normalized source files.
    {
        NormalizedSourceFileName(const QString &fn = QString(), bool e = false) : fileName(fn), exists(e) {}

        QString fileName;
        bool exists;
    };

    enum StopMode {
        NoStopRequested,
        Interrupt,
        Callback
    };

    enum ParseStackResultFlags // Flags returned by parseStackTrace
    {
        ParseStackStepInto = 1, // Need to execute a step, hit on a call frame in "Step into"
        ParseStackStepOut = 2, // Need to step out, hit on a frame without debug information
        ParseStackWow64 = 3 // Hit on a frame with 32bit emulation, switch debugger to 32 bit mode
    };
    enum CommandFlags {
        NoFlags = 0,
        BuiltinCommand,
        ExtensionCommand,
        ScriptCommand
    };

    void init();
    unsigned examineStopReason(const GdbMi &stopReason, QString *message,
                               QString *exceptionBoxMessage,
                               bool conditionalBreakPointTriggered = false);
    void processStop(const GdbMi &stopReason, bool conditionalBreakPointTriggered = false);
    bool commandsPending() const;
    void handleExtensionMessage(char t, int token, const QString &what, const QString &message);
    bool doSetupEngine(QString *errorMessage);
    void handleSessionAccessible(unsigned long cdbExState);
    void handleSessionInaccessible(unsigned long cdbExState);
    void handleSessionIdle(const QString &message);
    using InterruptCallback = std::function<void()>;
    void doInterruptInferior(const InterruptCallback &cb = InterruptCallback());
    void doContinueInferior();
    void parseOutputLine(QString line);
    bool isCdbProcessRunning() const { return m_process.state() != QProcess::NotRunning; }
    bool canInterruptInferior() const;
    inline void postDisassemblerCommand(quint64 address, DisassemblerAgent *agent);
    void postDisassemblerCommand(quint64 address, quint64 endAddress,
                                 DisassemblerAgent *agent);
    void postResolveSymbol(const QString &module, const QString &function,
                           DisassemblerAgent *agent);
    void showScriptMessages(const QString &message) const;
    void handleInitialSessionIdle();
    // Builtin commands
    void handleStackTrace(const DebuggerResponse &);
    void handleRegisters(const DebuggerResponse &);
    void handleJumpToLineAddressResolution(const DebuggerResponse &response, const ContextData &context);
    void handleExpression(const DebuggerResponse &command, const Breakpoint &bp, const GdbMi &stopReason);
    void handleResolveSymbol(const DebuggerResponse &command, const QString &symbol, DisassemblerAgent *agent);
    void handleResolveSymbolHelper(const QList<quint64> &addresses, DisassemblerAgent *agent);
    void handleBreakInsert(const DebuggerResponse &response, const Breakpoint &bp);
    void handleCheckWow64(const DebuggerResponse &response, const GdbMi &stack);
    void ensureUsing32BitStackInWow64(const DebuggerResponse &response, const GdbMi &stack);
    void handleSwitchWow64Stack(const DebuggerResponse &response);
    void jumpToAddress(quint64 address);

    // Extension commands
    void handleThreads(const DebuggerResponse &response);
    void handleLocals(const DebuggerResponse &response, bool partialUpdate);
    void handleExpandLocals(const DebuggerResponse &response);
    void handleRegistersExt(const DebuggerResponse &response);
    void handleModules(const DebuggerResponse &response);
    void handleWidgetAt(const DebuggerResponse &response);
    void handleBreakPoints(const DebuggerResponse &response);
    void handleAdditionalQmlStack(const DebuggerResponse &response);
    void setupScripting(const DebuggerResponse &response);
    NormalizedSourceFileName sourceMapNormalizeFileNameFromDebugger(const QString &f);
    void doUpdateLocals(const UpdateParameters &params) override;
    void updateAll() override;
    int elapsedLogTime() const;
    unsigned parseStackTrace(const GdbMi &data, bool sourceStepInto);
    void mergeStartParametersSourcePathMap();

    const QString m_tokenPrefix;
    void handleSetupFailure(const QString &errorMessage);

    QProcess m_process;
    DebuggerStartMode m_effectiveStartMode = NoStartMode;
    QByteArray m_outputBuffer;
    //! Debugger accessible (expecting commands)
    bool m_accessible = false;
    StopMode m_stopMode = NoStopRequested;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr m_signalOperation;
    int m_nextCommandToken = 0;
    QHash<int, DebuggerCommand> m_commandForToken;
    QString m_currentBuiltinResponse;
    int m_currentBuiltinResponseToken = -1;
    QMap<QString, NormalizedSourceFileName> m_normalizedFileCache;
    const QString m_extensionCommandPrefix; //!< Library name used as prefix
    bool m_lastOperateByInstruction = true; // Default CDB setting.
    bool m_hasDebuggee = false;
    enum Wow64State {
        wow64Uninitialized,
        noWow64Stack,
        wow64Stack32Bit,
        wow64Stack64Bit
    } m_wow64State = wow64Uninitialized;
    QTime m_logTime;
    mutable int m_elapsedLogTime = 0;
    QString m_extensionMessageBuffer;
    bool m_sourceStepInto = false;
    int m_watchPointX = 0;
    int m_watchPointY = 0;
    QSet<Breakpoint> m_pendingBreakpointMap;
    bool m_autoBreakPointCorrection = false;
    QMultiHash<QString, quint64> m_symbolAddressCache;
    bool m_ignoreCdbOutput = false;
    QList<InterruptCallback> m_interrupCallbacks;
    QList<SourcePathMapping> m_sourcePathMappings;
    QScopedPointer<GdbMi> m_coreStopReason;
    int m_pythonVersion = 0; // 0xMMmmpp MM = major; mm = minor; pp = patch
    bool m_initialSessionIdleHandled = false;
    mutable CPlusPlus::Snapshot m_codeModelSnapshot;
};

} // namespace Internal
} // namespace Debugger
