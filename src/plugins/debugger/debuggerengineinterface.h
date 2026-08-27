// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"
#include "breakpoint.h"
#include "debuggerconstants.h"
#include "debuggerprotocol.h"
#include "disassemblerlines.h"

#include <utils/filepath.h>
#include <utils/processhandle.h>
#include <utils/processinterface.h>

#include <chrono>
#include <functional>
#include <variant>

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QSet>
#include <QString>
#include <QUrl>

namespace Utils { class ProcessResultData; }

class tst_backends;
class DebuggerBackend;

namespace Debugger::Internal {

Q_NAMESPACE_EXPORT(DEBUGGER_EXPORT)

enum class ExecutionCommand {
    Continue, Interrupt, StepOver, StepIn, StepOut, Return,
    RunToLine, RunToFunction, JumpToLine,
    Detach, ResetInferior, Abort, RecordReverse, RepeatLastCommand,
};

class DEBUGGER_EXPORT ExecutionRequest
{
public:
    ExecutionCommand command = ExecutionCommand::Continue;
    bool flag = false;
    ContextData context = {};
    QString functionName = {};
    bool currentFrameIsQml = false;
};

enum class RefreshKind {
    Modules, ModuleSymbols, ModuleSections,
    Registers, PeripheralRegisters,
    SourceFiles, FullStack, StackSymbols, AllSymbols, QmlStack,
    DebuggingHelpers, Threads,
    Locals,
    FullBacktrace,
    InspectorTree,
};

// What the dumpers need to know about the user's display preferences. All of it
// can change while a session runs, so it travels with the request.
class DEBUGGER_EXPORT DumperOptions
{
public:
    bool useDebuggingHelpers = true;
    bool useDynamicType = true;
    bool showQObjectNames = true;
    bool logTimeStamps = false;
    int maximalStringLength = 10000;
    int displayStringLimit = 100;
};

class DEBUGGER_EXPORT RefreshRequest
{
public:
    quint64 requestId = 0;
    RefreshKind kind = RefreshKind::Modules;
    Utils::FilePath path = {};
    QString partialVariable = {};
    QString context = {};
    QList<quint64> addresses = {};
    QJsonArray watchers = {};
    QSet<QString> expandedINames = {};
    // What WatchHandler::appendFormatRequests() produces. expandedItems is
    // expandedINames plus each item's array count, which the dumpers index by
    // name, so a plain list of names makes them fail on the first container.
    QJsonObject expandedItems = {};
    QJsonObject typeFormats = {};
    QJsonObject individualFormats = {};
    QJsonObject formatTypes = {};
    DumperOptions dumperOptions = {};

    // The dumpers index the expanded items by name to find each one's array count,
    // so a caller that only knows the names gets the default count for all of them.
    QJsonObject expandedForDumpers() const
    {
        if (!expandedItems.isEmpty() || expandedINames.isEmpty())
            return expandedItems;
        QJsonObject result;
        for (const QString &iname : expandedINames)
            result.insert(iname, defaultArrayCount);
        return result;
    }

    static constexpr int defaultArrayCount = 100;
    bool allowInferiorCalls = true;
    bool autoDerefPointers = true;
};

enum class BreakpointOp { Insert, Remove, Update, EnableSub };

enum class LibraryEvent { Loaded, Unloaded };

class DEBUGGER_EXPORT BreakpointChangeRequest
{
public:
    BreakpointOp op = BreakpointOp::Insert;
    quint64 requestId = 0;
    BreakpointParameters params;
    QString responseId;
    QString subResponseId;
    bool enabled = true;
    int modelId = 0;
};

enum class MemoryOp { Fetch, Change };

enum class ShutdownMode { Kill, Detach };

class DEBUGGER_EXPORT AttachToProcessData
{
public:
    Utils::ProcessHandle pid;
};

class DEBUGGER_EXPORT AttachToTerminalStubData
{
public:
    Utils::ProcessHandle pid;
    qint64 mainThreadId = -1;
    Utils::FilePath executable;
};

class DEBUGGER_EXPORT AttachToRemoteServerData
{
public:
    QString channel;
    Utils::FilePath symbolFile;
    Utils::ProcessHandle attachPid;
    Utils::FilePath remoteExecutable;
    bool useQnxTarget = false;
};

class DEBUGGER_EXPORT AttachToCoreData
{
public:
    Utils::FilePath coreFile;
    Utils::FilePath executable;
};

class DEBUGGER_EXPORT AttachToQmlServerData
{
public:
    QUrl server;
};

using InferiorStartData = std::variant<
    Utils::ProcessRunData,
    AttachToProcessData,
    AttachToTerminalStubData,
    AttachToRemoteServerData,
    AttachToCoreData,
    AttachToQmlServerData
>;

enum class DebuggerStartModeFlag
{
    Launch = 1 << 0,
    AttachToProcess = 1 << 1,
    AttachToTerminalStub = 1 << 2,
    AttachToRemoteServer = 1 << 3,
    AttachToCore = 1 << 4,
    AttachToQmlServer = 1 << 5,
};
Q_DECLARE_FLAGS(DebuggerStartModes, DebuggerStartModeFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(DebuggerStartModes)
Q_FLAG_NS(DebuggerStartModes)

class DEBUGGER_EXPORT WatchItemData
{
public:
    qint64 id = -1;
    QString iname;
    QString type;
    bool isLocal = false;
    bool isWatcher = false;
    bool isInspect = false;
};

enum class InferiorEvent {
    RunOk, RunFailed, RunRequested,
    StopOk, StopFailed, SpontaneousStop,
    InferiorIll, ShutdownFinished,
    EngineSetupOk, EngineSetupFailed, EngineRunFailed, EngineIll,
    EngineShutdownFinished, EngineSpontaneousShutdown,
    RunAndInferiorStopOk, RunAndInferiorRunOk, RunOkAndInferiorUnrunnable,
};

enum class InferiorExitStatus {
    Normal,
    Crash,
    Detached,
};

class DEBUGGER_EXPORT InferiorResultData
{
public:
    int exitCode = 0;
    InferiorExitStatus exitStatus = InferiorExitStatus::Normal;
};

class DEBUGGER_EXPORT AcceptsBreakpointQuery
{
public:
    bool isCppBreakpoint() const;
    bool isQmlFileAndLineBreakpoint() const;

    BreakpointType type = UnknownBreakpointType;
    Utils::FilePath fileName;
    DebuggerStartMode startMode = NoStartMode;
    bool isNativeMixedEnabled = false;
};

class DEBUGGER_EXPORT DebuggerEngineSetupData
{
public:
    std::function<bool(const AcceptsBreakpointQuery &)> acceptsBreakpoint;
    unsigned capabilities = 0;
    unsigned attachToCoreCapabilities = 0;
    DebuggerExtraCapabilities extraCapabilities;
    DebuggerStartModes startModes;
    ToolTipHandling toolTipHandling = ToolTipHandling::IfStoppedInferiorAndCppEditor;
};

class DEBUGGER_EXPORT DebuggerEngineInterface : public QObject
{
    Q_OBJECT

public:
    const DebuggerEngineSetupData &setupData() const { return m_setupData; }
    bool hasCapability(unsigned cap, DebuggerStartMode startMode = NoStartMode) const;
    bool hasExtraCapability(DebuggerExtraCapability cap) const;

signals:
    void message(const QString &text, int channel, int timeout = -1);

    void inferiorEvent(InferiorEvent event);

    void breakpointEvent(quint64 requestId, BreakpointOp op, bool ok, const GdbMi &data = {});

    void locationChanged(const Utils::FilePath &fileName, int lineNumber);

    void inferiorDone(const InferiorResultData &resultData);

    void inferiorPidKnown(const Utils::ProcessHandle &pid);

    void engineProcessFinished(const Utils::ProcessResultData &resultData);

    void memoryDataReceived(quint64 requestId, quint64 address, const QByteArray &data);

    void disassemblyReceived(quint64 requestId, const DisassemblerLines &lines);

    void snapshotCreated(quint64 requestId, bool ok, const Utils::FilePath &coreFile);

    void watchPointResolved(quint64 requestId, quint64 address, const QString &expr);

    void refreshDataReceived(quint64 requestId, RefreshKind kind, const GdbMi &data);

    void libraryEvent(LibraryEvent event, const GdbMi &data);

    void breakpointModified(const GdbMi &data);

    void signalReceived(const QString &name, const QString &meaning);

    void notResponding(std::chrono::seconds waited, const QStringList &pendingCommands);

    void interruptTerminalRequested();
    void kickoffTerminalProcessRequested();

protected:
    explicit DebuggerEngineInterface(const DebuggerEngineSetupData &setupData)
        : m_setupData(setupData)
    {}

private:
    virtual void start() = 0;

    virtual void shutdownInferior(ShutdownMode mode) = 0;

    virtual void shutdownEngine() = 0;

    virtual void execute(const ExecutionRequest &request) = 0;

    virtual void refresh(const RefreshRequest &request) = 0;

    virtual void changeBreakpoint(const BreakpointChangeRequest &request) = 0;

    virtual void accessMemory(MemoryOp op, quint64 requestId,
                              quint64 addr, quint64 lengthOrSize, const QByteArray &data = {}) = 0;

    virtual void selectThread(const QString &threadId) = 0;

    virtual void activateFrame(int index) = 0;

    virtual void fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName) = 0;

    virtual void executeDebuggerCommand(const QString &command,
                                        const WatchItemData &inspectorItem) = 0;

    virtual void assignValueInDebugger(const WatchItemData &item, const QString &expr, const QString &value) = 0;

    virtual void setRegisterValue(const QString &name, const QString &value) = 0;

    virtual void setPeripheralRegisterValue(quint64 address, quint64 value) = 0;

    virtual void watchPoint(quint64 requestId, const QPoint &pnt) = 0;

    virtual void createSnapshot(quint64 requestId) = 0;

    friend class DebuggerEngine;
    friend class GenericDebuggerEngine;
    friend class ::tst_backends;
    friend class ::DebuggerBackend;

    DebuggerEngineSetupData m_setupData;
};
} // namespace Debugger::Internal
