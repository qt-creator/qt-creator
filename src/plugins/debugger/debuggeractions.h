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

#ifndef DEBUGGER_ACTIONS_H
#define DEBUGGER_ACTIONS_H

#include <QtCore/QHash>

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;
class QSettings;
QT_END_NAMESPACE

namespace Utils {
    class SavedAction;
}

namespace Debugger {
namespace Internal {

class DebuggerSettings : public QObject
{
    Q_OBJECT
public:
    DebuggerSettings(QObject *parent = 0);
    ~DebuggerSettings();

    void insertItem(int code, Utils::SavedAction *item);
    Utils::SavedAction *item(int code) const;

    QString dump() const;

    static DebuggerSettings *instance();

public slots:
    void readSettings(QSettings *settings);
    void writeSettings(QSettings *settings) const;

private:
    QHash<int, Utils::SavedAction *> m_items;
};


///////////////////////////////////////////////////////////

enum DebuggerActionCode
{
    // General
    SettingsDialog,
    AdjustColumnWidths,
    AlwaysAdjustColumnWidths,
    UseAlternatingRowColors,
    UseMessageBoxForSignals,
    AutoQuit,
    LockView,
    LogTimeStamps,
    VerboseLog,
    OperateByInstruction,
    AutoDerefPointers,

    RecheckDebuggingHelpers,
    UseDebuggingHelpers,
    UseCustomDebuggingHelperLocation,
    CustomDebuggingHelperLocation,
    DebugDebuggingHelpers,

    UseCodeModel,

    UseToolTipsInMainEditor,
    UseToolTipsInLocalsView,
    UseToolTipsInBreakpointsView,
    UseToolTipsInStackView,
    UseAddressInBreakpointsView,
    UseAddressInStackView,

    RegisterForPostMortem,

    // Gdb
    GdbLocation,
    GdbEnvironment,
    GdbScriptFile,
    ExecuteCommand,
    GdbWatchdogTimeout,

    // Stack
    MaximalStackDepth,
    ExpandStack,
    CreateFullBacktrace,

    // Watchers & Locals
    WatchExpression,
    WatchExpressionInWindow,
    RemoveWatchExpression,
    WatchPoint,
    AssignValue,
    AssignType,
    ShowStdNamespace,
    ShowQtNamespace,

    // Source List
    ListSourceFiles,

    // Running
    SkipKnownFrames,
    EnableReverseDebugging,

    // Breakpoints
    SynchronizeBreakpoints,
    AllPluginBreakpoints,
    SelectedPluginBreakpoints,
    NoPluginBreakpoints,
    SelectedPluginBreakpointsPattern,
    UsePreciseBreakpoints,
    BreakOnThrow,
    BreakOnCatch,

    // UI/Language switching
    SwitchLanguageAutomatically
};

// singleton access
Utils::SavedAction *theDebuggerAction(int code);

// convenience
bool theDebuggerBoolSetting(int code);
QString theDebuggerStringSetting(int code);

struct DebuggerManagerActions
{
    QAction *continueAction;
    QAction *stopAction;
    QAction *resetAction; // FIXME: Should not be needed in a stable release
    QAction *stepAction;
    QAction *stepOutAction;
    QAction *runToLineAction1; // in the Debug menu
    QAction *runToLineAction2; // in the text editor context menu
    QAction *runToFunctionAction;
    QAction *jumpToLineAction1; // in the Debug menu
    QAction *jumpToLineAction2; // in the text editor context menu
    QAction *returnFromFunctionAction;
    QAction *nextAction;
    QAction *snapshotAction;
    QAction *watchAction1; // in the Debug menu
    QAction *watchAction2; // in the text editor context menu
    QAction *breakAction;
    QAction *sepAction;
    QAction *reverseDirectionAction;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_WATCHWINDOW_H
