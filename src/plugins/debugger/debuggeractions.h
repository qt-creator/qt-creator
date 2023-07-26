// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

#include <QMap>

namespace Debugger::Internal {

class DebuggerSettings
{
public:
    DebuggerSettings();

    static QString dump();

    // Page 1: General
    Utils::BoolAspect &useAlternatingRowColors;
    Utils::BoolAspect &useAnnotationsInMainEditor;
    Utils::BoolAspect &useToolTipsInMainEditor;
    Utils::BoolAspect &closeSourceBuffersOnExit;
    Utils::BoolAspect &closeMemoryBuffersOnExit;
    Utils::BoolAspect &raiseOnInterrupt;
    Utils::BoolAspect &breakpointsFullPathByDefault;
    Utils::BoolAspect &warnOnReleaseBuilds;
    Utils::IntegerAspect &maximalStackDepth;

    Utils::BoolAspect &fontSizeFollowsEditor;
    Utils::BoolAspect &switchModeOnExit;
    Utils::BoolAspect &showQmlObjectTree;
    Utils::BoolAspect &stationaryEditorWhileStepping;
    Utils::BoolAspect &forceLoggingToConsole;

    Utils::TypedAspect<QMap<QString, QString>> &sourcePathMap;

    Utils::BoolAspect &registerForPostMortem;

    // Page 2: Gdb
    Utils::IntegerAspect &gdbWatchdogTimeout;
    Utils::BoolAspect &skipKnownFrames;
    Utils::BoolAspect &useMessageBoxForSignals;
    Utils::BoolAspect &adjustBreakpointLocations;
    Utils::BoolAspect &useDynamicType;
    Utils::BoolAspect &loadGdbInit;
    Utils::BoolAspect &loadGdbDumpers;
    Utils::BoolAspect &intelFlavor;
    Utils::BoolAspect &usePseudoTracepoints;
    Utils::BoolAspect &useIndexCache;
    Utils::StringAspect &gdbStartupCommands;
    Utils::StringAspect &gdbPostAttachCommands;

    Utils::BoolAspect &targetAsync;
    Utils::BoolAspect &autoEnrichParameters;
    Utils::BoolAspect &breakOnThrow;
    Utils::BoolAspect &breakOnCatch;
    Utils::BoolAspect &breakOnWarning;
    Utils::BoolAspect &breakOnFatal;
    Utils::BoolAspect &breakOnAbort;
    Utils::BoolAspect &enableReverseDebugging;
    Utils::BoolAspect &multiInferior;

    // Page 4: Locals and expressions
    Utils::BoolAspect &useDebuggingHelpers;
    Utils::BoolAspect &useCodeModel;
    Utils::BoolAspect &showThreadNames;
    Utils::FilePathAspect &extraDumperFile;   // For loading a file. Recommended.
    Utils::StringAspect &extraDumperCommands; // To modify an existing setup.

    Utils::BoolAspect &showStdNamespace;
    Utils::BoolAspect &showQtNamespace;
    Utils::BoolAspect &showQObjectNames;

    Utils::IntegerAspect &maximalStringLength;
    Utils::IntegerAspect &displayStringLimit;
    Utils::IntegerAspect &defaultArraySize;

    // Page 5: CDB
    Utils::StringAspect cdbAdditionalArguments;
    Utils::StringListAspect cdbBreakEvents;
    Utils::BoolAspect cdbBreakOnCrtDbgReport;
    Utils::BoolAspect useCdbConsole;
    Utils::BoolAspect cdbBreakPointCorrection;
    Utils::BoolAspect cdbUsePythonDumper;
    Utils::BoolAspect firstChanceExceptionTaskEntry;
    Utils::BoolAspect secondChanceExceptionTaskEntry;
    Utils::BoolAspect ignoreFirstChanceAccessViolation;

    // Page 6: CDB Paths
    Utils::StringListAspect cdbSymbolPaths;
    Utils::StringListAspect cdbSourcePaths;

    // Without pages
    Utils::BoolAspect alwaysAdjustColumnWidths;
    Utils::BaseAspect settingsDialog;
    Utils::BoolAspect autoQuit;
    Utils::BoolAspect lockView;
    Utils::BoolAspect logTimeStamps;

    // Stack
    Utils::BaseAspect expandStack;
    Utils::BaseAspect createFullBacktrace;
    Utils::BoolAspect useToolTipsInStackView;

    // Watchers & Locals
    Utils::BoolAspect autoDerefPointers;
    Utils::BoolAspect sortStructMembers;
    Utils::BoolAspect useToolTipsInLocalsView;

    // Breakpoints
    Utils::BoolAspect synchronizeBreakpoints; // ?
    Utils::BoolAspect allPluginBreakpoints;
    Utils::BoolAspect selectedPluginBreakpoints;
    Utils::BoolAspect noPluginBreakpoints;
    Utils::StringAspect selectedPluginBreakpointsPattern;
    Utils::BoolAspect useToolTipsInBreakpointsView;

    // QML Tools
    Utils::BoolAspect showAppOnTop;

    Utils::AspectContainer all; // All
    Utils::AspectContainer page5; // CDB
    Utils::AspectContainer page6; // CDB Paths

private:
    DebuggerSettings(const DebuggerSettings &) = delete;
    DebuggerSettings &operator=(const DebuggerSettings &) = delete;
};

DebuggerSettings &settings();

QString msgSetBreakpointAtFunction(const char *function);
QString msgSetBreakpointAtFunctionToolTip(const char *function, const QString &hint = {});

} // Debugger::Internal
