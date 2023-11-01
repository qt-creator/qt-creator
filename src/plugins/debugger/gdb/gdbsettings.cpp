// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gdbsettings.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <debugger/commonoptionspage.h>
#include <debugger/debuggeractions.h>
#include <debugger/debuggerconstants.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggericons.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggertr.h>

#include <utils/layoutbuilder.h>

using namespace Core;
using namespace Utils;

namespace Debugger::Internal {

GdbSettings &gdbSettings()
{
    static GdbSettings settings;
    return settings;
}

GdbSettings::GdbSettings()
{
    setAutoApply(false);
    setSettingsGroup("DebugMode");

    useMessageBoxForSignals.setSettingsKey("UseMessageBoxForSignals");
    useMessageBoxForSignals.setDefaultValue(true);
    useMessageBoxForSignals.setLabelText(Tr::tr(
        "Show a message box when receiving a signal"));
    useMessageBoxForSignals.setToolTip(Tr::tr(
        "Displays a message box as soon as your application\n"
        "receives a signal like SIGSEGV during debugging."));

    adjustBreakpointLocations.setDisplayName(Tr::tr("Adjust Breakpoint Locations"));
    adjustBreakpointLocations.setToolTip(
        "<p>"
        + Tr::tr("Not all source code lines generate "
                 "executable code. Putting a breakpoint on such a line acts as "
                 "if the breakpoint was set on the next line that generated code. "
                 "Selecting 'Adjust Breakpoint Locations' shifts the red "
                 "breakpoint markers in such cases to the location of the true "
                 "breakpoint."));
    adjustBreakpointLocations.setDefaultValue(true);
    adjustBreakpointLocations.setSettingsKey("AdjustBreakpointLocations");
    adjustBreakpointLocations.setLabelText(Tr::tr(
        "Adjust breakpoint locations"));
    adjustBreakpointLocations.setToolTip(Tr::tr(
        "GDB allows setting breakpoints on source lines for which no code \n"
        "was generated. In such situations the breakpoint is shifted to the\n"
        "next source code line for which code was actually generated.\n"
        "This option reflects such temporary change by moving the breakpoint\n"
        "markers in the source code editor."));


    breakOnThrow.setLabelText(Tr::tr("Break on \"throw\""));
    breakOnThrow.setSettingsKey("BreakOnThrow");

    breakOnCatch.setLabelText(Tr::tr("Break on \"catch\""));
    breakOnCatch.setSettingsKey("BreakOnCatch");

    breakOnWarning.setLabelText(Tr::tr("Break on \"qWarning\""));
    breakOnWarning.setSettingsKey("BreakOnWarning");
    // FIXME: Move to common settings page.
    breakOnWarning.setLabelText(msgSetBreakpointAtFunction("qWarning"));
    breakOnWarning.setToolTip(msgSetBreakpointAtFunctionToolTip("qWarning"));

    breakOnFatal.setLabelText(Tr::tr("Break on \"qFatal\""));
    breakOnFatal.setSettingsKey("BreakOnFatal");
    breakOnFatal.setLabelText(msgSetBreakpointAtFunction("qFatal"));
    breakOnFatal.setToolTip(msgSetBreakpointAtFunctionToolTip("qFatal"));

    breakOnAbort.setLabelText(Tr::tr("Break on \"abort\""));
    breakOnAbort.setSettingsKey("BreakOnAbort");
    breakOnAbort.setLabelText(msgSetBreakpointAtFunction("abort"));
    breakOnAbort.setToolTip(msgSetBreakpointAtFunctionToolTip("abort"));

    loadGdbInit.setSettingsKey("LoadGdbInit");
    loadGdbInit.setDefaultValue(true);
    loadGdbInit.setLabelText(Tr::tr("Load .gdbinit file on startup"));
    loadGdbInit.setToolTip(Tr::tr(
        "Allows or inhibits reading the user's default\n"
        ".gdbinit file on debugger startup."));

    loadGdbDumpers.setSettingsKey("LoadGdbDumpers2");
    loadGdbDumpers.setLabelText(Tr::tr("Load system GDB pretty printers"));
    loadGdbDumpers.setToolTip(Tr::tr(
        "Uses the default GDB pretty printers installed in your "
        "system or linked to the libraries your application uses."));

    autoEnrichParameters.setSettingsKey("AutoEnrichParameters");
    autoEnrichParameters.setDefaultValue(true);
    autoEnrichParameters.setLabelText(Tr::tr(
        "Use common locations for debug information"));
    autoEnrichParameters.setToolTip(Tr::tr(
        "<html><head/><body>Adds common paths to locations "
        "of debug information such as <i>/usr/src/debug</i> "
        "when starting GDB.</body></html>"));

    useDynamicType.setSettingsKey("UseDynamicType");
    useDynamicType.setDefaultValue(true);
    useDynamicType.setDisplayName(Tr::tr("Use Dynamic Object Type for Display"));
    useDynamicType.setLabelText(Tr::tr(
        "Use dynamic object type for display"));
    useDynamicType.setToolTip(Tr::tr(
        "Specifies whether the dynamic or the static type of objects will be "
        "displayed. Choosing the dynamic type might be slower."));

    targetAsync.setSettingsKey("TargetAsync");
    targetAsync.setLabelText(Tr::tr(
        "Use asynchronous mode to control the inferior"));

    QString howToUsePython = Tr::tr(
        "<p>To execute simple Python commands, prefix them with \"python\".</p>"
        "<p>To execute sequences of Python commands spanning multiple lines "
        "prepend the block with \"python\" on a separate line, and append "
        "\"end\" on a separate line.</p>"
        "<p>To execute arbitrary Python scripts, "
        "use <i>python execfile('/path/to/script.py')</i>.</p>");

    gdbStartupCommands.setSettingsKey("GdbStartupCommands");
    gdbStartupCommands.setDisplayStyle(StringAspect::TextEditDisplay);
    gdbStartupCommands.setUseGlobalMacroExpander();
    gdbStartupCommands.setToolTip("<html><head/><body><p>" + Tr::tr(
        "GDB commands entered here will be executed after "
        "GDB has been started, but before the debugged program is started or "
        "attached, and before the debugging helpers are initialized.") + "</p>"
        + howToUsePython + "</body></html>");

    gdbPostAttachCommands.setSettingsKey("GdbPostAttachCommands");
    gdbPostAttachCommands.setDisplayStyle(StringAspect::TextEditDisplay);
    gdbPostAttachCommands.setUseGlobalMacroExpander();
    gdbPostAttachCommands.setToolTip("<html><head/><body><p>" + Tr::tr(
        "GDB commands entered here will be executed after "
        "GDB has successfully attached to remote targets.</p>"
        "<p>You can add commands to further set up the target here, "
        "such as \"monitor reset\" or \"load\".") + "</p>"
        + howToUsePython + "</body></html>");

    multiInferior.setSettingsKey("MultiInferior");
    multiInferior.setLabelText(Tr::tr("Debug all child processes"));
    multiInferior.setToolTip(Tr::tr(
        "<html><head/><body>Keeps debugging all children after a fork."
        "</body></html>"));

    intelFlavor.setSettingsKey("IntelFlavor");
    intelFlavor.setLabelText(Tr::tr("Use Intel style disassembly"));
    intelFlavor.setToolTip(Tr::tr("GDB shows by default AT&&T style disassembly."));

    usePseudoTracepoints.setSettingsKey("UsePseudoTracepoints");
    usePseudoTracepoints.setLabelText(Tr::tr("Use pseudo message tracepoints"));
    usePseudoTracepoints.setToolTip(Tr::tr("Uses Python to extend the ordinary GDB breakpoint class."));
    usePseudoTracepoints.setDefaultValue(true);

    useIndexCache.setSettingsKey("UseIndexCache");
    useIndexCache.setLabelText(Tr::tr("Use automatic symbol cache"));
    useIndexCache.setToolTip(Tr::tr("It is possible for GDB to automatically save a copy of "
        "its symbol index in a cache on disk and retrieve it from there when loading the same "
        "binary in the future."));
    useIndexCache.setDefaultValue(true);

    skipKnownFrames.setSettingsKey("SkipKnownFrames");
    skipKnownFrames.setDisplayName(Tr::tr("Skip Known Frames"));
    skipKnownFrames.setLabelText(Tr::tr("Skip known frames when stepping"));
    skipKnownFrames.setToolTip(Tr::tr(
        "<html><head/><body><p>"
        "Allows <i>Step Into</i> to compress several steps into one step\n"
        "for less noisy debugging. For example, the atomic reference\n"
        "counting code is skipped, and a single <i>Step Into</i> for a signal\n"
        "emission ends up directly in the slot connected to it."));

    enableReverseDebugging.setSettingsKey("EnableReverseDebugging");
    enableReverseDebugging.setIcon(Icons::REVERSE_MODE.icon());
    enableReverseDebugging.setDisplayName(Tr::tr("Enable Reverse Debugging"));
    enableReverseDebugging.setLabelText(Tr::tr("Enable reverse debugging"));
    enableReverseDebugging.setToolTip(Tr::tr(
       "<html><head/><body><p>Enables stepping backwards.</p><p>"
       "<b>Note:</b> This feature is very slow and unstable on the GDB side. "
       "It exhibits unpredictable behavior when going backwards over system "
       "calls and is very likely to destroy your debugging session.</p></body></html>"));

    gdbWatchdogTimeout.setSettingsKey("WatchdogTimeout");
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


    setLayouter([this] {
        using namespace Layouting;

        auto labelDangerous = new QLabel("<html><head/><body><i>" +
            Tr::tr("The options below give access to advanced<br>"
                   "or experimental functions of GDB.<p>"
                   "Enabling them may negatively impact<br>"
                   "your debugging experience.") + "</i></body></html>");

        Group general {
            title(Tr::tr("General")),
            Column {
                Row { gdbWatchdogTimeout, st },
                skipKnownFrames,
                useMessageBoxForSignals,
                adjustBreakpointLocations,
                useDynamicType,
                loadGdbInit,
                loadGdbDumpers,
                intelFlavor,
                usePseudoTracepoints,
                useIndexCache,
                st
            }
        };

        Group extended {
            title(Tr::tr("Extended")),
            Column {
                labelDangerous,
                targetAsync,
                autoEnrichParameters,
                breakOnWarning,
                breakOnFatal,
                breakOnAbort,
                enableReverseDebugging,
                multiInferior,
                st
            }
        };

        Group startup {
            title(Tr::tr("Additional Startup Commands")),
            Column { gdbStartupCommands }
        };

        Group attach {
            title(Tr::tr("Additional Attach Commands")),
            Column { gdbPostAttachCommands },
        };

        return Grid { general, extended, br, startup, attach };
    });

    readSettings();
}

// GdbSettingsPage

class GdbSettingsPage final : public Core::IOptionsPage
{
public:
    GdbSettingsPage()
    {
        setId("M.Gdb");
        setDisplayName(Tr::tr("GDB"));
        setCategory(Constants::DEBUGGER_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &gdbSettings(); });
    }
};

const GdbSettingsPage settingsPage;

} // Debugger::Internal
