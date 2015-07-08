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

#include "debuggeractions.h"

#ifdef Q_OS_WIN
#include "registerpostmortemaction.h"
#endif

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <utils/savedaction.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QSettings>

using namespace Utils;

static const char debugModeSettingsGroupC[] = "DebugMode";
static const char cdbSettingsGroupC[] = "CDB2";
static const char sourcePathMappingArrayNameC[] = "SourcePathMappings";
static const char sourcePathMappingSourceKeyC[] = "Source";
static const char sourcePathMappingTargetKeyC[] = "Target";

namespace Debugger {
namespace Internal {

void GlobalDebuggerOptions::toSettings() const
{
    QSettings *s = Core::ICore::settings();
    s->beginWriteArray(QLatin1String(sourcePathMappingArrayNameC));
    if (!sourcePathMap.isEmpty() || !sourcePathRegExpMap.isEmpty()) {
        const QString sourcePathMappingSourceKey = QLatin1String(sourcePathMappingSourceKeyC);
        const QString sourcePathMappingTargetKey = QLatin1String(sourcePathMappingTargetKeyC);
        int i = 0;
        for (auto it = sourcePathMap.constBegin(), cend = sourcePathMap.constEnd();
             it != cend;
             ++it, ++i) {
            s->setArrayIndex(i);
            s->setValue(sourcePathMappingSourceKey, it.key());
            s->setValue(sourcePathMappingTargetKey, it.value());
        }
        for (auto it = sourcePathRegExpMap.constBegin(), cend = sourcePathRegExpMap.constEnd();
             it != cend;
             ++it, ++i) {
            s->setArrayIndex(i);
            s->setValue(sourcePathMappingSourceKey, it->first.pattern());
            s->setValue(sourcePathMappingTargetKey, it->second);
        }
    }
    s->endArray();
}

void GlobalDebuggerOptions::fromSettings()
{
    QSettings *s = Core::ICore::settings();
    sourcePathMap.clear();
    if (const int count = s->beginReadArray(QLatin1String(sourcePathMappingArrayNameC))) {
        const QString sourcePathMappingSourceKey = QLatin1String(sourcePathMappingSourceKeyC);
        const QString sourcePathMappingTargetKey = QLatin1String(sourcePathMappingTargetKeyC);
        for (int i = 0; i < count; ++i) {
             s->setArrayIndex(i);
             const QString key = s->value(sourcePathMappingSourceKey).toString();
             const QString value = s->value(sourcePathMappingTargetKey).toString();
             if (key.startsWith(QLatin1Char('(')))
                 sourcePathRegExpMap.append(qMakePair(QRegExp(key), value));
             else
                 sourcePathMap.insert(key, value);
        }
    }
    s->endArray();
}

//////////////////////////////////////////////////////////////////////////
//
// DebuggerSettings
//
//////////////////////////////////////////////////////////////////////////

DebuggerSettings::DebuggerSettings()
{
    const QString debugModeGroup = QLatin1String(debugModeSettingsGroupC);
    const QString cdbSettingsGroup = QLatin1String(cdbSettingsGroupC);

    SavedAction *item = 0;

    item = new SavedAction(this);
    insertItem(SettingsDialog, item);
    item->setText(tr("Configure Debugger..."));

    //
    // View
    //
    item = new SavedAction(this);
    item->setText(tr("Always Adjust View Column Widths to Contents"));
    item->setCheckable(true);
    item->setValue(true);
    item->setDefaultValue(true);
    item->setSettingsKey(debugModeGroup,
        QLatin1String("AlwaysAdjustColumnWidths"));
    insertItem(AlwaysAdjustColumnWidths, item);

    // Needed by QML Inspector
    item = new SavedAction(this);
    item->setText(tr("Use Alternating Row Colors"));
    item->setSettingsKey(debugModeGroup, QLatin1String("UseAlternatingRowColours"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(UseAlternatingRowColors, item);

    item = new SavedAction(this);
    item->setText(tr("Keep Editor Stationary When Stepping"));
    item->setSettingsKey(debugModeGroup, QLatin1String("StationaryEditorWhileStepping"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(StationaryEditorWhileStepping, item);

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
    item->setToolTip(tr("<p>This switches the debugger to instruction-wise "
        "operation mode. In this mode, stepping operates on single "
        "instructions and the source location view also shows the "
        "disassembled instructions."));
    item->setIconVisibleInMenu(false);
    insertItem(OperateByInstruction, item);

    item = new SavedAction(this);
    item->setText(tr("Native Mixed Mode"));
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setIcon(QIcon(QLatin1String(Core::Constants::ICON_LINK)));
    item->setToolTip(tr("<p>This switches the debugger to native-mixed "
        "operation mode. In this mode, stepping and data display will "
        "be handled by the native debugger backend (GDB, LLDB or CDB) "
        "for C++, QML and JS sources."));
    item->setIconVisibleInMenu(false);
    insertItem(OperateNativeMixed, item);

    item = new SavedAction(this);
    item->setText(tr("Dereference Pointers Automatically"));
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setSettingsKey(debugModeGroup, QLatin1String("AutoDerefPointers"));
    item->setToolTip(tr("<p>This switches the Locals&&Watchers view to "
        "automatically dereference pointers. This saves a level in the "
        "tree view, but also loses data for the now-missing intermediate "
        "level."));
    insertItem(AutoDerefPointers, item);

    //
    // Cdb Options
    //

    item = new SavedAction(this);
    item->setDefaultValue(QString());
    item->setSettingsKey(cdbSettingsGroup, QLatin1String("AdditionalArguments"));
    insertItem(CdbAdditionalArguments, item);

    item = new SavedAction(this);
    item->setDefaultValue(QStringList());
    item->setSettingsKey(cdbSettingsGroup, QLatin1String("SymbolPaths"));
    insertItem(CdbSymbolPaths, item);

    item = new SavedAction(this);
    item->setDefaultValue(QStringList());
    item->setSettingsKey(cdbSettingsGroup, QLatin1String("SourcePaths"));
    insertItem(CdbSourcePaths, item);

    item = new SavedAction(this);
    item->setDefaultValue(QStringList());
    item->setSettingsKey(cdbSettingsGroup, QLatin1String("BreakEvent"));
    insertItem(CdbBreakEvents, item);

    item = new SavedAction(this);
    item->setCheckable(true);
    item->setDefaultValue(false);
    item->setSettingsKey(cdbSettingsGroup, QLatin1String("BreakOnCrtDbgReport"));
    insertItem(CdbBreakOnCrtDbgReport, item);

    item = new SavedAction(this);
    item->setCheckable(true);
    item->setDefaultValue(false);
    item->setSettingsKey(cdbSettingsGroup, QLatin1String("CDB_Console"));
    insertItem(UseCdbConsole, item);

    item = new SavedAction(this);
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setSettingsKey(cdbSettingsGroup, QLatin1String("BreakpointCorrection"));
    insertItem(CdbBreakPointCorrection, item);

    item = new SavedAction(this);
    item->setCheckable(true);
    item->setDefaultValue(false);
    item->setSettingsKey(cdbSettingsGroup, QLatin1String("IgnoreFirstChanceAccessViolation"));
    insertItem(IgnoreFirstChanceAccessViolation, item);

    //
    // Locals & Watchers
    //
    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("ShowStandardNamespace"));
    item->setText(tr("Show \"std::\" Namespace in Types"));
    item->setDialogText(tr("Show \"std::\" namespace in types"));
    item->setToolTip(tr("<p>Shows \"std::\" prefix for types from the standard library."));
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setValue(true);
    insertItem(ShowStdNamespace, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("ShowQtNamespace"));
    item->setText(tr("Show Qt's Namespace in Types"));
    item->setDialogText(tr("Show Qt's namespace in types"));
    item->setToolTip(tr("<p>Shows Qt namespace prefix for Qt types. This is only "
                        "relevant if Qt was configured with \"-qtnamespace\"."));
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setValue(true);
    insertItem(ShowQtNamespace, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("SortStructMembers"));
    item->setText(tr("Sort Members of Classes and Structs Alphabetically"));
    item->setDialogText(tr("Sort members of classes and structs alphabetically"));
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
    item->setDialogText(tr("Use code model"));
    item->setToolTip(tr("<p>Selecting this causes the C++ Code Model being asked "
      "for variable scope information. This might result in slightly faster "
      "debugger operation but may fail for optimized code."));
    item->setCheckable(true);
    item->setDefaultValue(true);
    item->setValue(true);
    insertItem(UseCodeModel, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("ShowThreadNames"));
    item->setToolTip(tr("<p>Displays names of QThread based threads."));
    item->setDialogText(tr("Display thread names"));
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
    item->setToolTip(tr("<p>Not all source code lines generate "
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
    item->setDefaultValue(false);
    item->setValue(false);
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
    item->setSettingsKey(debugModeGroup, QLatin1String("LoadGdbDumpers2"));
    item->setDefaultValue(QString());
    item->setCheckable(true);
    item->setDefaultValue(false);
    item->setValue(false);
    insertItem(LoadGdbDumpers, item);

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
    item->setSettingsKey(debugModeGroup, QLatin1String("GdbCustomDumperCommands"));
    item->setDefaultValue(QString());
    insertItem(ExtraDumperCommands, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("ExtraDumperFile"));
    item->setDefaultValue(QString());
    insertItem(ExtraDumperFile, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("GdbPostAttachCommands"));
    item->setDefaultValue(QString());
    insertItem(GdbPostAttachCommands, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("CloseBuffersOnExit"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(CloseSourceBuffersOnExit, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("CloseMemoryBuffersOnExit"));
    item->setCheckable(true);
    item->setDefaultValue(true);
    insertItem(CloseMemoryBuffersOnExit, item);

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
    item->setSettingsKey(debugModeGroup, QLatin1String("IdentifyDebugInfoPackages"));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(IdentifyDebugInfoPackages, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseToolTips"));
    item->setText(tr("Use tooltips in main editor when debugging"));
    item->setToolTip(tr("<p>Checking this will enable tooltips for variable "
        "values during debugging. Since this can slow down debugging and "
        "does not provide reliable information as it does not use scope "
        "information, it is switched off by default."));
    item->setCheckable(true);
    item->setDefaultValue(true);
    insertItem(UseToolTipsInMainEditor, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseToolTipsInLocalsView"));
    item->setText(tr("Use Tooltips in Locals View when Debugging"));
    item->setToolTip(tr("<p>Checking this will enable tooltips in the locals "
        "view during debugging."));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(UseToolTipsInLocalsView, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseToolTipsInBreakpointsView"));
    item->setText(tr("Use Tooltips in Breakpoints View when Debugging"));
    item->setToolTip(tr("<p>Checking this will enable tooltips in the breakpoints "
        "view during debugging."));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(UseToolTipsInBreakpointsView, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseToolTipsInBreakpointsView"));
    item->setText(tr("Use Tooltips in Stack View when Debugging"));
    item->setToolTip(tr("<p>Checking this will enable tooltips in the stack "
        "view during debugging."));
    item->setCheckable(true);
    item->setDefaultValue(true);
    insertItem(UseToolTipsInStackView, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseAddressInBreakpointsView"));
    item->setText(tr("Show Address Data in Breakpoints View when Debugging"));
    item->setToolTip(tr("<p>Checking this will show a column with address "
        "information in the breakpoint view during debugging."));
    item->setCheckable(true);
    item->setDefaultValue(false);
    insertItem(UseAddressInBreakpointsView, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("UseAddressInStackView"));
    item->setText(tr("Show Address Data in Stack View when Debugging"));
    item->setToolTip(tr("<p>Checking this will show a column with address "
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
    item->setSettingsKey(debugModeGroup, QLatin1String("DisplayStringLimit"));
    item->setToolTip(tr("<p>The maximum length of string entries in the "
        "Locals and Expressions pane. Longer than that are cut off "
        "and displayed with an ellipsis attached."));
    item->setDefaultValue(100);
    insertItem(DisplayStringLimit, item);

    item = new SavedAction(this);
    item->setSettingsKey(debugModeGroup, QLatin1String("MaximalStringLength"));
    item->setToolTip(tr("<p>The maximum length for strings in separated windows. "
        "Longer strings are cut off and displayed with an ellipsis attached."));
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
    QSettings *settings = Core::ICore::settings();
    foreach (SavedAction *item, m_items)
        item->readSettings(settings);
}

void DebuggerSettings::writeSettings() const
{
    QSettings *settings = Core::ICore::settings();
    foreach (SavedAction *item, m_items)
        item->writeSettings(settings);
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

