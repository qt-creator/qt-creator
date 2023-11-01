// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindengine.h"

#include "callgrind/callgrindparser.h"
#include "valgrindtr.h"

#include <debugger/analyzer/analyzermanager.h>

#include <utils/filepath.h>
#include <utils/filestreamermanager.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/temporaryfile.h>

#define CALLGRIND_CONTROL_DEBUG 0

using namespace ProjectExplorer;
using namespace Valgrind::Callgrind;
using namespace Utils;

namespace Valgrind::Internal {

const char CALLGRIND_CONTROL_BINARY[] = "callgrind_control";

void setupCallgrindRunner(CallgrindToolRunner *);

CallgrindToolRunner::CallgrindToolRunner(RunControl *runControl)
    : ValgrindToolRunner(runControl)
{
    setId("CallgrindToolRunner");

    connect(&m_runner, &ValgrindProcess::valgrindStarted, this, [this](qint64 pid) {
        m_pid = pid;
    });
    connect(&m_runner, &ValgrindProcess::done, this, [this] {
        triggerParse();
        emit parserDataReady(this);
    });
    connect(&m_parser, &Callgrind::Parser::parserDataReady, this, [this] {
        emit parserDataReady(this);
    });

    m_valgrindRunnable = runControl->runnable();

    static int fileCount = 100;
    m_valgrindOutputFile = runControl->workingDirectory() / QString("callgrind.out.f%1").arg(++fileCount);

    setupCallgrindRunner(this);
}

CallgrindToolRunner::~CallgrindToolRunner()
{
    cleanupTempFile();
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

    cmd << "--callgrind-out-file=" + m_valgrindOutputFile.path();

    cmd.addArgs(m_settings.callgrindArguments(), CommandLine::Raw);
}

QString CallgrindToolRunner::progressTitle() const
{
    return Tr::tr("Profiling");
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

Callgrind::ParseData *CallgrindToolRunner::takeParserData()
{
    return m_parser.takeData();
}

void CallgrindToolRunner::showStatusMessage(const QString &message)
{
    Debugger::showPermanentStatusMessage(message);
}

static QString toOptionString(CallgrindToolRunner::Option option)
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
        case CallgrindToolRunner::Dump:
            return QLatin1String("--dump");
        case CallgrindToolRunner::ResetEventCounters:
            return QLatin1String("--zero");
        case CallgrindToolRunner::Pause:
            return QLatin1String("--instr=off");
        case CallgrindToolRunner::UnPause:
            return QLatin1String("--instr=on");
        default:
            return QString(); // never reached
    }
}

void CallgrindToolRunner::run(Option option)
{
    if (m_controllerProcess) {
        showStatusMessage(Tr::tr("Previous command has not yet finished."));
        return;
    }

    // save back current running operation
    m_lastOption = option;

    m_controllerProcess.reset(new Process);

    switch (option) {
        case CallgrindToolRunner::Dump:
            showStatusMessage(Tr::tr("Dumping profile data..."));
            break;
        case CallgrindToolRunner::ResetEventCounters:
            showStatusMessage(Tr::tr("Resetting event counters..."));
            break;
        case CallgrindToolRunner::Pause:
            showStatusMessage(Tr::tr("Pausing instrumentation..."));
            break;
        case CallgrindToolRunner::UnPause:
            showStatusMessage(Tr::tr("Unpausing instrumentation..."));
            break;
        default:
            break;
    }

#if CALLGRIND_CONTROL_DEBUG
    m_controllerProcess->setProcessChannelMode(QProcess::ForwardedChannels);
#endif
    connect(m_controllerProcess.get(), &Process::done,
            this, &CallgrindToolRunner::controllerProcessDone);

    const FilePath control =
            m_valgrindRunnable.command.executable().withNewPath(CALLGRIND_CONTROL_BINARY);
    m_controllerProcess->setCommand({control, {toOptionString(option), QString::number(m_pid)}});
    m_controllerProcess->setWorkingDirectory(m_valgrindRunnable.workingDirectory);
    m_controllerProcess->setEnvironment(m_valgrindRunnable.environment);
    m_controllerProcess->start();
}

void CallgrindToolRunner::controllerProcessDone()
{
    const QString error = m_controllerProcess->errorString();
    const ProcessResult result = m_controllerProcess->result();

    m_controllerProcess.release()->deleteLater();

    if (result != ProcessResult::FinishedWithSuccess) {
        showStatusMessage(Tr::tr("An error occurred while trying to run %1: %2").arg(CALLGRIND_CONTROL_BINARY).arg(error));
        qWarning() << "Controller exited abnormally:" << error;
        return;
    }

    // this call went fine, we might run another task after this
    switch (m_lastOption) {
        case ResetEventCounters:
            // lets dump the new reset profiling info
            run(Dump);
            return;
        case Pause:
            m_paused = true;
            break;
        case Dump:
            showStatusMessage(Tr::tr("Callgrind dumped profiling info"));
            triggerParse();
            break;
        case UnPause:
            m_paused = false;
            showStatusMessage(Tr::tr("Callgrind unpaused."));
            break;
        default:
            break;
    }

    m_lastOption = Unknown;
}

void CallgrindToolRunner::triggerParse()
{
    cleanupTempFile();
    {
        TemporaryFile dataFile("callgrind.out");
        if (!dataFile.open()) {
            showStatusMessage(Tr::tr("Failed opening temp file..."));
            return;
        }
        m_hostOutputFile = FilePath::fromString(dataFile.fileName());
    }

    const auto afterCopy = [this](expected_str<void> res) {
        if (!res) // failed to run callgrind
            return;
        showStatusMessage(Tr::tr("Parsing Profile Data..."));
        m_parser.parse(m_hostOutputFile);
    };
    // TODO: Store the handle and cancel on CallgrindToolRunner destructor?
    // TODO: Should d'tor of context object cancel the running task?
    FileStreamerManager::copy(m_valgrindOutputFile, m_hostOutputFile, this, afterCopy);
}

void CallgrindToolRunner::cleanupTempFile()
{
    if (!m_hostOutputFile.isEmpty() && m_hostOutputFile.exists())
        m_hostOutputFile.removeFile();

    m_hostOutputFile.clear();
}

} // namespace Valgrind::Internal
