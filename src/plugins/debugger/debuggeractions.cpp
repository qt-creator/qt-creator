// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggeractions.h"

#include "commonoptionspage.h"
#include "debuggericons.h"
#include "debuggerinternalconstants.h"
#include "debuggertr.h"

#ifdef Q_OS_WIN
#include "registerpostmortemaction.h"
#endif

#include <app/app_version.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDebug>

using namespace Utils;

namespace Debugger::Internal {

const char debugModeSettingsGroupC[] = "DebugMode";
const char cdbSettingsGroupC[] = "CDB2";

//////////////////////////////////////////////////////////////////////////
//
// DebuggerSettings
//
//////////////////////////////////////////////////////////////////////////

static DebuggerSettings *theDebuggerSettings_ = nullptr;

DebuggerSettings *debuggerSettings()
{
    QTC_CHECK(theDebuggerSettings_);
    return theDebuggerSettings_;
}

DebuggerSettings::DebuggerSettings()
{
    theDebuggerSettings_ = this;

    const QString debugModeGroup(debugModeSettingsGroupC);
    const QString cdbSettingsGroup(cdbSettingsGroupC);

    settingsDialog.setLabelText(Tr::tr("Configure Debugger..."));

    /*
    groupBoxPluginDebugging = new QGroupBox(q);
    groupBoxPluginDebugging->setTitle(Tr::tr(
        "Behavior of Breakpoint Setting in Plugins"));

    radioButtonAllPluginBreakpoints = new QRadioButton(groupBoxPluginDebugging);
    radioButtonAllPluginBreakpoints->setText(Tr::tr(
        "Always try to set breakpoints in plugins automatically"));
    radioButtonAllPluginBreakpoints->setToolTip(Tr::tr(
        "This is the slowest but safest option."));

    radioButtonSelectedPluginBreakpoints = new QRadioButton(groupBoxPluginDebugging);
    radioButtonSelectedPluginBreakpoints->setText(Tr::tr(
        "Try to set breakpoints in selected plugins"));

    radioButtonNoPluginBreakpoints = new QRadioButton(groupBoxPluginDebugging);
    radioButtonNoPluginBreakpoints->setText(Tr::tr(
        "Never set breakpoints in plugins automatically"));

    lineEditSelectedPluginBreakpointsPattern = new QLineEdit(groupBoxPluginDebugging);

    labelSelectedPluginBreakpoints = new QLabel(groupBoxPluginDebugging);
    labelSelectedPluginBreakpoints->setText(Tr::tr(
        "Matching regular expression: "));
    */

    //
    // View
    //
    alwaysAdjustColumnWidths.setLabelText(Tr::tr("Always Adjust View Column Widths to Contents"));
    alwaysAdjustColumnWidths.setSettingsKey(debugModeGroup, "AlwaysAdjustColumnWidths");
    alwaysAdjustColumnWidths.setDefaultValue(true);

    // Needed by QML Inspector
    //useAlternatingRowColors.setLabelText(Tr::tr("Use Alternating Row Colors"));
    useAlternatingRowColors.setSettingsKey(debugModeGroup, "UseAlternatingRowColours");
    useAlternatingRowColors.setLabelText(Tr::tr("Use alternating row colors in debug views"));

    stationaryEditorWhileStepping.setSettingsKey(debugModeGroup, "StationaryEditorWhileStepping");
    stationaryEditorWhileStepping.setLabelText(Tr::tr("Keep editor stationary when stepping"));
    stationaryEditorWhileStepping.setToolTip(Tr::tr("Scrolls the editor only when it is necessary "
                                                 "to keep the current line in view, "
                                                 "instead of keeping the next statement centered at "
                                                 "all times."));

    forceLoggingToConsole.setSettingsKey(debugModeGroup, "ForceLoggingToConsole");
    forceLoggingToConsole.setLabelText(Tr::tr("Force logging to console"));
    forceLoggingToConsole.setToolTip(Tr::tr("Sets QT_LOGGING_TO_CONSOLE=1 in the environment "
        "of the debugged program, preventing storing debug output "
        "in system logs."));

    fontSizeFollowsEditor.setSettingsKey(debugModeGroup, "FontSizeFollowsEditor");
    fontSizeFollowsEditor.setToolTip(Tr::tr("Changes the font size in the debugger views when "
                                        "the font size in the main editor changes."));
    fontSizeFollowsEditor.setLabelText(Tr::tr("Debugger font size follows main editor"));

    useMessageBoxForSignals.setSettingsKey(debugModeGroup, "UseMessageBoxForSignals");
    useMessageBoxForSignals.setDefaultValue(true);
    useMessageBoxForSignals.setLabelText(Tr::tr(
        "Show a message box when receiving a signal"));
    useMessageBoxForSignals.setToolTip(Tr::tr(
        "Displays a message box as soon as your application\n"
        "receives a signal like SIGSEGV during debugging."));

    logTimeStamps.setLabelText(Tr::tr("Log Time Stamps"));
    logTimeStamps.setSettingsKey(debugModeGroup, "LogTimeStamps");

    autoDerefPointers.setLabelText(Tr::tr("Dereference Pointers Automatically"));
    autoDerefPointers.setDefaultValue(true);
    autoDerefPointers.setSettingsKey(debugModeGroup, "AutoDerefPointers");
    autoDerefPointers.setToolTip(Tr::tr("<p>This switches the Locals and Expressions views to "
        "automatically dereference pointers. This saves a level in the "
        "tree view, but also loses data for the now-missing intermediate "
        "level."));

    //
    // Cdb Options
    //

    cdbAdditionalArguments.setSettingsKey(cdbSettingsGroup, "AdditionalArguments");
    cdbAdditionalArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    cdbAdditionalArguments.setLabelText(Tr::tr("Additional arguments:"));

    cdbSymbolPaths.setSettingsKey(cdbSettingsGroup, "SymbolPaths");
    cdbSourcePaths.setSettingsKey(cdbSettingsGroup, "SourcePaths");

    cdbBreakEvents.setSettingsKey(cdbSettingsGroup, "BreakEvent");
    cdbBreakOnCrtDbgReport.setSettingsKey(cdbSettingsGroup, "BreakOnCrtDbgReport");
    cdbBreakOnCrtDbgReport.setLabelText(
        CommonOptionsPage::msgSetBreakpointAtFunction(Constants::CRT_DEBUG_REPORT));
    cdbBreakOnCrtDbgReport.setToolTip(
        CommonOptionsPage::msgSetBreakpointAtFunctionToolTip(Constants::CRT_DEBUG_REPORT,
            Tr::tr("Catches runtime error messages caused by assert(), for example.")));

    useCdbConsole.setSettingsKey(cdbSettingsGroup, "CDB_Console");
    useCdbConsole.setToolTip("<html><head/><body><p>" + Tr::tr(
        "Uses CDB's native console for console applications. "
        "This overrides the setting in Environment > System. "
        "The native console does not prompt on application exit. "
        "It is suitable for diagnosing cases in which the application does not "
        "start up properly in the configured console and the subsequent attach fails.")
        + "</p></body></html>");
    useCdbConsole.setLabelText(Tr::tr("Use CDB &console"));

    cdbBreakPointCorrection.setSettingsKey(cdbSettingsGroup, "BreakpointCorrection");
    cdbBreakPointCorrection.setDefaultValue(true);
    cdbBreakPointCorrection.setToolTip("<html><head/><body><p>" + Tr::tr(
        "Attempts to correct the location of a breakpoint based on file and line number should"
        "it be in a comment or in a line for which no code is generated. "
        "The correction is based on the code model.") + "</p></body></html>");
    cdbBreakPointCorrection.setLabelText(Tr::tr("Correct breakpoint location"));

    cdbUsePythonDumper.setSettingsKey(cdbSettingsGroup, "UsePythonDumper");
    cdbUsePythonDumper.setDefaultValue(true);
    cdbUsePythonDumper.setLabelText(Tr::tr("Use Python dumper"));

    firstChanceExceptionTaskEntry.setSettingsKey(cdbSettingsGroup, "FirstChanceExceptionTaskEntry");
    firstChanceExceptionTaskEntry.setDefaultValue(true);
    firstChanceExceptionTaskEntry.setLabelText(Tr::tr("First chance exceptions"));

    secondChanceExceptionTaskEntry.setSettingsKey(cdbSettingsGroup, "SecondChanceExceptionTaskEntry");
    secondChanceExceptionTaskEntry.setDefaultValue(true);
    secondChanceExceptionTaskEntry.setLabelText(Tr::tr("Second chance exceptions"));

    ignoreFirstChanceAccessViolation.setSettingsKey(cdbSettingsGroup, "IgnoreFirstChanceAccessViolation");
    ignoreFirstChanceAccessViolation.setLabelText(Tr::tr("Ignore first chance access violations"));

    //
    // Locals & Watchers
    //
    showStdNamespace.setSettingsKey(debugModeGroup, "ShowStandardNamespace");
    showStdNamespace.setDefaultValue(true);
    showStdNamespace.setDisplayName(Tr::tr("Show \"std::\" Namespace in Types"));
    showStdNamespace.setLabelText(Tr::tr("Show \"std::\" namespace in types"));
    showStdNamespace.setToolTip(Tr::tr("<p>Shows \"std::\" prefix for types from the standard library."));

    showQtNamespace.setSettingsKey(debugModeGroup, "ShowQtNamespace");
    showQtNamespace.setDefaultValue(true);
    showQtNamespace.setDisplayName(Tr::tr("Show Qt's Namespace in Types"));
    showQtNamespace.setLabelText(Tr::tr("Show Qt's namespace in types"));
    showQtNamespace.setToolTip(Tr::tr("<p>Shows Qt namespace prefix for Qt types. This is only "
                        "relevant if Qt was configured with \"-qtnamespace\"."));

    showQObjectNames.setSettingsKey(debugModeGroup, "ShowQObjectNames2");
    showQObjectNames.setDefaultValue(true);
    showQObjectNames.setDisplayName(Tr::tr("Show QObject names if available"));
    showQObjectNames.setLabelText(Tr::tr("Show QObject names if available"));
    showQObjectNames.setToolTip(Tr::tr("<p>Displays the objectName property of QObject based items. "
                        "Note that this can negatively impact debugger performance "
                        "even if no QObjects are present."));

    sortStructMembers.setSettingsKey(debugModeGroup, "SortStructMembers");
    sortStructMembers.setDisplayName(Tr::tr("Sort Members of Classes and Structs Alphabetically"));
    sortStructMembers.setLabelText(Tr::tr("Sort members of classes and structs alphabetically"));
    sortStructMembers.setDefaultValue(true);

    //
    // DebuggingHelper
    //
    useDebuggingHelpers.setSettingsKey(debugModeGroup, "UseDebuggingHelper");
    useDebuggingHelpers.setDefaultValue(true);
    useDebuggingHelpers.setLabelText(Tr::tr("Use Debugging Helpers"));

    useCodeModel.setSettingsKey(debugModeGroup, "UseCodeModel");
    useCodeModel.setDefaultValue(true);
    useCodeModel.setLabelText(Tr::tr("Use code model"));
    useCodeModel.setToolTip(Tr::tr("<p>Selecting this causes the C++ Code Model being asked "
      "for variable scope information. This might result in slightly faster "
      "debugger operation but may fail for optimized code."));

    showThreadNames.setSettingsKey(debugModeGroup, "ShowThreadNames");
    showThreadNames.setLabelText(Tr::tr("Display thread names"));
    showThreadNames.setToolTip(Tr::tr("<p>Displays names of QThread based threads."));


    //
    // Breakpoints
    //
    synchronizeBreakpoints.setLabelText(Tr::tr("Synchronize Breakpoints"));

    adjustBreakpointLocations.setDisplayName(Tr::tr("Adjust Breakpoint Locations"));
    adjustBreakpointLocations.setToolTip(Tr::tr("<p>Not all source code lines generate "
      "executable code. Putting a breakpoint on such a line acts as "
      "if the breakpoint was set on the next line that generated code. "
      "Selecting 'Adjust Breakpoint Locations' shifts the red "
      "breakpoint markers in such cases to the location of the true "
      "breakpoint."));
    adjustBreakpointLocations.setDefaultValue(true);
    adjustBreakpointLocations.setSettingsKey(debugModeGroup, "AdjustBreakpointLocations");
    adjustBreakpointLocations.setLabelText(Tr::tr(
        "Adjust breakpoint locations"));
    adjustBreakpointLocations.setToolTip(Tr::tr(
        "GDB allows setting breakpoints on source lines for which no code \n"
        "was generated. In such situations the breakpoint is shifted to the\n"
        "next source code line for which code was actually generated.\n"
        "This option reflects such temporary change by moving the breakpoint\n"
        "markers in the source code editor."));


    breakOnThrow.setLabelText(Tr::tr("Break on \"throw\""));
    breakOnThrow.setSettingsKey(debugModeGroup, "BreakOnThrow");

    breakOnCatch.setLabelText(Tr::tr("Break on \"catch\""));
    breakOnCatch.setSettingsKey(debugModeGroup, "BreakOnCatch");

    breakOnWarning.setLabelText(Tr::tr("Break on \"qWarning\""));
    breakOnWarning.setSettingsKey(debugModeGroup, "BreakOnWarning");
    // FIXME: Move to common settings page.
    breakOnWarning.setLabelText(CommonOptionsPage::msgSetBreakpointAtFunction("qWarning"));
    breakOnWarning.setToolTip(CommonOptionsPage::msgSetBreakpointAtFunctionToolTip("qWarning"));

    breakOnFatal.setLabelText(Tr::tr("Break on \"qFatal\""));
    breakOnFatal.setSettingsKey(debugModeGroup, "BreakOnFatal");
    breakOnFatal.setLabelText(CommonOptionsPage::msgSetBreakpointAtFunction("qFatal"));
    breakOnFatal.setToolTip(CommonOptionsPage::msgSetBreakpointAtFunctionToolTip("qFatal"));

    breakOnAbort.setLabelText(Tr::tr("Break on \"abort\""));
    breakOnAbort.setSettingsKey(debugModeGroup, "BreakOnAbort");
    breakOnAbort.setLabelText(CommonOptionsPage::msgSetBreakpointAtFunction("abort"));
    breakOnAbort.setToolTip(CommonOptionsPage::msgSetBreakpointAtFunctionToolTip("abort"));

    //
    // Settings
    //

    loadGdbInit.setSettingsKey(debugModeGroup, "LoadGdbInit");
    loadGdbInit.setDefaultValue(true);
    loadGdbInit.setLabelText(Tr::tr("Load .gdbinit file on startup"));
    loadGdbInit.setToolTip(Tr::tr(
        "Allows or inhibits reading the user's default\n"
        ".gdbinit file on debugger startup."));

    loadGdbDumpers.setSettingsKey(debugModeGroup, "LoadGdbDumpers2");
    loadGdbDumpers.setLabelText(Tr::tr("Load system GDB pretty printers"));
    loadGdbDumpers.setToolTip(Tr::tr(
        "Uses the default GDB pretty printers installed in your "
        "system or linked to the libraries your application uses."));

    autoEnrichParameters.setSettingsKey(debugModeGroup, "AutoEnrichParameters");
    autoEnrichParameters.setDefaultValue(true);
    autoEnrichParameters.setLabelText(Tr::tr(
        "Use common locations for debug information"));
    autoEnrichParameters.setToolTip(Tr::tr(
        "<html><head/><body>Adds common paths to locations "
        "of debug information such as <i>/usr/src/debug</i> "
        "when starting GDB.</body></html>"));

    useDynamicType.setSettingsKey(debugModeGroup, "UseDynamicType");
    useDynamicType.setDefaultValue(true);
    useDynamicType.setDisplayName(Tr::tr("Use Dynamic Object Type for Display"));
    useDynamicType.setLabelText(Tr::tr(
        "Use dynamic object type for display"));
    useDynamicType.setToolTip(Tr::tr(
        "Specifies whether the dynamic or the static type of objects will be "
        "displayed. Choosing the dynamic type might be slower."));

    targetAsync.setSettingsKey(debugModeGroup, "TargetAsync");
    targetAsync.setLabelText(Tr::tr(
        "Use asynchronous mode to control the inferior"));

    warnOnReleaseBuilds.setSettingsKey(debugModeGroup, "WarnOnReleaseBuilds");
    warnOnReleaseBuilds.setDefaultValue(true);
    warnOnReleaseBuilds.setLabelText(Tr::tr("Warn when debugging \"Release\" builds"));
    warnOnReleaseBuilds.setToolTip(Tr::tr("Shows a warning when starting the debugger "
                                      "on a binary with insufficient debug information."));

    QString howToUsePython = Tr::tr(
        "<p>To execute simple Python commands, prefix them with \"python\".</p>"
        "<p>To execute sequences of Python commands spanning multiple lines "
        "prepend the block with \"python\" on a separate line, and append "
        "\"end\" on a separate line.</p>"
        "<p>To execute arbitrary Python scripts, "
        "use <i>python execfile('/path/to/script.py')</i>.</p>");

    gdbStartupCommands.setSettingsKey(debugModeGroup, "GdbStartupCommands");
    gdbStartupCommands.setDisplayStyle(StringAspect::TextEditDisplay);
    gdbStartupCommands.setUseGlobalMacroExpander();
    gdbStartupCommands.setToolTip("<html><head/><body><p>" + Tr::tr(
        "GDB commands entered here will be executed after "
        "GDB has been started, but before the debugged program is started or "
        "attached, and before the debugging helpers are initialized.") + "</p>"
        + howToUsePython + "</body></html>");

    gdbPostAttachCommands.setSettingsKey(debugModeGroup, "GdbPostAttachCommands");
    gdbPostAttachCommands.setDisplayStyle(StringAspect::TextEditDisplay);
    gdbPostAttachCommands.setUseGlobalMacroExpander();
    gdbPostAttachCommands.setToolTip("<html><head/><body><p>" + Tr::tr(
        "GDB commands entered here will be executed after "
        "GDB has successfully attached to remote targets.</p>"
        "<p>You can add commands to further set up the target here, "
        "such as \"monitor reset\" or \"load\".") + "</p>"
        + howToUsePython + "</body></html>");

    extraDumperCommands.setSettingsKey(debugModeGroup, "GdbCustomDumperCommands");
    extraDumperCommands.setDisplayStyle(StringAspect::TextEditDisplay);
    extraDumperCommands.setUseGlobalMacroExpander();
    extraDumperCommands.setToolTip("<html><head/><body><p>"
                        + Tr::tr("Python commands entered here will be executed after built-in "
                             "debugging helpers have been loaded and fully initialized. You can "
                             "load additional debugging helpers or modify existing ones here.")
                        + "</p></body></html>");

    extraDumperFile.setSettingsKey(debugModeGroup, "ExtraDumperFile");
    extraDumperFile.setDisplayStyle(StringAspect::PathChooserDisplay);
    extraDumperFile.setDisplayName(Tr::tr("Extra Debugging Helpers"));
    // Label text is intentional empty in the GUI.
    extraDumperFile.setToolTip(Tr::tr("Path to a Python file containing additional data dumpers."));

    const QString t = Tr::tr("Stopping and stepping in the debugger "
          "will automatically open views associated with the current location.") + '\n';

    closeSourceBuffersOnExit.setSettingsKey(debugModeGroup, "CloseBuffersOnExit");
    closeSourceBuffersOnExit.setLabelText(Tr::tr("Close temporary source views on debugger exit"));
    closeSourceBuffersOnExit.setToolTip(t + Tr::tr("Closes automatically opened source views when the debugger exits."));

    closeMemoryBuffersOnExit.setSettingsKey(debugModeGroup, "CloseMemoryBuffersOnExit");
    closeMemoryBuffersOnExit.setDefaultValue(true);
    closeMemoryBuffersOnExit.setLabelText(Tr::tr("Close temporary memory views on debugger exit"));
    closeMemoryBuffersOnExit.setToolTip(t + Tr::tr("Closes automatically opened memory views when the debugger exits."));

    switchModeOnExit.setSettingsKey(debugModeGroup, "SwitchModeOnExit");
    switchModeOnExit.setLabelText(Tr::tr("Switch to previous mode on debugger exit"));

    breakpointsFullPathByDefault.setSettingsKey(debugModeGroup, "BreakpointsFullPath");
    breakpointsFullPathByDefault.setToolTip(Tr::tr("Enables a full file path in breakpoints by default also for GDB."));
    breakpointsFullPathByDefault.setLabelText(Tr::tr("Set breakpoints using a full absolute path"));

    raiseOnInterrupt.setSettingsKey(debugModeGroup, "RaiseOnInterrupt");
    raiseOnInterrupt.setDefaultValue(true);
    raiseOnInterrupt.setLabelText(Tr::tr("Bring %1 to foreground when application interrupts")
                                  .arg(Core::Constants::IDE_DISPLAY_NAME));

    autoQuit.setSettingsKey(debugModeGroup, "AutoQuit");
    autoQuit.setLabelText(Tr::tr("Automatically Quit Debugger"));

    multiInferior.setSettingsKey(debugModeGroup, "MultiInferior");
    multiInferior.setLabelText(Tr::tr("Debug all child processes"));
    multiInferior.setToolTip(Tr::tr(
        "<html><head/><body>Keeps debugging all children after a fork."
        "</body></html>"));

    intelFlavor.setSettingsKey(debugModeGroup, "IntelFlavor");
    intelFlavor.setLabelText(Tr::tr("Use Intel style disassembly"));
    intelFlavor.setToolTip(Tr::tr("GDB shows by default AT&&T style disassembly."));

    useAnnotationsInMainEditor.setSettingsKey(debugModeGroup, "UseAnnotations");
    useAnnotationsInMainEditor.setLabelText(Tr::tr("Use annotations in main editor when debugging"));
    useAnnotationsInMainEditor.setToolTip(Tr::tr("<p>Shows simple variable values "
        "as annotations in the main editor during debugging."));
    useAnnotationsInMainEditor.setDefaultValue(true);

    usePseudoTracepoints.setSettingsKey(debugModeGroup, "UsePseudoTracepoints");
    usePseudoTracepoints.setLabelText(Tr::tr("Use pseudo message tracepoints"));
    usePseudoTracepoints.setToolTip(Tr::tr("Uses Python to extend the ordinary GDB breakpoint class."));
    usePseudoTracepoints.setDefaultValue(true);

    useIndexCache.setSettingsKey(debugModeGroup, "UseIndexCache");
    useIndexCache.setLabelText(Tr::tr("Use automatic symbol cache"));
    useIndexCache.setToolTip(Tr::tr("It is possible for GDB to automatically save a copy of "
        "its symbol index in a cache on disk and retrieve it from there when loading the same "
        "binary in the future."));
    useIndexCache.setDefaultValue(true);

    useToolTipsInMainEditor.setSettingsKey(debugModeGroup, "UseToolTips");
    useToolTipsInMainEditor.setLabelText(Tr::tr("Use tooltips in main editor when debugging"));
    useToolTipsInMainEditor.setToolTip(Tr::tr("<p>Enables tooltips for variable "
        "values during debugging. Since this can slow down debugging and "
        "does not provide reliable information as it does not use scope "
        "information, it is switched off by default."));
    useToolTipsInMainEditor.setDefaultValue(true);

    useToolTipsInLocalsView.setSettingsKey(debugModeGroup, "UseToolTipsInLocalsView");
    useToolTipsInLocalsView.setLabelText(Tr::tr("Use Tooltips in Locals View when Debugging"));
    useToolTipsInLocalsView.setToolTip(Tr::tr("<p>Enables tooltips in the locals "
        "view during debugging."));

    useToolTipsInBreakpointsView.setSettingsKey(debugModeGroup, "UseToolTipsInBreakpointsView");
    useToolTipsInBreakpointsView.setLabelText(Tr::tr("Use Tooltips in Breakpoints View when Debugging"));
    useToolTipsInBreakpointsView.setToolTip(Tr::tr("<p>Enables tooltips in the breakpoints "
        "view during debugging."));

    useToolTipsInStackView.setSettingsKey(debugModeGroup, "UseToolTipsInStackView");
    useToolTipsInStackView.setLabelText(Tr::tr("Use Tooltips in Stack View when Debugging"));
    useToolTipsInStackView.setToolTip(Tr::tr("<p>Enables tooltips in the stack "
        "view during debugging."));
    useToolTipsInStackView.setDefaultValue(true);

    skipKnownFrames.setSettingsKey(debugModeGroup, "SkipKnownFrames");
    skipKnownFrames.setDisplayName(Tr::tr("Skip Known Frames"));
    skipKnownFrames.setLabelText(Tr::tr("Skip known frames when stepping"));
    skipKnownFrames.setToolTip(Tr::tr(
        "<html><head/><body><p>"
        "Allows <i>Step Into</i> to compress several steps into one step\n"
        "for less noisy debugging. For example, the atomic reference\n"
        "counting code is skipped, and a single <i>Step Into</i> for a signal\n"
        "emission ends up directly in the slot connected to it."));

    enableReverseDebugging.setSettingsKey(debugModeGroup, "EnableReverseDebugging");
    enableReverseDebugging.setIcon(Icons::REVERSE_MODE.icon());
    enableReverseDebugging.setDisplayName(Tr::tr("Enable Reverse Debugging"));
    enableReverseDebugging.setLabelText(Tr::tr("Enable reverse debugging"));
    enableReverseDebugging.setToolTip(Tr::tr(
       "<html><head/><body><p>Enables stepping backwards.</p><p>"
       "<b>Note:</b> This feature is very slow and unstable on the GDB side. "
       "It exhibits unpredictable behavior when going backwards over system "
       "calls and is very likely to destroy your debugging session.</p></body></html>"));


#ifdef Q_OS_WIN
    registerForPostMortem = new RegisterPostMortemAction;
    registerForPostMortem->setSettingsKey(debugModeGroup, "RegisterForPostMortem");
    registerForPostMortem->setToolTip(
                Tr::tr("Registers %1 for debugging crashed applications.")
                .arg(Core::Constants::IDE_DISPLAY_NAME));
    registerForPostMortem->setLabelText(
                Tr::tr("Use %1 for post-mortem debugging")
                .arg(Core::Constants::IDE_DISPLAY_NAME));
#else
    // Some dummy.
    registerForPostMortem = new BoolAspect;
    registerForPostMortem->setVisible(false);
#endif

    allPluginBreakpoints.setSettingsKey(debugModeGroup, "AllPluginBreakpoints");
    allPluginBreakpoints.setDefaultValue(true);

    selectedPluginBreakpoints.setSettingsKey(debugModeGroup, "SelectedPluginBreakpoints");

    noPluginBreakpoints.setSettingsKey(debugModeGroup, "NoPluginBreakpoints");

    selectedPluginBreakpointsPattern.setSettingsKey(debugModeGroup, "SelectedPluginBreakpointsPattern");
    selectedPluginBreakpointsPattern.setDefaultValue(QString(".*"));

    maximalStackDepth.setSettingsKey(debugModeGroup, "MaximalStackDepth");
    maximalStackDepth.setDefaultValue(20);
    maximalStackDepth.setSpecialValueText(Tr::tr("<unlimited>"));
    maximalStackDepth.setRange(0, 1000);
    maximalStackDepth.setSingleStep(5);
    maximalStackDepth.setLabelText(Tr::tr("Maximum stack depth:"));

    displayStringLimit.setSettingsKey(debugModeGroup, "DisplayStringLimit");
    displayStringLimit.setDefaultValue(300);
    displayStringLimit.setSpecialValueText(Tr::tr("<unlimited>"));
    displayStringLimit.setRange(20, 10000);
    displayStringLimit.setSingleStep(10);
    displayStringLimit.setLabelText(Tr::tr("Display string length:"));
    displayStringLimit.setToolTip(Tr::tr("<p>The maximum length of string entries in the "
        "Locals and Expressions views. Longer than that are cut off "
        "and displayed with an ellipsis attached."));

    maximalStringLength.setSettingsKey(debugModeGroup, "MaximalStringLength");
    maximalStringLength.setDefaultValue(10000);
    maximalStringLength.setSpecialValueText(Tr::tr("<unlimited>"));
    maximalStringLength.setRange(20, 10000000);
    maximalStringLength.setSingleStep(20);
    maximalStringLength.setLabelText(Tr::tr("Maximum string length:"));
    maximalStringLength.setToolTip(Tr::tr("<p>The maximum length for strings in separated windows. "
        "Longer strings are cut off and displayed with an ellipsis attached."));

    expandStack.setLabelText(Tr::tr("Reload Full Stack"));

    createFullBacktrace.setLabelText(Tr::tr("Create Full Backtrace"));

    gdbWatchdogTimeout.setSettingsKey(debugModeGroup, "WatchdogTimeout");
    gdbWatchdogTimeout.setDefaultValue(20);
    gdbWatchdogTimeout.setSuffix(Tr::tr("sec"));
    gdbWatchdogTimeout.setRange(20, 1000000);
    gdbWatchdogTimeout.setLabelText(Tr::tr("GDB timeout:"));
    gdbWatchdogTimeout.setToolTip(Tr::tr(
        "The number of seconds before a non-responsive GDB process is terminated.\n"
        "The default value of 20 seconds should be sufficient for most\n"
        "applications, but there are situations when loading big libraries or\n"
        "listing source files takes much longer than that on slow machines.\n"
        "In this case, the value should be increased."));

    //
    // QML Tools
    //
    showQmlObjectTree.setSettingsKey(debugModeGroup, "ShowQmlObjectTree");
    showQmlObjectTree.setDefaultValue(true);
    showQmlObjectTree.setToolTip(Tr::tr("Shows QML object tree in Locals and Expressions "
                                        "when connected and not stepping."));
    showQmlObjectTree.setLabelText(Tr::tr("Show QML object tree"));

    const QString qmlInspectorGroup = "QML.Inspector";
    showAppOnTop.setSettingsKey(qmlInspectorGroup, "QmlInspector.ShowAppOnTop");

    // Page 1
    page1.registerAspect(&useAlternatingRowColors);
    page1.registerAspect(&useAnnotationsInMainEditor);
    page1.registerAspect(&useToolTipsInMainEditor);
    page1.registerAspect(&closeSourceBuffersOnExit);
    page1.registerAspect(&closeMemoryBuffersOnExit);
    page1.registerAspect(&raiseOnInterrupt);
    page1.registerAspect(&breakpointsFullPathByDefault);
    page1.registerAspect(&warnOnReleaseBuilds);
    page1.registerAspect(&maximalStackDepth);

    page1.registerAspect(&fontSizeFollowsEditor);
    page1.registerAspect(&switchModeOnExit);
    page1.registerAspect(&showQmlObjectTree);
    page1.registerAspect(&stationaryEditorWhileStepping);
    page1.registerAspect(&forceLoggingToConsole);

    page1.registerAspect(&sourcePathMap);

    // Page 2
    page2.registerAspect(&gdbWatchdogTimeout);
    page2.registerAspect(&skipKnownFrames);
    page2.registerAspect(&useMessageBoxForSignals);
    page2.registerAspect(&adjustBreakpointLocations);
    page2.registerAspect(&useDynamicType);
    page2.registerAspect(&loadGdbInit);
    page2.registerAspect(&loadGdbDumpers);
    page2.registerAspect(&intelFlavor);
    page2.registerAspect(&usePseudoTracepoints);
    page2.registerAspect(&useIndexCache);
    page2.registerAspect(&gdbStartupCommands);
    page2.registerAspect(&gdbPostAttachCommands);
    page2.registerAspect(&targetAsync);
    page2.registerAspect(&autoEnrichParameters);
    page2.registerAspect(&breakOnWarning);
    page2.registerAspect(&breakOnFatal);
    page2.registerAspect(&breakOnAbort);
    page2.registerAspect(&enableReverseDebugging);
    page2.registerAspect(&multiInferior);

    // Page 4
    page4.registerAspect(&useDebuggingHelpers);
    page4.registerAspect(&useCodeModel);
    page4.registerAspect(&showThreadNames);
    page4.registerAspect(&showStdNamespace);
    page4.registerAspect(&showQtNamespace);
    page4.registerAspect(&extraDumperFile);
    page4.registerAspect(&extraDumperCommands);
    page4.registerAspect(&showQObjectNames);
    page4.registerAspect(&displayStringLimit);
    page4.registerAspect(&maximalStringLength);

    // Page 5
    page5.registerAspect(&cdbAdditionalArguments);
    page5.registerAspect(&cdbBreakEvents);
    page5.registerAspect(&cdbBreakOnCrtDbgReport);
    page5.registerAspect(&useCdbConsole);
    page5.registerAspect(&cdbBreakPointCorrection);
    page5.registerAspect(&cdbUsePythonDumper);
    page5.registerAspect(&firstChanceExceptionTaskEntry);
    page5.registerAspect(&secondChanceExceptionTaskEntry);
    page5.registerAspect(&ignoreFirstChanceAccessViolation);
    if (HostOsInfo::isWindowsHost())
        page5.registerAspect(registerForPostMortem);

    // Page 6
    page6.registerAspect(&cdbSymbolPaths);
    page6.registerAspect(&cdbSourcePaths);

    // Pageless
    all.registerAspect(&autoDerefPointers);
    all.registerAspect(&useToolTipsInLocalsView);
    all.registerAspect(&alwaysAdjustColumnWidths);
    all.registerAspect(&useToolTipsInBreakpointsView);
    all.registerAspect(&useToolTipsInStackView);
    all.registerAspect(&logTimeStamps);
    all.registerAspect(&sortStructMembers);
    all.registerAspect(&breakOnThrow); // ??
    all.registerAspect(&breakOnCatch); // ??

    // Collect all
    all.registerAspects(page1);
    all.registerAspects(page2);
    all.registerAspects(page4);
    all.registerAspects(page5);
    all.registerAspects(page6);

    all.forEachAspect([](BaseAspect *aspect) {
        aspect->setAutoApply(false);
        // FIXME: Make the positioning part of the LayoutBuilder later
        if (auto boolAspect = dynamic_cast<BoolAspect *>(aspect))
            boolAspect->setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    });
}

DebuggerSettings::~DebuggerSettings()
{
    delete registerForPostMortem;
}

void DebuggerSettings::readSettings()
{
    all.readSettings(Core::ICore::settings());
}

void DebuggerSettings::writeSettings() const
{
    all.writeSettings(Core::ICore::settings());
}

QString DebuggerSettings::dump()
{
    QStringList settings;
    debuggerSettings()->all.forEachAspect([&settings](BaseAspect *aspect) {
        QString key = aspect->settingsKey();
        if (!key.isEmpty()) {
            const int pos = key.indexOf('/');
            if (pos >= 0)
                key = key.mid(pos);
            const QString current = aspect->value().toString();
            const QString default_ = aspect->defaultValue().toString();
            QString setting = key + ": " + current + "  (default: " + default_ + ')';
            if (current != default_)
                setting +=  "  ***";
            settings << setting;
        }
    });
    settings.sort();
    return "Debugger settings:\n" + settings.join('\n');
}

} // Debugger::Internal

