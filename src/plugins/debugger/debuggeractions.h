// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gdb/gdbsettings.h"

#include <QCoreApplication>
#include <QHash>
#include <QMap>
#include <QVector>

namespace Debugger::Internal {

class SourcePathMapAspectPrivate;

// Entries starting with '(' are considered regular expressions in the ElfReader.
// This is useful when there are multiple build machines with different
// path, and the user would like to match anything up to some known
// directory to his local project.
// Syntax: (/home/.*)/KnownSubdir -> /home/my/project
using SourcePathMap = QMap<QString, QString>;

class SourcePathMapAspect : public Utils::TypedAspect<SourcePathMap>
{
public:
    SourcePathMapAspect();
    ~SourcePathMapAspect() override;

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    void addToLayout(Layouting::LayoutItem &parent) override;

    void readSettings() override;
    void writeSettings() const override;

private:
    void guiToInternal() override;
    void internalToGui() override;

    SourcePathMapAspectPrivate *d = nullptr;
};

class GeneralSettings
{
    GeneralSettings();
    ~GeneralSettings();
};

class DebuggerSettings : public GdbSettings
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

    // Page 4: Locals and expressions
    Utils::BoolAspect useDebuggingHelpers;
    Utils::BoolAspect useCodeModel;
    Utils::BoolAspect showThreadNames;
    Utils::FilePathAspect extraDumperFile;   // For loading a file. Recommended.
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
    Utils::IntegerAspect defaultArraySize;
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
