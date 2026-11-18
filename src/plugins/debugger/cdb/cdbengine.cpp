// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cdbengine.h"

#include "cdbimpl.h"
#include "cdboptionspage.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggersourcepathmappingwidget.h>
#include <debugger/genericdebuggerengine.h>

#include <coreplugin/icore.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/processinterface.h>

#include <QCoreApplication>

using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger::Internal {

void addCdbOptionPages(QList<Core::IOptionsPage *> *opts)
{
    if (HostOsInfo::isWindowsHost()) {
        opts->push_back(new CdbOptionsPage);
        opts->push_back(new CdbPathsPage);
    }
}

#define QT_CREATOR_CDB_EXT "qtcreatorcdbext"

QString cdbExtensionLibraryName(bool is64Bit, bool isArm)
{
    return QString("%1/lib/" QT_CREATOR_CDB_EXT "%2%3/" QT_CREATOR_CDB_EXT ".dll")
        .arg(FilePath::fromString(QCoreApplication::applicationDirPath()).parentDir().path())
        .arg(isArm ? "arm" : QString())
        .arg(is64Bit ? QLatin1String("64") : QLatin1String("32"));
}

// Without a project, the parameters name the directory of the debugger instead.
static FilePath justMyCodeProjectDirectory(const DebuggerRunParameters &rp)
{
    return rp.projectSourceFiles().isEmpty() ? FilePath() : rp.projectSourceDirectory();
}

static QString msvcRunTime(const Abi::OSFlavor flavour)
{
    switch (flavour)  {
    case Abi::WindowsMsvc2005Flavor:
        return QLatin1String("MSVCR80");
    case Abi::WindowsMsvc2008Flavor:
        return QLatin1String("MSVCR90");
    case Abi::WindowsMsvc2010Flavor:
        return QLatin1String("MSVCR100");
    case Abi::WindowsMsvc2012Flavor:
        return QLatin1String("MSVCR110");
    case Abi::WindowsMsvc2013Flavor:
        return QLatin1String("MSVCR120");
    case Abi::WindowsMsvc2015Flavor:
        return QLatin1String("MSVCR140");
    default:
        break;
    }
    return QLatin1String("MSVCRT"); // MinGW, others.
}

// The debug runtime that has _CrtDbgReport, empty where there is none to break in.
static QString crtDebugReportModule(const Abi::OSFlavor flavor)
{
    if (flavor >= Abi::WindowsMsvc2015Flavor && flavor <= Abi::WindowsLastMsvcFlavor)
        return QLatin1String("ucrtbased");
    if (flavor >= Abi::WindowsMsvc2005Flavor && flavor <= Abi::WindowsMsvc2013Flavor)
        return msvcRunTime(flavor) + 'D';
    return {};
}

static QString moduleForSourceFile(const FilePath &sourceFile)
{
    const Project *project = ProjectManager::projectForFile(sourceFile);
    if (!project)
        return {};
    return cdbModuleName(project->binariesForSourceFile(sourceFile));
}

static InferiorStartData cdbImplInferiorStartData(const DebuggerRunParameters &rp)
{
    switch (rp.startMode()) {
    case AttachToCore:
        return AttachToCoreData{rp.coreFile(), rp.inferior().command.executable()};
    case AttachToLocalProcess:
        return AttachToProcessData{rp.attachPid()};
    case AttachToCrashedProcess:
        return AttachToProcessData{rp.attachPid(), rp.crashParameter()};
    case AttachToRemoteServer:
        return AttachToRemoteServerData{rp.remoteChannel(), rp.symbolFile(), rp.attachPid(), {}};
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

static CdbImplSearchPaths cdbImplSearchPaths(const DebuggerRunParameters &rp)
{
    CdbImplSearchPaths paths;
    paths.symbolPaths = settings().cdbSymbolPaths();
    for (const QString &key : QStringList{"_NT_ALT_SYMBOL_PATH", "_NT_SYMBOL_PATH"}) {
        const QString path = rp.inferior().environment.expandedValueForKey(key);
        if (!path.isEmpty())
            paths.symbolPaths.append(path);
    }
    paths.sourcePaths = settings().cdbSourcePaths();
    const SourcePathMap sourcePathMap
        = mergeStartParametersSourcePathMap(rp, mergePlatformQtPath(rp, settings().sourcePathMap()));
    for (auto it = sourcePathMap.cbegin(), end = sourcePathMap.cend(); it != end; ++it) {
        paths.sourcePathMap.append(
            {FilePath::fromUserInput(it.key()).path(),
             FilePath::fromUserInput(rp.macroExpander()->expand(it.value())).path()});
    }
    return paths;
}

static QString cdbImplCrtDebugReportModule(const DebuggerRunParameters &rp)
{
    if (!settings().cdbBreakOnCrtDbgReport())
        return {};
    return crtDebugReportModule(rp.toolChainAbi().osFlavor());
}

// The extension has to match cdb's architecture. For a local cdb it ships next to
// Qt Creator, for a cdb on a remote Windows device it lives on the device, in the
// per-architecture subdirectory of the extension path configured for it.
static FilePath cdbImplExtensionLibrary(const DebuggerRunParameters &rp, bool is64Bit, bool isArm)
{
    if (rp.debugger().command.executable().isLocal())
        return FilePath::fromString(cdbExtensionLibraryName(is64Bit, isArm));
    if (rp.cdbExtensionPath().isEmpty())
        return {};
    const QString archSubDir = QLatin1String(QT_CREATOR_CDB_EXT)
                               + QLatin1String(isArm ? "arm" : "")
                               + QLatin1String(is64Bit ? "64" : "32");
    return rp.cdbExtensionPath().pathAppended(archSubDir)
        .pathAppended(QLatin1String(QT_CREATOR_CDB_EXT ".dll"));
}

CdbImplStartData cdbImplStartData(const DebuggerRunParameters &rp)
{
    bool cdbIs64Bit = true;
    bool cdbIsArm = false;
    const Abis abisOfCdb = Abi::abisOfBinary(rp.debugger().command.executable());
    if (abisOfCdb.size() == 1) {
        const Abi abi = abisOfCdb.at(0);
        cdbIs64Bit = abi.wordWidth() == 64;
        cdbIsArm = abi.architecture() == Abi::Architecture::ArmArchitecture;
    }
    const FilePath extension = cdbImplExtensionLibrary(rp, cdbIs64Bit, cdbIsArm);
    const DebuggerSettings &s = settings();
    return {
        .debuggerRunData = rp.debugger(),
        .inferiorStartData = cdbImplInferiorStartData(rp),
        .extensionDir = extension.parentDir(),
        .extensionFileName = extension.fileName(),
        .dumperScriptsDir = Core::ICore::resourcePath("debugger"),
        .searchPaths = cdbImplSearchPaths(rp),
        .breakEvents = s.cdbBreakEvents(),
        .additionalArguments = s.cdbAdditionalArguments(),
        .startupCommands = rp.commandsAfterConnect(),
        .extraDumperFile = s.extraDumperFile(),
        .extraDumperCommands = s.extraDumperCommands(),
        .breakOnMain = rp.breakOnMain(),
        .continueAfterAttach = rp.continueAfterAttach(),
        .enableHeapDebugging = s.enableHeapDebugging(),
        .ignoreFirstChanceAccessViolation = s.ignoreFirstChanceAccessViolation(),
        .useTerminal = rp.useTerminal(),
        .crtDebugReportModule = cdbImplCrtDebugReportModule(rp),
        .reportFirstChanceExceptions = s.firstChanceExceptionTaskEntry(),
        .reportSecondChanceExceptions = s.secondChanceExceptionTaskEntry(),
        .usePythonDumper = s.cdbUsePythonDumper(),
        .inferiorWordWidth = rp.toolChainAbi().wordWidth(),
        .nativeMixed = rp.isNativeMixedDebugging(),
        .qtVersion = rp.qtVersion(),
        .qtNamespace = rp.configuredQtNamespace(),
        .useCtrlCStub = true,
        .watchdogTimeout = std::chrono::seconds(s.gdbWatchdogTimeout()),
        .moduleForSourceFile = &moduleForSourceFile,
        .justMyCode = s.cdbJustMyCode(),
        .projectDirectory = justMyCodeProjectDirectory(rp),
    };
}

// Accessed by debuggerRecipe()
DebuggerEngine *createCdbEngine(const DebuggerRunParameters &rp)
{
    return new GenericDebuggerEngine("CDB (CdbImpl)", new CdbImpl(cdbImplStartData(rp)));
}

} // namespace Debugger::Internal
