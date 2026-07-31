// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../debuggerengineinterface.h"

#include <utils/filepath.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

#include <functional>
#include <optional>

namespace Debugger::Internal {

class DEBUGGER_EXPORT CdbImplStartData
{
public:
    Utils::ProcessRunData debuggerRunData;
    InferiorStartData inferiorStartData;
    Utils::FilePath extensionDir;
    QString extensionFileName;
    Utils::FilePath dumperScriptsDir;
    int inferiorWordWidth = 64;
    bool nativeMixed = false;
    // Only the ctrl-c stub next to the qtcreator executable makes
    // Process::interrupt() reach a console cdb.exe.
    bool useCtrlCStub = false;
};

class DEBUGGER_EXPORT CdbImpl final : public DebuggerEngineInterface
{
    Q_OBJECT

public:
    explicit CdbImpl(const CdbImplStartData &startData);
    ~CdbImpl() override;

private:
    void start() final;
    void shutdownInferior(ShutdownMode mode) final;
    void shutdownEngine() final;

    void execute(const ExecutionRequest &request) final;
    void changeBreakpoint(const BreakpointChangeRequest &request) final;
    void refresh(const RefreshRequest &request) final;

    void activateFrame(int index) final;
    void selectThread(const QString &threadId) final;

    void setRegisterValue(const QString &name, const QString &value) final;
    void assignValueInDebugger(const WatchItemData &item, const QString &expr,
                               const QString &value) final;

    void accessMemory(MemoryOp op, quint64 requestId, quint64 addr, quint64 lengthOrSize,
                      const QByteArray &data) final;
    void fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName) final;
    void setPeripheralRegisterValue(quint64 address, quint64 value) final;
    void watchPoint(quint64 requestId, const QPoint &pnt) final;
    void createSnapshot(quint64 requestId) final;

    void executeDebuggerCommand(const QString &command,
                                const WatchItemData &inspectorItem) final;

    void handleCdbOutputLine(const QString &rawLine);
    void handleExtensionMessage(char type, int token, const QString &what,
                                const QString &payload);
    void reportStop(const GdbMi &stopData);

    void insertBreakpoint(quint64 requestId, const QString &id, int modelId,
                          const BreakpointParameters &params, bool report);
    void reportTracepoint(const QStringList &tracepointMessages, const GdbMi &stopData,
                          bool stopAfterwards);

    void runCommand(const DebuggerCommand &command);
    void initializeSession(const std::function<void()> &whenReady);
    void restartSession();
    void resumeAfterSetup();
    class InterpreterBreakpoint
    {
    public:
        int modelId = 0;
        BreakpointParameters params;
    };
    void resolvePendingInterpreterBreakpoints();
    void handleInterpreterMessage(const GdbMi &stopData);
    void armInterpreterMessageWatch();
    void resumeFromInternalStop();
    void insertInterpreterBreakpoint(quint64 requestId, int modelId,
                                     const BreakpointParameters &params, bool report);
    void armInterpreterHooks();
    void setupScripting();
    void flushPendingBridgeWork();
    void adjustOperateByInstruction(bool operateByInstruction);
    void jumpToAddress(quint64 address, const Utils::FilePath &file, int line);
    QString nextBreakpointId();
    struct ResolvedFunction
    {
        QString name;
        QString qualifiedName;
        QString file;
        int line = 0;
        quint64 address = 0;
    };
    static void parseFunctionDisassembly(const QString &reply, ResolvedFunction *function);
    void insertFunctionBreakpoint(quint64 requestId, const QString &id, bool enabled,
                                  const QString &module, const QString &functionName,
                                  bool report);
    void setResolvedFunctionBreakpoints(quint64 requestId, const QString &id, bool enabled,
                                        const QString &functionName,
                                        const QList<ResolvedFunction> &functions,
                                        bool report);
    void reportBreakpointInserted(quint64 requestId, const QString &id, bool enabled,
                                  const QString &file, int line, const QString &function,
                                  const GdbMi &locations, bool report);

    CdbImplStartData m_startData;
    Utils::Process m_cdbProc;

    QString m_extensionCommandPrefix = "!qtcreatorcdbext.";
    QString m_tokenPrefix = "<token>";
    int m_nextCommandToken = 0;
    QHash<int, DebuggerCommand> m_commandForToken;

    int m_currentBuiltinResponseToken = -1;
    QString m_currentBuiltinResponse;

    QString m_extensionMessageBuffer;

    bool m_initialSessionIdleHandled = false;
    bool m_expectSpontaneousStop = false;
    bool m_inInternalStop = false;
    bool m_callbackStop = false;
    QList<std::function<void()>> m_interruptCallbacks;
    bool m_interpreterMessageWatchArmed = false;
    QString m_interpreterMessageWatchId;
    bool m_inferiorRunning = false;
    bool m_interruptRequested = false;
    bool m_inferiorExited = false;
    bool m_shuttingDown = false;

    std::optional<bool> m_lastOperateByInstruction;

    int m_nextBreakpointId = 1;

    QHash<QString, int> m_breakpointHitCounts;

    QSet<QString> m_internalBreakpointIds;

    QHash<QString, QString> m_parentForSubBreakpointId;

    QHash<QString, QString> m_conditionForBreakpointId;
    QHash<QString, BreakpointParameters> m_insertedBreakpoints;
    bool m_resumeWhenRepliesDrain = false;
    bool m_isResetRestart = false;
    int m_currentFrameIndex = 0;
    DebuggerCommand m_lastDebuggableCommand;
    bool m_evaluatingCondition = false;
    bool m_expandingTracepoint = false;
    unsigned m_pythonVersion = 0;
    QSet<QString> m_interpreterResolverIds;
    QSet<QString> m_interpreterMessageIds;
    QList<std::function<void()>> m_pendingBridgeWork;
    QList<InterpreterBreakpoint> m_pendingInterpreterBreakpoints;
};

} // namespace Debugger::Internal
