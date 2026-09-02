// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dapstartdata.h"

#include "../debuggerengineinterface.h"

#include <utils/filepath.h>

#include <QHash>
#include <QJsonObject>
#include <QMap>
#include <QQueue>

#include <optional>

namespace Debugger::Internal {

class DapClient;
class DebuggerEngine;
enum class DapEventType;
enum class DapResponseType;

// A backend speaking stock Debug Adapter Protocol to an adapter Qt Creator does
// not own. Only what the protocol itself defines is available: no Qt dumpers,
// so values are whatever the adapter chose to print.
class DEBUGGER_EXPORT DapImpl : public DebuggerEngineInterface
{
    Q_OBJECT

public:
    explicit DapImpl(const DapStartData &startData);
    ~DapImpl() override;

protected:
    void start() override;
    void shutdownInferior(ShutdownMode mode) override;
    void shutdownEngine() override;

    void execute(const ExecutionRequest &request) override;
    void changeBreakpoint(const BreakpointChangeRequest &request) override;
    void refresh(const RefreshRequest &request) override;

    void selectThread(const QString &threadId) override;
    void activateFrame(int index) override;
    void setRegisterValue(const QString &name, const QString &value) override;
    void accessMemory(MemoryOp op, quint64 requestId, quint64 addr, quint64 lengthOrSize,
                      const QByteArray &data) override;
    void fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName) override;
    void setPeripheralRegisterValue(quint64 address, quint64 value) override;
    void watchPoint(quint64 requestId, const QPoint &pnt) override;
    void createSnapshot(quint64 requestId) override;

    void assignValueInDebugger(const WatchItemData &item, const QString &expr,
                               const QString &value) override;
    void executeDebuggerCommand(const QString &command,
                                const WatchItemData &inspectorItem) override;

    // A superset announces itself differently: its initialize carries what its
    // own host needs to set up.
    virtual void handleStarted();
    void handleFinished();
    void handleStandardError();
    // A superset extends the dispatch: whatever it does not claim lands here.
    virtual void handleResponse(DapResponseType type, const QJsonObject &response);
    virtual void handleEvent(DapEventType type, const QJsonObject &event);

    void handleStopped(const QJsonObject &event);
    void handleStackTrace(const QJsonObject &response);
    void handleScopes(const QJsonObject &response);
    void handleVariables(const QJsonObject &response);
    void handleReadMemory(const QJsonObject &response);
    void handleDisassemble(const QJsonObject &response);
    void handleBreakpointsSet(const QJsonObject &response);
    void reportStop();
    void reportInferiorDone(const InferiorResultData &result);

    int postRequest(const QString &command, const QJsonObject &arguments = {});
    // The launch body is this layer's passthrough configuration; a superset
    // builds its own from what it was started with.
    virtual void postLaunchOrAttach();
    void reportUnsupported(const QString &what);

    void sendCustomRequest(const QString &command, const QJsonObject &arguments,
                           const DapSessionChannel::Answer &answer);
    void reportRunning(bool running);
    void reportRunRequested();
    void reportRunResult(bool ok);

    const DapStartData m_startData;
    DapClient *m_client = nullptr;
    // Who is waiting for the answer to a request the adapter defines itself.
    QHash<int, DapSessionChannel::Answer> m_customRequests;
    bool m_runningReported = false;
    // The scopes request the locals walk running now belongs to.
    int m_scopesSeq = -1;

    int m_currentThreadId = -1;
    int m_currentFrameId = -1;
    bool m_inferiorRunning = false;
    bool m_stopRequested = false;
    bool m_configured = false;
    // The engine has to hear that the run began before it hears it ended,
    // and an adapter whose debuggee exits at once says both before the
    // configuration it was still answering has been acknowledged.
    bool m_runReported = false;
    bool m_runRequestPending = false;
    std::optional<InferiorResultData> m_pendingResult;

    // The stop event carries no frame, so the location has to be asked for.
    class StackTraceRequest
    {
    public:
        bool reportsStop = false;
        quint64 refreshRequestId = 0;
    };
    QHash<int, StackTraceRequest> m_stackTraceRequests;
    // A foreign adapter hands out its own frame ids, so the view's index into
    // the stack has to be translated back through the ids it reported.
    QList<int> m_frameIds;

private:
    void sendBreakpointsFor(const Utils::FilePath &file);
    void sendFunctionBreakpoints();
    void queueVariables(const QString &iname, int reference);
    void continueLocalsWalk();
    void reportLocals();
    GdbMi localsItem(const QString &iname) const;

    // A breakpoint as the protocol wants it: whole-file arrays, so each one has
    // to be remembered even when only its neighbour changed.
    class Breakpoint
    {
    public:
        quint64 requestId = 0;
        int modelId = 0;
        BreakpointParameters params;
        bool enabled = true;
    };
    QHash<Utils::FilePath, QList<Breakpoint>> m_sourceBreakpoints;
    QList<Breakpoint> m_functionBreakpoints;
    // Which file's answer a setBreakpoints reply is, routed by sequence number.
    QHash<int, Utils::FilePath> m_breakpointRequests;

    // One node of the locals tree. The protocol answers a level at a time, so
    // the tree is collected flat and assembled once the walk is done.
    class Local
    {
    public:
        QString iname;
        QString name;
        QString type;
        QString value;
        quint64 address = 0;
        int reference = 0;
        bool hasChildren = false;
        QStringList childINames;
    };
    quint64 m_localsRequestId = 0;
    QSet<QString> m_expandedINames;
    QMap<QString, Local> m_locals;
    QStringList m_localRoots;
    QQueue<QPair<QString, int>> m_pendingVariables;
    QHash<int, QString> m_variableRequests;
    QHash<int, quint64> m_threadRequests;

    class MemoryRequest
    {
    public:
        quint64 requestId = 0;
        quint64 address = 0;
    };
    QHash<int, MemoryRequest> m_memoryRequests;

    class DisassemblyRequest
    {
    public:
        quint64 requestId = 0;
        quint64 address = 0;
    };
    QHash<int, DisassemblyRequest> m_disassemblyRequests;
};

DEBUGGER_EXPORT DebuggerEngine *createDapAdapterEngine(const DapStartData &data);

} // namespace Debugger::Internal
