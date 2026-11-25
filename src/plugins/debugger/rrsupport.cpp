// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rrsupport.h"

#include "debuggerengine.h"
#include "debuggerkitaspect.h"
#include "debuggerruncontrol.h"
#include "debuggertr.h"
#include "gdb/gdbsettings.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runcontrol.h>

#include <QtTaskTree/QBarrier>

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>

#include <QAction>
#include <QMessageBox>
#include <QRegularExpression>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Debugger::Internal {

const char RR_RECORD_RUN_MODE[] = "Debugger.RrRecordRunMode";

static FilePath rrExecutable()
{
    const FilePath rr = gdbSettings().rrExecutable();
    const FilePath found = rr.searchInPath();
    return found.isEmpty() ? rr : found;
}

// The environment rr record ran in decides where the trace went.
static Environment &rrRecordEnvironment()
{
    static Environment environment = Environment::systemEnvironment();
    return environment;
}

static Group rrRecordRecipe(RunControl *runControl)
{
    const auto modifier = [](Process &process) {
        rrRecordEnvironment() = process.environment();
        CommandLine cmd{rrExecutable(), {"record"}};
        cmd.addArgs(gdbSettings().rrRecordArguments(), CommandLine::Raw);
        cmd.addCommandLineAsArgs(process.commandLine(), CommandLine::Raw);
        process.setCommand(cmd);
    };
    return runControl->processRecipe(modifier);
}

class RrRecordRunWorkerFactory final : public RunWorkerFactory
{
public:
    RrRecordRunWorkerFactory()
    {
        setId("Debugger.RrRecordRunWorkerFactory");
        setRecipeProducer(rrRecordRecipe);
        addSupportedRunMode(RR_RECORD_RUN_MODE);
        addSupportedDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
        addSupportForLocalRunConfigs();
        setExecutionType(ProjectExplorer::Constants::STDPROCESS_EXECUTION_TYPE_ID);
    }
};

// Where rr record puts its traces, following rr's own lookup.
FilePath rrTraceRoot(const Environment &env)
{
    if (const QString dir = env.value("_RR_TRACE_DIR"); !dir.isEmpty())
        return FilePath::fromUserInput(dir);
    const QString homeDir = env.value("HOME");
    const FilePath home = homeDir.isEmpty() ? FileUtils::homePath()
                                            : FilePath::fromUserInput(homeDir);
    const QString dataHome = env.value("XDG_DATA_HOME");
    const FilePath xdg
        = (dataHome.isEmpty() ? home / ".local/share" : FilePath::fromUserInput(dataHome)) / "rr";
    // rr uses ~/.rr only when the XDG directory does not exist.
    if (const FilePath legacy = home / ".rr"; !xdg.isDir() && legacy.isDir())
        return legacy;
    return xdg;
}

std::optional<RrLaunchLine> parseRrLaunchLine(const QString &output)
{
    // rr quotes every gdb option but prints the executable last, unquoted.
    static const QRegularExpression launch(
        "'target extended-remote ([^']+)'\\s+'?([^'\\n]+?)'?[ \\t\\r]*$",
        QRegularExpression::MultilineOption);
    const QRegularExpressionMatch match = launch.match(output);
    // A line that is not terminated yet may still grow.
    if (!match.hasMatch() || match.capturedEnd() >= output.size())
        return {};
    return RrLaunchLine{match.captured(1), FilePath::fromUserInput(match.captured(2))};
}

static Kit *rrReplayKit()
{
    const auto isUsable = [](const Kit *kit) {
        return RunDeviceTypeKitAspect::deviceTypeId(kit)
                   == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE
               && DebuggerKitAspect::engineType(kit) == GdbEngineType
               && !DebuggerKitAspect::configurationErrors(kit);
    };
    if (Project *project = ProjectManager::startupProject()) {
        if (Kit *kit = project->activeKit(); kit && isUsable(kit))
            return kit;
    }
    return KitManager::kit(isUsable);
}

static void replayRrRecording(const FilePath &traceDir)
{
    Kit *kit = rrReplayKit();
    if (!kit) {
        QMessageBox::critical(ICore::dialogParent(), Tr::tr("Cannot Replay rr Recording"),
                              Tr::tr("There is no kit with a desktop device and GDB."));
        return;
    }

    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->setKit(kit);

    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    rp.setStartMode(AttachToRemoteServer);
    rp.setCloseMode(KillAtClose);
    rp.setUseContinueInsteadOfRun(true);
    // rr's server refuses non-stop, and it does not take a second connection to retry.
    rp.setTargetSupportsNonStop(false);
    rp.setDisplayName(Tr::tr("Replay of %1").arg(traceDir.canonicalPath().fileName()));

    struct RrServer
    {
        QString output;
        QString channel;
        FilePath executable;
    };
    const Storage<RrServer> storage;
    const QStoredBarrier serverReady;

    const auto onReplaySetup = [runControl, traceDir, storage, serverReady](Process &process) {
        process.setCommand({rrExecutable(), {"replay", "-s", "0", traceDir.path()}});
        RrServer *server = storage.activeStorage();
        QBarrier *barrier = serverReady.activeStorage();
        QObject::connect(&process, &Process::readyReadStandardOutput, runControl,
                         [runControl, process = &process] {
            runControl->postMessage(process->readAllStandardOutput(), StdOutFormat, false);
        });
        QObject::connect(&process, &Process::readyReadStandardError, runControl,
                         [runControl, process = &process, server, barrier] {
            const QString output = process->readAllStandardError();
            runControl->postMessage(output, StdErrFormat, false);
            if (!server->channel.isEmpty())
                return;
            // rr prints the gdb command line to connect to it, e.g.
            // gdb '-ex' 'target extended-remote 127.0.0.1:21837' /path/to/exe
            server->output += output;
            const std::optional<RrLaunchLine> line = parseRrLaunchLine(server->output);
            if (!line)
                return;
            server->channel = line->channel;
            server->executable = line->executable;
            barrier->advance();
        });
    };

    const auto onReplayDone = [runControl](const Process &process) {
        runControl->postMessage(process.exitMessage(), ErrorMessageFormat);
    };

    const auto setServer = [storage](DebuggerRunParameters &rp) {
        rp.setRemoteChannel(storage->channel);
        rp.setSymbolFile(storage->executable);
        ProcessRunData inferior = rp.inferior();
        inferior.command.setExecutable(storage->executable);
        rp.setInferior(inferior);
    };

    runControl->setRunRecipe({
        storage,
        serverReady,
        parallel,
        stopOnSuccessOrError,
        ProcessTask(onReplaySetup, onReplayDone, CallDoneFlag::OnError),
        Group {
            barrierAwaiterTask(serverReady).withCancel(runControl->canceler()),
            debuggerRecipe(runControl, rp, setServer)
        }
    });
    runControl->start();
}

void setupRrSupport()
{
    if (!HostOsInfo::isLinuxHost())
        return;

    static RrRecordRunWorkerFactory theRrRecordRunWorkerFactory;

    QAction *recordAction = nullptr;
    ActionBuilder(ICore::instance(), "Debugger.RrRecord")
        .setText(Tr::tr("Record with rr"))
        .addToContainer(ProjectExplorer::Constants::M_DEBUG_STARTDEBUGGING,
                        "Debugger.Group.Special")
        .bindContextAction(&recordAction)
        .addOnTriggered([] { ProjectExplorerPlugin::runStartupProject(RR_RECORD_RUN_MODE); });

    const auto updateRecordAction = [recordAction] {
        const Result<> canRun = ProjectExplorerPlugin::canRunStartupProject(RR_RECORD_RUN_MODE);
        recordAction->setEnabled(bool(canRun));
        recordAction->setToolTip(canRun ? QString() : canRun.error());
    };
    QObject::connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runActionsUpdated,
                     recordAction, updateRecordAction);
    updateRecordAction();

    ActionBuilder(ICore::instance(), "Debugger.RrReplayLast")
        .setText(Tr::tr("Replay Last rr Recording"))
        .addToContainer(ProjectExplorer::Constants::M_DEBUG_STARTDEBUGGING,
                        "Debugger.Group.Special")
        .addOnTriggered([] {
            replayRrRecording(rrTraceRoot(rrRecordEnvironment()) / "latest-trace");
        });

    ActionBuilder(ICore::instance(), "Debugger.RrReplay")
        .setText(Tr::tr("Replay rr Recording..."))
        .addToContainer(ProjectExplorer::Constants::M_DEBUG_STARTDEBUGGING,
                        "Debugger.Group.Special")
        .addOnTriggered([] {
            const FilePath traceDir = FileUtils::getExistingDirectory(
                Tr::tr("Select rr Trace Directory"),
                rrTraceRoot(rrRecordEnvironment()));
            if (!traceDir.isEmpty())
                replayRrRecording(traceDir);
        });
}

} // Debugger::Internal
