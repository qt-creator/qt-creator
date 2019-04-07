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

namespace Constants {

// Common actions
const char INTERRUPT[]                        = "Debugger.Interrupt";
const char CONTINUE[]                         = "Debugger.Continue";
const char STOP[]                             = "Debugger.Stop";
const char ABORT[]                            = "Debugger.Abort";
const char STEP[]                             = "Debugger.StepLine";
const char STEPOUT[]                          = "Debugger.StepOut";
const char NEXT[]                             = "Debugger.NextLine";
const char RUNTOLINE[]                        = "Debugger.RunToLine";
const char RUNTOSELECTEDFUNCTION[]            = "Debugger.RunToSelectedFunction";
const char JUMPTOLINE[]                       = "Debugger.JumpToLine";
const char RETURNFROMFUNCTION[]               = "Debugger.ReturnFromFunction";
const char RESET[]                            = "Debugger.Reset";
const char WATCH[]                            = "Debugger.AddToWatch";
const char DETACH[]                           = "Debugger.Detach";
const char OPERATE_BY_INSTRUCTION[]           = "Debugger.OperateByInstruction";
const char OPEN_MEMORY_EDITOR[]               = "Debugger.Views.OpenMemoryEditor";
const char FRAME_UP[]                         = "Debugger.FrameUp";
const char FRAME_DOWN[]                       = "Debugger.FrameDown";
const char QML_SHOW_APP_ON_TOP[]              = "Debugger.QmlShowAppOnTop";
const char QML_SELECTTOOL[]                   = "Debugger.QmlSelectTool";

const char DEBUGGER_COMMON_SETTINGS_ID[]      = "A.Debugger.General";
const char DEBUGGER_SETTINGS_CATEGORY[]       = "O.Debugger";

// Contexts
const char C_CPPDEBUGGER[]                    = "Gdb Debugger";
const char C_QMLDEBUGGER[]                    = "Qml/JavaScript Debugger";
const char C_DEBUGGER_NOTRUNNING[]            = "Debugger.NotRunning";

const char PRESET_PERSPECTIVE_ID[]            = "Debugger.Perspective.Preset";

const char TASK_CATEGORY_DEBUGGER_DEBUGINFO[] = "Debuginfo";
const char TASK_CATEGORY_DEBUGGER_RUNTIME[]   = "DebugRuntime";

const char TEXT_MARK_CATEGORY_BREAKPOINT[]    = "Debugger.Mark.Breakpoint";
const char TEXT_MARK_CATEGORY_LOCATION[]      = "Debugger.Mark.Location";

const char OPENED_BY_DEBUGGER[]               = "OpenedByDebugger";
const char OPENED_WITH_DISASSEMBLY[]          = "DisassemblerView";
const char DISASSEMBLER_SOURCE_FILE[]         = "DisassemblerSourceFile";

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
