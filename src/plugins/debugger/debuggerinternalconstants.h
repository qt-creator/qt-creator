// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Debugger {

namespace Constants {

// Common actions
inline constexpr char INTERRUPT[]                        = "Debugger.Interrupt";
inline constexpr char CONTINUE[]                         = "Debugger.Continue";
inline constexpr char STOP[]                             = "Debugger.Stop";
inline constexpr char ABORT[]                            = "Debugger.Abort";
inline constexpr char STEP[]                             = "Debugger.StepLine";
inline constexpr char STEPOUT[]                          = "Debugger.StepOut";
inline constexpr char NEXT[]                             = "Debugger.NextLine";
inline constexpr char RUNTOLINE[]                        = "Debugger.RunToLine";
inline constexpr char RUNTOSELECTEDFUNCTION[]            = "Debugger.RunToSelectedFunction";
inline constexpr char JUMPTOLINE[]                       = "Debugger.JumpToLine";
inline constexpr char RETURNFROMFUNCTION[]               = "Debugger.ReturnFromFunction";
inline constexpr char RESET[]                            = "Debugger.Reset";
inline constexpr char WATCH[]                            = "Debugger.AddToWatch";
inline constexpr char DETACH[]                           = "Debugger.Detach";
inline constexpr char OPERATE_BY_INSTRUCTION[]           = "Debugger.OperateByInstruction";
inline constexpr char OPEN_MEMORY_EDITOR[]               = "Debugger.Views.OpenMemoryEditor";
inline constexpr char FRAME_UP[]                         = "Debugger.FrameUp";
inline constexpr char FRAME_DOWN[]                       = "Debugger.FrameDown";
inline constexpr char QML_SHOW_APP_ON_TOP[]              = "Debugger.QmlShowAppOnTop";
inline constexpr char QML_SELECTTOOL[]                   = "Debugger.QmlSelectTool";
inline constexpr char RELOAD_DEBUGGING_HELPERS[]         = "Debugger.ReloadDebuggingHelpers";

inline constexpr char DEBUGGER_COMMON_SETTINGS_ID[]      = "A.Debugger.General";
inline constexpr char DEBUGGER_SETTINGS_CATEGORY[]       = "O.Debugger";

// Contexts
inline constexpr char C_CPPDEBUGGER[]                    = "Gdb Debugger";
inline constexpr char C_QMLDEBUGGER[]                    = "Qml/JavaScript Debugger";
inline constexpr char C_DEBUGGER_NOTRUNNING[]            = "Debugger.NotRunning";

inline constexpr char PRESET_PERSPECTIVE_ID[]            = "Debugger.Perspective.Preset";
inline constexpr char DAP_PERSPECTIVE_ID[]               = "DAPDebugger";

inline constexpr char TASK_CATEGORY_DEBUGGER_RUNTIME[]   = "DebugRuntime";

inline constexpr char TEXT_MARK_CATEGORY_BREAKPOINT[]    = "Debugger.Mark.Breakpoint";
inline constexpr char TEXT_MARK_CATEGORY_LOCATION[]      = "Debugger.Mark.Location";
inline constexpr char TEXT_MARK_CATEGORY_VALUE[]         = "Debugger.Mark.Value";

inline constexpr char OPENED_BY_DEBUGGER[]               = "OpenedByDebugger";
inline constexpr char OPENED_WITH_DISASSEMBLY[]          = "DisassemblerView";
inline constexpr char DISASSEMBLER_SOURCE_FILE[]         = "DisassemblerSourceFile";

inline constexpr char CRT_DEBUG_REPORT[]                 = "CrtDbgReport";

inline constexpr char NO_DEBUG_HEAP[]                    = "_NO_DEBUG_HEAP";

inline constexpr char DEBUGSERVER_TOOL_ID[]              = "DebugServer";

inline constexpr char RunConfigId[]                      = "RemoteDebugger.RunConfig";
inline constexpr char GdbServerAddressAspectId[]         = "RemoteDebugger.GdbServerAddress";
inline constexpr char GdbServerBreakOnMainAspectId[]     = "RemoteDebugger.GdbServerBreakOnMain";
inline constexpr char GdbServerExtendedModeAspectId[]    = "RemoteDebugger.GdbServerExtendedMode";

} // namespace Constants

enum ModelRoles
{
    DisplaySourceRole = 32,  // Qt::UserRole

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
