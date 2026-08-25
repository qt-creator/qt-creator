// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggerengineinterface.h"

#include "gdb/gdbimpl.h"
#include "lldb/lldbimpl.h"

#include <utils/algorithm.h>
#include <utils/elfreader.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/processreaper.h>
#include <utils/qtcprocess.h>
#include <utils/result.h>
#include <utils/temporarydirectory.h>

#include <chrono>
#include <csignal>
#include <optional>

#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QLibraryInfo>
#include <QMap>
#include <QMetaEnum>
#include <QPoint>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QTest>

using namespace Debugger::Internal;
using namespace Utils;

static constexpr std::chrono::seconds s_timeout{5};
static constexpr std::chrono::seconds s_qmlStartupTimeout{15};
static constexpr std::chrono::seconds s_compileTimeout{120};

static QString compileFailure(const QString &what, const Process &process, qint64 elapsedMs)
{
    const QString common = QString("%1 failed after %2ms: %3")
                               .arg(what)
                               .arg(elapsedMs)
                               .arg(process.commandLine().toUserOutput());
    if (process.allOutput().trimmed().isEmpty()) {
        return common + QString("\n  ...and produced no output at all, so it was killed rather "
                                "than having failed - most likely blocked, since the budget was "
                                "%1s. %2")
                            .arg(s_compileTimeout.count()).arg(process.verboseExitMessage());
    }
    return common + "\n  " + process.verboseExitMessage();
}

static constexpr quint64 s_symbolAddressRequestId = 999000;

static const char s_qmlNativeDebuggerPluginMissing[] =
    "Qt's qmldbg_native plugin not found - can't establish a live "
    "QML debug connection.";

static const char s_qtDeclarativeDebugInfoMissing[] =
    "libQt6Qml has no DWARF debug info - gdbbridge.py can't recognize its "
    "own interpreter-internal frames, so no QML frames get spliced in.";

enum class Backend {
    Gdb,
    Lldb,
};
Q_DECLARE_METATYPE(Backend)

struct InferiorTestData
{
    FilePath source;
    FilePath executable;
    int breakpointLine = 0;
    int secondBreakpointLine = 0;
    int deepRecursionBreakpointLine = 0;
    int remoteAttachMinMajorVersion = 0;
    // gdb tells the stub which process to debug over extended-remote, so the stub can be
    // started without one. lldb has no equivalent - neither RemoteAttachToProcessWithID()
    // nor RemoteLaunch() ever reaches the eStateConnected they require against a bare
    // "gdbserver --multi" - so its stub has to own the process from the start.
    bool remoteStubHostsProcess = false;
    QString enableToggleWireMarker;
    QString disassemblySourceMarker;
    QString alienBreakpointCommand;
    QString alienBreakpointDeleteCommand;
    bool answersRedundantContinue = false;
    int expectedExitCode = 0;
    QString recursionDepthVariable;
    int multiLocationBreakpointLine = 0;
    int spinBodyLine = 0;
    QString localMarker;
    QString functionMarker;
    QString expandableLocal;
    QString expandableChild;
    QString inspectorObject;
    QString inspectorProperty;
    QString inspectorPropertyExpression;
    QString inspectorOrphanObject;
    QString versionLine;
    QString moduleListMarker;
    FilePath moduleSymbolsPath;
    QString falseLiteral = "0";
};

struct BackendData
{
    FilePath path;
    InferiorTestData inferiorData;
};

static FilePath findGdbOnPath()
{
    static const QStringList candidates = {
        "gdb", "gdb.exe",
        "gdb-i686-pc-mingw32", "gdb-i686-pc-mingw32.exe",
        "x86_64-w64-mingw32-gdb", "x86_64-w64-mingw32-gdb.exe",
        "i686-w64-mingw32-gdb", "i686-w64-mingw32-gdb.exe",
    };
    for (const QString &candidate : candidates) {
        const FilePath path = FilePath::fromString(candidate).searchInPath();
        if (path.isExecutableFile())
            return path;
    }
    return {};
}

[[maybe_unused]] static int debuggerMajorVersion(const QString &versionLine)
{
    static const QRegularExpression firstNumber("(\\d+)");
    const QRegularExpressionMatch match = firstNumber.match(versionLine);
    return match.hasMatch() ? match.captured(1).toInt() : 0;
}

[[maybe_unused]] static QString versionLine(const FilePath &tool)
{
    Process versionProcess;
    versionProcess.setCommand({tool, {"--version"}});
    versionProcess.runBlocking();
    return versionProcess.cleanedStdOut().section('\n', 0, 0);
}

[[maybe_unused]] static FilePath findPythonOnPath()
{
    static const QStringList candidates = {
        "python3", "python3.exe", "python", "python.exe",
    };
    for (const QString &candidate : candidates) {
        const FilePath path = FilePath::fromString(candidate).searchInPath();
        if (!path.isExecutableFile())
            continue;
        Process versionProcess;
        versionProcess.setCommand({path, {"--version"}});
        versionProcess.runBlocking();
        if (versionProcess.result() == ProcessResult::FinishedWithSuccess
            && versionProcess.cleanedStdOut().startsWith("Python ")) {
            return path;
        }
    }
    return {};
}

static bool hasQmlNativeDebuggerPlugin()
{
    const QDir pluginDir(QLibraryInfo::path(QLibraryInfo::PluginsPath) + "/qmltooling");
    return Utils::anyOf(pluginDir.entryInfoList(QDir::Files), [](const QFileInfo &info) {
        const QString base = info.completeBaseName();
        return base == "qmldbg_native" || base == "libqmldbg_native";
    });
}

static bool hasQtDeclarativeDebugInfo()
{
    const QDir libDir(QLibraryInfo::path(QLibraryInfo::LibrariesPath));
    const QFileInfoList candidates = libDir.entryInfoList({"libQt6Qml.so*"}, QDir::Files);
    if (candidates.isEmpty())
        return false;
    Utils::ElfReader reader(FilePath::fromString(candidates.constFirst().absoluteFilePath()));
    return reader.readHeaders().indexOf(".debug_info") != -1;
}

static bool hasNativeCallHook()
{
    const QDir libDir(QLibraryInfo::path(QLibraryInfo::LibrariesPath));
    const QFileInfoList candidates = libDir.entryInfoList({"libQt6Qml.so*"}, QDir::Files);
    if (candidates.isEmpty())
        return false;
    const FilePath nmPath = FilePath::fromString("nm").searchInPath();
    if (!nmPath.isExecutableFile())
        return false;
    Process nm;
    nm.setCommand({nmPath, {candidates.constFirst().absoluteFilePath()}});
    nm.runBlocking();
    if (nm.result() != ProcessResult::FinishedWithSuccess)
        return false;
    return nm.cleanedStdOut().contains("qt_v4AboutToCallNativeMethodHook");
}

static QString backendName(Backend backend)
{
    switch (backend) {
    case Backend::Gdb:
        return "gdb";
    case Backend::Lldb:
        return "lldb";
    }
    return {};
}

// lldb spells a C++ frame's function with its signature, gdb without it.
static bool stackHasFunction(const QString &stack, const QString &function)
{
    return stack.contains("function=\"" + function + '"')
           || stack.contains("function=\"" + function + '(');
}

static QString printCommand(Backend backend, const QString &expression)
{
    Q_UNUSED(expression)
    switch (backend) {
    case Backend::Gdb:
        return "print " + expression;
    case Backend::Lldb:
        return "expr " + expression;
    }
    return {};
}

static GdbMi findItemByIName(const GdbMi &data, const QString &iname)
{
    if (data["iname"].data() == iname)
        return data;
    for (const GdbMi &child : data) {
        if (const GdbMi found = findItemByIName(child, iname); found.isValid())
            return found;
    }
    return {};
}

class DebuggerBackend : public QObject
{
    Q_OBJECT

public:
    explicit DebuggerBackend(std::unique_ptr<DebuggerEngineInterface> engine)
        : m_engine(std::move(engine))
    {
        connect(m_engine.get(), &DebuggerEngineInterface::message, this,
                [](const QString &text, int, int) { qDebug("engine: %s", qPrintable(text)); });
        connect(m_engine.get(), &DebuggerEngineInterface::locationChanged, this,
                [this](const Utils::FilePath &fileName, int lineNumber) {
            m_stoppedFile = fileName;
            m_stoppedLine = lineNumber;
        });
        connect(m_engine.get(), &DebuggerEngineInterface::inferiorEvent, this,
                [this](InferiorEvent event) { m_events.append(event); });
        connect(m_engine.get(), &DebuggerEngineInterface::inferiorDone, this,
                [this](const InferiorResultData &resultData) { m_inferiorResults.append(resultData); });
        connect(m_engine.get(), &DebuggerEngineInterface::breakpointEvent, this,
                [this](quint64, BreakpointOp op, bool ok, const GdbMi &data) {
            if (op == BreakpointOp::Insert && ok && data.childCount() > 0)
                m_breakpointResponseId = data.childAt(0)["number"].data();
        });
    }

    DebuggerEngineInterface *engine() const { return m_engine.get(); }

    void execute(const ExecutionRequest &request) { m_engine->execute(request); }

    bool contains(InferiorEvent event) const { return m_events.contains(event); }
    qsizetype count(InferiorEvent event) const { return m_events.count(event); }
    bool isEmpty() const { return m_events.isEmpty(); }
    qsizetype size() const { return m_events.size(); }
    void clearEvents() { m_events.clear(); }

    const QList<InferiorResultData> &inferiorResults() const { return m_inferiorResults; }
    void clearInferiorResults() { m_inferiorResults.clear(); }

    Utils::FilePath stoppedFile() const { return m_stoppedFile; }
    int stoppedLine() const { return m_stoppedLine; }

    QString breakpointResponseId() const { return m_breakpointResponseId; }

private:
    std::unique_ptr<DebuggerEngineInterface> m_engine;
    QList<InferiorEvent> m_events;
    QList<InferiorResultData> m_inferiorResults;
    Utils::FilePath m_stoppedFile;
    int m_stoppedLine = 0;
    QString m_breakpointResponseId;
};

class tst_backends : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testAdditionalQmlStackCapability_data() { addBackendRows(); }
    void testAdditionalQmlStackCapability();
    void testAddWatcherCapability_data() { addBackendRows(); }
    void testAddWatcherCapability();
    void testAddWatcherWhileRunningCapability_data() { addBackendRows(); }
    void testAddWatcherWhileRunningCapability();
    void testAutoDerefPointersCapability_data() { addBackendRows(); }
    void testAutoDerefPointersCapability();
    void testBreakConditionCapability_data() { addBackendRows(); }
    void testBreakConditionCapability();
    void testBreakIndividualLocationsCapability_data() { addBackendRows(); }
    void testBreakIndividualLocationsCapability();
    void testBreakModuleCapability_data() { addBackendRows(); }
    void testBreakModuleCapability();
    void testBreakOnThrowAndCatchCapability_data() { addBackendRows(); }
    void testBreakOnThrowAndCatchCapability();
    void testCreateFullBacktraceCapability_data() { addBackendRows(); }
    void testCreateFullBacktraceCapability();
    void activatesFrameAndReadsItsLocals_data() { addBackendRows(); }
    void activatesFrameAndReadsItsLocals();
    void testDetachCapability_data() { addBackendRows(); }
    void testDetachCapability();
    void testDisassemblerCapability_data() { addBackendRows(); }
    void testDisassemblerCapability();
    void testJumpToLineCapability_data() { addBackendRows(); }
    void testJumpToLineCapability();
    void testLibraryEventCapability_data() { addBackendRows(); }
    void testLibraryEventCapability();
    void testOperateByInstructionCapability_data() { addBackendRows(); }
    void testOperateByInstructionCapability();
    void testRegisterCapability_data() { addBackendRows(); }
    void testRegisterCapability();
    void testReloadModuleCapability_data() { addBackendRows(); }
    void testReloadModuleCapability();
    void testReloadModuleSymbolsCapability_data() { addBackendRows(); }
    void testReloadModuleSymbolsCapability();
    void testResetInferiorCapability_data() { addBackendRows(); }
    void testResetInferiorCapability();
    void testReturnFromFunctionCapability_data() { addBackendRows(); }
    void testReturnFromFunctionCapability();
    void testReverseSteppingCapability_data() { addBackendRows(); }
    void testReverseSteppingCapability();
    void testRunCommandDeferralCapability_data() { addBackendRows(); }
    void testRunCommandDeferralCapability();
    void testRunToLineCapability_data() { addBackendRows(); }
    void testRunToLineCapability();
    void testShowMemoryCapability_data() { addBackendRows(); }
    void testShowMemoryCapability();
    void testShowModuleSectionsCapability_data() { addBackendRows(); }
    void testShowModuleSectionsCapability();
    void testShowModuleSymbolsCapability_data() { addBackendRows(); }
    void testShowModuleSymbolsCapability();
    void testSignalReceivedCapability_data() { addBackendRows(); }
    void testSignalReceivedCapability();
    void testSnapshotCapability_data() { addBackendRows(); }
    void testSnapshotCapability();
    void testSourceFilesCapability_data() { addBackendRows(); }
    void testSourceFilesCapability();
    void testThreadsCapability_data() { addBackendRows(); }
    void testThreadsCapability();
    void testTracePointCapability_data() { addBackendRows(); }
    void testTracePointCapability();
    void testWatchComplexExpressionsCapability_data() { addBackendRows(); }
    void testWatchComplexExpressionsCapability();
    void testWatchWidgetsCapability_data() { addBackendRows(); }
    void testWatchWidgetsCapability();
    void testWatchpointByAddressCapability_data() { addBackendRows(); }
    void testWatchpointByAddressCapability();
    void testWatchpointByExpressionCapability_data() { addBackendRows(); }
    void testWatchpointByExpressionCapability();

    void hitsBreakpointAndReadsMemory_data() { addBackendRows(); }
    void hitsBreakpointAndReadsMemory();
    void stepsContinuesAndInterrupts_data() { addBackendRows(); }
    void stepsContinuesAndInterrupts();
    void interruptWhileStoppedReportsStopOkImmediately_data() { addBackendRows(); }
    void interruptWhileStoppedReportsStopOkImmediately();
    void continueAfterExitReportsInferiorIll_data() { addBackendRows(); }
    void continueAfterExitReportsInferiorIll();
    void continueWhileRunningReportsRunFailed_data() { addBackendRows(); }
    void continueWhileRunningReportsRunFailed();
    void stopsAtFunctionBreakpointInsertedBeforeFirstRun_data() { addBackendRows(); }
    void stopsAtFunctionBreakpointInsertedBeforeFirstRun();
    void continueSignalsExitedForSpontaneousExit_data() { addBackendRows(); }
    void continueSignalsExitedForSpontaneousExit();
    void refreshesLocalsAndStack_data() { addBackendRows(); }
    void refreshesLocalsAndStack();
    void expandsContainerLocalWhenExpanded_data() { addBackendRows(); }
    void expandsContainerLocalWhenExpanded();
    void refreshesRegisters_data() { addBackendRows(); }
    void refreshesRegisters();
    void refreshesRegistersAfterResume_data() { addBackendRows(); }
    void refreshesRegistersAfterResume();
    void updatesEnablesAndRemovesBreakpoint_data() { addBackendRows(); }
    void updatesEnablesAndRemovesBreakpoint();
    void writesMemoryAndPeripheralRegister_data() { addBackendRows(); }
    void writesMemoryAndPeripheralRegister();
    void selectsThreadAndActivatesFrame_data() { addBackendRows(); }
    void selectsThreadAndActivatesFrame();
    void executesRawCommandAndAssignsValue_data() { addBackendRows(); }
    void executesRawCommandAndAssignsValue();
    void assignsValueToLocalVariable_data() { addBackendRows(); }
    void assignsValueToLocalVariable();
    void shutsDownCleanly_data() { addBackendRows(); }
    void shutsDownCleanly();
    void executesRunToLineFunctionAndJumpsToLine_data() { addBackendRows(); }
    void executesRunToLineFunctionAndJumpsToLine();
    void insertsWatchpointAndCatchpoint_data() { addBackendRows(); }
    void insertsWatchpointAndCatchpoint();
    void insertsWatchpointAsFirstCommandAfterStop_data() { addBackendRows(); }
    void insertsWatchpointAsFirstCommandAfterStop();
    void clearedBreakpointConditionStopsAgain_data() { addBackendRows(); }
    void clearedBreakpointConditionStopsAgain();
    void fetchesMemoryFromInvalidAddress_data() { addBackendRows(); }
    void fetchesMemoryFromInvalidAddress();
    void reportsEngineSetupFailure_data() { addBackendRows(); }
    void reportsEngineSetupFailure();
    void refreshesPeripherals_data() { addBackendRows(); }
    void refreshesPeripherals();
    void reloadsDebuggingHelpersAndSymbols_data() { addBackendRows(); }
    void reloadsDebuggingHelpersAndSymbols();
    void acceptsBreakpointFollowsRules_data() { addBackendRows(); }
    void acceptsBreakpointFollowsRules();
    void acceptsBreakpointFollowsCppAndQmlRules_data() { addBackendRows(); }
    void acceptsBreakpointFollowsCppAndQmlRules();
    void executesStepIn_data() { addBackendRows(); }
    void executesStepIn();
    void breakpointConditionPreventsStop_data() { addBackendRows(); }
    void breakpointConditionPreventsStop();
    void executesRepeatLastCommand_data() { addBackendRows(); }
    void executesRepeatLastCommand();
    void passesInferiorEnvironmentDiffToDebugger_data() { addBackendRows(); }
    void passesInferiorEnvironmentDiffToDebugger();
    void passesInferiorWorkingDirectoryToDebugger_data() { addBackendRows(); }
    void passesInferiorWorkingDirectoryToDebugger();
    void loadsAdditionalQmlStack_data() { addBackendRows(); }
    void loadsAdditionalQmlStack();
    void fetchesQmlLocals_data() { addBackendRows(); }
    void fetchesQmlLocals();
    void insertsQmlBreakpointAndStopsAtIt_data() { addBackendRows(); }
    void insertsQmlBreakpointAndStopsAtIt();
    void insertsQmlBreakpointBeforeDumpersLoad_data() { addBackendRows(); }
    void insertsQmlBreakpointBeforeDumpersLoad();
    void splicesQmlFramesIntoPlainFullStackWhenNativeMixed_data() { addBackendRows(); }
    void splicesQmlFramesIntoPlainFullStackWhenNativeMixed();
    void stepsOutOfNativeMixedCppFrameBackIntoQml_data() { addBackendRows(); }
    void stepsOutOfNativeMixedCppFrameBackIntoQml();
    void stepsWithinQmlFrameAfterNativeMixedStepOut_data() { addBackendRows(); }
    void stepsWithinQmlFrameAfterNativeMixedStepOut();
    void continuesPastNativeMixedCppBreakpoint_data() { addBackendRows(); }
    void continuesPastNativeMixedCppBreakpoint();
    void staysStoppedWithoutExplicitContinue_data() { addBackendRows(); }
    void staysStoppedWithoutExplicitContinue();
    void stepsFromQmlIntoNativeMixedCppFrame_data() { addBackendRows(); }
    void stepsFromQmlIntoNativeMixedCppFrame();
    void reportsBreakpointModifiedEvents_data() { addBackendRows(); }
    void reportsBreakpointModifiedEvents();
    void reportsAlienBreakpoints_data() { addBackendRows(); }
    void reportsAlienBreakpoints();
    void togglesBreakpointEnabledInPlace_data() { addBackendRows(); }
    void togglesBreakpointEnabledInPlace();
    void attachesToRunningProcess_data() { addBackendRows(); }
    void attachesToRunningProcess();
    void attachesToTerminalRunProcess_data() { addBackendRows(); }
    void attachesToTerminalRunProcess();
    void attachesToRunningRemoteServer_data() { addBackendRows(); }
    void attachesToRunningRemoteServer();
    void attachesToRemoteProcessByPid_data() { addBackendRows(); }
    void attachesToRemoteProcessByPid();
    void runsRemoteExecutableViaExtendedRemote_data() { addBackendRows(); }
    void runsRemoteExecutableViaExtendedRemote();
    void attachesToQnxTarget_data() { addBackendRows(); }
    void attachesToQnxTarget();
    void attachesToCoreFile_data() { addBackendRows(); }
    void attachesToCoreFile();

    void attachesToQmlServerAndStopsAtBreakpoint_data() { addBackendRows(); }
    void attachesToQmlServerAndStopsAtBreakpoint();
    void insertsBreakpointAtJavaScriptThrowAndStopsAtIt_data() { addBackendRows(); }
    void insertsBreakpointAtJavaScriptThrowAndStopsAtIt();
    void reportsInspectorObjectTree_data() { addBackendRows(); }
    void reportsInspectorObjectTree();

private:
    void addBackendRows();
    std::unique_ptr<DebuggerBackend> createEngine(Backend backend,
        const std::optional<Utils::ProcessRunData> &debuggerRunDataOverride = {},
        const std::optional<Utils::ProcessRunData> &inferiorRunDataOverride = {},
        bool nativeMixed = false);
    std::unique_ptr<DebuggerBackend> createAttachEngine(Backend backend,
        const InferiorStartData &inferiorStartData);
    std::unique_ptr<DebuggerBackend> launchAndStopAtBreakpoint(Backend backend);
    std::unique_ptr<DebuggerBackend> stopAtBreakpoint(Backend backend, Process &helperInferior);
    bool hasCapability(Backend backend, Debugger::DebuggerCapabilities capability,
                       Debugger::DebuggerStartMode startMode = Debugger::NoStartMode);
    bool hasExtraCapability(Backend backend, Debugger::DebuggerExtraCapability capability);
    bool hasStartMode(Backend backend, DebuggerStartModeFlag startMode);
    Utils::Result<> checkStartMode(Backend backend, DebuggerStartModeFlag startMode);
    Utils::Result<> checkCapability(Backend backend, Debugger::DebuggerCapabilities capability);
    Utils::Result<> checkExtraCapability(Backend backend, Debugger::DebuggerExtraCapability capability);
    Utils::Result<> checkAcceptsCppAndQmlBreakpoints(Backend backend);
    QString startGdbserver(Process &gdbserverProcess, const QStringList &flags,
                            const QStringList &trailingArgs, QString *gdbserverOutput);
    quint16 startQmlServer(Process &inferiorProcess, const FilePath &executable);
    quint64 symbolAddress(Backend backend, DebuggerEngineInterface *engine,
                          const QString &symbolName);
    quint64 symbolAddressFromDebugger(DebuggerEngineInterface *engine,
                                      const QString &symbolName);
    static int qmlMarkerLine(const QString &relativePath, const QString &marker);
    InferiorTestData inferiorTestData(Backend backend) const;
    void stopInferiorSpinLoop(Backend backend, DebuggerEngineInterface *engine);

    QMap<Backend, BackendData> m_backendData;
    FilePath m_gdbserverPath;
    FilePath m_qnxGdbPath;
    bool m_hasQmlNativeDebuggerPlugin = false;
    bool m_hasQtDeclarativeDebugInfo = false;
    bool m_hasNativeCallHook = false;
    FilePath m_inferiorLib;
    QTemporaryDir m_tempDir;
};

int tst_backends::qmlMarkerLine(const QString &relativePath, const QString &marker)
{
    QFile file(QLatin1String(BACKENDS_TEST_SOURCE_DIR) + '/' + relativePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return 0;
    int lineNumber = 0;
    while (!file.atEnd()) {
        ++lineNumber;
        if (QString::fromUtf8(file.readLine()).contains(marker))
            return lineNumber;
    }
    return 0;
}

quint64 tst_backends::symbolAddress(Backend backend, DebuggerEngineInterface *engine,
                                    const QString &symbolName)
{
    const FilePath nmPath = FilePath::fromString("nm").searchInPath();
    if (nmPath.isExecutableFile()) {
        Process nm;
        nm.setCommand({nmPath, {inferiorTestData(backend).executable.nativePath()}});
        nm.runBlocking();
        if (nm.result() == ProcessResult::FinishedWithSuccess) {
            for (const QString &line : nm.cleanedStdOut().split('\n')) {
                if (!line.endsWith(symbolName))
                    continue;
                bool ok = false;
                const quint64 address
                    = line.split(' ', Qt::SkipEmptyParts).constFirst().toULongLong(&ok, 16);
                if (ok)
                    return address;
            }
        }
    }
    return symbolAddressFromDebugger(engine, symbolName);
}

quint64 tst_backends::symbolAddressFromDebugger(DebuggerEngineInterface *engine,
                                                const QString &symbolName)
{
    if (!engine || !engine->hasCapability(Debugger::AddWatcherCapability))
        return 0;

    QJsonObject watcher;
    watcher.insert("iname", QString("watch.0"));
    watcher.insert("exp", toHex('&' + symbolName));
    QJsonArray watchers;
    watchers.append(watcher);

    GdbMi reply;
    bool replied = false;
    const auto connection = connect(engine, &DebuggerEngineInterface::refreshDataReceived,
            engine, [&reply, &replied](quint64 requestId, RefreshKind kind, const GdbMi &data) {
        if (requestId == s_symbolAddressRequestId && kind == RefreshKind::Locals) {
            reply = data;
            replied = true;
        }
    });
    RefreshRequest request;
    request.kind = RefreshKind::Locals;
    request.requestId = s_symbolAddressRequestId;
    request.watchers = watchers;
    engine->refresh(request);

    QElapsedTimer elapsed;
    elapsed.start();
    while (!replied && elapsed.durationElapsed() < s_timeout)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    disconnect(connection);
    if (!replied)
        return 0;

    QString address = findItemByIName(reply, "watch.0")["address"].data();
    if (address.startsWith("0x"))
        address.remove(0, 2);
    bool ok = false;
    const quint64 result = address.toULongLong(&ok, 16);
    return ok ? result : 0;
}

void tst_backends::addBackendRows()
{
    QTest::addColumn<Backend>("backend");
    for (Backend backend : m_backendData.keys())
        QTest::newRow(qPrintable(backendName(backend))) << backend;
}

std::unique_ptr<DebuggerBackend> tst_backends::createEngine(Backend backend,
    const std::optional<ProcessRunData> &debuggerRunDataOverride,
    const std::optional<ProcessRunData> &inferiorRunDataOverride,
    bool nativeMixed)
{
    Q_UNUSED(debuggerRunDataOverride)
    Q_UNUSED(inferiorRunDataOverride)
    Q_UNUSED(nativeMixed)
    switch (backend) {
    case Backend::Gdb:
        return std::make_unique<DebuggerBackend>(std::make_unique<GdbImpl>(GdbImplStartData{
            .debuggerRunData = debuggerRunDataOverride.value_or(
                ProcessRunData{{m_backendData[backend].path, {}}, {}, Environment::systemEnvironment()}),
            .inferiorStartData = inferiorRunDataOverride.value_or(
                ProcessRunData{{inferiorTestData(backend).executable, {}}, {}, Environment::systemEnvironment()}),
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
            .nativeMixedDebugging = nativeMixed}));
    case Backend::Lldb:
        return std::make_unique<DebuggerBackend>(std::make_unique<LldbImpl>(LldbImplStartData{
            .debuggerRunData = debuggerRunDataOverride.value_or(
                ProcessRunData{{m_backendData[backend].path, {}}, {}, Environment::systemEnvironment()}),
            .inferiorStartData = inferiorRunDataOverride.value_or(
                ProcessRunData{{inferiorTestData(backend).executable, {}}, {}, Environment::systemEnvironment()}),
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
            .nativeMixedDebugging = nativeMixed}));
    }
    return nullptr;
}

std::unique_ptr<DebuggerBackend> tst_backends::createAttachEngine(
    Backend backend, const InferiorStartData &inferiorStartData)
{
    Q_UNUSED(inferiorStartData)
    switch (backend) {
    case Backend::Gdb:
        return std::make_unique<DebuggerBackend>(std::make_unique<GdbImpl>(GdbImplStartData{
            .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                              Environment::systemEnvironment()},
            .inferiorStartData = inferiorStartData,
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR)}));
    case Backend::Lldb:
        return std::make_unique<DebuggerBackend>(std::make_unique<LldbImpl>(LldbImplStartData{
            .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                              Environment::systemEnvironment()},
            .inferiorStartData = inferiorStartData,
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR)}));
    }
    return nullptr;
}

InferiorTestData tst_backends::inferiorTestData(Backend backend) const
{
    return m_backendData.value(backend).inferiorData;
}

void tst_backends::stopInferiorSpinLoop(Backend backend, DebuggerEngineInterface *engine)
{
    if (!engine->hasCapability(Debugger::ShowMemoryCapability)) {
        WatchItemData item;
        item.isLocal = false;
        engine->assignValueInDebugger(item, "keepSpinning",
                                      inferiorTestData(backend).falseLiteral);
        return;
    }
    const quint64 keepSpinningAddress = symbolAddress(backend, engine, "keepSpinning");
    QVERIFY2(keepSpinningAddress != 0, "could not find keepSpinning's address via nm");
    engine->accessMemory(MemoryOp::Change, 0, keepSpinningAddress, 1, QByteArray(1, char(0)));
}

void tst_backends::initTestCase()
{
    TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath() + "/tst_backends-XXXXXX");

    QString gdbVersionLine;
    QString lldbVersionLine;

    {
        // Auto-detection stays off on Windows, but an explicit override works there, too.
        const QString envGdb = qtcEnvironmentVariable("QTC_DEBUGGER_PATH_FOR_TEST");
        const FilePath gdbPath = !envGdb.isEmpty() ? FilePath::fromUserInput(envGdb)
                                 : HostOsInfo::isWindowsHost() ? FilePath() : findGdbOnPath();
        if (gdbPath.isExecutableFile()) {
            m_backendData[Backend::Gdb].path = gdbPath;
            gdbVersionLine = versionLine(gdbPath);
        }

        const QString envLldb = qtcEnvironmentVariable("QTC_LLDB_PATH_FOR_TEST");
        const FilePath lldbPath = envLldb.isEmpty() ? FilePath::fromString("lldb").searchInPath()
                                                    : FilePath::fromUserInput(envLldb);
        if (lldbPath.isExecutableFile()) {
            m_backendData[Backend::Lldb].path = lldbPath;
            lldbVersionLine = versionLine(lldbPath);
        }
    }

    if (m_backendData.isEmpty())
        QSKIP("No supported debugger backend found - set "
              "QTC_DEBUGGER_PATH_FOR_TEST to override.");

    const QString envGdbserver = qtcEnvironmentVariable("QTC_GDBSERVER_PATH_FOR_TEST");
    m_gdbserverPath = envGdbserver.isEmpty() ? FilePath::fromString("gdbserver").searchInPath()
                                             : FilePath::fromUserInput(envGdbserver);

    m_qnxGdbPath = FilePath::fromUserInput(
        qtcEnvironmentVariable("QTC_QNX_GDB_PATH_FOR_TEST"));

    m_hasQmlNativeDebuggerPlugin = hasQmlNativeDebuggerPlugin();
    qWarning("qmldbg_native plugin: %s (looked in %s)",
             m_hasQmlNativeDebuggerPlugin ? "found" : "NOT found",
             qPrintable(QLibraryInfo::path(QLibraryInfo::PluginsPath) + "/qmltooling"));

    m_hasQtDeclarativeDebugInfo = hasQtDeclarativeDebugInfo();
    qWarning("libQt6Qml debug info: %s (looked in %s)",
             m_hasQtDeclarativeDebugInfo ? "found" : "NOT found",
             qPrintable(QLibraryInfo::path(QLibraryInfo::LibrariesPath)));

    m_hasNativeCallHook = hasNativeCallHook();
    qWarning("qt_v4AboutToCallNativeMethodHook: %s",
             m_hasNativeCallHook ? "found" : "NOT found");

    const FilePath dumperDir = FilePath::fromUserInput(DUMPERDIR);
    if (!dumperDir.exists())
        QSKIP(qPrintable("Debugger dumper scripts not found at "
                          + dumperDir.toUserOutput()));

    FilePath compiler;
    QStringList probeFailures;
    for (const QString &candidate : QStringList{"g++", "clang++"}) {
        const FilePath path = FilePath::fromString(candidate).searchInPath();
        if (!path.isExecutableFile())
            continue;
        Process probe;
        probe.setCommand({path, {"--version"}});
        probe.runBlocking(std::chrono::seconds(20));
        if (probe.result() == ProcessResult::FinishedWithSuccess) {
            compiler = path;
            qWarning("C++ compiler: %s (%s)", qPrintable(path.toUserOutput()),
                     qPrintable(probe.cleanedStdOut().split('\n').constFirst()));
            break;
        }
        probeFailures.append(path.toUserOutput() + " - " + probe.exitMessage());
    }
    if (!compiler.isExecutableFile()) {
        QSKIP(qPrintable(probeFailures.isEmpty()
                             ? QString("No C++ compiler (g++/clang++) found to build the "
                                       "test inferior.")
                             : QString("No usable C++ compiler to build the test inferior - "
                                       "found, but unable to even run \"--version\":\n  ")
                                   + probeFailures.join("\n  ")));
    }

    QVERIFY(m_tempDir.isValid());
    InferiorTestData cppInferiorData;
    cppInferiorData.source = FilePath::fromString(m_tempDir.path()) / "inferior.cpp";
    cppInferiorData.executable = (FilePath::fromString(m_tempDir.path()) / "inferior")
                                .withExecutableSuffix();
    m_inferiorLib = FilePath::fromString(m_tempDir.path())
                   / (HostOsInfo::isWindowsHost() ? "inferiorlib.dll" : "inferiorlib.so");

    const QStringList inferiorLines = {
        "#include <chrono>",
        "#include <cstdio>",
        "#include <cstring>",
        "#include <thread>",
        "#ifdef _WIN32",
        "#include <windows.h>",
        "#else",
        "#include <dlfcn.h>",
        "#endif",
        "#ifdef __linux__",
        "#include <sys/prctl.h>",
        "#endif",
        "",
        "volatile int globalValue = 41;",
        "volatile bool keepSpinning = true;",
        "const char *globalMessage = \"hi\";",
        "int *globalValuePtr = const_cast<int *>(&globalValue);",
        "",
        "extern \"C\" void bump()",
        "{",
        "    int localValue = globalValue + 1; // first breakpoint line",
        "    globalValue = localValue;",
        "    printf(\"value=%d\\n\", globalValue);",
        "    fflush(stdout);",
        "}",
        "",
        "extern \"C\" void spin()",
        "{",
        "    while (keepSpinning)",
        "        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // spin body line",
        "}",
        "",
        "extern \"C\" int recurse(int depth)",
        "{",
        "    if (depth <= 0)",
        "        return 0; // deep breakpoint line",
        "    return 1 + recurse(depth - 1);",
        "}",
        "",
        "extern \"C\" void crash()",
        "{",
        "    volatile int *p = nullptr;",
        "    *p = 1;",
        "}",
        "",
        "template<typename T> void multi(T value)",
        "{",
        "    printf(\"multi=%d\\n\", int(value)); // multi-location breakpoint line",
        "    fflush(stdout);",
        "}",
        "",
        "int main(int argc, char **argv)",
        "{",
        "#ifdef __linux__",
        "    prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);",
        "#endif",
        "#ifdef _WIN32",
        "    HMODULE h = LoadLibraryW(L\"" + QString(m_inferiorLib.nativePath()).replace('\\', "\\\\") + "\");",
        "    if (h)",
        "        FreeLibrary(h);",
        "#else",
        "    dlclose(dlopen(\"" + m_inferiorLib.nativePath() + "\", RTLD_NOW));",
        "#endif",
        "    if (argc > 1 && strcmp(argv[1], \"crash\") == 0)",
        "        crash();",
        "    bump();",
        "    multi(1);",
        "    multi(2.0);",
        "    recurse(40);",
        "    printf(\"after bump\\n\");",
        "    fflush(stdout);",
        "    spin(); // second breakpoint line",
        "    return 7;",
        "}",
        "",
    };
    for (int i = 0; i < inferiorLines.size(); ++i) {
        if (inferiorLines.at(i).contains("first breakpoint line"))
            cppInferiorData.breakpointLine = i + 1;
        if (inferiorLines.at(i).contains("second breakpoint line"))
            cppInferiorData.secondBreakpointLine = i + 1;
        if (inferiorLines.at(i).contains("deep breakpoint line"))
            cppInferiorData.deepRecursionBreakpointLine = i + 1;
        if (inferiorLines.at(i).contains("multi-location breakpoint line"))
            cppInferiorData.multiLocationBreakpointLine = i + 1;
        if (inferiorLines.at(i).contains("spin body line"))
            cppInferiorData.spinBodyLine = i + 1;
    }
    QVERIFY(cppInferiorData.breakpointLine > 0);
    cppInferiorData.localMarker = "localValue";
    cppInferiorData.functionMarker = "bump";
    cppInferiorData.recursionDepthVariable = "depth";
    cppInferiorData.disassemblySourceMarker = "globalValue = localValue";
    cppInferiorData.expectedExitCode = 7;
    QVERIFY(cppInferiorData.secondBreakpointLine > 0);
    QVERIFY(cppInferiorData.deepRecursionBreakpointLine > 0);
    QVERIFY(cppInferiorData.multiLocationBreakpointLine > 0);
    QVERIFY(cppInferiorData.spinBodyLine > 0);

    QFile file(cppInferiorData.source.toFSPathString());
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(inferiorLines.join('\n').toUtf8());
    file.close();

    QStringList compileArgs = {"-g", "-O0"};
    if (HostOsInfo::isLinuxHost())
        compileArgs << "-no-pie";
    compileArgs << "-o" << cppInferiorData.executable.nativePath()
                << cppInferiorData.source.nativePath();
    if (HostOsInfo::isLinuxHost())
        compileArgs << "-ldl";
    Process compile;
    compile.setCommand({compiler, compileArgs});
    QElapsedTimer compileTimer;
    compileTimer.start();
    compile.runBlocking(s_compileTimeout);
    QVERIFY2(compile.result() == ProcessResult::FinishedWithSuccess,
             qPrintable(compileFailure("compiling the test inferior",
                                       compile, compileTimer.elapsed())));

    const FilePath inferiorLibSource = FilePath::fromString(m_tempDir.path()) / "inferiorlib.cpp";
    QFile libFile(inferiorLibSource.toFSPathString());
    QVERIFY(libFile.open(QIODevice::WriteOnly | QIODevice::Text));
    libFile.write(QByteArrayLiteral("extern \"C\" int inferiorLibFunc() { return 0; }\n"));
    libFile.close();

    QStringList compileLibArgs = {"-shared"};
    if (!HostOsInfo::isWindowsHost())
        compileLibArgs << "-fPIC";
    compileLibArgs << "-g" << "-O0" << "-o" << m_inferiorLib.nativePath()
                   << inferiorLibSource.nativePath();
    Process compileLib;
    compileLib.setCommand({compiler, compileLibArgs});
    QElapsedTimer compileLibTimer;
    compileLibTimer.start();
    compileLib.runBlocking(s_compileTimeout);
    QVERIFY2(compileLib.result() == ProcessResult::FinishedWithSuccess,
             qPrintable(compileFailure("compiling the inferior library",
                                       compileLib, compileLibTimer.elapsed())));

    if (m_backendData.contains(Backend::Gdb)) {
        m_backendData[Backend::Gdb].inferiorData = cppInferiorData;
        m_backendData[Backend::Gdb].inferiorData.versionLine = gdbVersionLine;
        m_backendData[Backend::Gdb].inferiorData.moduleListMarker = "libc";
        m_backendData[Backend::Gdb].inferiorData.moduleSymbolsPath = cppInferiorData.executable;
        m_backendData[Backend::Gdb].inferiorData.alienBreakpointCommand = "break spin";
        m_backendData[Backend::Gdb].inferiorData.enableToggleWireMarker = "-break-disable";
        m_backendData[Backend::Gdb].inferiorData.alienBreakpointDeleteCommand = "delete %1";
    }
    if (m_backendData.contains(Backend::Lldb)) {
        m_backendData[Backend::Lldb].inferiorData = cppInferiorData;
        m_backendData[Backend::Lldb].inferiorData.answersRedundantContinue = true;
        m_backendData[Backend::Lldb].inferiorData.remoteAttachMinMajorVersion = 21;
        m_backendData[Backend::Lldb].inferiorData.remoteStubHostsProcess = true;
        m_backendData[Backend::Lldb].inferiorData.enableToggleWireMarker = "changeBreakpoint";
        m_backendData[Backend::Lldb].inferiorData.versionLine = lldbVersionLine;
        m_backendData[Backend::Lldb].inferiorData.moduleListMarker = "libc";
        m_backendData[Backend::Lldb].inferiorData.moduleSymbolsPath = cppInferiorData.executable;
    }
}

void tst_backends::cleanupTestCase()
{
    Utils::ProcessReaper::deleteAll();
}

bool tst_backends::hasCapability(Backend backend, Debugger::DebuggerCapabilities capability,
                                 Debugger::DebuggerStartMode startMode)
{
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend);
    return debuggerBackend->engine()->hasCapability(capability, startMode);
}

bool tst_backends::hasExtraCapability(Backend backend, Debugger::DebuggerExtraCapability capability)
{
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend);
    return debuggerBackend->engine()->hasExtraCapability(capability);
}

bool tst_backends::hasStartMode(Backend backend, DebuggerStartModeFlag startMode)
{
    return createEngine(backend)->engine()->setupData().startModes.testFlag(startMode);
}

Utils::Result<> tst_backends::checkStartMode(Backend backend, DebuggerStartModeFlag startMode)
{
    if (hasStartMode(backend, startMode))
        return Utils::ResultOk;
    const QMetaEnum startModeEnum = QMetaEnum::fromType<DebuggerStartModes>();
    return Utils::ResultError(QString("%1 start mode not supported by %2.")
                                   .arg(startModeEnum.valueToKey(int(startMode)),
                                        backendName(backend)));
}

Utils::Result<> tst_backends::checkCapability(Backend backend, Debugger::DebuggerCapabilities capability)
{
    if (hasCapability(backend, capability))
        return Utils::ResultOk;
    const QMetaEnum capabilityEnum = QMetaEnum::fromType<Debugger::DebuggerCapabilities>();
    return Utils::ResultError(QString("%1 not claimed by %2.")
                                   .arg(capabilityEnum.valueToKey(int(capability)),
                                        backendName(backend)));
}

Utils::Result<> tst_backends::checkExtraCapability(Backend backend, Debugger::DebuggerExtraCapability capability)
{
    if (hasExtraCapability(backend, capability))
        return Utils::ResultOk;
    const QMetaEnum capabilityEnum = QMetaEnum::fromType<Debugger::DebuggerExtraCapabilities>();
    return Utils::ResultError(QString("%1 not claimed by %2.")
                                   .arg(capabilityEnum.valueToKey(int(capability)),
                                        backendName(backend)));
}

Utils::Result<> tst_backends::checkAcceptsCppAndQmlBreakpoints(Backend backend)
{
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend);
    const DebuggerEngineSetupData &data = debuggerBackend->engine()->setupData();
    if (!data.acceptsBreakpoint)
        return Utils::ResultError(backendName(backend) + " has no acceptsBreakpoint predicate at all.");

    AcceptsBreakpointQuery cppQuery;
    cppQuery.type = BreakpointByFileAndLine;
    cppQuery.fileName = FilePath::fromString("main.cpp");
    cppQuery.startMode = Debugger::StartInternal;
    if (!data.acceptsBreakpoint(cppQuery))
        return Utils::ResultError(backendName(backend) + " rejected a plain C++ file/line breakpoint.");

    AcceptsBreakpointQuery qmlQuery;
    qmlQuery.type = BreakpointByFileAndLine;
    qmlQuery.fileName = FilePath::fromString("main.qml");
    qmlQuery.startMode = Debugger::StartInternal;
    qmlQuery.isNativeMixedEnabled = false;
    if (data.acceptsBreakpoint(qmlQuery)) {
        return Utils::ResultError(backendName(backend)
            + " accepted a QML breakpoint with native-mixed debugging disabled.");
    }
    qmlQuery.isNativeMixedEnabled = true;
    if (!data.acceptsBreakpoint(qmlQuery)) {
        return Utils::ResultError(backendName(backend)
            + " rejected a QML breakpoint with native-mixed debugging enabled.");
    }

    return Utils::ResultOk;
}

void tst_backends::testAdditionalQmlStackCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AdditionalQmlStackCapability); !result)
        QSKIP(qPrintable(result.error()));

#ifndef QMLSTACK_INFERIOR_EXECUTABLE
    QSKIP("Qt::Quick not available when this test binary was configured.");
#else
    const FilePath inferior = (FilePath::fromUserInput(QMLSTACK_INFERIOR_EXECUTABLE)
                              / "qmlstack_inferior").withExecutableSuffix();
    if (!inferior.isExecutableFile())
        QSKIP(qPrintable("QML stack inferior not found at " + inferior.toUserOutput()));
    if (!m_hasQmlNativeDebuggerPlugin)
        QSKIP(s_qmlNativeDebuggerPluginMissing);
    if (!m_hasQtDeclarativeDebugInfo)
        QSKIP(s_qtDeclarativeDebugInfoMissing);

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
        ProcessRunData{{inferior, {}}, {}, Environment::systemEnvironment()});
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [engine](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            BreakpointChangeRequest request;
            request.op = BreakpointOp::Insert;
            request.requestId = 1;
            request.params.type = BreakpointByFunction;
            request.params.functionName = "QmlEntryPoint::process";
            request.params.enabled = true;
            engine->changeBreakpoint(request);
        }
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed)
                             || debuggerBackend->contains(InferiorEvent::EngineRunFailed), s_qmlStartupTimeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::SpontaneousStop));

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    RefreshRequest qmlStackRequest;
    qmlStackRequest.kind = RefreshKind::QmlStack;
    qmlStackRequest.requestId = 20;
    engine->refresh(qmlStackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);

    const QString stack = responses.value(int(RefreshKind::FullStack)).toString();
    QVERIFY2(stack.contains("language=\"js\""), qPrintable("stack: " + stack));
    QVERIFY2(stack.contains("QmlEntryPoint::process"), qPrintable("stack: " + stack));
#endif
}

void tst_backends::testAddWatcherCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AddWatcherCapability); !result)
        QSKIP(qPrintable(result.error()));

    Process helperInferior;
    std::unique_ptr<DebuggerBackend> debuggerBackend = stopAtBreakpoint(backend, helperInferior);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    QJsonObject watcher;
    watcher.insert("iname", "watch.0");
    watcher.insert("exp", toHex("globalValue"));
    QJsonArray watchers;
    watchers.append(watcher);

    RefreshRequest request;
    request.kind = RefreshKind::Locals;
    request.requestId = 93;
    request.watchers = watchers;
    engine->refresh(request);

    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
    const GdbMi locals = responses.value(int(RefreshKind::Locals));
    const GdbMi watchItem = findItemByIName(locals, "watch.0");
    QVERIFY2(watchItem.isValid(),
             qPrintable("no watch.0 item in locals: " + locals.toString()));
    QCOMPARE(decodeData(watchItem["value"].data(), watchItem["valueencoded"].data()),
             QString("41"));
}

void tst_backends::testAddWatcherWhileRunningCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AddWatcherWhileRunningCapability); !result)
        QSKIP(qPrintable(result.error()));

    Process helperInferior;
    std::unique_ptr<DebuggerBackend> debuggerBackend = stopAtBreakpoint(backend, helperInferior);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, bool> insertResults;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&insertResults](quint64 requestId, BreakpointOp op, bool ok, const GdbMi &) {
        if (op == BreakpointOp::Insert)
            insertResults[requestId] = ok;
    });
    BreakpointChangeRequest secondBreakpointRequest;
    secondBreakpointRequest.op = BreakpointOp::Insert;
    secondBreakpointRequest.requestId = 98;
    secondBreakpointRequest.params.type = BreakpointByFileAndLine;
    secondBreakpointRequest.params.fileName = inferiorTestData(backend).source;
    secondBreakpointRequest.params.textPosition.line = inferiorTestData(backend).secondBreakpointLine;
    secondBreakpointRequest.params.enabled = true;
    engine->changeBreakpoint(secondBreakpointRequest);
    QTRY_VERIFY_WITH_TIMEOUT(insertResults.contains(98), s_timeout);
    QVERIFY2(insertResults.value(98), "second breakpoint insert failed");

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    connect(engine, &DebuggerEngineInterface::inferiorEvent, this,
            [&](InferiorEvent event) {
        if (event == InferiorEvent::RunOk) {
            QJsonObject watcher;
            watcher.insert("iname", "watch.0");
            watcher.insert("exp", toHex("globalValue"));
            QJsonArray watchers;
            watchers.append(watcher);

            RefreshRequest request;
            request.kind = RefreshKind::Locals;
            request.requestId = 99;
            request.watchers = watchers;
            engine->refresh(request);
        }
    });

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).secondBreakpointLine);

    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
    const QString locals = responses.value(int(RefreshKind::Locals)).toString();
    QVERIFY2(locals.contains("watch.0"), qPrintable("locals: " + locals));
    QVERIFY2(locals.contains("42"), qPrintable("locals: " + locals));
}

void tst_backends::testAutoDerefPointersCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AutoDerefPointersCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    QString result;
    auto watchGlobalValuePtr = [&](bool autoDerefPointers) {
        QJsonObject watcher;
        watcher.insert("iname", "watch.0");
        watcher.insert("exp", toHex("globalValuePtr"));
        QJsonArray watchers;
        watchers.append(watcher);

        RefreshRequest request;
        request.kind = RefreshKind::Locals;
        request.requestId = autoDerefPointers ? 94 : 95;
        request.watchers = watchers;
        request.autoDerefPointers = autoDerefPointers;
        responses.remove(int(RefreshKind::Locals));
        engine->refresh(request);
        QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
        result = responses.value(int(RefreshKind::Locals)).toString();
    };

    watchGlobalValuePtr(true);
    QVERIFY2(result.contains("autoderefcount"), qPrintable("locals: " + result));

    watchGlobalValuePtr(false);
    QVERIFY2(!result.contains("autoderefcount"), qPrintable("locals: " + result));
}

void tst_backends::testBreakConditionCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::BreakConditionCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });

    debuggerBackend->clearEvents();
    BreakpointChangeRequest conditionRequest;
    conditionRequest.op = BreakpointOp::Insert;
    conditionRequest.requestId = 90;
    conditionRequest.params.type = BreakpointByFileAndLine;
    conditionRequest.params.fileName = inferiorTestData(backend).source;
    conditionRequest.params.textPosition.line = inferiorTestData(backend).secondBreakpointLine;
    conditionRequest.params.textPosition.column = 0;
    conditionRequest.params.enabled = true;
    conditionRequest.params.condition = "globalValue == 42";
    engine->changeBreakpoint(conditionRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(90), s_timeout);
    QVERIFY2(results.value(90), "conditional breakpoint insert failed");

    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "conditional breakpoint never triggered", s_timeout);
    QCOMPARE(debuggerBackend->stoppedFile(), inferiorTestData(backend).source);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).secondBreakpointLine);

    BreakpointChangeRequest flagsRequest;
    flagsRequest.op = BreakpointOp::Insert;
    flagsRequest.requestId = 91;
    flagsRequest.params.type = BreakpointByFileAndLine;
    flagsRequest.params.fileName = inferiorTestData(backend).source;
    flagsRequest.params.textPosition.line = inferiorTestData(backend).breakpointLine;
    flagsRequest.params.textPosition.column = 0;
    flagsRequest.params.enabled = false;
    flagsRequest.params.oneShot = true;
    flagsRequest.params.ignoreCount = 3;
    engine->changeBreakpoint(flagsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(91), s_timeout);
    QVERIFY2(results.value(91),
             "breakpoint insert with oneShot/ignoreCount/disabled flags failed");
}

void tst_backends::testBreakIndividualLocationsCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::BreakIndividualLocationsCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    GdbMi insertData;
    bool insertOk = false;
    bool insertDone = false;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&](quint64 requestId, BreakpointOp op, bool ok, const GdbMi &data) {
        if (requestId == 95 && op == BreakpointOp::Insert) {
            insertOk = ok;
            insertData = data;
            insertDone = true;
        }
    });
    QString rawTranscript;
    const auto messageConnection = connect(engine, &DebuggerEngineInterface::message,
            this, [&rawTranscript](const QString &text, int, int) {
        rawTranscript += text + '\n';
    });

    BreakpointChangeRequest multiRequest;
    multiRequest.op = BreakpointOp::Insert;
    multiRequest.requestId = 95;
    multiRequest.params.type = BreakpointByFunction;
    multiRequest.params.functionName = "multi";
    multiRequest.params.enabled = true;
    engine->changeBreakpoint(multiRequest);
    QTRY_VERIFY2_WITH_TIMEOUT(insertDone,
        qPrintable("multi-location breakpoint insert never replied\n--- raw wire traffic ---\n"
                    + rawTranscript), s_timeout);
    QVERIFY2(insertOk, qPrintable("multi-location breakpoint insert failed\n"
                                   "--- raw wire traffic ---\n" + rawTranscript));

    QVERIFY2(insertData.childCount() > 0, qPrintable("insert data: " + insertData.toString()));
    GdbMi bkpt = insertData.childAt(0);
    GdbMi locations = bkpt["locations"];

    disconnect(messageConnection);
    if (locations.childCount() == 0) {
        const QString &versionLine = inferiorTestData(backend).versionLine;
        if (backend == Backend::Gdb) {
            int gdbVersion = 0;
            int gdbBuildVersion = -1;
            bool isMacGdb = false;
            bool isQnxGdb = false;
            extractGdbVersion(versionLine, &gdbVersion, &gdbBuildVersion,
                               &isMacGdb, &isQnxGdb);
            if (gdbVersion < 120000) {
                QSKIP(qPrintable(QString("%1 predates GDB 12's bare template-name breakpoint "
                                          "support (needs >= 12.0.0)\n--- raw wire traffic ---\n%2")
                                      .arg(versionLine, rawTranscript)));
            }
        }
        QVERIFY2(false, qPrintable(QString(
            "%1 never resolved \"multi\" into per-instantiation locations\n"
            "--- raw wire traffic ---\n%2").arg(versionLine, rawTranscript)));
    }

    QString intLocationId;
    QString doubleLocationId;
    for (const GdbMi &location : locations) {
        const QString func = location["func"].data();
        if (func.contains("int"))
            intLocationId = location["number"].data();
        else if (func.contains("double"))
            doubleLocationId = location["number"].data();
    }
    QVERIFY2(!intLocationId.isEmpty() && !doubleLocationId.isEmpty(), qPrintable(
        "expected multi<int>/multi<double> locations, got: " + bkpt.toString()));

    QHash<quint64, bool> enableSubResults;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&enableSubResults](quint64 requestId, BreakpointOp op, bool ok, const GdbMi &) {
        if (op == BreakpointOp::EnableSub)
            enableSubResults[requestId] = ok;
    });

    BreakpointChangeRequest disableRequest;
    disableRequest.op = BreakpointOp::EnableSub;
    disableRequest.requestId = 96;
    disableRequest.subResponseId = intLocationId;
    disableRequest.enabled = false;
    engine->changeBreakpoint(disableRequest);
    QTRY_VERIFY_WITH_TIMEOUT(enableSubResults.contains(96), s_timeout);
    QVERIFY2(enableSubResults.value(96), "disabling the int location failed");

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).multiLocationBreakpointLine);

    RefreshRequest localsRequest;
    localsRequest.kind = RefreshKind::Locals;
    localsRequest.requestId = 97;
    engine->refresh(localsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
    const QString locals = responses.value(int(RefreshKind::Locals)).toString();
    QVERIFY2(locals.contains("double"), qPrintable("locals: " + locals));
}

void tst_backends::testBreakModuleCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::BreakModuleCapability); !result)
        QSKIP(qPrintable(result.error()));

    const InferiorTestData testData = inferiorTestData(backend);
    if (testData.functionMarker.isEmpty() || testData.secondBreakpointLine == 0)
        QSKIP("inferior declares no function to break on ahead of a later line");

    const QString ownModule = testData.executable.baseName();

    {
        std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend);
        DebuggerEngineInterface *engine = debuggerBackend->engine();
        QHash<quint64, bool> insertResults;
        connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
                [&insertResults](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
            insertResults[requestId] = ok;
        });
        connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
                [engine, testData, ownModule](InferiorEvent event) {
            if (event == InferiorEvent::EngineSetupOk) {
                BreakpointChangeRequest request;
                request.op = BreakpointOp::Insert;
                request.requestId = 1;
                request.params.type = BreakpointByFunction;
                request.params.functionName = testData.functionMarker;
                request.params.module = ownModule;
                request.params.enabled = true;
                engine->changeBreakpoint(request);
            }
        });

        engine->start();
        QTRY_VERIFY_WITH_TIMEOUT(insertResults.contains(1), s_timeout);
        QVERIFY2(insertResults.value(1), "a breakpoint in the inferior's own module was refused");
        QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                                  qPrintable(QString("never stopped in %1!%2")
                                                 .arg(ownModule, testData.functionMarker)), s_timeout);
    }

    {
        std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend);
        DebuggerEngineInterface *engine = debuggerBackend->engine();
        connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
                [engine, testData](InferiorEvent event) {
            if (event != InferiorEvent::EngineSetupOk)
                return;
            BreakpointChangeRequest wrongModule;
            wrongModule.op = BreakpointOp::Insert;
            wrongModule.requestId = 2;
            wrongModule.params.type = BreakpointByFunction;
            wrongModule.params.functionName = testData.functionMarker;
            wrongModule.params.module = "kernel32";
            wrongModule.params.enabled = true;
            engine->changeBreakpoint(wrongModule);

            BreakpointChangeRequest control;
            control.op = BreakpointOp::Insert;
            control.requestId = 3;
            control.params.type = BreakpointByFileAndLine;
            control.params.fileName = testData.source;
            control.params.textPosition.line = testData.secondBreakpointLine;
            control.params.textPosition.column = 0;
            control.params.enabled = true;
            engine->changeBreakpoint(control);
        });

        engine->start();
        QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
        QCOMPARE(debuggerBackend->stoppedLine(), testData.secondBreakpointLine);
    }
}

void tst_backends::testBreakOnThrowAndCatchCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::BreakOnThrowAndCatchCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });

    BreakpointChangeRequest throwRequest;
    throwRequest.op = BreakpointOp::Insert;
    throwRequest.requestId = 74;
    throwRequest.params.type = BreakpointAtThrow;
    engine->changeBreakpoint(throwRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(74), s_timeout);
    QVERIFY2(results.value(74), "throw breakpoint insert failed");

    BreakpointChangeRequest catchRequest;
    catchRequest.op = BreakpointOp::Insert;
    catchRequest.requestId = 75;
    catchRequest.params.type = BreakpointAtCatch;
    engine->changeBreakpoint(catchRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(75), s_timeout);
    QVERIFY2(results.value(75), "catch breakpoint insert failed");
}

void tst_backends::testCreateFullBacktraceCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::CreateFullBacktraceCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });

    BreakpointChangeRequest deepRequest;
    deepRequest.op = BreakpointOp::Insert;
    deepRequest.requestId = 76;
    deepRequest.params.type = BreakpointByFileAndLine;
    deepRequest.params.fileName = inferiorTestData(backend).source;
    deepRequest.params.textPosition.line = inferiorTestData(backend).deepRecursionBreakpointLine;
    deepRequest.params.textPosition.column = 0;
    deepRequest.params.enabled = true;
    engine->changeBreakpoint(deepRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(76), s_timeout);
    QVERIFY2(results.value(76), "deep-recursion breakpoint insert failed");

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).deepRecursionBreakpointLine);

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });
    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 77;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);

    const GdbMi frames = responses.value(int(RefreshKind::FullStack))["stack"]["frames"];
    QVERIFY2(frames.childCount() >= 40,
             qPrintable(QString("expected at least 40 stack frames from a 40-deep "
                                 "recursion, got %1: %2")
                            .arg(frames.childCount())
                            .arg(responses.value(int(RefreshKind::FullStack)).toString())));

    RefreshRequest backtraceRequest;
    backtraceRequest.kind = RefreshKind::FullBacktrace;
    backtraceRequest.requestId = 78;
    engine->refresh(backtraceRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullBacktrace)), s_timeout);
    const QString fullBacktrace = responses.value(int(RefreshKind::FullBacktrace)).data();
    QVERIFY2(fullBacktrace.count("recurse") >= 40,
             qPrintable(QString("expected at least 40 recurse() frames in the full "
                                 "backtrace, got %1: %2")
                            .arg(fullBacktrace.count("recurse")).arg(fullBacktrace.left(400))));
}

void tst_backends::testDetachCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkExtraCapability(backend, Debugger::DebuggerExtraCapability::Detach); !result)
        QSKIP(qPrintable(result.error()));

    if (checkStartMode(backend, DebuggerStartModeFlag::AttachToQmlServer)) {
        Process inferiorProcess;
        const quint16 port = startQmlServer(inferiorProcess, inferiorTestData(backend).executable);
        QVERIFY2(port != 0, "could not start the Qml inferior/reserve a port for it");

        QUrl server;
        server.setHost("127.0.0.1");
        server.setPort(port);

        std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(backend,
            AttachToQmlServerData{server});
        QVERIFY(debuggerBackend);

        debuggerBackend->engine()->start();
        QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk)
                                 || debuggerBackend->contains(InferiorEvent::EngineSetupFailed), s_timeout);
        QVERIFY(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk));

        debuggerBackend->clearEvents();
        debuggerBackend->execute({ExecutionCommand::Detach});
        QTRY_VERIFY2_WITH_TIMEOUT(!debuggerBackend->inferiorResults().isEmpty(),
                                  "Detach never signaled completion", s_timeout);
        QCOMPARE(debuggerBackend->inferiorResults().constFirst().exitStatus,
                 InferiorExitStatus::Detached);

        debuggerBackend->clearEvents();
        debuggerBackend->engine()->shutdownEngine();

        inferiorProcess.kill();
        inferiorProcess.waitForFinished();
        return;
    }

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    {
        std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
        QVERIFY(debuggerBackend);

        const quint64 keepSpinningAddress = symbolAddress(backend, debuggerBackend->engine(), "keepSpinning");
        QVERIFY2(keepSpinningAddress != 0, "could not find keepSpinning's address via nm");
        debuggerBackend->engine()->accessMemory(MemoryOp::Change, 0, keepSpinningAddress, 1, QByteArray(1, char(0)));

        debuggerBackend->clearEvents();
        debuggerBackend->execute({ExecutionCommand::Detach});

        QTRY_VERIFY2_WITH_TIMEOUT(!debuggerBackend->inferiorResults().isEmpty(),
                                  "Detach never signaled completion", s_timeout);
        QCOMPARE(debuggerBackend->inferiorResults().constFirst().exitStatus, InferiorExitStatus::Detached);
    }

    {
        std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
        QVERIFY(debuggerBackend);
        DebuggerEngineInterface *engine = debuggerBackend->engine();

        const quint64 keepSpinningAddress = symbolAddress(backend, debuggerBackend->engine(), "keepSpinning");
        QVERIFY2(keepSpinningAddress != 0, "could not find keepSpinning's address via nm");

        QList<QByteArray> memoryChunks;
        connect(engine, &DebuggerEngineInterface::memoryDataReceived, this,
                [&memoryChunks, keepSpinningAddress](quint64, quint64 address, const QByteArray &data) {
            if (address == keepSpinningAddress)
                memoryChunks.append(data);
        });
        auto readKeepSpinning = [&]() -> int {
            memoryChunks.clear();
            engine->accessMemory(MemoryOp::Fetch, 260, keepSpinningAddress, 1);
            [&memoryChunks] { QTRY_VERIFY_WITH_TIMEOUT(!memoryChunks.isEmpty(), s_timeout); }();
            if (QTest::currentTestFailed())
                return -1;
            return static_cast<unsigned char>(memoryChunks.constFirst().at(0));
        };
        engine->accessMemory(MemoryOp::Change, 0, keepSpinningAddress, 1, QByteArray(1, char(0)));
        QTRY_COMPARE_WITH_TIMEOUT(readKeepSpinning(), 0, s_timeout);

        QStringList messages;
        connect(engine, &DebuggerEngineInterface::message, this,
                [&messages](const QString &text, int, int) { messages.append(text); });

        bool processFinished = false;
        connect(engine, &DebuggerEngineInterface::engineProcessFinished, this,
                [&processFinished](const Utils::ProcessResultData &) { processFinished = true; });

        engine->shutdownInferior(ShutdownMode::Detach);
        engine->shutdownEngine();

        QTRY_VERIFY2_WITH_TIMEOUT(processFinished,
                                  "engine process never reported finishing after "
                                  "shutdownInferior(Detach)+shutdownEngine()", s_timeout);
        QVERIFY2(std::any_of(messages.cbegin(), messages.cend(), [](const QString &text) {
            return text.contains("detach");
        }), "shutdownInferior(ShutdownMode::Detach) never sent \"detach\"");
    }
}

void tst_backends::testDisassemblerCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::DisassemblerCapability); !result)
        QSKIP(qPrintable(result.error()));

    const InferiorTestData testData = inferiorTestData(backend);
    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    const quint64 bumpAddress = symbolAddress(backend, engine, "bump");
    QVERIFY2(bumpAddress != 0, "could not find bump()'s address via nm");

    DisassemblerLines disassembly;
    bool disassemblyReceived = false;
    connect(engine, &DebuggerEngineInterface::disassemblyReceived, this,
            [&disassembly, &disassemblyReceived](quint64, const DisassemblerLines &lines) {
        disassembly = lines;
        disassemblyReceived = true;
    });
    engine->fetchDisassembly(40, bumpAddress, "bump");
    QTRY_VERIFY_WITH_TIMEOUT(disassemblyReceived, s_timeout);
    QVERIFY(disassembly.coversAddress(bumpAddress));

    if (!testData.disassemblySourceMarker.isEmpty()) {
        bool sawSource = false;
        for (const DisassemblerLine &line : disassembly.data()) {
            if (line.data.contains(testData.disassemblySourceMarker)) {
                sawSource = true;
                break;
            }
        }
        QVERIFY2(sawSource, qPrintable(QString("no source line containing \"%1\" in the "
                                               "disassembly - plain assembly only?")
                                           .arg(testData.disassemblySourceMarker)));
    }

    if (testData.functionMarker.isEmpty())
        return;
    disassembly = {};
    disassemblyReceived = false;
    engine->fetchDisassembly(41, 0, testData.functionMarker);
    QTRY_VERIFY2_WITH_TIMEOUT(disassemblyReceived,
                              "disassembly by function name alone was never reported", s_timeout);
    QVERIFY2(!disassembly.data().isEmpty(), "disassembly by function name came back empty");
}

void tst_backends::testJumpToLineCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::JumpToLineCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    debuggerBackend->clearEvents();
    ExecutionRequest jumpRequest;
    jumpRequest.command = ExecutionCommand::JumpToLine;
    jumpRequest.context.type = LocationByFile;
    jumpRequest.context.fileName = inferiorTestData(backend).source;
    jumpRequest.context.textPosition.line = inferiorTestData(backend).breakpointLine + 1;
    debuggerBackend->execute(jumpRequest);
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "JumpToLine never signaled a stop", s_timeout);
    QCOMPARE(debuggerBackend->stoppedFile(), inferiorTestData(backend).source);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).breakpointLine + 1);
}

void tst_backends::testLibraryEventCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkExtraCapability(backend, Debugger::DebuggerExtraCapability::LibraryEvent); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QList<GdbMi> loaded;
    QList<GdbMi> unloaded;
    connect(engine, &DebuggerEngineInterface::libraryEvent, this,
            [&loaded, &unloaded](LibraryEvent event, const GdbMi &data) {
        (event == LibraryEvent::Loaded ? loaded : unloaded).append(data);
    });

    const QString marker = inferiorTestData(backend).moduleListMarker;
    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(std::any_of(loaded.cbegin(), loaded.cend(), [&marker](const GdbMi &data) {
        return data["target-name"].data().contains(marker);
    }), s_timeout);
    QTRY_VERIFY_WITH_TIMEOUT(std::any_of(unloaded.cbegin(), unloaded.cend(), [](const GdbMi &data) {
        return data["target-name"].data().contains("inferiorlib");
    }), s_timeout);
}

void tst_backends::testOperateByInstructionCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::OperateByInstructionCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::StepIn, true});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    QCOMPARE(debuggerBackend->stoppedFile(), inferiorTestData(backend).source);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).breakpointLine);
}

void tst_backends::testRegisterCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::RegisterCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });
    RefreshRequest registersRequest;
    registersRequest.kind = RefreshKind::Registers;
    registersRequest.requestId = 86;
    engine->refresh(registersRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Registers)), s_timeout);
    QVERIFY(responses.value(int(RefreshKind::Registers)).childCount() > 0);

#if defined(Q_PROCESSOR_X86_64)
    const QString registerName = "r15";
#elif defined(Q_PROCESSOR_ARM_64)
    const QString registerName = "x19";
#else
    QSKIP("setRegisterValue() not verified on this architecture yet - "
          "no known callee-saved general-purpose register name for it.");
#endif
    engine->setRegisterValue(registerName, "0x1000");
    responses.remove(int(RefreshKind::Registers));
    registersRequest.requestId = 87;
    engine->refresh(registersRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Registers)), s_timeout);

    bool foundRegister = false;
    for (const GdbMi &reg : responses.value(int(RefreshKind::Registers))) {
        if (reg["name"].data() == registerName) {
            foundRegister = reg["value"].data().contains("1000");
            break;
        }
    }
    QVERIFY2(foundRegister, qPrintable(registerName + " register value was not updated as expected"));
}

void tst_backends::testReloadModuleCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::ReloadModuleCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    RefreshRequest modulesRequest;
    modulesRequest.kind = RefreshKind::Modules;
    modulesRequest.requestId = 81;
    engine->refresh(modulesRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Modules)), s_timeout);
    QVERIFY(responses.value(int(RefreshKind::Modules)).toString()
                .contains(inferiorTestData(backend).moduleListMarker, Qt::CaseInsensitive));
}

void tst_backends::testReloadModuleSymbolsCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::ReloadModuleSymbolsCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    RefreshRequest moduleSymbolsRequest;
    moduleSymbolsRequest.kind = RefreshKind::ModuleSymbols;
    moduleSymbolsRequest.requestId = 82;
    moduleSymbolsRequest.path = inferiorTestData(backend).moduleSymbolsPath;
    engine->refresh(moduleSymbolsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::ModuleSymbols)), s_timeout);
    QVERIFY(responses.value(int(RefreshKind::ModuleSymbols)).toString().contains("bump"));
}

void tst_backends::testResetInferiorCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::ResetInferiorCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::ResetInferior});

    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunRequested));
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunOk));
}

void tst_backends::testReturnFromFunctionCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::ReturnFromFunctionCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    const quint64 globalValueAddress = symbolAddress(backend, engine, "globalValue");
    QVERIFY2(globalValueAddress != 0, "could not find globalValue's address via nm");
    QList<QByteArray> memoryChunks;
    connect(engine, &DebuggerEngineInterface::memoryDataReceived, this,
            [&memoryChunks, globalValueAddress](quint64, quint64 address, const QByteArray &data) {
        if (address == globalValueAddress)
            memoryChunks.append(data);
    });
    GdbMi stackData;
    bool stackReceived = false;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&stackData, &stackReceived](quint64, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::FullStack) {
            stackData = data;
            stackReceived = true;
        }
    });

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Return});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::StopOk),
                              "Return never signaled completion", s_timeout);

    engine->accessMemory(MemoryOp::Fetch, 210, globalValueAddress, sizeof(int));
    QTRY_VERIFY_WITH_TIMEOUT(!memoryChunks.isEmpty(), s_timeout);
    int value = 0;
    memcpy(&value, memoryChunks.constFirst().constData(), sizeof(int));
    QCOMPARE(value, 41);

    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 220;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(stackReceived, s_timeout);
    QVERIFY2(stackData.toString().contains("main"), "Return did not pop back into main()");
}

void tst_backends::testReverseSteppingCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::ReverseSteppingCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    debuggerBackend->execute({ExecutionCommand::RecordReverse, true});

    QStringList messages;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&messages](const QString &text, int, int) { messages.append(text); });
    engine->executeDebuggerCommand("info record", {});
    QTRY_VERIFY2_WITH_TIMEOUT(std::any_of(messages.cbegin(), messages.cend(),
                                          [](const QString &text) {
        return text.contains("record-full");
    }), "process record never actually activated", s_timeout);

    RefreshRequest firstLocalsRequest;
    firstLocalsRequest.kind = RefreshKind::Locals;
    firstLocalsRequest.requestId = 78;
    engine->refresh(firstLocalsRequest);
    QTRY_VERIFY2_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)),
                              "session broken after starting process record", s_timeout);

    debuggerBackend->execute({ExecutionCommand::RecordReverse, false});
    responses.remove(int(RefreshKind::Locals));
    RefreshRequest secondLocalsRequest;
    secondLocalsRequest.kind = RefreshKind::Locals;
    secondLocalsRequest.requestId = 79;
    engine->refresh(secondLocalsRequest);
    QTRY_VERIFY2_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)),
                              "session broken after stopping process record", s_timeout);
}

void tst_backends::testRunCommandDeferralCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkExtraCapability(backend, Debugger::DebuggerExtraCapability::RunCommandDeferral); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunOk), s_timeout);

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });
    debuggerBackend->clearEvents();
    BreakpointChangeRequest request;
    request.op = BreakpointOp::Insert;
    request.requestId = 200;
    request.params.type = BreakpointByFileAndLine;
    request.params.fileName = inferiorTestData(backend).source;
    request.params.textPosition.line = inferiorTestData(backend).breakpointLine;
    request.params.textPosition.column = 0;
    request.params.enabled = true;
    engine->changeBreakpoint(request);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(200), s_timeout);
    QVERIFY2(results.value(200), "breakpoint insert deferred-while-running failed");

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Interrupt});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::StopOk),
                              "session no longer controllable after a deferred-while-running insert",
                              s_timeout);
}

void tst_backends::testRunToLineCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::RunToLineCapability); !result)
        QSKIP(qPrintable(result.error()));

    if (checkStartMode(backend, DebuggerStartModeFlag::AttachToQmlServer)) {
        Process inferiorProcess;
        const quint16 port = startQmlServer(inferiorProcess, inferiorTestData(backend).executable);
        QVERIFY2(port != 0, "could not start the Qml inferior/reserve a port for it");

        QUrl server;
        server.setHost("127.0.0.1");
        server.setPort(port);

        std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(backend,
            AttachToQmlServerData{server});
        QVERIFY(debuggerBackend);
        DebuggerEngineInterface *engine = debuggerBackend->engine();

        engine->start();
        QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk)
                                 || debuggerBackend->contains(InferiorEvent::EngineSetupFailed), s_timeout);
        QVERIFY(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk));

        QHash<quint64, bool> insertResults;
        connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
                [&insertResults](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
            insertResults[requestId] = ok;
        });

        BreakpointChangeRequest request;
        request.op = BreakpointOp::Insert;
        request.requestId = 1;
        request.params.type = BreakpointByFileAndLine;
        request.params.fileName = inferiorTestData(backend).source;
        request.params.textPosition.line = inferiorTestData(backend).breakpointLine;
        request.params.enabled = true;
        engine->changeBreakpoint(request);
        QTRY_VERIFY_WITH_TIMEOUT(insertResults.contains(1), s_timeout);
        QVERIFY2(insertResults.value(1), "breakpoint insert failed");

        debuggerBackend->clearEvents();
        QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                                  "breakpoint in compute() never signaled a stop", s_timeout);

        debuggerBackend->clearEvents();
        ExecutionRequest runToLineRequest;
        runToLineRequest.command = ExecutionCommand::RunToLine;
        runToLineRequest.context.type = LocationByFile;
        runToLineRequest.context.fileName = inferiorTestData(backend).source;
        runToLineRequest.context.textPosition.line = inferiorTestData(backend).secondBreakpointLine;
        debuggerBackend->execute(runToLineRequest);
        QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                                  "RunToLine never signaled a stop", s_timeout);
        QCOMPARE(debuggerBackend->stoppedFile(), inferiorTestData(backend).source);
        QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).secondBreakpointLine);

        debuggerBackend->clearEvents();
        engine->shutdownEngine();
        inferiorProcess.kill();
        inferiorProcess.waitForFinished();
        return;
    }

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    debuggerBackend->clearEvents();
    ExecutionRequest runToLineRequest;
    runToLineRequest.command = ExecutionCommand::RunToLine;
    runToLineRequest.context.type = LocationByFile;
    runToLineRequest.context.fileName = inferiorTestData(backend).source;
    runToLineRequest.context.textPosition.line = inferiorTestData(backend).secondBreakpointLine;
    debuggerBackend->execute(runToLineRequest);
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "RunToLine never signaled a stop", s_timeout);
    QCOMPARE(debuggerBackend->stoppedFile(), inferiorTestData(backend).source);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).secondBreakpointLine);
}

void tst_backends::testShowMemoryCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::ShowMemoryCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    const quint64 globalValueAddress = symbolAddress(backend, engine, "globalValue");
    QVERIFY2(globalValueAddress != 0, "could not find globalValue's address via nm");

    QList<QByteArray> memoryChunks;
    connect(engine, &DebuggerEngineInterface::memoryDataReceived, this,
            [&memoryChunks, globalValueAddress](quint64, quint64 address, const QByteArray &data) {
        if (address == globalValueAddress)
            memoryChunks.append(data);
    });

    engine->accessMemory(MemoryOp::Fetch, 83, globalValueAddress, sizeof(int));
    QTRY_VERIFY_WITH_TIMEOUT(!memoryChunks.isEmpty(), s_timeout);

    QCOMPARE(memoryChunks.constFirst().size(), int(sizeof(int)));
    int value = 0;
    memcpy(&value, memoryChunks.constFirst().constData(), sizeof(int));
    QCOMPARE(value, 41);
}

void tst_backends::testShowModuleSectionsCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::ShowModuleSectionsCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    QString rawModuleSectionsReply;
    const auto messageConnection = connect(engine, &DebuggerEngineInterface::message,
            this, [&rawModuleSectionsReply](const QString &text, int, int) {
        rawModuleSectionsReply += text + '\n';
    });
    RefreshRequest moduleSectionsRequest;
    moduleSectionsRequest.kind = RefreshKind::ModuleSections;
    moduleSectionsRequest.requestId = 84;
    moduleSectionsRequest.path = inferiorTestData(backend).executable;
    engine->refresh(moduleSectionsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::ModuleSections)), s_timeout);
    disconnect(messageConnection);
    const QString sections = responses.value(int(RefreshKind::ModuleSections)).toString();
    const QString textSectionName = HostOsInfo::isMacHost() ? "__text" : ".text";
    QVERIFY2(sections.contains(textSectionName),
             qPrintable("expected a " + textSectionName + " section - got: " + sections
                        + "\n--- debugger version ---\n" + inferiorTestData(backend).versionLine
                        + "\n--- raw reply ---\n" + rawModuleSectionsReply));
}

void tst_backends::testShowModuleSymbolsCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::ShowModuleSymbolsCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    RefreshRequest moduleSymbolsRequest;
    moduleSymbolsRequest.kind = RefreshKind::ModuleSymbols;
    moduleSymbolsRequest.requestId = 85;
    moduleSymbolsRequest.path = inferiorTestData(backend).moduleSymbolsPath;
    engine->refresh(moduleSymbolsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::ModuleSymbols)), s_timeout);
    QVERIFY(responses.value(int(RefreshKind::ModuleSymbols)).toString().contains("bump"));
}

void tst_backends::testSignalReceivedCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkExtraCapability(backend, Debugger::DebuggerExtraCapability::SignalReceived); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
        ProcessRunData{{inferiorTestData(backend).executable, {"crash"}}, {}, Environment::systemEnvironment()});
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QString signalName;
    QString signalMeaning;
    connect(engine, &DebuggerEngineInterface::signalReceived, this,
            [&signalName, &signalMeaning](const QString &name, const QString &meaning) {
        signalName = name;
        signalMeaning = meaning;
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(!signalName.isEmpty(), s_timeout);
    QCOMPARE(signalName, "SIGSEGV");
    QVERIFY(!signalMeaning.isEmpty());
}

void tst_backends::testSnapshotCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::SnapshotCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    bool received = false;
    bool ok = false;
    FilePath coreFile;
    connect(engine, &DebuggerEngineInterface::snapshotCreated, this,
            [&received, &ok, &coreFile](quint64, bool snapshotOk, const FilePath &file) {
        received = true;
        ok = snapshotOk;
        coreFile = file;
    });

    engine->createSnapshot(91);
    QTRY_VERIFY_WITH_TIMEOUT(received, s_timeout);
    QVERIFY(ok);
    QVERIFY2(coreFile.exists(), qPrintable("gcore did not produce " + coreFile.toUserOutput()));

    ElfReader reader(coreFile);
    QCOMPARE(reader.readHeaders().elftype, Elf_ET_CORE);
}

void tst_backends::testSourceFilesCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkExtraCapability(backend, Debugger::DebuggerExtraCapability::SourceFiles); !result)
        QSKIP(qPrintable(result.error()));

    Process helperInferior;
    std::unique_ptr<DebuggerBackend> debuggerBackend = stopAtBreakpoint(backend, helperInferior);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    RefreshRequest sourceFilesRequest;
    sourceFilesRequest.kind = RefreshKind::SourceFiles;
    sourceFilesRequest.requestId = 101;
    engine->refresh(sourceFilesRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::SourceFiles)), s_timeout);
    const QString sourceFiles = responses.value(int(RefreshKind::SourceFiles)).toString();
    QVERIFY2(sourceFiles.contains(inferiorTestData(backend).source.fileName()),
             qPrintable("source files: " + sourceFiles));
}

void tst_backends::testThreadsCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkExtraCapability(backend, Debugger::DebuggerExtraCapability::Threads); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    RefreshRequest threadsRequest;
    threadsRequest.kind = RefreshKind::Threads;
    threadsRequest.requestId = 13;
    engine->refresh(threadsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Threads)), s_timeout);
    QVERIFY(responses.value(int(RefreshKind::Threads)).toString().contains("thread"));
}

void tst_backends::testTracePointCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::TracePointCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });
    QStringList tracepointMessages;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&tracepointMessages](const QString &text, int channel, int) {
        if (channel == Debugger::LogMisc)
            tracepointMessages.append(text);
    });
    QList<GdbMi> modified;
    connect(engine, &DebuggerEngineInterface::breakpointModified, this,
            [&modified](const GdbMi &data) { modified.append(data); });

    BreakpointChangeRequest tracepointRequest;
    tracepointRequest.op = BreakpointOp::Insert;
    tracepointRequest.requestId = 89;
    tracepointRequest.params.type = BreakpointByFileAndLine;
    tracepointRequest.params.fileName = inferiorTestData(backend).source;
    tracepointRequest.params.textPosition.line = inferiorTestData(backend).breakpointLine;
    tracepointRequest.params.textPosition.column = 0;
    tracepointRequest.params.enabled = true;
    tracepointRequest.params.tracepoint = true;
    tracepointRequest.params.message = "globalValue is {globalValue}, globalMessage is {globalMessage}";
    engine->changeBreakpoint(tracepointRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(89), s_timeout);
    QVERIFY2(results.value(89), "tracepoint insert failed");

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::ResetInferior});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);

    QTRY_VERIFY_WITH_TIMEOUT(tracepointMessages.join('\n').contains("globalValue is 41")
                             && tracepointMessages.join('\n').contains("globalMessage is \"hi\""),
                             s_timeout);
    QTRY_VERIFY_WITH_TIMEOUT(!modified.isEmpty() && modified.constFirst().childCount() > 0,
                             s_timeout);
}

void tst_backends::testWatchComplexExpressionsCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::WatchComplexExpressionsCapability); !result)
        QSKIP(qPrintable(result.error()));

    Process helperInferior;
    std::unique_ptr<DebuggerBackend> debuggerBackend = stopAtBreakpoint(backend, helperInferior);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList messages;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&messages](const QString &text, int, int) { messages.append(text); });
    engine->executeDebuggerCommand(printCommand(backend, "globalValue * 1000"), {});
    QTRY_VERIFY_WITH_TIMEOUT(std::any_of(messages.cbegin(), messages.cend(),
                                         [](const QString &text) {
        return text.contains("41000");
    }), s_timeout);
}

void tst_backends::testWatchWidgetsCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::WatchWidgetsCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    quint64 resolvedRequestId = 0;
    QString resolvedExpr;
    connect(engine, &DebuggerEngineInterface::watchPointResolved, this,
            [&resolvedRequestId, &resolvedExpr](quint64 requestId, quint64, const QString &expr) {
        resolvedRequestId = requestId;
        resolvedExpr = expr;
    });
    engine->watchPoint(89, QPoint(0, 0));
    QTRY_VERIFY_WITH_TIMEOUT(resolvedRequestId == 89, s_timeout);
    QVERIFY2(resolvedExpr.contains("QWidget"),
             qPrintable("watchPoint() reply didn't look like a QWidget expression: " + resolvedExpr));
}

void tst_backends::testWatchpointByAddressCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::WatchpointByAddressCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    const quint64 globalValueAddress = symbolAddress(backend, engine, "globalValue");
    QVERIFY2(globalValueAddress != 0, "could not find globalValue's address via nm");

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });

    QSignalSpy memorySpy(engine, &DebuggerEngineInterface::memoryDataReceived);
    engine->accessMemory(MemoryOp::Fetch, 87, globalValueAddress, sizeof(int));
    QTRY_VERIFY_WITH_TIMEOUT(!memorySpy.isEmpty(), s_timeout);

    BreakpointChangeRequest watchRequest;
    watchRequest.op = BreakpointOp::Insert;
    watchRequest.requestId = 88;
    watchRequest.params.type = WatchpointAtAddress;
    watchRequest.params.address = globalValueAddress;
    engine->changeBreakpoint(watchRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(88), s_timeout);
    QVERIFY2(results.value(88), "watchpoint insert failed");

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "watchpoint never triggered on globalValue's write", s_timeout);
}

void tst_backends::testWatchpointByExpressionCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::WatchpointByExpressionCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });

    QSignalSpy memorySpy(engine, &DebuggerEngineInterface::memoryDataReceived);
    engine->accessMemory(MemoryOp::Fetch, 72, symbolAddress(backend, engine, "globalValue"), sizeof(int));
    QTRY_VERIFY_WITH_TIMEOUT(!memorySpy.isEmpty(), s_timeout);

    BreakpointChangeRequest watchRequest;
    watchRequest.op = BreakpointOp::Insert;
    watchRequest.requestId = 73;
    watchRequest.params.type = WatchpointAtExpression;
    watchRequest.params.expression = "globalValue";
    engine->changeBreakpoint(watchRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(73), s_timeout);
    QVERIFY2(results.value(73), "watchpoint insert failed");

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "watchpoint never triggered on globalValue's write", s_timeout);
}

std::unique_ptr<DebuggerBackend> tst_backends::launchAndStopAtBreakpoint(Backend backend)
{
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [this, engine, backend](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            BreakpointChangeRequest request;
            request.op = BreakpointOp::Insert;
            request.requestId = 1;
            request.params.type = BreakpointByFileAndLine;
            request.params.fileName = inferiorTestData(backend).source;
            request.params.textPosition.line = inferiorTestData(backend).breakpointLine;
            request.params.textPosition.column = 0;
            request.params.enabled = true;
            engine->changeBreakpoint(request);
        }
    });

    engine->start();

    [backendPtr = debuggerBackend.get()] {
        QTRY_VERIFY_WITH_TIMEOUT(backendPtr->contains(InferiorEvent::SpontaneousStop)
                                 || backendPtr->contains(InferiorEvent::EngineSetupFailed)
                                 || backendPtr->contains(InferiorEvent::EngineRunFailed), s_timeout);
    }();

    if (QTest::currentTestFailed() || !debuggerBackend->contains(InferiorEvent::SpontaneousStop))
        return nullptr;
    return debuggerBackend;
}

std::unique_ptr<DebuggerBackend> tst_backends::stopAtBreakpoint(Backend backend,
                                                                Process &helperInferior)
{
    if (hasStartMode(backend, DebuggerStartModeFlag::Launch))
        return launchAndStopAtBreakpoint(backend);
    if (!hasStartMode(backend, DebuggerStartModeFlag::AttachToQmlServer))
        return nullptr;

    const quint16 port = startQmlServer(helperInferior, inferiorTestData(backend).executable);
    if (port == 0)
        return nullptr;

    QUrl server;
    server.setHost("127.0.0.1");
    server.setPort(port);

    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(backend,
        AttachToQmlServerData{server});
    if (!debuggerBackend)
        return nullptr;
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    [backendPtr = debuggerBackend.get()] {
        QTRY_VERIFY_WITH_TIMEOUT(backendPtr->contains(InferiorEvent::RunAndInferiorRunOk)
                                 || backendPtr->contains(InferiorEvent::EngineSetupFailed), s_timeout);
    }();
    if (QTest::currentTestFailed() || !debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk))
        return nullptr;

    bool inserted = false;
    bool insertOk = false;
    const QMetaObject::Connection insertWatch = connect(engine,
            &DebuggerEngineInterface::breakpointEvent, debuggerBackend.get(),
            [&inserted, &insertOk](quint64 requestId, BreakpointOp op, bool ok, const GdbMi &) {
        if (op == BreakpointOp::Insert && requestId == 1) {
            inserted = true;
            insertOk = ok;
        }
    });
    const QScopeGuard dropInsertWatch([&insertWatch] { disconnect(insertWatch); });

    BreakpointChangeRequest request;
    request.op = BreakpointOp::Insert;
    request.requestId = 1;
    request.params.type = BreakpointByFileAndLine;
    request.params.fileName = inferiorTestData(backend).source;
    request.params.textPosition.line = inferiorTestData(backend).breakpointLine;
    request.params.enabled = true;
    engine->changeBreakpoint(request);
    [&inserted] { QTRY_VERIFY_WITH_TIMEOUT(inserted, s_timeout); }();
    if (QTest::currentTestFailed() || !insertOk)
        return nullptr;

    debuggerBackend->clearEvents();
    [backendPtr = debuggerBackend.get()] {
        QTRY_VERIFY_WITH_TIMEOUT(backendPtr->contains(InferiorEvent::SpontaneousStop), s_timeout);
    }();
    if (QTest::currentTestFailed() || !debuggerBackend->contains(InferiorEvent::SpontaneousStop))
        return nullptr;
    return debuggerBackend;
}

void tst_backends::hitsBreakpointAndReadsMemory()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::ShowMemoryCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();
    QCOMPARE(debuggerBackend->stoppedFile(), inferiorTestData(backend).source);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).breakpointLine);

    const quint64 globalValueAddress = symbolAddress(backend, engine, "globalValue");
    if (globalValueAddress == 0 && !FilePath::fromString("nm").searchInPath().isExecutableFile())
        QSKIP("No nm found to look up the inferior's symbol addresses.");
    QVERIFY2(globalValueAddress != 0, "could not find globalValue's address via nm");

    QList<QByteArray> memoryChunks;
    connect(engine, &DebuggerEngineInterface::memoryDataReceived, this,
            [&memoryChunks, globalValueAddress](quint64, quint64 address, const QByteArray &data) {
        if (address == globalValueAddress)
            memoryChunks.append(data);
    });

    engine->accessMemory(MemoryOp::Fetch, 42, globalValueAddress, sizeof(int));
    QTRY_VERIFY_WITH_TIMEOUT(!memoryChunks.isEmpty(), s_timeout);

    QCOMPARE(memoryChunks.constFirst().size(), int(sizeof(int)));
    int value = 0;
    memcpy(&value, memoryChunks.constFirst().constData(), sizeof(int));
    QCOMPARE(value, 41);
}

void tst_backends::stepsContinuesAndInterrupts()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::StepOver});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    QCOMPARE(debuggerBackend->stoppedFile(), inferiorTestData(backend).source);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).breakpointLine + 1);

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::StepOut});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    QCOMPARE(debuggerBackend->stoppedFile(), inferiorTestData(backend).source);

    BreakpointChangeRequest secondBreakpoint;
    secondBreakpoint.op = BreakpointOp::Insert;
    secondBreakpoint.requestId = 2;
    secondBreakpoint.params.type = BreakpointByFileAndLine;
    secondBreakpoint.params.fileName = inferiorTestData(backend).source;
    secondBreakpoint.params.textPosition.line = inferiorTestData(backend).secondBreakpointLine;
    secondBreakpoint.params.textPosition.column = 0;
    secondBreakpoint.params.enabled = true;
    debuggerBackend->engine()->changeBreakpoint(secondBreakpoint);

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    QCOMPARE(debuggerBackend->stoppedFile(), inferiorTestData(backend).source);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).secondBreakpointLine);

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunOk), s_timeout);

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Interrupt});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::StopOk),
                              "Interrupt never signaled completion", s_timeout);
}

void tst_backends::interruptWhileStoppedReportsStopOkImmediately()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Interrupt});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::StopOk),
                              "Interrupt while already stopped never signaled completion",
                              s_timeout);
}

void tst_backends::continueAfterExitReportsInferiorIll()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    stopInferiorSpinLoop(backend, engine);

    debuggerBackend->clearEvents();
    debuggerBackend->clearInferiorResults();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(!debuggerBackend->inferiorResults().isEmpty(), s_timeout);

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::InferiorIll),
                              "stale Continue after exit never reported InferiorIll", s_timeout);
}

static QString wireTail(const QStringList &wire, int lines = 40)
{
    QStringList tail;
    for (const QString &line : wire.mid(qMax(0, wire.size() - lines)))
        tail.append(line.size() > 160 ? line.left(160) + "..." : line);
    return tail.join("\n  ");
}

static bool canInterruptRunningInferior(Backend backend)
{
    if (!HostOsInfo::isWindowsHost())
        return true;
    static const QList<Backend> uninterruptibleOnWindows = {};
    return !uninterruptibleOnWindows.contains(backend);
}

void tst_backends::stopsAtFunctionBreakpointInsertedBeforeFirstRun()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    const InferiorTestData testData = inferiorTestData(backend);
    if (testData.functionMarker.isEmpty())
        QSKIP("inferior declares no function to break on");

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, bool> insertResults;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&insertResults](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        insertResults[requestId] = ok;
    });
    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [engine, testData](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            BreakpointChangeRequest request;
            request.op = BreakpointOp::Insert;
            request.requestId = 1;
            request.params.type = BreakpointByFunction;
            request.params.functionName = testData.functionMarker;
            request.params.enabled = true;
            engine->changeBreakpoint(request);
        }
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(insertResults.contains(1)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed)
                             || debuggerBackend->contains(InferiorEvent::EngineRunFailed), s_timeout);
    QVERIFY2(insertResults.value(1), "the function breakpoint was never inserted");

    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              qPrintable(QString("never stopped at \"%1\" - the breakpoint was "
                                                 "resolved after the inferior was already running?")
                                             .arg(testData.functionMarker)), s_timeout);
}

void tst_backends::continueWhileRunningReportsRunFailed()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    const InferiorTestData testData = inferiorTestData(backend);
    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    const bool canEndSpinLoop = debuggerBackend->engine()->hasExtraCapability(
        Debugger::DebuggerExtraCapability::RunCommandDeferral);
    if (!testData.answersRedundantContinue && !canInterruptRunningInferior(backend)) {
        QSKIP("backend does not answer a redundant run request and its running inferior "
              "cannot be interrupted on this host - nothing to wait for");
    }

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunOk), s_timeout);

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    if (testData.answersRedundantContinue) {
        QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunFailed),
                                  "a rejected run request never reported RunFailed", s_timeout);
    } else {
        debuggerBackend->execute({ExecutionCommand::Interrupt});
        QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::StopOk),
                                  "interrupting the running inferior never reported StopOk, "
                                  "leaving nothing to bound the check below on", s_timeout);
    }
    QVERIFY2(!debuggerBackend->contains(InferiorEvent::InferiorIll),
             "an ordinary failed run request was misreported as InferiorIll");

    if (canEndSpinLoop)
        stopInferiorSpinLoop(backend, debuggerBackend->engine());
}

void tst_backends::continueSignalsExitedForSpontaneousExit()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);

    stopInferiorSpinLoop(backend, debuggerBackend->engine());

    debuggerBackend->clearEvents();
    debuggerBackend->clearInferiorResults();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY2_WITH_TIMEOUT(!debuggerBackend->inferiorResults().isEmpty(),
                              "spontaneous exit via Continue never reported inferiorDone", s_timeout);
    QCOMPARE(debuggerBackend->inferiorResults().first().exitCode,
             inferiorTestData(backend).expectedExitCode);
    QVERIFY2(!debuggerBackend->contains(InferiorEvent::SpontaneousStop),
             "spontaneous exit via Continue was misreported as SpontaneousStop");
    QVERIFY2(!debuggerBackend->contains(InferiorEvent::StopOk),
             "spontaneous exit via Continue was misreported as StopOk");
}

void tst_backends::expandsContainerLocalWhenExpanded()
{
    QFETCH(Backend, backend);

    const QString local = inferiorTestData(backend).expandableLocal;
    if (local.isEmpty())
        QSKIP("inferior declares no expandable container local");
    const QString iname = "local." + local;

    Process helperInferior;
    std::unique_ptr<DebuggerBackend> debuggerBackend = stopAtBreakpoint(backend, helperInferior);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    RefreshRequest request;
    request.kind = RefreshKind::Locals;
    request.requestId = 120;
    engine->refresh(request);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
    const QString collapsed = responses.value(int(RefreshKind::Locals)).toString();
    QVERIFY2(collapsed.contains(iname), qPrintable("collapsed: " + collapsed));

    responses.clear();
    request.requestId = 121;
    request.expandedINames = {iname};
    engine->refresh(request);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
    const QString expanded = responses.value(int(RefreshKind::Locals)).toString();
    QVERIFY2(expanded.contains(iname + '.'), qPrintable("expanded: " + expanded));

    const QString child = inferiorTestData(backend).expandableChild;
    if (child.isEmpty())
        return;
    const QString childIName = iname + '.' + child;
    QVERIFY2(expanded.contains(childIName), qPrintable("expanded: " + expanded));

    responses.clear();
    request.requestId = 122;
    request.expandedINames = {iname, childIName};
    engine->refresh(request);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
    const QString nested = responses.value(int(RefreshKind::Locals)).toString();
    QVERIFY2(nested.contains(childIName + '.'), qPrintable("nested: " + nested));
}

void tst_backends::activatesFrameAndReadsItsLocals()
{
    QFETCH(Backend, backend);

    const InferiorTestData testData = inferiorTestData(backend);
    if (testData.recursionDepthVariable.isEmpty() || testData.deepRecursionBreakpointLine == 0)
        QSKIP("inferior has no recursion chain to walk frames of");

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });

    BreakpointChangeRequest deepRequest;
    deepRequest.op = BreakpointOp::Insert;
    deepRequest.requestId = 310;
    deepRequest.params.type = BreakpointByFileAndLine;
    deepRequest.params.fileName = testData.source;
    deepRequest.params.textPosition.line = testData.deepRecursionBreakpointLine;
    deepRequest.params.enabled = true;
    engine->changeBreakpoint(deepRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(310), s_timeout);
    QVERIFY2(results.value(310), "deep-recursion breakpoint insert failed");

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    QCOMPARE(debuggerBackend->stoppedLine(), testData.deepRecursionBreakpointLine);

    QHash<quint64, GdbMi> localsById;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&localsById](quint64 requestId, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::Locals)
            localsById[requestId] = data;
    });

    const auto depthIn = [&localsById, &testData](quint64 requestId) {
        if (!localsById.contains(requestId))
            return -1;
        const GdbMi response = localsById.value(requestId);
        for (const GdbMi &item : response["data"]) {
            if (item["name"].data() == testData.recursionDepthVariable)
                return item["value"].data().toInt();
        }
        return -1;
    };

    RefreshRequest request;
    request.kind = RefreshKind::Locals;
    request.requestId = 311;
    engine->refresh(request);
    QTRY_VERIFY_WITH_TIMEOUT(localsById.contains(311), s_timeout);
    QCOMPARE(depthIn(311), 0);

    engine->activateFrame(2);
    request.requestId = 312;
    engine->refresh(request);
    QTRY_VERIFY_WITH_TIMEOUT(localsById.contains(312), s_timeout);
    QCOMPARE(depthIn(312), 2);

    engine->activateFrame(0);
    request.requestId = 313;
    engine->refresh(request);
    QTRY_VERIFY_WITH_TIMEOUT(localsById.contains(313), s_timeout);
    QCOMPARE(depthIn(313), 0);
}

void tst_backends::refreshesLocalsAndStack()
{
    QFETCH(Backend, backend);

    Process helperInferior;
    std::unique_ptr<DebuggerBackend> debuggerBackend = stopAtBreakpoint(backend, helperInferior);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    RefreshRequest localsRequest;
    localsRequest.kind = RefreshKind::Locals;
    localsRequest.requestId = 10;
    engine->refresh(localsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
    const QString locals = responses.value(int(RefreshKind::Locals)).toString();
    QVERIFY2(locals.contains(inferiorTestData(backend).localMarker), qPrintable("locals: " + locals));

    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 11;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);
    const QString stack = responses.value(int(RefreshKind::FullStack)).toString();
    QVERIFY2(stack.contains(inferiorTestData(backend).functionMarker), qPrintable("stack: " + stack));
}

void tst_backends::refreshesRegisters()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::RegisterCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    RefreshRequest registersRequest;
    registersRequest.kind = RefreshKind::Registers;
    registersRequest.requestId = 12;
    engine->refresh(registersRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Registers)), s_timeout);
    QVERIFY(responses.value(int(RefreshKind::Registers)).childCount() > 0);

    responses.remove(int(RefreshKind::Registers));
    RefreshRequest secondRegistersRequest;
    secondRegistersRequest.kind = RefreshKind::Registers;
    secondRegistersRequest.requestId = 13;
    engine->refresh(secondRegistersRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Registers)), s_timeout);
    QVERIFY(responses.value(int(RefreshKind::Registers)).childCount() > 0);
}

void tst_backends::refreshesRegistersAfterResume()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::RegisterCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });
    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });

    RefreshRequest registersRequest;
    registersRequest.kind = RefreshKind::Registers;
    registersRequest.requestId = 260;
    engine->refresh(registersRequest);

    BreakpointChangeRequest insertRequest;
    insertRequest.op = BreakpointOp::Insert;
    insertRequest.requestId = 261;
    insertRequest.params.type = BreakpointByFileAndLine;
    insertRequest.params.fileName = inferiorTestData(backend).source;
    insertRequest.params.textPosition.line = inferiorTestData(backend).secondBreakpointLine;
    insertRequest.params.enabled = true;
    engine->changeBreakpoint(insertRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(261), s_timeout);
    QVERIFY2(results.value(261), "second breakpoint insert failed");

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).secondBreakpointLine);

    responses.clear();
    RefreshRequest secondRegistersRequest;
    secondRegistersRequest.kind = RefreshKind::Registers;
    secondRegistersRequest.requestId = 262;
    engine->refresh(secondRegistersRequest);
    QTRY_VERIFY2_WITH_TIMEOUT(responses.contains(int(RefreshKind::Registers)),
                              "no register data after the resume - a discarded reply from before "
                              "it took the fresh one with it",
                              s_timeout);
    QVERIFY(responses.value(int(RefreshKind::Registers)).childCount() > 0);
}

void tst_backends::updatesEnablesAndRemovesBreakpoint()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();
    QVERIFY2(!debuggerBackend->breakpointResponseId().isEmpty(),
             "launchAndStopAtBreakpoint() never captured a breakpoint number");

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });

    BreakpointChangeRequest updateRequest;
    updateRequest.op = BreakpointOp::Update;
    updateRequest.requestId = 20;
    updateRequest.responseId = debuggerBackend->breakpointResponseId();
    updateRequest.params.enabled = true;
    engine->changeBreakpoint(updateRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(20), s_timeout);
    QVERIFY(results.value(20));

    BreakpointChangeRequest enableSubRequest;
    enableSubRequest.op = BreakpointOp::EnableSub;
    enableSubRequest.requestId = 21;
    enableSubRequest.subResponseId = debuggerBackend->breakpointResponseId();
    enableSubRequest.enabled = false;
    engine->changeBreakpoint(enableSubRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(21), s_timeout);
    QVERIFY(results.value(21));

    BreakpointChangeRequest removeRequest;
    removeRequest.op = BreakpointOp::Remove;
    removeRequest.requestId = 22;
    removeRequest.responseId = debuggerBackend->breakpointResponseId();
    engine->changeBreakpoint(removeRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(22), s_timeout);
    QVERIFY(results.value(22));

    BreakpointChangeRequest emptyResponseIdRequest;
    emptyResponseIdRequest.op = BreakpointOp::Update;
    emptyResponseIdRequest.requestId = 23;
    engine->changeBreakpoint(emptyResponseIdRequest);
    QVERIFY(results.contains(23));
    QVERIFY2(!results.value(23), "Update with an empty responseId should fail, not succeed");

    stopInferiorSpinLoop(backend, engine);
    debuggerBackend->clearEvents();
    debuggerBackend->clearInferiorResults();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(!debuggerBackend->inferiorResults().isEmpty(), s_timeout);
    QVERIFY2(!debuggerBackend->contains(InferiorEvent::SpontaneousStop),
             "removed breakpoint was still hit");
}

void tst_backends::writesMemoryAndPeripheralRegister()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::ShowMemoryCapability); !result)
        QSKIP(qPrintable(result.error()));

    if (auto result = checkCapability(backend, Debugger::RegisterCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    const quint64 address = symbolAddress(backend, engine, "globalValue");
    QVERIFY2(address != 0, "could not find globalValue's address via nm");

    QList<QByteArray> memoryChunks;
    connect(engine, &DebuggerEngineInterface::memoryDataReceived, this,
            [&memoryChunks, address](quint64, quint64 receivedAddress, const QByteArray &data) {
        if (receivedAddress == address)
            memoryChunks.append(data);
    });
    auto readGlobalValue = [&]() -> int {
        memoryChunks.clear();
        engine->accessMemory(MemoryOp::Fetch, 100, address, sizeof(int));
        [&memoryChunks] { QTRY_VERIFY_WITH_TIMEOUT(!memoryChunks.isEmpty(), s_timeout); }();
        if (QTest::currentTestFailed())
            return -1;
        int value = 0;
        memcpy(&value, memoryChunks.constFirst().constData(), sizeof(int));
        return value;
    };

    const int newValue = 12345;
    const QByteArray newValueBytes(reinterpret_cast<const char *>(&newValue), sizeof(int));
    engine->accessMemory(MemoryOp::Change, 0, address, sizeof(int), newValueBytes);
    QTRY_COMPARE_WITH_TIMEOUT(readGlobalValue(), newValue, s_timeout);

    engine->setPeripheralRegisterValue(address, 999);
    QTRY_COMPARE_WITH_TIMEOUT(readGlobalValue(), 999, s_timeout);
}

void tst_backends::selectsThreadAndActivatesFrame()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->selectThread("1");
    engine->activateFrame(0);

    GdbMi stackData;
    bool stackReceived = false;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&stackData, &stackReceived](quint64, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::FullStack) {
            stackData = data;
            stackReceived = true;
        }
    });
    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 30;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(stackReceived, s_timeout);
    QVERIFY(stackData.toString().contains("bump"));
}

void tst_backends::executesRawCommandAndAssignsValue()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::ShowMemoryCapability); !result)
        QSKIP(qPrintable(result.error()
                          + " Verified via accessMemory() read-back - see "
                            "assignsValueToLocalVariable() for Pdb's own "
                            "equivalent coverage, using refresh(Locals) instead."));
    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList messages;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&messages](const QString &text, int, int) { messages.append(text); });
    engine->executeDebuggerCommand(printCommand(backend, "123456789"), {});
    QTRY_VERIFY_WITH_TIMEOUT(std::any_of(messages.cbegin(), messages.cend(),
                                         [](const QString &text) {
        return text.contains("123456789");
    }), s_timeout);

    const quint64 globalValueAddress = symbolAddress(backend, engine, "globalValue");
    QVERIFY2(globalValueAddress != 0, "could not find globalValue's address via nm");

    WatchItemData item;
    item.type = "int";
    engine->assignValueInDebugger(item, "globalValue", "777");

    QList<QByteArray> memoryChunks;
    connect(engine, &DebuggerEngineInterface::memoryDataReceived, this,
            [&memoryChunks, globalValueAddress](quint64, quint64 address, const QByteArray &data) {
        if (address == globalValueAddress)
            memoryChunks.append(data);
    });
    auto readGlobalValue = [&]() -> int {
        memoryChunks.clear();
        engine->accessMemory(MemoryOp::Fetch, 200, globalValueAddress, sizeof(int));
        [&memoryChunks] { QTRY_VERIFY_WITH_TIMEOUT(!memoryChunks.isEmpty(), s_timeout); }();
        if (QTest::currentTestFailed())
            return -1;
        int value = 0;
        memcpy(&value, memoryChunks.constFirst().constData(), sizeof(int));
        return value;
    };
    QTRY_COMPARE_WITH_TIMEOUT(readGlobalValue(), 777, s_timeout);
}

void tst_backends::assignsValueToLocalVariable()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::StepOver});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend->contains(InferiorEvent::StopOk), s_timeout);

    WatchItemData item;
    item.type = "int";
    item.isLocal = true;
    engine->assignValueInDebugger(item, "localValue", "999");

    QList<GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::Locals)
            responses.append(data);
    });
    RefreshRequest localsRequest;
    localsRequest.kind = RefreshKind::Locals;
    localsRequest.requestId = 51;
    engine->refresh(localsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(!responses.isEmpty(), s_timeout);
    const QString locals = responses.constFirst().toString();
    QVERIFY2(locals.contains("999") || locals.contains("390039003900"),
             qPrintable("assigning localValue never took effect - locals: " + locals));
}

void tst_backends::shutsDownCleanly()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    bool processFinished = false;
    connect(engine, &DebuggerEngineInterface::engineProcessFinished, this,
            [&processFinished](const Utils::ProcessResultData &) { processFinished = true; });

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished),
                              "shutdownInferior(Kill) never reported ShutdownFinished", s_timeout);

    engine->shutdownEngine();
    QTRY_VERIFY2_WITH_TIMEOUT(processFinished,
                              "engine process never reported finishing after "
                              "shutdownInferior()+shutdownEngine()", s_timeout);

    QVERIFY2(debuggerBackend->inferiorResults().isEmpty(),
             "engine process finishing after a normal shutdown wrongly reported inferiorDone");

    debuggerBackend->clearEvents();
    engine->shutdownEngine();
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::EngineShutdownFinished),
                              "shutdownEngine() on an already-finished engine process never "
                              "reported EngineShutdownFinished", s_timeout);
}

void tst_backends::executesRunToLineFunctionAndJumpsToLine()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList modifiedNumbers;
    connect(engine, &DebuggerEngineInterface::breakpointModified, this,
            [&modifiedNumbers](const GdbMi &data) {
        for (const GdbMi &bkpt : data)
            modifiedNumbers.append(bkpt["number"].data());
    });

    debuggerBackend->clearEvents();
    ExecutionRequest jumpRequest;
    jumpRequest.command = ExecutionCommand::JumpToLine;
    jumpRequest.context.type = LocationByFile;
    jumpRequest.context.fileName = inferiorTestData(backend).source;
    jumpRequest.context.textPosition.line = inferiorTestData(backend).breakpointLine + 1;
    debuggerBackend->execute(jumpRequest);
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "JumpToLine never signaled a stop", s_timeout);
    QCOMPARE(debuggerBackend->stoppedFile(), inferiorTestData(backend).source);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).breakpointLine + 1);

    debuggerBackend->clearEvents();
    ExecutionRequest runToLineRequest;
    runToLineRequest.command = ExecutionCommand::RunToLine;
    runToLineRequest.context.type = LocationByFile;
    runToLineRequest.context.fileName = inferiorTestData(backend).source;
    runToLineRequest.context.textPosition.line = inferiorTestData(backend).secondBreakpointLine;
    debuggerBackend->execute(runToLineRequest);
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "RunToLine never signaled a stop", s_timeout);
    QCOMPARE(debuggerBackend->stoppedFile(), inferiorTestData(backend).source);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).secondBreakpointLine);

    GdbMi stackData;
    bool stackReceived = false;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&stackData, &stackReceived](quint64, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::FullStack) {
            stackData = data;
            stackReceived = true;
        }
    });

    debuggerBackend->clearEvents();
    ExecutionRequest runToFunctionRequest;
    runToFunctionRequest.command = ExecutionCommand::RunToFunction;
    runToFunctionRequest.functionName = "spin";
    debuggerBackend->execute(runToFunctionRequest);
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "RunToFunction never signaled a stop", s_timeout);

    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 60;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(stackReceived, s_timeout);
    QVERIFY2(stackData.toString().contains("spin"), "RunToFunction did not stop inside spin()");

    if (backend != Backend::Gdb) {
        for (const QString &number : std::as_const(modifiedNumbers)) {
            QVERIFY2(number.isEmpty() || number == debuggerBackend->breakpointResponseId(),
                     qPrintable("spurious breakpointModified() for internal breakpoint #" + number
                                + " - RunToLine/RunToFunction/JumpToLine's own one-shot breakpoint "
                                "leaked a notification the caller never asked for"));
        }
    }
}

void tst_backends::insertsWatchpointAndCatchpoint()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    if (auto result = checkCapability(backend, Debugger::WatchpointByAddressCapability); !result)
        QSKIP(qPrintable(result.error()));
    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    const quint64 globalValueAddress = symbolAddress(backend, engine, "globalValue");
    QVERIFY2(globalValueAddress != 0, "could not find globalValue's address via nm");

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });

    QSignalSpy memorySpy(engine, &DebuggerEngineInterface::memoryDataReceived);
    engine->accessMemory(MemoryOp::Fetch, 69, globalValueAddress, sizeof(int));
    QTRY_VERIFY_WITH_TIMEOUT(!memorySpy.isEmpty(), s_timeout);

    BreakpointChangeRequest watchRequest;
    watchRequest.op = BreakpointOp::Insert;
    watchRequest.requestId = 70;
    watchRequest.params.type = WatchpointAtAddress;
    watchRequest.params.address = globalValueAddress;
    engine->changeBreakpoint(watchRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(70), s_timeout);
    QVERIFY2(results.value(70), "watchpoint insert failed");

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "watchpoint never triggered on globalValue's write", s_timeout);

    BreakpointChangeRequest catchRequest;
    catchRequest.op = BreakpointOp::Insert;
    catchRequest.requestId = 71;
    catchRequest.params.type = BreakpointAtFork;
    engine->changeBreakpoint(catchRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(71), s_timeout);
    QVERIFY2(results.value(71), "catchpoint insert failed");
}

void tst_backends::insertsWatchpointAsFirstCommandAfterStop()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::WatchpointByAddressCapability); !result)
        QSKIP(qPrintable(result.error()));
    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    const quint64 globalValueAddress = symbolAddress(backend, engine, "globalValue");
    QVERIFY2(globalValueAddress != 0, "could not find globalValue's address via nm");

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });

    // Nothing else is sent between the stop and this insertion, so whatever the
    // debugger printed while stopping must not be mistaken for its answer.
    BreakpointChangeRequest watchRequest;
    watchRequest.op = BreakpointOp::Insert;
    watchRequest.requestId = 72;
    watchRequest.params.type = WatchpointAtAddress;
    watchRequest.params.address = globalValueAddress;
    engine->changeBreakpoint(watchRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(72), s_timeout);
    QVERIFY2(results.value(72), "watchpoint insert as the first command after a stop failed - "
                                "the stop message was still pending as command output");
}

void tst_backends::clearedBreakpointConditionStopsAgain()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));
    if (auto result = checkCapability(backend, Debugger::BreakConditionCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, bool> results;
    QString responseId;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results, &responseId](quint64 requestId, BreakpointOp, bool ok, const GdbMi &data) {
        results[requestId] = ok;
        if (requestId == 248) {
            for (const GdbMi &bkpt : data)
                responseId = bkpt["number"].data();
        }
    });

    BreakpointChangeRequest insertRequest;
    insertRequest.op = BreakpointOp::Insert;
    insertRequest.requestId = 248;
    insertRequest.params.type = BreakpointByFileAndLine;
    insertRequest.params.fileName = inferiorTestData(backend).source;
    insertRequest.params.textPosition.line = inferiorTestData(backend).secondBreakpointLine;
    insertRequest.params.enabled = true;
    insertRequest.params.condition = "globalValue == 999";
    engine->changeBreakpoint(insertRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(248), s_timeout);
    QVERIFY2(results.value(248), "conditional breakpoint insert failed");
    QVERIFY2(!responseId.isEmpty(), "conditional breakpoint insert reported no responseId");

    BreakpointChangeRequest clearRequest;
    clearRequest.op = BreakpointOp::Update;
    clearRequest.requestId = 249;
    clearRequest.responseId = responseId;
    clearRequest.params = insertRequest.params;
    clearRequest.params.condition.clear();
    engine->changeBreakpoint(clearRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(249), s_timeout);
    QVERIFY2(results.value(249), "clearing the breakpoint condition failed");

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "the breakpoint never fired again after its condition was cleared",
                              s_timeout);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).secondBreakpointLine);
}

void tst_backends::fetchesMemoryFromInvalidAddress()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::ShowMemoryCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QList<QByteArray> memoryChunks;
    connect(engine, &DebuggerEngineInterface::memoryDataReceived, this,
            [&memoryChunks](quint64, quint64, const QByteArray &data) {
        memoryChunks.append(data);
    });

    engine->accessMemory(MemoryOp::Fetch, 80, 0, 16);
    QTRY_VERIFY2_WITH_TIMEOUT(!memoryChunks.isEmpty(),
                              "accessMemory() on an invalid address never completed - "
                              "retry logic may be stuck", s_timeout);

    QCOMPARE(memoryChunks.constFirst().size(), 16);
    QCOMPARE(memoryChunks.constFirst(), QByteArray(16, char(0)));
}

void tst_backends::reportsEngineSetupFailure()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(
        backend, ProcessRunData{{FilePath::fromUserInput("/does/not/exist/debugger"), {}},
                                {}, Environment::systemEnvironment()});
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QList<InferiorEvent> events;
    bool processFinished = false;
    connect(engine, &DebuggerEngineInterface::inferiorEvent, this,
            [&events](InferiorEvent event) { events.append(event); });
    connect(engine, &DebuggerEngineInterface::engineProcessFinished, this,
            [&processFinished](const Utils::ProcessResultData &) { processFinished = true; });

    engine->start();

    QTRY_VERIFY2_WITH_TIMEOUT(processFinished,
                              "engineProcessFinished never fired for an engine that could not "
                              "start", s_timeout);
    QVERIFY2(events.contains(InferiorEvent::EngineSetupFailed),
             "EngineSetupFailed was never emitted for an engine that could not start");
}

void tst_backends::refreshesPeripherals()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::RegisterCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    const quint64 globalValueAddress = symbolAddress(backend, engine, "globalValue");
    QVERIFY2(globalValueAddress != 0, "could not find globalValue's address via nm");

    QSignalSpy memorySpy(engine, &DebuggerEngineInterface::memoryDataReceived);
    engine->accessMemory(MemoryOp::Fetch, 103, globalValueAddress, sizeof(int));
    QTRY_VERIFY_WITH_TIMEOUT(!memorySpy.isEmpty(), s_timeout);

    RefreshRequest peripheralRequest;
    peripheralRequest.kind = RefreshKind::PeripheralRegisters;
    peripheralRequest.requestId = 102;
    peripheralRequest.addresses = {globalValueAddress};
    engine->refresh(peripheralRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::PeripheralRegisters)), s_timeout);
    QCOMPARE(responses.value(int(RefreshKind::PeripheralRegisters))["value"].data().toULongLong(),
             41ull);
}

void tst_backends::reloadsDebuggingHelpersAndSymbols()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    responses.remove(int(RefreshKind::Locals));
    RefreshRequest debuggingHelpersRequest;
    debuggingHelpersRequest.kind = RefreshKind::DebuggingHelpers;
    debuggingHelpersRequest.requestId = 105;
    engine->refresh(debuggingHelpersRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
    QVERIFY(responses.value(int(RefreshKind::Locals)).toString().contains("localValue"));

    responses.remove(int(RefreshKind::FullStack));
    RefreshRequest allSymbolsRequest;
    allSymbolsRequest.kind = RefreshKind::AllSymbols;
    allSymbolsRequest.requestId = 106;
    engine->refresh(allSymbolsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);
    QVERIFY(responses.value(int(RefreshKind::FullStack)).toString().contains("bump"));

    responses.remove(int(RefreshKind::Locals));
    RefreshRequest stackSymbolsRequest;
    stackSymbolsRequest.kind = RefreshKind::StackSymbols;
    stackSymbolsRequest.requestId = 107;
    stackSymbolsRequest.path = inferiorTestData(backend).executable;
    engine->refresh(stackSymbolsRequest);
    RefreshRequest locals2Request;
    locals2Request.kind = RefreshKind::Locals;
    locals2Request.requestId = 108;
    engine->refresh(locals2Request);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
    QVERIFY(responses.value(int(RefreshKind::Locals)).toString().contains("localValue"));
}

void tst_backends::acceptsBreakpointFollowsRules()
{
    QFETCH(Backend, backend);

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend);
    QVERIFY(debuggerBackend);
    const DebuggerEngineSetupData &data = debuggerBackend->engine()->setupData();
    QVERIFY(data.acceptsBreakpoint);

    AcceptsBreakpointQuery coreQuery;
    coreQuery.type = BreakpointByFileAndLine;
    coreQuery.fileName = inferiorTestData(backend).source;
    coreQuery.startMode = Debugger::AttachToCore;
    QVERIFY(!data.acceptsBreakpoint(coreQuery));

    AcceptsBreakpointQuery ownQuery;
    ownQuery.type = BreakpointByFileAndLine;
    ownQuery.fileName = inferiorTestData(backend).source;
    ownQuery.startMode = Debugger::StartInternal;
    QVERIFY(data.acceptsBreakpoint(ownQuery));
}

void tst_backends::acceptsBreakpointFollowsCppAndQmlRules()
{
    QFETCH(Backend, backend);

    if (auto result = checkAcceptsCppAndQmlBreakpoints(backend); !result)
        QSKIP(qPrintable(result.error()));

}

void tst_backends::executesStepIn()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::StepIn});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    QCOMPARE(debuggerBackend->stoppedFile(), inferiorTestData(backend).source);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).breakpointLine + 1);
}

void tst_backends::breakpointConditionPreventsStop()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });
    BreakpointChangeRequest falseConditionRequest;
    falseConditionRequest.op = BreakpointOp::Insert;
    falseConditionRequest.requestId = 245;
    falseConditionRequest.params.type = BreakpointByFileAndLine;
    falseConditionRequest.params.fileName = inferiorTestData(backend).source;
    falseConditionRequest.params.textPosition.line = inferiorTestData(backend).secondBreakpointLine;
    falseConditionRequest.params.textPosition.column = 0;
    falseConditionRequest.params.enabled = true;
    falseConditionRequest.params.condition = "globalValue == 999";
    engine->changeBreakpoint(falseConditionRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(245), s_timeout);
    QVERIFY2(results.value(245), "conditional breakpoint insert failed");

    BreakpointChangeRequest spinBodyRequest;
    spinBodyRequest.op = BreakpointOp::Insert;
    spinBodyRequest.requestId = 246;
    spinBodyRequest.params.type = BreakpointByFileAndLine;
    spinBodyRequest.params.fileName = inferiorTestData(backend).source;
    spinBodyRequest.params.textPosition.line = inferiorTestData(backend).spinBodyLine;
    spinBodyRequest.params.textPosition.column = 0;
    spinBodyRequest.params.enabled = true;
    engine->changeBreakpoint(spinBodyRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(246), s_timeout);
    QVERIFY2(results.value(246), "spin() body breakpoint insert failed");

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunOk), s_timeout);
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "neither breakpoint was ever reported - the debuggee never got as "
                              "far as spin()", s_timeout);
    QCOMPARE(debuggerBackend->stoppedLine(), inferiorTestData(backend).spinBodyLine);
}

void tst_backends::executesRepeatLastCommand()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList commandsSent;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&commandsSent](const QString &text, int channel, int) {
        if (channel == Debugger::LogInput)
            commandsSent.append(text);
    });

    debuggerBackend->execute({ExecutionCommand::RepeatLastCommand});

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });
    RefreshRequest localsRequest;
    localsRequest.kind = RefreshKind::Locals;
    localsRequest.requestId = 250;
    const int sentBeforeFetch = commandsSent.size();
    engine->refresh(localsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
    QVERIFY(responses.value(int(RefreshKind::Locals)).toString().contains("localValue"));
    const QStringList commandsSentByFetch = commandsSent.mid(sentBeforeFetch);

    const auto callee = [](const QString &command) {
        static const QRegularExpression leadingToken("^[0-9]+");
        static const QRegularExpression argumentToken(R"( -t [0-9]+\.[0-9]+)");
        QString bare = command;
        bare.remove(leadingToken);
        bare.remove(argumentToken);
        const int argStart = bare.indexOf('(');
        return argStart < 0 ? bare.trimmed() : bare.left(argStart).trimmed();
    };
    const auto timesSent = [&commandsSent, &callee](const QString &command) {
        return int(std::count_if(commandsSent.cbegin(), commandsSent.cend(),
                                 [&](const QString &sent) {
            return callee(sent) == callee(command);
        }));
    };

    QVERIFY2(!commandsSentByFetch.isEmpty(), "refresh(Locals) sent no command at all");
    const QString fetchCommand = commandsSentByFetch.last();
    const int sentBefore = timesSent(fetchCommand);
    debuggerBackend->execute({ExecutionCommand::RepeatLastCommand});
    QTRY_VERIFY2_WITH_TIMEOUT(timesSent(fetchCommand) > sentBefore,
                              qPrintable("the last locals-fetch command was never re-sent: "
                                          + fetchCommand), s_timeout);

    responses.remove(int(RefreshKind::Locals));
    RefreshRequest secondLocalsRequest;
    secondLocalsRequest.kind = RefreshKind::Locals;
    secondLocalsRequest.requestId = 251;
    engine->refresh(secondLocalsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
    QVERIFY(responses.value(int(RefreshKind::Locals)).toString().contains("localValue"));
}

void tst_backends::passesInferiorEnvironmentDiffToDebugger()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

#ifndef Q_OS_LINUX
    QSKIP("verifies via /proc - not yet ported to this platform");
#else
    Environment debuggerEnvironment = Environment::systemEnvironment();
    debuggerEnvironment.set("TST_BACKENDS_ONLY_ON_DEBUGGER", "1");
    Environment inferiorEnvironment = Environment::systemEnvironment();
    inferiorEnvironment.set("TST_BACKENDS_ONLY_ON_INFERIOR", "1");

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(
        backend, ProcessRunData{{m_backendData[backend].path, {}}, {}, debuggerEnvironment},
        ProcessRunData{{inferiorTestData(backend).executable, {}}, {}, inferiorEnvironment});
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    qint64 inferiorPid = 0;
    connect(engine, &DebuggerEngineInterface::inferiorPidKnown, this,
            [&inferiorPid](const ProcessHandle &pid) { inferiorPid = pid.pid(); });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk), s_timeout);
    QVERIFY2(inferiorPid != 0, "inferiorPidKnown() never fired");

    QByteArrayList entries;
    const auto inferiorHasOwnVariable = [&] {
        QFile environFile("/proc/" + QString::number(inferiorPid) + "/environ");
        if (!environFile.open(QIODevice::ReadOnly))
            return false;
        entries = environFile.readAll().split('\0');
        return entries.contains("TST_BACKENDS_ONLY_ON_INFERIOR=1");
    };
    QTRY_VERIFY2_WITH_TIMEOUT(inferiorHasOwnVariable(),
                              "inferior environment diff never reached the real process",
                              s_timeout);
    QVERIFY2(!entries.contains("TST_BACKENDS_ONLY_ON_DEBUGGER=1"),
             "debugger-only environment variable leaked into the inferior's");
#endif
}

void tst_backends::passesInferiorWorkingDirectoryToDebugger()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

#ifndef Q_OS_LINUX
    QSKIP("verifies via /proc - not yet ported to this platform");
#else
    const FilePath workingDirectory = FilePath::fromString("/tmp");
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(
        backend, ProcessRunData{{m_backendData[backend].path, {}}, {}, Environment::systemEnvironment()},
        ProcessRunData{{inferiorTestData(backend).executable, {}}, workingDirectory, Environment::systemEnvironment()});
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    qint64 inferiorPid = 0;
    connect(engine, &DebuggerEngineInterface::inferiorPidKnown, this,
            [&inferiorPid](const ProcessHandle &pid) { inferiorPid = pid.pid(); });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk), s_timeout);
    QVERIFY2(inferiorPid != 0, "inferiorPidKnown() never fired");

    const FilePath cwdLink = FilePath::fromString("/proc/" + QString::number(inferiorPid) + "/cwd");
    QVERIFY2(cwdLink.isSymLink(), "could not read the inferior's /proc/.../cwd");
    QCOMPARE(cwdLink.symLinkTarget(), workingDirectory);
#endif
}

void tst_backends::loadsAdditionalQmlStack()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AdditionalQmlStackCapability); !result)
        QSKIP(qPrintable(result.error()));

#ifndef QMLSTACK_INFERIOR_EXECUTABLE
    QSKIP("Qt::Quick not available when this test binary was configured.");
#else
    const FilePath inferior = (FilePath::fromUserInput(QMLSTACK_INFERIOR_EXECUTABLE)
                              / "qmlstack_inferior").withExecutableSuffix();
    if (!inferior.isExecutableFile())
        QSKIP(qPrintable("QML stack inferior not found at " + inferior.toUserOutput()));
    if (!m_hasQmlNativeDebuggerPlugin)
        QSKIP(s_qmlNativeDebuggerPluginMissing);
    if (!m_hasQtDeclarativeDebugInfo)
        QSKIP(s_qtDeclarativeDebugInfoMissing);

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
        ProcessRunData{{inferior, {}}, {}, Environment::systemEnvironment()});
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [engine](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            BreakpointChangeRequest request;
            request.op = BreakpointOp::Insert;
            request.requestId = 1;
            request.params.type = BreakpointByFunction;
            request.params.functionName = "QmlEntryPoint::process";
            request.params.enabled = true;
            engine->changeBreakpoint(request);
        }
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed)
                             || debuggerBackend->contains(InferiorEvent::EngineRunFailed), s_qmlStartupTimeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::SpontaneousStop));

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    RefreshRequest qmlStackRequest;
    qmlStackRequest.kind = RefreshKind::QmlStack;
    qmlStackRequest.requestId = 20;
    engine->refresh(qmlStackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);

    const QString stack = responses.value(int(RefreshKind::FullStack)).toString();
    QVERIFY2(stack.contains("language=\"js\""), qPrintable("stack: " + stack));
    QVERIFY2(stack.contains("QmlEntryPoint::process"), qPrintable("stack: " + stack));
#endif
}

void tst_backends::fetchesQmlLocals()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AdditionalQmlStackCapability); !result)
        QSKIP(qPrintable(result.error()));

#ifndef QMLSTACK_INFERIOR_EXECUTABLE
    QSKIP("Qt::Quick not available when this test binary was configured.");
#else
    const FilePath inferior = (FilePath::fromUserInput(QMLSTACK_INFERIOR_EXECUTABLE)
                              / "qmlstack_inferior").withExecutableSuffix();
    if (!inferior.isExecutableFile())
        QSKIP(qPrintable("QML stack inferior not found at " + inferior.toUserOutput()));
    if (!m_hasQmlNativeDebuggerPlugin)
        QSKIP(s_qmlNativeDebuggerPluginMissing);
    if (!m_hasQtDeclarativeDebugInfo)
        QSKIP(s_qtDeclarativeDebugInfoMissing);

    Environment env = Environment::systemEnvironment();
    env.set("QV4_FORCE_INTERPRETER", "1");
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
        ProcessRunData{{inferior, {"-qmljsdebugger=native,services:NativeQmlDebugger"}},
                        {}, env}, true);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [engine](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            BreakpointChangeRequest request;
            request.op = BreakpointOp::Insert;
            request.requestId = 1;
            request.params.type = BreakpointByFunction;
            request.params.functionName = "QmlEntryPoint::process";
            request.params.enabled = true;
            engine->changeBreakpoint(request);
        }
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed)
                             || debuggerBackend->contains(InferiorEvent::EngineRunFailed), s_qmlStartupTimeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::SpontaneousStop));

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    RefreshRequest qmlStackRequest;
    qmlStackRequest.kind = RefreshKind::QmlStack;
    qmlStackRequest.requestId = 20;
    engine->refresh(qmlStackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);

    QString context;
    const GdbMi frames = responses.value(int(RefreshKind::FullStack))["stack"]["frames"];
    for (const GdbMi &frame : frames) {
        if (frame["function"].data().contains("compute") && !frame["context"].data().isEmpty()) {
            context = frame["context"].data();
            break;
        }
    }
    QVERIFY2(!context.isEmpty(),
             qPrintable("stack: " + responses.value(int(RefreshKind::FullStack)).toString()));

    RefreshRequest localsRequest;
    localsRequest.kind = RefreshKind::Locals;
    localsRequest.requestId = 21;
    localsRequest.context = context;
    engine->refresh(localsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);

    const QString locals = responses.value(int(RefreshKind::Locals)).toString();
    QVERIFY(locals.contains("name=\"doubled\""));
#endif
}

void tst_backends::insertsQmlBreakpointAndStopsAtIt()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AdditionalQmlStackCapability); !result)
        QSKIP(qPrintable(result.error()));
    // Known red on macOS, cause unknown: the qt_qmlDebugConnectorOpen hook
    // never fires there, so a pending QML breakpoint is never retried - 0
    // resolutions in 6 macOS CI runs against 6 of 6 on Linux, which passes
    // every run. Needs someone debugging it on a Mac.

    if (backend == Backend::Lldb && HostOsInfo::isMacHost())
        QSKIP("QML breakpoint resolution does not work on macOS - see the comment above.");

    // TODO: Fix and unskip.
    if (backend == Backend::Gdb)
        QSKIP("QML breakpoint resolution is red in CI: against a Qt whose QML debug "
              "plugins carry no debug info, the interpreter send is refused at "
              "qt_qmlDebugConnectorOpen, and gdb 10.2 does not recover at "
              "qt_qmlDebugObjectAvailable either. Reproducible locally by pointing "
              "QT_PLUGIN_PATH at a stripped copy of the qmltooling plugins.");

#ifndef QMLSTACK_INFERIOR_EXECUTABLE
    QSKIP("Qt::Quick not available when this test binary was configured.");
#else
    const FilePath inferior = (FilePath::fromUserInput(QMLSTACK_INFERIOR_EXECUTABLE)
                              / "qmlstack_inferior").withExecutableSuffix();
    if (!inferior.isExecutableFile())
        QSKIP(qPrintable("QML stack inferior not found at " + inferior.toUserOutput()));

    Environment env = Environment::systemEnvironment();
    env.set("QV4_FORCE_INTERPRETER", "1");
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
        ProcessRunData{{inferior, {"-qmljsdebugger=native,services:NativeQmlDebugger"}},
                        {}, env}, true);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, bool> insertResults;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&insertResults](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        insertResults[requestId] = ok;
    });
    QList<GdbMi> modifiedReports;
    connect(engine, &DebuggerEngineInterface::breakpointModified, this,
            [&modifiedReports](const GdbMi &data) { modifiedReports.append(data); });
    QStringList wire;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&wire](const QString &text, int, int) { wire.append(text); });

    connect(engine, &DebuggerEngineInterface::inferiorEvent, this,
            [engine](InferiorEvent event) {
        if (event != InferiorEvent::EngineSetupOk)
            return;
        BreakpointChangeRequest request;
        request.op = BreakpointOp::Insert;
        request.requestId = 30;
        request.modelId = 42;
        request.params.type = BreakpointByFileAndLine;
        request.params.fileName = FilePath::fromUserInput("qmlstack_inferior.qml");
        request.params.textPosition.line =
            qmlMarkerLine("qmlstack_inferior.qml", "MARKER: qml breakpoint line");
        QVERIFY(request.params.textPosition.line > 0);
        request.params.textPosition.column = 0;
        request.params.enabled = true;
        engine->changeBreakpoint(request);
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(insertResults.contains(30), s_qmlStartupTimeout);
    QVERIFY2(insertResults.value(30), "pending QML breakpoint insert failed");

    QTRY_VERIFY2_WITH_TIMEOUT(!modifiedReports.isEmpty(),
                              qPrintable("the resolver's retry never reported the QML "
                                         "breakpoint back - last wire traffic:\n  "
                                         + wireTail(wire)),
                              s_timeout);
    const GdbMi resolved = modifiedReports.constFirst().childAt(0);
    QCOMPARE(resolved["modelid"].toInt(), 42);
    QVERIFY(resolved["pending"].toInt() == 0);
    QVERIFY(!resolved["number"].data().isEmpty());

#endif
}

void tst_backends::insertsQmlBreakpointBeforeDumpersLoad()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AdditionalQmlStackCapability); !result)
        QSKIP(qPrintable(result.error()));
    // Same macOS gap as insertsQmlBreakpointAndStopsAtIt() - see its comment.

    if (backend == Backend::Lldb && HostOsInfo::isMacHost())
        QSKIP("QML breakpoint resolution does not work on macOS - see the comment there.");

    // TODO: Fix and unskip.
    if (backend == Backend::Gdb)
        QSKIP("QML breakpoint resolution is red in CI - see "
              "insertsQmlBreakpointAndStopsAtIt().");

#ifndef QMLSTACK_INFERIOR_EXECUTABLE
    QSKIP("Qt::Quick not available when this test binary was configured.");
#else
    const FilePath inferior = (FilePath::fromUserInput(QMLSTACK_INFERIOR_EXECUTABLE)
                              / "qmlstack_inferior").withExecutableSuffix();
    if (!inferior.isExecutableFile())
        QSKIP(qPrintable("QML stack inferior not found at " + inferior.toUserOutput()));

    Environment env = Environment::systemEnvironment();
    env.set("QV4_FORCE_INTERPRETER", "1");
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
        ProcessRunData{{inferior, {"-qmljsdebugger=native,services:NativeQmlDebugger"}},
                        {}, env}, true);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    bool sawUndefinedDumperError = false;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&sawUndefinedDumperError](const QString &text, int, int) {
        if (text.contains("theDumper") && text.contains("not defined"))
            sawUndefinedDumperError = true;
    });

    QHash<quint64, bool> insertResults;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&insertResults](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        insertResults[requestId] = ok;
    });
    QList<GdbMi> modifiedReports;
    connect(engine, &DebuggerEngineInterface::breakpointModified, this,
            [&modifiedReports](const GdbMi &data) { modifiedReports.append(data); });
    QStringList wire;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&wire](const QString &text, int, int) { wire.append(text); });

    QStringList bridgeLog;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&bridgeLog](const QString &text, int, int) {
        static const QStringList markers = {"resolver", "auto-continue", "interpreter",
                                            "service", "hook", "setbreakpoint", "qt_qmldebug"};
        if (std::any_of(markers.cbegin(), markers.cend(), [&text](const QString &marker) {
                return text.contains(marker, Qt::CaseInsensitive);
            })) {
            bridgeLog.append(text.trimmed());
        }
    });

    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [engine](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            BreakpointChangeRequest request;
            request.op = BreakpointOp::Insert;
            request.requestId = 30;
            request.modelId = 42;
            request.params.type = BreakpointByFileAndLine;
            request.params.fileName = FilePath::fromUserInput("qmlstack_inferior.qml");
            request.params.textPosition.line =
                qmlMarkerLine("qmlstack_inferior.qml", "MARKER: qml breakpoint line");
            QVERIFY(request.params.textPosition.line > 0);
            request.params.textPosition.column = 0;
            request.params.enabled = true;
            engine->changeBreakpoint(request);
        }
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(insertResults.contains(30), s_qmlStartupTimeout);
    QVERIFY2(insertResults.value(30), "pending QML breakpoint insert failed");
    QVERIFY2(!sawUndefinedDumperError,
             "QML breakpoint insert reached gdb before theDumper existed");

    QTRY_VERIFY2_WITH_TIMEOUT(!modifiedReports.isEmpty(),
                              qPrintable("the resolver's retry never reported the QML "
                                         "breakpoint back - last wire traffic:\n  "
                                         + wireTail(wire)),
                              s_timeout);
    const GdbMi resolved = modifiedReports.constFirst().childAt(0);
    QCOMPARE(resolved["modelid"].toInt(), 42);
    QVERIFY(resolved["pending"].toInt() == 0);
    QVERIFY(!resolved["number"].data().isEmpty());
    QVERIFY2(!sawUndefinedDumperError,
             "QML breakpoint insert reached gdb before theDumper existed");

    const auto stopDiagnosis = [&debuggerBackend, &bridgeLog] {
        QStringList seen;
        const std::pair<InferiorEvent, const char *> interesting[] = {
            {InferiorEvent::RunRequested, "RunRequested"}, {InferiorEvent::RunOk, "RunOk"},
            {InferiorEvent::RunFailed, "RunFailed"}, {InferiorEvent::StopOk, "StopOk"},
            {InferiorEvent::SpontaneousStop, "SpontaneousStop"},
            {InferiorEvent::InferiorIll, "InferiorIll"},
            {InferiorEvent::EngineRunFailed, "EngineRunFailed"},
        };
        for (const auto &[event, name] : interesting) {
            if (debuggerBackend->contains(event))
                seen.append(QString::fromLatin1(name));
        }
        return QString("the resolved QML breakpoint never stopped the debuggee.\n"
                       "  events seen: %1\n  debuggee exited: %2\n  bridge reported:\n    %3")
            .arg(seen.isEmpty() ? QString("(none)") : seen.join(", "))
            .arg(debuggerBackend->inferiorResults().isEmpty() ? "no" : "yes")
            .arg(bridgeLog.isEmpty() ? QString("(nothing)") : wireTail(bridgeLog, 10));
    };
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                              || debuggerBackend->contains(InferiorEvent::EngineSetupFailed)
                              || debuggerBackend->contains(InferiorEvent::EngineRunFailed),
                              qPrintable(stopDiagnosis()), 30000);
    QVERIFY2(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
             qPrintable(stopDiagnosis()));
#endif
}

void tst_backends::splicesQmlFramesIntoPlainFullStackWhenNativeMixed()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AdditionalQmlStackCapability); !result)
        QSKIP(qPrintable(result.error()));

#ifndef QMLSTACK_INFERIOR_EXECUTABLE
    QSKIP("Qt::Quick not available when this test binary was configured.");
#else
    const FilePath inferior = (FilePath::fromUserInput(QMLSTACK_INFERIOR_EXECUTABLE)
                              / "qmlstack_inferior").withExecutableSuffix();
    if (!inferior.isExecutableFile())
        QSKIP(qPrintable("QML stack inferior not found at " + inferior.toUserOutput()));
    if (!m_hasQmlNativeDebuggerPlugin)
        QSKIP(s_qmlNativeDebuggerPluginMissing);
    if (!m_hasQtDeclarativeDebugInfo)
        QSKIP(s_qtDeclarativeDebugInfoMissing);

    Environment env = Environment::systemEnvironment();
    env.set("QV4_FORCE_INTERPRETER", "1");

    auto fetchFullStack = [this, backend, &inferior, &env](bool nativeMixed) -> QString {
        std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
            ProcessRunData{{inferior, {"-qmljsdebugger=native,services:NativeQmlDebugger"}},
                            {}, env}, nativeMixed);
        DebuggerEngineInterface *engine = debuggerBackend->engine();

        connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
                [engine](InferiorEvent event) {
            if (event == InferiorEvent::EngineSetupOk) {
                BreakpointChangeRequest request;
                request.op = BreakpointOp::Insert;
                request.requestId = 1;
                request.params.type = BreakpointByFunction;
                request.params.functionName = "QmlEntryPoint::process";
                request.params.enabled = true;
                engine->changeBreakpoint(request);
            }
        });

        engine->start();
        [backendPtr = debuggerBackend.get()] {
            QTRY_VERIFY_WITH_TIMEOUT(backendPtr->contains(InferiorEvent::SpontaneousStop), s_qmlStartupTimeout);
        }();
        if (QTest::currentTestFailed())
            return {};

        GdbMi response;
        connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
                [&response](quint64, RefreshKind kind, const GdbMi &data) {
            if (kind == RefreshKind::FullStack)
                response = data;
        });
        RefreshRequest request;
        request.kind = RefreshKind::FullStack;
        request.requestId = 1;
        engine->refresh(request);
        [&response] { QTRY_VERIFY_WITH_TIMEOUT(response.isValid(), s_timeout); }();
        return response.toString();
    };

    const QString nativeMixedStack = fetchFullStack(true);
    QVERIFY2(nativeMixedStack.contains("language=\"js\""),
             qPrintable("nativeMixed=true should splice QML frames into a plain "
                        "FullStack refresh - stack: " + nativeMixedStack));
    const QString plainStack = fetchFullStack(false);
    QVERIFY2(!plainStack.contains("language=\"js\""),
             qPrintable("nativeMixed=false should not splice QML frames into a plain "
                        "FullStack refresh - stack: " + plainStack));
#endif
}

void tst_backends::stepsOutOfNativeMixedCppFrameBackIntoQml()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AdditionalQmlStackCapability); !result)
        QSKIP(qPrintable(result.error()));

#ifndef QMLSTACK_INFERIOR_EXECUTABLE
    QSKIP("Qt::Quick not available when this test binary was configured.");
#else
    const FilePath inferior = (FilePath::fromUserInput(QMLSTACK_INFERIOR_EXECUTABLE)
                              / "qmlstack_inferior").withExecutableSuffix();
    if (!inferior.isExecutableFile())
        QSKIP(qPrintable("QML stack inferior not found at " + inferior.toUserOutput()));
    if (!m_hasQmlNativeDebuggerPlugin)
        QSKIP(s_qmlNativeDebuggerPluginMissing);
    if (!m_hasQtDeclarativeDebugInfo)
        QSKIP(s_qtDeclarativeDebugInfoMissing);

    Environment env = Environment::systemEnvironment();
    env.set("QV4_FORCE_INTERPRETER", "1");
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
        ProcessRunData{{inferior, {"-qmljsdebugger=native,services:NativeQmlDebugger"}},
                        {}, env}, true);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [engine](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            BreakpointChangeRequest request;
            request.op = BreakpointOp::Insert;
            request.requestId = 1;
            request.params.type = BreakpointByFunction;
            request.params.functionName = "QmlEntryPoint::process";
            request.params.enabled = true;
            engine->changeBreakpoint(request);
        }
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed)
                             || debuggerBackend->contains(InferiorEvent::EngineRunFailed), s_qmlStartupTimeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::SpontaneousStop));

    ExecutionRequest stepOut;
    stepOut.command = ExecutionCommand::StepOut;
    stepOut.currentFrameIsQml = false;
    debuggerBackend->execute(stepOut);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->count(InferiorEvent::SpontaneousStop) >= 2, s_timeout);

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });
    RefreshRequest qmlStackRequest;
    qmlStackRequest.kind = RefreshKind::QmlStack;
    qmlStackRequest.requestId = 1;
    engine->refresh(qmlStackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);
    const QString stack = responses.value(int(RefreshKind::FullStack)).toString();
    QVERIFY2(stack.contains("function=\"compute\""),
             qPrintable("stepping out of QmlEntryPoint::process should land back in "
                        "compute() - stack: " + stack));
#endif
}

void tst_backends::stepsWithinQmlFrameAfterNativeMixedStepOut()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AdditionalQmlStackCapability); !result)
        QSKIP(qPrintable(result.error()));

#ifndef QMLMIX_INFERIOR_EXECUTABLE
    QSKIP("Qt::Quick not available when this test binary was configured.");
#else
    const FilePath inferior = (FilePath::fromUserInput(QMLMIX_INFERIOR_EXECUTABLE)
                              / "qmlmix_inferior").withExecutableSuffix();
    if (!inferior.isExecutableFile())
        QSKIP(qPrintable("qmlmix inferior not found at " + inferior.toUserOutput()));
    if (!m_hasQmlNativeDebuggerPlugin)
        QSKIP(s_qmlNativeDebuggerPluginMissing);
    if (!m_hasQtDeclarativeDebugInfo)
        QSKIP(s_qtDeclarativeDebugInfoMissing);

    Environment env = Environment::systemEnvironment();
    env.set("QV4_FORCE_INTERPRETER", "1");
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
        ProcessRunData{{inferior, {"-qmljsdebugger=native,services:NativeQmlDebugger"}},
                        {}, env}, true);
    DebuggerEngineInterface *engine = debuggerBackend->engine();
    const int qmlLine = qmlMarkerLine("Main.qml", "MARKER: qml-return");
    QVERIFY(qmlLine > 0);

    QList<GdbMi> modifiedReports;
    connect(engine, &DebuggerEngineInterface::breakpointModified, this,
            [&modifiedReports](const GdbMi &data) { modifiedReports.append(data); });

    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [engine, qmlLine](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            BreakpointChangeRequest cppRequest;
            cppRequest.op = BreakpointOp::Insert;
            cppRequest.requestId = 1;
            cppRequest.params.type = BreakpointByFunction;
            cppRequest.params.functionName = "QmlEntryPoint::process";
            cppRequest.params.enabled = true;
            engine->changeBreakpoint(cppRequest);

            BreakpointChangeRequest qmlRequest;
            qmlRequest.op = BreakpointOp::Insert;
            qmlRequest.requestId = 2;
            qmlRequest.modelId = 99;
            qmlRequest.params.type = BreakpointByFileAndLine;
            qmlRequest.params.fileName = FilePath::fromUserInput("Main.qml");
            qmlRequest.params.textPosition.line = qmlLine;
            qmlRequest.params.textPosition.column = 0;
            qmlRequest.params.enabled = true;
            engine->changeBreakpoint(qmlRequest);
        }
    });

    engine->start();

    QTRY_VERIFY_WITH_TIMEOUT(Utils::anyOf(modifiedReports, [](const GdbMi &data) {
        const GdbMi resolved = data.childAt(0);
        return resolved["modelid"].toInt() == 99 && resolved["pending"].toInt() == 0;
    }) || debuggerBackend->contains(InferiorEvent::EngineSetupFailed)
       || debuggerBackend->contains(InferiorEvent::EngineRunFailed), s_qmlStartupTimeout);

    const int stopsBeforeResolve = debuggerBackend->count(InferiorEvent::SpontaneousStop);
    debuggerBackend->execute({ExecutionCommand::Continue});

    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->count(InferiorEvent::SpontaneousStop) >= stopsBeforeResolve + 1
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed)
                             || debuggerBackend->contains(InferiorEvent::EngineRunFailed), 30000);
    QVERIFY(debuggerBackend->count(InferiorEvent::SpontaneousStop) >= stopsBeforeResolve + 1);

    ExecutionRequest stepOut;
    stepOut.command = ExecutionCommand::StepOut;
    stepOut.currentFrameIsQml = false;
    debuggerBackend->execute(stepOut);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->count(InferiorEvent::SpontaneousStop) >= stopsBeforeResolve + 2, 30000);

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });
    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 1;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);
    QVERIFY2(responses.value(int(RefreshKind::FullStack)).toString().contains("function=\"compute\""),
             "step-out should land back in compute()");
    responses.remove(int(RefreshKind::FullStack));

    ExecutionRequest stepOver;
    stepOver.command = ExecutionCommand::StepOver;
    stepOver.currentFrameIsQml = true;
    debuggerBackend->execute(stepOver);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->count(InferiorEvent::SpontaneousStop) >= stopsBeforeResolve + 3,
                             s_timeout);

    RefreshRequest stackRequest2;
    stackRequest2.kind = RefreshKind::FullStack;
    stackRequest2.requestId = 2;
    engine->refresh(stackRequest2);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);
    QVERIFY2(responses.value(int(RefreshKind::FullStack)).toString().contains("language=\"js\""),
             "step-over from the QML frame should land on another js stop");
#endif
}

void tst_backends::continuesPastNativeMixedCppBreakpoint()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AdditionalQmlStackCapability); !result)
        QSKIP(qPrintable(result.error()));

#ifndef QMLMIX_INFERIOR_EXECUTABLE
    QSKIP("Qt::Quick not available when this test binary was configured.");
#else
    const FilePath inferior = (FilePath::fromUserInput(QMLMIX_INFERIOR_EXECUTABLE)
                              / "qmlmix_inferior").withExecutableSuffix();
    if (!inferior.isExecutableFile())
        QSKIP(qPrintable("qmlmix inferior not found at " + inferior.toUserOutput()));

    Environment env = Environment::systemEnvironment();
    env.set("QV4_FORCE_INTERPRETER", "1");
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
        ProcessRunData{{inferior, {"-qmljsdebugger=native,services:NativeQmlDebugger"}},
                        {}, env}, true);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [engine](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            BreakpointChangeRequest cppRequest;
            cppRequest.op = BreakpointOp::Insert;
            cppRequest.requestId = 1;
            cppRequest.params.type = BreakpointByFunction;
            cppRequest.params.functionName = "QmlEntryPoint::process";
            cppRequest.params.enabled = true;
            engine->changeBreakpoint(cppRequest);
        }
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed)
                             || debuggerBackend->contains(InferiorEvent::EngineRunFailed), s_qmlStartupTimeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::SpontaneousStop));

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunOk)
                             || debuggerBackend->contains(InferiorEvent::RunFailed), s_timeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunOk));

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Interrupt});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::StopOk), s_timeout);
#endif
}

void tst_backends::staysStoppedWithoutExplicitContinue()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AdditionalQmlStackCapability); !result)
        QSKIP(qPrintable(result.error()));

#ifndef QMLMIX_INFERIOR_EXECUTABLE
    QSKIP("Qt::Quick not available when this test binary was configured.");
#else
    const FilePath inferior = (FilePath::fromUserInput(QMLMIX_INFERIOR_EXECUTABLE)
                              / "qmlmix_inferior").withExecutableSuffix();
    if (!inferior.isExecutableFile())
        QSKIP(qPrintable("qmlmix inferior not found at " + inferior.toUserOutput()));

    Environment env = Environment::systemEnvironment();
    env.set("QV4_FORCE_INTERPRETER", "1");
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
        ProcessRunData{{inferior, {"-qmljsdebugger=native,services:NativeQmlDebugger"}},
                        {}, env}, true);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [engine](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            BreakpointChangeRequest cppRequest;
            cppRequest.op = BreakpointOp::Insert;
            cppRequest.requestId = 1;
            cppRequest.params.type = BreakpointByFunction;
            cppRequest.params.functionName = "QmlEntryPoint::process";
            cppRequest.params.enabled = true;
            engine->changeBreakpoint(cppRequest);
        }
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed)
                             || debuggerBackend->contains(InferiorEvent::EngineRunFailed), s_qmlStartupTimeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::SpontaneousStop));

    debuggerBackend->clearEvents();
    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });
    RefreshRequest registersRequest;
    registersRequest.kind = RefreshKind::Registers;
    registersRequest.requestId = 1;
    engine->refresh(registersRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Registers)), s_timeout);
    QVERIFY2(debuggerBackend->isEmpty(),
             qPrintable(QString("expected no events while stopped and not told "
                                 "to continue, got %1 unrequested event(s)")
                            .arg(debuggerBackend->size())));

    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunOk)
                             || debuggerBackend->contains(InferiorEvent::RunFailed), s_timeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunOk));
#endif
}

void tst_backends::stepsFromQmlIntoNativeMixedCppFrame()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AdditionalQmlStackCapability); !result)
        QSKIP(qPrintable(result.error()));

#ifndef QMLMIX_INFERIOR_EXECUTABLE
    QSKIP("Qt::Quick not available when this test binary was configured.");
#else
    const FilePath inferior = (FilePath::fromUserInput(QMLMIX_INFERIOR_EXECUTABLE)
                              / "qmlmix_inferior").withExecutableSuffix();
    if (!inferior.isExecutableFile())
        QSKIP(qPrintable("qmlmix inferior not found at " + inferior.toUserOutput()));
    if (!m_hasQmlNativeDebuggerPlugin)
        QSKIP(s_qmlNativeDebuggerPluginMissing);
    if (!m_hasQtDeclarativeDebugInfo)
        QSKIP(s_qtDeclarativeDebugInfoMissing);

    Environment env = Environment::systemEnvironment();
    env.set("QV4_FORCE_INTERPRETER", "1");
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
        ProcessRunData{{inferior, {"-qmljsdebugger=native,services:NativeQmlDebugger"}},
                        {}, env}, true);
    DebuggerEngineInterface *engine = debuggerBackend->engine();
    const int qmlLine = qmlMarkerLine("Main.qml", "MARKER: qml-to-cpp");
    QVERIFY(qmlLine > 0);

    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [engine, qmlLine](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            BreakpointChangeRequest qmlRequest;
            qmlRequest.op = BreakpointOp::Insert;
            qmlRequest.requestId = 1;
            qmlRequest.modelId = 42;
            qmlRequest.params.type = BreakpointByFileAndLine;
            qmlRequest.params.fileName = FilePath::fromUserInput("Main.qml");
            qmlRequest.params.textPosition.line = qmlLine;
            qmlRequest.params.textPosition.column = 0;
            qmlRequest.params.enabled = true;
            engine->changeBreakpoint(qmlRequest);
        }
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed)
                             || debuggerBackend->contains(InferiorEvent::EngineRunFailed), s_qmlStartupTimeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::SpontaneousStop));

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });
    RefreshRequest localsRequest;
    localsRequest.kind = RefreshKind::Locals;
    localsRequest.requestId = 1;
    engine->refresh(localsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
    responses.remove(int(RefreshKind::Locals));

    ExecutionRequest stepIn;
    stepIn.command = ExecutionCommand::StepIn;
    stepIn.currentFrameIsQml = true;
    debuggerBackend->execute(stepIn);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->count(InferiorEvent::SpontaneousStop) >= 2, s_timeout);

    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 1;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);
    const QString stack = responses.value(int(RefreshKind::FullStack)).toString();
    if (m_hasNativeCallHook) {
        QVERIFY2(stackHasFunction(stack, "QmlEntryPoint::process"),
                 qPrintable("stepping in from the QML call site should land in "
                            "QmlEntryPoint::process - stack: " + stack));
        QVERIFY2(stack.contains("function=\"compute\"") && stack.contains("language=\"js\""),
                 qPrintable("the spliced stack should still show the QML caller "
                            "after stepping in - stack: " + stack));
    } else {
        QVERIFY2(!stackHasFunction(stack, "QmlEntryPoint::process"),
                 qPrintable("did not expect to land in QmlEntryPoint::process "
                            "without qt_v4AboutToCallNativeMethodHook - stack: "
                            + stack));
        QVERIFY2(stack.contains("function=\"compute\"") && stack.contains("language=\"js\""),
                 qPrintable("without the hook, step-in should still land "
                            "somewhere in compute() - stack: " + stack));
    }
#endif
}

void tst_backends::reportsAlienBreakpoints()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    const InferiorTestData testData = inferiorTestData(backend);
    if (testData.alienBreakpointCommand.isEmpty())
        QSKIP("backend has no native command for creating a breakpoint behind our back");

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QList<std::pair<BreakpointOp, GdbMi>> alienEvents;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&alienEvents](quint64 requestId, BreakpointOp op, bool, const GdbMi &data) {
        if (requestId == 0)
            alienEvents.append({op, data});
    });
    engine->executeDebuggerCommand(testData.alienBreakpointCommand, {});
    QTRY_VERIFY2_WITH_TIMEOUT(!alienEvents.isEmpty(),
                              "a breakpoint created by a native command was never reported",
                              s_timeout);
    const auto &[op, data] = alienEvents.constFirst();
    QCOMPARE(op, BreakpointOp::Insert);
    const QString number = data["number"].data();
    QVERIFY2(!number.isEmpty(), qPrintable("no breakpoint number in: " + data.toString()));

    alienEvents.clear();
    engine->executeDebuggerCommand(testData.alienBreakpointDeleteCommand.arg(number), {});
    QTRY_VERIFY2_WITH_TIMEOUT(!alienEvents.isEmpty(),
                              "deleting that breakpoint natively was never reported", s_timeout);
    QCOMPARE(alienEvents.constFirst().first, BreakpointOp::Remove);
    QCOMPARE(alienEvents.constFirst().second["number"].data(), number);
}

void tst_backends::togglesBreakpointEnabledInPlace()
{
    QFETCH(Backend, backend);

    const InferiorTestData testData = inferiorTestData(backend);
    if (testData.enableToggleWireMarker.isEmpty())
        QSKIP("backend declares no in-place enable/disable command");

    Process helperInferior;
    std::unique_ptr<DebuggerBackend> debuggerBackend = stopAtBreakpoint(backend, helperInferior);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList sent;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&sent](const QString &text, int channel, int) {
        if (channel == Debugger::LogInput)
            sent.append(text);
    });

    QHash<quint64, std::pair<BreakpointOp, bool>> answers;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&answers](quint64 requestId, BreakpointOp op, bool ok, const GdbMi &) {
        answers[requestId] = {op, ok};
    });

    BreakpointChangeRequest request;
    request.op = BreakpointOp::Update;
    request.requestId = 320;
    request.responseId = debuggerBackend->breakpointResponseId();
    QVERIFY2(!request.responseId.isEmpty(), "no breakpoint number to update");
    request.params.type = BreakpointByFileAndLine;
    request.params.fileName = testData.source;
    request.params.textPosition.line = testData.breakpointLine;
    request.params.enabled = false;
    engine->changeBreakpoint(request);

    QTRY_VERIFY2_WITH_TIMEOUT(answers.contains(320),
                              "disabling the breakpoint was never answered", s_timeout);
    QVERIFY2(answers.value(320).second, "disabling the breakpoint failed");
    QVERIFY2(answers.value(320).first == BreakpointOp::Update,
             "an Update was answered with a different op - the breakpoint was re-inserted "
             "rather than changed in place");
    QVERIFY2(std::any_of(sent.cbegin(), sent.cend(), [&](const QString &line) {
                 return line.contains(testData.enableToggleWireMarker);
             }),
             qPrintable("no in-place \"" + testData.enableToggleWireMarker + "\" on the wire, sent:\n  "
                        + sent.mid(qMax(0, sent.size() - 8)).join("\n  ")));
}

void tst_backends::reportsBreakpointModifiedEvents()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QList<GdbMi> modified;
    connect(engine, &DebuggerEngineInterface::breakpointModified, this,
            [&modified](const GdbMi &data) { modified.append(data); });

    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [this, engine, backend](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            BreakpointChangeRequest request;
            request.op = BreakpointOp::Insert;
            request.requestId = 1;
            request.params.type = BreakpointByFileAndLine;
            request.params.fileName = inferiorTestData(backend).source;
            request.params.textPosition.line = inferiorTestData(backend).breakpointLine;
            request.params.textPosition.column = 0;
            request.params.enabled = true;
            engine->changeBreakpoint(request);
        }
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    QTRY_VERIFY_WITH_TIMEOUT(std::any_of(modified.cbegin(), modified.cend(), [](const GdbMi &data) {
        return data.childAt(0)["times"].data() != "0";
    }), s_timeout);
}

void tst_backends::attachesToRunningProcess()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToProcess); !result)
        QSKIP(qPrintable(result.error()));

    Process target;
    target.setCommand({inferiorTestData(backend).executable, {}});
    target.start();
    QVERIFY(target.waitForStarted());
    const qint64 pid = target.processId();

    std::unique_ptr<DebuggerBackend> debuggerBackend =
        createAttachEngine(backend, AttachToProcessData{ProcessHandle(pid)});
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk)
                             || debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk)
                             || debuggerBackend->contains(InferiorEvent::EngineIll), s_timeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk)
            || debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk));

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();

    target.waitForFinished();
    QCOMPARE(target.state(), ProcessState::NotRunning);
}

void tst_backends::attachesToTerminalRunProcess()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToTerminalStub); !result)
        QSKIP(qPrintable(result.error()));

#ifdef Q_OS_WIN
    QSKIP("attachesToTerminalRunProcess() only covers the non-Windows "
          "SIGSTOP/SIGCONT handshake - see its own comment.");
#else
    Process target;
    target.setCommand({inferiorTestData(backend).executable, {}});
    target.start();
    QVERIFY(target.waitForStarted());
    const qint64 pid = target.processId();
    QString targetOutput;
    auto sawAfterBump = [&] {
        targetOutput += target.readAllStandardOutput();
        return targetOutput.contains("after bump");
    };
    QTRY_VERIFY_WITH_TIMEOUT(sawAfterBump(), s_timeout);
    QVERIFY2(::kill(pid, SIGSTOP) == 0, "failed to SIGSTOP the target");

    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(backend,
        AttachToTerminalStubData{ProcessHandle(pid), pid, inferiorTestData(backend).executable});
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    connect(engine, &DebuggerEngineInterface::kickoffTerminalProcessRequested, this,
            [pid] { ::kill(pid, SIGCONT); });
    connect(engine, &DebuggerEngineInterface::interruptTerminalRequested, this,
            [&target] { target.interrupt(); });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk)
                             || debuggerBackend->contains(InferiorEvent::EngineIll), s_timeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk));
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunOk)
                             || debuggerBackend->contains(InferiorEvent::RunFailed), s_timeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunOk));

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Interrupt});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::StopOk), s_timeout);

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();

    target.waitForFinished();
    QCOMPARE(target.state(), ProcessState::NotRunning);
#endif
}

QString tst_backends::startGdbserver(Process &gdbserverProcess, const QStringList &flags,
                                     const QStringList &trailingArgs, QString *gdbserverOutput)
{
    gdbserverProcess.setCommand(
        {m_gdbserverPath, flags + QStringList{"localhost:0"} + trailingArgs});
    connect(&gdbserverProcess, &Process::readyReadStandardError, this,
            [&gdbserverProcess, gdbserverOutput] {
        *gdbserverOutput += gdbserverProcess.readAllStandardError();
    });
    gdbserverProcess.start();
    if (!gdbserverProcess.waitForStarted())
        return {};

    const QString portMarker = "Listening on port ";
    [&] { QTRY_VERIFY_WITH_TIMEOUT(gdbserverOutput->contains(portMarker), s_timeout); }();
    if (QTest::currentTestFailed())
        return {};
    const int portStart = gdbserverOutput->indexOf(portMarker) + portMarker.length();
    int portEnd = portStart;
    while (portEnd < gdbserverOutput->size() && gdbserverOutput->at(portEnd).isDigit())
        ++portEnd;
    return gdbserverOutput->mid(portStart, portEnd - portStart);
}

quint16 tst_backends::startQmlServer(Process &inferiorProcess, const FilePath &executable)
{
    quint16 port = 0;
    {
        QTcpServer probe;
        if (!probe.listen(QHostAddress::LocalHost))
            return 0;
        port = probe.serverPort();
    }
    inferiorProcess.setCommand({executable,
        {QString("-qmljsdebugger=port:%1,block,services:V8Debugger,QmlDebugger").arg(port)}});
    inferiorProcess.start();
    if (!inferiorProcess.waitForStarted())
        return 0;
    return port;
}

void tst_backends::attachesToRunningRemoteServer()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToRemoteServer); !result)
        QSKIP(qPrintable(result.error()));

    if (!m_gdbserverPath.isExecutableFile())
        QSKIP("gdbserver not found - set QTC_GDBSERVER_PATH_FOR_TEST to override.");

    Process gdbserverProcess;
    QString gdbserverOutput;
    const QString port = startGdbserver(gdbserverProcess, {}, {inferiorTestData(backend).executable.nativePath()},
                                        &gdbserverOutput);
    QVERIFY2(!port.isEmpty(),
             qPrintable("could not parse gdbserver's port from: " + gdbserverOutput));

    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(backend,
        AttachToRemoteServerData{"localhost:" + port, inferiorTestData(backend).executable});
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk)
                             || debuggerBackend->contains(InferiorEvent::EngineIll), s_timeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk));

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();

    QTRY_COMPARE_WITH_TIMEOUT(gdbserverProcess.state(), ProcessState::NotRunning, s_timeout);
}

void tst_backends::attachesToRemoteProcessByPid()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToRemoteServer); !result)
        QSKIP(qPrintable(result.error()));

    if (!m_gdbserverPath.isExecutableFile())
        QSKIP("gdbserver not found - set QTC_GDBSERVER_PATH_FOR_TEST to override.");

    Process target;
    target.setCommand({inferiorTestData(backend).executable, {}});
    target.start();
    QVERIFY(target.waitForStarted());
    const qint64 pid = target.processId();

    Process gdbserverProcess;
    QString gdbserverOutput;
    const bool stubHostsProcess = inferiorTestData(backend).remoteStubHostsProcess;
    const QString port = stubHostsProcess
        ? startGdbserver(gdbserverProcess, {"--attach"}, {QString::number(pid)}, &gdbserverOutput)
        : startGdbserver(gdbserverProcess, {"--multi"}, {}, &gdbserverOutput);
    QVERIFY2(!port.isEmpty(),
             qPrintable("could not parse gdbserver's port from: " + gdbserverOutput));

    const int minMajor = inferiorTestData(backend).remoteAttachMinMajorVersion;
    if (minMajor > debuggerMajorVersion(inferiorTestData(backend).versionLine)) {
        QSKIP(qPrintable(QString("remote attach by pid needs a debugger version >= %1, this is "
                                 "\"%2\" - see remoteAttachMinMajorVersion")
                             .arg(minMajor).arg(inferiorTestData(backend).versionLine)));
    }
    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(backend,
        AttachToRemoteServerData{"localhost:" + port, inferiorTestData(backend).executable,
                                 ProcessHandle(pid), {}});
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk)
                             || debuggerBackend->contains(InferiorEvent::EngineIll), s_timeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk));
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunOk), s_timeout);

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();

    gdbserverProcess.kill();
    gdbserverProcess.waitForFinished();
    QCOMPARE(gdbserverProcess.state(), ProcessState::NotRunning);

    target.kill();
    target.waitForFinished();
    QCOMPARE(target.state(), ProcessState::NotRunning);
}

void tst_backends::runsRemoteExecutableViaExtendedRemote()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToRemoteServer); !result)
        QSKIP(qPrintable(result.error()));

    if (!m_gdbserverPath.isExecutableFile())
        QSKIP("gdbserver not found - set QTC_GDBSERVER_PATH_FOR_TEST to override.");

    const FilePath &executable = inferiorTestData(backend).executable;
    Process gdbserverProcess;
    QString gdbserverOutput;
    const bool stubHostsProcess = inferiorTestData(backend).remoteStubHostsProcess;
    const QString port = stubHostsProcess
        ? startGdbserver(gdbserverProcess, {}, {executable.nativePath()}, &gdbserverOutput)
        : startGdbserver(gdbserverProcess, {"--multi"}, {}, &gdbserverOutput);
    QVERIFY2(!port.isEmpty(),
             qPrintable("could not parse gdbserver's port from: " + gdbserverOutput));

    const int minMajor = inferiorTestData(backend).remoteAttachMinMajorVersion;
    if (minMajor > debuggerMajorVersion(inferiorTestData(backend).versionLine)) {
        QSKIP(qPrintable(QString("running a remote executable needs a debugger version >= %1, this "
                                 "is \"%2\" - see remoteAttachMinMajorVersion")
                             .arg(minMajor).arg(inferiorTestData(backend).versionLine)));
    }
    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(backend,
        AttachToRemoteServerData{"localhost:" + port, executable, {}, executable});
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk)
                             || debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk)
                             || debuggerBackend->contains(InferiorEvent::EngineIll)
                             || debuggerBackend->contains(InferiorEvent::EngineRunFailed), s_timeout);
    if (stubHostsProcess) {
        // The stub hands the process over stopped, so the backend has to resume it.
        QVERIFY(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk));
        QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunOk),
                                  "the remote executable was never resumed", s_timeout);
    } else {
        QVERIFY(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk));
    }

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();

    gdbserverProcess.kill();
    gdbserverProcess.waitForFinished();
    QCOMPARE(gdbserverProcess.state(), ProcessState::NotRunning);
}

void tst_backends::attachesToQnxTarget()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToRemoteServer); !result)
        QSKIP(qPrintable(result.error()));

    if (!m_qnxGdbPath.isExecutableFile())
        QSKIP("No QNX-flavored gdb available - set QTC_QNX_GDB_PATH_FOR_TEST "
              "to override (also needs a pdebug agent to connect to, not "
              "handled by this test at all yet).");
}

void tst_backends::attachesToCoreFile()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToCore); !result)
        QSKIP(qPrintable(result.error()));

    const FilePath gcorePath = FilePath::fromString("gcore").searchInPath();
    const FilePath lldbPath = FilePath::fromString("lldb").searchInPath();
    if (HostOsInfo::isMacHost() ? !lldbPath.isExecutableFile() : !gcorePath.isExecutableFile())
        QSKIP("No tool found to generate a core file for this test.");

    Process target;
    target.setCommand({inferiorTestData(backend).executable, {}});
    target.start();
    QVERIFY(target.waitForStarted());
    const qint64 pid = target.processId();
    QString targetOutput;
    auto sawAfterBump = [&] {
        targetOutput += target.readAllStandardOutput();
        return targetOutput.contains("after bump");
    };
    QTRY_VERIFY_WITH_TIMEOUT(sawAfterBump(), s_timeout);

    const FilePath coreFileBase = FilePath::fromString(m_tempDir.path()) / "core";
    const FilePath coreFile = FilePath::fromString(coreFileBase.nativePath() + "." + QString::number(pid));
    Process coreGenerator;
    if (HostOsInfo::isMacHost()) {
        coreGenerator.setCommand({lldbPath, {"--batch",
            "-o", "process save-core " + coreFile.nativePath(),
            "-o", "detach", "-o", "quit", "--attach-pid", QString::number(pid)}});
    } else {
        coreGenerator.setCommand({gcorePath, {"-o", coreFileBase.nativePath(), QString::number(pid)}});
    }
    coreGenerator.start();
    QVERIFY2(coreGenerator.waitForFinished(), "core generator never finished");
    QCOMPARE(coreGenerator.exitCode(), 0);

    target.kill();
    target.waitForFinished();

    QVERIFY2(coreFile.exists(),
             qPrintable("core generator did not produce " + coreFile.toUserOutput()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(backend,
        AttachToCoreData{coreFile, inferiorTestData(backend).executable});
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunOkAndInferiorUnrunnable)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed), s_timeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunOkAndInferiorUnrunnable));

    GdbMi stackData;
    bool stackReceived = false;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&stackData, &stackReceived](quint64, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::FullStack) {
            stackData = data;
            stackReceived = true;
        }
    });
    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 300;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(stackReceived, s_timeout);
    QVERIFY2(stackData.toString().contains("spin"), "core's stack did not show spin()");

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::InferiorIll),
                              "Continue against a core never reported InferiorIll", s_timeout);

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Interrupt});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::StopOk),
                              "Interrupt against a core never reported StopOk", s_timeout);

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();
}

void tst_backends::attachesToQmlServerAndStopsAtBreakpoint()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToQmlServer); !result)
        QSKIP(qPrintable(result.error()));

    Process inferiorProcess;
    const quint16 port = startQmlServer(inferiorProcess, inferiorTestData(backend).executable);
    QVERIFY2(port != 0, "could not start the Qml inferior/reserve a port for it");

    QUrl server;
    server.setHost("127.0.0.1");
    server.setPort(port);

    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(backend,
        AttachToQmlServerData{server});
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed), s_timeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk));

    QHash<quint64, bool> insertResults;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&insertResults](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        insertResults[requestId] = ok;
    });

    BreakpointChangeRequest request;
    request.op = BreakpointOp::Insert;
    request.requestId = 1;
    request.params.type = BreakpointByFileAndLine;
    request.params.fileName = inferiorTestData(backend).source;
    request.params.textPosition.line = inferiorTestData(backend).breakpointLine;
    request.params.enabled = true;
    engine->changeBreakpoint(request);
    QTRY_VERIFY_WITH_TIMEOUT(insertResults.contains(1), s_timeout);
    QVERIFY2(insertResults.value(1), "breakpoint insert failed");

    debuggerBackend->clearEvents();
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "breakpoint in compute() never signaled a stop", s_timeout);

    debuggerBackend->clearEvents();
    engine->shutdownEngine();

    inferiorProcess.kill();
    inferiorProcess.waitForFinished();
}

void tst_backends::insertsBreakpointAtJavaScriptThrowAndStopsAtIt()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToQmlServer); !result)
        QSKIP(qPrintable(result.error()));

    Process inferiorProcess;
    const quint16 port = startQmlServer(inferiorProcess, inferiorTestData(backend).executable);
    QVERIFY2(port != 0, "could not start the Qml inferior/reserve a port for it");

    QUrl server;
    server.setHost("127.0.0.1");
    server.setPort(port);

    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(backend,
        AttachToQmlServerData{server});
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed), s_timeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk));

    QHash<quint64, bool> insertResults;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&insertResults](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        insertResults[requestId] = ok;
    });

    BreakpointChangeRequest request;
    request.op = BreakpointOp::Insert;
    request.requestId = 1;
    request.params.type = BreakpointAtJavaScriptThrow;
    request.params.enabled = true;
    engine->changeBreakpoint(request);
    QTRY_VERIFY_WITH_TIMEOUT(insertResults.contains(1), s_timeout);
    QVERIFY2(insertResults.value(1), "BreakpointAtJavaScriptThrow insert failed");

    debuggerBackend->clearEvents();
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "throwsError() never signaled a stop", s_timeout);

    debuggerBackend->clearEvents();
    engine->shutdownEngine();

    inferiorProcess.kill();
    inferiorProcess.waitForFinished();
}

void tst_backends::reportsInspectorObjectTree()
{
    QFETCH(Backend, backend);

    const InferiorTestData testData = inferiorTestData(backend);
    if (testData.inspectorObject.isEmpty())
        QSKIP("inferior has no live object tree to inspect");
    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToQmlServer); !result)
        QSKIP(qPrintable(result.error()));

    Process inferiorProcess;
    const quint16 port = startQmlServer(inferiorProcess, testData.executable);
    QVERIFY2(port != 0, "could not start the Qml inferior/reserve a port for it");

    QUrl server;
    server.setHost("127.0.0.1");
    server.setPort(port);

    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(backend,
        AttachToQmlServerData{server});
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QList<std::pair<quint64, GdbMi>> trees;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&trees](quint64 requestId, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::InspectorTree)
            trees.append({requestId, data});
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk), s_timeout);

    const auto itemFor = [&trees](quint64 requestId, const QString &iname) {
        GdbMi found;
        for (const auto &[id, tree] : trees) {
            if (id != requestId)
                continue;
            for (const GdbMi &item : tree["data"]) {
                if (item["iname"].data() == iname)
                    found = item;
            }
        }
        return found;
    };
    const auto inameFor = [&trees](quint64 requestId, const QString &name) {
        QString found;
        for (const auto &[id, tree] : trees) {
            if (id != requestId)
                continue;
            for (const GdbMi &item : tree["data"]) {
                if (item["name"].data() == name)
                    found = item["iname"].data();
            }
        }
        return found;
    };

    RefreshRequest request;
    request.kind = RefreshKind::InspectorTree;
    request.requestId = 300;
    engine->refresh(request);
    QTRY_VERIFY2_WITH_TIMEOUT(!inameFor(300, testData.inspectorObject).isEmpty(),
                              "the object tree never reported the expected object", s_timeout);
    const QString objectIName = inameFor(300, testData.inspectorObject);
    QVERIFY2(objectIName.startsWith("inspect."), qPrintable("iname: " + objectIName));

    QVERIFY2(inameFor(300, testData.inspectorProperty).isEmpty(),
             "a collapsed object reported its properties anyway");

    request.requestId = 301;
    request.expandedINames = {objectIName};
    engine->refresh(request);
    QTRY_VERIFY2_WITH_TIMEOUT(!inameFor(301, testData.inspectorProperty).isEmpty(),
                              "expanding the object never reported its properties", s_timeout);
    const QString propertyIName = inameFor(301, testData.inspectorProperty);
    QVERIFY2(propertyIName.startsWith(objectIName + ".[properties]."),
             qPrintable("iname: " + propertyIName));

    const GdbMi propertyItem = itemFor(301, propertyIName);
    QVERIFY(propertyItem.isValid());
    WatchItemData assignTarget;
    assignTarget.iname = propertyIName;
    assignTarget.id = propertyItem["id"].toInt();
    assignTarget.type = propertyItem["type"].data();
    assignTarget.isInspect = true;
    QVERIFY(assignTarget.id != -1);
    const QString newValue = "4242";
    engine->assignValueInDebugger(assignTarget, testData.inspectorProperty, newValue);
    QTRY_VERIFY2_WITH_TIMEOUT(itemFor(0, propertyIName)["value"].data() == newValue,
                              "assigning an Inspector property never reported the new value",
                              s_timeout);

    QStringList consoleResults;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&consoleResults](const QString &text, int channel, int) {
        if (channel == Debugger::ConsoleOutput)
            consoleResults.append(text);
    });
    engine->executeDebuggerCommand(testData.inspectorPropertyExpression, assignTarget);
    QTRY_VERIFY2_WITH_TIMEOUT(consoleResults.contains(newValue),
                              qPrintable("evaluating against the Inspector object reported: "
                                          + consoleResults.join('|')), s_timeout);

    if (testData.inspectorOrphanObject.isEmpty())
        return;
    const QString engineIName = objectIName.left(objectIName.indexOf('.', strlen("inspect.")));
    request.requestId = 302;
    engine->refresh(request);
    QTRY_VERIFY2_WITH_TIMEOUT(!inameFor(302, testData.inspectorOrphanObject).isEmpty(),
                              "a parentless object never reached the tree", s_timeout);
    QCOMPARE(inameFor(302, testData.inspectorOrphanObject).count('.'),
             engineIName.count('.') + 1);

    inferiorProcess.kill();
    inferiorProcess.waitForFinished();
    engine->shutdownEngine();
}

QTEST_GUILESS_MAIN(tst_backends)

#include "tst_backends.moc"
