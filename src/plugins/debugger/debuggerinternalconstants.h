/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtGlobal>

namespace Debugger {
namespace Internal {

// DebuggerMainWindow dock widget names
const char DOCKWIDGET_BREAK[]         = "Debugger.Docks.Break";
const char DOCKWIDGET_MODULES[]       = "Debugger.Docks.Modules";
const char DOCKWIDGET_REGISTER[]      = "Debugger.Docks.Register";
const char DOCKWIDGET_OUTPUT[]        = "Debugger.Docks.Output";
const char DOCKWIDGET_SNAPSHOTS[]     = "Debugger.Docks.Snapshots";
const char DOCKWIDGET_STACK[]         = "Debugger.Docks.Stack";
const char DOCKWIDGET_SOURCE_FILES[]  = "Debugger.Docks.SourceFiles";
const char DOCKWIDGET_THREADS[]       = "Debugger.Docks.Threads";
const char DOCKWIDGET_WATCHERS[]      = "Debugger.Docks.LocalsAndWatchers";

} // namespace Internal

namespace Constants {

const char DEBUGGER_COMMON_SETTINGS_ID[]   = "A.Debugger.General";
const char DEBUGGER_SETTINGS_CATEGORY[]    = "O.Debugger";
const char DEBUGGER_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("Debugger", "Debugger");
const char DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON[] = ":/debugger/images/category_debug.png";

namespace Internal {
    enum { debug = 0 };
} // namespace Internal

const char OPENED_BY_DEBUGGER[]         = "OpenedByDebugger";
const char OPENED_WITH_DISASSEMBLY[]    = "DisassemblerView";
const char DISASSEMBLER_SOURCE_FILE[]   = "DisassemblerSourceFile";

// Debug action
const char DEBUG[]                      = "Debugger.Debug";
const int  P_ACTION_DEBUG               = 90; // Priority for the modemanager.

} // namespace Constants

enum ModelRoles
{
    DisplaySourceRole = 32,  // Qt::UserRole

    EngineStateRole,
    EngineActionsEnabledRole,
    RequestActivationRole,
    RequestContextMenuRole,

    // Locals and Watchers
    LocalsINameRole,
    LocalsNameRole,
    LocalsExpandedRole,     // The preferred expanded state to the view
    LocalsTypeFormatRole,   // Used to communicate alternative formats to the view
    LocalsIndividualFormatRole,

    // Snapshots
    SnapshotCapabilityRole
};

} // namespace Debugger
