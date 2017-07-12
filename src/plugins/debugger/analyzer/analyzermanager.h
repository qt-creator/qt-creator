/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "analyzerconstants.h"

#include "../debuggermainwindow.h"

#include <projectexplorer/runconfiguration.h>

#include <QWidget>

#include <QCoreApplication>

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

// Register a tool for a given start mode.
DEBUGGER_EXPORT void registerPerspective(const QByteArray &perspectiveId, const Utils::Perspective *perspective);
DEBUGGER_EXPORT void setPerspectiveEnabled(const QByteArray &perspectiveId, bool enable);
DEBUGGER_EXPORT void registerToolbar(const QByteArray &perspectiveId, const Utils::ToolbarDescription &desc);

DEBUGGER_EXPORT void enableMainWindow(bool on);
DEBUGGER_EXPORT QWidget *mainWindow();

DEBUGGER_EXPORT void selectPerspective(const QByteArray &perspectiveId);
DEBUGGER_EXPORT QByteArray currentPerspective();

// Convenience functions.
DEBUGGER_EXPORT void showStatusMessage(const QString &message, int timeoutMS = 10000);
DEBUGGER_EXPORT void showPermanentStatusMessage(const QString &message);

DEBUGGER_EXPORT QAction *createStartAction();
DEBUGGER_EXPORT QAction *createStopAction();

} // namespace Debugger
