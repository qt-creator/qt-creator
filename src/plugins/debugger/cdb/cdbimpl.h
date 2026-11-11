// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../debuggerengineinterface.h"

#include <utils/filepath.h>
#include <utils/result.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>

#include <chrono>
#include <functional>
#include <optional>

#include <QTimer>

namespace Debugger::Internal {

DEBUGGER_EXPORT bool stoppedInArtificialThread(const GdbMi &stopData);

DEBUGGER_EXPORT std::optional<QString> stoppedByException(const GdbMi &stopData);

DEBUGGER_EXPORT QStringList scriptMessages(const GdbMi &data);

DEBUGGER_EXPORT QString debuggeeOutput(const QString &hexEncodedUtf16);

DEBUGGER_EXPORT QString extensionCommandLines(const QString &prefix, int token,
                                              const QString &arguments);

enum class StepIntoLanding { Arrived, OnThunk, WithoutSource };

DEBUGGER_EXPORT StepIntoLanding stepIntoLanding(
    const GdbMi &stopData, const std::function<bool(const QString &)> &hasSource);

DEBUGGER_EXPORT bool isJustMyCodeUserFile(const Utils::FilePath &file,
                                          const Utils::FilePath &projectDirectory);

class DEBUGGER_EXPORT JustMyCode
{
public:
    using IsUserFile = std::function<bool(const QString &)>;

    enum class Next { Stop, ReturnFromHelper, StepOut, StepAgain };

    QString queryArguments() const;
    QStringList adopt(const GdbMi &reply, const IsUserFile &isUserFile,
                      const std::function<QString()> &nextBreakpointId);
    bool isCurrentImageInstrumented() const { return m_currentImageInstrumented; }
    QStringList armCommands() const;
    QStringList disarmCommands() const;
    bool isStepBreakpoint(const QString &id) const;
    bool isStepStop(const GdbMi &stopData) const;
    Next next(const GdbMi &stopData, bool stepInto, const IsUserFile &isUserFile) const;
    void forget(quint64 base);
    QStringList takeForgottenBreakpointIds();

    static bool leavesForUserCode(const GdbMi &stack, const IsUserFile &isUserFile);

private:
    struct Image
    {
        quint64 base = 0;
        quint64 threadIdAddress = 0;
        QString breakpointId;
    };

    bool isOnStartLine(const GdbMi &stack) const;

    QList<Image> m_images;
    QStringList m_forgottenBreakpointIds;
    QString m_threadId;
    GdbMi m_startFrame;
    int m_startDepth = 0;
    bool m_currentImageInstrumented = false;
};

class DEBUGGER_EXPORT BreakpointStopMessages
{
public:
    QStringList tracepointMessages;
    QString plainMessage;
    bool stopAfterwards = false;
};

DEBUGGER_EXPORT BreakpointStopMessages breakpointStopMessages(
    const QHash<QString, BreakpointParameters> &inserted, const QString &stoppedId);

DEBUGGER_EXPORT GdbMi resolvedBreakpointUpdates(
    const GdbMi &reported,
    QSet<QString> *wanted,
    const QList<QPair<QString, QString>> &sourcePathMap,
    const QHash<QString, QString> &conditions);

DEBUGGER_EXPORT GdbMi modulesTree(const GdbMi &reply);

DEBUGGER_EXPORT QString registerTypeName(const QString &reportedType);

DEBUGGER_EXPORT GdbMi registersTree(const GdbMi &reply);

DEBUGGER_EXPORT QString setParameterArguments(bool reportFirstChance, bool reportSecondChance);

DEBUGGER_EXPORT QList<quint64> symbolAddresses(const QString &reply);

// What to disassemble, empty when there is nothing to go by.
class DEBUGGER_EXPORT DisassemblyRange
{
public:
    quint64 start = 0;
    quint64 end = 0;

    bool isEmpty() const { return start == end; }
};

DEBUGGER_EXPORT DisassemblyRange disassemblyRange(quint64 address,
                                                  const QList<quint64> &functionAddresses);

DEBUGGER_EXPORT QString normalizedSourceFileName(
    const QString &file,
    QHash<QString, QString> *cache = nullptr,
    const std::function<bool(const QString &)> &isFile = {});

DEBUGGER_EXPORT QStringList cdbModuleNames(const Utils::FilePaths &binaries);

DEBUGGER_EXPORT QString cdbModuleName(const Utils::FilePaths &binaries);

DEBUGGER_EXPORT BreakpointParameters scopedToModule(
    const BreakpointParameters &params,
    const std::function<QString(const Utils::FilePath &)> &moduleForSourceFile);

DEBUGGER_EXPORT QString breakpointInsertCommand(
    const BreakpointParameters &params,
    const QString &id,
    const QList<QPair<QString, QString>> &sourcePathMap,
    const QString &location = {});

class DEBUGGER_EXPORT CdbImplSearchPaths
{
public:
    QStringList symbolPaths;
    QStringList sourcePaths;
    // What the debugger reports against where the sources are, in native form.
    QList<QPair<QString, QString>> sourcePathMap;
};

class DEBUGGER_EXPORT CdbImplStartData
{
public:
    Utils::ProcessRunData debuggerRunData;
    InferiorStartData inferiorStartData;
    Utils::FilePath extensionDir;
    QString extensionFileName;
    Utils::FilePath dumperScriptsDir;
    CdbImplSearchPaths searchPaths;
    // cdb event codes to break on, as the options page spells them.
    QStringList breakEvents;
    // One raw string, as the options page takes it.
    QString additionalArguments;
    QStringList startupCommands;
    Utils::FilePath extraDumperFile;
    QString extraDumperCommands;
    // Stop where the program starts, before anything of it has run.
    bool breakOnMain = false;
    bool enableHeapDebugging = false;
    bool ignoreFirstChanceAccessViolation = false;
    // The program gets a console of its own, so its output goes there.
    bool useTerminal = false;
    // The debug C runtime to break in when it reports a problem, empty for none.
    QString crtDebugReportModule;
    bool reportFirstChanceExceptions = false;
    bool reportSecondChanceExceptions = false;
    int inferiorWordWidth = 64;
    bool nativeMixed = false;
    int qtVersion = 0;
    QString qtNamespace;
    // Only the ctrl-c stub next to the qtcreator executable makes
    // Process::interrupt() reach a console cdb.exe.
    bool useCtrlCStub = false;
    std::chrono::seconds watchdogTimeout{0};
    // What the sources of a file end up in, for scoping a breakpoint to a
    // module that is not loaded yet.
    std::function<QString(const Utils::FilePath &)> moduleForSourceFile;
    // Step through the code of the user only, in binaries built with /JMC.
    bool justMyCode = false;
    // Where the code of the user is, for Just My Code. Empty for all but the
    // runtime, the standard library and the Windows SDK.
    Utils::FilePath projectDirectory;
};

Utils::Result<QByteArray> cdbEnvironmentScript(const Utils::Environment &env,
                                               const Utils::Environment &deviceEnvironment,
                                               const Utils::FilePath &workingDirectory,
                                               const Utils::CommandLine &cdb,
                                               QStringList *skippedKeys = nullptr);
Utils::FilePath cdbScriptDir(const Utils::FilePath &cdb,
                             const Utils::Environment &deviceEnvironment);
Utils::Result<Utils::CommandLine> cdbDeviceCommandLine(const Utils::FilePath &script,
                                                       const Utils::FilePath &cmdExe);

class CdbDeviceStart
{
public:
    Utils::CommandLine command;
    Utils::FilePath scriptFile;
    QStringList warnings;
};

using CdbScriptStager = std::function<Utils::Result<Utils::FilePath>(
    const QByteArray &contents, const Utils::FilePath &dir)>;

Utils::Result<CdbDeviceStart> cdbDeviceStart(
    const Utils::CommandLine &cdb, const Utils::Environment &env,
    const Utils::Result<Utils::Environment> &deviceEnvironment,
    const Utils::FilePath &workingDirectory, const CdbScriptStager &stageScript);

class DEBUGGER_EXPORT CdbImpl final : public DebuggerEngineInterface
{
    Q_OBJECT

public:
    explicit CdbImpl(const CdbImplStartData &startData);
    ~CdbImpl() override;

private:
#ifdef WITH_TESTS
    friend class DebuggerUnitTests;
#endif

    Utils::Result<Utils::FilePath> stageScript(const QByteArray &contents,
                                               const Utils::FilePath &dir = {});
    void removeStagedScripts();
    Utils::Result<> setupProcess();
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

    std::optional<bool> registersFollowSelectedFrame() const override { return false; }
    void accessMemory(MemoryOp op, quint64 requestId, quint64 addr, quint64 lengthOrSize,
                      const QByteArray &data) final;
    void fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName,
                          DisassemblyFlavor flavor) final;
    void disassembleFunction(quint64 requestId, quint64 address, const QString &functionName,
                             const QList<quint64> &functionAddresses);
    void disassemble(quint64 requestId, const DisassemblyRange &range);
    void setPeripheralRegisterValue(quint64 address, quint64 value) final;
    void watchPoint(quint64 requestId, const QPoint &pnt) final;
    void createSnapshot(quint64 requestId) final;

    void executeDebuggerCommand(const QString &command,
                                const WatchItemData &inspectorItem) final;

    void handleInitialSessionIdle();
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
    bool isAttach() const;
    bool isCore() const;
    bool isRemoteServer() const;
    QList<QPair<QString, QString>> sourcePathMap() const;
    enum class Wow64State { Unknown, None, Stack32Bit };
    void ensureStackBitness(const std::function<void()> &whenReady);
    void checkStackBitness(bool maySwitch);
    void settleStackBitness(Wow64State state);
    void syncExceptionEvents();
    void interruptInferior();
    QString executableModule() const;
    void insertMainBreakpoint();
    void insertCrtDebugReportBreakpoints();
    void resumeAfterSetup();
    class InterpreterBreakpoint
    {
    public:
        int modelId = 0;
        BreakpointParameters params;
    };
    void resolvePendingInterpreterBreakpoints();
    void handleInterpreterMessage(const GdbMi &stopData);
    void reportSplicedStack(quint64 requestId, const GdbMi &nativeFrames);
    void runInterpreterCommand(const QString &function, const QString &resumeCommand,
                               bool armNativeCallHook = false);
    void setNativeCallHookArmed(bool armed);
    bool handleNativeCallStop(const GdbMi &stopData, const QString &stopFunction);
    void stepIntoNativeCallHookBody(const GdbMi &stopData);
    void descendIntoNativeMethod(const GdbMi &stopData);
    void stepOutOfMetacallTrampoline(const GdbMi &stopData);
    void stepIntoNativeMethodBody(const GdbMi &stopData);
    void armInterpreterMessageWatch();
    void resumeFromInternalStop();
    void restartWatchdog();
    void insertInterpreterBreakpoint(quint64 requestId, int modelId,
                                     const BreakpointParameters &params, bool report);
    void changeInterpreterBreakpoint(const BreakpointChangeRequest &request);
    void armInterpreterHooks();
    void runServiceCommand(const DebuggerCommand &cmd);
    void armServiceSafePoint();
    void handleServiceSafePoint(const GdbMi &stopData);
    void finishServiceSafePoint();
    void loadConfiguredDumpers();
    void setupScripting();
    enum class DumperPipe { Initial, Reload };
    Utils::Result<QStringList> pipeDumperModules(DumperPipe mode);
    void flushPendingBridgeWork();
    void adjustOperateByInstruction(bool operateByInstruction);
    bool isJustMyCodeUserFile(const QString &debuggerFile) const;
    bool stepsWithJustMyCode() const;
    void stepWithJustMyCode(const QString &command, bool stepInto);
    void setJustMyCodeArmed(bool armed);
    bool continueJustMyCodeStep(const GdbMi &stopData);
    void endJustMyCodeStep();
    void updateJustMyCodeAtStop(const GdbMi &stopData, bool ownBreakIn);
    void jumpToAddress(quint64 address, const Utils::FilePath &file, int line);
    QString nextBreakpointId();
    struct ResolvedFunction
    {
        QString name;
        QString qualifiedName;
        QString file;
        int line = 0;
        quint64 address = 0;
        quint64 entryAddress = 0;
    };
    static void parseFunctionDisassembly(const QString &reply, ResolvedFunction *function);
    void insertFunctionBreakpoint(quint64 requestId, const QString &id,
                                  const BreakpointParameters &params, bool report);
    void setResolvedFunctionBreakpoints(quint64 requestId, const QString &id,
                                        const BreakpointParameters &params,
                                        const QList<ResolvedFunction> &functions,
                                        bool report);
    void reportBreakpointInserted(quint64 requestId, const QString &id, bool enabled,
                                  const QString &file, int line, const QString &function,
                                  const GdbMi &locations, bool report);
    void listBreakpoints();

    CdbImplStartData m_startData;
    Utils::Process m_cdbProc;
    Utils::FilePath m_initScriptFile;
    Utils::FilePath m_environmentScriptFile;
    QTimer m_watchdog;

    QString m_extensionCommandPrefix = "!qtcreatorcdbext.";
    QString m_tokenPrefix = "<token>";
    int m_nextCommandToken = 0;
    QHash<int, DebuggerCommand> m_commandForToken;

    int m_currentBuiltinResponseToken = -1;
    QString m_currentBuiltinResponse;

    QString m_extensionMessageBuffer;

    bool m_accessible = false;
    bool m_initialSessionIdleHandled = false;
    bool m_expectSpontaneousStop = false;
    bool m_inInternalStop = false;
    bool m_stopReported = false;
    bool m_expectStaleStop = false;
    bool m_callbackStop = false;
    QList<DebuggerCommand> m_deferredCommands;
    QHash<QString, bool> m_throwBreakpoints;
    Wow64State m_wow64State = Wow64State::Unknown;
    QList<std::function<void()>> m_pendingStackBitness;
    bool m_interpreterMessageWatchArmed = false;
    bool m_interpreterStepArmed = false;
    bool m_atNativeToQmlBoundary = false;
    QSet<QString> m_nativeCallHookIds;
    bool m_nativeCallHookArmed = false;
    QString m_metacallBreakpointId;
    enum class NativeCallDescent { None, ToHookBody, ToMetacall, SteppingOut, ToMethodBody };
    NativeCallDescent m_nativeCallDescent = NativeCallDescent::None;
    int m_nativeCallStepsTaken = 0;
    Utils::ProcessHandle m_inferiorPid;
    bool m_sourceStepInto = false;
    bool m_nativeMethodBodyHopPending = false;
    int m_thunkStepsTaken = 0;
    JustMyCode m_justMyCode;
    bool m_justMyCodeStepping = false;
    bool m_justMyCodeStepInto = false;
    bool m_justMyCodeArmed = false;
    bool m_justMyCodeUnsupported = false;
    int m_lastJustMyCodeQuery = 0;
    int m_pendingJustMyCodeQuery = 0;
    bool m_inferiorRunning = false;
    bool m_stopAnnounced = false;
    bool m_interruptRequested = false;
    bool m_inferiorExited = false;
    bool m_shuttingDown = false;

    std::optional<bool> m_lastOperateByInstruction;

    int m_nextBreakpointId = 1;

    QHash<QString, int> m_breakpointHitCounts;

    QSet<QString> m_internalBreakpointIds;

    QStringList m_runToBreakpointIds;

    QHash<QString, QString> m_parentForSubBreakpointId;

    QHash<QString, QList<quint64>> m_symbolAddressCache;
    QHash<QString, QString> m_normalizedFileCache;
    QHash<QString, QString> m_conditionForBreakpointId;
    QHash<QString, BreakpointParameters> m_insertedBreakpoints;
    QSet<QString> m_unresolvedBreakpointIds;
    bool m_resumeWhenRepliesDrain = false;
    bool m_isResetRestart = false;
    DebuggerCommand m_lastDebuggableCommand;
    bool m_evaluatingCondition = false;
    bool m_expandingTracepoint = false;
    unsigned m_pythonVersion = 0;
    QSet<QString> m_interpreterResolverIds;
    QSet<QString> m_interpreterMessageIds;
    QList<std::function<void()>> m_pendingBridgeWork;
    QList<InterpreterBreakpoint> m_pendingInterpreterBreakpoints;
    QHash<int, int> m_interpreterBreakpointNumbers;
    QString m_qtCoreModule;
    QString m_qmlThreadId;
    QString m_serviceSafePointId;
    QList<DebuggerCommand> m_serviceCommands;
    bool m_atInterruptStop = false;
    bool m_resumeWhenServiceRepliesDrain = false;
};

} // namespace Debugger::Internal
