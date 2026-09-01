// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "debugger_global.h"

#include <QFlags>
#include <QMetaObject>

namespace Debugger {

Q_NAMESPACE_EXPORT(DEBUGGER_EXPORT)

namespace Constants {

// Debug mode
inline constexpr char MODE_DEBUG[]             = "Mode.Debug";

// Debug mode context
inline constexpr char C_DEBUGMODE[]            = "Debugger.DebugMode";

inline constexpr char DEBUGGER_RUN_FACTORY[]   = "DebuggerRunWorkerFactory";

// Debugger commands
inline constexpr char DEBUGGER_START[]          = "Debugger.Start";

} // namespace Constants

// Keep in sync with debugger/utils.py
enum DebuggerStartMode {
    NoStartMode,
    StartInternal,          // Start current start project's binary
    StartExternal,          // Start binary found in file system
    AttachToLocalProcess,   // Attach to running local process by process id
    AttachToCrashedProcess, // Attach to crashed process by process id
    AttachToCore,           // Attach to a core file
    AttachToRemoteServer,   // Attach to a running gdbserver
    AttachToRemoteProcess,  // Attach to a running remote process
    AttachToQmlServer,      // Attach to a running QmlServer
    StartRemoteProcess,     // Start and attach to a remote process
    AttachToIosDevice       // Attach to an application on a iOS 17+ device
};

enum DebuggerCloseMode
{
    KillAtClose,
    KillAndExitMonitorAtClose,
    DetachAtClose
};

// Governs DebuggerEngine::canHandleToolTip(): whether an engine supports
// showing a debugger tooltip at all, and under what condition.
enum class ToolTipHandling
{
    Always,
    IfStoppedInferior,
    IfStoppedInferiorAndCppEditor, // default
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
    OperateByInstructionCapability    = 1 << 21,
    RunToLineCapability               = 1 << 22,
    ShowModuleSectionsCapability      = 1 << 23,
    WatchComplexExpressionsCapability = 1 << 24, // Used to filter out challenges for cdb.
    AdditionalQmlStackCapability      = 1 << 25, //!< C++ debugger engine is able to retrieve QML stack as well.
    ResetInferiorCapability           = 1 << 26, //!< restart program while debugging
    BreakIndividualLocationsCapability= 1 << 27  //!< Allows to enable/disable individual location for multi-location bps
};
Q_ENUM_NS(DebuggerCapabilities)

enum class DebuggerExtraCapability : unsigned
{
    Detach               = 1u << 0,
    LibraryEvent         = 1u << 1,
    RunCommandDeferral   = 1u << 2,
    SignalReceived       = 1u << 3,
    SourceFiles          = 1u << 4,
    Threads              = 1u << 5,
    BreakOnMain          = 1u << 6,
    StopBeforeRun        = 1u << 7,
    SpecialBreakpoints   = 1u << 8, // Breaking on abort(), qWarning(), qFatal().
    SkipKnownFrames      = 1u << 9,
    JumpTargetCheck      = 1u << 10, // Refusing a jump to a line of several locations.
    PeripheralRegisters  = 1u << 11,
    ContinueAfterAttach  = 1u << 12
};
Q_DECLARE_FLAGS(DebuggerExtraCapabilities, DebuggerExtraCapability)
Q_DECLARE_OPERATORS_FOR_FLAGS(DebuggerExtraCapabilities)
Q_FLAG_NS(DebuggerExtraCapabilities)

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
    LldbEngineType    = 0x100,
    GdbDapEngineType  = 0x200,
    LldbDapEngineType = 0x400,
    UvscEngineType    = 0x1000,
    BridgeEngineType  = 0x2000
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
