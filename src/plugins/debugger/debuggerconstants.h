/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGERCONSTANTS_H
#define DEBUGGERCONSTANTS_H

#include <QtCore/QtGlobal>

namespace Debugger {
namespace Constants {

// modes and their priorities
const char * const MODE_DEBUG           = "Debugger.Mode.Debug";
const int          P_MODE_DEBUG         = 85;

// common actions
const char * const INTERRUPT            = "Debugger.Interrupt";
const char * const CONTINUE             = "Debugger.Continue";
const char * const STOP                 = "Debugger.Stop";
const char * const RESET                = "Debugger.Reset";
const char * const STEP                 = "Debugger.StepLine";
const char * const STEPOUT              = "Debugger.StepOut";
const char * const NEXT                 = "Debugger.NextLine";
const char * const REVERSE              = "Debugger.ReverseDirection";
const char * const OPERATE_BY_INSTRUCTION   = "Debugger.OperateByInstruction";

const char * const M_DEBUG_VIEWS        = "Debugger.Menu.View.Debug";

const char * const C_DEBUGMODE          = "Debugger.DebugMode";
const char * const C_CPPDEBUGGER        = "Gdb Debugger";
const char * const C_QMLDEBUGGER        = "Qml/JavaScript Debugger";

const char * const DEBUGGER_COMMON_SETTINGS_ID = "A.Common";
const char * const DEBUGGER_COMMON_SETTINGS_NAME =
    QT_TRANSLATE_NOOP("Debugger", "General");
const char * const DEBUGGER_SETTINGS_CATEGORY = "O.Debugger";
const char * const DEBUGGER_SETTINGS_TR_CATEGORY =
    QT_TRANSLATE_NOOP("Debugger", "Debugger");
const char * const DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON =
    ":/core/images/category_debug.png";

// dock widget names
const char * const DOCKWIDGET_BREAK      = "Debugger.Docks.Break";
const char * const DOCKWIDGET_CONSOLE    = "Debugger.Docks.Console";
const char * const DOCKWIDGET_MODULES    = "Debugger.Docks.Modules";
const char * const DOCKWIDGET_REGISTER   = "Debugger.Docks.Register";
const char * const DOCKWIDGET_OUTPUT     = "Debugger.Docks.Output";
const char * const DOCKWIDGET_SNAPSHOTS  = "Debugger.Docks.Snapshots";
const char * const DOCKWIDGET_STACK      = "Debugger.Docks.Stack";
const char * const DOCKWIDGET_SOURCE_FILES = "Debugger.Docks.SourceFiles";
const char * const DOCKWIDGET_THREADS    = "Debugger.Docks.Threads";
const char * const DOCKWIDGET_WATCHERS   = "Debugger.Docks.LocalsAndWatchers";

const char * const DOCKWIDGET_QML_INSPECTOR = "Debugger.Docks.QmlInspector";
const char * const DOCKWIDGET_QML_SCRIPTCONSOLE = "Debugger.Docks.ScriptConsole";
const char * const DOCKWIDGET_DEFAULT_AREA = "Debugger.Docks.DefaultArea";

namespace Internal {
    enum { debug = 0 };
} // namespace Internal

const char * const OPENED_BY_DEBUGGER         = "OpenedByDebugger";
const char * const OPENED_WITH_DISASSEMBLY    = "DisassemblerView";
const char * const OPENED_WITH_MEMORY         = "MemoryView";

const char * const DEBUGMODE            = "Debugger.DebugMode";
const char * const DEBUG                = "Debugger.Debug";
const int          P_ACTION_DEBUG       = 90; //priority for the modemanager
#ifdef Q_OS_MAC
const char * const DEBUG_KEY = "Ctrl+Y";
#else
const char * const DEBUG_KEY = "F5";
#endif

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
    AttachTcf,             // Attach to a running Target Communication Framework agent
    AttachCore,            // Attach to a core file
    AttachToRemote,        // Start and attach to a remote process
    StartRemoteGdb,        // Start gdb itself remotely
    StartRemoteEngine      // Start ipc guest engine on other machine
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
    WatchpointCapability = 0x10000,
    ShowModuleSymbolsCapability = 0x20000,
    CatchCapability = 0x40000, //!< fork, vfork, syscall
    AllDebuggerCapabilities = 0xFFFFFFFF
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
    AppOutput,
    AppError,
    AppStuff,
    StatusBar,                // LogStatus and also put to the status bar
    ScriptConsoleOutput
};

enum ModelRoles
{
    DisplaySourceRole = 32,  // Qt::UserRole

    EngineStateRole,
    EngineCapabilitiesRole,
    EngineActionsEnabledRole,
    RequestActivationRole,
    RequestContextMenuRole,

    // Locals and Watchers
    LocalsINameRole,
    LocalsEditTypeRole,     // A QVariant::type describing the item
    LocalsIntegerBaseRole,  // Number base 16, 10, 8, 2
    LocalsExpressionRole,
    LocalsRawExpressionRole,
    LocalsExpandedRole,     // The preferred expanded state to the view
    LocalsTypeFormatListRole,
    LocalsTypeFormatRole,   // Used to communicate alternative formats to the view
    LocalsIndividualFormatRole,
    LocalsAddressRole,      // Memory address of variable as quint64
    LocalsRawValueRole,     // Unformatted value as string
    LocalsPointerValueRole, // Pointer value (address) as quint64
    LocalsIsWatchpointAtAddressRole,
    LocalsIsWatchpointAtPointerValueRole,

    // Snapshots
    SnapshotCapabilityRole
};

enum DebuggerEngineType
{
    NoEngineType      = 0,
    GdbEngineType     = 0x01,
    ScriptEngineType  = 0x02,
    CdbEngineType     = 0x04,
    PdbEngineType     = 0x08,
    TcfEngineType     = 0x10,
    QmlEngineType     = 0x20,
    QmlCppEngineType  = 0x40,
    LldbEngineType  = 0x80,
    AllEngineTypes = GdbEngineType
        | ScriptEngineType
        | CdbEngineType
        | PdbEngineType
        | TcfEngineType
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

