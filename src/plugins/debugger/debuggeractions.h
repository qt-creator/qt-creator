// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QCoreApplication>
#include <QHash>
#include <QMap>
#include <QVector>

#include <utils/aspects.h>

namespace Debugger::Internal {

class SourcePathMapAspectPrivate;

// Entries starting with '(' are considered regular expressions in the ElfReader.
// This is useful when there are multiple build machines with different
// path, and the user would like to match anything up to some known
// directory to his local project.
// Syntax: (/home/.*)/KnownSubdir -> /home/my/project
using SourcePathMap = QMap<QString, QString>;

class SourcePathMapAspect : public Utils::BaseAspect
{
public:
    SourcePathMapAspect();
    ~SourcePathMapAspect() override;

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    void addToLayout(Utils::LayoutBuilder &builder) override;

    QVariant volatileValue() const override;
    void setVolatileValue(const QVariant &val) override;

    void readSettings(const QSettings *settings) override;
    void writeSettings(QSettings *settings) const override;

    SourcePathMap value() const;

private:
    SourcePathMapAspectPrivate *d = nullptr;
};

class GeneralSettings
{
    GeneralSettings();
    ~GeneralSettings();
};

class DebuggerSettings
{
public:
    explicit DebuggerSettings();
    ~DebuggerSettings();

    static QString dump();

    // Page 1: General
    Utils::BoolAspect useAlternatingRowColors;
    Utils::BoolAspect useAnnotationsInMainEditor;
    Utils::BoolAspect useToolTipsInMainEditor;
    Utils::BoolAspect closeSourceBuffersOnExit;
    Utils::BoolAspect closeMemoryBuffersOnExit;
    Utils::BoolAspect raiseOnInterrupt;
    Utils::BoolAspect breakpointsFullPathByDefault;
    Utils::BoolAspect warnOnReleaseBuilds;
    Utils::IntegerAspect maximalStackDepth;

    Utils::BoolAspect fontSizeFollowsEditor;
    Utils::BoolAspect switchModeOnExit;
    Utils::BoolAspect showQmlObjectTree;
    Utils::BoolAspect stationaryEditorWhileStepping;
    Utils::BoolAspect forceLoggingToConsole;

    SourcePathMapAspect sourcePathMap;

    // Page 2: GDB
    Utils::IntegerAspect gdbWatchdogTimeout;
    Utils::BoolAspect skipKnownFrames;
    Utils::BoolAspect useMessageBoxForSignals;
    Utils::BoolAspect adjustBreakpointLocations;
    Utils::BoolAspect useDynamicType;
    Utils::BoolAspect loadGdbInit;
    Utils::BoolAspect loadGdbDumpers;
    Utils::BoolAspect intelFlavor;
    Utils::BoolAspect usePseudoTracepoints;
    Utils::BoolAspect useIndexCache;
    Utils::StringAspect gdbStartupCommands;
    Utils::StringAspect gdbPostAttachCommands;

    // Page 3: GDB Extended
    Utils::BoolAspect targetAsync;
    Utils::BoolAspect autoEnrichParameters;
    Utils::BoolAspect breakOnThrow;
    Utils::BoolAspect breakOnCatch;
    Utils::BoolAspect breakOnWarning;
    Utils::BoolAspect breakOnFatal;
    Utils::BoolAspect breakOnAbort;
    Utils::BoolAspect enableReverseDebugging;
    Utils::BoolAspect multiInferior;

    // Page 4: Locals and expressions
    Utils::BoolAspect useDebuggingHelpers;
    Utils::BoolAspect useCodeModel;
    Utils::BoolAspect showThreadNames;
    Utils::StringAspect extraDumperFile;     // For loading a file. Recommended.
    Utils::StringAspect extraDumperCommands; // To modify an existing setup.

    Utils::BoolAspect showStdNamespace;
    Utils::BoolAspect showQtNamespace;
    Utils::BoolAspect showQObjectNames;

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

    Utils::BoolAspect *registerForPostMortem = nullptr;

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
    Utils::IntegerAspect maximalStringLength;
    Utils::IntegerAspect displayStringLimit;
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
    Utils::AspectContainer page1; // General
    Utils::AspectContainer page2; // GDB
    Utils::AspectContainer page4; // Locals & Expressions
    Utils::AspectContainer page5; // CDB
    Utils::AspectContainer page6; // CDB Paths

    void readSettings();
    void writeSettings() const;

private:
    DebuggerSettings(const DebuggerSettings &) = delete;
    DebuggerSettings &operator=(const DebuggerSettings &) = delete;
};

DebuggerSettings *debuggerSettings();

} // Debugger::Internal

Q_DECLARE_METATYPE(Debugger::Internal::SourcePathMap)
