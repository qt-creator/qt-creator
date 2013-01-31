/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggeractions.h"
#include "debuggerconstants.h"

#ifdef Q_OS_WIN
#include "registerpostmortemaction.h"
#endif

#include <utils/savedaction.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QDebug>
#include <QVariant>
#include <QSettings>

using namespace Utils;

static const char debugModeSettingsGroupC[] = "DebugMode";
static const char sourcePathMappingArrayNameC[] = "SourcePathMappings";
static const char sourcePathMappingSourceKeyC[] = "Source";
static const char sourcePathMappingTargetKeyC[] = "Target";

namespace Debugger {
namespace Internal {

void GlobalDebuggerOptions::toSettings(QSettings *s) const
{
    s->beginWriteArray(QLatin1String(sourcePathMappingArrayNameC));
    if (!sourcePathMap.isEmpty()) {
        const QString sourcePathMappingSourceKey = QLatin1String(sourcePathMappingSourceKeyC);
        const QString sourcePathMappingTargetKey = QLatin1String(sourcePathMappingTargetKeyC);
        int i = 0;
        const SourcePathMap::const_iterator cend = sourcePathMap.constEnd();
        for (SourcePathMap::const_iterator it = sourcePathMap.constBegin(); it != cend; ++it, ++i) {
            s->setArrayIndex(i);
            s->setValue(sourcePathMappingSourceKey, it.key());
            s->setValue(sourcePathMappingTargetKey, it.value());
        }
    }
    s->endArray();
}

void GlobalDebuggerOptions::fromSettings(QSettings *s)
{
    sourcePathMap.clear();
    if (const int count = s->beginReadArray(QLatin1String(sourcePathMappingArrayNameC))) {
        const QString sourcePathMappingSourceKey = QLatin1String(sourcePathMappingSourceKeyC);
        const QString sourcePathMappingTargetKey = QLatin1String(sourcePathMappingTargetKeyC);
        for (int i = 0; i < count; ++i) {
             s->setArrayIndex(i);
             sourcePathMap.insert(s->value(sourcePathMappingSourceKey).toString(),
                                  s->value(sourcePathMappingTargetKey).toString());
        }
    }
    s->endArray();
}

//////////////////////////////////////////////////////////////////////////
//
// DebuggerSettings
//
//////////////////////////////////////////////////////////////////////////

DebuggerSettings::DebuggerSettings(QSettings *settings)
{
    m_settings = settings;
    const QString debugModeGroup = QLatin1String(debugModeSettingsGroupC);

    SavedAction *item = 0;

    item = new SavedAction(this);
    insertItem(SettingsDialog, item);
    item->setText(tr("Debugger Properties..."));

    //
    // View
    //
    item = new SavedAction(this);
    item->setText(tr("Always Adjust Column Widths to Contents"));
    item->setCheckable(true);
    item->setValue(false);
    item->setDefaultValue(false);
    item->setSettingsKey(debugModeGroup,
        QLatin1String("AlwaysAdjustLocalsColumnWidths"));
    insertItem(AlwaysAdjustLocalsColumnWidths, item);

    item = new SavedAction(this);
    item->setText(tr("Always Adjust Column Widths to Contents"));
    item->setCheckable(true);
    item->setValue(false);
    item->setDefaultValue(false);
    item->setSettingsKey(debugModeGroup,
        QLatin1String("AlwaysAdjustStackColumnWidths"));
    insertItem(AlwaysAdjustStackColumnWidths, item);

    item = new SavedAction(this);
    item->setText(tr("Always Adjust Column Widths to Contents"));
    item->setCheckable(true);
    item->setValue(false);
    item->setDefaultValue(false);
    item->setSettingsKey(debugModeGroup,
        QLatin1String("AlwaysAdjustThreadsColumnWidths"));
    insertItem(AlwaysAdjustThreadsColumnWidths, item);

    item = new SavedAction(this);
    item->setText(tr("Always Adjust Column Widths to Contents"));
    item->setCheckable(true);
    item->setValue(false);
    item->setDefaultValue(false);
    item->setSettingsKey(debugModeGroup,
        QLatin1String("AlwaysAdjustRegistersColumnWidths"));
    insertItem(AlwaysAdjustRegistersColumnWidths, item);

    item = new SavedAction(this);
    item->setText(tr("Always Adjust Column Widths to Contents"));
    item->setCheckable(true);
    item->setValue(false);
    item->setDefaultValue(false);
    item->setSettingsKey(debugModeGroup,
        QLatin1String("AlwaysAdjustSnapshotsColumnWidths"));
    insertItem(AlwaysAdjustSnapshotsColumnWidths, item);

    item = new SavedAction(this);
    item->setText(tr("Always Adjust Column Widths to Contents"));
    item->setCheckable(true);
    item->setValue(false);
    item->setDefaultValue(false);
    item->setSettingsKey(debugModeGroup,
        QLatin1String("AlwaysAdjustBreakpointsColumnWidths"));
    insertItem(AlwaysAdjustBreakpointsColumnWidths, item);

    item = new SavedAction(this);
    item->setText(tr("Always Adjust Column Widths to Contents"));
    item->setCheckable(true);
    item->setValue(false);
    item->setDefaultValue(false);
    item->setSettingsKey(debugModeGroup,
        QLatin1String("AlwaysAdjustModulesColumnWidths"));
    insertItem(AlwaysAdjustModulesColumnWidths, item);

    //Needed by QML Inspector
    item = new SavedAction(this);
    item->setText(tr("Use Alternating Row Colors"));
    item->setSettingsKey(debugModeGroup, QLatin1String("UseAlternatingRowColours"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(UseAlternatingRowColors, item);

    item = new SavedAction(this);
    item->setText(tr("Debugger Font Size Follows Main Editor"));
    item->setSettingsKey(debugModeGroup, QLatin1String("FontSizeFollowsEditor"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(FontSizeFollowsEditor, item);

    item = new SavedAction(this);
    item->setText(tr("Show a Message Box When Receiving a Signal"));
    item->setSettingsKey(debugModeGroup, QLatin1String("UseMessageBoxForSignals"));
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setValue(true);
    insertItem(UseMessageBoxForSignals, item);

    item = new SavedAction(this);
    item->setText(tr("Log Time Stamps"));
    item->setSettingsKey(debugModeGroup, QLatin1String("LogTimeStamps"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(LogTimeStamps, item);

    item = new SavedAction(this);
    item->setText(tr("Verbose Log"));
    item->setSettingsKey(debugModeGroup, QLatin1String("VerboseLog"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(VerboseLog, item);

    item = new SavedAction(this);
    item->setText(tr("Operate by Instruction"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    item->setIcon(QIcon(QLatin1String(":/debugger/images/debugger_singleinstructionmode.png")));
    item->setToolTip(tr("This switches the debugger to instruction-wise "
        "operation mode. In this mode, stepping operates on single "
        "instructions and the source location view also shows the "
        "disassembled instructions."));
    item->setIconVisibleInMenu(false);
    insertItem(OperateByInstruction, item);

    item = new SavedAction(this);
    item->setText(tr("Dereference Pointers Automatically"));
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setSettingsKey(debugModeGroup, QLatin1String("AutoDerefPointers"));
    item->setToolTip(tr("This switches the Locals&&Watchers view to "
        "automatically dereference pointers. This saves a level in the "
        "tree view, but also loses data for the now-missing intermediate "
        "level."));
    insertItem(AutoDerefPointers, item);

    //
    // Locals & Watchers
    //
    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("ShowStandardNamespace"));
    item->setText(tr("Show \"std::\" Namespace in Types"));
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setValue(true);
    insertItem(ShowStdNamespace, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("ShowQtNamespace"));
    item->setText(tr("Show Qt's Namespace in Types"));
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setValue(true);
    insertItem(ShowQtNamespace, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("SortStructMembers"));
    item->setText(tr("Sort Members of Classes and Structs Alphabetically"));
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setValue(true);
    insertItem(SortStructMembers, item);

    //
    // DebuggingHelper
    //
    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseDebuggingHelper"));
    item->setText(tr("Use Debugging Helpers"));
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setValue(true);
    insertItem(UseDebuggingHelpers, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseCodeModel"));
    item->setText(tr("Use Code Model"));
    item->setToolTip(tr("Selecting this causes the C++ Code Model being asked "
      "for variable scope information. This might result in slightly faster "
      "debugger operation but may fail for optimized code."));
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setValue(true);
    insertItem(UseCodeModel, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("ShowThreadNames"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    item->setValue(false);
    insertItem(ShowThreadNames, item);


    //
    // Breakpoints
    //
    item = new SavedAction(this);
    item->setText(tr("Synchronize Breakpoints"));
    insertItem(SynchronizeBreakpoints, item);

    item = new SavedAction(this);
    item->setText(tr("Adjust Breakpoint Locations"));
    item->setToolTip(tr("Not all source code lines generate "
      "executable code. Putting a breakpoint on such a line acts as "
      "if the breakpoint was set on the next line that generated code. "
      "Selecting 'Adjust Breakpoint Locations' shifts the red "
      "breakpoint markers in such cases to the location of the true "
      "breakpoint."));
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setValue(true);
    item->setSettingsKey(debugModeGroup, QLatin1String("AdjustBreakpointLocations"));
    insertItem(AdjustBreakpointLocations, item);

    item = new SavedAction(this);
    item->setText(tr("Break on \"throw\""));
    item->setCheckable(true);
    item->setDefaultValue(false);
    item->setValue(false);
    item->setSettingsKey(debugModeGroup, QLatin1String("BreakOnThrow"));
    insertItem(BreakOnThrow, item);

    item = new SavedAction(this);
    item->setText(tr("Break on \"catch\""));
    item->setCheckable(true);
    item->setDefaultValue(false);
    item->setValue(false);
    item->setSettingsKey(debugModeGroup, QLatin1String("BreakOnCatch"));
    insertItem(BreakOnCatch, item);

    item = new SavedAction(this);
    item->setText(tr("Break on \"qWarning\""));
    item->setCheckable(true);
    item->setDefaultValue(false);
    item->setValue(false);
    item->setSettingsKey(debugModeGroup, QLatin1String("BreakOnWarning"));
    insertItem(BreakOnWarning, item);

    item = new SavedAction(this);
    item->setText(tr("Break on \"qFatal\""));
    item->setCheckable(true);
    item->setDefaultValue(false);
    item->setValue(false);
    item->setSettingsKey(debugModeGroup, QLatin1String("BreakOnFatal"));
    insertItem(BreakOnFatal, item);

    item = new SavedAction(this);
    item->setText(tr("Break on \"abort\""));
    item->setCheckable(true);
#ifdef Q_OS_WIN
    item->setDefaultValue(true);
    item->setValue(true);
#else
    item->setDefaultValue(false);
    item->setValue(false);
#endif
    item->setSettingsKey(debugModeGroup, QLatin1String("BreakOnAbort"));
    insertItem(BreakOnAbort, item);

    //
    // Settings
    //

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("LoadGdbInit"));
    item->setDefaultValue(QString());
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setValue(true);
    insertItem(LoadGdbInit, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("AutoEnrichParameters"));
    item->setDefaultValue(QString());
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setValue(true);
    insertItem(AutoEnrichParameters, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseDynamicType"));
    item->setText(tr("Use Dynamic Object Type for Display"));
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setValue(true);
    insertItem(UseDynamicType, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("TargetAsync"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    item->setValue(false);
    insertItem(TargetAsync, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("WarnOnReleaseBuilds"));
    item->setCheckable(true);
    item->setDefaultValue(true);
    insertItem(WarnOnReleaseBuilds, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("GdbStartupCommands"));
    item->setDefaultValue(QString());
    insertItem(GdbStartupCommands, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("GdbPostAttachCommands"));
    item->setDefaultValue(QString());
    insertItem(GdbPostAttachCommands, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("CloseBuffersOnExit"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(CloseBuffersOnExit, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("SwitchModeOnExit"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(SwitchModeOnExit, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("BreakpointsFullPath"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(BreakpointsFullPathByDefault, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("RaiseOnInterrupt"));
    item->setCheckable(true);
    item->setDefaultValue(true);
    insertItem(RaiseOnInterrupt, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("AutoQuit"));
    item->setText(tr("Automatically Quit Debugger"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(AutoQuit, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("AttemptQuickStart"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(AttemptQuickStart, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("MultiInferior"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(MultiInferior, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("IntelFlavor"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(IntelFlavor, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseToolTips"));
    item->setText(tr("Use tooltips in main editor when debugging"));
    item->setToolTip(tr("Checking this will enable tooltips for variable "
        "values during debugging. Since this can slow down debugging and "
        "does not provide reliable information as it does not use scope "
        "information, it is switched off by default."));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(UseToolTipsInMainEditor, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseToolTipsInLocalsView"));
    item->setText(tr("Use Tooltips in Locals View when Debugging"));
    item->setToolTip(tr("Checking this will enable tooltips in the locals "
        "view during debugging."));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(UseToolTipsInLocalsView, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseToolTipsInBreakpointsView"));
    item->setText(tr("Use Tooltips in Breakpoints View when Debugging"));
    item->setToolTip(tr("Checking this will enable tooltips in the breakpoints "
        "view during debugging."));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(UseToolTipsInBreakpointsView, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseAddressInBreakpointsView"));
    item->setText(tr("Show Address Data in Breakpoints View when Debugging"));
    item->setToolTip(tr("Checking this will show a column with address "
        "information in the breakpoint view during debugging."));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(UseAddressInBreakpointsView, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseAddressInStackView"));
    item->setText(tr("Show Address Data in Stack View when Debugging"));
    item->setToolTip(tr("Checking this will show a column with address "
        "information in the stack view during debugging."));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(UseAddressInStackView, item);
    item = new SavedAction(this);

    item->setSettingsKey(debugModeGroup, QLatin1String("ListSourceFiles"));
    item->setText(tr("List Source Files"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(ListSourceFiles, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("SkipKnownFrames"));
    item->setText(tr("Skip Known Frames"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(SkipKnownFrames, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("EnableReverseDebugging"));
    item->setText(tr("Enable Reverse Debugging"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(EnableReverseDebugging, item);

#ifdef Q_OS_WIN
    item = new RegisterPostMortemAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("RegisterForPostMortem"));
    item->setText(tr("Register For Post-Mortem Debugging"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(RegisterForPostMortem, item);
#endif

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("AllPluginBreakpoints"));
    item->setDefaultValue(true);
    insertItem(AllPluginBreakpoints, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("SelectedPluginBreakpoints"));
    item->setDefaultValue(false);
    insertItem(SelectedPluginBreakpoints, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("NoPluginBreakpoints"));
    item->setDefaultValue(false);
    insertItem(NoPluginBreakpoints, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("SelectedPluginBreakpointsPattern"));
    item->setDefaultValue(QLatin1String(".*"));
    insertItem(SelectedPluginBreakpointsPattern, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("MaximalStackDepth"));
    item->setDefaultValue(20);
    insertItem(MaximalStackDepth, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("MaximalStringLength"));
    item->setDefaultValue(10000);
    insertItem(MaximalStringLength, item);

    item = new SavedAction(this);
    item->setText(tr("Reload Full Stack"));
    insertItem(ExpandStack, item);

    item = new SavedAction(this);
    item->setText(tr("Create Full Backtrace"));
    insertItem(CreateFullBacktrace, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("WatchdogTimeout"));
    item->setDefaultValue(20);
    insertItem(GdbWatchdogTimeout, item);

    //
    // QML Tools
    //
    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("ShowQmlObjectTree"));
    item->setDefaultValue(true);
    insertItem(ShowQmlObjectTree, item);

    const QString qmlInspectorGroup = QLatin1String("QML.Inspector");
    item = new SavedAction(this);
    item->setSettingsKey(qmlInspectorGroup, QLatin1String("QmlInspector.ShowAppOnTop"));
    item->setDefaultValue(false);
    insertItem(ShowAppOnTop, item);

    item = new SavedAction(this);
    item->setSettingsKey(qmlInspectorGroup, QLatin1String("QmlInspector.FromQml"));
    item->setDefaultValue(false);
    insertItem(QmlUpdateOnSave, item);
}

DebuggerSettings::~DebuggerSettings()
{
    qDeleteAll(m_items);
}

void DebuggerSettings::insertItem(int code, SavedAction *item)
{
    QTC_ASSERT(!m_items.contains(code),
        qDebug() << code << item->toString(); return);
    QTC_ASSERT(item->settingsKey().isEmpty() || item->defaultValue().isValid(),
        qDebug() << "NO DEFAULT VALUE FOR " << item->settingsKey());
    m_items[code] = item;
}

void DebuggerSettings::readSettings()
{
    foreach (SavedAction *item, m_items)
        item->readSettings(m_settings);
}

void DebuggerSettings::writeSettings() const
{
    foreach (SavedAction *item, m_items)
        item->writeSettings(m_settings);
}

SavedAction *DebuggerSettings::item(int code) const
{
    QTC_ASSERT(m_items.value(code, 0), qDebug() << "CODE: " << code; return 0);
    return m_items.value(code, 0);
}

QString DebuggerSettings::dump() const
{
    QString out;
    QTextStream ts(&out);
    ts << "Debugger settings: ";
    foreach (SavedAction *item, m_items) {
        QString key = item->settingsKey();
        if (!key.isEmpty()) {
            const QString current = item->value().toString();
            const QString default_ = item->defaultValue().toString();
            ts << '\n' << key << ": " << current
               << "  (default: " << default_ << ')';
            if (current != default_)
                ts <<  "  ***";
        }
    }
    return out;
}

} // namespace Internal
} // namespace Debugger

