// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commonoptionspage.h"

#include "debuggeractions.h"
#include "debuggerinternalconstants.h"
#include "debuggertr.h"

#ifdef Q_OS_WIN
#include "registerpostmortemaction.h"
#endif

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/environment.h>
#include <utils/layoutbuilder.h>

#include <QGuiApplication>
#include <QLabel>

#include <optional>

using namespace Core;
using namespace Debugger::Constants;
using namespace Utils;

namespace Debugger::Internal {

// CommonSettings

CommonSettings &commonSettings()
{
    static CommonSettings settings;
    return settings;
}

static std::optional<bool> useGenericDebuggerOverride()
{
    static const std::optional<bool> forced = []() -> std::optional<bool> {
        bool ok = false;
        const int value = Utils::qtcEnvironmentVariableIntValue("QTC_USE_GENERIC_DEBUGGER", &ok);
        if (!ok)
            return {};
        qWarning("QTC_USE_GENERIC_DEBUGGER is set: the new debugger backends are %s.",
                 value ? "enabled" : "disabled");
        return value != 0;
    }();
    return forced;
}

bool isUseGenericDebuggerOverride()
{
    return useGenericDebuggerOverride().has_value();
}

bool useGenericDebuggerEnabled()
{
    return useGenericDebuggerOverride().value_or(commonSettings().useGenericDebugger());
}

CommonSettings::CommonSettings()
{
    setAutoApply(false);
    const Key debugModeGroup("DebugMode");

    useAlternatingRowColors.setSettingsKey(debugModeGroup, "UseAlternatingRowColours");
    useAlternatingRowColors.setLabelText(Tr::tr("Use alternating row colors in debug views"));

    stationaryEditorWhileStepping.setSettingsKey(debugModeGroup, "StationaryEditorWhileStepping");
    stationaryEditorWhileStepping.setLabelText(Tr::tr("Keep editor stationary when stepping"));
    stationaryEditorWhileStepping.setToolTip(
        Tr::tr("Scrolls the editor only when it is necessary to keep the current line in view, "
               "instead of keeping the next statement centered at all times."));

    showUnsupportedBreakpointWarning.setSettingsKey(debugModeGroup, "ShowUnsupportedBreakpointWarning");
    showUnsupportedBreakpointWarning.setDefaultValue(true);
    showUnsupportedBreakpointWarning.setLabelText(
        Tr::tr("Show warnings for unsupported breakpoints"));
    showUnsupportedBreakpointWarning.setToolTip(
        Tr::tr("Shows a warning on debugger start-up when breakpoints are requested "
               "which are not supported by the selected debugger engine."));

    forceLoggingToConsole.setSettingsKey(debugModeGroup, "ForceLoggingToConsole");
    forceLoggingToConsole.setLabelText(Tr::tr("Force logging to console"));
    forceLoggingToConsole.setToolTip(
        Tr::tr("Sets QT_LOGGING_TO_CONSOLE=1 in the environment of the debugged program, "
               "preventing storing debug output in system logs."));

    fontSizeFollowsEditor.setSettingsKey(debugModeGroup, "FontSizeFollowsEditor");
    fontSizeFollowsEditor.setToolTip(Tr::tr("Changes the font size in the debugger views when "
                                            "the font size in the main editor changes."));
    fontSizeFollowsEditor.setLabelText(Tr::tr("Debugger font size follows main editor"));

#ifdef Q_OS_WIN
    registerForPostMortem = new RegisterPostMortemAction;
    registerForPostMortem->setSettingsKey(debugModeGroup, "RegisterForPostMortem");
    registerForPostMortem->setToolTip(Tr::tr("Registers %1 for debugging crashed applications.")
                                          .arg(QGuiApplication::applicationDisplayName()));
    registerForPostMortem->setLabelText(
        Tr::tr("Use %1 for post-mortem debugging").arg(QGuiApplication::applicationDisplayName()));
    registerAspect(registerForPostMortem);
#else
    // Some dummy.
    registerForPostMortem = new BoolAspect;
    registerForPostMortem->setVisible(false);
#endif

    maximalStackDepth.setSettingsKey(debugModeGroup, "MaximalStackDepth");
    maximalStackDepth.setDefaultValue(20);
    maximalStackDepth.setSpecialValueText(Tr::tr("<unlimited>"));
    maximalStackDepth.setRange(0, 1000);
    maximalStackDepth.setSingleStep(5);
    maximalStackDepth.setLabelText(Tr::tr("Maximum stack depth:"));

    showQmlObjectTree.setSettingsKey(debugModeGroup, "ShowQmlObjectTree");
    showQmlObjectTree.setDefaultValue(true);
    showQmlObjectTree.setToolTip(Tr::tr("Shows QML object tree in Locals and Expressions "
                                        "when connected and not stepping."));
    showQmlObjectTree.setLabelText(Tr::tr("Show QML object tree"));

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

    resolveBreakpointSymlinks.setSettingsKey(debugModeGroup, "ResolveBreakpointSymlinks");
    resolveBreakpointSymlinks.setToolTip(
        Tr::tr("Resolves symbolic links in the file path when setting breakpoints, so the path "
               "matches the one recorded in the debug information. This helps LLDB bind "
               "breakpoints in files opened through a symbolic link. Leave this off if the "
               "project is built through a symbolic link, as the debug information then records "
               "the unresolved path."));
    resolveBreakpointSymlinks.setLabelText(Tr::tr("Resolve symbolic links in breakpoint paths"));

    lookUpSourcesOnDebuggerDevice.setSettingsKey(debugModeGroup, "LookUpSourcesOnDebuggerDevice");
    lookUpSourcesOnDebuggerDevice.setDefaultValue(true);
    lookUpSourcesOnDebuggerDevice.setToolTip(
        Tr::tr("Looks for a source file the debugger reports on the device the debugger itself "
               "runs on, once it is not readable on the host. This finds sources that exist only "
               "inside a container or on a remote device, at the price of one lookup on that "
               "device for every name the debugger reports for the first time."));
    lookUpSourcesOnDebuggerDevice.setLabelText(
        Tr::tr("Look for sources on the debugger's device"));

    raiseOnInterrupt.setSettingsKey(debugModeGroup, "RaiseOnInterrupt");
    raiseOnInterrupt.setDefaultValue(true);
    raiseOnInterrupt.setLabelText(Tr::tr("Bring %1 to foreground when application interrupts")
                                      .arg(QGuiApplication::applicationDisplayName()));

    useAnnotationsInMainEditor.setSettingsKey(debugModeGroup, "UseAnnotations");
    useAnnotationsInMainEditor.setLabelText(Tr::tr("Use annotations in main editor when debugging"));
    useAnnotationsInMainEditor.setToolTip(
        "<p>"
        + Tr::tr("Shows simple variable values "
                 "as annotations in the main editor during debugging."));
    useAnnotationsInMainEditor.setDefaultValue(true);

    warnOnReleaseBuilds.setSettingsKey(debugModeGroup, "WarnOnReleaseBuilds");
    warnOnReleaseBuilds.setDefaultValue(true);
    warnOnReleaseBuilds.setLabelText(Tr::tr("Warn when debugging \"Release\" builds"));
    warnOnReleaseBuilds.setToolTip(Tr::tr("Shows a warning when starting the debugger "
                                          "on a binary with insufficient debug information."));

    collapseMachineryFrames.setSettingsKey(debugModeGroup, "CollapseDebuggerMachineryFrames");
    collapseMachineryFrames.setDefaultValue(true);
    collapseMachineryFrames.setLabelText(
        Tr::tr("Collapse debugger machinery stack frames"));
    collapseMachineryFrames.setToolTip(
        "<p>"
        + Tr::tr("When debugging C++ and QML with a combined engine, replaces the frames "
                 "between a QML frame and the application code, which only execute "
                 "debugger machinery, with a single placeholder frame in the "
                 "stack view."));

    useGenericDebugger.setSettingsKey(debugModeGroup, "UseGenericDebugger");
    useGenericDebugger.setLabelText(Tr::tr("Use the new debugger backends (experimental)"));
    QString useGenericDebuggerTooltip
        = Tr::tr("Runs the debugger through the new engine-independent front end "
                 "instead of the engine's own implementation. The new backends are "
                 "incomplete, so expect missing views and unhandled situations. "
                 "The QTC_USE_GENERIC_DEBUGGER environment variable overrides "
                 "this setting.");
    if (isUseGenericDebuggerOverride()) {
        useGenericDebuggerTooltip += QLatin1Char(' ');
        useGenericDebuggerTooltip
            += Tr::tr("Unset QTC_USE_GENERIC_DEBUGGER to enable this setting.");
    }
    useGenericDebugger.setToolTip("<p>" + useGenericDebuggerTooltip);
    useGenericDebugger.setEnabled(!isUseGenericDebuggerOverride());

    useToolTipsInMainEditor.setSettingsKey(debugModeGroup, "UseToolTips");
    useToolTipsInMainEditor.setLabelText(Tr::tr("Use tooltips in main editor when debugging"));
    useToolTipsInMainEditor.setToolTip(
        "<p>"
        + Tr::tr("Enables tooltips for variable values during debugging. "
                 "This can slow down debugging and does not provide reliable information "
                 "as it does not use scope information."));
    useToolTipsInMainEditor.setDefaultValue(true);

    setLayouter([this] {
        using namespace Layouting;

        Group behavior {
            title(Tr::tr("Behavior")),
            Column {
                registerForPostMortem,
                raiseOnInterrupt,
                warnOnReleaseBuilds,
                breakpointsFullPathByDefault,
                resolveBreakpointSymlinks,
                lookUpSourcesOnDebuggerDevice,
                forceLoggingToConsole,
                collapseMachineryFrames,
                useGenericDebugger,
                Row { maximalStackDepth, st },
                st
            }
        };

        Group userInterface {
            title(Tr::tr("User Interface")),
            Column {
                useAnnotationsInMainEditor,
                useToolTipsInMainEditor,
                useAlternatingRowColors,
                fontSizeFollowsEditor,
                stationaryEditorWhileStepping,
                showQmlObjectTree,
                showUnsupportedBreakpointWarning,
            }
        };

        Group afterLife {
            title(Tr::tr("When Debugging Stops")),
            Column {
                closeSourceBuffersOnExit,
                closeMemoryBuffersOnExit,
                switchModeOnExit,
            }
        };

        return Column {
            Grid {
                Column { behavior, afterLife },
                Column { userInterface, st },
                columnStretch(0, 1),
                columnStretch(1, 1)
            },
            sourcePathMap,
            st
        };
    });

    readSettings();
}

CommonSettings::~CommonSettings()
{
    delete registerForPostMortem;
}

QString msgSetBreakpointAtFunction(const char *function)
{
    return Tr::tr("Stop when %1() is called").arg(QLatin1String(function));
}

QString msgSetBreakpointAtFunctionToolTip(const char *function, const QString &hint)
{
    QString result = "<html><head/><body>";
    result += Tr::tr("Always adds a breakpoint on the %1 function.")
            .arg("<i>" + QLatin1String(function) + "()</i>");
    if (!hint.isEmpty()) {
        result += "<br>";
        result += hint;
    }
    result += "</body></html>";
    return result;
}

// CommonSettingPage

class CommonSettingsPage final : public Core::IOptionsPage
{
public:
    CommonSettingsPage()
    {
        setId(DEBUGGER_COMMON_SETTINGS_ID);
        setDisplayName(Tr::tr("General"));
        setCategory(DEBUGGER_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &commonSettings(); });
        setRecreateOnCancel(true);
    }
};

const CommonSettingsPage commonSettingsPage;


// LocalsAndExpressions

LocalsAndExpressionsSettings &localsAndExpressionSettings()
{
    static LocalsAndExpressionsSettings settings;
    return settings;
}

LocalsAndExpressionsSettings::LocalsAndExpressionsSettings()
{
    setAutoApply(false);

    const Key debugModeGroup("DebugMode");

    useDebuggingHelpers.setSettingsKey(debugModeGroup, "UseDebuggingHelper");
    useDebuggingHelpers.setDefaultValue(true);
    useDebuggingHelpers.setLabelText(Tr::tr("Use Debugging Helpers"));

    allowInferiorCalls.setSettingsKey(debugModeGroup, "AllowInferiorCalls");
    allowInferiorCalls.setDefaultValue(true);
    allowInferiorCalls.setLabelText(Tr::tr("Allow inferior calls in debugging helper"));

    useCodeModel.setSettingsKey(debugModeGroup, "UseCodeModel");
    useCodeModel.setDefaultValue(true);
    useCodeModel.setLabelText(Tr::tr("Use code model"));
    useCodeModel.setToolTip(
        "<p>"
        + Tr::tr("Selecting this causes the C++ Code Model being asked "
                 "for variable scope information. This might result in slightly faster "
                 "debugger operation but may fail for optimized code."));

    showThreadNames.setSettingsKey(debugModeGroup, "ShowThreadNames");
    showThreadNames.setLabelText(Tr::tr("Display thread names"));
    showThreadNames.setToolTip("<p>" + Tr::tr("Displays names of QThread based threads."));

    showStdNamespace.setSettingsKey(debugModeGroup, "ShowStandardNamespace");
    showStdNamespace.setDefaultValue(true);
    showStdNamespace.setDisplayName(Tr::tr("Show \"std::\" Namespace in Types"));
    showStdNamespace.setLabelText(Tr::tr("Show \"std::\" namespace in types"));
    showStdNamespace.setToolTip(
        "<p>" + Tr::tr("Shows \"std::\" prefix for types from the standard library."));

    showQtNamespace.setSettingsKey(debugModeGroup, "ShowQtNamespace");
    showQtNamespace.setDefaultValue(true);
    showQtNamespace.setDisplayName(Tr::tr("Show Qt's Namespace in Types"));
    showQtNamespace.setLabelText(Tr::tr("Show Qt's namespace in types"));
    showQtNamespace.setToolTip("<p>"
                               + Tr::tr("Shows Qt namespace prefix for Qt types. This is only "
                                        "relevant if Qt was configured with \"-qtnamespace\"."));

    showQObjectNames.setSettingsKey(debugModeGroup, "ShowQObjectNames2");
    showQObjectNames.setDefaultValue(true);
    showQObjectNames.setDisplayName(Tr::tr("Show QObject Names if Available"));
    showQObjectNames.setLabelText(Tr::tr("Show QObject names if available"));
    showQObjectNames.setToolTip(
        "<p>"
        + Tr::tr("Displays the objectName property of QObject based items. "
                 "Note that this can negatively impact debugger performance "
                 "even if no QObjects are present."));

    extraDumperCommands.setSettingsKey(debugModeGroup, "GdbCustomDumperCommands");
    extraDumperCommands.setDisplayStyle(StringAspect::TextEditDisplay);
    extraDumperCommands.setToolTip("<html><head/><body><p>"
                        + Tr::tr("Python commands entered here will be executed after built-in "
                             "debugging helpers have been loaded and fully initialized. You can "
                             "load additional debugging helpers or modify existing ones here.")
                        + "</p></body></html>");

    extraDumperFile.setSettingsKey(debugModeGroup, "ExtraDumperFile");
    extraDumperFile.setDisplayName(Tr::tr("Extra Debugging Helpers"));
    // Label text is intentional empty in the GUI.
    extraDumperFile.setToolTip(Tr::tr("Path to a Python file containing additional data dumpers."));

    displayStringLimit.setSettingsKey(debugModeGroup, "DisplayStringLimit");
    displayStringLimit.setDefaultValue(300);
    displayStringLimit.setSpecialValueText(Tr::tr("<unlimited>"));
    displayStringLimit.setRange(20, 10000);
    displayStringLimit.setSingleStep(10);
    displayStringLimit.setLabelText(Tr::tr("Display string length:"));
    displayStringLimit.setToolTip(
        "<p>"
        + Tr::tr("The maximum length of string entries in the "
                 "Locals and Expressions views. Longer than that are cut off "
                 "and displayed with an ellipsis attached."));

    maximalStringLength.setSettingsKey(debugModeGroup, "MaximalStringLength");
    maximalStringLength.setDefaultValue(10000);
    maximalStringLength.setSpecialValueText(Tr::tr("<unlimited>"));
    maximalStringLength.setRange(20, 10000000);
    maximalStringLength.setSingleStep(20);
    maximalStringLength.setLabelText(Tr::tr("Maximum string length:"));
    maximalStringLength.setToolTip(
        "<p>"
        + Tr::tr("The maximum length for strings in separated windows. "
                 "Longer strings are cut off and displayed with an ellipsis attached."));

    defaultArraySize.setSettingsKey(debugModeGroup, "DefaultArraySize");
    defaultArraySize.setDefaultValue(100);
    defaultArraySize.setRange(10, 1000000000);
    defaultArraySize.setSingleStep(100);
    defaultArraySize.setLabelText(Tr::tr("Default array size:"));
    defaultArraySize.setToolTip("<p>"
                                + Tr::tr("The number of array elements requested when expanding "
                                         "entries in the Locals and Expressions views."));

    setLayouter([this] {
        auto label = new QLabel; //(useHelperGroup);
        label->setTextFormat(Qt::AutoText);
        label->setWordWrap(true);
        label->setText("<html><head/><body>\n<p>"
           + Tr::tr("The debugging helpers are used to produce a nice "
                "display of objects of certain types like QString or "
                "std::map in the &quot;Locals&quot; and &quot;Expressions&quot; views.")
            + "</p></body></html>");

        using namespace Layouting;
        Column left {
            label,
            useCodeModel,
            showThreadNames,
            Group { title(Tr::tr("Extra Debugging Helper")), Column { extraDumperFile } }
        };

        Group useHelper {
            Row {
                left,
                Group {
                    title(Tr::tr("Debugging Helper Customization")),
                    Column { extraDumperCommands }
                }
            }
        };

        Grid limits {
            maximalStringLength, br,
            displayStringLimit, br,
            defaultArraySize
        };

        return Column {
            useDebuggingHelpers,
            allowInferiorCalls,
            useHelper,
            Space(10),
            showStdNamespace,
            showQtNamespace,
            showQObjectNames,
            Space(10),
            Row { limits, st },
            st
        };
    });

    readSettings();
}

class LocalsAndExpressionsSettingsPage final : public Core::IOptionsPage
{
public:
    LocalsAndExpressionsSettingsPage()
    {
        setId("Z.Debugger.LocalsAndExpressions");
        //: '&&' will appear as one (one is marking keyboard shortcut)
        setDisplayName(Tr::tr("Locals && Expressions"));
        setCategory(DEBUGGER_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &localsAndExpressionSettings(); });
    }
};

const LocalsAndExpressionsSettingsPage localsAndExpressionSettingsPage;

} // Debugger::Internal
