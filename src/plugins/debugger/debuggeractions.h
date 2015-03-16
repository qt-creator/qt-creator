/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DEBUGGER_ACTIONS_H
#define DEBUGGER_ACTIONS_H

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
    bool operator==(const GlobalDebuggerOptions &rhs) const
    {
        return sourcePathMap == rhs.sourcePathMap
                && sourcePathRegExpMap == rhs.sourcePathRegExpMap;
    }
    bool operator!=(const GlobalDebuggerOptions &rhs) const
    {
        return !(*this == rhs);
    }

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
    VerboseLog,
    OperateByInstruction,
    OperateNativeMixed,
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
    IgnoreFirstChanceAccessViolation,

    // Gdb
    LoadGdbInit,
    LoadGdbDumpers,
    AttemptQuickStart,
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
    SortStructMembers,
    AutoDerefPointers,
    MaximalStringLength,
    DisplayStringLimit,

    // Source List
    ListSourceFiles,

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

#endif // DEBUGGER_WATCHWINDOW_H
