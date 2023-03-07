# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Debugger start modes. Keep in sync with DebuggerStartMode in debuggerconstants.h


# MT: Why does this not match (anymore?) to debuggerconstants.h : DebuggerStartMode ?
class DebuggerStartMode():
    (
        NoStartMode,
        StartInternal,
        StartExternal,
        AttachExternal,
        AttachCrashedExternal,
        AttachCore,
        AttachToRemoteServer,
        AttachToRemoteProcess,
        StartRemoteProcess,
    ) = range(0, 9)


# Known special formats. Keep in sync with DisplayFormat in debuggerprotocol.h
class DisplayFormat():
    (
        Automatic,
        Raw,
        Simple,
        Enhanced,
        Separate,
        Latin1String,
        SeparateLatin1String,
        Utf8String,
        SeparateUtf8String,
        Local8BitString,
        Utf16String,
        Ucs4String,
        Array10,
        Array100,
        Array1000,
        Array10000,
        ArrayPlot,
        CompactMap,
        DirectQListStorage,
        IndirectQListStorage,
    ) = range(0, 20)


# Breakpoints. Keep synchronized with BreakpointType in breakpoint.h
class BreakpointType():
    (
        UnknownType,
        BreakpointByFileAndLine,
        BreakpointByFunction,
        BreakpointByAddress,
        BreakpointAtThrow,
        BreakpointAtCatch,
        BreakpointAtMain,
        BreakpointAtFork,
        BreakpointAtExec,
        BreakpointAtSysCall,
        WatchpointAtAddress,
        WatchpointAtExpression,
        BreakpointOnQmlSignalEmit,
        BreakpointAtJavaScriptThrow,
    ) = range(0, 14)


# Internal codes for types. Keep in sync with cdbextensions pytype.cpp
class TypeCode():
    (
        Typedef,
        Struct,
        Void,
        Integral,
        Float,
        Enum,
        Pointer,
        Array,
        Complex,
        Reference,
        Function,
        MemberPointer,
        FortranString,
        Unresolvable,
        Bitfield,
        RValueReference,
    ) = range(0, 16)


# Internal codes for logging channels. Keep in sync woth debuggerconstants.h
class LogChannel():
    (
        LogInput,                # Used for user input
        LogMiscInput,            # Used for misc stuff in the input pane
        LogOutput,
        LogWarning,
        LogError,
        LogStatus,               # Used for status changed messages
        LogTime,                 # Used for time stamp messages
        LogDebug,
        LogMisc,
        AppOutput,               # stdout
        AppError,                # stderr
        AppStuff,                # (possibly) windows debug channel
        StatusBar,               # LogStatus and also put to the status bar
        ConsoleOutput            # Used to output to console
    ) = range(0, 14)


def isIntegralTypeName(name):
    return name in (
        "int",
        "unsigned int",
        "signed int",
        "short",
        "unsigned short",
        "long",
        "unsigned long",
        "long long",
        "unsigned long long",
        "char",
        "signed char",
        "unsigned char",
        "bool",
    )


def isFloatingPointTypeName(name):
    return name in ("float", "double", "long double")
