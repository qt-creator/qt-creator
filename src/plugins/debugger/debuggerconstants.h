/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGERCONSTANTS_H
#define DEBUGGERCONSTANTS_H

#include <QFlags>

namespace Debugger {
namespace Constants {

// Debug mode
const char MODE_DEBUG[]             = "Mode.Debug";

// Contexts
const char C_DEBUGMODE[]            = "Debugger.DebugMode";
const char C_CPPDEBUGGER[]          = "Gdb Debugger";
const char C_QMLDEBUGGER[]          = "Qml/JavaScript Debugger";

// Menu Groups
const char G_GENERAL[]              = "Debugger.Group.General";
const char G_SPECIAL[]              = "Debugger.Group.Special";
const char G_START_QML[]            = "Debugger.Group.Start.Qml";

// Common actions
const char INTERRUPT[]              = "Debugger.Interrupt";
const char CONTINUE[]               = "Debugger.Continue";
const char STOP[]                   = "Debugger.Stop";
const char HIDDEN_STOP[]            = "Debugger.HiddenStop";
const char ABORT[]                  = "Debugger.Abort";
const char STEP[]                   = "Debugger.StepLine";
const char STEPOUT[]                = "Debugger.StepOut";
const char NEXT[]                   = "Debugger.NextLine";
const char REVERSE[]                = "Debugger.ReverseDirection";
const char OPERATE_BY_INSTRUCTION[] = "Debugger.OperateByInstruction";
const char QML_SHOW_APP_ON_TOP[]    = "Debugger.QmlShowAppOnTop";
const char QML_UPDATE_ON_SAVE[]     = "Debugger.QmlUpdateOnSave";
const char QML_SELECTTOOL[]         = "Debugger.QmlSelectTool";
const char QML_ZOOMTOOL[]           = "Debugger.QmlZoomTool";

// DebuggerMainWindow dock widget names
const char DOCKWIDGET_BREAK[]        = "Debugger.Docks.Break";
const char DOCKWIDGET_MODULES[]      = "Debugger.Docks.Modules";
const char DOCKWIDGET_REGISTER[]     = "Debugger.Docks.Register";
const char DOCKWIDGET_OUTPUT[]       = "Debugger.Docks.Output";
const char DOCKWIDGET_SNAPSHOTS[]    = "Debugger.Docks.Snapshots";
const char DOCKWIDGET_STACK[]        = "Debugger.Docks.Stack";
const char DOCKWIDGET_SOURCE_FILES[] = "Debugger.Docks.SourceFiles";
const char DOCKWIDGET_THREADS[]      = "Debugger.Docks.Threads";
const char DOCKWIDGET_WATCHERS[]     = "Debugger.Docks.LocalsAndWatchers";

const char DOCKWIDGET_QML_INSPECTOR[]     = "Debugger.Docks.QmlInspector";
const char DOCKWIDGET_DEFAULT_AREA[]      = "Debugger.Docks.DefaultArea";

const char TASK_CATEGORY_DEBUGGER_TEST[]      = "DebuggerTest";
const char TASK_CATEGORY_DEBUGGER_DEBUGINFO[] = "Debuginfo";
const char TASK_CATEGORY_DEBUGGER_RUNTIME[]   = "DebugRuntime";
} // namespace Constants

enum DebuggerState
{
    DebuggerNotReady,          // Debugger not started

    EngineSetupRequested,      // Engine starts
    EngineSetupFailed,
    EngineSetupOk,

    InferiorSetupRequested,
    InferiorSetupFailed,
    InferiorSetupOk,

    EngineRunRequested,
    EngineRunFailed,

    InferiorUnrunnable,        // Used in the core dump adapter

    InferiorRunRequested,      // Debuggee requested to run
    InferiorRunOk,             // Debuggee running
    InferiorRunFailed,         // Debuggee running

    InferiorStopRequested,     // Debuggee running, stop requested
    InferiorStopOk,            // Debuggee stopped
    InferiorStopFailed,        // Debuggee not stopped, will kill debugger

    InferiorExitOk,

    InferiorShutdownRequested,
    InferiorShutdownFailed,
    InferiorShutdownOk,

    EngineShutdownRequested,
    EngineShutdownFailed,
    EngineShutdownOk,

    DebuggerFinished
};

enum DebuggerStartMode
{
    NoStartMode,
    StartInternal,         // Start current start project's binary
    StartExternal,         // Start binary found in file system
    AttachExternal,        // Attach to running process by process id
    AttachCrashedExternal, // Attach to crashed process by process id
    AttachCore,            // Attach to a core file
    AttachToRemoteServer,  // Attach to a running gdbserver
    AttachToRemoteProcess, // Attach to a running remote process
    LoadRemoteCore,    // Load a remote core file
    StartRemoteProcess,    // Start and attach to a remote process
    StartRemoteGdb,        // Start gdb itself remotely
    StartRemoteEngine      // Start ipc guest engine on other machine
};

enum DebuggerCloseMode
{
    KillAtClose,
    DetachAtClose
};

enum DebuggerCapabilities
{
    ReverseSteppingCapability = 0x1,
    SnapshotCapability = 0x2,
    AutoDerefPointersCapability = 0x4,
    DisassemblerCapability = 0x8,
    RegisterCapability = 0x10,
    ShowMemoryCapability = 0x20,
    JumpToLineCapability = 0x40,
    ReloadModuleCapability = 0x80,
    ReloadModuleSymbolsCapability = 0x100,
    BreakOnThrowAndCatchCapability = 0x200,
    BreakConditionCapability = 0x400, //!< Conditional Breakpoints
    BreakModuleCapability = 0x800, //!< Breakpoint specification includes module
    TracePointCapability = 0x1000,
    ReturnFromFunctionCapability = 0x2000,
    CreateFullBacktraceCapability = 0x4000,
    AddWatcherCapability = 0x8000,
    AddWatcherWhileRunningCapability = 0x10000,
    WatchWidgetsCapability = 0x20000,
    WatchpointByAddressCapability = 0x40000,
    WatchpointByExpressionCapability = 0x80000,
    ShowModuleSymbolsCapability = 0x100000,
    CatchCapability = 0x200000, //!< fork, vfork, syscall
    OperateByInstructionCapability = 0x400000,
    RunToLineCapability = 0x800000,
    MemoryAddressCapability = 0x1000000,
    ShowModuleSectionsCapability = 0x200000,
    WatchComplexExpressionsCapability = 0x400000 // Used to filter out challenges for cdb.
};

enum LogChannel
{
    LogInput,                // Used for user input
    LogMiscInput,            // Used for misc stuff in the input pane
    LogOutput,
    LogWarning,
    LogError,
    LogStatus,               // Used for status changed messages
    LogTime,                 // Used for time stamp messages
    LogDebug,
    LogMisc,
    AppOutput,               // stdout
    AppError,                // stderr
    AppStuff,                // (possibly) windows debug channel
    StatusBar,               // LogStatus and also put to the status bar
    ConsoleOutput            // Used to output to console
};

enum DebuggerEngineType
{
    NoEngineType      = 0,
    GdbEngineType     = 0x01,
    ScriptEngineType  = 0x02,
    CdbEngineType     = 0x04,
    PdbEngineType     = 0x08,
    QmlEngineType     = 0x20,
    QmlCppEngineType  = 0x40,
    LldbEngineType  = 0x80,
    AllEngineTypes = GdbEngineType
        | ScriptEngineType
        | CdbEngineType
        | PdbEngineType
        | QmlEngineType
        | QmlCppEngineType
        | LldbEngineType
};

enum DebuggerLanguage
{
    AnyLanguage       = 0x0,
    CppLanguage      = 0x1,
    QmlLanguage      = 0x2
};
Q_DECLARE_FLAGS(DebuggerLanguages, DebuggerLanguage)

} // namespace Debugger

#endif // DEBUGGERCONSTANTS_H
