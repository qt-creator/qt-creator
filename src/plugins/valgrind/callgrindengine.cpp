// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindengine.h"

#include "callgrindtool.h"

#include "valgrindtr.h"

using namespace ProjectExplorer;
using namespace Utils;

namespace Valgrind::Internal {

CallgrindToolRunner::CallgrindToolRunner(RunControl *runControl)
    : ValgrindToolRunner(runControl)
{
    setId("CallgrindToolRunner");
    setProgressTitle(Tr::tr("Profiling"));

    connect(&m_runner, &ValgrindProcess::valgrindStarted, this, [](qint64 pid) { setupPid(pid); });
    connect(&m_runner, &ValgrindProcess::done, this, [] { startParser(); });

    setupRunControl(runControl);
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

    if (isPaused())
        cmd << "--instr-atstart=no";

    const QString toggleCollectFunction = fetchAndResetToggleCollectFunction();
    if (!toggleCollectFunction.isEmpty())
        cmd << "--toggle-collect=" + toggleCollectFunction;

    cmd << "--callgrind-out-file=" + remoteOutputFile().path();

    cmd.addArgs(m_settings.callgrindArguments(), CommandLine::Raw);
}

void CallgrindToolRunner::start()
{
    const FilePath executable = runControl()->commandLine().executable();
    appendMessage(Tr::tr("Profiling %1").arg(executable.toUserOutput()), NormalMessageFormat);
    return ValgrindToolRunner::start();
}

} // namespace Valgrind::Internal
