// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../debuggerengineinterface.h"

#include <utils/qtcprocess.h>

#include <qmldebug/qmldebugclient.h>
#include <qmldebug/qmldebugconnection.h>
#include <qmldebug/qmlenginedebugclient.h>

#include <QHash>
#include <QList>
#include <QSet>
#include <QTimer>
#include <QVariantMap>

#include <chrono>
#include <functional>
#include <memory>
#include <optional>

namespace Debugger::Internal {

class DEBUGGER_EXPORT QmlImplStartData
{
public:
    InferiorStartData inferiorStartData;
    // Whom the runtime this session starts itself runs as, empty for the
    // current user. There is no debugger process of its own to run instead.
    QString runAsUser;
    // Zero leaves the commands unwatched.
    std::chrono::seconds watchdogTimeout{0};
    // Whether every command's turnaround goes into the log.
    bool logTimeStamps = false;
};

class DEBUGGER_EXPORT QmlImpl final : public DebuggerEngineInterface
{
    Q_OBJECT

public:
    explicit QmlImpl(const QmlImplStartData &startData);
    ~QmlImpl() override;

private:
    void start() final;
    void shutdownInferior(ShutdownMode mode) final;
    void shutdownEngine() final;

    void execute(const ExecutionRequest &request) final;
    void changeBreakpoint(const BreakpointChangeRequest &request) final;
    void clearBreakpointNumber(const QString &responseId);
    BreakpointChangeRequest withKnownLocation(const BreakpointChangeRequest &request) const;
    bool isEnabledOnlyChange(const BreakpointChangeRequest &request) const;
    void refresh(const RefreshRequest &request) final;

    class RefreshCollector
    {
    public:
        quint64 requestId = 0;
        RefreshKind kind = RefreshKind::Locals;
        int remaining = 0;
        GdbMi items;
        QSet<QString> expandedINames;
        QSet<int> seenDebugIds;
        QString partialVariable;
    };
    std::shared_ptr<RefreshCollector> makeCollector(const RefreshRequest &request);
    std::function<void()> legFinisher(const std::shared_ptr<RefreshCollector> &pending);
    class LookupRequest
    {
    public:
        int handle = 0;
        QString iname;
        QString name;
        QString exp;
    };
    void refreshLocals(const RefreshRequest &request);
    void handleScopeReply(const QVariantMap &response,
                          const std::shared_ptr<RefreshCollector> &pending,
                          const std::function<void()> &finishLeg);
    QList<LookupRequest> appendV8Children(const QString &iname, const QString &exp,
                                          const QVariantMap &resolved,
                                          const std::shared_ptr<RefreshCollector> &pending);
    void lookupHandles(const QList<LookupRequest> &requests,
                       const std::shared_ptr<RefreshCollector> &pending,
                       const std::function<void()> &finishLeg);
    static GdbMi emptyLocalsData();
    void refreshSourceFiles(const RefreshRequest &request);

    using InspectorCallback = std::function<void(const QVariant &value,
                                                const QByteArray &type)>;
    void refreshInspectorTree(const RefreshRequest &request);
    bool runInspectorQuery(quint32 queryId, const InspectorCallback &cb);
    void runInspectorLeg(quint32 queryId, const std::shared_ptr<RefreshCollector> &pending,
                         const std::function<void()> &finishLeg, const InspectorCallback &cb);
    void appendObjectItems(const QmlDebug::ObjectReference &object, const QString &parentIname,
                           int engineId, const std::shared_ptr<RefreshCollector> &pending,
                           const std::function<void()> &finishLeg);
    void addObjectWatch(int debugId);
    void rebuildInspectorTree();
    void handleObjectCreated(int engineId, int objectId, int parentId);
    void handlePropertyValueChanged(int debugId, const QByteArray &name, const QVariant &value);
    void queryObjectExpression(int debugId, const QString &expression);

    void selectThread(const QString &threadId) final;
    void activateFrame(int index) final;
    void setRegisterValue(const QString &name, const QString &value) final;
    void accessMemory(MemoryOp op, quint64 requestId, quint64 addr, quint64 lengthOrSize,
                      const QByteArray &data) final;
    void fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName) final;
    void setPeripheralRegisterValue(quint64 address, quint64 value) final;
    void watchPoint(quint64 requestId, const QPoint &pnt) final;
    void createSnapshot(quint64 requestId) final;

    void assignValueInDebugger(const WatchItemData &item, const QString &expr,
                               const QString &value) final;

    void executeDebuggerCommand(const QString &command,
                                const WatchItemData &inspectorItem) final;

    class V8Client;
    friend class V8Client;

    void handleStateChanged(QmlDebug::QmlDebugClient::State state);
    void handleMessageReceived(const QByteArray &data);

    void runDirectCommand(const QByteArray &type, const QByteArray &msg = {});

    using QmlCallback = std::function<void(const QVariantMap &)>;
    int runCommand(const DebuggerCommand &command, const QmlCallback &cb = {});
    // The service answers in order, so whatever is still outstanding is what a
    // debugger that stopped answering is busy with.
    void restartWatchdog();

    void handleV8Message(const QByteArray &payload);
    void handleConnectHandshakeDone();
    // A breakpoint the runtime has not seen before, or, when the response id
    // the model knows is passed in, the same one again in a restarted runtime.
    void setScriptBreakpoint(quint64 requestId, const BreakpointChangeRequest &request,
                             const QString &knownResponseId = {});
    // The number the runtime addresses a breakpoint by, which a restart changes
    // while the id the model knows stays what it was.
    int serviceNumberFor(const QString &responseId) const;
    void handleBreakEvent(const QVariantMap &response);
    void reportBreakpointHit(const QString &responseId, int hits);
    class TracepointHit
    {
    public:
        QString pattern;
        QList<TracepointCapture> captures;
        GdbMi values;
        GdbMi expressions;
        int outstanding = 1;
        std::function<void()> finished;
    };
    void reportTracepointHit(const QString &responseId, const std::function<void()> &finished);
    void finishTracepointHit(const std::shared_ptr<TracepointHit> &hit);
    void handleExceptionEvent(const QVariantMap &response);

    void sendDisconnect();
    void beginConnection();
    void resetTransientState();
    // Starting the runtime here rather than attaching to one somebody else
    // started: the port it is told to listen on is the one to connect to.
    void launchInferior();

    QmlImplStartData m_startData;
    Utils::Process m_inferiorProcess;
    quint16 m_port = 0;
    QmlDebug::QmlDebugConnection m_connection;
    V8Client *m_v8Client = nullptr;
    QmlDebug::QmlEngineDebugClient *m_engineClient = nullptr;
    int m_connectRetriesLeft = 50;

    int m_sequence = 0;
    QHash<int, QmlCallback> m_callbackForToken;
    class PendingCommand
    {
    public:
        QString command;
        qint64 postTime = 0;
    };
    QHash<int, PendingCommand> m_pendingCommands;
    QTimer m_watchdog;

    QHash<QString, BreakpointChangeRequest> m_activeBreakpointsByResponseId;
    QHash<QString, QString> m_serviceNumberByResponseId;
    QHash<QString, int> m_hitCountsByResponseId;

    bool m_supportChangeBreakpoint = false;
    int m_currentFrameIndex = 0;
    int m_stringLimit = 0;
    std::optional<RefreshRequest> m_deferredWatchers;
    // The locals fetch, kept for RepeatLastCommand.
    std::optional<RefreshRequest> m_lastLocalsRequest;
    bool m_inferiorRunning = false;
    bool m_inferiorExited = false;
    bool m_interruptRequested = false;
    bool m_shuttingDown = false;
    bool m_isResetRestart = false;
    // Whether the user gave up on the session: the runtime's exit then reports
    // the session gone rather than an inferior that ended on its own.
    bool m_aborting = false;
    bool m_disconnected = false;

    QHash<quint32, InspectorCallback> m_inspectorCallbackForQueryId;
    QList<QmlDebug::EngineReference> m_qmlEngines;
    QHash<int, QString> m_inameForDebugId;
    QHash<int, int> m_engineIdForDebugId;
    QSet<QString> m_expandedInspectorINames;
    QList<int> m_objectWatches;
    QHash<int, int> m_knownDelegateIds;
    int m_engineQueryRetriesLeft = 50;
    QTimer *m_objectCreatedTimer = nullptr;
};
} // namespace Debugger::Internal
