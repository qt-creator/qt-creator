// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "analyzerconstants.h"

#include "../debuggermainwindow.h"

#include <projectexplorer/runconfiguration.h>

#include <QCoreApplication>
#include <QWidget>

#include <functional>

namespace Debugger {

/**
 * The mode in which this tool should preferably be run
 *
 * Debugging tools which try to show stack traces as close as possible to what the source code
 * looks like will prefer SymbolsMode. Profiling tools which need optimized code for realistic
 * performance, but still want to show analytical output that depends on debug symbols, will prefer
 * ProfileMode.
 */
enum ToolMode {
    DebugMode     = 0x1,
    ProfileMode   = 0x2,
    ReleaseMode   = 0x4,
    SymbolsMode   = DebugMode   | ProfileMode,
    OptimizedMode = ProfileMode | ReleaseMode,
    //AnyMode       = DebugMode   | ProfileMode | ReleaseMode
};

// FIXME: Merge with something sensible.
DEBUGGER_EXPORT bool wantRunTool(ToolMode toolMode, const QString &toolName);
DEBUGGER_EXPORT void showCannotStartDialog(const QString &toolName);

DEBUGGER_EXPORT void enableMainWindow(bool on);

// Convenience functions.
DEBUGGER_EXPORT void showPermanentStatusMessage(const QString &message);

DEBUGGER_EXPORT QAction *createStartAction();
DEBUGGER_EXPORT QAction *createStopAction();

} // namespace Debugger
