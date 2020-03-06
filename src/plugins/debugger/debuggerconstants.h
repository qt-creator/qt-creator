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

#include <QFlags>

namespace Debugger {
namespace Constants {

// Debug mode
const char MODE_DEBUG[]             = "Mode.Debug";

// Debug mode context
const char C_DEBUGMODE[]            = "Debugger.DebugMode";

// Common debugger constants.
const char kPeripheralDescriptionFile[] = "PeripheralDescriptionFile";

// UVSC-specific debugger constants.
const char kUVisionProjectFilePath[] = "UVisionProjectFilePath";
const char kUVisionOptionsFilePath[] = "UVisionOptionsFilePath";
const char kUVisionSimulator[] = "UVisionSimulator";

} // namespace Constants

// Keep in sync with dumper.py
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
    AttachToQmlServer,     // Attach to a running QmlServer
    StartRemoteProcess     // Start and attach to a remote process
};

enum DebuggerCloseMode
{
    KillAtClose,
    KillAndExitMonitorAtClose,
    DetachAtClose
};

enum DebuggerCapabilities
{
    ReverseSteppingCapability         = 1 <<  0,
    SnapshotCapability                = 1 <<  1,
    AutoDerefPointersCapability       = 1 <<  2,
    DisassemblerCapability            = 1 <<  3,
    RegisterCapability                = 1 <<  4,
    ShowMemoryCapability              = 1 <<  5,
    JumpToLineCapability              = 1 <<  6,
    ReloadModuleCapability            = 1 <<  7,
    ReloadModuleSymbolsCapability     = 1 <<  8,
    BreakOnThrowAndCatchCapability    = 1 <<  9,
    BreakConditionCapability          = 1 << 10, //!< Conditional Breakpoints
    BreakModuleCapability             = 1 << 11, //!< Breakpoint specification includes module
    TracePointCapability              = 1 << 12,
    ReturnFromFunctionCapability      = 1 << 13,
    CreateFullBacktraceCapability     = 1 << 14,
    AddWatcherCapability              = 1 << 15,
    AddWatcherWhileRunningCapability  = 1 << 16,
    WatchWidgetsCapability            = 1 << 17,
    WatchpointByAddressCapability     = 1 << 18,
    WatchpointByExpressionCapability  = 1 << 19,
    ShowModuleSymbolsCapability       = 1 << 20,
    CatchCapability                   = 1 << 21, //!< fork, vfork, syscall
    OperateByInstructionCapability    = 1 << 22,
    RunToLineCapability               = 1 << 23,
    MemoryAddressCapability           = 1 << 24,
    ShowModuleSectionsCapability      = 1 << 25,
    WatchComplexExpressionsCapability = 1 << 26, // Used to filter out challenges for cdb.
    AdditionalQmlStackCapability      = 1 << 27, //!< C++ debugger engine is able to retrieve QML stack as well.
    ResetInferiorCapability           = 1 << 28, //!< restart program while debugging
    BreakIndividualLocationsCapability= 1 << 29  //!< Allows to enable/disable individual location for multi-location bps
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

// Keep values compatible between Qt Creator versions,
// because they are used by the installer for registering debuggers
enum DebuggerEngineType
{
    NoEngineType      = 0,
    GdbEngineType     = 0x001,
    CdbEngineType     = 0x004,
    PdbEngineType     = 0x008,
    LldbEngineType    = 0x100,
    UvscEngineType    = 0x1000
};

enum DebuggerLanguage
{
    NoLanguage       = 0x0,
    CppLanguage      = 0x1,
    QmlLanguage      = 0x2,
    AnyLanguage      = CppLanguage | QmlLanguage
};

Q_DECLARE_FLAGS(DebuggerLanguages, DebuggerLanguage)

} // namespace Debugger
