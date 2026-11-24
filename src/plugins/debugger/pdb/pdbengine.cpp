// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pdbengine.h"
#include "pdbimpl.h"

#include <debugger/debuggeractions.h>
#include <debugger/debuggersourcepathmappingwidget.h>
#include <debugger/genericdebuggerengine.h>

#include <coreplugin/icore.h>

#include <projectexplorer/runcontrol.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/macroexpander.h>

using namespace Core;
using namespace Utils;

namespace Debugger::Internal {

DebuggerEngine *createPdbEngine(const DebuggerRunParameters &rp)
{
    ProcessRunData scriptRunData = rp.inferior();
    QStringList arguments = scriptRunData.command.splitArguments();
    if (!arguments.isEmpty() && arguments.constFirst() == "-u")
        arguments.removeFirst();
    if (!arguments.isEmpty())
        arguments.removeFirst();
    scriptRunData.command = CommandLine(rp.mainScript(), arguments);

    ProcessRunData debuggerRunData;
    debuggerRunData.command = CommandLine(rp.interpreter());
    debuggerRunData.environment = rp.debugger().environment;
    if (!rp.runAsUser().isEmpty())
        ProjectExplorer::RunControl::provideAskPassEntry(debuggerRunData.environment);

    QList<QPair<QString, QString>> sourcePathMap;
    const SourcePathMap mergedMap
        = mergeStartParametersSourcePathMap(rp, userSourcePathMap(rp));
    for (auto it = mergedMap.cbegin(), end = mergedMap.cend(); it != end; ++it)
        sourcePathMap.append(qMakePair(it.key(), rp.macroExpander()->expand(it.value())));

    return new GenericDebuggerEngine(
        "PDB (PdbImpl)",
        new PdbImpl({.debuggerRunData = debuggerRunData,
                     .inferiorStartData = scriptRunData,
                     .dumperScriptsDir = ICore::resourcePath("debugger"),
                     .loadInitFile = settings().loadGdbInit(),
                     .sourcePathMap = sourcePathMap,
                     .runAsUser = rp.runAsUser(),
                     .startScript = rp.overrideStartScript(),
                     .startupCommands = Utils::filtered(
                         rp.additionalStartupCommands().split('\n', Qt::SkipEmptyParts),
                         [](const QString &line) { return !line.trimmed().startsWith('#'); }),
                     .forResetCommands = rp.commandsForReset(),
                     .breakOnMain = rp.breakOnMain(),
                     .watchdogTimeout = std::chrono::seconds(settings().gdbWatchdogTimeout()),
                     .logTimeStamps = settings().logTimeStamps()}));
}

} // Debugger::Internal
