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

#include <QObject>
#include <QHash>
#include <QMap>
#include <QRegExp>
#include <QVector>

namespace Utils { class SavedAction; }

namespace Debugger {
namespace Internal {

typedef QMap<QString, QString> SourcePathMap;
typedef QVector<QPair<QRegExp, QString> > SourcePathRegExpMap;

// Global debugger options that are not stored as saved action.
class GlobalDebuggerOptions
{
public:
    void toSettings() const;
    void fromSettings();

    SourcePathMap sourcePathMap;
    SourcePathRegExpMap sourcePathRegExpMap;
};

class DebuggerSettings : public QObject
{
    Q_OBJECT // For tr().

public:
    explicit DebuggerSettings();
    ~DebuggerSettings();

    void insertItem(int code, Utils::SavedAction *item);
    Utils::SavedAction *item(int code) const;

    QString dump() const;

    void readSettings();
    void writeSettings() const;

private:
    QHash<int, Utils::SavedAction *> m_items;
};

///////////////////////////////////////////////////////////

enum DebuggerActionCode
{
    // General
    SettingsDialog,
    UseAlternatingRowColors,
    FontSizeFollowsEditor,
    UseMessageBoxForSignals,
    AutoQuit,
    LockView,
    LogTimeStamps,
    OperateByInstruction,
    CloseSourceBuffersOnExit,
    CloseMemoryBuffersOnExit,
    SwitchModeOnExit,
    BreakpointsFullPathByDefault,
    RaiseOnInterrupt,
    StationaryEditorWhileStepping,

    UseDebuggingHelpers,

    UseCodeModel,
    ShowThreadNames,

    UseToolTipsInMainEditor,
    UseToolTipsInLocalsView,
    UseToolTipsInBreakpointsView,
    UseToolTipsInStackView,
    UseAddressInBreakpointsView,
    UseAddressInStackView,

    RegisterForPostMortem,
    AlwaysAdjustColumnWidths,

    ExtraDumperFile,     // For loading a file. Recommended.
    ExtraDumperCommands, // To modify an existing setup.

    // Cdb
    CdbAdditionalArguments,
    CdbSymbolPaths,
    CdbSourcePaths,
    CdbBreakEvents,
    CdbBreakOnCrtDbgReport,
    UseCdbConsole,
    CdbBreakPointCorrection,
    CdbUsePythonDumper,
    IgnoreFirstChanceAccessViolation,

    // Gdb
    LoadGdbInit,
    LoadGdbDumpers,
    GdbStartupCommands,
    GdbPostAttachCommands,
    GdbWatchdogTimeout,
    AutoEnrichParameters,
    UseDynamicType,
    TargetAsync,
    WarnOnReleaseBuilds,
    MultiInferior,
    IntelFlavor,
    IdentifyDebugInfoPackages,

    // Stack
    MaximalStackDepth,
    ExpandStack,
    CreateFullBacktrace,

    // Watchers & Locals
    ShowStdNamespace,
    ShowQtNamespace,
    ShowQObjectNames,
    SortStructMembers,
    AutoDerefPointers,
    MaximalStringLength,
    DisplayStringLimit,

    // Running
    SkipKnownFrames,
    EnableReverseDebugging,

    // Breakpoints
    SynchronizeBreakpoints,
    AllPluginBreakpoints,
    SelectedPluginBreakpoints,
    AdjustBreakpointLocations,
    NoPluginBreakpoints,
    SelectedPluginBreakpointsPattern,
    BreakOnThrow,
    BreakOnCatch,
    BreakOnWarning,
    BreakOnFatal,
    BreakOnAbort,

    // QML Tools
    ShowQmlObjectTree,
    ShowAppOnTop
};

} // namespace Internal
} // namespace Debugger
