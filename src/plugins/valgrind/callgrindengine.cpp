// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindengine.h"

#include "callgrind/callgrindparser.h"
#include "valgrindtr.h"

#include <debugger/analyzer/analyzerutils.h>

#include <utils/filepath.h>
#include <utils/filestreamer.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#define CALLGRIND_CONTROL_DEBUG 0

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;
using namespace Valgrind::Callgrind;

namespace Valgrind::Internal {

const char CALLGRIND_CONTROL_BINARY[] = "callgrind_control";

void setupCallgrindRunner(CallgrindToolRunner *);

CallgrindToolRunner::CallgrindToolRunner(RunControl *runControl)
    : ValgrindToolRunner(runControl)
{
    setId("CallgrindToolRunner");
    setProgressTitle(Tr::tr("Profiling"));

    connect(&m_runner, &ValgrindProcess::valgrindStarted, this, [this](qint64 pid) {
        m_pid = pid;
    });
    connect(&m_runner, &ValgrindProcess::done, this, [this] {
        executeController({ parseRecipe() });
    });

    static int fileCount = 100;
    m_remoteOutputFile = runControl->workingDirectory() / QString("callgrind.out.f%1").arg(++fileCount);

    setupCallgrindRunner(this);
}

CallgrindToolRunner::~CallgrindToolRunner()
{
    m_controllerRunner.cancel();
}

void CallgrindToolRunner::addToolArguments(CommandLine &cmd) const
{
    cmd << "--tool=callgrind";

    if (m_settings.enableCacheSim())
        cmd << "--cache-sim=yes";

    if (m_settings.enableBranchSim())
        cmd << "--branch-sim=yes";

    if (m_settings.collectBusEvents())
        cmd << "--collect-bus=yes";

    if (m_settings.collectSystime())
        cmd << "--collect-systime=yes";

    if (m_markAsPaused)
        cmd << "--instr-atstart=no";

    // add extra arguments
    if (!m_argumentForToggleCollect.isEmpty())
        cmd << m_argumentForToggleCollect;

    cmd << "--callgrind-out-file=" + m_remoteOutputFile.path();

    cmd.addArgs(m_settings.callgrindArguments(), CommandLine::Raw);
}

void CallgrindToolRunner::start()
{
    const FilePath executable = runControl()->commandLine().executable();
    appendMessage(Tr::tr("Profiling %1").arg(executable.toUserOutput()), NormalMessageFormat);
    return ValgrindToolRunner::start();
}

void CallgrindToolRunner::setPaused(bool paused)
{
    if (m_markAsPaused == paused)
        return;

    m_markAsPaused = paused;

    // call controller only if it is attached to a valgrind process
    if (paused)
        pause();
    else
        unpause();
}

void CallgrindToolRunner::setToggleCollectFunction(const QString &toggleCollectFunction)
{
    if (toggleCollectFunction.isEmpty())
        return;

    m_argumentForToggleCollect = "--toggle-collect=" + toggleCollectFunction;
}

enum class Option
{
    Dump,
    ResetEventCounters,
    Pause,
    UnPause
};

static QString statusMessage(Option option)
{
    switch (option) {
    case Option::Dump:
        return Tr::tr("Dumping profile data...");
    case Option::ResetEventCounters:
        return Tr::tr("Resetting event counters...");
    case Option::Pause:
        return Tr::tr("Pausing instrumentation...");
    case Option::UnPause:
        return Tr::tr("Unpausing instrumentation...");
    }
    return {};
}

static QString toOptionString(Option option)
{
    /* callgrind_control help from v3.9.0

    Options:
    -h --help        Show this help text
    --version        Show version
    -s --stat        Show statistics
    -b --back        Show stack/back trace
    -e [<A>,...]     Show event counters for <A>,... (default: all)
    --dump[=<s>]     Request a dump optionally using <s> as description
    -z --zero        Zero all event counters
    -k --kill        Kill
    --instr=<on|off> Switch instrumentation state on/off
    */

    switch (option) {
    case Option::Dump:
        return QLatin1String("--dump");
    case Option::ResetEventCounters:
        return QLatin1String("--zero");
    case Option::Pause:
        return QLatin1String("--instr=off");
    case Option::UnPause:
        return QLatin1String("--instr=on");
    default:
        return QString(); // never reached
    }
}

static ExecutableItem recipeForOption(Option option, RunControl *runControl, qint64 pid)
{
    const auto onSetup = [option, runControl, pid](Process &process) {
        Debugger::showPermanentStatusMessage(statusMessage(option));
        const ProcessRunData runnable = runControl->runnable();
        const FilePath control = runnable.command.executable().withNewPath(CALLGRIND_CONTROL_BINARY);
        process.setCommand({control, {toOptionString(option), QString::number(pid)}});
        process.setWorkingDirectory(runnable.workingDirectory);
        process.setEnvironment(runnable.environment);
#if CALLGRIND_CONTROL_DEBUG
        process.setProcessChannelMode(QProcess::ForwardedChannels);
#endif
    };
    const auto onDone = [option](const Process &process, DoneWith result) {
        if (result != DoneWith::Success) {
            Debugger::showPermanentStatusMessage(Tr::tr("An error occurred while trying to run %1: %2")
                                                     .arg(CALLGRIND_CONTROL_BINARY)
                                                     .arg(process.errorString()));
            return;
        }
        switch (option) {
        case Option::Pause:
            Debugger::showPermanentStatusMessage(Tr::tr("Callgrind paused."));
            break;
        case Option::UnPause:
            Debugger::showPermanentStatusMessage(Tr::tr("Callgrind unpaused."));
            break;
        case Option::Dump:
            Debugger::showPermanentStatusMessage(Tr::tr("Callgrind dumped profiling info."));
            break;
        default:
            break;
        }
    };
    return ProcessTask(onSetup, onDone);
}

ExecutableItem CallgrindToolRunner::parseRecipe()
{
    const Storage<FilePath> storage; // host output path

    const auto onSetup = [this, storage](FileStreamer &streamer) {
        TemporaryFile dataFile("callgrind.out");
        if (!dataFile.open()) {
            Debugger::showPermanentStatusMessage(Tr::tr("Failed opening temp file..."));
            return;
        }
        const FilePath hostOutputFile = FilePath::fromString(dataFile.fileName());
        *storage = hostOutputFile;
        streamer.setSource(m_remoteOutputFile);
        streamer.setDestination(hostOutputFile);
    };
    const auto onDone = [this, storage](DoneWith result) {
        const FilePath hostOutputFile = *storage;
        if (result == DoneWith::Success) {
            Debugger::showPermanentStatusMessage(Tr::tr("Parsing Profile Data..."));
            emit parserDataReady(parseDataFile(hostOutputFile));
        }
        if (!hostOutputFile.isEmpty() && hostOutputFile.exists())
            hostOutputFile.removeFile();
    };

    return Group {
        storage,
        FileStreamerTask(onSetup, onDone)
    };
}

void CallgrindToolRunner::dump()
{
    executeController({
        recipeForOption(Option::Dump, runControl(), m_pid),
        parseRecipe()
    });
}

void CallgrindToolRunner::reset()
{
    executeController({
        recipeForOption(Option::ResetEventCounters, runControl(), m_pid),
        recipeForOption(Option::Dump, runControl(), m_pid)
    });
}

void CallgrindToolRunner::pause()
{
    executeController({ recipeForOption(Option::Pause, runControl(), m_pid) });
}

void CallgrindToolRunner::unpause()
{
    executeController({ recipeForOption(Option::UnPause, runControl(), m_pid) });
}

void CallgrindToolRunner::executeController(const Tasking::Group &recipe)
{
    if (m_controllerRunner.isRunning())
        Debugger::showPermanentStatusMessage(Tr::tr("Previous command has not yet finished."));
    else
        m_controllerRunner.start(recipe);
}

} // namespace Valgrind::Internal
