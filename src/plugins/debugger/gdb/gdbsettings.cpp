// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gdbsettings.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <debugger/debuggeractions.h>
#include <debugger/debuggericons.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggertr.h>

#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcsettings.h>

#include <QLabel>

using namespace Core;
using namespace Utils;

namespace Debugger::Internal {

// The tri-state "UseDebugInfoD" the mode replaces stored 0 for "enabled" and 1 for
// "disabled". Its default "use system settings" was never written, so an absent
// key ends up as Disabled, the default of the mode.
int debugInfoDModeFromOldSetting(const QVariant &oldValue)
{
    return oldValue.isValid() && oldValue.toInt() == 0 ? DebugInfoDFromEnvironment
                                                         : DebugInfoDDisabled;
}

GdbSettings &gdbSettings()
{
    static GdbSettings settings;
    return settings;
}

GdbSettings::GdbSettings()
{
    setAutoApply(false);
    const Key debugModeGroup("DebugMode");

    useMessageBoxForSignals.setSettingsKey(debugModeGroup, "UseMessageBoxForSignals");
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
    breakOnWarning.setLabelText(msgSetBreakpointAtFunction("qWarning"));
    breakOnWarning.setToolTip(msgSetBreakpointAtFunctionToolTip("qWarning"));

    breakOnFatal.setLabelText(Tr::tr("Break on \"qFatal\""));
    breakOnFatal.setSettingsKey(debugModeGroup, "BreakOnFatal");
    breakOnFatal.setLabelText(msgSetBreakpointAtFunction("qFatal"));
    breakOnFatal.setToolTip(msgSetBreakpointAtFunctionToolTip("qFatal"));

    breakOnAbort.setLabelText(Tr::tr("Break on \"abort\""));
    breakOnAbort.setSettingsKey(debugModeGroup, "BreakOnAbort");
    breakOnAbort.setLabelText(msgSetBreakpointAtFunction("abort"));
    breakOnAbort.setToolTip(msgSetBreakpointAtFunctionToolTip("abort"));

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
        "Use asynchronous mode to control the debugged process"));

    targetNonStop.setSettingsKey(debugModeGroup, "TargetNonStop");
    targetNonStop.setDefaultValue(true);
    targetNonStop.setLabelText(Tr::tr("Use non-stop mode for remote Linux targets"));
    targetNonStop.setToolTip(Tr::tr(
        "<html><head/><body><p>Lets the other threads keep running while GDB steps "
        "over a breakpoint. Without it, stepping can hang when the stepped thread "
        "waits for a lock held by another thread.</p>"
        "<p>GDB still stops all threads when the debugged program stops, but one "
        "by one, which can make each stop noticeably slower with many threads "
        "on a slow connection.</p>"
        "</body></html>"));

    QString howToUsePython = Tr::tr(
        "<p>To execute simple Python commands, prefix them with \"python\".</p>"
        "<p>To execute sequences of Python commands spanning multiple lines "
        "prepend the block with \"python\" on a separate line, and append "
        "\"end\" on a separate line.</p>"
        "<p>To execute arbitrary Python scripts, "
        "use <i>source /path/to/script.py</i>.</p>");

    gdbStartupCommands.setSettingsKey(debugModeGroup, "GdbStartupCommands");
    gdbStartupCommands.setDisplayStyle(StringAspect::TextEditDisplay);
    gdbStartupCommands.setToolTip("<html><head/><body><p>" + Tr::tr(
        "GDB commands entered here will be executed after "
        "GDB has been started, but before the debugged program is started or "
        "attached, and before the debugging helpers are initialized.") + "</p>"
        + howToUsePython + "</body></html>");

    gdbPostAttachCommands.setSettingsKey(debugModeGroup, "GdbPostAttachCommands");
    gdbPostAttachCommands.setDisplayStyle(StringAspect::TextEditDisplay);
    gdbPostAttachCommands.setToolTip("<html><head/><body><p>" + Tr::tr(
        "GDB commands entered here will be executed after "
        "GDB has successfully attached to remote targets.</p>"
        "<p>You can add commands to further set up the target here, "
        "such as \"monitor reset\" or \"load\".") + "</p>"
        + howToUsePython + "</body></html>");

    multiInferior.setSettingsKey(debugModeGroup, "MultiInferior");
    multiInferior.setLabelText(Tr::tr("Debug all child processes"));
    multiInferior.setToolTip(Tr::tr(
        "<html><head/><body>Keeps debugging all children after a fork."
        "</body></html>"));

    intelFlavor.setSettingsKey(debugModeGroup, "IntelFlavor");
    intelFlavor.setLabelText(Tr::tr("Use Intel style disassembly"));
    intelFlavor.setToolTip(Tr::tr("GDB shows by default AT&&T style disassembly."));

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

    const QString envUrls = Environment::systemEnvironment().value("DEBUGINFOD_URLS");
    debugInfoDMode.setSettingsKey("DebugInfoDMode");
    debugInfoDMode.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    debugInfoDMode.setLabelText(Tr::tr("Download debug info (debuginfod):"));
    debugInfoDMode.addOption(Tr::tr("Disabled"));
    debugInfoDMode.addOption(Tr::tr("From environment"));
    debugInfoDMode.addOption(Tr::tr("Use custom URLs"));
    debugInfoDMode.setDefaultValue(DebugInfoDDisabled);
    debugInfoDMode.setToolTip(Tr::tr("Lets the debugger download missing debug information "
        "for system packages on demand from a debuginfod server.<ul>"
        "<li><b>Disabled</b>: do not use debuginfod. This unsets DEBUGINFOD_URLS "
        "and tells GDB to turn debuginfod off, overriding a setting in your gdbinit.</li>"
        "<li><b>From environment</b>: use the DEBUGINFOD_URLS environment variable "
        "(currently: %1).</li>"
        "<li><b>Use custom URLs</b>: use the URLs configured below.</li></ul>"
        "An unreachable server can stall startup, so a timeout is applied.")
        .arg(envUrls.isEmpty() ? Tr::tr("not set") : envUrls));

    debugInfoDUrls.setSettingsKey("DebugInfoDUrls");
    debugInfoDUrls.setDisplayStyle(StringAspect::LineEditDisplay);
    debugInfoDUrls.setLabelText(Tr::tr("Custom debuginfod URLs:"));
    debugInfoDUrls.setPlaceHolderText("https://debuginfod.elfutils.org/");
    debugInfoDUrls.setToolTip(Tr::tr("Space-separated list of debuginfod server URLs, used "
        "when \"Use custom URLs\" is selected."));

    debugInfoDTimeout.setSettingsKey("DebugInfoDTimeout");
    debugInfoDTimeout.setLabelText(Tr::tr("debuginfod connection timeout:"));
    debugInfoDTimeout.setSuffix(Tr::tr("sec"));
    debugInfoDTimeout.setRange(0, 3600);
    debugInfoDTimeout.setDefaultValue(10);
    debugInfoDTimeout.setToolTip(Tr::tr("Maximum time to wait when connecting to a debuginfod "
        "server, so an unreachable server fails fast instead of stalling startup. "
        "Set to 0 to impose no timeout."));

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

    rrExecutable.setSettingsKey(debugModeGroup, "RrExecutable");
    rrExecutable.setDefaultValue("rr");
    rrExecutable.setExpectedKind(PathChooserKind::Command);
    rrExecutable.setHistoryCompleter("Debugger.Rr.Command.History");
    rrExecutable.setLabelText(Tr::tr("rr executable:"));
    rrExecutable.setToolTip(Tr::tr("The rr record and replay tool used by "
                                   "\"Record with rr\" and \"Replay rr Recording\"."));

    rrRecordArguments.setSettingsKey(debugModeGroup, "RrRecordArguments");
    rrRecordArguments.setDisplayStyle(StringAspect::LineEditDisplay);
    rrRecordArguments.setLabelText(Tr::tr("Additional arguments for rr record:"));
    rrRecordArguments.setToolTip(Tr::tr("For example, \"--bind-to-cpu=0\" on CPUs with "
                                        "cores of different types."));

    gdbWatchdogTimeout.setSettingsKey(debugModeGroup, "WatchdogTimeout");
    gdbWatchdogTimeout.setDefaultValue(40);
    gdbWatchdogTimeout.setSuffix(Tr::tr("sec"));
    gdbWatchdogTimeout.setRange(10, 1000000);
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
            QLatin1String("%1<p>%2")
                .arg(Tr::tr("The options below give access to advanced or experimental functions of GDB."),
                     Tr::tr("Enabling them may negatively affect debugging."))
            + "</i></body></html>");
        labelDangerous->setWordWrap(true);

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
                Row { debugInfoDMode, st },
                Row { debugInfoDUrls },
                Row { debugInfoDTimeout, st },
                Row { rrExecutable },
                Row { rrRecordArguments },
                st
            }
        };

        Group extended {
            title(Tr::tr("Extended")),
            Column {
                labelDangerous,
                targetAsync,
                targetNonStop,
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

    const auto updateDebugInfoDEnabled = [this] {
        const int mode = debugInfoDMode.volatileValue();
        debugInfoDUrls.setEnabled(mode == DebugInfoDCustom);
        debugInfoDTimeout.setEnabled(mode != DebugInfoDDisabled);
    };
    connect(&debugInfoDMode, &BaseAspect::volatileValueChanged,
            this, updateDebugInfoDEnabled);

    readSettings();
    updateDebugInfoDEnabled();

    const Key oldDebugInfoDKey("UseDebugInfoD");
    if (Utils::userSettings().contains(oldDebugInfoDKey)) {
        debugInfoDMode.setValue(
            debugInfoDModeFromOldSetting(Utils::userSettings().value(oldDebugInfoDKey)));
        debugInfoDMode.writeSettings();
        Utils::userSettings().remove(oldDebugInfoDKey);
    }
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
