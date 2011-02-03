/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_ACTIONS_H
#define DEBUGGER_ACTIONS_H

#include <QtCore/QHash>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {
class SavedAction;
}

namespace Debugger {
namespace Internal {

class DebuggerSettings : public QObject
{
    Q_OBJECT // For tr().
public:
    explicit DebuggerSettings(QSettings *setting);
    ~DebuggerSettings();

    void insertItem(int code, Utils::SavedAction *item);
    Utils::SavedAction *item(int code) const;

    QString dump() const;

    void readSettings();
    void writeSettings() const;

private:
    QHash<int, Utils::SavedAction *> m_items;
    QSettings *m_settings;
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
    CloseBuffersOnExit,
    SwitchModeOnExit,

    UseDebuggingHelpers,
    UseCustomDebuggingHelperLocation,
    CustomDebuggingHelperLocation,

    UseCodeModel,
    ShowThreadNames,

    UseToolTipsInMainEditor,
    UseToolTipsInLocalsView,
    UseToolTipsInBreakpointsView,
    UseToolTipsInStackView,
    UseAddressInBreakpointsView,
    UseAddressInStackView,

    RegisterForPostMortem,

    // Gdb
    LoadGdbInit,
    GdbScriptFile,
    ExecuteCommand,
    GdbWatchdogTimeout,
    TargetAsync,

    // Stack
    MaximalStackDepth,
    ExpandStack,
    CreateFullBacktrace,

    // Watchers & Locals
    ShowStdNamespace,
    ShowQtNamespace,
    SortStructMembers,
    AutoDerefPointers,

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
    BreakOnCatch
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_WATCHWINDOW_H
