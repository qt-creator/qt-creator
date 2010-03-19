/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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
const char * const LANG_CPP             = "C++";
// common actions
const char * const INTERRUPT            = "Debugger.Interrupt";
const char * const RESET                = "Debugger.Reset";
const char * const STEP                 = "Debugger.StepLine";
const char * const STEPOUT              = "Debugger.StepOut";
const char * const NEXT                 = "Debugger.NextLine";
const char * const REVERSE              = "Debugger.ReverseDirection";

const char * const M_DEBUG_LANGUAGES    = "Debugger.Menu.View.Languages";
const char * const M_DEBUG_VIEWS        = "Debugger.Menu.View.Debug";

const char * const C_BASEDEBUGGER       = "BaseDebugger";
const char * const C_GDBDEBUGGER        = "Gdb Debugger";
const char * const GDBRUNNING           = "Gdb.Running";

const char * const DEBUGGER_COMMON_SETTINGS_ID = "A.Common";
const char * const DEBUGGER_COMMON_SETTINGS_NAME =
    QT_TRANSLATE_NOOP("Debugger", "Common");
const char * const DEBUGGER_SETTINGS_CATEGORY = "O.Debugger";
const char * const DEBUGGER_SETTINGS_TR_CATEGORY =
    QT_TRANSLATE_NOOP("Debugger", "Debugger");

namespace Internal {
    enum { debug = 0 };
#ifdef Q_OS_MAC
    const char * const LD_PRELOAD_ENV_VAR = "DYLD_INSERT_LIBRARIES";
#else
    const char * const LD_PRELOAD_ENV_VAR = "LD_PRELOAD";
#endif

} // namespace Internal
} // namespace Constants


enum DebuggerState
{
    DebuggerNotReady,          // Debugger not started

    EngineStarting,            // Engine starts

    AdapterStarting,
    AdapterStarted,
    AdapterStartFailed,
    InferiorUnrunnable,         // Used in the core dump adapter
    InferiorStarting,
    // InferiorStarted,         // Use InferiorRunningRequested or InferiorStopped
    InferiorStartFailed,

    InferiorRunningRequested,   // Debuggee requested to run
    InferiorRunningRequested_Kill, // Debuggee requested to run, but want to kill it
    InferiorRunning,            // Debuggee running

    InferiorStopping,           // Debuggee running, stop requested
    InferiorStopping_Kill,      // Debuggee running, stop requested, want to kill it
    InferiorStopped,            // Debuggee stopped
    InferiorStopFailed,         // Debuggee not stopped, will kill debugger

    InferiorShuttingDown,
    InferiorShutDown,
    InferiorShutdownFailed,

    EngineShuttingDown
};

enum DebuggerStartMode
{
    NoStartMode,
    StartInternal,         // Start current start project's binary
    StartExternal,         // Start binary found in file system
    AttachExternal,        // Attach to running process by process id
    AttachCrashedExternal, // Attach to crashed process by process id
    AttachCore,            // Attach to a core file
    StartRemote            // Start and attach to a remote process
};

enum DebuggerCapabilities
{
    ReverseSteppingCapability = 0x1,
    SnapshotCapability = 0x2,
    AutoDerefPointersCapability = 0x4,
    DisassemblerCapability = 0x80,
    RegisterCapability = 0x10,
    ShowMemoryCapability = 0x20,
    JumpToLineCapability = 0x40,
    ReloadModuleCapability = 0x80,
    ReloadModuleSymbolsCapability = 0x100,
    BreakOnThrowAndCatchCapability = 0x200,
    ReturnFromFunctionCapability = 0x400,
};

enum LogChannel
{
    LogInput,               // Used for user input
    LogOutput,
    LogWarning,
    LogError,
    LogStatus,              // Used for status changed messages
    LogTime,                // Used for time stamp messages
    LogDebug,
    LogMisc
};

} // namespace Debugger

#endif // DEBUGGERCONSTANTS_H

