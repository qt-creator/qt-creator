/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_ACTIONS_H
#define DEBUGGER_ACTIONS_H

#include <QtCore/QHash>

#include <utils/savedaction.h>

namespace Debugger {
namespace Internal {

class DebuggerSettings : public QObject
{
    Q_OBJECT

public:
    DebuggerSettings(QObject *parent = 0);
    ~DebuggerSettings();
    
    void insertItem(int code, Core::Utils::SavedAction *item);
    Core::Utils::SavedAction *item(int code);

    QString dump();

    static DebuggerSettings *instance();

public slots:
    void readSettings(QSettings *settings);
    void writeSettings(QSettings *settings);

private:
    QHash<int, Core::Utils::SavedAction *> m_items; 
};


///////////////////////////////////////////////////////////

enum DebuggerActionCode
{
    // General
    SettingsDialog,
    AdjustColumnWidths,
    AlwaysAdjustColumnWidths,
    AutoQuit,
    TerminalApplication,
    LockView,

    // Gdb
    GdbLocation,
    GdbEnvironment,
    GdbScriptFile,
    ExecuteCommand,

    // Stack
    MaximalStackDepth,
    ExpandStack,

    // Watchers & Locals
    WatchExpression,
    WatchExpressionInWindow,
    RemoveWatchExpression,
    WatchModelUpdate,
    UseToolTips,
    AssignValue,
    AssignType,
    ExpandItem,
    CollapseItem,

    RecheckDebuggingHelpers,
    UseDebuggingHelpers,
    UseCustomDebuggingHelperLocation,
    CustomDebuggingHelperLocation,
    DebugDebuggingHelpers,

    // Source List
    ListSourceFiles,

    // Running
    SkipKnownFrames,

    // Breakpoints
    SynchronizeBreakpoints,
    AllPluginBreakpoints,
    SelectedPluginBreakpoints,
    NoPluginBreakpoints,
    SelectedPluginBreakpointsPattern,

    // Registers
    FormatHexadecimal,
    FormatDecimal,
    FormatOctal,
    FormatBinary,
    FormatRaw,
    FormatNatural,
};

// singleton access
Core::Utils::SavedAction *theDebuggerAction(int code);

// convienience
bool theDebuggerBoolSetting(int code);
QString theDebuggerStringSetting(int code);

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_WATCHWINDOW_H
