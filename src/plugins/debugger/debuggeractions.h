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
#include <QtCore/QMap>

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
    typedef QMultiMap<QString, int> GdbBinaryToolChainMap;

    explicit DebuggerSettings(QObject *parent = 0);
    ~DebuggerSettings();

    GdbBinaryToolChainMap gdbBinaryToolChainMap() const
        { return m_gdbBinaryToolChainMap; }
    void setGdbBinaryToolChainMap(const GdbBinaryToolChainMap &map)
        { m_gdbBinaryToolChainMap = map; }

    void insertItem(int code, Utils::SavedAction *item);
    Utils::SavedAction *item(int code) const;

    QString dump() const;

    static DebuggerSettings *instance();

public slots:
    void readSettings(QSettings *settings);
    void writeSettings(QSettings *settings) const;

private:
    QHash<int, Utils::SavedAction *> m_items;
    GdbBinaryToolChainMap m_gdbBinaryToolChainMap;
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
    GdbEnvironment,
    GdbScriptFile,
    ExecuteCommand,
    GdbWatchdogTimeout,

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

// singleton access
Utils::SavedAction *theDebuggerAction(int code);

// convenience
bool theDebuggerBoolSetting(int code);
QString theDebuggerStringSetting(int code);

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_WATCHWINDOW_H
