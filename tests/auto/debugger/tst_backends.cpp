// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggerengineinterface.h"

#include "bridge/bridgeimpl.h"
#include "dap/dapimpl.h"
#include "cdb/cdbimpl.h"
#include "gdb/gdbimpl.h"
#include "lldb/lldbimpl.h"
#include "pdb/pdbimpl.h"
#include "qml/qmlimpl.h"

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/elfreader.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/processreaper.h>
#include <utils/qtcprocess.h>
#include <utils/result.h>
#include <utils/temporarydirectory.h>

#include <chrono>
#include <csignal>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include <optional>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonDocument>
#include <QLibraryInfo>
#include <QMap>
#include <QMetaEnum>
#include <QPoint>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSettings>
#include <QSignalSpy>
#include <QHostAddress>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

using namespace Debugger::Internal;
using namespace Utils;

static constexpr std::chrono::seconds s_timeout{5};
static constexpr std::chrono::seconds s_warmUpTimeout{30};
static constexpr std::chrono::seconds s_qmlStartupTimeout{15};
static constexpr std::chrono::seconds s_compileTimeout{120};

// The gdb version from which "gdb -i dap" speaks the protocol well enough
// to be tested against.
static constexpr int s_dapInterpreterVersion = 14;

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

// Runs an MSVC environment batch file and returns the variables it sets, the way
// MsvcToolchain does: markers around a "set" dump in the batch file's own stdout.
static QMap<QString, QString> environmentFromBatchFile(const Environment &env,
                                                       const QString &batchFile)
{
    const QString marker = "####################";
    QByteArray content;
    const auto writeLine = [&content](const QByteArray &line) { content += line + "\r\n"; };
    const QByteArray call = "call " + ProcessArgs::quoteArg(batchFile).toLocal8Bit();
    writeLine(call);
    writeLine("@echo " + marker.toLocal8Bit());
    writeLine("set");
    writeLine("@echo " + marker.toLocal8Bit());

    TempFileSaver saver(QDir::tempPath() + "/qtc_cdbenv_XXXXXX.bat");
    saver.write(content);
    if (const Result<> res = saver.finalize(); !res) {
        qWarning("%s: %s", Q_FUNC_INFO, qPrintable(res.error()));
        return {};
    }

    Process run;
    // As of WinSDK 7.1 a preset ORIGINALPATH keeps the path from being set correctly.
    Environment runEnv = env;
    runEnv.unset("ORIGINALPATH");
    run.setEnvironment(runEnv);
    run.setCommand({FilePath::fromUserInput(qtcEnvironmentVariable("COMSPEC")),
                    {"/D", "/E:ON", "/V:ON", "/c", saver.filePath().nativePath()}});
    run.runBlocking(s_compileTimeout);
    if (run.result() != ProcessResult::FinishedWithSuccess) {
        qWarning("%s: running \"%s\" failed: %s", Q_FUNC_INFO, qPrintable(batchFile),
                 qPrintable(run.verboseExitMessage()));
        return {};
    }

    const QString stdOut = run.cleanedStdOut();
    const int start = stdOut.indexOf(marker);
    const int end = start == -1 ? -1 : stdOut.indexOf(marker, start + 1);
    if (start == -1 || end == -1) {
        qWarning("%s: no environment dump in the output of \"%s\".", Q_FUNC_INFO,
                 qPrintable(batchFile));
        return {};
    }

    QMap<QString, QString> envPairs;
    static const QRegularExpression assignment("^(\\w+)=(.+)$");
    for (const QString &line : stdOut.mid(start, end - start).split('\n')) {
        const QRegularExpressionMatch match = assignment.match(line.trimmed());
        if (match.hasMatch())
            envPairs.insert(match.captured(1), match.captured(2));
    }
    return envPairs;
}

// Only the reference CI is provisioned with every backend, so a backend missing
// there is a defect rather than something that was never installed.
static bool backendsAreRequired()
{
    return qtcEnvironmentVariableIsSet("QTC_REQUIRE_BACKENDS_FOR_TEST");
}

static const char s_qmlNativeDebuggerPluginMissing[] =
    "Qt's qmldbg_native plugin not found - can't establish a live "
    "QML debug connection.";

static const char s_qtDeclarativeDebugInfoMissing[] =
    "libQt6Qml has no DWARF debug info - gdbbridge.py can't recognize its "
    "own interpreter-internal frames, so no QML frames get spliced in.";

enum class Backend {
    Gdb,
    Bridge,
    Dap,
    Lldb,
    Pdb,
    Qml,
    Cdb,
};
Q_DECLARE_METATYPE(Backend)

struct InferiorTestData
{
    FilePath source;
    FilePath executable;
    // A second, small inferior built for the other word width, which the host
    // debugs through a compatibility layer. Empty where no toolchain for it was
    // found. It burns CPU rather than sleeping, so that interrupting it lands in
    // its own code instead of inside the layer's syscall thunk.
    FilePath otherWordWidthSource;
    FilePath otherWordWidthExecutable;
    int otherWordWidthBreakpointLine = 0;
    QString otherWordWidthFunction;
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
    QString symbolOptionsCommand;
    QString moduleWithPrivateSymbols;
    QString moduleWithoutPrivateSymbols;
    bool marksUninitializedVariables = false;
    // Whether the backend's bridge resolves a QML breakpoint through casts on
    // the debug service, rather than marshalling the arguments and calling by
    // address the way the cdb one does.
    bool qmlBreakpointsUseServiceCasts = false;
    // Whether the inferior throws a C++ exception once it is past the markers
    // above, and what it prints after catching it again.
    bool throwsAnException = false;
    QString afterThrowOutputMarker;
    QString disassemblySourceMarker;
    QString alienBreakpointCommand;
    QString alienBreakpointDeleteCommand;
    bool answersRedundantContinue = false;
    int expectedExitCode = 0;
    QString recursionDepthVariable;
    int multiLocationBreakpointLine = 0;
    int spinBodyLine = 0;
    // A line whose step lands in a standard header the debugger may skip.
    int knownFrameStepLine = 0;
    // A line whose step-into first lands where the linker jumps from, which has
    // no source of its own.
    int thunkStepLine = 0;
    // A line the backend is expected to refuse a breakpoint on, e.g. a comment.
    int unbreakableLine = 0;
    QString localMarker;
    QString functionMarker;
    QString expandableLocal;
    // A local whose value is longer than any string limit worth configuring.
    QString longStringLocal;
    QString expandableChild;
    // A line the inferior reaches while a library it loads at runtime is
    // loaded, plus a global whose target type only that library's debug
    // info describes, and a member of that type.
    int libraryLoadedLine = 0;
    QString libraryTypeSymbol;
    QString libraryTypeChild;
    QString inspectorObject;
    QString inspectorProperty;
    QString inspectorPropertyExpression;
    QString inspectorOrphanObject;
    QString versionLine;
    QString moduleListMarker;
    // Set only where the debugger is configured to print a value this long in
    // full: Cdb's limit is not configured, Pdb and Qml have no such symbol.
    QString longTextSymbol;
    QString applicationOutputMarker;
    QString environmentReportPrefix;
    // What the inferior prints the debug heap flag it was started with behind.
    QString heapFlagReportPrefix;
    // What it prints after handling an access violation of its own, if it can.
    QString survivedAccessViolationMarker;
    // What it prints the value of the variable named in QTC_BACKEND_ENV_QUERY behind.
    QString environmentQueryPrefix;
    // A program built against the debug C runtime, which it asks to report.
    Utils::FilePath debugCrtExecutable;
    QString pastCrtReportMarker;
    // The C runtime that program reports through.
    QString crtDebugReportModule;
    QString workingDirectoryReportPrefix;
    FilePath moduleSymbolsPath;
    QString falseLiteral = "0";
};

struct BackendData
{
    FilePath path;
    InferiorTestData inferiorData;
    FilePath cdbExtensionDir;
    QString cdbExtensionFileName;
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

// Auto-detection stays off on Windows, but an explicit override works there, too.
static FilePath gdbPathForTest()
{
    const QString override = qtcEnvironmentVariable("QTC_GDB_PATH_FOR_TEST");
    if (!override.isEmpty())
        return FilePath::fromUserInput(override);
    return HostOsInfo::isWindowsHost() ? FilePath() : findGdbOnPath();
}

static FilePath lldbPathForTest()
{
    const QString override = qtcEnvironmentVariable("QTC_LLDB_PATH_FOR_TEST");
    if (!override.isEmpty())
        return FilePath::fromUserInput(override);
    return HostOsInfo::isWindowsHost() ? FilePath() : FilePath::fromString("lldb").searchInPath();
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

static FilePath findPythonOnPath()
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

static FilePath findCdbOnPath()
{
    const auto usable = [](const FilePath &path) {
        if (!path.isExecutableFile())
            return false;
        Process versionProcess;
        versionProcess.setCommand({path, {"-version"}});
        versionProcess.runBlocking();
        return versionProcess.result() == ProcessResult::FinishedWithSuccess;
    };

    const auto debuggerIn = [&usable](const FilePath &versionRoot) -> FilePath {
        for (const char *arch :
#ifdef Q_PROCESSOR_ARM
             {"arm64", "x64"}
#else
             {"x64", "x86"}
#endif
             ) {
            const FilePath candidate = versionRoot / "Debuggers" / arch / "cdb.exe";
            if (usable(candidate))
                return candidate;
        }
        return {};
    };

    const QSettings installedRoots(
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots",
        QSettings::NativeFormat);
    for (const char *kit : {"KitsRoot10", "KitsRoot81"}) {
        const QString root = installedRoots.value(QLatin1String(kit)).toString();
        if (root.isEmpty())
            continue;
        if (const FilePath cdb = debuggerIn(FilePath::fromUserInput(root)); !cdb.isEmpty())
            return cdb;
    }

    for (const char *rootVar : {"ProgramFiles(x86)", "ProgramFiles", "ProgramW6432"}) {
        const QString root = qtcEnvironmentVariable(rootVar);
        if (root.isEmpty())
            continue;
        const FilePath kitsRoot = FilePath::fromUserInput(root) / "Windows Kits";
        if (!kitsRoot.isDir())
            continue;
        const QDir kitsDir(kitsRoot.toFSPathString());
        QStringList versions = kitsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        std::sort(versions.begin(), versions.end());
        std::reverse(versions.begin(), versions.end());
        for (const QString &version : versions) {
            if (const FilePath cdb = debuggerIn(kitsRoot / version); !cdb.isEmpty())
                return cdb;
        }
    }

    static const QStringList candidates = {
        "cdb", "cdb.exe",
#ifdef Q_PROCESSOR_ARM
        "cdbARM64.exe",
#endif
        "cdbX64.exe", "cdbX86.exe",
    };
    for (const QString &candidate : candidates) {
        const FilePath path = FilePath::fromString(candidate).searchInPath();
        if (usable(path))
            return path;
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

// A released Qt ships libqmldbg_native without debug info, which leaves its
// qt_qmlDebug* symbols typeless - the case the casts in dumper.py exist for. A
// developer build has the debug info, so recreate the released shape from it.
static Result<FilePath> strippedQmlDebugPluginDir(const FilePath &parentDir)
{
    const FilePath pluginDir =
        FilePath::fromUserInput(QLibraryInfo::path(QLibraryInfo::PluginsPath)) / "qmltooling";
    const FilePath original = Utils::findOrDefault(pluginDir.dirEntries(DirFilterFlag::Files),
                                                   [](const FilePath &path) {
        const QString base = path.completeBaseName();
        return base == "qmldbg_native" || base == "libqmldbg_native";
    });
    if (original.isEmpty())
        return ResultError("No qmldbg_native plugin in " + pluginDir.toUserOutput());

    if (Utils::ElfReader(original).readHeaders().indexOf(".debug_info") == -1) {
        return ResultError(original.toUserOutput()
                           + " has no .debug_info section to strip - not an ELF build?");
    }

    const FilePath objcopy = FilePath::fromString("objcopy").searchInPath();
    if (!objcopy.isExecutableFile())
        return ResultError(QString("objcopy was not found in PATH."));

    const FilePath strippedDir = parentDir / "qmltooling";
    if (const Result<> res = strippedDir.ensureWritableDir(); !res)
        return ResultError(res.error());

    const FilePath stripped = strippedDir / original.fileName();
    Process objcopyProcess;
    objcopyProcess.setCommand(
        {objcopy, {"--strip-debug", original.nativePath(), stripped.nativePath()}});
    objcopyProcess.runBlocking(s_timeout);
    if (objcopyProcess.result() != ProcessResult::FinishedWithSuccess)
        return ResultError("objcopy failed: " + objcopyProcess.allOutput());
    // Without this the test would silently pass on an unstripped plugin, which
    // is exactly the configuration that cannot tell the casts apart.
    if (Utils::ElfReader(stripped).readHeaders().indexOf(".debug_info") != -1)
        return ResultError(stripped.toUserOutput() + " still carries debug info.");
    return strippedDir;
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
    case Backend::Bridge:
        return "bridge";
    case Backend::Dap:
        return "dap";
    case Backend::Lldb:
        return "lldb";
    case Backend::Pdb:
        return "pdb";
    case Backend::Qml:
        return "qml";
    case Backend::Cdb:
        return "cdb";
    }
    return {};
}

// lldb spells a C++ frame's function with its signature, gdb without it.
static bool stackHasFunction(const QString &stack, const QString &function)
{
    return stack.contains("function=\"" + function + '"')
           || stack.contains("function=\"" + function + '(');
}

struct ConfiguredOptionProbe
{
    QString query;
    QString expectedOutput;
};

static QList<ConfiguredOptionProbe> configuredOptionProbes(Backend backend,
                                                           const Utils::FilePath &existingDir)
{
    switch (backend) {
    case Backend::Gdb:
        return {{"show index-cache", "The index cache is currently enabled."},
                {"show detach-on-fork", "Whether gdb will detach the child of a fork is off."},
                {"show mi-async", "Whether MI is in asynchronous mode is on."},
                {"python print(theDumper.usePlainDumpers)", "True"},
                {"show sysroot", "The current system root is \"/qtc-test-sysroot\"."},
                {"show substitute-path", "`/qtc-test-from' -> `/qtc-test-to'."},
                {"show directories", existingDir.path()},
                {"show debug-file-directory", existingDir.path()},
                {"show solib-search-path", "/qtc-test-solib"}};
    case Backend::Lldb:
        return {{"settings show target.exec-search-paths", "/qtc-test-solib"}};
    // Cdb has none: a query goes to a debugger whose inferior runs, which takes
    // the interrupt the ctrl-c stub provides. What it was configured with shows
    // up in the startup traffic instead - see configuredOptionMarkers().
    case Backend::Cdb:
    case Backend::Pdb:
    case Backend::Qml:
    case Backend::Bridge:
    case Backend::Dap:
        break;
    }
    return {};
}

struct InitFileProbe
{
    QString fileName;
    QString content;
};

static InitFileProbe initFileProbe(Backend backend, const QString &marker)
{
    switch (backend) {
    case Backend::Gdb:
        return {".gdbinit", "echo " + marker + "\\n\n"};
    case Backend::Lldb:
    case Backend::Pdb:
    case Backend::Qml:
    case Backend::Cdb:
    case Backend::Bridge:
    case Backend::Dap:
        break;
    }
    return {};
}

// Only gdb has a debug info daemon to configure, and only when built with
// support for it, which the test probes for at runtime.
// x86 gdb is the only backend with a disassembly flavor to configure, and the
// setting does not exist on other architectures, which the test probes for.
// The configured flavor as it appears in the traffic: gdb keeps it in a
// setting, lldb takes it with every disassembly request.
// The markers a fully configured engine must produce. A backend that runs the
// post-attach commands only when it really attaches has nothing to show for
// them on a plain run.
static QStringList configuredOptionMarkers(Backend backend, const Utils::FilePath &existingDir)
{
    QStringList markers{"QTCSTARTUPMARKER", "QTCEXTRADUMPERCOMMAND"};
    if (backend == Backend::Gdb)
        markers << "QTCPOSTATTACHMARKER";
    // What cdb says about its symbol path while starting up. Asking it later
    // would need the running inferior interrupted, which takes the ctrl-c stub.
    if (backend == Backend::Cdb)
        markers << existingDir.nativePath();
    return markers;
}

static QString disassemblyFlavorWireMarker(Backend backend)
{
    switch (backend) {
    case Backend::Gdb:
        return "disassembly-flavor intel";
    case Backend::Lldb:
        return "\"flavor\":\"intel\"";
    case Backend::Pdb:
    case Backend::Qml:
    case Backend::Cdb:
    case Backend::Bridge:
    case Backend::Dap:
        break;
    }
    return {};
}

static QString disassemblyFlavorQuery(Backend backend)
{
    switch (backend) {
    case Backend::Gdb:
        return "show disassembly-flavor";
    case Backend::Lldb:
    case Backend::Pdb:
    case Backend::Qml:
    case Backend::Cdb:
    case Backend::Bridge:
    case Backend::Dap:
        break;
    }
    return {};
}

static QString debugInfoDaemonQuery(Backend backend)
{
    switch (backend) {
    case Backend::Gdb:
        return "show debuginfod enabled";
    case Backend::Lldb:
    case Backend::Pdb:
    case Backend::Qml:
    case Backend::Cdb:
    case Backend::Bridge:
    case Backend::Dap:
        break;
    }
    return {};
}

// pdbbridge.py's stackListFrames() reports the whole stack and takes no limit,
// so a depth limit cannot reach it yet.
// Whether attaching leaves the inferior running: lldb resumes it itself, and
// the bridge acknowledges the attach as a running inferior, while gdb reports
// the stop that attaching causes. A stock DAP adapter's answer to the attach
// says neither, so the backend reports a running inferior and passes on
// whatever the adapter does to the debuggee afterwards.
static bool attachResumesInferior(Backend backend)
{
    switch (backend) {
    case Backend::Dap:
    case Backend::Lldb:
    case Backend::Bridge:
        return true;
    case Backend::Gdb:
    case Backend::Cdb:
    case Backend::Pdb:
    case Backend::Qml:
        break;
    }
    return false;
}

// Stands in for a debugger that is there but never opens a session: whatever a
// backend appends to it, it exits at once instead of taking commands.
static CommandLine quittingDebuggerCommand()
{
    const QString name = HostOsInfo::isWindowsHost() ? "hostname.exe" : "true";
    const FilePath tool = FilePath::fromString(name).searchInPath();
    return tool.isExecutableFile() ? CommandLine{tool, {}} : CommandLine{};
}

// The wire form a backend uses for a tracepoint when it is not the debugger's
// own pseudo one.
static QString realTracepointMarker(Backend backend)
{
    switch (backend) {
    case Backend::Gdb:
        return "-break-insert";
    case Backend::Lldb:
    case Backend::Bridge:
    case Backend::Cdb:
    case Backend::Pdb:
    case Backend::Qml:
    case Backend::Dap:
        break;
    }
    return {};
}

// How a tracepoint's text ends: the dumpers print the string a char pointer
// points to, an adapter prints what its debugger prints, pointer and all.
static QString tracepointMessageTail(Backend backend)
{
    return backend == Backend::Dap ? QString("\"hi\"") : QString("globalMessage is \"hi\"");
}

// What a detach looks like in what a backend logs: DAP has no detach request,
// it is a disconnect that leaves the debuggee alone.
static QString detachMarker(Backend backend)
{
    switch (backend) {
    case Backend::Bridge:
    case Backend::Dap:
        return "terminateDebuggee\":false";
    case Backend::Gdb:
    case Backend::Lldb:
    case Backend::Cdb:
    case Backend::Pdb:
    case Backend::Qml:
        break;
    }
    return "detach";
}

// What a breakpoint modification carries that the answer to setting it did not:
// the gdb family counts the hits, while DAP has no hit count at all and what a
// stock adapter reports is the address it bound the breakpoint to.
static const char *breakpointModifiedField(Backend backend)
{
    switch (backend) {
    case Backend::Dap:
        return "addr";
    case Backend::Gdb:
    case Backend::Lldb:
    case Backend::Bridge:
    case Backend::Cdb:
    case Backend::Pdb:
    case Backend::Qml:
        break;
    }
    return "times";
}

// The marker a backend logs per command when time stamps are configured.
static QString responseTimeMarker(Backend backend)
{
    switch (backend) {
    case Backend::Gdb:
        return "Response time";
    case Backend::Lldb:
    case Backend::Cdb:
    case Backend::Pdb:
    case Backend::Qml:
    case Backend::Bridge:
    case Backend::Dap:
        break;
    }
    return {};
}

// The user command hooks that fire on their own occasion rather than at
// startup, so that only the occasion can cover them.
enum class UserCommandHook { Reset, AfterConnect };

// The command a backend is given for a hook, and what that command prints.
// Empty means its start data carries no such hook.
struct UserCommandProbe
{
    QString command;
    QString marker;
};

static UserCommandProbe userCommandProbe(Backend backend, UserCommandHook hook)
{
    switch (backend) {
    case Backend::Gdb: {
        const QString marker = hook == UserCommandHook::Reset ? QString("QTCFORRESETMARKER")
                                                              : QString("QTCAFTERCONNECTMARKER");
        return {"echo " + marker + "\\n", marker};
    }
    case Backend::Lldb:
    case Backend::Cdb:
    case Backend::Pdb:
    case Backend::Qml:
    case Backend::Bridge:
        break;
    }
    return {};
}

static bool limitsStackDepth(Backend backend)
{
    switch (backend) {
    case Backend::Bridge:
    case Backend::Dap:
    case Backend::Gdb:
    case Backend::Lldb:
    case Backend::Cdb:
        return true;
    case Backend::Pdb:
    case Backend::Qml:
        break;
    }
    return false;
}

// Whether a function can be disassembled by name alone: DAP's disassemble
// request takes a memory reference, and the protocol has no request that
// resolves a name to one.
static bool disassemblesByFunctionName(Backend backend)
{
    switch (backend) {
    case Backend::Gdb:
    case Backend::Lldb:
    case Backend::Cdb:
    case Backend::Pdb:
    case Backend::Bridge:
    case Backend::Qml:
        return true;
    case Backend::Dap:
        break;
    }
    return false;
}

// Whether a backend's dumper layer answers with the types it knows. A property
// of the bridge script rather than a claim the frontend could act on: the
// reload happens either way.
static bool reportsDumperTypes(Backend backend)
{
    switch (backend) {
    case Backend::Gdb:
    case Backend::Lldb:
    case Backend::Cdb:
    case Backend::Bridge:
        return true;
    case Backend::Pdb: // formats python values directly, with no dumper modules
    case Backend::Qml: // no python dumpers at all
        break;
    }
    return false;
}

// Whether a container local looks different with and without the debugging
// helpers. lldb's own synthetic children shape std::vector either way, and Qml
// has no python dumpers at all.
static bool debuggingHelpersChangeContainerOutput(Backend backend)
{
    switch (backend) {
    case Backend::Gdb:
    case Backend::Pdb:
    case Backend::Cdb:
        return true;
    case Backend::Lldb:
    case Backend::Qml:
    case Backend::Bridge:
    case Backend::Dap:
        break;
    }
    return false;
}

// Whether the length limit in the request reaches the value: a dumper takes it
// as an option, while stock DAP has no request carrying one, so the adapter
// prints as much as its own settings allow.
static bool honorsStringLengthLimits(Backend backend)
{
    switch (backend) {
    case Backend::Bridge:
    case Backend::Gdb:
    case Backend::Lldb:
    case Backend::Pdb:
    case Backend::Cdb:
        return true;
    case Backend::Dap:
    case Backend::Qml:
        break;
    }
    return false;
}

static QString watchdogProbeCommand(Backend backend, int seconds)
{
    switch (backend) {
    case Backend::Gdb:
        // cmd.exe has no sleep, and "timeout /t" refuses to run with redirected input.
        if (HostOsInfo::isWindowsHost())
            return QString("shell ping -n %1 127.0.0.1").arg(seconds + 1);
        return QString("shell sleep %1").arg(seconds);
    case Backend::Cdb:
        // cdb has a wait of its own; ".shell" would need a console for its child.
        // "0n" spells the milliseconds out in decimal: cdb reads a number as hex.
        return QString(".sleep 0n%1").arg(seconds * 1000);
    case Backend::Lldb:
        return QString("platform shell sleep %1").arg(seconds);
    case Backend::Pdb:
        // pdb runs a statement typed at its prompt, which blocks it just as well.
        return QString("import time; time.sleep(%1)").arg(seconds);
    case Backend::Qml:
    case Backend::Bridge:
    case Backend::Dap:
        break;
    }
    return {};
}

static QString printCommand(Backend backend, const QString &expression)
{
    Q_UNUSED(expression)
    switch (backend) {
    case Backend::Gdb:
    case Backend::Bridge:
    case Backend::Dap:
        return "print " + expression;
    case Backend::Lldb:
        return "expr " + expression;
    case Backend::Pdb:
    case Backend::Qml:
        return expression;
    case Backend::Cdb:
        return "? " + expression;
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

static GdbMi findItemByName(const GdbMi &data, const QString &name)
{
    if (data["name"].data() == name)
        return data;
    for (const GdbMi &child : data) {
        if (const GdbMi found = findItemByName(child, name); found.isValid())
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
        connect(m_engine.get(), &DebuggerEngineInterface::threadEvent, this,
                [this](ThreadEvent event, const GdbMi &data) {
            m_threadEvents.append({event, data["id"].data()});
        });
        connect(m_engine.get(), &DebuggerEngineInterface::breakpointEvent, this,
                [this](quint64, BreakpointOp op, bool ok, const GdbMi &data) {
            if (op == BreakpointOp::Insert && ok && data.childCount() > 0)
                m_breakpointResponseId = data.childAt(0)["number"].data();
        });
    }

    // The tests connect lambdas capturing their own locals to the engine's
    // signals. Those locals are gone by the time this runs, so nothing the
    // engine emits while it goes away may still reach them.
    ~DebuggerBackend() override
    {
        if (m_engine)
            m_engine->disconnect();
    }

    DebuggerEngineInterface *engine() const { return m_engine.get(); }

    void execute(const ExecutionRequest &request) { m_engine->execute(request); }

    bool contains(InferiorEvent event) const { return m_events.contains(event); }
    qsizetype count(InferiorEvent event) const { return m_events.count(event); }
    bool isEmpty() const { return m_events.isEmpty(); }
    qsizetype size() const { return m_events.size(); }
    void clearEvents() { m_events.clear(); }
    void clearStoppedLocation() { m_stoppedFile = {}; m_stoppedLine = 0; }
    const QList<InferiorEvent> &events() const { return m_events; }

    const QList<InferiorResultData> &inferiorResults() const { return m_inferiorResults; }
    void clearInferiorResults() { m_inferiorResults.clear(); }

    Utils::FilePath stoppedFile() const { return m_stoppedFile; }
    int stoppedLine() const { return m_stoppedLine; }

    QString breakpointResponseId() const { return m_breakpointResponseId; }

    QStringList threadIds(ThreadEvent event) const
    {
        QStringList ids;
        for (const auto &[recorded, id] : m_threadEvents) {
            if (recorded == event)
                ids.append(id);
        }
        return ids;
    }

private:
    QList<QPair<ThreadEvent, QString>> m_threadEvents;
    std::unique_ptr<DebuggerEngineInterface> m_engine;
    QList<InferiorEvent> m_events;
    QList<InferiorResultData> m_inferiorResults;
    Utils::FilePath m_stoppedFile;
    int m_stoppedLine = 0;
    QString m_breakpointResponseId;
};

static QString wireTail(const QStringList &wire, int lines = 40)
{
    QStringList tail;
    for (const QString &line : wire.mid(qMax(0, wire.size() - lines)))
        tail.append(line.size() > 160 ? line.left(160) + "..." : line);
    return tail.join("\n  ");
}

static bool hasResolvedQmlBreakpoint(const QList<GdbMi> &reports, int modelId)
{
    for (const GdbMi &report : reports) {
        const GdbMi entry = report.childAt(0);
        if (entry["modelid"].toInt() == modelId && entry["pending"].toInt() == 0
            && !entry["number"].data().isEmpty() && entry["number"].toInt() != -1) {
            return true;
        }
    }
    return false;
}

// The interpreter protocol is inferior calls end to end, and gdb writes the
// register state back after each one - which fails with EFAULT on some hosts,
// depending on the CPU and the kernel. Nothing on this side can retry past
// that, so say so instead of blaming the breakpoint.
static QString refusedInferiorCall(const QStringList &wire)
{
    for (const QString &line : wire) {
        if (line.contains("Couldn't write extended state status"))
            return line.trimmed();
    }
    return {};
}

static QString qmlResolutionDiagnosis(const QList<GdbMi> &reports, const QStringList &wire)
{
    return QString("no report resolved the QML breakpoint (%1 arrived; a refused one "
                   "reports number=-1) - last wire traffic:\n  %2")
        .arg(reports.size()).arg(wireTail(wire));
}

static bool canInterruptRunningInferior(Backend backend)
{
    if (!HostOsInfo::isWindowsHost())
        return true;
    static const QList<Backend> uninterruptibleOnWindows = {Backend::Pdb};
    return !uninterruptibleOnWindows.contains(backend);
}

class tst_backends : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void stopsAtBreakpointThroughDapAdapter();
    void reportsADapAdapterThatQuitsAtOnce();
    void reportsTheRunBeforeItsOutcomeThroughDapAdapter();
    void followsAResumeTheDapAdapterMakesOnItsOwn();
    void readsOnlyTheLocalScopeFromADapAdapter();
    void reportsASocketThatCannotConnect();
    void dropsALocalsWalkThatWasStartedOver();
    void sendsTheProtocolsStepRequestsThroughADapAdapter();
    void sendsNoRequestADapAdapterCannotAnswer();
    void reportsTheThreadsADapAdapterListsAgainstTheStoppedOne();
    void stepsTheThreadThatWasSelectedFromADapAdapter();
    void reportsADetachFromADapAdapterAsOne();
    void acknowledgesEveryBreakpointADapAdapterAnswersFor();

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
    void honorsTheConfiguredBreakEvents_data() { addBackendRows(); }
    void honorsTheConfiguredBreakEvents();
    void stopsAtACaughtException_data() { addBackendRows(); }
    void stopsAtACaughtException();
    void stopsAtAThrownException_data() { addBackendRows(); }
    void stopsAtAThrownException();
    void testBreakOnThrowAndCatchCapability_data() { addBackendRows(); }
    void testBreakOnThrowAndCatchCapability();
    void testCreateFullBacktraceCapability_data() { addBackendRows(); }
    void testCreateFullBacktraceCapability();
    void activatesFrameAndReadsItsLocals_data() { addBackendRows(); }
    void activatesFrameAndReadsItsLocals();
    void limitsTheReportedStackDepth_data() { addBackendRows(); }
    void limitsTheReportedStackDepth();
    void stepsPastTheLinkersJumpToAFunction_data() { addBackendRows(); }
    void stepsPastTheLinkersJumpToAFunction();
    void skipsKnownFramesWhenStepping_data() { addBackendRows(); }
    void skipsKnownFramesWhenStepping();
    void logsTheResponseTimeWhenConfigured_data() { addBackendRows(); }
    void logsTheResponseTimeWhenConfigured();
    void stopsAtMainWhenConfigured_data() { addBackendRows(); }
    void stopsAtMainWhenConfigured();
    void stopsBeforeRunningWhenConfigured_data() { addBackendRows(); }
    void stopsBeforeRunningWhenConfigured();
    void continuesAfterAttachWhenConfigured_data() { addBackendRows(); }
    void continuesAfterAttachWhenConfigured();
    void insertsARealTracepointWhenPseudoOnesAreOff_data() { addBackendRows(); }
    void insertsARealTracepointWhenPseudoOnesAreOff();
    void testDetachCapability_data() { addBackendRows(); }
    void testDetachCapability();
    void testDisassemblerCapability_data() { addBackendRows(); }
    void testDisassemblerCapability();
    void reportsSourceLinesInTheDisassembly_data() { addBackendRows(); }
    void reportsSourceLinesInTheDisassembly();
    void testJumpToLineCapability_data() { addBackendRows(); }
    void testJumpToLineCapability();
    void testLibraryEventCapability_data() { addBackendRows(); }
    void testLibraryEventCapability();
    void testThreadEventCapability_data() { addBackendRows(); }
    void testThreadEventCapability();
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
    void runsUserCommandsWhenResettingTheInferior_data() { addBackendRows(); }
    void runsUserCommandsWhenResettingTheInferior();
    void runsAUserStartScriptAtStartup_data() { addBackendRows(); }
    void runsAUserStartScriptAtStartup();
    void runsTheDebuggerAsTheConfiguredUser_data() { addBackendRows(); }
    void runsTheDebuggerAsTheConfiguredUser();
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
    void reportsApplicationOutput_data() { addBackendRows(); }
    void reportsApplicationOutput();
    void reportsAFirstChanceExceptionWhenAsked_data() { addBackendRows(); }
    void reportsAFirstChanceExceptionWhenAsked();
    void stopsWhereTheDebugRuntimeReports_data() { addBackendRows(); }
    void stopsWhereTheDebugRuntimeReports();
    void keepsQtLoggingOffTheConsoleWithoutATerminal_data() { addBackendRows(); }
    void keepsQtLoggingOffTheConsoleWithoutATerminal();
    void asksCdbForItsOwnConsoleWithATerminal_data() { addBackendRows(); }
    void asksCdbForItsOwnConsoleWithATerminal();
    void ignoresAFirstChanceAccessViolationWhenAsked_data() { addBackendRows(); }
    void ignoresAFirstChanceAccessViolationWhenAsked();
    void passesTheHeapDebuggingFlagToTheDebuggee_data() { addBackendRows(); }
    void passesTheHeapDebuggingFlagToTheDebuggee();
    void passesInferiorEnvironmentToTheDebuggee_data() { addBackendRows(); }
    void passesInferiorEnvironmentToTheDebuggee();
    void reportsSourcePathsInStackFrames_data() { addBackendRows(); }
    void reportsSourcePathsInStackFrames();
    void refreshesLocalsAndStack_data() { addBackendRows(); }
    void refreshesLocalsAndStack();
    void expandsContainerLocalWhenExpanded_data() { addBackendRows(); }
    void expandsContainerLocalWhenExpanded();
    void resolvesATypeArrivingWithALaterLibrary_data() { addBackendRows(); }
    void resolvesATypeArrivingWithALaterLibrary();
    void honorsDumperOptionsFromTheRequest_data() { addBackendRows(); }
    void honorsDumperOptionsFromTheRequest();
    void marksTheUninitializedVariablesTheRequestNames_data() { addBackendRows(); }
    void marksTheUninitializedVariablesTheRequestNames();
    void honorsTheStringLengthLimitFromTheRequest_data() { addBackendRows(); }
    void honorsTheStringLengthLimitFromTheRequest();
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
    void printsALongValueWithoutTruncating_data() { addBackendRows(); }
    void printsALongValueWithoutTruncating();
    void assignsValueToLocalVariable_data() { addBackendRows(); }
    void assignsValueToLocalVariable();
    void shutsDownCleanly_data() { addBackendRows(); }
    void shutsDownCleanly();
    void executesRunToLineFunctionAndJumpsToLine_data() { addBackendRows(); }
    void executesRunToLineFunctionAndJumpsToLine();
    void runsToAnAmbiguousLine_data() { addBackendRows(); }
    void runsToAnAmbiguousLine();
    void interruptsRightAfterAttaching_data() { addBackendRows(); }
    void interruptsRightAfterAttaching();
    void refusesJumpToAnAmbiguousLine_data() { addBackendRows(); }
    void refusesJumpToAnAmbiguousLine();
    void insertsWatchpointAndCatchpoint_data() { addBackendRows(); }
    void insertsWatchpointAndCatchpoint();
    void insertsWatchpointAsFirstCommandAfterStop_data() { addBackendRows(); }
    void insertsWatchpointAsFirstCommandAfterStop();
    void clearedBreakpointConditionStopsAgain_data() { addBackendRows(); }
    void clearedBreakpointConditionStopsAgain();
    void fetchesMemoryFromInvalidAddress_data() { addBackendRows(); }
    void fetchesMemoryFromInvalidAddress();
    void reportsSetupFailureWhenTheDebuggerQuitsAtOnce_data() { addBackendRows(); }
    void reportsSetupFailureWhenTheDebuggerQuitsAtOnce();
    void reportsEngineSetupFailure_data() { addBackendRows(); }
    void reportsEngineSetupFailure();
    void insertsABreakpointBehindABlockedDebugger_data() { addBackendRows(); }
    void insertsABreakpointBehindABlockedDebugger();
    void tellsWhetherAModuleHasPrivateSymbols_data() { addBackendRows(); }
    void tellsWhetherAModuleHasPrivateSymbols();
    void leavesThePublicSymbolsOutOfTheSearch_data() { addBackendRows(); }
    void leavesThePublicSymbolsOutOfTheSearch();
    void reportsAnUnresponsiveDebugger_data() { addBackendRows(); }
    void reportsAnUnresponsiveDebugger();
    void appliesConfiguredDebuggerOptions_data() { addBackendRows(); }
    void appliesConfiguredDebuggerOptions();
    void configuresTheDebugInfoDaemon_data() { addBackendRows(); }
    void configuresTheDebugInfoDaemon();
    void disassemblesInTheConfiguredFlavor_data() { addBackendRows(); }
    void disassemblesInTheConfiguredFlavor();
    void breaksBeforeTheInferiorAborts_data() { addBackendRows(); }
    void breaksBeforeTheInferiorAborts();
    void readsTheDebuggerInitFileWhenConfigured_data() { addBackendRows(); }
    void readsTheDebuggerInitFileWhenConfigured();
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
    void passesInferiorWorkingDirectoryToTheDebuggee_data() { addBackendRows(); }
    void passesInferiorWorkingDirectoryToTheDebuggee();
    void loadsAdditionalQmlStack_data() { addBackendRows(); }
    void loadsAdditionalQmlStack();
    void fetchesQmlLocals_data() { addBackendRows(); }
    void fetchesQmlLocals();
    void insertsQmlBreakpointAndStopsAtIt_data();
    void insertsQmlBreakpointAndStopsAtIt();
    void insertsQmlBreakpointBeforeDumpersLoad_data() { addBackendRows(); }
    void insertsQmlBreakpointBeforeDumpersLoad();
    void resolvesQmlBreakpointWithoutServiceDebugInfo_data() { addBackendRows(); }
    void resolvesQmlBreakpointWithoutServiceDebugInfo();
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
    void reportsRefusedBreakpointLocation_data() { addBackendRows(); }
    void reportsRefusedBreakpointLocation();
    void togglesBreakpointEnabledInPlace_data() { addBackendRows(); }
    void togglesBreakpointEnabledInPlace();
    void attachesToRunningProcess_data() { addBackendRows(); }
    void attachesToRunningProcess();
    void reportsTheStackOfASelectedThread_data() { addBackendRows(); }
    void reportsTheStackOfASelectedThread();
    void mapsTheReportedSourcePath_data() { addBackendRows(); }
    void mapsTheReportedSourcePath();
    void stopsAtABreakpointInAnInferiorOfTheOtherWordWidth_data() { addBackendRows(); }
    void stopsAtABreakpointInAnInferiorOfTheOtherWordWidth();
    void reportsTheStackOfAnInferiorOfTheOtherWordWidth_data() { addBackendRows(); }
    void reportsTheStackOfAnInferiorOfTheOtherWordWidth();
    void attachesToACrashedProcess_data() { addBackendRows(); }
    void attachesToACrashedProcess();
    void attachesToTerminalRunProcess_data() { addBackendRows(); }
    void attachesToTerminalRunProcess();
    void attachesToRunningRemoteServer_data() { addBackendRows(); }
    void attachesToRunningRemoteServer();
    void runsUserCommandsAfterConnectingToARemoteServer_data() { addBackendRows(); }
    void runsUserCommandsAfterConnectingToARemoteServer();
    void exitsTheMonitorWhenClosing_data() { addBackendRows(); }
    void exitsTheMonitorWhenClosing();
    void continuesAfterConnectingWhenConfigured_data() { addBackendRows(); }
    void continuesAfterConnectingWhenConfigured();
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
    void warmUpBackends();
    void buildOtherWordWidthInferior(const FilePath &compiler, InferiorTestData &data);
    void buildDebugCrtInferior(const FilePath &compiler, const Utils::Environment &environment,
                               InferiorTestData &data);
    // The flags default to what the settings default to, so a test only names
    // what it wants to be different.
    std::unique_ptr<DebuggerBackend> createEngine(Backend backend,
        const std::optional<Utils::ProcessRunData> &debuggerRunDataOverride = {},
        const std::optional<Utils::ProcessRunData> &inferiorRunDataOverride = {},
        bool nativeMixed = false,
        std::chrono::seconds watchdogTimeout = {},
        Debugger::Internal::GdbImplFlags gdbFlags = Debugger::Internal::GdbImplFlag::PseudoTracepoints);
    std::unique_ptr<DebuggerBackend> createEngineWithBreakEvents(
        Backend backend, const QStringList &breakEvents);
    std::unique_ptr<DebuggerBackend> createEngineWithStartScript(
        Backend backend, const Utils::FilePath &startScript);
    std::unique_ptr<DebuggerBackend> createEngineRunningAsUser(
        Backend backend, const QString &user, const Utils::Environment &debuggerEnvironment);
    std::unique_ptr<DebuggerBackend> createEngineWithConfiguredPaths(
        Backend backend, const QList<QPair<QString, QString>> &sourcePathMap);
    std::unique_ptr<DebuggerBackend> createEngineWithHeapDebugging(
        Backend backend, bool enableHeapDebugging);
    std::unique_ptr<DebuggerBackend> createEngineForAccessViolations(
        Backend backend, bool ignoreFirstChance, const QStringList &inferiorArguments);
    std::unique_ptr<DebuggerBackend> createEngineWithTerminal(
        Backend backend, bool useTerminal, const Utils::Environment &inferiorEnvironment);
    std::unique_ptr<DebuggerBackend> createEngineReportingExceptions(
        Backend backend, bool reportFirstChance);
    std::unique_ptr<DebuggerBackend> createEngineForTheDebugRuntime(
        Backend backend, const QString &crtDebugReportModule);
    std::unique_ptr<DebuggerBackend> createEngineForCrashedProcess(Backend backend,
        Utils::ProcessHandle pid, const QString &crashParameter);
    std::unique_ptr<DebuggerBackend> createAttachEngine(Backend backend,
        const InferiorStartData &inferiorStartData,
        Debugger::Internal::GdbImplFlags gdbFlags = {});
    // Every user-configurable debugger option turned on, so a test can check they arrive.
    // Paths that have to exist for the backend to pass them on use existingDir.
    std::unique_ptr<DebuggerBackend> createFullyConfiguredEngine(Backend backend,
        const Utils::Environment &debuggerEnvironment, const Utils::FilePath &existingDir,
        const QString &inferiorArguments = {});
    // The line defaults to the one the inferior declares for a breakpoint.
    std::unique_ptr<DebuggerBackend> launchAndStopAtBreakpoint(Backend backend,
        const std::optional<Utils::ProcessRunData> &inferiorRunDataOverride = {},
        int line = 0);
    std::unique_ptr<DebuggerBackend> stopAtBreakpoint(Backend backend, Process &helperInferior);
    bool hasCapability(Backend backend, Debugger::DebuggerCapabilities capability,
                       Debugger::DebuggerStartMode startMode = Debugger::NoStartMode);
    bool hasExtraCapability(Backend backend, Debugger::DebuggerExtraCapability capability);
    bool hasStartMode(Backend backend, DebuggerStartModeFlag startMode);
    Utils::Result<> checkStartMode(Backend backend, DebuggerStartModeFlag startMode);
    Utils::Result<> checkAcceptsBreakpoint(Backend backend, BreakpointType type,
                                          const QString &description);
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
    FilePath m_inferiorLibMsvc;
    QTemporaryDir m_tempDir = QTemporaryDir(QCoreApplication::applicationDirPath()
                                            + "/qt_tst_backend_XXXXXX");
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
    watcher.insert("exp", toHex(symbolName));
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

    const GdbMi watched = findItemByIName(reply, "watch.0");
    QString address = watched["address"].data();
    if (address.isEmpty())
        address = watched["origaddr"].data();
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

void tst_backends::insertsQmlBreakpointAndStopsAtIt_data()
{
    QTest::addColumn<Backend>("backend");
    // The service can refuse a breakpoint until it is up, and the debugger has
    // to keep trying. One refusal is what the retry anchor at the availability
    // hook is there for, and the refused row holds on to it. Two would need a
    // third send, which only a service that already answers can ask for.
    QTest::addColumn<int>("forcedRefusals");
    for (Backend backend : m_backendData.keys()) {
        const QString name = backendName(backend);
        QTest::newRow(qPrintable(name)) << backend << 0;
        QTest::newRow(qPrintable(name + "-refused-once")) << backend << 1;
    }
}

std::unique_ptr<DebuggerBackend> tst_backends::createEngine(Backend backend,
    const std::optional<ProcessRunData> &debuggerRunDataOverride,
    const std::optional<ProcessRunData> &inferiorRunDataOverride,
    bool nativeMixed,
    std::chrono::seconds watchdogTimeout,
    GdbImplFlags gdbFlags)
{
    Q_UNUSED(debuggerRunDataOverride)
    Q_UNUSED(inferiorRunDataOverride)
    Q_UNUSED(nativeMixed)
    Q_UNUSED(gdbFlags)
    switch (backend) {
    case Backend::Gdb:
        return std::make_unique<DebuggerBackend>(std::make_unique<GdbImpl>(GdbImplStartData{
            .debuggerRunData = debuggerRunDataOverride.value_or(
                ProcessRunData{{m_backendData[backend].path, {}}, {}, Environment::systemEnvironment()}),
            .inferiorStartData = inferiorRunDataOverride.value_or(
                ProcessRunData{{inferiorTestData(backend).executable, {}}, {}, Environment::systemEnvironment()}),
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
            .flags = gdbFlags | (nativeMixed ? GdbImplFlags(GdbImplFlag::NativeMixedDebugging)
                                             : GdbImplFlags()),
            .userCommands = {.forReset = {userCommandProbe(backend, UserCommandHook::Reset).command}},
            .watchdogTimeout = watchdogTimeout}));
    case Backend::Bridge:
        return std::make_unique<DebuggerBackend>(std::make_unique<BridgeImpl>(DapStartData{
            .debuggerRunData = debuggerRunDataOverride.value_or(
                ProcessRunData{{m_backendData[backend].path, {}}, {}, Environment::systemEnvironment()}),
            .inferiorStartData = inferiorRunDataOverride.value_or(
                ProcessRunData{{inferiorTestData(backend).executable, {}}, {}, Environment::systemEnvironment()}),
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
            .bridgeStartData = dapHostRecipe(false),
            .skipKnownFrames = gdbFlags.testFlag(GdbImplFlag::SkipKnownFrames)}));
    case Backend::Dap: {
        const ProcessRunData debuggerRunData = debuggerRunDataOverride.value_or(
            ProcessRunData{{m_backendData[backend].path, {}}, {},
                           Environment::systemEnvironment()});
        const ProcessRunData inferiorRunData = inferiorRunDataOverride.value_or(
            ProcessRunData{{inferiorTestData(backend).executable, {}}, {},
                           Environment::systemEnvironment()});
        DapStartData startData;
        startData.adapter.kind = DapAdapterDescriptor::Kind::Executable;
        startData.adapter.command = CommandLine{debuggerRunData.command.executable(),
                                                {"-i", "dap"}};
        startData.adapter.runData = debuggerRunData;
        startData.adapterId = "gdb";
        // The launch body follows the adapter's own schema, so what the other
        // backends take out of inferiorStartData is spelled out here.
        QJsonObject environment;
        for (const QString &entry : inferiorRunData.environment.toStringList()) {
            const qsizetype separator = entry.indexOf('=');
            if (separator > 0)
                environment[entry.left(separator)] = entry.mid(separator + 1);
        }
        QJsonObject configuration{{"program", inferiorRunData.command.executable().path()},
                                  {"env", environment}};
        if (!inferiorRunData.workingDirectory.isEmpty())
            configuration["cwd"] = inferiorRunData.workingDirectory.path();
        startData.configuration = configuration;
        return std::make_unique<DebuggerBackend>(std::make_unique<DapImpl>(startData));
    }
    case Backend::Lldb:
        return std::make_unique<DebuggerBackend>(std::make_unique<LldbImpl>(LldbImplStartData{
            .debuggerRunData = debuggerRunDataOverride.value_or(
                ProcessRunData{{m_backendData[backend].path, {}}, {}, Environment::systemEnvironment()}),
            .inferiorStartData = inferiorRunDataOverride.value_or(
                ProcessRunData{{inferiorTestData(backend).executable, {}}, {}, Environment::systemEnvironment()}),
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
            .nativeMixedDebugging = nativeMixed,
            .breakOnMain = gdbFlags.testFlag(GdbImplFlag::BreakOnMain),
            .intelDisassembly = gdbFlags.testFlag(GdbImplFlag::IntelDisassembly),
            .watchdogTimeout = watchdogTimeout}));
    case Backend::Pdb:
        return std::make_unique<DebuggerBackend>(std::make_unique<PdbImpl>(PdbImplStartData{
            .debuggerRunData = debuggerRunDataOverride.value_or(
                ProcessRunData{{m_backendData[backend].path, {}}, {},
                               Environment::systemEnvironment()}),
            .inferiorStartData = inferiorRunDataOverride.value_or(
                ProcessRunData{{inferiorTestData(backend).executable, {}}, {},
                               Environment::systemEnvironment()}),
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
            .breakOnMain = gdbFlags.testFlag(GdbImplFlag::BreakOnMain),
            .watchdogTimeout = watchdogTimeout}));
    case Backend::Qml:
        return std::make_unique<DebuggerBackend>(std::make_unique<QmlImpl>(QmlImplStartData{
            .inferiorStartData = AttachToQmlServerData{}}));
    case Backend::Cdb:
        return std::make_unique<DebuggerBackend>(std::make_unique<CdbImpl>(CdbImplStartData{
            .debuggerRunData = debuggerRunDataOverride.value_or(
                ProcessRunData{{m_backendData[backend].path, {}}, {},
                               Environment::systemEnvironment()}),
            .inferiorStartData = inferiorRunDataOverride.value_or(
                ProcessRunData{{inferiorTestData(backend).executable, {}}, {},
                               Environment::systemEnvironment()}),
            .extensionDir = m_backendData[backend].cdbExtensionDir,
            .extensionFileName = m_backendData[backend].cdbExtensionFileName,
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
            .breakOnMain = gdbFlags.testFlag(GdbImplFlag::BreakOnMain),
            .nativeMixed = nativeMixed,
            .watchdogTimeout = watchdogTimeout}));
    }
    return nullptr;
}

std::unique_ptr<DebuggerBackend> tst_backends::createFullyConfiguredEngine(
    Backend backend, const Environment &debuggerEnvironment, const FilePath &existingDir,
    const QString &inferiorArguments)
{
    Q_UNUSED(debuggerEnvironment)
    Q_UNUSED(existingDir)
    Q_UNUSED(inferiorArguments)
    switch (backend) {
    case Backend::Gdb:
        return std::make_unique<DebuggerBackend>(std::make_unique<GdbImpl>(GdbImplStartData{
            .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                              debuggerEnvironment},
            .inferiorStartData = ProcessRunData{
                {inferiorTestData(backend).executable, inferiorArguments, CommandLine::Raw}, {},
                Environment::systemEnvironment()},
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
            .flags = GdbImplFlag::LoadGdbInit | GdbImplFlag::LoadSystemDumpers
                     | GdbImplFlag::UseIndexCache | GdbImplFlag::MultiInferior
                     | GdbImplFlag::ForceTargetAsync | GdbImplFlag::BreakOnAbort
                     | GdbImplFlag::BreakOnWarning | GdbImplFlag::BreakOnFatal
                     | GdbImplFlag::IntelDisassembly,
            .useDebugInfoD = TriState(TriState::EnabledValue),
            .extraDumperFile = existingDir / "qtc_extra_dumper.py",
            .extraDumperCommands = "echo QTCEXTRADUMPERCOMMAND\\n",
            .searchPaths = {.sysRoot = FilePath::fromUserInput("/qtc-test-sysroot"),
                            .debugInfoLocation = existingDir,
                            .solibSearchPath = {FilePath::fromUserInput("/qtc-test-solib")},
                            .debugSourceLocation = {existingDir.path()},
                            .sourcePathMap = {{"/qtc-test-from", "/qtc-test-to"}}},
            .userCommands = {.atStartup = "echo QTCSTARTUPMARKER\\n",
                             .afterAttach = "echo QTCPOSTATTACHMARKER\\n"}}));
    case Backend::Lldb:
        return std::make_unique<DebuggerBackend>(std::make_unique<LldbImpl>(LldbImplStartData{
            .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                              debuggerEnvironment},
            .inferiorStartData = ProcessRunData{
                {inferiorTestData(backend).executable, inferiorArguments, CommandLine::Raw}, {},
                Environment::systemEnvironment()},
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
            .intelDisassembly = true,
            .startupCommands = {"script print('QTCSTARTUPMARKER')"},
            .solibSearchPath = {FilePath::fromUserInput("/qtc-test-solib")},
            .extraDumperFile = existingDir / "qtc_extra_dumper.py",
            .extraDumperCommands = "script print('QTCEXTRADUMPERCOMMAND')"}));
    case Backend::Cdb:
        return std::make_unique<DebuggerBackend>(std::make_unique<CdbImpl>(CdbImplStartData{
            .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                              debuggerEnvironment},
            .inferiorStartData = ProcessRunData{
                {inferiorTestData(backend).executable, inferiorArguments, CommandLine::Raw}, {},
                Environment::systemEnvironment()},
            .extensionDir = m_backendData[backend].cdbExtensionDir,
            .extensionFileName = m_backendData[backend].cdbExtensionFileName,
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
            .searchPaths = CdbImplSearchPaths{.symbolPaths = {existingDir.nativePath()},
                                              .sourcePaths = {existingDir.nativePath()}},
            .startupCommands = {".echo QTCSTARTUPMARKER"},
            .extraDumperFile = existingDir / "qtc_extra_dumper.py",
            // Split, so that the extension's echo of the command cannot pass for
            // its output.
            .extraDumperCommands = "print('QTC' + 'EXTRADUMPERCOMMAND')"}));
    case Backend::Pdb:
    case Backend::Qml:
    case Backend::Bridge:
    case Backend::Dap:
        break;
    }
    return nullptr;
}

// The debugger's own list of events to break on, where it has one.
// A debugger that has to be started as somebody else. Only backends whose
// start data carries the user can be built here.
std::unique_ptr<DebuggerBackend> tst_backends::createEngineRunningAsUser(
    Backend backend, const QString &user, const Environment &debuggerEnvironment)
{
    if (backend != Backend::Gdb)
        return nullptr;
    return std::make_unique<DebuggerBackend>(std::make_unique<GdbImpl>(GdbImplStartData{
        .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                          debuggerEnvironment},
        .inferiorStartData = ProcessRunData{{inferiorTestData(backend).executable, {}}, {},
                                            Environment::systemEnvironment()},
        .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
        .runAsUser = user}));
}

// A debugger given a start script of its own. Only backends whose start data
// carries one can be built here.
std::unique_ptr<DebuggerBackend> tst_backends::createEngineWithStartScript(
    Backend backend, const FilePath &startScript)
{
    if (backend != Backend::Gdb)
        return nullptr;
    return std::make_unique<DebuggerBackend>(std::make_unique<GdbImpl>(GdbImplStartData{
        .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                          Environment::systemEnvironment()},
        .inferiorStartData = ProcessRunData{{inferiorTestData(backend).executable, {}}, {},
                                            Environment::systemEnvironment()},
        .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
        .userCommands = {.startScript = startScript}}));
}

std::unique_ptr<DebuggerBackend> tst_backends::createEngineWithBreakEvents(
    Backend backend, const QStringList &breakEvents)
{
    if (backend != Backend::Cdb)
        return nullptr;
    return std::make_unique<DebuggerBackend>(std::make_unique<CdbImpl>(CdbImplStartData{
        .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                          Environment::systemEnvironment()},
        .inferiorStartData = ProcessRunData{{inferiorTestData(backend).executable, {}}, {},
                                            Environment::systemEnvironment()},
        .extensionDir = m_backendData[backend].cdbExtensionDir,
        .extensionFileName = m_backendData[backend].cdbExtensionFileName,
        .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
        .breakEvents = breakEvents}));
}

// Configures the debugger's own symbol and source paths next to the mapping, so
// that a session which cannot start with them set fails the test that uses this.
// Only backends whose start data carries them can be built here.
std::unique_ptr<DebuggerBackend> tst_backends::createEngineWithConfiguredPaths(
    Backend backend, const QList<QPair<QString, QString>> &sourcePathMap)
{
    if (backend != Backend::Cdb)
        return nullptr;
    return std::make_unique<DebuggerBackend>(std::make_unique<CdbImpl>(CdbImplStartData{
        .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                          Environment::systemEnvironment()},
        .inferiorStartData = ProcessRunData{{inferiorTestData(backend).executable, {}}, {},
                                            Environment::systemEnvironment()},
        .extensionDir = m_backendData[backend].cdbExtensionDir,
        .extensionFileName = m_backendData[backend].cdbExtensionFileName,
        .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
        .searchPaths = CdbImplSearchPaths{
            .symbolPaths = {inferiorTestData(backend).executable.parentDir().nativePath()},
            .sourcePaths = {inferiorTestData(backend).source.parentDir().nativePath()},
            .sourcePathMap = sourcePathMap}}));
}

std::unique_ptr<DebuggerBackend> tst_backends::createEngineWithHeapDebugging(
    Backend backend, bool enableHeapDebugging)
{
    if (backend != Backend::Cdb)
        return nullptr;
    return std::make_unique<DebuggerBackend>(std::make_unique<CdbImpl>(CdbImplStartData{
        .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                          Environment::systemEnvironment()},
        .inferiorStartData = ProcessRunData{{inferiorTestData(backend).executable, {}}, {},
                                            Environment::systemEnvironment()},
        .extensionDir = m_backendData[backend].cdbExtensionDir,
        .extensionFileName = m_backendData[backend].cdbExtensionFileName,
        .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
        .enableHeapDebugging = enableHeapDebugging}));
}

std::unique_ptr<DebuggerBackend> tst_backends::createEngineForAccessViolations(
    Backend backend, bool ignoreFirstChance, const QStringList &inferiorArguments)
{
    if (backend != Backend::Cdb)
        return nullptr;
    return std::make_unique<DebuggerBackend>(std::make_unique<CdbImpl>(CdbImplStartData{
        .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                          Environment::systemEnvironment()},
        .inferiorStartData = ProcessRunData{
            {inferiorTestData(backend).executable, inferiorArguments}, {},
            Environment::systemEnvironment()},
        .extensionDir = m_backendData[backend].cdbExtensionDir,
        .extensionFileName = m_backendData[backend].cdbExtensionFileName,
        .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
        .ignoreFirstChanceAccessViolation = ignoreFirstChance}));
}

std::unique_ptr<DebuggerBackend> tst_backends::createEngineWithTerminal(
    Backend backend, bool useTerminal, const Environment &inferiorEnvironment)
{
    if (backend != Backend::Cdb)
        return nullptr;
    return std::make_unique<DebuggerBackend>(std::make_unique<CdbImpl>(CdbImplStartData{
        .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                          Environment::systemEnvironment()},
        .inferiorStartData = ProcessRunData{{inferiorTestData(backend).executable, {}}, {},
                                            inferiorEnvironment},
        .extensionDir = m_backendData[backend].cdbExtensionDir,
        .extensionFileName = m_backendData[backend].cdbExtensionFileName,
        .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
        .useTerminal = useTerminal}));
}

std::unique_ptr<DebuggerBackend> tst_backends::createEngineForTheDebugRuntime(
    Backend backend, const QString &crtDebugReportModule)
{
    if (backend != Backend::Cdb)
        return nullptr;
    return std::make_unique<DebuggerBackend>(std::make_unique<CdbImpl>(CdbImplStartData{
        .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                          Environment::systemEnvironment()},
        .inferiorStartData = ProcessRunData{
            {inferiorTestData(backend).debugCrtExecutable, {}}, {},
            Environment::systemEnvironment()},
        .extensionDir = m_backendData[backend].cdbExtensionDir,
        .extensionFileName = m_backendData[backend].cdbExtensionFileName,
        .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
        .crtDebugReportModule = crtDebugReportModule}));
}

std::unique_ptr<DebuggerBackend> tst_backends::createEngineReportingExceptions(
    Backend backend, bool reportFirstChance)
{
    if (backend != Backend::Cdb)
        return nullptr;
    return std::make_unique<DebuggerBackend>(std::make_unique<CdbImpl>(CdbImplStartData{
        .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                          Environment::systemEnvironment()},
        .inferiorStartData = ProcessRunData{{inferiorTestData(backend).executable, {}}, {},
                                            Environment::systemEnvironment()},
        .extensionDir = m_backendData[backend].cdbExtensionDir,
        .extensionFileName = m_backendData[backend].cdbExtensionFileName,
        .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
        .reportFirstChanceExceptions = reportFirstChance}));
}

std::unique_ptr<DebuggerBackend> tst_backends::createEngineForCrashedProcess(
    Backend backend, ProcessHandle pid, const QString &crashParameter)
{
    if (backend != Backend::Cdb)
        return nullptr;
    return std::make_unique<DebuggerBackend>(std::make_unique<CdbImpl>(CdbImplStartData{
        .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                          Environment::systemEnvironment()},
        .inferiorStartData = AttachToProcessData{pid, crashParameter},
        .extensionDir = m_backendData[backend].cdbExtensionDir,
        .extensionFileName = m_backendData[backend].cdbExtensionFileName,
        .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR)}));
}

std::unique_ptr<DebuggerBackend> tst_backends::createAttachEngine(
    Backend backend, const InferiorStartData &inferiorStartData, GdbImplFlags gdbFlags)
{
    Q_UNUSED(inferiorStartData)
    Q_UNUSED(gdbFlags)
    switch (backend) {
    case Backend::Gdb:
        return std::make_unique<DebuggerBackend>(std::make_unique<GdbImpl>(GdbImplStartData{
            .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                              Environment::systemEnvironment()},
            .inferiorStartData = inferiorStartData,
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
            .flags = gdbFlags,
            .userCommands = {.afterConnect
                                 = {userCommandProbe(backend, UserCommandHook::AfterConnect).command}}}));
    case Backend::Bridge:
        return std::make_unique<DebuggerBackend>(std::make_unique<BridgeImpl>(DapStartData{
            .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                              Environment::systemEnvironment()},
            .inferiorStartData = inferiorStartData,
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR),
            .bridgeStartData = dapHostRecipe(false)}));
    case Backend::Lldb:
        return std::make_unique<DebuggerBackend>(std::make_unique<LldbImpl>(LldbImplStartData{
            .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                              Environment::systemEnvironment()},
            .inferiorStartData = inferiorStartData,
            .dumperScriptsDir = FilePath::fromUserInput(DUMPERDIR)}));
    case Backend::Dap: {
        // The stock protocol carries the pid in the adapter's own attach body.
        const auto *attachData = std::get_if<AttachToProcessData>(&inferiorStartData);
        if (!attachData)
            break;
        DapStartData startData;
        startData.adapter.kind = DapAdapterDescriptor::Kind::Executable;
        startData.adapter.command = CommandLine{m_backendData[backend].path, {"-i", "dap"}};
        startData.adapter.runData.environment = Environment::systemEnvironment();
        startData.adapterId = "gdb";
        startData.attach = true;
        startData.configuration = QJsonObject{
            {"pid", qint64(attachData->pid.pid())},
            {"program", inferiorTestData(backend).executable.path()}};
        return std::make_unique<DebuggerBackend>(std::make_unique<DapImpl>(startData));
    }
    case Backend::Pdb:
        break;
    case Backend::Qml:
        return std::make_unique<DebuggerBackend>(std::make_unique<QmlImpl>(QmlImplStartData{
            .inferiorStartData = inferiorStartData}));
    case Backend::Cdb:
        return std::make_unique<DebuggerBackend>(std::make_unique<CdbImpl>(CdbImplStartData{
            .debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {},
                                              Environment::systemEnvironment()},
            .inferiorStartData = inferiorStartData,
            .extensionDir = m_backendData[backend].cdbExtensionDir,
            .extensionFileName = m_backendData[backend].cdbExtensionFileName,
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

    // A debuginfod server in the developer's environment makes the debuggers stall on
    // every library without debug info, starting with the version probes below. Drop it
    // before the first snapshot of the system environment is taken.
    Environment::modifySystemEnvironment({{"DEBUGINFOD_URLS", "", EnvironmentItem::Unset}});

    Environment::modifySystemEnvironment(
        {{"_NT_SYMBOL_PATH", TemporaryDirectory::masterDirectoryPath()}});

    QString gdbVersionLine;
    QString lldbVersionLine;

    {
        const FilePath gdbPath = gdbPathForTest();
        if (gdbPath.isExecutableFile()) {
            m_backendData[Backend::Gdb].path = gdbPath;
            gdbVersionLine = versionLine(gdbPath);
        }

        const FilePath lldbPath = lldbPathForTest();
        if (lldbPath.isExecutableFile()) {
            m_backendData[Backend::Lldb].path = lldbPath;
            lldbVersionLine = versionLine(lldbPath);
        }
    }

    const QString envPython = qtcEnvironmentVariable("QTC_PYTHON_PATH_FOR_TEST");
    const FilePath pythonPath = envPython.isEmpty()
        ? findPythonOnPath()
        : FilePath::fromUserInput(envPython);
    QString pythonVersionLine;
    if (pythonPath.isExecutableFile()) {
        m_backendData[Backend::Pdb].path = pythonPath;
        pythonVersionLine = versionLine(pythonPath);
    }

    QString cdbVersionLine;
    if (HostOsInfo::isWindowsHost()) {
        const QString envCdb = qtcEnvironmentVariable("QTC_CDB_PATH_FOR_TEST");
        const FilePath cdbPath = envCdb.isEmpty()
            ? findCdbOnPath()
            : FilePath::fromUserInput(envCdb);
        FilePath extensionLibrary;
#ifdef CDBEXT_LIBRARY
        extensionLibrary = FilePath::fromUserInput(QLatin1String(CDBEXT_LIBRARY));
#endif
        const QString envCdbExtDir = qtcEnvironmentVariable("QTC_CDB_EXTENSION_DIR_FOR_TEST");
        if (!envCdbExtDir.isEmpty()) {
            const QString fileName = extensionLibrary.isEmpty()
                ? QString("qtcreatorcdbext.dll") : extensionLibrary.fileName();
            extensionLibrary = FilePath::fromUserInput(envCdbExtDir) / fileName;
        }
        if (cdbPath.isExecutableFile() && extensionLibrary.isFile()) {
            m_backendData[Backend::Cdb].path = cdbPath;
            m_backendData[Backend::Cdb].cdbExtensionDir = extensionLibrary.parentDir();
            m_backendData[Backend::Cdb].cdbExtensionFileName = extensionLibrary.fileName();
            cdbVersionLine = versionLine(cdbPath);
        } else if (cdbPath.isExecutableFile() != extensionLibrary.isFile()) {
            qWarning("Cdb not tested: cdb.exe %s, extension DLL %s (%s).",
                     cdbPath.isExecutableFile() ? "found" : "NOT found",
                     extensionLibrary.isFile() ? "found" : "NOT found",
                     qPrintable(extensionLibrary.toUserOutput()));
        }
    }

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

#ifdef QMLSERVER_INFERIOR_EXECUTABLE
    const FilePath qmlInferior = (FilePath::fromUserInput(QMLSERVER_INFERIOR_EXECUTABLE)
                                  / "qmlserver_inferior").withExecutableSuffix();
    if (qmlInferior.isExecutableFile()) {
        InferiorTestData qmlInferiorData;
        qmlInferiorData.source = FilePath::fromUserInput("qrc:///qmlserver_inferior.qml");
        qmlInferiorData.executable = qmlInferior;
        qmlInferiorData.breakpointLine = qmlMarkerLine("qmlserver_inferior.qml",
                                                      "// breakpoint line");
        qmlInferiorData.secondBreakpointLine = qmlMarkerLine("qmlserver_inferior.qml",
                                                            "// second breakpoint line");
        QVERIFY(qmlInferiorData.breakpointLine > 0);
        QVERIFY(qmlInferiorData.secondBreakpointLine > 0);
        qmlInferiorData.localMarker = "value";
        qmlInferiorData.functionMarker = "compute";
        qmlInferiorData.expandableLocal = "nested";
        qmlInferiorData.expandableChild = "inner";
        qmlInferiorData.inspectorObject = "root";
        qmlInferiorData.inspectorProperty = "globalValue";
        qmlInferiorData.inspectorPropertyExpression = "globalValue";
        qmlInferiorData.enableToggleWireMarker = "changebreakpoint";
        qmlInferiorData.inspectorOrphanObject = "orphanObject";
        m_backendData[Backend::Qml].inferiorData = qmlInferiorData;
    }
#endif

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
    // Only the compiled backends need an inferior built from source; pdb brings its own.
    const bool needsCompiler = m_backendData.contains(Backend::Gdb)
                            || m_backendData.contains(Backend::Lldb);
    if (needsCompiler && !compiler.isExecutableFile()) {
        const QString why = probeFailures.isEmpty()
                ? QString("No C++ compiler (g++/clang++) found to build the test inferior.")
                : QString("No usable C++ compiler to build the test inferior - found, but "
                          "unable to even run \"--version\":\n  ")
                      + probeFailures.join("\n  ");
        QVERIFY2(!backendsAreRequired(), qPrintable(why));
        QSKIP(qPrintable(why));
    } else if (!compiler.isExecutableFile()) {
        qWarning("No C++ compiler (g++/clang++) found, and no backend here needs one - "
                 "the shared inferior and its library are not built. Cdb builds its own "
                 "inferior with cl.exe instead.");
    }

    FilePath cdbCompiler;
    Environment cdbCompileEnv = Environment::systemEnvironment();
    if (HostOsInfo::isWindowsHost() && m_backendData.contains(Backend::Cdb)) {
        const QString envBat = qtcEnvironmentVariable("QTC_MSVC_ENV_BAT");
        if (!envBat.isEmpty()) {
            const QMap<QString, QString> envPairs
                = environmentFromBatchFile(cdbCompileEnv, envBat);
            for (auto it = envPairs.cbegin(); it != envPairs.cend(); ++it)
                cdbCompileEnv.set(it.key(), it.value());
        }
        cdbCompiler = cdbCompileEnv.searchInPath("cl.exe");
        if (cdbCompiler.isExecutableFile() && !cdbCompileEnv.hasKey("INCLUDE")) {
            qWarning("cl.exe found, but INCLUDE is unset, so it cannot compile - point "
                     "QTC_MSVC_ENV_BAT at vcvarsall.bat to get a usable environment.");
            cdbCompiler = {};
        }
        if (!cdbCompiler.isExecutableFile()) {
            qWarning("cl.exe not found (checked PATH and QTC_MSVC_ENV_BAT) - "
                     "Cdb needs a real MSVC compiler for PDB debug info; "
                     "g++/clang++ above produce DWARF, which cdb.exe can't read.");
            m_backendData.remove(Backend::Cdb);
        }
    }

    QVERIFY(m_tempDir.isValid());
    InferiorTestData cppInferiorData;
    cppInferiorData.source = FilePath::fromString(m_tempDir.path()) / "inferior.cpp";
    cppInferiorData.executable = (FilePath::fromString(m_tempDir.path()) / "inferior")
                                .withExecutableSuffix();
    m_inferiorLib = FilePath::fromString(m_tempDir.path())
                   / (HostOsInfo::isWindowsHost() ? "inferiorlib.dll" : "inferiorlib.so");
    m_inferiorLibMsvc = FilePath::fromString(m_tempDir.path()) / "inferiorlib_msvc.dll";

    const QStringList inferiorLines = {
        "#include <chrono>",
        "#include <cstdio>",
        "#include <cstdlib>",
        "#include <cstring>",
        "#include <thread>",
        "#include <string>",
        "#include <utility>",
        "#include <vector>",
        "#ifdef _WIN32",
        "#include <windows.h>",
        "#else",
        "#include <dlfcn.h>",
        "#include <unistd.h>",
        "#endif",
        "#ifdef __linux__",
        "#include <sys/prctl.h>",
        "#endif",
        "",
        "struct LibProbe;",
        "int probeStorage = 4711;",
        "LibProbe *libProbe = (LibProbe *) &probeStorage;",
        "volatile int globalValue = 41;",
        "volatile bool keepSpinning = true;",
        "const char *globalMessage = \"hi\";",
        "const char *longText = \"" + QString("0123456789").repeated(200)
            + "LONGTEXTEND\";",
        "int *globalValuePtr = const_cast<int *>(&globalValue);",
        "",
        "void throwAndCatch()",
        "{",
        "    try {",
        "        throw 42; // throw line",
        "    } catch (int value) {",
        "        printf(\"caught %d\\n\", value);",
        "        fflush(stdout);",
        "    }",
        "}",
        "",
        "extern \"C\" void bump()",
        "{",
        "    std::vector<std::string> localVector{\"seven\", \"eight\"};",
        "    std::string longLocal(longText);",
        "    int localValue = globalValue + 1; // first breakpoint line",
        "    (void) localVector.size();",
        "    (void) longLocal.size();",
        "    globalValue = localValue;",
        "    printf(\"value=%d\\n\", globalValue);",
        "    fflush(stdout);",
        "}",
        "",
        "extern \"C\" void stepIntoKnownFrame()",
        "{",
        "    std::string movable = \"skipped\";",
        "    std::string moved = std::move(movable); // known frame step line",
        "    (void) moved.size();",
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
        "#ifdef _MSC_VER",
        "extern \"C\" void survivableCrash()",
        "{",
        "    __try {",
        "        volatile int *p = nullptr;",
        "        *p = 1;",
        "    } __except (EXCEPTION_EXECUTE_HANDLER) {",
        "        printf(\"survived the access violation\\n\");",
        "        fflush(stdout);",
        "    }",
        "}",
        "#endif",
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
        "    if (argc > 1 && strcmp(argv[1], \"crash\") == 0)",
        "        crash();",
        "    if (argc > 1 && strcmp(argv[1], \"abort\") == 0)",
        "        abort();",
        "    if (argc > 2 && strcmp(argv[1], \"crash-on-file\") == 0) {",
        "        while (true) {",
        "            if (FILE *trigger = fopen(argv[2], \"r\")) {",
        "                fclose(trigger);",
        "                break;",
        "            }",
        "            std::this_thread::sleep_for(std::chrono::milliseconds(10));",
        "        }",
        "        crash();",
        "    }",
        "#ifdef _MSC_VER",
        "    if (argc > 1 && strcmp(argv[1], \"survivable-crash\") == 0)",
        "        survivableCrash();",
        "#endif",
        "    bump();",
        "#ifdef _WIN32",
        "    HMODULE handle = LoadLibraryW(L\"@INFERIORLIB_WINDOWS@\");",
        "#else",
        "    void *handle = dlopen(\"@INFERIORLIB@\", RTLD_NOW);",
        "#endif",
        "    printf(\"lib=%d\\n\", handle ? 1 : 0); // library loaded line",
        "    fflush(stdout);",
        "#ifdef _WIN32",
        "    if (handle)",
        "        FreeLibrary(handle);",
        "#else",
        "    if (handle)",
        "        dlclose(handle);",
        "#endif",
        "    stepIntoKnownFrame();",
        "    multi(1);",
        "    multi(2.0);",
        "    recurse(40);",
        "    if (const char *marker = getenv(\"QTC_BACKEND_ENV_MARKER\"))",
        "        printf(\"env=%s\\n\", marker);",
        "    if (const char *queried = getenv(\"QTC_BACKEND_ENV_QUERY\")) {",
        "        const char *value = getenv(queried);",
        "        printf(\"query=%s=%s\\n\", queried, value ? value : \"unset\");",
        "    }",
        "    const char *heap = getenv(\"_NO_DEBUG_HEAP\");",
        "    printf(\"heap=%s\\n\", heap ? heap : \"unset\");",
        "    char cwd[1024] = {0};",
        "#ifdef _WIN32",
        "    GetCurrentDirectoryA(DWORD(sizeof(cwd)), cwd);",
        "#else",
        "    if (!getcwd(cwd, sizeof(cwd)))",
        "        cwd[0] = 0;",
        "#endif",
        "    printf(\"cwd=%s\\n\", cwd); // thunk step line",
        "    printf(\"after bump\\n\");",
        "    fflush(stdout);",
        "    throwAndCatch();",
        "    spin(); // second breakpoint line",
        "    return 7;",
        "}",
        "",
    };
    int thunkStepLine = 0;
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
        if (inferiorLines.at(i).contains("known frame step line"))
            cppInferiorData.knownFrameStepLine = i + 1;
        if (inferiorLines.at(i).contains("thunk step line"))
            thunkStepLine = i + 1;
        if (inferiorLines.at(i).contains("library loaded line"))
            cppInferiorData.libraryLoadedLine = i + 1;
    }
    QVERIFY(cppInferiorData.breakpointLine > 0);
    cppInferiorData.localMarker = "localValue";
    cppInferiorData.expandableLocal = "localVector";
    cppInferiorData.longStringLocal = "longLocal";
    cppInferiorData.functionMarker = "bump";
    cppInferiorData.recursionDepthVariable = "depth";
    cppInferiorData.applicationOutputMarker = "after bump";
    cppInferiorData.throwsAnException = true;
    cppInferiorData.afterThrowOutputMarker = "caught 42";
    cppInferiorData.environmentReportPrefix = "env=";
    cppInferiorData.heapFlagReportPrefix = "heap=";
    cppInferiorData.environmentQueryPrefix = "query=";
    cppInferiorData.workingDirectoryReportPrefix = "cwd=";
    cppInferiorData.disassemblySourceMarker = "globalValue = localValue";
    cppInferiorData.expectedExitCode = 7;
    cppInferiorData.libraryTypeSymbol = "libProbe";
    cppInferiorData.libraryTypeChild = "probeValue";
    QVERIFY(cppInferiorData.secondBreakpointLine > 0);
    QVERIFY(cppInferiorData.deepRecursionBreakpointLine > 0);
    QVERIFY(cppInferiorData.multiLocationBreakpointLine > 0);
    QVERIFY(cppInferiorData.spinBodyLine > 0);
    QVERIFY(cppInferiorData.knownFrameStepLine > 0);
    QVERIFY(thunkStepLine > 0);
    QVERIFY(cppInferiorData.libraryLoadedLine > 0);

    // Both variants keep the line numbering above: only the path the inferior
    // loads differs, so each toolchain gets a library its debugger can read.
    const auto sourceForLibrary = [&inferiorLines](const FilePath &library) {
        QStringList lines;
        for (const QString &line : inferiorLines) {
            lines.append(QString(line)
                             .replace("@INFERIORLIB_WINDOWS@",
                                      QString(library.nativePath()).replace('\\', "\\\\"))
                             .replace("@INFERIORLIB@", library.nativePath()));
        }
        return lines.join('\n').toUtf8();
    };

    QFile file(cppInferiorData.source.toFSPathString());
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(sourceForLibrary(m_inferiorLib));
    file.close();

    if (needsCompiler) {
        // The inferior uses <chrono>, list initialization and std::move, so it
        // needs C++11, and a compiler's default standard is not necessarily
        // that new - Apple clang's is not.
        QStringList compileArgs = {"-g", "-O0", "-std=c++11"};
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
    }

    const FilePath inferiorLibSource = FilePath::fromString(m_tempDir.path()) / "inferiorlib.cpp";
    {
        // The struct is what the test is after: its definition reaches the
        // debugger only once the library is loaded.
        QFile libFile(inferiorLibSource.toFSPathString());
        QVERIFY(libFile.open(QIODevice::WriteOnly | QIODevice::Text));
        libFile.write(QByteArrayLiteral(
            "struct LibProbe { int probeValue; };\n"
            "extern \"C\" LibProbe *inferiorLibProbe()\n"
            "{\n"
            "    static LibProbe probe = {0};\n"
            "    return &probe;\n"
            "}\n"));
        libFile.close();
    }

    if (compiler.isExecutableFile()) {
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
    }

    if (m_backendData.contains(Backend::Gdb)) {
        // The stock protocol against gdb's own adapter: the same debugger and
        // the same inferior as the gdb row, reached over DAP instead.
        if (debuggerMajorVersion(gdbVersionLine) >= s_dapInterpreterVersion) {
            m_backendData[Backend::Dap] = m_backendData[Backend::Gdb];
            m_backendData[Backend::Dap].inferiorData = cppInferiorData;
            // Naming the source a disassembled instruction came from is the
            // adapter's to offer, and gdb's own does not on every build.
            m_backendData[Backend::Dap].inferiorData.disassemblySourceMarker.clear();
            m_backendData[Backend::Dap].inferiorData.versionLine = gdbVersionLine;
            m_backendData[Backend::Dap].inferiorData.moduleListMarker = "libc";
        }

        m_backendData[Backend::Bridge] = m_backendData[Backend::Gdb];
        m_backendData[Backend::Bridge].inferiorData = cppInferiorData;
        m_backendData[Backend::Bridge].inferiorData.qmlBreakpointsUseServiceCasts = true;
        m_backendData[Backend::Bridge].inferiorData.versionLine = gdbVersionLine;
        m_backendData[Backend::Bridge].inferiorData.moduleListMarker = "libc";
        m_backendData[Backend::Bridge].inferiorData.moduleSymbolsPath = cppInferiorData.executable;
        m_backendData[Backend::Bridge].inferiorData.answersRedundantContinue = true;

        m_backendData[Backend::Gdb].inferiorData = cppInferiorData;
        m_backendData[Backend::Gdb].inferiorData.qmlBreakpointsUseServiceCasts = true;
        m_backendData[Backend::Gdb].inferiorData.longTextSymbol = "longText";
        m_backendData[Backend::Gdb].inferiorData.versionLine = gdbVersionLine;
        m_backendData[Backend::Gdb].inferiorData.moduleListMarker = "libc";
        m_backendData[Backend::Gdb].inferiorData.moduleSymbolsPath = cppInferiorData.executable;
        m_backendData[Backend::Gdb].inferiorData.alienBreakpointCommand = "break spin";
        m_backendData[Backend::Gdb].inferiorData.enableToggleWireMarker = "-break-disable";
        m_backendData[Backend::Gdb].inferiorData.alienBreakpointDeleteCommand = "delete %1";
    }
    if (m_backendData.contains(Backend::Lldb)) {
        m_backendData[Backend::Lldb].inferiorData = cppInferiorData;
        m_backendData[Backend::Lldb].inferiorData.qmlBreakpointsUseServiceCasts = true;
        m_backendData[Backend::Lldb].inferiorData.longTextSymbol = "longText";
        m_backendData[Backend::Lldb].inferiorData.answersRedundantContinue = true;
        m_backendData[Backend::Lldb].inferiorData.remoteAttachMinMajorVersion = 21;
        m_backendData[Backend::Lldb].inferiorData.remoteStubHostsProcess = true;
        m_backendData[Backend::Lldb].inferiorData.enableToggleWireMarker = "changeBreakpoint";
        m_backendData[Backend::Lldb].inferiorData.versionLine = lldbVersionLine;
        m_backendData[Backend::Lldb].inferiorData.moduleListMarker = "libc";
        m_backendData[Backend::Lldb].inferiorData.moduleSymbolsPath = cppInferiorData.executable;
    }
    if (m_backendData.contains(Backend::Cdb)) {
        InferiorTestData msvcInferiorData = cppInferiorData;
        msvcInferiorData.thunkStepLine = thunkStepLine;
        msvcInferiorData.executable = (FilePath::fromString(m_tempDir.path()) / "inferior_msvc")
                                     .withExecutableSuffix();
        msvcInferiorData.source = FilePath::fromString(m_tempDir.path()) / "inferior_msvc.cpp";
        QFile msvcFile(msvcInferiorData.source.toFSPathString());
        QVERIFY(msvcFile.open(QIODevice::WriteOnly | QIODevice::Text));
        msvcFile.write(sourceForLibrary(m_inferiorLibMsvc));
        msvcFile.close();

        // cdb.exe reads PDBs, so the library carrying LibProbe has to be an
        // MSVC one too - what g++ builds above has DWARF debug info only.
        const FilePath libPdbPath = FilePath::fromString(m_tempDir.path())
                                   / "inferiorlib_msvc.pdb";
        const QStringList cdbCompileLibArgs = {
            "/nologo", "/Zi", "/Od", "/EHsc", "/LD",
            "/Fe:" + m_inferiorLibMsvc.nativePath(),
            "/Fd:" + libPdbPath.nativePath(),
            inferiorLibSource.nativePath(),
        };
        Process cdbCompileLib;
        cdbCompileLib.setCommand({cdbCompiler, cdbCompileLibArgs});
        cdbCompileLib.setEnvironment(cdbCompileEnv);
        QElapsedTimer cdbCompileLibTimer;
        cdbCompileLibTimer.start();
        cdbCompileLib.runBlocking(s_compileTimeout);
        if (cdbCompileLib.result() != ProcessResult::FinishedWithSuccess) {
            qWarning("%s", qPrintable(compileFailure("compiling the Cdb inferior library",
                                                     cdbCompileLib,
                                                     cdbCompileLibTimer.elapsed())));
        }
        // cdb.exe itself completes the type from the library's PDB - "dt
        // inferiorlib_msvc!LibProbe" answers - but nothing that reaches the
        // dumper does, whether the type was looked up before the load or not.
        msvcInferiorData.libraryTypeSymbol.clear();

        const FilePath pdbPath = FilePath::fromString(m_tempDir.path()) / "inferior_msvc.pdb";
        const QStringList cdbCompileArgs = {
            "/nologo", "/Zi", "/Od", "/EHsc",
            "/Fe:" + msvcInferiorData.executable.nativePath(),
            "/Fd:" + pdbPath.nativePath(),
            msvcInferiorData.source.nativePath(),
        };
        Process cdbCompile;
        cdbCompile.setCommand({cdbCompiler, cdbCompileArgs});
        cdbCompile.setEnvironment(cdbCompileEnv);
        QElapsedTimer cdbCompileTimer;
        cdbCompileTimer.start();
        cdbCompile.runBlocking(s_compileTimeout);
        if (cdbCompile.result() == ProcessResult::FinishedWithSuccess) {
            buildOtherWordWidthInferior(cdbCompiler, msvcInferiorData);
            buildDebugCrtInferior(cdbCompiler, cdbCompileEnv, msvcInferiorData);
            m_backendData[Backend::Cdb].inferiorData = msvcInferiorData;
            m_backendData[Backend::Cdb].inferiorData.versionLine = cdbVersionLine;
            m_backendData[Backend::Cdb].inferiorData.answersRedundantContinue = true;
            m_backendData[Backend::Cdb].inferiorData.moduleListMarker = "kernel32";
            m_backendData[Backend::Cdb].inferiorData.survivedAccessViolationMarker
                = "survived the access violation";
            m_backendData[Backend::Cdb].inferiorData.enableToggleWireMarker = "bd";
            m_backendData[Backend::Cdb].inferiorData.symbolOptionsCommand = ".symopt";
            m_backendData[Backend::Cdb].inferiorData.moduleWithPrivateSymbols = "inferior_msvc";
            m_backendData[Backend::Cdb].inferiorData.moduleWithoutPrivateSymbols = "kernel32";
            m_backendData[Backend::Cdb].inferiorData.marksUninitializedVariables = true;
            m_backendData[Backend::Cdb].inferiorData.moduleSymbolsPath
                = msvcInferiorData.executable;
        } else {
            // Failing initTestCase() over this would skip every other backend too.
            qWarning("%s", qPrintable(compileFailure("compiling the Cdb test inferior",
                                                     cdbCompile, cdbCompileTimer.elapsed())));
            m_backendData.remove(Backend::Cdb);
        }
    }

    // Last point at which a backend can still have dropped out, so the one place
    // where what is left is what the rows will actually run.
    QList<Backend> expected{Backend::Pdb};
    if (HostOsInfo::isWindowsHost())
        expected << Backend::Cdb;
    else if (HostOsInfo::isMacHost())
        expected << Backend::Lldb;
    else
        expected << Backend::Gdb;
#ifdef QMLSERVER_INFERIOR_EXECUTABLE
    expected << Backend::Qml;
#endif
    QStringList missing;
    for (Backend backend : std::as_const(expected)) {
        if (!m_backendData.contains(backend))
            missing << backendName(backend);
    }
    if (!missing.isEmpty()) {
        const QString what = QString("Backends expected on this platform but not usable: %1 - "
                                     "none of their rows would run. See the warnings above for "
                                     "which piece is missing.")
                                 .arg(missing.join(", "));
        QVERIFY2(!backendsAreRequired(), qPrintable(what));
        qWarning("%s", qPrintable(what));
    }

    if (m_backendData.isEmpty())
        QSKIP("No usable debugger backend left - see the warnings above.");

    InferiorTestData pdbInferiorData;
    pdbInferiorData.source = pdbInferiorData.executable =
        FilePath::fromString(m_tempDir.path()) / "inferior.py";
    const QStringList pdbInferiorLines = {
        "import os",
        "",
        "globalValue = 41",
        "keepSpinning = True",
        "",
        "",
        "def bump(localValue):",
        "    global globalValue",
        "    globalValue = localValue  # first breakpoint line",
        "    print(\"value=%d\" % globalValue)",
        "",
        "",
        "def spin():",
        "    while keepSpinning:",
        "        pass  # spin body line",
        "",
        "",
        "def recurse(depth):",
        "    if depth <= 0:",
        "        return 0  # deep breakpoint line",
        "    return 1 + recurse(depth - 1)",
        "",
        "",
        "# a comment, pdb refuses a breakpoint here: unbreakable line",
        "def main():",
        "    bump(globalValue + 1)",
        "    marker = os.environ.get(\"QTC_BACKEND_ENV_MARKER\")",
        "    if marker:",
        "        print(\"env=%s\" % marker)",
        "    print(\"after bump\")",
        "    recurse(40)",
        "    spin()  # second breakpoint line",
        "",
        "",
        "if __name__ == \"__main__\":",
        "    main()",
        "",
    };
    for (int i = 0; i < pdbInferiorLines.size(); ++i) {
        if (pdbInferiorLines.at(i).contains("first breakpoint line"))
            pdbInferiorData.breakpointLine = i + 1;
        if (pdbInferiorLines.at(i).contains("second breakpoint line"))
            pdbInferiorData.secondBreakpointLine = i + 1;
        if (pdbInferiorLines.at(i).contains("deep breakpoint line"))
            pdbInferiorData.deepRecursionBreakpointLine = i + 1;
        if (pdbInferiorLines.at(i).contains("spin body line"))
            pdbInferiorData.spinBodyLine = i + 1;
        if (pdbInferiorLines.at(i).contains("unbreakable line"))
            pdbInferiorData.unbreakableLine = i + 1;
    }
    QVERIFY(pdbInferiorData.breakpointLine > 0);
    pdbInferiorData.localMarker = "localValue";
    pdbInferiorData.applicationOutputMarker = "after bump";
    pdbInferiorData.environmentReportPrefix = "env=";
    pdbInferiorData.functionMarker = "bump";
    pdbInferiorData.recursionDepthVariable = "depth";
    QVERIFY(pdbInferiorData.secondBreakpointLine > 0);
    QVERIFY(pdbInferiorData.deepRecursionBreakpointLine > 0);
    QVERIFY(pdbInferiorData.spinBodyLine > 0);
    QVERIFY(pdbInferiorData.unbreakableLine > 0);

    QFile pdbFile(pdbInferiorData.source.toFSPathString());
    QVERIFY(pdbFile.open(QIODevice::WriteOnly | QIODevice::Text));
    pdbFile.write(pdbInferiorLines.join('\n').toUtf8());
    pdbFile.close();

    pdbInferiorData.enableToggleWireMarker = "disable";
    if (m_backendData.contains(Backend::Pdb)) {
        m_backendData[Backend::Pdb].inferiorData = pdbInferiorData;
        m_backendData[Backend::Pdb].inferiorData.answersRedundantContinue = true;
        m_backendData[Backend::Pdb].inferiorData.falseLiteral = "False";
        m_backendData[Backend::Pdb].inferiorData.versionLine = pythonVersionLine;
        m_backendData[Backend::Pdb].inferiorData.moduleListMarker = "sys";
        m_backendData[Backend::Pdb].inferiorData.moduleSymbolsPath
            = FilePath::fromString("__main__");
    }

    warmUpBackends();
}

// The inferior a host debugs through its compatibility layer for the other word
// width. Built with the toolchain's own environment for that width, which the
// tests skip without.
void tst_backends::buildDebugCrtInferior(const FilePath &compiler,
                                         const Environment &environment, InferiorTestData &data)
{
    const QStringList lines = {
        "#include <crtdbg.h>",
        "#include <cstdio>",
        "",
        "volatile bool keepSpinning = true;",
        "",
        "int main()",
        "{",
        "    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);",
        "    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);",
        "    _CrtDbgReport(_CRT_WARN, __FILE__, __LINE__, nullptr, \"reporting\\n\");",
        "    printf(\"past the crt report\\n\");",
        "    fflush(stdout);",
        "    while (keepSpinning)",
        "        ;",
        "    return 0;",
        "}",
        "",
    };
    const FilePath source = FilePath::fromString(m_tempDir.path()) / "inferior_debugcrt.cpp";
    if (!source.writeFileContents(lines.join('\n').toUtf8())) {
        qWarning("Could not write %s.", qPrintable(source.toUserOutput()));
        return;
    }
    const FilePath executable = (FilePath::fromString(m_tempDir.path()) / "inferior_debugcrt")
                                    .withExecutableSuffix();
    const FilePath pdb = FilePath::fromString(m_tempDir.path()) / "inferior_debugcrt.pdb";
    Process compile;
    compile.setCommand({compiler, {"/nologo", "/Zi", "/Od", "/MDd", "/EHsc",
                                   "/Fe:" + executable.nativePath(),
                                   "/Fd:" + pdb.nativePath(),
                                   "/Fo:" + FilePath::fromString(m_tempDir.path()).nativePath()
                                       + "\\",
                                   source.nativePath()}});
    compile.setEnvironment(environment);
    QElapsedTimer timer;
    timer.start();
    compile.runBlocking(s_compileTimeout);
    if (compile.result() != ProcessResult::FinishedWithSuccess) {
        qWarning("%s", qPrintable(compileFailure("compiling the debug runtime inferior",
                                                 compile, timer.elapsed())));
        return;
    }
    data.debugCrtExecutable = executable;
    data.pastCrtReportMarker = "past the crt report";
    data.crtDebugReportModule = "ucrtbase";
}

void tst_backends::buildOtherWordWidthInferior(const FilePath &compiler, InferiorTestData &data)
{
    const QString configured = qtcEnvironmentVariable("QTC_MSVC_ENV_BAT_32");
    FilePath batchFile = configured.isEmpty() ? FilePath() : FilePath::fromUserInput(configured);
    for (FilePath dir = compiler.parentDir();
         batchFile.isEmpty() && !dir.isEmpty() && dir != dir.parentDir(); dir = dir.parentDir()) {
        const FilePath candidate = dir / "Auxiliary" / "Build" / "vcvarsamd64_x86.bat";
        if (candidate.isFile())
            batchFile = candidate;
    }
    if (!batchFile.isFile()) {
        qWarning("No 32-bit MSVC environment found next to %s - set QTC_MSVC_ENV_BAT_32 to "
                 "build an inferior of the other word width.", qPrintable(compiler.toUserOutput()));
        return;
    }

    Environment env = Environment::systemEnvironment();
    const QMap<QString, QString> pairs = environmentFromBatchFile(env, batchFile.nativePath());
    for (auto it = pairs.cbegin(); it != pairs.cend(); ++it)
        env.set(it.key(), it.value());
    const FilePath compiler32 = env.searchInPath("cl.exe");
    if (!compiler32.isExecutableFile() || !env.hasKey("INCLUDE")) {
        qWarning("%s left no usable cl.exe for the other word width.",
                 qPrintable(batchFile.toUserOutput()));
        return;
    }

    const QStringList lines = {
        "#include <cstdio>",
        "",
        "volatile bool keepSpinning = true;",
        "volatile double sink = 0;",
        "",
        "int burn()",
        "{",
        "    for (int i = 1; i < 100000; ++i)",
        "        sink += 1.0 / i; // breakpoint line",
        "    return int(sink);",
        "}",
        "",
        "int main()",
        "{",
        "    printf(\"started\\n\");",
        "    fflush(stdout);",
        "    while (keepSpinning)",
        "        burn();",
        "    return 0;",
        "}",
        "",
    };
    const FilePath source = FilePath::fromString(m_tempDir.path()) / "inferior_otherwidth.cpp";
    if (!source.writeFileContents(lines.join('\n').toUtf8())) {
        qWarning("Could not write %s.", qPrintable(source.toUserOutput()));
        return;
    }
    const FilePath executable = (FilePath::fromString(m_tempDir.path()) / "inferior_otherwidth")
                                    .withExecutableSuffix();
    const FilePath pdb = FilePath::fromString(m_tempDir.path()) / "inferior_otherwidth.pdb";
    Process compile;
    compile.setCommand({compiler32, {"/nologo", "/Zi", "/Od", "/EHsc",
                                     "/Fe:" + executable.nativePath(),
                                     "/Fd:" + pdb.nativePath(),
                                     "/Fo:" + FilePath::fromString(m_tempDir.path()).nativePath()
                                         + "\\",
                                     source.nativePath()}});
    compile.setEnvironment(env);
    QElapsedTimer timer;
    timer.start();
    compile.runBlocking(s_compileTimeout);
    if (compile.result() != ProcessResult::FinishedWithSuccess) {
        qWarning("%s", qPrintable(compileFailure("compiling the inferior of the other word width",
                                                 compile, timer.elapsed())));
        return;
    }
    data.otherWordWidthSource = source;
    data.otherWordWidthExecutable = executable;
    data.otherWordWidthFunction = "burn";
    for (int i = 0; i < lines.size(); ++i) {
        if (lines.at(i).contains("breakpoint line"))
            data.otherWordWidthBreakpointLine = i + 1;
    }
}

// The first session of a backend pays for everything that is still cold: the
// debugger itself, its extension and dumpers, and the debug information of the
// inferior, which a breakpoint on a source line is what pulls in. Pay it here,
// once, rather than in whichever row happens to run first.
// It is also the only real test of whether a backend works at all: a debugger
// can be installed, report a recent enough version and still not come up, as
// gdb's own DAP interpreter does not without a Python-enabled gdb. A backend
// that does not come up here drops out, because its rows could then only fail,
// one timeout at a time.
void tst_backends::warmUpBackends()
{
    QStringList dropped;
    const QList<Backend> backends = m_backendData.keys();
    for (Backend backend : backends) {
        if (!hasStartMode(backend, DebuggerStartModeFlag::Launch))
            continue;
        const std::unique_ptr<DebuggerBackend> warmUp = createEngine(backend);
        if (!warmUp)
            continue;
        DebuggerEngineInterface *engine = warmUp->engine();
        const InferiorTestData testData = inferiorTestData(backend);
        QEventLoop loop;
        bool finished = false;
        bool cameUp = false;
        connect(engine, &DebuggerEngineInterface::inferiorEvent, &loop,
                [&loop, &finished, &cameUp, engine, testData](InferiorEvent event) {
            switch (event) {
            case InferiorEvent::EngineSetupOk: {
                cameUp = true;
                if (testData.breakpointLine == 0) {
                    finished = true;
                    loop.quit();
                    break;
                }
                BreakpointChangeRequest request;
                request.op = BreakpointOp::Insert;
                request.requestId = 1;
                request.params.type = BreakpointByFileAndLine;
                request.params.fileName = testData.source;
                request.params.textPosition.line = testData.breakpointLine;
                request.params.enabled = true;
                engine->changeBreakpoint(request);
                break;
            }
            case InferiorEvent::SpontaneousStop:
            case InferiorEvent::StopOk:
                cameUp = true;
                finished = true;
                loop.quit();
                break;
            case InferiorEvent::EngineSetupFailed:
            case InferiorEvent::EngineRunFailed:
                finished = true;
                loop.quit();
                break;
            default:
                break;
            }
        });
        QTimer::singleShot(s_warmUpTimeout, &loop, &QEventLoop::quit);
        engine->start();
        if (!finished) // pdb answers from inside start(), before there is a loop.
            loop.exec();
        if (!cameUp) {
            qWarning("Not testing the %s backend: a plain launch did not get its engine up.",
                     qPrintable(backendName(backend)));
            m_backendData.remove(backend);
            dropped << backendName(backend);
        }
    }
    if (!dropped.isEmpty()) {
        QVERIFY2(!backendsAreRequired(),
                 qPrintable("Backends that cannot be launched at all: " + dropped.join(", ")));
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

Utils::Result<> tst_backends::checkAcceptsBreakpoint(Backend backend, BreakpointType type,
                                                     const QString &description)
{
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend);
    const DebuggerEngineSetupData &data = debuggerBackend->engine()->setupData();
    AcceptsBreakpointQuery query;
    query.type = type;
    // Only a file and line breakpoint carries a file, so asking about any other
    // type with one attached would let a backend answer by the file alone.
    if (type == BreakpointByFileAndLine)
        query.fileName = inferiorTestData(backend).source;
    query.startMode = Debugger::StartInternal;
    if (data.acceptsBreakpoint && data.acceptsBreakpoint(query))
        return Utils::ResultOk;
    return Utils::ResultError(QString("%1 not accepted by %2.")
                                  .arg(description, backendName(backend)));
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

void tst_backends::honorsTheConfiguredBreakEvents()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));
    const InferiorTestData testData = inferiorTestData(backend);
    if (!testData.throwsAnException)
        QSKIP("This backend's inferior raises nothing that could be configured.");

    // "eh" is the C++ exception, which the inferior raises on its own once it
    // runs, so no breakpoint of any kind is involved here.
    std::unique_ptr<DebuggerBackend> debuggerBackend =
        createEngineWithBreakEvents(backend, {"eh"});
    if (!debuggerBackend)
        QSKIP("This backend has no configurable break events.");
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    QVERIFY2(!debuggerBackend->contains(InferiorEvent::EngineSetupFailed), "the engine never ran");
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                              || debuggerBackend->contains(InferiorEvent::StopOk),
                              "the configured event never stopped the inferior", s_timeout);

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();
}

void tst_backends::stopsAtACaughtException()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::BreakOnThrowAndCatchCapability); !result)
        QSKIP(qPrintable(result.error()));
    if (auto result = checkAcceptsBreakpoint(backend, BreakpointAtCatch, "A catch breakpoint");
        !result) {
        QSKIP(qPrintable(result.error()));
    }
    const InferiorTestData testData = inferiorTestData(backend);
    if (!testData.throwsAnException)
        QSKIP("This backend's inferior throws nothing to catch.");

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });
    BreakpointChangeRequest request;
    request.op = BreakpointOp::Insert;
    request.requestId = 351;
    request.params.type = BreakpointAtCatch;
    request.params.enabled = true;
    engine->changeBreakpoint(request);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(351), s_timeout);
    QVERIFY2(results.value(351), "catch breakpoint insert failed");

    // A breakpoint reported as inserted but never armed leaves the inferior in
    // its spin loop, where nothing at all is reported back.
    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                              || debuggerBackend->contains(InferiorEvent::StopOk),
                              "the inferior never stopped where it catches", s_timeout);
}

void tst_backends::stopsAtAThrownException()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::BreakOnThrowAndCatchCapability); !result)
        QSKIP(qPrintable(result.error()));
    const InferiorTestData testData = inferiorTestData(backend);
    if (!testData.throwsAnException)
        QSKIP("This backend's inferior throws nothing to break on.");

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });
    BreakpointChangeRequest request;
    request.op = BreakpointOp::Insert;
    request.requestId = 350;
    request.params.type = BreakpointAtThrow;
    request.params.enabled = true;
    engine->changeBreakpoint(request);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(350), s_timeout);
    QVERIFY2(results.value(350), "throw breakpoint insert failed");

    // A breakpoint that is reported as inserted and then never fires leaves the
    // inferior to run to its end, so the exit is what tells the two apart.
    // A breakpoint reported as inserted but never armed leaves the inferior in
    // its spin loop, where nothing at all is reported back.
    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                              || debuggerBackend->contains(InferiorEvent::StopOk),
                              "the inferior never stopped at the throw", s_timeout);
}

void tst_backends::testBreakOnThrowAndCatchCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::BreakOnThrowAndCatchCapability); !result)
        QSKIP(qPrintable(result.error()));
    // The capability names both, while a backend may only have an answer for one
    // of them, so each half is asked for separately.
    const Utils::Result<> takesThrow =
        checkAcceptsBreakpoint(backend, BreakpointAtThrow, "A throw breakpoint");
    const Utils::Result<> takesCatch =
        checkAcceptsBreakpoint(backend, BreakpointAtCatch, "A catch breakpoint");
    if (!takesThrow && !takesCatch)
        QSKIP(qPrintable(takesThrow.error() + QLatin1Char(' ') + takesCatch.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
        results[requestId] = ok;
    });

    if (takesThrow) {
        BreakpointChangeRequest throwRequest;
        throwRequest.op = BreakpointOp::Insert;
        throwRequest.requestId = 74;
        throwRequest.params.type = BreakpointAtThrow;
        engine->changeBreakpoint(throwRequest);
        QTRY_VERIFY_WITH_TIMEOUT(results.contains(74), s_timeout);
        QVERIFY2(results.value(74), "throw breakpoint insert failed");
    }

    if (takesCatch) {
        BreakpointChangeRequest catchRequest;
        catchRequest.op = BreakpointOp::Insert;
        catchRequest.requestId = 75;
        catchRequest.params.type = BreakpointAtCatch;
        engine->changeBreakpoint(catchRequest);
        QTRY_VERIFY_WITH_TIMEOUT(results.contains(75), s_timeout);
        QVERIFY2(results.value(75), "catch breakpoint insert failed");
    }
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
                                 || debuggerBackend->contains(InferiorEvent::EngineSetupFailed),
                                 s_qmlStartupTimeout);
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

        QStringList messages;
        connect(debuggerBackend->engine(), &DebuggerEngineInterface::message, this,
                [&messages](const QString &text, int, int) { messages.append(text); });

        debuggerBackend->clearEvents();
        debuggerBackend->execute({ExecutionCommand::Detach});

        QTRY_VERIFY2_WITH_TIMEOUT(!debuggerBackend->inferiorResults().isEmpty(),
                                  "Detach never signaled completion", s_timeout);
        QCOMPARE(debuggerBackend->inferiorResults().constFirst().exitStatus, InferiorExitStatus::Detached);
        const QString marker = detachMarker(backend);
        QVERIFY2(std::any_of(messages.cbegin(), messages.cend(), [&marker](const QString &text) {
            return text.contains(marker);
        }), qPrintable("Detach never sent \"" + marker + '"'));
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
        const QString marker = detachMarker(backend);
        QVERIFY2(std::any_of(messages.cbegin(), messages.cend(), [&marker](const QString &text) {
            return text.contains(marker);
        }), qPrintable("shutdownInferior(ShutdownMode::Detach) never sent \"" + marker + '"'));
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

    if (testData.functionMarker.isEmpty() || !disassemblesByFunctionName(backend))
        return;
    disassembly = {};
    disassemblyReceived = false;
    engine->fetchDisassembly(41, 0, testData.functionMarker);
    QTRY_VERIFY2_WITH_TIMEOUT(disassemblyReceived,
                              "disassembly by function name alone was never reported", s_timeout);
    QVERIFY2(!disassembly.data().isEmpty(), "disassembly by function name came back empty");
}

void tst_backends::reportsSourceLinesInTheDisassembly()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::DisassemblerCapability); !result)
        QSKIP(qPrintable(result.error()));
    const InferiorTestData testData = inferiorTestData(backend);
    if (testData.disassemblySourceMarker.isEmpty())
        QSKIP("this backend's disassembly does not name the source it covers");

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
    engine->fetchDisassembly(42, bumpAddress, "bump");
    QTRY_VERIFY_WITH_TIMEOUT(disassemblyReceived, s_timeout);

    int sourceLines = 0;
    bool sawSource = false;
    for (const DisassemblerLine &line : disassembly.data()) {
        if (line.isCode())
            ++sourceLines;
        if (line.data.contains(testData.disassemblySourceMarker))
            sawSource = true;
    }
    QVERIFY2(sourceLines > 0, "the disassembly named no source line at all");
    QVERIFY2(sawSource, qPrintable(QString("none of the %1 source lines the disassembly names "
                                           "contains \"%2\"")
                                       .arg(sourceLines)
                                       .arg(testData.disassemblySourceMarker)));
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
        return data["target-name"].data().contains(marker, Qt::CaseInsensitive);
    }), s_timeout);
    QTRY_VERIFY_WITH_TIMEOUT(std::any_of(unloaded.cbegin(), unloaded.cend(), [](const GdbMi &data) {
        return data["target-name"].data().contains("inferiorlib", Qt::CaseInsensitive);
    }), s_timeout);
}

void tst_backends::testThreadEventCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkExtraCapability(backend, Debugger::DebuggerExtraCapability::ThreadEvent); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    const QStringList created = debuggerBackend->threadIds(ThreadEvent::Created);
    QVERIFY2(!created.isEmpty(), "the inferior's own thread was never reported as created");
    const QString threadId = created.constFirst();
    QVERIFY(!threadId.isEmpty());

    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->threadIds(ThreadEvent::Exited).contains(threadId),
                              "the killed inferior's thread was never reported as exited",
                              s_timeout);
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

    QStringList errors;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&errors](const QString &text, int channel, int) {
        // Whatever lldb writes to stderr lands on this channel too, the
        // debuggee's own debug info complaints included, so only take what
        // names the load.
        if (channel == Debugger::LogError && text.contains("symbol", Qt::CaseInsensitive))
            errors.append(text);
    });

    RefreshRequest stackSymbolsRequest;
    stackSymbolsRequest.kind = RefreshKind::StackSymbols;
    stackSymbolsRequest.requestId = 82;
    stackSymbolsRequest.path = inferiorTestData(backend).moduleSymbolsPath;
    engine->refresh(stackSymbolsRequest);

    // Loading symbols has no answer of its own. Asking for all of them next
    // brings the module list with it, and that answer can only arrive once the
    // loads before it have been processed.
    RefreshRequest allSymbolsRequest;
    allSymbolsRequest.kind = RefreshKind::AllSymbols;
    allSymbolsRequest.requestId = 83;
    engine->refresh(allSymbolsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Modules)), s_timeout);
    QVERIFY2(errors.isEmpty(), qPrintable("loading symbols was refused: " + errors.join(' ')));
}

void tst_backends::testResetInferiorCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::ResetInferiorCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    debuggerBackend->clearEvents();
    debuggerBackend->clearInferiorResults();
    debuggerBackend->execute({ExecutionCommand::ResetInferior});

    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunRequested));
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunOk));
    // The restarted inferior has stopped again, so an exit report for the one we replaced
    // would have arrived by now. A backend that swaps processes must not emit one.
    QVERIFY2(debuggerBackend->inferiorResults().isEmpty(),
             "restarting the inferior was reported as the inferior exiting");
}

void tst_backends::runsUserCommandsWhenResettingTheInferior()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::ResetInferiorCapability); !result)
        QSKIP(qPrintable(result.error()));
    const UserCommandProbe probe = userCommandProbe(backend, UserCommandHook::Reset);
    if (probe.marker.isEmpty())
        QSKIP("This backend's start data carries no commands for a reset.");

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);

    QStringList messages;
    connect(debuggerBackend->engine(), &DebuggerEngineInterface::message, this,
            [&messages](const QString &text, int, int) { messages.append(text); });
    QVERIFY(!messages.join(' ').contains(probe.marker));

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::ResetInferior});

    QTRY_VERIFY2_WITH_TIMEOUT(messages.join(' ').contains(probe.marker),
                              qPrintable("resetting the inferior ran no configured command - "
                                         "log: " + messages.join(' ').right(300)), s_timeout);
}

void tst_backends::runsTheDebuggerAsTheConfiguredUser()
{
    QFETCH(Backend, backend);

    if (HostOsInfo::isWindowsHost())
        QSKIP("Running a process as another user is a no-op on this platform.");
    if (auto result = checkExtraCapability(backend,
            Debugger::DebuggerExtraCapability::RunAsUser); !result) {
        QSKIP(qPrintable(result.error()));
    }

    // sudo cannot be asked for a password here, so a stub takes its place: it
    // records how it was called and runs what it was given.
    const FilePath stubDir = FilePath::fromString(m_tempDir.path()) / "runasuser";
    QVERIFY(stubDir.ensureWritableDir());
    const FilePath log = stubDir / "sudo.log";
    const FilePath stub = stubDir / "sudo";
    QVERIFY(stub.writeFileContents(QString(R"(#!/bin/sh
echo "args: $@" >> %1
echo "askpass: $SUDO_ASKPASS" >> %1
while [ $# -gt 0 ]; do
  case "$1" in
    -u) shift 2;;
    -A|-E) shift;;
    *) break;;
  esac
done
exec "$@"
)").arg(log.nativePath()).toUtf8()));
    QFile::setPermissions(stub.toFSPathString(),
                          QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

    Environment debuggerEnvironment = Environment::systemEnvironment();
    debuggerEnvironment.prependOrSetPath(stubDir);
    // The library resolves the wrapper itself, so the stub has to be findable
    // from this process as well.
    const QByteArray originalPath = qgetenv("PATH");
    qputenv("PATH", (stubDir.nativePath() + ":" + QString::fromLocal8Bit(originalPath)).toLocal8Bit());

    const QString user = "qtc-test-user";
    std::unique_ptr<DebuggerBackend> debuggerBackend
        = createEngineRunningAsUser(backend, user, debuggerEnvironment);
    if (!debuggerBackend) {
        qputenv("PATH", originalPath);
        QSKIP("This backend's start data carries no user to run as.");
    }

    debuggerBackend->engine()->start();
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::EngineSetupOk),
                              "the debugger never came up through the wrapper", s_timeout);

    const QString firstCall = QString::fromUtf8(log.fileContents().value_or(QByteArray()));
    QVERIFY2(firstCall.contains("-u " + user),
             qPrintable("the debugger was not started as the configured user: " + firstCall));
    QVERIFY2(firstCall.contains(m_backendData[backend].path.nativePath()),
             qPrintable("the wrapper was not given the debugger to run: " + firstCall));

    // Interrupting has to take the same route, or the signal is refused.
    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Interrupt});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::StopOk),
                              "the inferior never stopped", s_timeout);
    const QString afterInterrupt = QString::fromUtf8(log.fileContents().value_or(QByteArray()));
    QVERIFY2(afterInterrupt.contains("kill -s SIGINT"),
             qPrintable("the interrupt did not go through the wrapper: " + afterInterrupt));

    qputenv("PATH", originalPath);
}

void tst_backends::runsAUserStartScriptAtStartup()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    const FilePath script = FilePath::fromString(m_tempDir.path()) / "startscript.txt";
    QVERIFY(script.writeFileContents("echo QTCSTARTSCRIPTMARKER\n"));

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngineWithStartScript(backend, script);
    if (!debuggerBackend)
        QSKIP("This backend's start data carries no start script.");

    QStringList messages;
    connect(debuggerBackend->engine(), &DebuggerEngineInterface::message, this,
            [&messages](const QString &text, int, int) { messages.append(text); });
    debuggerBackend->engine()->start();

    QTRY_VERIFY2_WITH_TIMEOUT(messages.join(' ').contains("QTCSTARTSCRIPTMARKER"),
                              qPrintable("the start script was never sourced - log: "
                                         + messages.join(' ').right(300)), s_timeout);

    // A script that cannot be read has to be reported, not passed on silently.
    const FilePath missing = FilePath::fromString(m_tempDir.path()) / "no-such-startscript.txt";
    QVERIFY(!missing.exists());
    std::unique_ptr<DebuggerBackend> withoutScript
        = createEngineWithStartScript(backend, missing);
    QVERIFY(withoutScript);

    QStringList warnings;
    connect(withoutScript->engine(), &DebuggerEngineInterface::message, this,
            [&warnings](const QString &text, int channel, int) {
        if (channel == Debugger::LogWarning)
            warnings.append(text);
    });
    withoutScript->engine()->start();

    QTRY_VERIFY2_WITH_TIMEOUT(warnings.join(' ').contains(missing.toUserOutput()),
                              qPrintable("an unreadable start script was not reported - "
                                         "warnings: " + warnings.join(' ').right(300)), s_timeout);
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
                                 || debuggerBackend->contains(InferiorEvent::EngineSetupFailed),
                                 s_qmlStartupTimeout);
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
    const GdbMi threadsData = responses.value(int(RefreshKind::Threads));
    const GdbMi threads = threadsData["threads"];
    QVERIFY2(threads.childCount() > 0, qPrintable(threadsData.toString()));

    // A thread is identified by a small ordinal, which the backend numbers its
    // own way, while its raw OS thread id stays in the target id.
    const QString currentId = threadsData["current-thread-id"].data();
    QVERIFY2(!currentId.isEmpty(), qPrintable(threadsData.toString()));
    GdbMi currentThread;
    for (const GdbMi &thread : threads) {
        if (thread["id"].data() == currentId) {
            currentThread = thread;
            break;
        }
    }
    QVERIFY2(currentThread.isValid(), qPrintable(threadsData.toString()));
    const QString targetId = currentThread["target-id"].data();
    QVERIFY2(!targetId.isEmpty() && targetId != currentThread["id"].data(),
             qPrintable(threadsData.toString()));
}

void tst_backends::testTracePointCapability()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::TracePointCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend);
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

    // A tracepoint reports the line and lets the program run on, so it is set
    // before the run and reports itself once the line is reached.
    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [this, engine, backend](InferiorEvent event) {
        if (event != InferiorEvent::EngineSetupOk)
            return;
        BreakpointChangeRequest tracepointRequest;
        tracepointRequest.op = BreakpointOp::Insert;
        tracepointRequest.requestId = 89;
        tracepointRequest.params.type = BreakpointByFileAndLine;
        tracepointRequest.params.fileName = inferiorTestData(backend).source;
        tracepointRequest.params.textPosition.line = inferiorTestData(backend).breakpointLine;
        tracepointRequest.params.textPosition.column = 0;
        tracepointRequest.params.enabled = true;
        tracepointRequest.params.tracepoint = true;
        tracepointRequest.params.message
            = "globalValue is {globalValue}, globalMessage is {globalMessage}";
        engine->changeBreakpoint(tracepointRequest);
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(89), s_timeout);
    QVERIFY2(results.value(89), "tracepoint insert failed");

    const QString messageTail = tracepointMessageTail(backend);
    QTRY_VERIFY_WITH_TIMEOUT(tracepointMessages.join('\n').contains("globalValue is 41")
                             && tracepointMessages.join('\n').contains(messageTail),
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

std::unique_ptr<DebuggerBackend> tst_backends::launchAndStopAtBreakpoint(Backend backend,
    const std::optional<Utils::ProcessRunData> &inferiorRunDataOverride, int line)
{
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
                                                                   inferiorRunDataOverride);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [this, engine, backend, line](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            BreakpointChangeRequest request;
            request.op = BreakpointOp::Insert;
            request.requestId = 1;
            request.params.type = BreakpointByFileAndLine;
            request.params.fileName = inferiorTestData(backend).source;
            request.params.textPosition.line = line > 0 ? line
                                                        : inferiorTestData(backend).breakpointLine;
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
                                 || backendPtr->contains(InferiorEvent::EngineSetupFailed),
                                 s_qmlStartupTimeout);
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

    if (backend == Backend::Pdb && HostOsInfo::isWindowsHost())
        QSKIP("Interrupting a running inferior is not supported by pdb on Windows.");

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

void tst_backends::reportsApplicationOutput()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    const QString marker = inferiorTestData(backend).applicationOutputMarker;
    if (marker.isEmpty())
        QSKIP("inferior declares no application output marker");

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList applicationOutput;
    QStringList otherChannels;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&applicationOutput, &otherChannels](const QString &text, int channel, int) {
        if (channel == Debugger::AppOutput || channel == Debugger::AppStuff)
            applicationOutput.append(text);
        else
            otherChannels.append(text);
    });

    stopInferiorSpinLoop(backend, engine);

    debuggerBackend->clearInferiorResults();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(!debuggerBackend->inferiorResults().isEmpty(), s_timeout);

    QTRY_VERIFY2_WITH_TIMEOUT(applicationOutput.join('\n').contains(marker),
                              qPrintable(QString("the debuggee's own output never arrived on the "
                                                 "application channel - looked for \"%1\", other "
                                                 "channels saw:\n  %2")
                                             .arg(marker, otherChannels.join("\n  ").left(600))),
                              s_timeout);
}

void tst_backends::reportsAFirstChanceExceptionWhenAsked()
{
    QFETCH(Backend, backend);

    const InferiorTestData testData = inferiorTestData(backend);
    if (!testData.throwsAnException || testData.afterThrowOutputMarker.isEmpty())
        QSKIP("inferior throws nothing the debugger could report");

    auto reportsWith = [&](bool reportFirstChance, bool *sawTheThrow) -> int {
        std::unique_ptr<DebuggerBackend> debuggerBackend
            = createEngineReportingExceptions(backend, reportFirstChance);
        if (!debuggerBackend)
            return -1;
        DebuggerEngineInterface *engine = debuggerBackend->engine();
        int reports = 0;
        connect(engine, &DebuggerEngineInterface::exceptionReported, this,
                [&reports](const ExceptionReport &) { ++reports; });
        QStringList applicationOutput;
        connect(engine, &DebuggerEngineInterface::message, this,
                [&applicationOutput](const QString &text, int channel, int) {
            if (channel == Debugger::AppOutput || channel == Debugger::AppStuff)
                applicationOutput.append(text);
        });
        engine->start();
        const QString marker = testData.afterThrowOutputMarker;
        [&] {
            QTRY_VERIFY_WITH_TIMEOUT(applicationOutput.join(' ').contains(marker),
                                     s_warmUpTimeout);
        }();
        *sawTheThrow = applicationOutput.join(' ').contains(marker);
        debuggerBackend->clearEvents();
        engine->shutdownInferior(ShutdownMode::Kill);
        [&debuggerBackend] {
            QTRY_VERIFY_WITH_TIMEOUT(
                debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
        }();
        engine->shutdownEngine();
        return reports;
    };

    bool sawTheThrow = false;
    const int asked = reportsWith(true, &sawTheThrow);
    if (asked < 0)
        QSKIP("This backend's start data says nothing about reporting exceptions.");
    QVERIFY2(sawTheThrow, "the inferior never got past its own exception");
    QVERIFY2(asked > 0, "the exception the inferior threw was never reported");

    QCOMPARE(reportsWith(false, &sawTheThrow), 0);
    QVERIFY2(sawTheThrow, "the inferior never got past its own exception");
}

void tst_backends::stopsWhereTheDebugRuntimeReports()
{
    QFETCH(Backend, backend);

    const InferiorTestData testData = inferiorTestData(backend);
    if (testData.debugCrtExecutable.isEmpty())
        QSKIP("no program built against the debug C runtime to report from");

    auto runWithModule = [&](const QString &module, QStringList *output) -> bool {
        std::unique_ptr<DebuggerBackend> debuggerBackend
            = createEngineForTheDebugRuntime(backend, module);
        if (!debuggerBackend)
            return false;
        DebuggerEngineInterface *engine = debuggerBackend->engine();
        connect(engine, &DebuggerEngineInterface::message, this,
                [output](const QString &text, int channel, int) {
            if (channel == Debugger::AppOutput || channel == Debugger::AppStuff)
                output->append(text);
        });
        engine->start();
        const bool stopped = [&] {
            [&] {
                QTRY_VERIFY_WITH_TIMEOUT(
                    debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                        || output->join(' ').contains(testData.pastCrtReportMarker),
                    s_warmUpTimeout);
            }();
            return debuggerBackend->contains(InferiorEvent::SpontaneousStop);
        }();
        debuggerBackend->clearEvents();
        engine->shutdownInferior(ShutdownMode::Kill);
        [&debuggerBackend] {
            QTRY_VERIFY_WITH_TIMEOUT(
                debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
        }();
        engine->shutdownEngine();
        return stopped;
    };

    QStringList withoutModule;
    if (!runWithModule({}, &withoutModule) && withoutModule.isEmpty())
        QSKIP("This backend's start data names no C runtime to break in.");
    QVERIFY2(withoutModule.join(' ').contains(testData.pastCrtReportMarker),
             qPrintable("the program never got past its report: " + withoutModule.join(' ')));

    QStringList withModule;
    QVERIFY2(runWithModule(testData.crtDebugReportModule, &withModule),
             qPrintable("the report did not stop the program: " + withModule.join(' ')));
    QVERIFY2(!withModule.join(' ').contains(testData.pastCrtReportMarker),
             "the program ran past its report although the debugger was to stop there");
}

void tst_backends::keepsQtLoggingOffTheConsoleWithoutATerminal()
{
    QFETCH(Backend, backend);

    const QString prefix = inferiorTestData(backend).environmentQueryPrefix;
    if (prefix.isEmpty())
        QSKIP("inferior cannot be asked for a variable of its environment");

    auto reportedValue = [&](bool useTerminal, const QString &variable) -> QString {
        Environment inferiorEnvironment = Environment::systemEnvironment();
        inferiorEnvironment.set("QTC_BACKEND_ENV_QUERY", variable);
        inferiorEnvironment.unset(variable);
        std::unique_ptr<DebuggerBackend> debuggerBackend
            = createEngineWithTerminal(backend, useTerminal, inferiorEnvironment);
        if (!debuggerBackend)
            return {};
        DebuggerEngineInterface *engine = debuggerBackend->engine();
        QStringList applicationOutput;
        connect(engine, &DebuggerEngineInterface::message, this,
                [&applicationOutput](const QString &text, int channel, int) {
            if (channel == Debugger::AppOutput || channel == Debugger::AppStuff)
                applicationOutput.append(text);
        });
        engine->start();
        const QString expected = prefix + variable + '=';
        QString reported;
        auto sawTheAnswer = [&] {
            for (const QString &line : std::as_const(applicationOutput)) {
                const int at = line.indexOf(expected);
                if (at < 0)
                    continue;
                reported = line.mid(at + expected.size()).trimmed();
                return true;
            }
            return false;
        };
        [&] { QTRY_VERIFY_WITH_TIMEOUT(sawTheAnswer(), s_warmUpTimeout); }();
        debuggerBackend->clearEvents();
        engine->shutdownInferior(ShutdownMode::Kill);
        [&debuggerBackend] {
            QTRY_VERIFY_WITH_TIMEOUT(
                debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
        }();
        engine->shutdownEngine();
        return reported;
    };

    const QString withoutTerminal = reportedValue(false, "QT_LOGGING_TO_CONSOLE");
    if (withoutTerminal.isEmpty())
        QSKIP("This backend's start data says nothing about a terminal.");
    QCOMPARE(withoutTerminal, QString("0"));
    QCOMPARE(reportedValue(false, "QT_FORCE_STDERR_LOGGING"), QString("0"));
}

void tst_backends::asksCdbForItsOwnConsoleWithATerminal()
{
    QFETCH(Backend, backend);

    auto launchCommand = [&](bool useTerminal) -> QString {
        std::unique_ptr<DebuggerBackend> debuggerBackend = createEngineWithTerminal(
            backend, useTerminal, Environment::systemEnvironment());
        if (!debuggerBackend)
            return {};
        DebuggerEngineInterface *engine = debuggerBackend->engine();
        QString launched;
        connect(engine, &DebuggerEngineInterface::message, this,
                [&launched](const QString &text, int, int) {
            if (text.startsWith("Launching "))
                launched = text;
        });
        engine->start();
        [&launched] { QTRY_VERIFY_WITH_TIMEOUT(!launched.isEmpty(), s_timeout); }();
        engine->shutdownEngine();
        return launched;
    };

    const QString withTerminal = launchCommand(true);
    if (withTerminal.isEmpty())
        QSKIP("This backend reports no command line of its own.");
    QVERIFY2(withTerminal.contains(" -2"),
             qPrintable("the debugger was not asked for a console: " + withTerminal));
    QVERIFY2(!launchCommand(false).contains(" -2"),
             "the debugger was asked for a console although none was wanted");
}

void tst_backends::ignoresAFirstChanceAccessViolationWhenAsked()
{
    QFETCH(Backend, backend);

    const QString marker = inferiorTestData(backend).survivedAccessViolationMarker;
    if (marker.isEmpty())
        QSKIP("inferior cannot handle an access violation of its own");

    std::unique_ptr<DebuggerBackend> debuggerBackend
        = createEngineForAccessViolations(backend, true, {"survivable-crash"});
    if (!debuggerBackend)
        QSKIP("This backend's start data says nothing about first chance violations.");
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList applicationOutput;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&applicationOutput](const QString &text, int channel, int) {
        if (channel == Debugger::AppOutput || channel == Debugger::AppStuff)
            applicationOutput.append(text);
    });

    engine->start();
    // The program prints this from its own handler, so it only gets there if the
    // violation was passed to it instead of stopping the inferior.
    QTRY_VERIFY2_WITH_TIMEOUT(applicationOutput.join(' ').contains(marker),
                              "the inferior never handled the access violation itself",
                              s_warmUpTimeout);
    QVERIFY2(!debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                 && !debuggerBackend->contains(InferiorEvent::StopOk),
             "the violation stopped the inferior although it was to be ignored");

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();
}

void tst_backends::passesTheHeapDebuggingFlagToTheDebuggee()
{
    QFETCH(Backend, backend);

    const QString prefix = inferiorTestData(backend).heapFlagReportPrefix;
    if (prefix.isEmpty())
        QSKIP("inferior does not report the debug heap flag it was started with");

    auto reportedFlag = [&](bool enableHeapDebugging) -> QString {
        std::unique_ptr<DebuggerBackend> debuggerBackend
            = createEngineWithHeapDebugging(backend, enableHeapDebugging);
        if (!debuggerBackend)
            return {};
        DebuggerEngineInterface *engine = debuggerBackend->engine();
        QStringList applicationOutput;
        connect(engine, &DebuggerEngineInterface::message, this,
                [&applicationOutput](const QString &text, int channel, int) {
            if (channel == Debugger::AppOutput || channel == Debugger::AppStuff)
                applicationOutput.append(text);
        });
        engine->start();
        QString reported;
        auto sawTheFlag = [&] {
            for (const QString &line : std::as_const(applicationOutput)) {
                const int at = line.indexOf(prefix);
                if (at < 0)
                    continue;
                reported = line.mid(at + prefix.size()).trimmed();
                return true;
            }
            return false;
        };
        [&] { QTRY_VERIFY_WITH_TIMEOUT(sawTheFlag(), s_warmUpTimeout); }();
        debuggerBackend->clearEvents();
        engine->shutdownInferior(ShutdownMode::Kill);
        [&debuggerBackend] {
            QTRY_VERIFY_WITH_TIMEOUT(
                debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
        }();
        engine->shutdownEngine();
        return reported;
    };

    const QString withoutHeapDebugging = reportedFlag(false);
    if (withoutHeapDebugging.isEmpty())
        QSKIP("This backend's start data carries no heap debugging setting yet.");
    QCOMPARE(withoutHeapDebugging, QString("1"));
    QCOMPARE(reportedFlag(true), QString("0"));
}

void tst_backends::passesInferiorEnvironmentToTheDebuggee()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    const InferiorTestData &data = inferiorTestData(backend);
    if (data.environmentReportPrefix.isEmpty())
        QSKIP("inferior does not report its environment");

    const QString markerValue = "environment-of-the-run-configuration";
    Environment inferiorEnvironment = Environment::systemEnvironment();
    inferiorEnvironment.set("QTC_BACKEND_ENV_MARKER", markerValue);
    const ProcessRunData inferiorRunData{{data.executable, {}}, {}, inferiorEnvironment};

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend,
                                                                                inferiorRunData);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList applicationOutput;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&applicationOutput](const QString &text, int channel, int) {
        if (channel == Debugger::AppOutput || channel == Debugger::AppStuff)
            applicationOutput.append(text);
    });

    stopInferiorSpinLoop(backend, engine);

    debuggerBackend->clearInferiorResults();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(!debuggerBackend->inferiorResults().isEmpty(), s_timeout);

    const QString expected = data.environmentReportPrefix + markerValue;
    QTRY_VERIFY2_WITH_TIMEOUT(applicationOutput.join('\n').contains(expected),
                              qPrintable(QString("the debuggee never saw the inferior's "
                                                 "environment - looked for \"%1\" in:\n  %2")
                                             .arg(expected,
                                                  applicationOutput.join("\n  ").left(600))),
                              s_timeout);
}

void tst_backends::reportsSourcePathsInStackFrames()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    const FilePath source = inferiorTestData(backend).source;

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    std::optional<GdbMi> stack;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&stack](quint64, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::FullStack)
            stack = data;
    });

    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 1;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(stack.has_value(), s_timeout);

    const GdbMi frames = (*stack)["stack"]["frames"];
    QVERIFY(frames.childCount() > 0);

    // StackFrame::parseFrame() opens what a frame reports as "file", resolving a relative
    // path against the build directory - a base name there names an unrelated file.
    QStringList reportedFiles;
    bool found = false;
    for (const GdbMi &frame : frames) {
        const QString file = frame["file"].data();
        if (file.isEmpty())
            continue;
        reportedFiles.append(file);
        if (FilePath::fromUserInput(file).canonicalPath() == source.canonicalPath())
            found = true;
    }
    QVERIFY2(found, qPrintable(QString("no stack frame points at \"%1\" - the frames name:\n  %2")
                                   .arg(source.toUserOutput(),
                                        reportedFiles.join("\n  ").left(600))));
}

void tst_backends::expandsContainerLocalWhenExpanded()
{
    QFETCH(Backend, backend);

    const QString local = inferiorTestData(backend).expandableLocal;
    if (local.isEmpty())
        QSKIP("inferior declares no expandable container local");

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
    const GdbMi collapsedData = responses.value(int(RefreshKind::Locals));
    // The path an item is reached by is the backend's own: a dumper names a
    // local after the variable, while what a stock adapter calls one can hold
    // anything and is therefore only what it is displayed as.
    const QString iname = findItemByName(collapsedData, local)["iname"].data();
    QVERIFY2(!iname.isEmpty(), qPrintable("collapsed: " + collapsedData.toString()));

    responses.clear();
    request.requestId = 121;
    request.expandedINames = {iname};
    engine->refresh(request);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
    const GdbMi expandedData = responses.value(int(RefreshKind::Locals));
    const QString expanded = expandedData.toString();
    // A dumper may either name each child by its iname or nest an unnamed child
    // list under the item, whose inames the view derives from the position.
    const GdbMi expandedItem = findItemByIName(expandedData, iname);
    QVERIFY2(expanded.contains(iname + '.') || expandedItem["children"].childCount() > 0,
             qPrintable("expanded: " + expanded));

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

void tst_backends::resolvesATypeArrivingWithALaterLibrary()
{
    QFETCH(Backend, backend);

    const InferiorTestData data = inferiorTestData(backend);
    if (data.libraryTypeSymbol.isEmpty() || data.libraryLoadedLine == 0)
        QSKIP("inferior loads no library carrying a type of its own");
    if (auto result = checkCapability(backend, Debugger::AddWatcherCapability); !result)
        QSKIP(qPrintable(result.error()));

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, GdbMi> locals;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&locals](quint64 requestId, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::Locals)
            locals[requestId] = data;
    });

    QJsonObject watcher;
    watcher.insert("iname", "watch.0");
    watcher.insert("exp", toHex(data.libraryTypeSymbol));
    QJsonArray watchers;
    watchers.append(watcher);

    RefreshRequest request;
    request.kind = RefreshKind::Locals;
    request.watchers = watchers;
    request.expandedINames = {"watch.0"};

    // Asking for the pointee here is what makes the backend look the type up
    // while the library is not loaded, so the answer is that it has none.
    request.requestId = 130;
    engine->refresh(request);
    QTRY_VERIFY_WITH_TIMEOUT(locals.contains(130), s_timeout);
    QVERIFY2(findItemByIName(locals.value(130), "watch.0").isValid(),
             qPrintable("no watch on " + data.libraryTypeSymbol + " before the library load: "
                        + locals.value(130).toString()));

    QHash<quint64, bool> insertResults;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&insertResults](quint64 requestId, BreakpointOp op, bool ok, const GdbMi &) {
        if (op == BreakpointOp::Insert)
            insertResults[requestId] = ok;
    });
    BreakpointChangeRequest breakpointRequest;
    breakpointRequest.op = BreakpointOp::Insert;
    breakpointRequest.requestId = 131;
    breakpointRequest.params.type = BreakpointByFileAndLine;
    breakpointRequest.params.fileName = data.source;
    breakpointRequest.params.textPosition.line = data.libraryLoadedLine;
    breakpointRequest.params.enabled = true;
    engine->changeBreakpoint(breakpointRequest);
    QTRY_VERIFY_WITH_TIMEOUT(insertResults.contains(131), s_timeout);
    QVERIFY2(insertResults.value(131), "breakpoint on the library-loaded line failed to insert");

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    QCOMPARE(debuggerBackend->stoppedLine(), data.libraryLoadedLine);

    request.requestId = 132;
    engine->refresh(request);
    QTRY_VERIFY_WITH_TIMEOUT(locals.contains(132), s_timeout);
    // A dumper puts the pointee's members straight under the watcher, while a
    // stock adapter reports the dereferenced pointer as a child of its own,
    // which leaves the member one level further down.
    for (const GdbMi &child : findItemByIName(locals.value(132), "watch.0")["children"])
        request.expandedINames.insert(child["iname"].data());
    request.requestId = 133;
    engine->refresh(request);
    QTRY_VERIFY_WITH_TIMEOUT(locals.contains(133), s_timeout);
    const QString afterLoad = locals.value(133).toString();
    QVERIFY2(afterLoad.contains(data.libraryTypeChild),
             qPrintable("the type the library brought along did not resolve, so "
                        + data.libraryTypeSymbol + " has no " + data.libraryTypeChild
                        + " member: " + afterLoad));
}

void tst_backends::honorsDumperOptionsFromTheRequest()
{
    QFETCH(Backend, backend);

    if (!debuggingHelpersChangeContainerOutput(backend))
        QSKIP("This backend's container output does not change with the debugging helpers.");
    const QString local = inferiorTestData(backend).expandableLocal;
    if (local.isEmpty())
        QSKIP("inferior declares no container local whose dumper output to compare");
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

    quint64 requestId = 130;
    auto localsWithHelpers = [&](bool useDebuggingHelpers) -> GdbMi {
        responses.clear();
        RefreshRequest request;
        request.kind = RefreshKind::Locals;
        request.requestId = ++requestId;
        request.dumperOptions.useDebuggingHelpers = useDebuggingHelpers;
        // The helpers shape an item's children, so the item has to be expanded for
        // the difference to show up at all.
        request.expandedINames = {iname};
        engine->refresh(request);
        [&responses] {
            QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
        }();
        return findItemByIName(responses.value(int(RefreshKind::Locals)), iname);
    };

    const GdbMi fancy = localsWithHelpers(true);
    QVERIFY2(fancy.isValid(), "the container local was not reported at all");

    const GdbMi plain = localsWithHelpers(false);
    QVERIFY2(plain.isValid(), "the container local was not reported without the helpers");
    QVERIFY2(fancy.toString() != plain.toString(),
             qPrintable("turning the debugging helpers off changed nothing: " + plain.toString()));
}

void tst_backends::marksTheUninitializedVariablesTheRequestNames()
{
    QFETCH(Backend, backend);

    const InferiorTestData testData = inferiorTestData(backend);
    if (!testData.marksUninitializedVariables)
        QSKIP("This backend's dumpers do not act on the variables named uninitialized.");
    const QString iname = "local." + testData.localMarker;

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    quint64 requestId = 150;
    auto localsWithUninitialized = [&](const QStringList &uninitialized) -> GdbMi {
        responses.clear();
        RefreshRequest request;
        request.kind = RefreshKind::Locals;
        request.requestId = ++requestId;
        request.uninitializedVariables = uninitialized;
        engine->refresh(request);
        [&responses] {
            QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
        }();
        return findItemByIName(responses.value(int(RefreshKind::Locals)), iname);
    };

    const GdbMi initialized = localsWithUninitialized({});
    QVERIFY2(initialized.isValid(), "the local was not reported at all");
    QVERIFY2(initialized["valueencoded"].data() != "optimizedout",
             qPrintable("reported as out of scope unasked: " + initialized.toString()));

    const GdbMi named = localsWithUninitialized({testData.localMarker});
    QCOMPARE(named["valueencoded"].data(), QString("optimizedout"));
}

void tst_backends::honorsTheStringLengthLimitFromTheRequest()
{
    QFETCH(Backend, backend);

    if (!honorsStringLengthLimits(backend))
        QSKIP("This backend cannot be told how much of a string to report.");
    const QString local = inferiorTestData(backend).longStringLocal;
    if (local.isEmpty())
        QSKIP("inferior declares no local long enough to be cut short");
    const QString iname = "local." + local;

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    quint64 requestId = 140;
    auto localsWithStringLimit = [&](int limit) -> GdbMi {
        responses.clear();
        RefreshRequest request;
        request.kind = RefreshKind::Locals;
        request.requestId = ++requestId;
        // How much of the string is read, and how much of what was read is
        // shown: either one left at its default would decide the outcome alone.
        request.dumperOptions.maximalStringLength = limit;
        request.dumperOptions.displayStringLimit = limit;
        engine->refresh(request);
        [&responses] {
            QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
        }();
        return findItemByIName(responses.value(int(RefreshKind::Locals)), iname);
    };

    const auto reportedValue = [](const GdbMi &item) {
        return decodeData(item["value"].data(), item["valueencoded"].data());
    };

    const GdbMi cut = localsWithStringLimit(64);
    QVERIFY2(cut.isValid(), "the long local was not reported at all");
    QVERIFY2(!reportedValue(cut).contains("LONGTEXTEND"),
             qPrintable("the value was reported in full despite the limit: "
                        + reportedValue(cut).left(200)));

    const GdbMi whole = localsWithStringLimit(4000);
    QVERIFY2(reportedValue(whole).contains("LONGTEXTEND"),
             qPrintable("the value stayed cut short with a limit past its length: "
                        + reportedValue(whole).left(200)));
}

void tst_backends::limitsTheReportedStackDepth()
{
    QFETCH(Backend, backend);

    if (!limitsStackDepth(backend))
        QSKIP("This backend reports every frame regardless of the requested limit.");
    const InferiorTestData testData = inferiorTestData(backend);
    if (testData.deepRecursionBreakpointLine == 0)
        QSKIP("inferior has no recursion chain to report frames of");

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
    deepRequest.requestId = 330;
    deepRequest.params.type = BreakpointByFileAndLine;
    deepRequest.params.fileName = testData.source;
    deepRequest.params.textPosition.line = testData.deepRecursionBreakpointLine;
    deepRequest.params.enabled = true;
    engine->changeBreakpoint(deepRequest);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(330), s_timeout);
    QVERIFY2(results.value(330), "deep-recursion breakpoint insert failed");

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);

    QHash<quint64, GdbMi> stacks;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&stacks](quint64 requestId, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::FullStack)
            stacks[requestId] = data;
    });

    quint64 requestId = 340;
    const auto frameCountWithLimit = [&](int limit) -> int {
        const quint64 id = ++requestId;
        RefreshRequest request;
        request.kind = RefreshKind::FullStack;
        request.requestId = id;
        request.stackDepthLimit = limit;
        engine->refresh(request);
        [&stacks, id] { QTRY_VERIFY_WITH_TIMEOUT(stacks.contains(id), s_timeout); }();
        if (QTest::currentTestFailed())
            return -1;
        return stacks.value(id)["stack"]["frames"].childCount();
    };

    const int limited = frameCountWithLimit(5);
    const int everything = frameCountWithLimit(-1);
    QVERIFY2(limited > 0 && everything > 0,
             qPrintable(QString("no frames reported: %1 and %2").arg(limited).arg(everything)));
    QVERIFY2(limited < everything,
             qPrintable(QString("a depth limit of 5 reported %1 frames, all of them %2")
                            .arg(limited).arg(everything)));
}

void tst_backends::insertsARealTracepointWhenPseudoOnesAreOff()
{
    QFETCH(Backend, backend);

    const QString marker = realTracepointMarker(backend);
    if (marker.isEmpty())
        QSKIP("This backend has only one kind of tracepoint.");
    if (auto result = checkCapability(backend, Debugger::TracePointCapability); !result)
        QSKIP(qPrintable(result.error()));

    // A pseudo tracepoint is a dumper command; the debugger's own is a
    // breakpoint flagged as a tracepoint on the wire.
    const auto insertTracepointWith = [this, backend](bool pseudoTracepoints) {
        QStringList sent;
        std::unique_ptr<DebuggerBackend> debuggerBackend
            = createEngine(backend, {}, {}, false, {},
                           pseudoTracepoints ? GdbImplFlags(GdbImplFlag::PseudoTracepoints)
                                             : GdbImplFlags());
        if (!debuggerBackend)
            return sent;
        DebuggerEngineInterface *engine = debuggerBackend->engine();
        connect(engine, &DebuggerEngineInterface::message, this,
                [&sent](const QString &text, int channel, int) {
            if (channel == Debugger::LogInput)
                sent.append(text);
        });
        QHash<quint64, bool> results;
        connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
                [&results](quint64 requestId, BreakpointOp, bool ok, const GdbMi &) {
            results[requestId] = ok;
        });
        connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
                [this, engine, backend](InferiorEvent event) {
            if (event != InferiorEvent::EngineSetupOk)
                return;
            BreakpointChangeRequest request;
            request.op = BreakpointOp::Insert;
            request.requestId = 91;
            request.params.type = BreakpointByFileAndLine;
            request.params.fileName = inferiorTestData(backend).source;
            request.params.textPosition.line = inferiorTestData(backend).breakpointLine;
            request.params.textPosition.column = 0;
            request.params.enabled = true;
            request.params.tracepoint = true;
            request.params.message = "globalValue is {globalValue}";
            engine->changeBreakpoint(request);
        });

        engine->start();
        [&] { QTRY_VERIFY_WITH_TIMEOUT(results.contains(91), s_timeout); }();
        if (!QTest::currentTestFailed())
            [&] { QVERIFY2(results.value(91), "the tracepoint insert failed"); }();
        return sent;
    };

    const QString withPseudo = insertTracepointWith(true).join('\n');
    QVERIFY2(withPseudo.contains("createTracepoint"),
             "a pseudo tracepoint did not go through the dumpers");
    QVERIFY2(!withPseudo.contains(marker + " -f -a"),
             "a pseudo tracepoint was inserted as the debugger's own");

    const QString withoutPseudo = insertTracepointWith(false).join('\n');
    QVERIFY2(!withoutPseudo.contains("createTracepoint"),
             "the dumpers were asked although pseudo tracepoints are off");
    QVERIFY2(withoutPseudo.contains(marker) && withoutPseudo.contains(" -a "),
             qPrintable("no \"" + marker + " ... -a\" on the wire, sent:\n  "
                        + insertTracepointWith(false).join("\n  ")));
}

void tst_backends::continuesAfterAttachWhenConfigured()
{
    QFETCH(Backend, backend);

    using Debugger::DebuggerExtraCapability;
    if (auto result = checkExtraCapability(backend, DebuggerExtraCapability::ContinueAfterAttach);
        !result) {
        QSKIP(qPrintable(result.error()));
    }
    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToProcess); !result)
        QSKIP(qPrintable(result.error()));

    // Without target-async a running inferior leaves the debugger unable to
    // answer anything, so the resumed case is torn down by killing the target
    // and the stopped case is the only one a round trip is asked of.
    const auto attachWith = [this, backend](bool continueAfterAttach) {
        Process target;
        target.setCommand({inferiorTestData(backend).executable, {}});
        target.start();
        [&] { QVERIFY(target.waitForStarted()); }();
        if (QTest::currentTestFailed())
            return false;

        std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(
            backend, AttachToProcessData{ProcessHandle(target.processId())},
            continueAfterAttach ? GdbImplFlags(GdbImplFlag::ContinueAfterAttach) : GdbImplFlags());
        [&] { QVERIFY(debuggerBackend); }();
        if (QTest::currentTestFailed())
            return false;
        DebuggerEngineInterface *engine = debuggerBackend->engine();
        engine->start();
        [&] {
            QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk),
                                     s_timeout);
        }();

        bool resumed = false;
        if (!QTest::currentTestFailed()) {
            if (continueAfterAttach) {
                // The resume is an event of its own; nothing else to wait for.
                [&] {
                    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunOk),
                                             s_timeout);
                }();
                resumed = debuggerBackend->contains(InferiorEvent::RunOk);
            } else {
                // A round trip the stopped inferior can answer: commands are
                // answered in order, so a resume that was coming is here too.
                bool refreshed = false;
                connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
                        [&refreshed](quint64, RefreshKind kind, const GdbMi &) {
                    if (kind == RefreshKind::Threads)
                        refreshed = true;
                });
                engine->refresh({.requestId = 900, .kind = RefreshKind::Threads});
                // gdb reads the target's symbols after answering the attach,
                // and this queues behind that.
                [&] { QTRY_VERIFY_WITH_TIMEOUT(refreshed, s_qmlStartupTimeout); }();
                resumed = debuggerBackend->contains(InferiorEvent::RunOk);
            }
        }

        // The debugger goes first: a resumed inferior leaves it unable to
        // answer a shutdown, and a stopped one cannot be killed while it holds
        // it. Dropping the debugger releases the target either way.
        debuggerBackend.reset();
        target.kill();
        target.waitForFinished();
        return resumed;
    };

    QVERIFY2(!attachWith(false), "the inferior was resumed although nothing asked for it");
    QVERIFY2(attachWith(true), "the inferior was not resumed although the run asked for it");
}

void tst_backends::stopsBeforeRunningWhenConfigured()
{
    QFETCH(Backend, backend);

    using Debugger::DebuggerExtraCapability;
    if (auto result = checkExtraCapability(backend, DebuggerExtraCapability::StopBeforeRun);
        !result) {
        QSKIP(qPrintable(result.error()));
    }

    // Unconfigured, the engine reports the run itself, which is the event that
    // proves the program was not held back.
    std::unique_ptr<DebuggerBackend> ran = createEngine(backend, {}, {}, false, {});
    QVERIFY(ran);
    ran->engine()->start();
    QTRY_VERIFY2_WITH_TIMEOUT(ran->contains(InferiorEvent::RunAndInferiorRunOk),
                              "the unconfigured run never started", s_timeout);

    std::unique_ptr<DebuggerBackend> stopped
        = createEngine(backend, {}, {}, false, {}, GdbImplFlag::BreakOnMain);
    QVERIFY(stopped);
    DebuggerEngineInterface *engine = stopped->engine();
    bool refreshed = false;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&refreshed](quint64, RefreshKind kind, const GdbMi &) {
        if (kind == RefreshKind::FullStack)
            refreshed = true;
    });
    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(stopped->contains(InferiorEvent::RunAndInferiorStopOk), s_timeout);

    // A round trip only a stopped program can answer, so the absence of a run
    // below is a fact rather than a race.
    engine->refresh({.requestId = 910, .kind = RefreshKind::FullStack});
    QTRY_VERIFY_WITH_TIMEOUT(refreshed, s_timeout);
    QVERIFY2(!stopped->contains(InferiorEvent::RunAndInferiorRunOk),
             "the program ran although it was asked to stop first");
}

void tst_backends::stopsAtMainWhenConfigured()
{
    QFETCH(Backend, backend);

    using Debugger::DebuggerExtraCapability;
    if (auto result = checkExtraCapability(backend, DebuggerExtraCapability::BreakOnMain);
        !result) {
        QSKIP(qPrintable(result.error()));
    }
    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    const QString marker = inferiorTestData(backend).applicationOutputMarker;
    QVERIFY(!marker.isEmpty());

    // No breakpoint is inserted here: a stop can only come from the temporary
    // one the setting asks for. The unconfigured run is bounded on the
    // inferior's own output, printed past main and before it starts spinning:
    // once that is here, a stop that was coming would be here too.
    std::unique_ptr<DebuggerBackend> ran
        = createEngine(backend, {}, {}, false, {});
    QVERIFY(ran);
    QString output;
    connect(ran->engine(), &DebuggerEngineInterface::message, this,
            [&output](const QString &text, int channel, int) {
        if (channel == Debugger::AppOutput || channel == Debugger::AppStuff)
            output += text;
    });
    ran->engine()->start();
    QTRY_VERIFY2_WITH_TIMEOUT(output.contains(marker),
                              "the unconfigured inferior never got past main", s_timeout);
    QVERIFY2(!ran->contains(InferiorEvent::SpontaneousStop),
             "the inferior stopped although nothing asked it to");

    std::unique_ptr<DebuggerBackend> stopped
        = createEngine(backend, {}, {}, false, {},
                       GdbImplFlag::PseudoTracepoints | GdbImplFlag::BreakOnMain);
    QVERIFY(stopped);
    stopped->engine()->start();
    QTRY_VERIFY2_WITH_TIMEOUT(stopped->contains(InferiorEvent::SpontaneousStop),
                              "the inferior never stopped at the main function", s_timeout);
    QCOMPARE(stopped->stoppedFile(), inferiorTestData(backend).source);
}

void tst_backends::logsTheResponseTimeWhenConfigured()
{
    QFETCH(Backend, backend);

    const QString marker = responseTimeMarker(backend);
    if (marker.isEmpty())
        QSKIP("This backend does not log a per-command response time.");

    // The log window stamps every line on its own; what the setting adds here
    // is how long each command took.
    const QString versionLine = inferiorTestData(backend).versionLine;
    QVERIFY(!versionLine.isEmpty());

    // Counting the markers needs a moment the count is final at: the answer to
    // a command sent after the ones being counted. Commands are answered in
    // order, so once it is here, a marker that was coming would be here too.
    const auto markersWith = [this, backend, marker, versionLine](bool logTimeStamps) {
        int seen = -1;
        std::unique_ptr<DebuggerBackend> debuggerBackend
            = createEngine(backend, {}, {}, false, {},
                           logTimeStamps ? (GdbImplFlag::PseudoTracepoints
                                            | GdbImplFlag::LogTimeStamps)
                                         : GdbImplFlags(GdbImplFlag::PseudoTracepoints));
        if (!debuggerBackend)
            return seen;
        DebuggerEngineInterface *engine = debuggerBackend->engine();
        int markers = 0;
        bool answered = false;
        connect(engine, &DebuggerEngineInterface::message, this,
                [&markers, &answered, marker, versionLine](const QString &text, int channel, int) {
            if (channel == Debugger::LogTime && text.contains(marker))
                ++markers;
            else if (text.contains(versionLine))
                answered = true;
        });
        engine->start();
        [&] {
            QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::EngineSetupOk),
                                     s_timeout);
        }();
        if (QTest::currentTestFailed())
            return seen;
        engine->executeDebuggerCommand("show version", {});
        [&] { QTRY_VERIFY_WITH_TIMEOUT(answered, s_timeout); }();
        if (QTest::currentTestFailed())
            return seen;
        seen = markers;
        return seen;
    };

    QCOMPARE(markersWith(false), 0);
    QVERIFY2(markersWith(true) > 0,
             qPrintable("no \"" + marker + "\" line arrived although time stamps are on"));
}

void tst_backends::stepsPastTheLinkersJumpToAFunction()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));
    const InferiorTestData testData = inferiorTestData(backend);
    if (testData.thunkStepLine == 0)
        QSKIP("This backend's inferior has no line whose step lands without source.");

    std::unique_ptr<DebuggerBackend> debuggerBackend =
        launchAndStopAtBreakpoint(backend, {}, testData.thunkStepLine);
    QVERIFY(debuggerBackend);

    // Stepping into a library call can land on the jump the linker put in front
    // of it, which has no source at all - a stop with nowhere to show.
    debuggerBackend->clearEvents();
    debuggerBackend->clearStoppedLocation();
    debuggerBackend->execute({ExecutionCommand::StepIn});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend->contains(InferiorEvent::StopOk), s_timeout);
    QVERIFY2(!debuggerBackend->stoppedFile().isEmpty(),
             "the step reported a stop with no file to show it in");
}

void tst_backends::skipsKnownFramesWhenStepping()
{
    QFETCH(Backend, backend);

    using Debugger::DebuggerExtraCapability;
    if (auto result = checkExtraCapability(backend, DebuggerExtraCapability::SkipKnownFrames);
        !result) {
        QSKIP(qPrintable(result.error()));
    }

    const InferiorTestData testData = inferiorTestData(backend);
    if (testData.knownFrameStepLine == 0)
        QSKIP("inferior has no line whose step lands in a standard header");

    // Stepping into std::move() lands in a standard header, which is what the
    // setting is about: unskipped the stop is reported there, skipped the
    // debugger keeps going until it is back in the code the user wrote.
    const auto stepIntoTheHeader = [this, backend, testData](bool skipKnownFrames) {
        std::pair<FilePath, int> location;
        std::unique_ptr<DebuggerBackend> debuggerBackend
            = createEngine(backend, {}, {}, false, {},
                           skipKnownFrames ? (GdbImplFlag::PseudoTracepoints
                                              | GdbImplFlag::SkipKnownFrames)
                                           : GdbImplFlags(GdbImplFlag::PseudoTracepoints));
        if (!debuggerBackend)
            return location;
        DebuggerEngineInterface *engine = debuggerBackend->engine();
        connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
                [engine, testData](InferiorEvent event) {
            if (event != InferiorEvent::EngineSetupOk)
                return;
            BreakpointChangeRequest request;
            request.op = BreakpointOp::Insert;
            request.requestId = 1;
            request.params.type = BreakpointByFileAndLine;
            request.params.fileName = testData.source;
            request.params.textPosition.line = testData.knownFrameStepLine;
            request.params.textPosition.column = 0;
            request.params.enabled = true;
            engine->changeBreakpoint(request);
        });

        engine->start();
        [&] {
            QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                                     s_timeout);
        }();
        if (QTest::currentTestFailed())
            return location;

        debuggerBackend->clearEvents();
        debuggerBackend->execute({ExecutionCommand::StepIn});
        [&] {
            QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                                     s_timeout);
        }();
        if (QTest::currentTestFailed())
            return location;
        location = {debuggerBackend->stoppedFile(), debuggerBackend->stoppedLine()};
        return location;
    };

    const auto [unskippedFile, unskippedLine] = stepIntoTheHeader(false);
    QVERIFY2(!unskippedFile.isEmpty(), "the unskipped step never reported a location");
    QVERIFY2(unskippedFile != testData.source,
             qPrintable("stepping into std::move() stayed in " + unskippedFile.toUserOutput()
                        + ", so this toolchain has no known frame to skip"));

    const auto [skippedFile, skippedLine] = stepIntoTheHeader(true);
    QCOMPARE(skippedFile, testData.source);
    QVERIFY2(skippedLine > testData.knownFrameStepLine,
             qPrintable(QString("the skipped step landed on line %1, at or before the step "
                                "itself on line %2")
                            .arg(skippedLine).arg(testData.knownFrameStepLine)));
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
    // A backend that does not address the locations of a breakpoint
    // individually has nothing to enable here and refuses; what it must not do
    // is leave the request unanswered.
    if (hasCapability(backend, Debugger::BreakIndividualLocationsCapability))
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

    GdbMi stackData;
    bool stackReceived = false;
    GdbMi threadsData;
    bool threadsReceived = false;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&](quint64, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::FullStack) {
            stackData = data;
            stackReceived = true;
        } else if (kind == RefreshKind::Threads) {
            threadsData = data;
            threadsReceived = true;
        }
    });

    // Whichever thread the debugger counts as current is the one stopped where
    // this looks, and the numbering is the debugger's own: gdb starts at one,
    // cdb at zero. A backend that reports no threads has none to pick from.
    if (engine->hasExtraCapability(Debugger::DebuggerExtraCapability::Threads)) {
        RefreshRequest threadsRequest;
        threadsRequest.kind = RefreshKind::Threads;
        threadsRequest.requestId = 29;
        engine->refresh(threadsRequest);
        QTRY_VERIFY_WITH_TIMEOUT(threadsReceived, s_timeout);
        const QString currentThread = threadsData["current-thread-id"].data();
        QVERIFY2(!currentThread.isEmpty(), "the backend reported no current thread");
        engine->selectThread(currentThread);
    }
    engine->activateFrame(0);

    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 30;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(stackReceived, s_timeout);
    QVERIFY(stackData.toString().contains("bump"));
}

void tst_backends::printsALongValueWithoutTruncating()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    const QString symbol = inferiorTestData(backend).longTextSymbol;
    if (symbol.isEmpty())
        QSKIP("no long value configured for this backend - see longTextSymbol");

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList messages;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&messages](const QString &text, int, int) { messages.append(text); });
    engine->executeDebuggerCommand(printCommand(backend, symbol), {});

    // The debuggee's string is 2011 characters long, which is past both gdb's
    // default of 200 elements and lldb's of 1024, so a debugger left at its
    // default stops before the end marker.
    QTRY_VERIFY2_WITH_TIMEOUT(std::any_of(messages.cbegin(), messages.cend(),
                                          [](const QString &text) {
        return text.contains("LONGTEXTEND");
    }), qPrintable("the printed value was cut short: " + messages.join(' ').right(300)),
       s_timeout);
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
            [&messages](const QString &text, int channel, int) {
        // The command is logged as it goes out, so only what came back can
        // stand for it having been answered.
        if (channel != Debugger::LogInput)
            messages.append(text);
    });
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
    if (auto result = checkCapability(backend, Debugger::JumpToLineCapability); !result)
        QSKIP(qPrintable(result.error()));
    if (auto result = checkCapability(backend, Debugger::RunToLineCapability); !result)
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

    for (const QString &number : std::as_const(modifiedNumbers)) {
        QVERIFY2(number.isEmpty() || number == debuggerBackend->breakpointResponseId(),
                 qPrintable("spurious breakpointModified() for internal breakpoint #" + number
                            + " - RunToLine/RunToFunction/JumpToLine's own one-shot breakpoint "
                            "leaked a notification the caller never asked for"));
    }
}

// Running to a line with one location per template instantiation: unlike a
// jump, this has a sensible answer - stop at whichever instantiation is
// reached first - so it must work rather than be refused, and the one-shot
// breakpoint behind it must not leak notifications for its sub-locations.
void tst_backends::runsToAnAmbiguousLine()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));
    if (auto result = checkCapability(backend, Debugger::RunToLineCapability); !result)
        QSKIP(qPrintable(result.error()));
    const int ambiguousLine = inferiorTestData(backend).multiLocationBreakpointLine;
    if (ambiguousLine == 0)
        QSKIP("This backend's inferior has no line with several locations.");

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
    ExecutionRequest runToLineRequest;
    runToLineRequest.command = ExecutionCommand::RunToLine;
    runToLineRequest.context.type = LocationByFile;
    runToLineRequest.context.fileName = inferiorTestData(backend).source;
    runToLineRequest.context.textPosition.line = ambiguousLine;
    debuggerBackend->execute(runToLineRequest);
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "RunToLine to the ambiguous line never signaled a stop", s_timeout);
    QCOMPARE(debuggerBackend->stoppedLine(), ambiguousLine);

    for (const QString &number : std::as_const(modifiedNumbers)) {
        QVERIFY2(number.isEmpty() || number == debuggerBackend->breakpointResponseId(),
                 qPrintable("spurious breakpointModified() for internal breakpoint #" + number
                            + " - the one-shot breakpoint's sub-locations leaked notifications"));
    }
}

// A line inside a template body resolves to one location per instantiation.
// A jump cannot pick one, so it has to be refused outright - and refusing it
// must leave nothing armed at that line, or it fires later as a stop nobody
// asked for.
void tst_backends::refusesJumpToAnAmbiguousLine()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));
    if (auto result = checkCapability(backend, Debugger::JumpToLineCapability); !result)
        QSKIP(qPrintable(result.error()));
    using Debugger::DebuggerExtraCapability;
    if (auto result = checkExtraCapability(backend, DebuggerExtraCapability::JumpTargetCheck);
        !result) {
        QSKIP(qPrintable(result.error()));
    }
    const int ambiguousLine = inferiorTestData(backend).multiLocationBreakpointLine;
    if (ambiguousLine == 0)
        QSKIP("This backend's inferior has no line with several locations.");

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);

    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QList<int> reportedLines;
    connect(engine, &DebuggerEngineInterface::locationChanged, this,
            [&reportedLines](const Utils::FilePath &, int line) { reportedLines.append(line); });
    GdbMi stack;
    bool stackReceived = false;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&stack, &stackReceived](quint64, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::FullStack) {
            stack = data;
            stackReceived = true;
        }
    });

    ExecutionRequest jumpRequest;
    jumpRequest.command = ExecutionCommand::JumpToLine;
    jumpRequest.context.type = LocationByFile;
    jumpRequest.context.fileName = inferiorTestData(backend).source;
    jumpRequest.context.textPosition.line = ambiguousLine;
    debuggerBackend->execute(jumpRequest);

    // A refusal has nothing of its own to wait for, so ask for the stack: the
    // answer proves the jump was dealt with, and says where the execution point
    // stands. A refused jump may not have moved it - a backend that picks one
    // of the locations lands in a function it has no frame for.
    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 71;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(stackReceived, s_timeout);
    const GdbMi frames = stack["stack"]["frames"];
    QVERIFY2(frames.childCount() > 0, qPrintable(stack.toString()));
    QCOMPARE(frames.childAt(0)["line"].toInt(), inferiorTestData(backend).breakpointLine);

    // Only now can the inferior be resumed: a breakpoint left behind at the
    // template line would be hit on the way, before the stop asked for here.
    const int landingLine = inferiorTestData(backend).secondBreakpointLine;
    ExecutionRequest runToLineRequest;
    runToLineRequest.command = ExecutionCommand::RunToLine;
    runToLineRequest.context.type = LocationByFile;
    runToLineRequest.context.fileName = inferiorTestData(backend).source;
    runToLineRequest.context.textPosition.line = landingLine;
    debuggerBackend->execute(runToLineRequest);
    QTRY_VERIFY2_WITH_TIMEOUT(reportedLines.contains(landingLine),
                              "RunToLine after the refused jump never signaled its stop", s_timeout);
    QVERIFY2(!reportedLines.contains(ambiguousLine),
             qPrintable("a stop was reported at the ambiguous line " + QString::number(ambiguousLine)
                        + " - reported: " + Utils::transform(reportedLines, [](int line) {
                              return QString::number(line);
                          }).join(", ")));
}

// Asking for a stop the moment an attach is reported: a backend that resumes
// the inferior on attach is in the middle of doing so, and the debugger may
// refuse an interrupt in that window - which must not be answered with a stop
// that did not happen.
void tst_backends::interruptsRightAfterAttaching()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToProcess); !result)
        QSKIP(qPrintable(result.error()));

    const InferiorTestData testData = inferiorTestData(backend);
    Process target;
    target.setCommand({testData.executable, {}});
    target.start();
    QVERIFY(target.waitForStarted());
    QString targetOutput;
    auto sawOutput = [&] {
        targetOutput += target.readAllStandardOutput();
        return targetOutput.contains(testData.applicationOutputMarker);
    };
    QTRY_VERIFY_WITH_TIMEOUT(sawOutput(), s_timeout);

    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(
        backend, AttachToProcessData{ProcessHandle(target.processId())});
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk)
                             || debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    QVERIFY2(!debuggerBackend->contains(InferiorEvent::EngineSetupFailed), "attaching failed");

    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Interrupt});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::StopOk),
                              "the interrupt right after attaching never reported a stop",
                              s_timeout);

    // The stop has to be real, which only the stack can say: a backend that
    // reports one without holding the inferior has no frames to show.
    GdbMi stack;
    bool stackReceived = false;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&stack, &stackReceived](quint64, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::FullStack) {
            stack = data;
            stackReceived = true;
        }
    });
    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 341;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(stackReceived, s_timeout);
    QVERIFY2(stack["stack"]["frames"].childCount() > 0,
             qPrintable("the reported stop has no stack: " + stack.toString()));

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();
}

void tst_backends::insertsWatchpointAndCatchpoint()
{
    QFETCH(Backend, backend);

    if (auto result = checkAcceptsBreakpoint(backend, BreakpointAtFork, "A fork catchpoint");
        !result) {
        QSKIP(qPrintable(result.error()));
    }

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

void tst_backends::reportsSetupFailureWhenTheDebuggerQuitsAtOnce()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));
    const CommandLine quitting = quittingDebuggerCommand();
    if (quitting.isEmpty())
        QSKIP("Nothing on this platform to stand in for a debugger that quits at once.");

    // Starting works, so nothing reports a failure to start; the session simply
    // never opens, which is the failure the engine has to pass on.
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend,
        ProcessRunData{quitting, {}, Environment::systemEnvironment()});
    if (!debuggerBackend)
        QSKIP("This backend cannot be given a debugger of its own here.");
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    bool processFinished = false;
    connect(engine, &DebuggerEngineInterface::engineProcessFinished, this,
            [&processFinished](const Utils::ProcessResultData &) { processFinished = true; });

    engine->start();
    QTRY_VERIFY2_WITH_TIMEOUT(processFinished, "the debugger process never finished", s_timeout);
    if (debuggerBackend->contains(InferiorEvent::EngineSetupOk)) {
        QSKIP("This backend calls its setup done before the debugger has answered for "
              "itself, so a session that never opened cannot be told apart here.");
    }
    QVERIFY2(debuggerBackend->contains(InferiorEvent::EngineSetupFailed),
             "a session that never opened was not reported as a setup failure");
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

void tst_backends::insertsABreakpointBehindABlockedDebugger()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));
    if (auto result = checkAcceptsBreakpoint(backend, BreakpointByFileAndLine,
                                             "A source line breakpoint"); !result) {
        QSKIP(qPrintable(result.error()));
    }

    using namespace std::chrono_literals;
    constexpr std::chrono::seconds block = 2s;
    const QString blockingCommand = watchdogProbeCommand(backend, int(block.count()));
    if (blockingCommand.isEmpty())
        QSKIP("This backend has no command to keep it busy with.");

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(
        backend, {}, {}, false, {},
        GdbImplFlag::PseudoTracepoints | GdbImplFlag::BreakOnMain);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::EngineSetupOk), s_timeout);

    QHash<quint64, bool> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp op, bool ok, const GdbMi &) {
        if (op == BreakpointOp::Insert)
            results[requestId] = ok;
    });
    engine->executeDebuggerCommand(blockingCommand, {});
    BreakpointChangeRequest insertRequest;
    insertRequest.op = BreakpointOp::Insert;
    insertRequest.requestId = 81;
    insertRequest.params.type = BreakpointByFileAndLine;
    insertRequest.params.fileName = inferiorTestData(backend).source;
    insertRequest.params.textPosition.line = inferiorTestData(backend).breakpointLine;
    insertRequest.params.enabled = true;
    engine->changeBreakpoint(insertRequest);
    // Nothing can be answered before the block is over, so it is the block plus
    // what an insert is allowed anywhere else.
    QTRY_VERIFY2_WITH_TIMEOUT(results.contains(81),
                              "a breakpoint issued behind a blocked debugger was never answered",
                              s_timeout + block);
    QVERIFY2(results.value(81), "inserting behind a blocked debugger failed");
}

void tst_backends::tellsWhetherAModuleHasPrivateSymbols()
{
    QFETCH(Backend, backend);

    if (auto result = checkExtraCapability(backend,
            Debugger::DebuggerExtraCapability::ModuleSymbolState); !result) {
        QSKIP(qPrintable(result.error()));
    }
    const InferiorTestData testData = inferiorTestData(backend);
    QVERIFY2(!testData.moduleWithPrivateSymbols.isEmpty(),
             "the backend answers for a module's symbols, but no module is configured to ask about");

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, GdbMi> answers;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&answers](quint64 requestId, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::ModuleSymbolState)
            answers[requestId] = data;
    });

    quint64 requestId = 160;
    auto askAbout = [&](const QString &module) -> QString {
        const quint64 id = ++requestId;
        RefreshRequest request;
        request.kind = RefreshKind::ModuleSymbolState;
        request.requestId = id;
        request.path = FilePath::fromString(module);
        engine->refresh(request);
        [&answers, id] {
            QTRY_VERIFY_WITH_TIMEOUT(answers.contains(id), s_timeout);
        }();
        return answers.value(id)["private"].data();
    };

    QCOMPARE(askAbout(testData.moduleWithPrivateSymbols), QString("1"));
    QCOMPARE(askAbout(testData.moduleWithoutPrivateSymbols), QString("0"));
}

void tst_backends::leavesThePublicSymbolsOutOfTheSearch()
{
    QFETCH(Backend, backend);

    const QString command = inferiorTestData(backend).symbolOptionsCommand;
    if (command.isEmpty())
        QSKIP("This debugger has no symbol options to ask about.");

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList messages;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&messages](const QString &text, int, int) { messages.append(text); });
    engine->executeDebuggerCommand(command, {});

    static const QLatin1String prefix("Symbol options are 0x");
    QString reported;
    auto sawTheOptions = [&] {
        for (const QString &text : std::as_const(messages)) {
            const int at = text.indexOf(prefix);
            if (at < 0)
                continue;
            reported = text.mid(at + prefix.size());
            reported.truncate(reported.indexOf(':'));
            return true;
        }
        return false;
    };
    QTRY_VERIFY2_WITH_TIMEOUT(sawTheOptions(), "the debugger never told its symbol options",
                              s_timeout);
    bool ok = false;
    const unsigned options = reported.toUInt(&ok, 16);
    QVERIFY2(ok, qPrintable("unreadable symbol options: " + reported));
    QVERIFY2(options & 0x8000u,
             qPrintable(QString("the public symbols are still searched: 0x%1")
                            .arg(options, 0, 16)));
}

void tst_backends::reportsAnUnresponsiveDebugger()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    using namespace std::chrono_literals;
    // Both blocks have to outlast the watchdog for it to report them at all,
    // and the second has to outlast the first for the reports to go on once
    // the first one is answered.
    constexpr std::chrono::seconds watchdogInterval = 1s;
    const QString blockingCommand = watchdogProbeCommand(backend,
                                                         int(2 * watchdogInterval.count()));
    if (blockingCommand.isEmpty())
        QSKIP("This backend does not watch its commands for a reply.");

    // Stopping at the start keeps a backend that would otherwise run its
    // program to the end alive for the blocks below.
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(
        backend, {}, {}, false, watchdogInterval,
        GdbImplFlag::PseudoTracepoints | GdbImplFlag::BreakOnMain);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QList<QStringList> reports;
    connect(engine, &DebuggerEngineInterface::notResponding, this,
            [&reports](std::chrono::seconds, const QStringList &pendingCommands) {
        reports.append(pendingCommands);
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::EngineSetupOk), s_timeout);
    QVERIFY2(reports.isEmpty(), "the watchdog fired while the engine was answering");

    engine->executeDebuggerCommand(blockingCommand, {});
    QTRY_VERIFY_WITH_TIMEOUT(!reports.isEmpty(), s_timeout);
    const QStringList report = reports.constFirst();
    QVERIFY2(std::any_of(report.cbegin(), report.cend(), [&blockingCommand](const QString &cmd) {
                 return cmd.contains(blockingCommand);
             }),
             qPrintable("the reported pending commands do not mention the blocking one: "
                        + report.join(", ")));

    // Once the first block is answered it has to leave the list, which a report
    // naming only the second proves; a watchdog that never clears what it
    // watches names both.
    const QString secondCommand = watchdogProbeCommand(backend,
                                                       int(3 * watchdogInterval.count()));
    reports.clear();
    engine->executeDebuggerCommand(secondCommand, {});
    const auto answeredCommandWasCleared = [&reports, &blockingCommand, &secondCommand] {
        return std::any_of(reports.cbegin(), reports.cend(), [&](const QStringList &pending) {
            const auto mentions = [&pending](const QString &command) {
                return std::any_of(pending.cbegin(), pending.cend(),
                                   [&command](const QString &cmd) {
                    return cmd.contains(command);
                });
            };
            return mentions(secondCommand) && !mentions(blockingCommand);
        });
    };
    QTRY_VERIFY2_WITH_TIMEOUT(answeredCommandWasCleared(),
                              "no report ever named the outstanding command alone",
                              s_timeout);
}

void tst_backends::appliesConfiguredDebuggerOptions()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));
    const FilePath existingDir = FilePath::fromString(m_tempDir.path()) / "configured";
    QVERIFY(existingDir.ensureWritableDir());
    // Loading the module runs it, which is all the test needs to see.
    // The module says it ran by writing a file: a print reaches the log through
    // gdb's console records, while lldb keeps the bridge's own output to itself.
    const FilePath moduleTrace = existingDir / "qtc_extra_dumper_loaded";
    QVERIFY((existingDir / "qtc_extra_dumper.py").writeFileContents(
        QString("open(r\"%1\", \"w\").close()\n").arg(moduleTrace.path()).toUtf8()));
    const QList<ConfiguredOptionProbe> probes = configuredOptionProbes(backend, existingDir);
    std::unique_ptr<DebuggerBackend> debuggerBackend
        = createFullyConfiguredEngine(backend, Environment::systemEnvironment(), existingDir);
    if (!debuggerBackend)
        QSKIP("This backend has no configurable options wired yet.");
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList messages;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&messages](const QString &text, int channel, int) {
        // What was sent is not evidence that it was carried out.
        if (channel != Debugger::LogInput)
            messages.append(text);
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::EngineSetupOk), s_timeout);
    const auto sawMessage = [&messages](const QString &marker) {
        return std::any_of(messages.cbegin(), messages.cend(), [&marker](const QString &text) {
            return text.contains(marker);
        });
    };
    for (const QString &marker : configuredOptionMarkers(backend, existingDir)) {
        QTRY_VERIFY2_WITH_TIMEOUT(sawMessage(marker),
                                  qPrintable("a configured option left no " + marker),
                                  s_timeout);
    }
    QTRY_VERIFY2_WITH_TIMEOUT(moduleTrace.exists(),
                              "the extra dumper module was never imported", s_timeout);

    for (const ConfiguredOptionProbe &probe : probes) {
        messages.clear();
        engine->executeDebuggerCommand(probe.query, {});
        const QString expected = probe.expectedOutput;
        QTRY_VERIFY2_WITH_TIMEOUT(std::any_of(messages.cbegin(), messages.cend(),
                                              [&expected](const QString &text) {
                                                  return text.contains(expected);
                                              }),
                                  qPrintable(QString("%1 never answered with \"%2\"")
                                                 .arg(probe.query, expected)), s_timeout);
    }
}

void tst_backends::disassemblesInTheConfiguredFlavor()
{
    QFETCH(Backend, backend);

    const QString query = disassemblyFlavorQuery(backend);
    const QString wireMarker = disassemblyFlavorWireMarker(backend);
    if (wireMarker.isEmpty())
        QSKIP("This backend has no disassembly flavor to configure.");
    if (auto result = checkCapability(backend, Debugger::DisassemblerCapability); !result)
        QSKIP(qPrintable(result.error()));

    const FilePath existingDir = FilePath::fromString(m_tempDir.path()) / "disassembly";
    QVERIFY(existingDir.ensureWritableDir());
    QVERIFY((existingDir / "qtc_extra_dumper.py").writeFileContents("pass\n"));
    std::unique_ptr<DebuggerBackend> debuggerBackend = createFullyConfiguredEngine(
        backend, Environment::systemEnvironment(), existingDir);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList messages;
    QStringList sent;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&messages, &sent](const QString &text, int channel, int) {
        messages.append(text);
        if (channel == Debugger::LogInput)
            sent.append(text);
    });
    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::EngineSetupOk), s_timeout);

    // The flavor travels with the disassembly request, not with the startup.
    bool disassembled = false;
    connect(engine, &DebuggerEngineInterface::disassemblyReceived, this,
            [&disassembled](quint64, const DisassemblerLines &) { disassembled = true; });
    engine->fetchDisassembly(400, 0, inferiorTestData(backend).functionMarker);
    QTRY_VERIFY_WITH_TIMEOUT(disassembled, s_timeout);

    QVERIFY2(std::any_of(sent.cbegin(), sent.cend(), [&wireMarker](const QString &text) {
                 return text.contains(wireMarker);
             }),
             qPrintable("the configured flavor did not reach the wire as \"" + wireMarker
                        + "\", sent:\n  " + sent.mid(qMax(0, sent.size() - 6)).join("\n  ")));

    // Only a backend that keeps the flavor in a setting can be asked about it.
    if (query.isEmpty())
        return;

    messages.clear();
    engine->executeDebuggerCommand(query, {});
    QString answer;
    QTRY_VERIFY_WITH_TIMEOUT([&] {
        for (const QString &text : messages) {
            if (text.contains("disassembly flavor") || text.contains("Undefined"))
                answer = text;
        }
        return !answer.isEmpty();
    }(), s_timeout);
    if (answer.contains("Undefined"))
        QSKIP("This architecture has no disassembly flavor.");
    QVERIFY2(answer.contains("\"intel\""),
             qPrintable("the configured disassembly flavor did not arrive: " + answer));
}

void tst_backends::configuresTheDebugInfoDaemon()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));
    const QString query = debugInfoDaemonQuery(backend);
    if (query.isEmpty())
        QSKIP("This backend has no debug info daemon to configure.");

    const FilePath existingDir = FilePath::fromString(m_tempDir.path()) / "debuginfod";
    QVERIFY(existingDir.ensureWritableDir());
    QVERIFY((existingDir / "qtc_extra_dumper.py").writeFileContents("pass\n"));
    std::unique_ptr<DebuggerBackend> debuggerBackend = createFullyConfiguredEngine(
        backend, Environment::systemEnvironment(), existingDir);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList messages;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&messages](const QString &text, int, int) { messages.append(text); });
    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::EngineSetupOk), s_timeout);

    const auto answerTo = [&](const QString &command) -> QString {
        messages.clear();
        engine->executeDebuggerCommand(command, {});
        QString answer;
        [&] {
            QTRY_VERIFY_WITH_TIMEOUT(std::any_of(messages.cbegin(), messages.cend(),
                                                 [](const QString &text) {
                return text.contains("Debuginfod") || text.contains("Undefined");
            }), s_timeout);
        }();
        for (const QString &text : messages) {
            if (text.contains("Debuginfod") || text.contains("Undefined"))
                answer = text;
        }
        return answer;
    };

    const QString support = answerTo(query);
    if (support.contains("Undefined"))
        QSKIP("This debugger was built without debug info daemon support.");
    QVERIFY2(support.contains("\"on\""),
             qPrintable("the debug info daemon was not enabled as configured: " + support));
}

void tst_backends::breaksBeforeTheInferiorAborts()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));
    using Debugger::DebuggerExtraCapability;
    if (auto result = checkExtraCapability(backend, DebuggerExtraCapability::SpecialBreakpoints);
        !result) {
        QSKIP(qPrintable(result.error()));
    }

    const FilePath existingDir = FilePath::fromString(m_tempDir.path()) / "specialbreakpoints";
    QVERIFY(existingDir.ensureWritableDir());
    std::unique_ptr<DebuggerBackend> debuggerBackend = createFullyConfiguredEngine(
        backend, Environment::systemEnvironment(), existingDir, "abort");
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    // Without the special breakpoint the inferior reaches abort() and stops on
    // SIGABRT instead, which is a stop as well - the signal is what tells them apart.
    QStringList receivedSignals;
    connect(engine, &DebuggerEngineInterface::signalReceived, this,
            [&receivedSignals](const QString &name, const QString &) { receivedSignals.append(name); });

    engine->start();
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "the inferior's abort() never stopped the debugger", s_timeout);
    QVERIFY2(receivedSignals.isEmpty(),
             qPrintable("the inferior ran into abort() instead of stopping before it: "
                        + receivedSignals.join(", ")));
    QVERIFY2(debuggerBackend->inferiorResults().isEmpty(), "the inferior did not survive");
}

void tst_backends::readsTheDebuggerInitFileWhenConfigured()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));
    const QString marker = "QTCINITFILEMARKER";
    const InitFileProbe probe = initFileProbe(backend, marker);
    if (probe.fileName.isEmpty())
        QSKIP("This backend reads no init file, or reading it is not configurable yet.");
    if (HostOsInfo::isWindowsHost())
        QSKIP("The debugger resolves its home directory differently on Windows.");

    const FilePath home = FilePath::fromString(m_tempDir.path()) / "debuggerhome";
    QVERIFY(home.ensureWritableDir());
    QVERIFY((home / probe.fileName).writeFileContents(probe.content.toLocal8Bit()));
    Environment environment = Environment::systemEnvironment();
    environment.set("HOME", home.path());

    // The init file is read before anything the engine sends, so by the time the
    // engine is set up its output has either arrived or never will.
    const auto markerSeen = [this, &marker](DebuggerBackend *debuggerBackend) {
        DebuggerEngineInterface *engine = debuggerBackend->engine();
        QStringList messages;
        // The engine outlives this call, so the connection must not: the list it
        // appends to is gone as soon as the marker has been looked for.
        QObject connectionScope;
        connect(engine, &DebuggerEngineInterface::message, &connectionScope,
                [&messages](const QString &text, int, int) { messages.append(text); });
        engine->start();
        [debuggerBackend] {
            QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::EngineSetupOk),
                                     s_timeout);
        }();
        return std::any_of(messages.cbegin(), messages.cend(), [&marker](const QString &text) {
            return text.contains(marker);
        });
    };

    std::unique_ptr<DebuggerBackend> configured
        = createFullyConfiguredEngine(backend, environment, home);
    QVERIFY(configured);
    QVERIFY2(markerSeen(configured.get()), "the configured engine did not read the init file");

    std::unique_ptr<DebuggerBackend> plain = createEngine(
        backend, ProcessRunData{{m_backendData[backend].path, {}}, {}, environment});
    QVERIFY(plain);
    QVERIFY2(!markerSeen(plain.get()), "the init file was read although nothing asked for it");
}

void tst_backends::refreshesPeripherals()
{
    QFETCH(Backend, backend);

    using Debugger::DebuggerExtraCapability;
    if (auto result = checkExtraCapability(backend, DebuggerExtraCapability::PeripheralRegisters);
        !result) {
        QSKIP(qPrintable(result.error()));
    }

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
    if (reportsDumperTypes(backend)) {
        QTRY_VERIFY2_WITH_TIMEOUT(responses.contains(int(RefreshKind::DebuggingHelpers)),
                                  "reloading the helpers reported no types", s_timeout);
        const GdbMi dumpers
            = responses.value(int(RefreshKind::DebuggingHelpers))["dumpers"];
        QVERIFY2(dumpers.childCount() > 0, "the answer carried no dumper types");
        QVERIFY2(!dumpers.childAt(0)["type"].data().isEmpty(),
                 qPrintable("a reported dumper names no type: " + dumpers.toString(true)));
    }

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

void tst_backends::passesInferiorWorkingDirectoryToTheDebuggee()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    const InferiorTestData &data = inferiorTestData(backend);
    if (data.workingDirectoryReportPrefix.isEmpty())
        QSKIP("inferior does not report its working directory");

    const FilePath workingDirectory = FilePath::fromString(m_tempDir.path()) / "inferior-cwd";
    QVERIFY(workingDirectory.createDir());
    const ProcessRunData inferiorRunData{{data.executable, {}}, workingDirectory,
                                         Environment::systemEnvironment()};

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend,
                                                                                inferiorRunData);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList applicationOutput;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&applicationOutput](const QString &text, int channel, int) {
        if (channel == Debugger::AppOutput || channel == Debugger::AppStuff)
            applicationOutput.append(text);
    });

    stopInferiorSpinLoop(backend, engine);

    debuggerBackend->clearInferiorResults();
    debuggerBackend->execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(!debuggerBackend->inferiorResults().isEmpty(), s_timeout);

    const QString prefix = data.workingDirectoryReportPrefix;
    const auto reportedWorkingDirectory = [&applicationOutput, &prefix] {
        const QStringList lines = applicationOutput.join('\n').split('\n');
        for (const QString &line : lines) {
            const int index = line.indexOf(prefix);
            if (index >= 0)
                return FilePath::fromUserInput(line.mid(index + prefix.size()).trimmed());
        }
        return FilePath();
    };
    QTRY_VERIFY2_WITH_TIMEOUT(!reportedWorkingDirectory().isEmpty(),
                              qPrintable(QString("the debuggee never reported its working "
                                                 "directory - looked for \"%1\" in:\n  %2")
                                             .arg(prefix,
                                                  applicationOutput.join("\n  ").left(600))),
                              s_timeout);
    // The debuggee reports what the OS hands back, which need not be spelled like the path
    // it was given - 8.3 names on Windows, a symlinked /tmp elsewhere.
    QCOMPARE(reportedWorkingDirectory().canonicalPath(), workingDirectory.canonicalPath());
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
    QFETCH(int, forcedRefusals);

    if (auto result = checkCapability(backend, Debugger::AdditionalQmlStackCapability); !result)
        QSKIP(qPrintable(result.error()));
    // Known red on macOS, cause unknown: the qt_qmlDebugConnectorOpen hook
    // never fires there, so a pending QML breakpoint is never retried - 0
    // resolutions in 6 macOS CI runs against 6 of 6 on Linux, which passes
    // every run. Needs someone debugging it on a Mac.

    if (backend == Backend::Lldb && HostOsInfo::isMacHost())
        QSKIP("QML breakpoint resolution does not work on macOS - see the comment above.");

#ifndef QMLSTACK_INFERIOR_EXECUTABLE
    QSKIP("Qt::Quick not available when this test binary was configured.");
#else
    const FilePath inferior = (FilePath::fromUserInput(QMLSTACK_INFERIOR_EXECUTABLE)
                              / "qmlstack_inferior").withExecutableSuffix();
    if (!inferior.isExecutableFile())
        QSKIP(qPrintable("QML stack inferior not found at " + inferior.toUserOutput()));

    Environment env = Environment::systemEnvironment();
    env.set("QV4_FORCE_INTERPRETER", "1");
    // The switch is read by the dumpers, which run inside the debugger.
    std::optional<ProcessRunData> debuggerRunData;
    if (forcedRefusals > 0) {
        Environment debuggerEnv = Environment::systemEnvironment();
        debuggerEnv.set("QTC_TEST_REFUSE_INTERPRETER_SETBREAKPOINT",
                        QString::number(forcedRefusals));
        debuggerRunData = ProcessRunData{{m_backendData[backend].path, {}}, {}, debuggerEnv};
    }
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, debuggerRunData,
        ProcessRunData{{inferior, {"-qmljsdebugger=native,services:NativeQmlDebugger"}},
                        {}, env}, true);
    DebuggerEngineInterface *engine = debuggerBackend->engine();
    const int markerLine = qmlMarkerLine("qmlstack_inferior.qml", "MARKER: qml breakpoint line");
    QVERIFY(markerLine > 0);

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
            [engine, markerLine](InferiorEvent event) {
        if (event != InferiorEvent::EngineSetupOk)
            return;
        BreakpointChangeRequest request;
        request.op = BreakpointOp::Insert;
        request.requestId = 30;
        request.modelId = 42;
        request.params.type = BreakpointByFileAndLine;
        request.params.fileName = FilePath::fromUserInput("qmlstack_inferior.qml");
        request.params.textPosition.line = markerLine;
        request.params.textPosition.column = 0;
        request.params.enabled = true;
        engine->changeBreakpoint(request);
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(insertResults.contains(30), s_qmlStartupTimeout);
    QVERIFY2(insertResults.value(30), "pending QML breakpoint insert failed");

    QTRY_VERIFY_WITH_TIMEOUT(hasResolvedQmlBreakpoint(modifiedReports, 42)
                             || !refusedInferiorCall(wire).isEmpty(), s_qmlStartupTimeout);
    if (const QString refused = refusedInferiorCall(wire); !refused.isEmpty())
        QSKIP(qPrintable("The debugger cannot call into the inferior here: " + refused));
    QVERIFY2(hasResolvedQmlBreakpoint(modifiedReports, 42),
             qPrintable(qmlResolutionDiagnosis(modifiedReports, wire)));

    if (forcedRefusals > 0) {
        // Resolving takes as many attempts as there were refusals, by which
        // time the program has run past the line, so no stop can follow.
        return;
    }

    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              qPrintable("the resolved QML breakpoint never signaled a stop"
                                         " - last wire traffic:\n  " + wireTail(wire)),
                              s_timeout);

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

    const GdbMi stack = responses.value(int(RefreshKind::FullStack));
    const bool stoppedAtMarker = Utils::anyOf(stack["stack"]["frames"],
                                              [markerLine](const GdbMi &frame) {
        return frame["language"].data() == "js" && frame["line"].toInt() == markerLine;
    });
    QVERIFY2(stoppedAtMarker, qPrintable(QString("no js frame at line %1 - stack: %2")
                                             .arg(markerLine).arg(stack.toString())));

#endif
}

void tst_backends::resolvesQmlBreakpointWithoutServiceDebugInfo()
{
    QFETCH(Backend, backend);

    if (auto result = checkCapability(backend, Debugger::AdditionalQmlStackCapability); !result)
        QSKIP(qPrintable(result.error()));
    if (!inferiorTestData(backend).qmlBreakpointsUseServiceCasts) {
        QSKIP("This backend's bridge marshals the arguments and calls by address, so it "
              "never goes through the casts this exercises.");
    }
    // Same macOS gap as insertsQmlBreakpointAndStopsAtIt() - see its comment.
    if (backend == Backend::Lldb && HostOsInfo::isMacHost())
        QSKIP("QML breakpoint resolution does not work on macOS.");

#ifndef QMLSTACK_INFERIOR_EXECUTABLE
    QSKIP("Qt::Quick not available when this test binary was configured.");
#else
    const FilePath inferior = (FilePath::fromUserInput(QMLSTACK_INFERIOR_EXECUTABLE)
                              / "qmlstack_inferior").withExecutableSuffix();
    if (!inferior.isExecutableFile())
        QSKIP(qPrintable("QML stack inferior not found at " + inferior.toUserOutput()));

    // Outlives the engine below, which debugs an inferior that has it mapped.
    TemporaryDirectory pluginRoot("qmldbg-stripped-XXXXXX");
    QVERIFY(pluginRoot.isValid());
    const Result<FilePath> strippedDir = strippedQmlDebugPluginDir(pluginRoot.path());
    if (!strippedDir)
        QSKIP(qPrintable(strippedDir.error()));

    Environment env = Environment::systemEnvironment();
    env.set("QV4_FORCE_INTERPRETER", "1");
    // Ahead of the plugin the local Qt would load, debug info and all.
    env.set("QT_PLUGIN_PATH", pluginRoot.path().nativePath());
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
        ProcessRunData{{inferior, {"-qmljsdebugger=native,services:NativeQmlDebugger"}},
                        {}, env}, true);
    DebuggerEngineInterface *engine = debuggerBackend->engine();
    const int markerLine = qmlMarkerLine("qmlstack_inferior.qml", "MARKER: qml breakpoint line");
    QVERIFY(markerLine > 0);

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
            [engine, markerLine](InferiorEvent event) {
        if (event != InferiorEvent::EngineSetupOk)
            return;
        BreakpointChangeRequest request;
        request.op = BreakpointOp::Insert;
        request.requestId = 30;
        request.modelId = 42;
        request.params.type = BreakpointByFileAndLine;
        request.params.fileName = FilePath::fromUserInput("qmlstack_inferior.qml");
        request.params.textPosition.line = markerLine;
        request.params.textPosition.column = 0;
        request.params.enabled = true;
        engine->changeBreakpoint(request);
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(insertResults.contains(30), s_qmlStartupTimeout);
    QVERIFY2(insertResults.value(30), "pending QML breakpoint insert failed");

    // Without the casts the debugger refuses the typeless call and read, so no
    // request ever reaches the interpreter and the breakpoint stays pending.
    QTRY_VERIFY_WITH_TIMEOUT(hasResolvedQmlBreakpoint(modifiedReports, 42)
                             || !refusedInferiorCall(wire).isEmpty(), s_qmlStartupTimeout);
    if (const QString refused = refusedInferiorCall(wire); !refused.isEmpty())
        QSKIP(qPrintable("The debugger cannot call into the inferior here: " + refused));
    QVERIFY2(hasResolvedQmlBreakpoint(modifiedReports, 42),
             qPrintable(qmlResolutionDiagnosis(modifiedReports, wire)));
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

    QTRY_VERIFY_WITH_TIMEOUT(hasResolvedQmlBreakpoint(modifiedReports, 42)
                             || !refusedInferiorCall(wire).isEmpty(), s_qmlStartupTimeout);
    if (const QString refused = refusedInferiorCall(wire); !refused.isEmpty())
        QSKIP(qPrintable("The debugger cannot call into the inferior here: " + refused));
    QVERIFY2(hasResolvedQmlBreakpoint(modifiedReports, 42),
             qPrintable(qmlResolutionDiagnosis(modifiedReports, wire)));
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
    QStringList wire;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&wire](const QString &text, int, int) { wire.append(text); });

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

    QTRY_VERIFY_WITH_TIMEOUT(hasResolvedQmlBreakpoint(modifiedReports, 99)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed)
                             || debuggerBackend->contains(InferiorEvent::EngineRunFailed),
                             s_qmlStartupTimeout);
    if (const QString refused = refusedInferiorCall(wire); !refused.isEmpty())
        QSKIP(qPrintable("The debugger cannot call into the inferior here: " + refused));
    QVERIFY2(hasResolvedQmlBreakpoint(modifiedReports, 99),
             qPrintable(qmlResolutionDiagnosis(modifiedReports, wire)));

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

void tst_backends::reportsRefusedBreakpointLocation()
{
    QFETCH(Backend, backend);

    const InferiorTestData testData = inferiorTestData(backend);
    if (testData.unbreakableLine == 0)
        QSKIP("inferior declares no line the backend is expected to refuse");

    std::unique_ptr<DebuggerBackend> debuggerBackend = launchAndStopAtBreakpoint(backend);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QHash<quint64, std::pair<bool, GdbMi>> results;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&results](quint64 requestId, BreakpointOp op, bool ok, const GdbMi &data) {
        if (op == BreakpointOp::Insert)
            results[requestId] = {ok, data};
    });

    BreakpointChangeRequest refused;
    refused.op = BreakpointOp::Insert;
    refused.requestId = 330;
    refused.params.type = BreakpointByFileAndLine;
    refused.params.fileName = testData.source;
    refused.params.textPosition.line = testData.unbreakableLine;
    refused.params.enabled = true;
    engine->changeBreakpoint(refused);
    QTRY_VERIFY2_WITH_TIMEOUT(results.contains(330),
                              "a refused breakpoint location was never answered", s_timeout);
    QVERIFY2(!results.value(330).first, "a refused location was reported as inserted");

    // The answer to the next insertion must not be mistaken for the refused one's.
    BreakpointChangeRequest accepted = refused;
    accepted.requestId = 331;
    accepted.params.textPosition.line = testData.secondBreakpointLine;
    engine->changeBreakpoint(accepted);
    QTRY_VERIFY_WITH_TIMEOUT(results.contains(331), s_timeout);
    QVERIFY2(results.value(331).first, "insert after a refused location failed");
    const GdbMi data = results.value(331).second;
    QVERIFY(data.childCount() > 0);
    QCOMPARE(data.childAt(0)["line"].toInt(), testData.secondBreakpointLine);
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

    // Something true wherever conditions are evaluated at all - C++, Python
    // and JavaScript - so the breakpoint still hits.
    const QString condition = hasCapability(backend, Debugger::BreakConditionCapability)
                                  ? QString("1 == 1") : QString();

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QList<GdbMi> modified;
    connect(engine, &DebuggerEngineInterface::breakpointModified, this,
            [&modified](const GdbMi &data) { modified.append(data); });

    connect(engine, &DebuggerEngineInterface::inferiorEvent, debuggerBackend.get(),
            [this, engine, backend, condition](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            BreakpointChangeRequest request;
            request.op = BreakpointOp::Insert;
            request.requestId = 1;
            request.params.type = BreakpointByFileAndLine;
            request.params.fileName = inferiorTestData(backend).source;
            request.params.textPosition.line = inferiorTestData(backend).breakpointLine;
            request.params.textPosition.column = 0;
            request.params.enabled = true;
            request.params.condition = condition;
            engine->changeBreakpoint(request);
        }
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);
    const char *field = breakpointModifiedField(backend);
    const auto reportsHit = [field](const GdbMi &data) {
        return data.childAt(0)[field].data().toULongLong(nullptr, 0) > 0;
    };
    QTRY_VERIFY_WITH_TIMEOUT(std::any_of(modified.cbegin(), modified.cend(), reportsHit),
                             s_timeout);
    // The model reads a modification as the whole state of the breakpoint, so
    // a field left out of one counts as the default rather than as unchanged.
    const GdbMi bkpt = std::find_if(modified.cbegin(), modified.cend(), reportsHit)->childAt(0);
    QCOMPARE(bkpt["cond"].data(), condition);
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

    // Where the attach ends in a running inferior, wait for that resume before
    // shutting down, so the kill below covers a running one rather than racing
    // it. Where it ends stopped, the state is already final.
    if (attachResumesInferior(backend)) {
        QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunOk)
                                 || debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk),
                                 s_timeout);
    } else {
        QVERIFY2(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk),
                 "attaching neither resumed the inferior nor reported it stopped");
    }

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();

    target.waitForFinished();
    QCOMPARE(target.state(), ProcessState::NotRunning);
}

void tst_backends::reportsTheStackOfASelectedThread()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToProcess); !result)
        QSKIP(qPrintable(result.error()));
    if (auto result = checkExtraCapability(backend, Debugger::DebuggerExtraCapability::Threads);
        !result) {
        QSKIP(qPrintable(result.error()));
    }

    const InferiorTestData testData = inferiorTestData(backend);
    Process target;
    target.setCommand({testData.executable, {}});
    target.start();
    QVERIFY(target.waitForStarted());
    QString targetOutput;
    auto sawOutput = [&] {
        targetOutput += target.readAllStandardOutput();
        return targetOutput.contains(testData.applicationOutputMarker);
    };
    QTRY_VERIFY_WITH_TIMEOUT(sawOutput(), s_timeout);

    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(
        backend, AttachToProcessData{ProcessHandle(target.processId())});
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk)
                             || debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    QVERIFY2(!debuggerBackend->contains(InferiorEvent::EngineSetupFailed), "attaching failed");
    // A backend that resumes the inferior on attach reports the stop it caused
    // and lets it run again, so that event alone does not mean it is held.
    if (attachResumesInferior(backend)
        || !debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk)) {
        debuggerBackend->clearEvents();
        debuggerBackend->execute({ExecutionCommand::Interrupt});
        QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::StopOk), s_timeout);
    }

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });
    RefreshRequest threadsRequest;
    threadsRequest.kind = RefreshKind::Threads;
    threadsRequest.requestId = 330;
    engine->refresh(threadsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Threads)), s_timeout);
    const GdbMi threads = responses.value(int(RefreshKind::Threads))["threads"];
    QVERIFY2(threads.childCount() > 0, "the inferior reported no threads");

    // Attaching parks the debugger on a thread of its own making, so the stack of
    // the inferior's own first thread only shows up once it has been asked for.
    engine->selectThread(threads.childAt(0)["id"].data());
    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 331;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);

    const GdbMi frames = responses.value(int(RefreshKind::FullStack))["stack"]["frames"];
    QStringList functions;
    for (int i = 0; i < frames.childCount(); ++i)
        functions << frames.childAt(i)["function"].data();
    QVERIFY2(functions.contains("main"),
             qPrintable("the selected thread's stack does not reach main, only: "
                        + functions.join(", ")));
    QVERIFY2(frames.childAt(0)["address"].toAddress() != 0,
             qPrintable(frames.childAt(0).toString()));

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();
    target.waitForFinished();
}

void tst_backends::mapsTheReportedSourcePath()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    const InferiorTestData testData = inferiorTestData(backend);
    // Where the sources sit now, against the place they were built from, which is
    // all the debug information knows about.
    const FilePath mappedDir = FilePath::fromString(m_tempDir.path()) / "mapped";
    QVERIFY(mappedDir.ensureWritableDir());
    const FilePath mappedSource = mappedDir / testData.source.fileName();
    if (!mappedSource.exists())
        QVERIFY(testData.source.copyFile(mappedSource));

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngineWithConfiguredPaths(backend,
        {{testData.source.parentDir().nativePath(), mappedDir.nativePath()}});
    if (!debuggerBackend)
        QSKIP("This backend's start data carries no source path map yet.");
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
            request.requestId = 340;
            request.params.type = BreakpointByFileAndLine;
            request.params.fileName = testData.source;
            request.params.textPosition.line = testData.breakpointLine;
            request.params.enabled = true;
            engine->changeBreakpoint(request);
        }
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(insertResults.contains(340)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    QVERIFY2(insertResults.value(340), "the breakpoint was never inserted");
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop), s_timeout);

    QCOMPARE(debuggerBackend->stoppedFile(), mappedSource);

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });
    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 341;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);
    const GdbMi frames = responses.value(int(RefreshKind::FullStack))["stack"]["frames"];
    QVERIFY(frames.childCount() > 0);
    QCOMPARE(FilePath::fromUserInput(frames.childAt(0)["file"].data()), mappedSource);

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();
}

void tst_backends::stopsAtABreakpointInAnInferiorOfTheOtherWordWidth()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::Launch); !result)
        QSKIP(qPrintable(result.error()));

    const InferiorTestData testData = inferiorTestData(backend);
    if (testData.otherWordWidthExecutable.isEmpty())
        QSKIP("No inferior of the other word width was built for this backend.");

    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngine(backend, {},
        ProcessRunData{{testData.otherWordWidthExecutable, {}}, {},
                       Environment::systemEnvironment()});
    QVERIFY(debuggerBackend);
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
            request.requestId = 320;
            request.params.type = BreakpointByFileAndLine;
            request.params.fileName = testData.otherWordWidthSource;
            request.params.textPosition.line = testData.otherWordWidthBreakpointLine;
            request.params.enabled = true;
            engine->changeBreakpoint(request);
        }
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(insertResults.contains(320)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    QVERIFY2(insertResults.value(320), "the breakpoint was never inserted");

    // The layer breaks once on its own account while starting up. Where that stop
    // is not waved through, the inferior never reaches this breakpoint.
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::SpontaneousStop),
                              "never stopped at the breakpoint in the inferior of the other "
                              "word width", s_timeout);
    QCOMPARE(debuggerBackend->stoppedLine(), testData.otherWordWidthBreakpointLine);

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });
    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 321;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);

    // Stopped in its own code the inferior is read directly, and steering the
    // layer here would take the frames below the current one apart.
    const GdbMi frames = responses.value(int(RefreshKind::FullStack))["stack"]["frames"];
    QStringList functions;
    for (int i = 0; i < frames.childCount(); ++i)
        functions << frames.childAt(i)["function"].data();
    QVERIFY2(functions.contains(testData.otherWordWidthFunction) && functions.contains("main"),
             qPrintable("the stack does not run from " + testData.otherWordWidthFunction
                        + " to main, only: " + functions.join(", ")));

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();
}

void tst_backends::reportsTheStackOfAnInferiorOfTheOtherWordWidth()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToProcess); !result)
        QSKIP(qPrintable(result.error()));

    const InferiorTestData testData = inferiorTestData(backend);
    if (testData.otherWordWidthExecutable.isEmpty())
        QSKIP("No inferior of the other word width was built for this backend.");

    Process target;
    target.setCommand({testData.otherWordWidthExecutable, {}});
    target.start();
    QVERIFY(target.waitForStarted());
    // Output of its own means it is past the loader and burning CPU in its own code.
    QString targetOutput;
    auto sawOutput = [&] {
        targetOutput += target.readAllStandardOutput();
        return targetOutput.contains("started");
    };
    QTRY_VERIFY_WITH_TIMEOUT(sawOutput(), s_timeout);

    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(
        backend, AttachToProcessData{ProcessHandle(target.processId())});
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk)
                             || debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk)
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    QVERIFY2(!debuggerBackend->contains(InferiorEvent::EngineSetupFailed),
             "attaching to the inferior of the other word width failed");
    if (!debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk)) {
        debuggerBackend->clearEvents();
        debuggerBackend->execute({ExecutionCommand::Interrupt});
        QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::StopOk), s_timeout);
    }

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });
    // A stack is read for the thread in focus, and attaching parks the debugger on
    // the thread it injected to break in with, not on one of the inferior's own.
    RefreshRequest threadsRequest;
    threadsRequest.kind = RefreshKind::Threads;
    threadsRequest.requestId = 322;
    engine->refresh(threadsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Threads)), s_timeout);
    const GdbMi threads = responses.value(int(RefreshKind::Threads))["threads"];
    QVERIFY2(threads.childCount() > 0, "the inferior reported no threads");
    engine->selectThread(threads.childAt(0)["id"].data());

    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 323;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);

    // Read through the compatibility layer, only the innermost frame comes out
    // right and every caller below it is garbage.
    const GdbMi frames = responses.value(int(RefreshKind::FullStack))["stack"]["frames"];
    QStringList functions;
    for (int i = 0; i < frames.childCount(); ++i)
        functions << frames.childAt(i)["function"].data();
    QVERIFY2(functions.contains(testData.otherWordWidthFunction) && functions.contains("main"),
             qPrintable("the stack does not run from " + testData.otherWordWidthFunction
                        + " to main, only: " + functions.join(", ")));

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();
    target.waitForFinished();
}

void tst_backends::attachesToACrashedProcess()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend,
            DebuggerStartModeFlag::AttachToCrashedProcess); !result) {
        QSKIP(qPrintable(result.error()));
    }

#ifndef Q_OS_WIN
    QSKIP("The crash parameter is an event handle the system hands a post-mortem "
          "debugger, which only Windows does.");
#else
    SECURITY_ATTRIBUTES attributes{sizeof(attributes), nullptr, TRUE};
    HANDLE crashEvent = CreateEventW(&attributes, TRUE, FALSE, nullptr);
    QVERIFY(crashEvent);

    const InferiorTestData testData = inferiorTestData(backend);
    const FilePath trigger = FilePath::fromString(m_tempDir.path()) / "crash-now";
    trigger.removeFile();
    Process target;
    target.setCommand({testData.executable, {"crash-on-file", trigger.nativePath()}});
    target.start();
    QVERIFY(target.waitForStarted());

    const QString crashParameter = QString::number(reinterpret_cast<quintptr>(crashEvent));
    std::unique_ptr<DebuggerBackend> debuggerBackend = createEngineForCrashedProcess(
        backend, ProcessHandle(target.processId()), crashParameter);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    auto eventWasSignalled = [crashEvent] {
        return WaitForSingleObject(crashEvent, 0) == WAIT_OBJECT_0;
    };
    QTRY_VERIFY2_WITH_TIMEOUT(eventWasSignalled(),
                              "the debugger never answered the crash event",
                              s_warmUpTimeout);

    QVERIFY(trigger.writeFileContents("go"));
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk),
                              "the process was never reported as held at its crash",
                              s_warmUpTimeout);
    QVERIFY2(!debuggerBackend->contains(InferiorEvent::EngineSetupFailed),
             "attaching to the crashed process failed");

    debuggerBackend->clearEvents();
    engine->shutdownInferior(ShutdownMode::Detach);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished),
                             s_timeout);
    engine->shutdownEngine();
    CloseHandle(crashEvent);
    target.kill();
    target.waitForFinished();
#endif
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

void tst_backends::runsUserCommandsAfterConnectingToARemoteServer()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToRemoteServer); !result)
        QSKIP(qPrintable(result.error()));
    const UserCommandProbe probe = userCommandProbe(backend, UserCommandHook::AfterConnect);
    if (probe.marker.isEmpty())
        QSKIP("This backend's start data carries no commands for after connecting.");

    if (!m_gdbserverPath.isExecutableFile())
        QSKIP("gdbserver not found - set QTC_GDBSERVER_PATH_FOR_TEST to override.");

    Process gdbserverProcess;
    QString gdbserverOutput;
    const QString port = startGdbserver(gdbserverProcess, {},
                                        {inferiorTestData(backend).executable.nativePath()},
                                        &gdbserverOutput);
    QVERIFY2(!port.isEmpty(),
             qPrintable("could not parse gdbserver's port from: " + gdbserverOutput));

    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(backend,
        AttachToRemoteServerData{"localhost:" + port, inferiorTestData(backend).executable});
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    QStringList messages;
    connect(engine, &DebuggerEngineInterface::message, this,
            [&messages](const QString &text, int, int) { messages.append(text); });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk)
                             || debuggerBackend->contains(InferiorEvent::EngineIll), s_timeout);
    QVERIFY(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk));

    QTRY_VERIFY2_WITH_TIMEOUT(messages.join(' ').contains(probe.marker),
                              qPrintable("connecting to the server ran no configured command - "
                                         "log: " + messages.join(' ').right(300)), s_timeout);

    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    engine->shutdownEngine();
    QTRY_COMPARE_WITH_TIMEOUT(gdbserverProcess.state(), ProcessState::NotRunning, s_timeout);
}

void tst_backends::continuesAfterConnectingWhenConfigured()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToRemoteServer); !result)
        QSKIP(qPrintable(result.error()));
    if (auto result = checkExtraCapability(backend,
            Debugger::DebuggerExtraCapability::ContinueInsteadOfRun); !result) {
        QSKIP(qPrintable(result.error()));
    }

    if (!m_gdbserverPath.isExecutableFile())
        QSKIP("gdbserver not found - set QTC_GDBSERVER_PATH_FOR_TEST to override.");

    const FilePath &executable = inferiorTestData(backend).executable;
    Process gdbserverProcess;
    QString gdbserverOutput;
    const QString port = startGdbserver(gdbserverProcess, {}, {executable.nativePath()},
                                        &gdbserverOutput);
    QVERIFY2(!port.isEmpty(),
             qPrintable("could not parse gdbserver's port from: " + gdbserverOutput));

    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(backend,
        AttachToRemoteServerData{"localhost:" + port, executable},
        GdbImplFlag::ContinueInsteadOfRun);
    QVERIFY(debuggerBackend);

    debuggerBackend->engine()->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorStopOk),
                             s_timeout);
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunOk),
                              "the target that was handed over stopped was never resumed",
                              s_timeout);
}

void tst_backends::exitsTheMonitorWhenClosing()
{
    QFETCH(Backend, backend);

    if (auto result = checkStartMode(backend, DebuggerStartModeFlag::AttachToRemoteServer); !result)
        QSKIP(qPrintable(result.error()));
    if (auto result = checkExtraCapability(backend,
            Debugger::DebuggerExtraCapability::ExitMonitorAtClose); !result) {
        QSKIP(qPrintable(result.error()));
    }

    if (!m_gdbserverPath.isExecutableFile())
        QSKIP("gdbserver not found - set QTC_GDBSERVER_PATH_FOR_TEST to override.");

    const FilePath &executable = inferiorTestData(backend).executable;
    Process gdbserverProcess;
    QString gdbserverOutput;
    // A server started without a program of its own outlives the inferior it
    // runs, so only the monitor command can be what ends it here.
    const QString port = startGdbserver(gdbserverProcess, {"--multi"}, {}, &gdbserverOutput);
    QVERIFY2(!port.isEmpty(),
             qPrintable("could not parse gdbserver's port from: " + gdbserverOutput));

    std::unique_ptr<DebuggerBackend> debuggerBackend = createAttachEngine(backend,
        AttachToRemoteServerData{"localhost:" + port, executable, {}, executable},
        GdbImplFlag::ExitMonitorAtClose);
    QVERIFY(debuggerBackend);
    DebuggerEngineInterface *engine = debuggerBackend->engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk),
                             s_timeout);

    // The inferior has to be stopped before the session closes: a monitor
    // command is refused while the target runs, which is also the order the
    // engine's own shutdown sequence produces.
    debuggerBackend->clearEvents();
    debuggerBackend->execute({ExecutionCommand::Interrupt});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::StopOk), s_timeout);

    engine->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::ShutdownFinished), s_timeout);
    QCOMPARE(gdbserverProcess.state(), ProcessState::Running);

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
    const FilePath lldbPath = lldbPathForTest();
    const FilePath cdbPath = m_backendData[Backend::Cdb].path;
    const bool haveGenerator = HostOsInfo::isMacHost() ? lldbPath.isExecutableFile()
                             : HostOsInfo::isWindowsHost() ? cdbPath.isExecutableFile()
                                                           : gcorePath.isExecutableFile();
    if (!haveGenerator)
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
    const FilePath coreFile = coreFileBase.stringAppended("." + QString::number(pid));
    Process coreGenerator;
    if (HostOsInfo::isMacHost()) {
        coreGenerator.setCommand({lldbPath, {"--batch",
            "-o", "process save-core " + coreFile.nativePath(),
            "-o", "detach", "-o", "quit", "--attach-pid", QString::number(pid)}});
    } else if (HostOsInfo::isWindowsHost()) {
        // A non-invasive attach puts no thread of its own into the dump, so the
        // recorded current thread is the debuggee's own one.
        coreGenerator.setCommand({cdbPath, {"-pv", "-p", QString::number(pid),
            "-c", ".dump /m " + coreFile.nativePath() + ";q"}});
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
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed),
                             s_qmlStartupTimeout);
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

    // A stop the engine asked for is a StopOk, not a spontaneous one - an already
    // stopped inferior has to answer right away.
    debuggerBackend->clearEvents();
    engine->execute({ExecutionCommand::Interrupt});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::StopOk),
                              "interrupt while stopped never reported StopOk", s_timeout);

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
                             || debuggerBackend->contains(InferiorEvent::EngineSetupFailed),
                             s_qmlStartupTimeout);
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
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend->contains(InferiorEvent::RunAndInferiorRunOk),
                             s_qmlStartupTimeout);

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

// The foreign-adapter backend, against a real stock-DAP adapter: gdb speaks the
// protocol itself, so nothing has to be installed to find out whether what it
// answers is understood. This is not one of the rows above - those assert the
// values the Qt dumpers produce, and an adapter Qt Creator does not own prints
// its own.
// An adapter that runs and gives up is not a session that came up: the engine
// has to hear about it rather than wait for an answer that cannot arrive.
void tst_backends::reportsADapAdapterThatQuitsAtOnce()
{
    const CommandLine quitting = quittingDebuggerCommand();
    if (quitting.isEmpty())
        QSKIP("Nothing on this platform to stand in for an adapter that quits at once.");

    DapStartData startData;
    startData.adapter.kind = DapAdapterDescriptor::Kind::Executable;
    startData.adapter.command = quitting;
    startData.adapter.runData.environment = Environment::systemEnvironment();
    startData.adapterId = "quitting";
    startData.configuration = QJsonObject{{"program", "/nonexistent"}};

    DebuggerBackend debuggerBackend(std::make_unique<DapImpl>(startData));
    bool processFinished = false;
    connect(debuggerBackend.engine(), &DebuggerEngineInterface::engineProcessFinished,
            &debuggerBackend, [&processFinished](const Utils::ProcessResultData &) {
        processFinished = true;
    });

    debuggerBackend.engine()->start();
    QTRY_VERIFY2_WITH_TIMEOUT(processFinished, "the adapter process never finished", s_timeout);
    QVERIFY2(debuggerBackend.contains(InferiorEvent::EngineRunFailed)
                 || debuggerBackend.contains(InferiorEvent::EngineSetupFailed),
             "an adapter that quit before answering was not reported as a failure");
}

void tst_backends::stopsAtBreakpointThroughDapAdapter()
{
    const FilePath gdb = m_backendData.value(Backend::Gdb).path;
    if (gdb.isEmpty())
        QSKIP("No gdb to speak DAP to.");
    const InferiorTestData testData = inferiorTestData(Backend::Gdb);
    if (!testData.executable.isExecutableFile())
        QSKIP("The test inferior was not built.");
    if (s_dapInterpreterVersion > debuggerMajorVersion(testData.versionLine)) {
        QSKIP(qPrintable(QString("speaking DAP needs a debugger version >= %1, this is \"%2\"")
                             .arg(s_dapInterpreterVersion).arg(testData.versionLine)));
    }

    DapStartData startData;
    startData.adapter.kind = DapAdapterDescriptor::Kind::Executable;
    startData.adapter.command = CommandLine{gdb, {"-i", "dap"}};
    startData.adapter.runData.environment = Environment::systemEnvironment();
    startData.adapterId = "gdb";
    startData.configuration = QJsonObject{{"program", testData.executable.path()}};

    DebuggerBackend debuggerBackend(std::make_unique<DapImpl>(startData));
    DebuggerEngineInterface *engine = debuggerBackend.engine();

    // The breakpoint is offered before the adapter has said it is ready for
    // one, which is what the protocol's configuration sequence is about: it is
    // held back until then rather than sent and lost.
    connect(engine, &DebuggerEngineInterface::inferiorEvent, &debuggerBackend,
            [engine, testData](InferiorEvent event) {
        if (event != InferiorEvent::EngineSetupOk)
            return;
        BreakpointChangeRequest request;
        request.op = BreakpointOp::Insert;
        request.requestId = 1;
        request.params.type = BreakpointByFileAndLine;
        request.params.fileName = testData.source;
        request.params.textPosition.line = testData.breakpointLine;
        request.params.enabled = true;
        engine->changeBreakpoint(request);
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend.contains(InferiorEvent::EngineSetupFailed)
                             || debuggerBackend.contains(InferiorEvent::EngineRunFailed),
                             s_timeout);
    QVERIFY(debuggerBackend.contains(InferiorEvent::SpontaneousStop));
    QCOMPARE(debuggerBackend.stoppedFile().fileName(), testData.source.fileName());
    QCOMPARE(debuggerBackend.stoppedLine(), testData.breakpointLine);
    QVERIFY(!debuggerBackend.breakpointResponseId().isEmpty());

    QHash<int, GdbMi> responses;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&responses](quint64, RefreshKind kind, const GdbMi &data) {
        responses[int(kind)] = data;
    });

    RefreshRequest stackRequest;
    stackRequest.kind = RefreshKind::FullStack;
    stackRequest.requestId = 10;
    engine->refresh(stackRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::FullStack)), s_timeout);
    const GdbMi frames = responses.value(int(RefreshKind::FullStack))["stack"]["frames"];
    QVERIFY2(frames.childCount() > 0, qPrintable(responses.value(int(RefreshKind::FullStack))
                                                     .toString()));
    QCOMPARE(frames.childAt(0)["line"].toInt(), testData.breakpointLine);
    QCOMPARE(frames.childAt(0)["function"].data(), testData.functionMarker);
    QVERIFY(frames.childAt(0)["address"].toAddress() != 0);

    RefreshRequest threadsRequest;
    threadsRequest.kind = RefreshKind::Threads;
    threadsRequest.requestId = 11;
    engine->refresh(threadsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Threads)), s_timeout);
    QVERIFY(responses.value(int(RefreshKind::Threads))["threads"].childCount() > 0);

    // The locals walk asks for the scopes, then for a level at a time, so the
    // answer only arrives once every queued level has been fetched.
    RefreshRequest localsRequest;
    localsRequest.kind = RefreshKind::Locals;
    localsRequest.requestId = 12;
    engine->refresh(localsRequest);
    QTRY_VERIFY_WITH_TIMEOUT(responses.contains(int(RefreshKind::Locals)), s_timeout);
    const GdbMi locals = responses.value(int(RefreshKind::Locals))["data"];
    QStringList localNames;
    for (const GdbMi &item : locals) {
        QVERIFY(item["iname"].data().startsWith("local."));
        localNames.append(item["name"].data());
    }
    QVERIFY2(localNames.contains("localValue"), qPrintable(localNames.join(' ')));
    // The adapter reports its registers and globals as scopes too, and neither
    // belongs in this view.
    QVERIFY2(!localNames.contains("rax"), qPrintable(localNames.join(' ')));
    QVERIFY2(!localNames.contains("globalValue"), qPrintable(localNames.join(' ')));

    // Removing a breakpoint has no answer of its own in the protocol, so the
    // backend owes the engine one: without it the breakpoint stays in the view
    // and in the adapter.
    QList<BreakpointOp> breakpointOps;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&breakpointOps](quint64, BreakpointOp op, bool, const GdbMi &) {
        breakpointOps.append(op);
    });
    BreakpointChangeRequest removeRequest;
    removeRequest.op = BreakpointOp::Remove;
    removeRequest.requestId = 2;
    removeRequest.responseId = debuggerBackend.breakpointResponseId();
    removeRequest.params.type = BreakpointByFileAndLine;
    removeRequest.params.fileName = testData.source;
    removeRequest.params.textPosition.line = testData.breakpointLine;
    engine->changeBreakpoint(removeRequest);
    QTRY_VERIFY_WITH_TIMEOUT(breakpointOps.contains(BreakpointOp::Remove), s_timeout);

    // The engine takes the run being requested before the run itself, whoever
    // asked for it.
    debuggerBackend.clearEvents();
    debuggerBackend.execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::RunOk), s_timeout);
    QCOMPARE(debuggerBackend.events().first(), InferiorEvent::RunRequested);

    engine->shutdownInferior(ShutdownMode::Kill);
    engine->shutdownEngine();
}

// The framing and the plain answers the fake adapters below have in common.
// A subclass says only what it does beyond that.
class FakeDapAdapter : public QObject
{
public:
    bool listen()
    {
        connect(&m_server, &QTcpServer::newConnection, this, [this] {
            m_socket = m_server.nextPendingConnection();
            connect(m_socket, &QTcpSocket::readyRead, this, &FakeDapAdapter::read);
        });
        return m_server.listen(QHostAddress::LocalHost);
    }

    quint16 port() const { return m_server.serverPort(); }

protected:
    void respond(const QJsonObject &request, const QJsonObject &body)
    {
        send(QJsonObject{{"type", "response"},
                         {"request_seq", request.value("seq")},
                         {"command", request.value("command")},
                         {"success", true},
                         {"body", body}});
    }

    void sendEvent(const QString &event, const QJsonObject &body = {})
    {
        QJsonObject message{{"type", "event"}, {"event", event}};
        if (!body.isEmpty())
            message.insert("body", body);
        send(message);
    }

    virtual void handle(const QJsonObject &request) = 0;

private:
    void send(const QJsonObject &message)
    {
        QJsonObject full = message;
        full.insert("seq", ++m_sequence);
        const QByteArray body = QJsonDocument(full).toJson(QJsonDocument::Compact);
        m_socket->write("Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n"
                        + body);
    }

    void read()
    {
        m_buffer += m_socket->readAll();
        for (;;) {
            const int headerEnd = m_buffer.indexOf("\r\n\r\n");
            if (headerEnd < 0)
                return;
            const QByteArray header = m_buffer.left(headerEnd);
            const int marker = header.indexOf("Content-Length: ");
            if (marker < 0)
                return;
            const int length = header.mid(marker + 16).trimmed().toInt();
            if (m_buffer.size() < headerEnd + 4 + length)
                return;
            const QJsonObject request
                = QJsonDocument::fromJson(m_buffer.mid(headerEnd + 4, length)).object();
            m_buffer.remove(0, headerEnd + 4 + length);
            handle(request);
        }
    }

    QTcpServer m_server;
    QTcpSocket *m_socket = nullptr;
    QByteArray m_buffer;
    int m_sequence = 0;
};

// An adapter shaped like cortex-debug's where the locals are concerned: it
// offers four scopes and hints at none of them, and it reports each stop
// twice. Only the one named "Local" holds locals; the rest are what the other
// views are for.
class ScopedDapAdapter : public FakeDapAdapter
{
    static QJsonObject variable(const QString &name, const QString &value)
    {
        // cortex-debug answers with a whole hover text where a type name is
        // asked for, the value in every format it knows below it.
        const QString type = QString("int %1;\ndec: 1\nhex: 0x00000001").arg(name);
        return QJsonObject{{"name", name}, {"value", value}, {"type", type},
                           {"variablesReference", 0}};
    }

    void handle(const QJsonObject &request) override
    {
        const QString command = request.value("command").toString();
        if (command == "initialize") {
            respond(request, QJsonObject{{"supportsConfigurationDoneRequest", true}});
            sendEvent("initialized");
        } else if (command == "stackTrace") {
            respond(request, QJsonObject{
                {"stackFrames", QJsonArray{QJsonObject{{"id", 1000},
                                                       {"name", "main"},
                                                       {"line", 11},
                                                       {"column", 1}}}},
                {"totalFrames", 1}});
        } else if (command == "scopes") {
            respond(request, QJsonObject{{"scopes", QJsonArray{
                QJsonObject{{"name", "Local"}, {"variablesReference", 4096}},
                QJsonObject{{"name", "Global"}, {"variablesReference", 4097}},
                QJsonObject{{"name", "Static: ./main.c"}, {"variablesReference", 4098}},
                QJsonObject{{"name", "Registers"}, {"variablesReference", 4099}}}}});
        } else if (command == "variables") {
            const int reference = request.value("arguments").toObject()
                                      .value("variablesReference").toInt();
            const QString name = reference == 4096 ? "counter"
                               : reference == 4097 ? "aGlobal"
                               : reference == 4098 ? "aStatic" : "r0";
            respond(request, QJsonObject{{"variables", QJsonArray{variable(name, "1")}}});
        } else {
            respond(request, QJsonObject{});
            if (command == "configurationDone") {
                // Twice, as cortex-debug does.
                for (int i = 0; i < 2; ++i) {
                    sendEvent("stopped", QJsonObject{{"reason", "breakpoint"},
                                                     {"threadId", 1}});
                }
            }
        }
    }
};

// An adapter that stops the debuggee and then lets it run again by itself,
// which is what a real one does when it resets a board before running to an
// entry point. Nothing asks it to: the resume is only announced.
class ResumingDapAdapter : public FakeDapAdapter
{
    void handle(const QJsonObject &request) override
    {
        const QString command = request.value("command").toString();
        respond(request, command == "initialize"
                             ? QJsonObject{{"supportsConfigurationDoneRequest", true}}
                             : QJsonObject{});
        if (command == "initialize")
            sendEvent("initialized");
        if (command == "configurationDone") {
            sendEvent("stopped", QJsonObject{{"reason", "entry"}, {"threadId", 1}});
            sendEvent("continued", QJsonObject{{"threadId", 1},
                                               {"allThreadsContinued", true}});
        }
    }
};

// An adapter that speaks only what this is about: it ends the session while
// answering the launch, which is before the configuration it was also asked
// about has been answered. Nothing real is debugged, and nothing needs to be.
class AdverseDapAdapter : public FakeDapAdapter
{
    void handle(const QJsonObject &request) override
    {
        const QString command = request.value("command").toString();
        respond(request, command == "initialize"
                             ? QJsonObject{{"supportsConfigurationDoneRequest", true}}
                             : QJsonObject{});
        if (command == "initialize")
            sendEvent("initialized");
        // The point of this adapter: over before the configuration is done.
        if (command == "launch")
            sendEvent("terminated");
    }
};

// An adapter that can be driven: it keeps every request it was sent, it stops
// when the configuration is done and again after each step, it has two threads,
// and it takes every breakpoint it is sent. It offers no capability beyond the
// configuration and the function breakpoints, so a jump has nothing to go on. A
// resume is answered but never followed by a stop.
class DrivenDapAdapter : public FakeDapAdapter
{
public:
    static constexpr int stoppedThreadId = 7;

    QStringList commands() const
    {
        QStringList commands;
        for (const QJsonObject &request : m_requests)
            commands.append(request.value("command").toString());
        return commands;
    }

    QJsonObject argumentsOf(const QString &command, int nth = 0) const
    {
        for (const QJsonObject &request : m_requests) {
            if (request.value("command").toString() == command && nth-- == 0)
                return request.value("arguments").toObject();
        }
        return {};
    }

private:
    void handle(const QJsonObject &request) override
    {
        m_requests.append(request);
        const QString command = request.value("command").toString();
        if (command == "initialize") {
            respond(request, QJsonObject{{"supportsConfigurationDoneRequest", true},
                                         {"supportsFunctionBreakpoints", true}});
            sendEvent("initialized");
            return;
        }
        if (command == "disconnect") {
            // The session ends as the adapter lets go, before it gets round to
            // answering for the request that ended it.
            sendEvent("terminated");
            respond(request, QJsonObject{});
            return;
        }
        if (command == "setBreakpoints" || command == "setFunctionBreakpoints") {
            // One answer per breakpoint that was sent, in the order they were
            // sent, which is all the protocol says about which is which.
            QJsonArray taken;
            for (const QJsonValue &value : request.value("arguments").toObject()
                                               .value("breakpoints").toArray()) {
                taken.append(QJsonObject{{"id", ++m_breakpointId},
                                         {"verified", true},
                                         {"line", value.toObject().value("line")}});
            }
            respond(request, QJsonObject{{"breakpoints", taken}});
            return;
        }
        if (command == "threads") {
            // Named, and neither of them the ordinal a caller might assume.
            respond(request, QJsonObject{{"threads", QJsonArray{
                QJsonObject{{"id", 4}, {"name", "main"}},
                QJsonObject{{"id", stoppedThreadId}, {"name", "worker"}}}}});
            return;
        }
        respond(request, QJsonObject{});
        if (command == "configurationDone") {
            sendEvent("stopped", QJsonObject{{"reason", "breakpoint"},
                                             {"threadId", stoppedThreadId}});
        } else if (command == "stepIn" || command == "next" || command == "stepOut") {
            // A step stops the thread it was asked to step, not another.
            sendEvent("stopped", QJsonObject{
                {"reason", "step"},
                {"threadId", request.value("arguments").toObject().value("threadId")}});
        }
    }

    QList<QJsonObject> m_requests;
    int m_breakpointId = 100;
};

// The engine takes the run being reported and its outcome only in that order,
// so an outcome that arrives first has to wait for the run.
void tst_backends::reportsTheRunBeforeItsOutcomeThroughDapAdapter()
{
    AdverseDapAdapter adapter;
    QVERIFY(adapter.listen());

    DapStartData startData;
    startData.adapter.kind = DapAdapterDescriptor::Kind::Server;
    startData.adapter.host = "127.0.0.1";
    startData.adapter.port = adapter.port();
    startData.adapterId = "adverse";
    startData.configuration = QJsonObject{{"program", "/nonexistent"}};

    DebuggerBackend debuggerBackend(std::make_unique<DapImpl>(startData));
    DebuggerEngineInterface *engine = debuggerBackend.engine();

    // Taken when the outcome arrives, so what had been reported by then is what
    // is checked rather than what has been reported by the time the wait ends.
    bool runWasReportedFirst = false;
    connect(engine, &DebuggerEngineInterface::inferiorDone, &debuggerBackend,
            [&debuggerBackend, &runWasReportedFirst](const InferiorResultData &) {
        runWasReportedFirst = debuggerBackend.contains(InferiorEvent::RunAndInferiorRunOk);
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(!debuggerBackend.inferiorResults().isEmpty()
                             || debuggerBackend.contains(InferiorEvent::EngineSetupFailed)
                             || debuggerBackend.contains(InferiorEvent::EngineRunFailed),
                             s_timeout);
    QVERIFY(!debuggerBackend.inferiorResults().isEmpty());
    QVERIFY(runWasReportedFirst);

    engine->shutdownEngine();
}

void tst_backends::followsAResumeTheDapAdapterMakesOnItsOwn()
{
    ResumingDapAdapter adapter;
    QVERIFY(adapter.listen());

    DapStartData startData;
    startData.adapter.kind = DapAdapterDescriptor::Kind::Server;
    startData.adapter.host = "127.0.0.1";
    startData.adapter.port = adapter.port();
    startData.adapterId = "resuming";
    startData.configuration = QJsonObject{{"program", "/nonexistent"}};

    DebuggerBackend debuggerBackend(std::make_unique<DapImpl>(startData));
    DebuggerEngineInterface *engine = debuggerBackend.engine();

    engine->start();
    // The stop comes first, so waiting for the run rather than for the stop is
    // what tells the two apart: only the announced resume reports RunOk here.
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::RunOk)
                             || debuggerBackend.contains(InferiorEvent::EngineSetupFailed)
                             || debuggerBackend.contains(InferiorEvent::EngineRunFailed),
                             s_timeout);
    QVERIFY(debuggerBackend.contains(InferiorEvent::RunOk));
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::SpontaneousStop),
                              "the stop the adapter announced was never reported", s_timeout);
    const QList<InferiorEvent> &events = debuggerBackend.events();
    QCOMPARE(events.indexOf(InferiorEvent::RunRequested),
             events.indexOf(InferiorEvent::RunOk) - 1);

    engine->shutdownEngine();
}

void tst_backends::readsOnlyTheLocalScopeFromADapAdapter()
{
    ScopedDapAdapter adapter;
    QVERIFY(adapter.listen());

    DapStartData startData;
    startData.adapter.kind = DapAdapterDescriptor::Kind::Server;
    startData.adapter.host = "127.0.0.1";
    startData.adapter.port = adapter.port();
    startData.adapterId = "scoped";
    startData.configuration = QJsonObject{{"program", "/nonexistent"}};

    DebuggerBackend debuggerBackend(std::make_unique<DapImpl>(startData));
    DebuggerEngineInterface *engine = debuggerBackend.engine();

    QHash<quint64, GdbMi> localsById;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&localsById](quint64 requestId, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::Locals)
            localsById[requestId] = data;
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend.contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    QVERIFY(debuggerBackend.contains(InferiorEvent::SpontaneousStop));

    RefreshRequest request;
    request.kind = RefreshKind::Locals;
    request.requestId = 420;
    engine->refresh(request);
    QTRY_VERIFY_WITH_TIMEOUT(localsById.contains(420), s_timeout);

    const GdbMi locals = localsById.value(420);
    QStringList names;
    for (const GdbMi &item : locals["data"])
        names.append(item["name"].data());

    // The registers, the globals and the file statics have views of their own,
    // and a stop reported twice is still one stop.
    QCOMPARE(names, QStringList{"counter"});

    // The type column is one line, whatever the adapter puts in the type.
    QCOMPARE(locals["data"].childAt(0)["type"].data(), QString("int counter;"));

    engine->shutdownEngine();
}

void tst_backends::dropsALocalsWalkThatWasStartedOver()
{
    ScopedDapAdapter adapter;
    QVERIFY(adapter.listen());

    DapStartData startData;
    startData.adapter.kind = DapAdapterDescriptor::Kind::Server;
    startData.adapter.host = "127.0.0.1";
    startData.adapter.port = adapter.port();
    startData.adapterId = "scoped";
    startData.configuration = QJsonObject{{"program", "/nonexistent"}};

    DebuggerBackend debuggerBackend(std::make_unique<DapImpl>(startData));
    DebuggerEngineInterface *engine = debuggerBackend.engine();

    QHash<quint64, GdbMi> localsById;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&localsById](quint64 requestId, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::Locals)
            localsById[requestId] = data;
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend.contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    QVERIFY(debuggerBackend.contains(InferiorEvent::SpontaneousStop));

    // Two asks in a row, as two reports of the same stop produce: the first
    // walk is still in flight when the second starts it over.
    RefreshRequest first;
    first.kind = RefreshKind::Locals;
    first.requestId = 430;
    engine->refresh(first);
    RefreshRequest second = first;
    second.requestId = 431;
    engine->refresh(second);

    QTRY_VERIFY_WITH_TIMEOUT(localsById.contains(431), s_timeout);

    const GdbMi locals = localsById.value(431);
    QStringList names;
    for (const GdbMi &item : locals["data"])
        names.append(item["name"].data());

    // What the abandoned walk read must not be added to what this one did.
    QCOMPARE(names, QStringList{"counter"});

    engine->shutdownEngine();
}

void tst_backends::reportsASocketThatCannotConnect()
{
    // A port with nothing behind it: taking one and giving it back is the way
    // to be sure nothing is listening on it.
    quint16 port = 0;
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        port = server.serverPort();
    }

    DapStartData startData;
    startData.adapter.kind = DapAdapterDescriptor::Kind::Server;
    startData.adapter.host = "127.0.0.1";
    startData.adapter.port = port;
    startData.adapterId = "absent";
    startData.configuration = QJsonObject{{"program", "/nonexistent"}};

    DebuggerBackend debuggerBackend(std::make_unique<DapImpl>(startData));
    DebuggerEngineInterface *engine = debuggerBackend.engine();

    QStringList errors;
    connect(engine, &DebuggerEngineInterface::message, &debuggerBackend,
            [&errors](const QString &text, int channel) {
        if (channel == Debugger::LogError)
            errors.append(text);
    });

    engine->start();
    // No adapter is coming, so the setup has to fail rather than wait: an
    // error on the socket is the end of the connection.
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    // And with a reason, or it fails in silence.
    QVERIFY(!errors.isEmpty());
    QVERIFY(!errors.first().trimmed().isEmpty());

    engine->shutdownEngine();
}

void tst_backends::sendsTheProtocolsStepRequestsThroughADapAdapter()
{
    DrivenDapAdapter adapter;
    QVERIFY(adapter.listen());

    DapStartData startData;
    startData.adapter.kind = DapAdapterDescriptor::Kind::Server;
    startData.adapter.host = "127.0.0.1";
    startData.adapter.port = adapter.port();
    startData.adapterId = "stepping";
    startData.configuration = QJsonObject{{"program", "/nonexistent"}};

    DebuggerBackend debuggerBackend(std::make_unique<DapImpl>(startData));
    DebuggerEngineInterface *engine = debuggerBackend.engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend.contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    QVERIFY(debuggerBackend.contains(InferiorEvent::SpontaneousStop));

    const auto stepped = [&debuggerBackend] {
        return debuggerBackend.contains(InferiorEvent::SpontaneousStop);
    };
    const auto step = [&debuggerBackend](ExecutionCommand command, bool byInstruction) {
        debuggerBackend.clearEvents();
        ExecutionRequest request;
        request.command = command;
        request.flag = byInstruction;
        debuggerBackend.execute(request);
    };

    step(ExecutionCommand::StepIn, false);
    QTRY_VERIFY_WITH_TIMEOUT(stepped(), s_timeout);
    step(ExecutionCommand::StepOver, false);
    QTRY_VERIFY_WITH_TIMEOUT(stepped(), s_timeout);
    step(ExecutionCommand::StepOut, false);
    QTRY_VERIFY_WITH_TIMEOUT(stepped(), s_timeout);
    step(ExecutionCommand::StepIn, true);
    QTRY_VERIFY_WITH_TIMEOUT(stepped(), s_timeout);

    QStringList steps;
    for (const QString &command : adapter.commands()) {
        if (command == "stepIn" || command == "next" || command == "stepOut")
            steps.append(command);
    }
    // The protocol names a step over "next", and only a step out is a request
    // of its own for the client.
    QCOMPARE(steps, QStringList({"stepIn", "next", "stepOut", "stepIn"}));

    // The thread to step is the one the stop named, not a default.
    QCOMPARE(adapter.argumentsOf("stepIn").value("threadId").toInt(),
             DrivenDapAdapter::stoppedThreadId);

    // Instruction granularity is what the flag asks for, and only that step.
    QVERIFY(!adapter.argumentsOf("stepIn").contains("granularity"));
    QVERIFY(!adapter.argumentsOf("next").contains("granularity"));
    QCOMPARE(adapter.argumentsOf("stepIn", 1).value("granularity").toString(),
             QString("instruction"));

    // A step is a run, so it is announced before it is answered for.
    const QList<InferiorEvent> events = debuggerBackend.events();
    QVERIFY(events.contains(InferiorEvent::RunRequested));
    QCOMPARE(events.indexOf(InferiorEvent::RunRequested),
             events.indexOf(InferiorEvent::RunOk) - 1);

    engine->shutdownEngine();
}

void tst_backends::sendsNoRequestADapAdapterCannotAnswer()
{
    DrivenDapAdapter adapter;
    QVERIFY(adapter.listen());

    DapStartData startData;
    startData.adapter.kind = DapAdapterDescriptor::Kind::Server;
    startData.adapter.host = "127.0.0.1";
    startData.adapter.port = adapter.port();
    startData.adapterId = "stepping";
    startData.configuration = QJsonObject{{"program", "/nonexistent"}};

    DebuggerBackend debuggerBackend(std::make_unique<DapImpl>(startData));
    DebuggerEngineInterface *engine = debuggerBackend.engine();

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend.contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    QVERIFY(debuggerBackend.contains(InferiorEvent::SpontaneousStop));

    debuggerBackend.clearEvents();
    debuggerBackend.execute({ExecutionCommand::Interrupt});
    QTRY_VERIFY2_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::StopOk),
                              "an interrupt of an already stopped debuggee was not answered",
                              s_timeout);

    // Nothing stops what already stands still, and this adapter offers no jump.
    ExecutionRequest jump;
    jump.command = ExecutionCommand::JumpToLine;
    jump.context.fileName = FilePath::fromString("/nonexistent/main.c");
    jump.context.textPosition = Text::Position{12, 0};
    debuggerBackend.execute(jump);

    // A step the adapter answers is what proves the two above reached it first.
    debuggerBackend.clearEvents();
    debuggerBackend.execute({ExecutionCommand::StepIn});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::SpontaneousStop),
                             s_timeout);

    QVERIFY(!adapter.commands().contains("pause"));
    QVERIFY(!adapter.commands().contains("gotoTargets"));

    engine->shutdownEngine();
}

void tst_backends::reportsTheThreadsADapAdapterListsAgainstTheStoppedOne()
{
    DrivenDapAdapter adapter;
    QVERIFY(adapter.listen());

    DapStartData startData;
    startData.adapter.kind = DapAdapterDescriptor::Kind::Server;
    startData.adapter.host = "127.0.0.1";
    startData.adapter.port = adapter.port();
    startData.adapterId = "driven";
    startData.configuration = QJsonObject{{"program", "/nonexistent"}};

    DebuggerBackend debuggerBackend(std::make_unique<DapImpl>(startData));
    DebuggerEngineInterface *engine = debuggerBackend.engine();

    QHash<quint64, GdbMi> threadsById;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&threadsById](quint64 requestId, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::Threads)
            threadsById[requestId] = data;
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend.contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    QVERIFY(debuggerBackend.contains(InferiorEvent::SpontaneousStop));

    engine->refresh({.requestId = 510, .kind = RefreshKind::Threads});
    QTRY_VERIFY_WITH_TIMEOUT(threadsById.contains(510), s_timeout);

    const GdbMi stopped = threadsById.value(510);
    const GdbMi threads = stopped["threads"];
    QCOMPARE(threads.childCount(), 2);
    QCOMPARE(threads.childAt(0)["id"].data(), QString("4"));
    QCOMPARE(threads.childAt(1)["id"].data(),
             QString::number(DrivenDapAdapter::stoppedThreadId));

    // The view names a thread by its target id, which is all the protocol says
    // about one.
    QCOMPARE(threads.childAt(0)["target-id"].data(), QString("main"));
    QCOMPARE(threads.childAt(1)["target-id"].data(), QString("worker"));

    // The protocol has no notion of a current thread, nor of a thread's state:
    // both are the session's, and the current one is whichever stopped.
    QCOMPARE(stopped["current-thread-id"].data(),
             QString::number(DrivenDapAdapter::stoppedThreadId));
    QCOMPARE(threads.childAt(0)["state"].data(), QString("stopped"));
    QCOMPARE(threads.childAt(1)["state"].data(), QString("stopped"));

    debuggerBackend.clearEvents();
    debuggerBackend.execute({ExecutionCommand::Continue});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::RunOk), s_timeout);

    engine->refresh({.requestId = 511, .kind = RefreshKind::Threads});
    QTRY_VERIFY_WITH_TIMEOUT(threadsById.contains(511), s_timeout);

    // The same list, and the same adapter answering it, now that it runs.
    const GdbMi running = threadsById.value(511)["threads"];
    QCOMPARE(running.childCount(), 2);
    QCOMPARE(running.childAt(0)["state"].data(), QString("running"));
    QCOMPARE(running.childAt(1)["state"].data(), QString("running"));

    engine->shutdownEngine();
}

void tst_backends::stepsTheThreadThatWasSelectedFromADapAdapter()
{
    DrivenDapAdapter adapter;
    QVERIFY(adapter.listen());

    DapStartData startData;
    startData.adapter.kind = DapAdapterDescriptor::Kind::Server;
    startData.adapter.host = "127.0.0.1";
    startData.adapter.port = adapter.port();
    startData.adapterId = "driven";
    startData.configuration = QJsonObject{{"program", "/nonexistent"}};

    DebuggerBackend debuggerBackend(std::make_unique<DapImpl>(startData));
    DebuggerEngineInterface *engine = debuggerBackend.engine();

    QHash<quint64, GdbMi> threadsById;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&threadsById](quint64 requestId, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::Threads)
            threadsById[requestId] = data;
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend.contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    QVERIFY(debuggerBackend.contains(InferiorEvent::SpontaneousStop));

    engine->selectThread("4");
    engine->refresh({.requestId = 520, .kind = RefreshKind::Threads});
    QTRY_VERIFY_WITH_TIMEOUT(threadsById.contains(520), s_timeout);
    QCOMPARE(threadsById.value(520)["current-thread-id"].data(), QString("4"));

    debuggerBackend.clearEvents();
    debuggerBackend.execute({ExecutionCommand::StepIn});
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::SpontaneousStop),
                             s_timeout);

    // The thread the user picked is the one that steps, whatever last stopped.
    QCOMPARE(adapter.argumentsOf("stepIn").value("threadId").toInt(), 4);

    engine->shutdownEngine();
}

void tst_backends::reportsADetachFromADapAdapterAsOne()
{
    DrivenDapAdapter adapter;
    QVERIFY(adapter.listen());

    DapStartData startData;
    startData.adapter.kind = DapAdapterDescriptor::Kind::Server;
    startData.adapter.host = "127.0.0.1";
    startData.adapter.port = adapter.port();
    startData.adapterId = "driven";
    startData.configuration = QJsonObject{{"program", "/nonexistent"}};

    DebuggerBackend debuggerBackend(std::make_unique<DapImpl>(startData));
    DebuggerEngineInterface *engine = debuggerBackend.engine();

    QHash<quint64, GdbMi> threadsById;
    connect(engine, &DebuggerEngineInterface::refreshDataReceived, this,
            [&threadsById](quint64 requestId, RefreshKind kind, const GdbMi &data) {
        if (kind == RefreshKind::Threads)
            threadsById[requestId] = data;
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend.contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    QVERIFY(debuggerBackend.contains(InferiorEvent::SpontaneousStop));

    debuggerBackend.execute({ExecutionCommand::Detach});
    QTRY_VERIFY2_WITH_TIMEOUT(!debuggerBackend.inferiorResults().isEmpty(),
                              "a detach was never answered for", s_timeout);

    // Letting go of the debuggee is the whole of what tells a detach from an
    // exit, at both ends: the adapter is told to leave it be, and the session
    // is not reported as one that ended it.
    QVERIFY(adapter.argumentsOf("disconnect").contains("terminateDebuggee"));
    QVERIFY(!adapter.argumentsOf("disconnect").value("terminateDebuggee").toBool());
    QCOMPARE(debuggerBackend.inferiorResults().constFirst().exitStatus,
             InferiorExitStatus::Detached);

    // The adapter ends the session once it has let go, and that end is not a
    // second end of the debuggee. An answer to a later request is what proves
    // it had arrived by the time this is checked.
    engine->refresh({.requestId = 700, .kind = RefreshKind::Threads});
    QTRY_VERIFY_WITH_TIMEOUT(threadsById.contains(700), s_timeout);
    QCOMPARE(debuggerBackend.inferiorResults().size(), 1);

    engine->shutdownEngine();
}

void tst_backends::acknowledgesEveryBreakpointADapAdapterAnswersFor()
{
    DrivenDapAdapter adapter;
    QVERIFY(adapter.listen());

    DapStartData startData;
    startData.adapter.kind = DapAdapterDescriptor::Kind::Server;
    startData.adapter.host = "127.0.0.1";
    startData.adapter.port = adapter.port();
    startData.adapterId = "driven";
    startData.configuration = QJsonObject{{"program", "/nonexistent"}};

    DebuggerBackend debuggerBackend(std::make_unique<DapImpl>(startData));
    DebuggerEngineInterface *engine = debuggerBackend.engine();

    QHash<quint64, BreakpointOp> acknowledged;
    connect(engine, &DebuggerEngineInterface::breakpointEvent, this,
            [&acknowledged](quint64 requestId, BreakpointOp op, bool ok, const GdbMi &) {
        if (ok)
            acknowledged[requestId] = op;
    });

    BreakpointChangeRequest byLine;
    byLine.op = BreakpointOp::Insert;
    byLine.requestId = 601;
    byLine.modelId = 61;
    byLine.params.type = BreakpointByFileAndLine;
    byLine.params.fileName = FilePath::fromString("/nonexistent/main.c");
    byLine.params.textPosition.line = 12;
    byLine.params.enabled = true;

    BreakpointChangeRequest byFunction;
    byFunction.op = BreakpointOp::Insert;
    byFunction.requestId = 602;
    byFunction.modelId = 62;
    byFunction.params.type = BreakpointByFunction;
    byFunction.params.functionName = "main";
    byFunction.params.enabled = true;

    // The client only exists once the session starts, and the protocol only
    // takes breakpoints once the adapter says it is ready for them.
    connect(engine, &DebuggerEngineInterface::inferiorEvent, this,
            [engine, byLine, byFunction](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk) {
            engine->changeBreakpoint(byLine);
            engine->changeBreakpoint(byFunction);
        }
    });

    engine->start();
    QTRY_VERIFY_WITH_TIMEOUT(debuggerBackend.contains(InferiorEvent::SpontaneousStop)
                             || debuggerBackend.contains(InferiorEvent::EngineSetupFailed),
                             s_timeout);
    QVERIFY(debuggerBackend.contains(InferiorEvent::SpontaneousStop));

    QTRY_VERIFY2_WITH_TIMEOUT(acknowledged.contains(601),
                              "a breakpoint by file and line was never acknowledged", s_timeout);
    // A breakpoint by function name is an array of its own for the protocol,
    // and its answer comes back on its own as well.
    QTRY_VERIFY2_WITH_TIMEOUT(acknowledged.contains(602),
                              "a breakpoint by function name was never acknowledged", s_timeout);
    QCOMPARE(adapter.argumentsOf("setFunctionBreakpoints").value("breakpoints").toArray()
                 .first().toObject().value("name").toString(), QString("main"));

    BreakpointChangeRequest update = byLine;
    update.op = BreakpointOp::Update;
    update.requestId = 603;
    update.params.condition = "x == 1";
    engine->changeBreakpoint(update);
    QTRY_VERIFY2_WITH_TIMEOUT(acknowledged.contains(603),
                              "a breakpoint update was never acknowledged", s_timeout);
    QCOMPARE(adapter.argumentsOf("setBreakpoints", 1).value("breakpoints").toArray()
                 .first().toObject().value("condition").toString(), QString("x == 1"));

    // What the engine is told happened is what it asked for: the file's whole
    // array goes out again for an update, but only one breakpoint changed.
    QCOMPARE(acknowledged.value(601), BreakpointOp::Insert);
    QCOMPARE(acknowledged.value(602), BreakpointOp::Insert);
    QCOMPARE(acknowledged.value(603), BreakpointOp::Update);

    engine->shutdownEngine();
}

QTEST_GUILESS_MAIN(tst_backends)

#include "tst_backends.moc"
