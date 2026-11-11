// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lldbengine.h"
#include "lldbimpl.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggersourcepathmappingwidget.h>
#include <debugger/genericdebuggerengine.h>

#include <coreplugin/icore.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/runcontrol.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>
#include <utils/temporaryfile.h>

using namespace Core;
using namespace Utils;

namespace Debugger::Internal {

// Work around https://bugs.launchpad.net/ubuntu/+source/llvm-defaults/+bug/1972855:
// the LLDB 14 installation on Ubuntu 22.04 does not find its own Python module.
static void setupLldbPythonEnvironment(const FilePath &lldbCmd, Environment &environment)
{
    const bool ndkPythonEnvTweaked = DebuggerItem::addAndroidLldbPythonEnv(lldbCmd, environment);

    if (!ndkPythonEnvTweaked && lldbCmd.osType() == OsTypeLinux) {
        Process lldbPythonPathFinder;
        lldbPythonPathFinder.setCommand({lldbCmd, {"-P"}});
        lldbPythonPathFinder.start();
        lldbPythonPathFinder.waitForFinished();
        QString lldbPythonPath = lldbPythonPathFinder.cleanedStdOut();
        if (lldbPythonPath.endsWith('\n'))
            lldbPythonPath.chop(1);
        if (lldbPythonPath == "/usr/lib/local/lib/python3.10/dist-packages")
            environment.appendOrSet("PYTHONPATH", "/usr/lib/llvm-14/lib/python3.10/dist-packages");
    }

    DebuggerItem::fixupAndroidLlldbPythonDylib(lldbCmd);
}

// The Impl handles every start mode it claims; without this the run data
// always looked like a plain launch, so attaching silently launched instead.
InferiorStartData lldbInferiorStartData(const DebuggerRunParameters &rp)
{
    switch (rp.startMode()) {
    case AttachToCore:
        return AttachToCoreData{rp.coreFile(), rp.inferior().command.executable()};
    case AttachToLocalProcess:
    case AttachToCrashedProcess:
    case AttachToIosDevice: // The device uuid tells the Impl which one it is.
        return AttachToProcessData{rp.attachPid()};
    case AttachToRemoteServer:
    case AttachToRemoteProcess:
    case StartRemoteProcess: {
        AttachToRemoteServerData data{rp.remoteChannel(), rp.symbolFile(), rp.attachPid()};
        // The one platform lldb launches the inferior on itself.
        if (rp.lldbPlatform() == "remote-linux") {
            data.remoteExecutable = rp.inferior().command.executable();
            data.remoteArguments = rp.inferior().command.arguments();
            data.remoteEnvironment = rp.inferior().environment;
        }
        return data;
    }
    default:
        break;
    }
    if (rp.useTerminal()) {
        // The stub started the program suspended in the terminal it opened for it,
        // so there is nothing left to launch, only something to attach to.
        return AttachToTerminalStubData{ProcessHandle(rp.applicationPid()),
                                        rp.applicationMainThreadId(),
                                        rp.inferior().command.executable()};
    }
    return rp.inferior();
}

static QList<QPair<QString, QString>> lldbImplSourcePathMap(const DebuggerRunParameters &rp)
{
    QList<QPair<QString, QString>> mappings;
    const SourcePathMap sourcePathMap = mergeStartParametersSourcePathMap(
        rp, mergePlatformQtPath(rp, settings().sourcePathMap()));
    for (auto it = sourcePathMap.cbegin(), end = sourcePathMap.cend(); it != end; ++it)
        mappings.append({it.key(), rp.macroExpander()->expand(it.value())});
    return mappings;
}

static TriState lldbHeapDebugging(const DebuggerRunParameters &rp)
{
    if (rp.inferior().command.executable().osType() != OsTypeWindows)
        return TriState::Default;
    return settings().enableHeapDebugging() ? TriState::Enabled : TriState::Disabled;
}

// The lldb an Android NDK ships brings its own Python, which it only finds
// when the environment says where it is.
static ProcessRunData lldbImplDebuggerRunData(const DebuggerRunParameters &rp)
{
    ProcessRunData debugger = rp.debugger();
    setupLldbPythonEnvironment(debugger.command.executable(), debugger.environment);
    return debugger;
}

static LldbImplStartData lldbImplStartData(const DebuggerRunParameters &rp)
{
    ProcessRunData debuggerRunData = lldbImplDebuggerRunData(rp);
    if (!rp.runAsUser().isEmpty())
        ProjectExplorer::RunControl::provideAskPassEntry(debuggerRunData.environment);
    FilePath dumperScriptsDir = ICore::resourcePath("debugger");
    std::shared_ptr<TemporaryFilePath> dumperScriptsCopy;
    bool pipeDumperScripts = false;
    QString dumperScriptsCopyError;
    if (!debuggerRunData.command.executable().isLocal()) {
        Result<std::unique_ptr<TemporaryFilePath>> copy
            = copyDumperScripts(debuggerRunData.command.executable());
        if (copy) {
            dumperScriptsCopy = std::move(*copy);
            dumperScriptsDir = dumperScriptsCopy->filePath();
        } else {
            dumperScriptsCopyError = copy.error();
            pipeDumperScripts = true;
        }
    }
    return {
        .debuggerRunData = debuggerRunData,
        .inferiorStartData = lldbInferiorStartData(rp),
        .dumperScriptsDir = dumperScriptsDir,
        .pipeDumperScripts = pipeDumperScripts,
        .dumperScriptsCopyError = dumperScriptsCopyError,
        .dumperScriptsCopy = dumperScriptsCopy,
        .loadInitFile = settings().loadGdbInit(),
        .nativeMixedDebugging = rp.isNativeMixedDebugging(),
        .breakOnMain = rp.breakOnMain(),
        .mainFunctionName = QLatin1String(
            rp.toolChainAbi().os() == ProjectExplorer::Abi::WindowsOS && !rp.useTerminal()
                ? "qMain" : "main"),
        .continueAfterAttach = rp.continueAfterAttach(),
        .continueInsteadOfRun = rp.useContinueInsteadOfRun(),
        .breakOnAbort = settings().breakOnAbort(),
        .breakOnWarning = settings().breakOnWarning(),
        .breakOnFatal = settings().breakOnFatal(),
        .enableHeapDebugging = lldbHeapDebugging(rp),
        .skipKnownFrames = settings().skipKnownFrames(),
        .logTimeStamps = settings().logTimeStamps(),
        .deviceSymbolsRoot = rp.deviceSymbolsRoot(),
        .deviceUuid = rp.deviceUuid(),
        .platform = rp.lldbPlatform(),
        .useJitLoader = rp.toolChainAbi().osFlavor() != ProjectExplorer::Abi::AndroidLinuxFlavor,
        .sysroot = rp.sysRoot(),
        .symbolFile = rp.symbolFile(),
        .runAsUser = rp.runAsUser(),
        .startScript = rp.overrideStartScript(),
        .startupCommands = Utils::filtered(
            QString(settings().gdbStartupCommands() + '\n' + rp.additionalStartupCommands())
                .split('\n', Qt::SkipEmptyParts),
            [](const QString &line) { return !line.trimmed().startsWith('#'); }),
        .postAttachCommands = settings().gdbPostAttachCommands().split('\n', Qt::SkipEmptyParts),
        .afterConnectCommands = rp.commandsAfterConnect(),
        .forResetCommands = rp.commandsForReset(),
        .sourcePathMap = lldbImplSourcePathMap(rp),
        .solibSearchPath = rp.solibSearchPath(),
        .debugInfoLocation = rp.debugInfoLocation(),
        // Not settings().useIndexCache(): that one is gdb's, and with a warm
        // index cache Apple's lldb (lldb-2103) crashes before main() is reached.
        .useDebugInfoD = settings().useDebugInfoD(),
        .qtVersion = rp.qtVersion(),
        .qtNamespace = rp.configuredQtNamespace(),
        .extraDumperFile = settings().extraDumperFile(),
        .extraDumperCommands = settings().extraDumperCommands(),
        .watchdogTimeout = std::chrono::seconds(settings().gdbWatchdogTimeout()),
    };
}

DebuggerEngine *createLldbEngine(const DebuggerRunParameters &rp)
{
    return new GenericDebuggerEngine("LLDB (LldbImpl)", new LldbImpl(lldbImplStartData(rp)));
}

} // namespace Debugger::Internal
