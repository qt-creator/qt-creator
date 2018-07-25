/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "iconlister.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <autotest/autotesticons.h>
#include <coreplugin/coreicons.h>
#include <cplusplus/Icons.h>
#include <diffeditor/diffeditoricons.h>
#include <help/helpicons.h>
#include <projectexplorer/projectexplorericons.h>
#include <qmldesigner/qmldesignericons.h>
#include <utils/utilsicons.h>

using namespace Utils;

const char ICONS_PATH[] = "qtcreator-icons/";

static QString iconFileName(const QString &id, const QString &idContext, IconLister::ThemeKind theme, int dpr)
{
    QString context = idContext;
    context.replace("::", "-");
    const QString scaleFactor = (dpr >= 2)
            ? QLatin1String("@") + QString::number(dpr) + QLatin1String("x")
            : QString();
    const QString themeName = QLatin1String(theme == IconLister::ThemeKindDark
            ? "darktheme"
            : "lighttheme");
    return QLatin1String(ICONS_PATH) + context.toLower() + "-" + id.toLower() + "-" + themeName + scaleFactor + ".png";
}

void IconLister::setCreatorTheme(ThemeKind themeKind)
{
    const QString themeKindStrig = QLatin1String(themeKind == ThemeKindDark ? "dark" : "light");
    static Theme theme("");
    QSettings settings(":/flat-" + themeKindStrig + ".creatortheme", QSettings::IniFormat);
    theme.readSettings(settings);
    Utils::setCreatorTheme(&theme);
    StyleHelper::setBaseColor(QColor(StyleHelper::DEFAULT_BASE_COLOR));
}

void IconLister::generateJson()
{
    setCreatorTheme(ThemeKindDark);
    IconLister lister;
    lister.addAllIcons();
    lister.saveJson();
}

void IconLister::generateIcons(IconLister::ThemeKind theme, int dpr)
{
    setCreatorTheme(theme);
    IconLister lister;
    lister.addAllIcons();
    lister.saveIcons(theme, dpr);
}

void IconLister::addAllIcons()
{
    addAutoTestIcons();
    addCoreIcons();
    addDiffEditorIcons();
    addHelpIcons();
    addProjectExplorerIcons();
    addDebuggerIcons();
    addUtilsIcons();
    addCPlusPlusIcons();
    addQmlDesignerIcons();
    addProfilerTimelineIcons();
    addWizardIcons();
    addAndroidIcons();
    addIosIcons();
    addBareMetalIcons();
    addQnxIcons();
    addWinRTIcons();
    addBoot2QtIcons();
    addVxWorksIcons();
}

void IconLister::saveJson() const
{
    QFile file("qtcreator-icons.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonArray iconArray;
        for (const IconInfo &iconInfo : m_icons) {
            QJsonObject icon;
            icon.insert("iconId", iconInfo.id);
            icon.insert("iconIdContext", iconInfo.idContext);
            icon.insert("iconDescription", iconInfo.description);
            icon.insert("iconLightTheme", iconFileName(iconInfo.id, iconInfo.idContext, ThemeKindLight, 1));
            icon.insert("iconDarkTheme", iconFileName(iconInfo.id, iconInfo.idContext, ThemeKindDark, 1));
            icon.insert("iconLightTheme@2x", iconFileName(iconInfo.id, iconInfo.idContext, ThemeKindLight, 2));
            icon.insert("iconDarkTheme@2x", iconFileName(iconInfo.id, iconInfo.idContext, ThemeKindDark, 2));
            iconArray.append(icon);
        }
        QJsonDocument doc;
        doc.setArray(iconArray);
        file.write(doc.toJson(QJsonDocument::Indented));
    }
}

void IconLister::addAutoTestIcons()
{
    Q_INIT_RESOURCE(autotest);
    using namespace Autotest::Icons;
    const QString prefix = "Autotest";
    const QList<IconInfo> icons = {
        {SORT_ALPHABETICALLY.icon(), "SORT_ALPHABETICALLY", prefix,
         ""},
        {SORT_NATURALLY.icon(), "SORT_NATURALLY", prefix,
         ""},
        {RUN_SELECTED_OVERLAY.icon(), "RUN_SELECTED_OVERLAY", prefix,
         ""},

        {RESULT_PASS.icon(), "RESULT_PASS", prefix,
         ""},
        {RESULT_FAIL.icon(), "RESULT_FAIL", prefix,
         ""},
        {RESULT_XFAIL.icon(), "RESULT_XFAIL", prefix,
         ""},
        {RESULT_XPASS.icon(), "RESULT_XPASS", prefix,
         ""},
        {RESULT_SKIP.icon(), "RESULT_SKIP", prefix,
         ""},
        {RESULT_BLACKLISTEDPASS.icon(), "RESULT_BLACKLISTEDPASS", prefix,
         ""},
        {RESULT_BLACKLISTEDFAIL.icon(), "RESULT_BLACKLISTEDFAIL", prefix,
         ""},
        {RESULT_BENCHMARK.icon(), "RESULT_BENCHMARK", prefix,
         ""},
        {RESULT_MESSAGEDEBUG.icon(), "RESULT_MESSAGEDEBUG", prefix,
         ""},
        {RESULT_MESSAGEWARN.icon(), "RESULT_MESSAGEWARN", prefix,
         ""},
        {RESULT_MESSAGEPASSWARN.icon(), "RESULT_MESSAGEPASSWARN", prefix,
         ""},
        {RESULT_MESSAGEFAILWARN.icon(), "RESULT_MESSAGEFAILWARN", prefix,
         ""},
        {RESULT_MESSAGEFATAL.icon(), "RESULT_MESSAGEFATAL", prefix,
         ""},
        {VISUAL_DISPLAY.icon(), "VISUAL_DISPLAY", prefix,
         ""},
        {TEXT_DISPLAY.icon(), "TEXT_DISPLAY", prefix,
         ""}
    };
    m_icons.append(icons);
}

void IconLister::addCoreIcons()
{
    using namespace Core::Icons;
    const QString prefix = "Core";
    const QList<IconInfo> icons = {
        {QTCREATORLOGO_BIG.icon(), "QTCREATORLOGO_BIG", prefix,
         "'About Qt Creator' dialog."},
        {FIND_CASE_INSENSITIVELY.icon(), "FIND_CASE_INSENSITIVELY", prefix,
         ""},
        {FIND_WHOLE_WORD.icon(), "FIND_WHOLE_WORD", prefix,
         ""},
        {FIND_REGEXP.icon(), "FIND_REGEXP", prefix,
         ""},
        {FIND_PRESERVE_CASE.icon(), "FIND_PRESERVE_CASE", prefix,
         ""},
        {MODE_EDIT_CLASSIC.icon(), "MODE_EDIT_CLASSIC", prefix,
         ""},
        {MODE_EDIT_FLAT.icon(), "MODE_EDIT_FLAT", prefix,
         ""},
        {MODE_EDIT_FLAT_ACTIVE.icon(), "MODE_EDIT_FLAT_ACTIVE", prefix,
         ""},
        {MODE_DESIGN_CLASSIC.icon(), "MODE_DESIGN_CLASSIC", prefix,
         ""},
        {MODE_DESIGN_FLAT.icon(), "MODE_DESIGN_FLAT", prefix,
         ""},
        {MODE_DESIGN_FLAT_ACTIVE.icon(), "MODE_DESIGN_FLAT_ACTIVE", prefix,
         ""},
        {Utils::Icon(":/find/images/wrapindicator.png").icon(), "wrapindicator", prefix,
         ""}
    };
    m_icons.append(icons);
}

void IconLister::addDiffEditorIcons()
{
    Q_INIT_RESOURCE(diffeditor);
    using namespace DiffEditor::Icons;
    const QString prefix = "DiffEditor";
    const QList<IconInfo> icons = {
        {TOP_BAR.icon(), "TOP_BAR", prefix,
         ""},
        {UNIFIED_DIFF.icon(), "UNIFIED_DIFF", prefix,
         ""},
        {SIDEBYSIDE_DIFF.icon(), "SIDEBYSIDE_DIFF", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::addHelpIcons()
{
    Q_INIT_RESOURCE(help);
    using namespace Help::Icons;
    const QString prefix = "Help";
    const QList<IconInfo> icons = {
        {MODE_HELP_CLASSIC.icon(), "MODE_HELP_CLASSIC", prefix,
         ""},
        {MODE_HELP_FLAT.icon(), "MODE_HELP_FLAT", prefix,
         ""},
        {MODE_HELP_FLAT_ACTIVE.icon(), "MODE_HELP_FLAT_ACTIVE", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::addProjectExplorerIcons()
{
    using namespace ProjectExplorer::Icons;
    const QString prefix = "ProjectExplorer";
    const QList<IconInfo> icons = {
        {BUILD.icon(), "BUILD", prefix,
         ""},
        {BUILD_FLAT.icon(), "BUILD_FLAT", prefix,
         ""},
        {BUILD_SMALL.icon(), "BUILD_SMALL", prefix,
         ""},
        {REBUILD.icon(), "REBUILD", prefix,
         ""},
        {RUN.icon(), "RUN", prefix,
         ""},
        {RUN_FLAT.icon(), "RUN_FLAT", prefix,
         ""},
        {WINDOW.icon(), "WINDOW", prefix,
         ""},
        {DEBUG_START.icon(), "DEBUG_START", prefix,
         ""},
        {DEVICE_READY_INDICATOR.icon(), "DEVICE_READY_INDICATOR", prefix,
         ""},
        {DEVICE_READY_INDICATOR_OVERLAY.icon(), "DEVICE_READY_INDICATOR_OVERLAY", prefix,
         ""},
        {DEVICE_CONNECTED_INDICATOR.icon(), "DEVICE_CONNECTED_INDICATOR", prefix,
         ""},
        {DEVICE_CONNECTED_INDICATOR_OVERLAY.icon(), "DEVICE_CONNECTED_INDICATOR_OVERLAY", prefix,
         ""},
        {DEVICE_DISCONNECTED_INDICATOR.icon(), "DEVICE_DISCONNECTED_INDICATOR", prefix,
         ""},
        {DEVICE_DISCONNECTED_INDICATOR_OVERLAY.icon(), "DEVICE_DISCONNECTED_INDICATOR_OVERLAY", prefix,
         ""},

        {DEBUG_START_FLAT.icon(), "DEBUG_START_FLAT", prefix,
         ""},
        {DEBUG_START_SMALL.icon(), "DEBUG_START_SMALL", prefix,
         ""},
        {DEBUG_START_SMALL_TOOLBAR.icon(), "DEBUG_START_SMALL_TOOLBAR", prefix,
         ""},
        {ANALYZER_START_SMALL.icon(), "ANALYZER_START_SMALL", prefix,
         ""},
        {ANALYZER_START_SMALL_TOOLBAR.icon(), "ANALYZER_START_SMALL_TOOLBAR", prefix,
         ""},

        {BUILDSTEP_MOVEUP.icon(), "BUILDSTEP_MOVEUP", prefix,
         ""},
        {BUILDSTEP_MOVEDOWN.icon(), "BUILDSTEP_MOVEDOWN", prefix,
         ""},
        {BUILDSTEP_DISABLE.icon(), "BUILDSTEP_DISABLE", prefix,
         ""},
        {BUILDSTEP_REMOVE.icon(), "BUILDSTEP_REMOVE", prefix,
         ""},

        {DESKTOP_DEVICE.icon(), "DESKTOP_DEVICE", prefix,
         ""},
        {DESKTOP_DEVICE_SMALL.icon(), "DESKTOP_DEVICE_SMALL", prefix,
         ""},

        {MODE_PROJECT_CLASSIC.icon(), "MODE_PROJECT_CLASSIC", prefix,
         ""},
        {MODE_PROJECT_FLAT.icon(), "MODE_PROJECT_FLAT", prefix,
         ""},
        {MODE_PROJECT_FLAT_ACTIVE.icon(), "MODE_PROJECT_FLAT_ACTIVE", prefix,
         ""},

        {Utils::Icon({{":/projectexplorer/images/settingscategory_kits.png", Utils::Theme::PanelTextColorDark}}, Utils::Icon::Tint).icon(), "OPTIONS_CATEGORY_KITS", prefix, ""},

        {QIcon(":/projectexplorer/images/fileoverlay_qml.png"), "fileoverlay_qml.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/fileoverlay_qrc.png"), "fileoverlay_qrc.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/fileoverlay_qt.png"), "fileoverlay_qt.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/fileoverlay_ui.png"), "fileoverlay_ui.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/fileoverlay_scxml.png"), "fileoverlay_scxml.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/fileoverlay_cpp.png"), "fileoverlay_cpp.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/fileoverlay_h.png"), "fileoverlay_h.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/fileoverlay_unknown.png"), "fileoverlay_unknown.png", prefix,
         ""},

        {QIcon(":/projectexplorer/images/category_buildrun.png"), "category_buildrun.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/closetab.png"), "closetab.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/projectexplorer.png"), "projectexplorer.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/session.png"), "session.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/BuildSettings.png"), "BuildSettings.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/CodeStyleSettings.png"), "CodeStyleSettings.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/RunSettings.png"), "RunSettings.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/EditorSettings.png"), "EditorSettings.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/ProjectDependencies.png"), "ProjectDependencies.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/MaemoDevice.png"), "MaemoDevice.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/Simulator.png"), "Simulator.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/targetpanel_bottom.png"), "targetpanel_bottom.png", prefix,
         ""},
        {QIcon(":/projectexplorer/images/unconfigured.png"), "unconfigured.png", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::addDebuggerIcons()
{
    Q_INIT_RESOURCE(debugger);

    const Icon BREAKPOINT({
            {":/utils/images/filledcircle.png", Theme::IconsErrorColor}}, Icon::Tint);
    const Icon BREAKPOINT_DISABLED({
            {":/debugger/images/breakpoint_disabled.png", Theme::IconsErrorColor}}, Icon::Tint);
    const Icon BREAKPOINT_PENDING({
            {":/utils/images/filledcircle.png", Theme::IconsErrorColor},
            {":/debugger/images/breakpoint_pending_overlay.png", Theme::PanelTextColorDark}}, Icon::IconStyleOptions(Icon::Tint | Icon::PunchEdges));
    const Icon BREAKPOINTS(
            ":/debugger/images/debugger_breakpoints.png");
    const Icon WATCHPOINT({
            {":/utils/images/eye_open.png", Theme::TextColorNormal}}, Icon::Tint);
    const Icon TRACEPOINT({
            {":/utils/images/eye_open.png", Theme::TextColorNormal},
            {":/debugger/images/tracepointoverlay.png", Theme::TextColorNormal}}, Icon::Tint | Icon::PunchEdges);
    const Icon CONTINUE(
            ":/debugger/images/debugger_continue.png");
    const Icon CONTINUE_FLAT({
            {":/debugger/images/debugger_continue_1_mask.png", Theme::IconsInterruptToolBarColor},
            {":/debugger/images/debugger_continue_2_mask.png", Theme::IconsRunToolBarColor},
            {":/projectexplorer/images/debugger_beetle_mask.png", Theme::IconsDebugColor}});
    const Icon DEBUG_CONTINUE_SMALL({
            {":/projectexplorer/images/continue_1_small.png", Theme::IconsInterruptColor},
            {":/projectexplorer/images/continue_2_small.png", Theme::IconsRunColor},
            {":/projectexplorer/images/debugger_overlay_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
    const Icon DEBUG_CONTINUE_SMALL_TOOLBAR({
            {":/projectexplorer/images/continue_1_small.png", Theme::IconsInterruptToolBarColor},
            {":/projectexplorer/images/continue_2_small.png", Theme::IconsRunToolBarColor},
            {":/projectexplorer/images/debugger_overlay_small.png", Theme::IconsDebugColor}});
    const Icon INTERRUPT(
            ":/debugger/images/debugger_interrupt.png");
    const Icon INTERRUPT_FLAT({
            {":/debugger/images/debugger_interrupt_mask.png", Theme::IconsInterruptToolBarColor},
            {":/projectexplorer/images/debugger_beetle_mask.png", Theme::IconsDebugColor}});
    const Icon DEBUG_INTERRUPT_SMALL({
            {":/utils/images/interrupt_small.png", Theme::IconsInterruptColor},
            {":/projectexplorer/images/debugger_overlay_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
    const Icon DEBUG_INTERRUPT_SMALL_TOOLBAR({
            {":/utils/images/interrupt_small.png", Theme::IconsInterruptToolBarColor},
            {":/projectexplorer/images/debugger_overlay_small.png", Theme::IconsDebugColor}});
    const Icon DEBUG_EXIT_SMALL({
            {":/utils/images/stop_small.png", Theme::IconsStopColor},
            {":/projectexplorer/images/debugger_overlay_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
    const Icon DEBUG_EXIT_SMALL_TOOLBAR({
            {":/utils/images/stop_small.png", Theme::IconsStopToolBarColor},
            {":/projectexplorer/images/debugger_overlay_small.png", Theme::IconsDebugColor}});
    const Icon LOCATION({
            {":/debugger/images/location_background.png", Theme::IconsCodeModelOverlayForegroundColor},
            {":/debugger/images/location.png", Theme::IconsWarningToolBarColor}}, Icon::Tint);
    const Icon REVERSE_MODE({
            {":/debugger/images/debugger_reversemode_background.png", Theme::IconsCodeModelOverlayForegroundColor},
            {":/debugger/images/debugger_reversemode.png", Theme::IconsInfoColor}}, Icon::Tint);
    const Icon APP_ON_TOP({
            {":/utils/images/app-on-top.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
    const Icon APP_ON_TOP_TOOLBAR({
            {":/utils/images/app-on-top.png", Theme::IconsBaseColor}});
    const Icon SELECT({
            {":/utils/images/select.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
    const Icon SELECT_TOOLBAR({
            {":/utils/images/select.png", Theme::IconsBaseColor}});
    const Icon EMPTY(
            ":/debugger/images/debugger_empty_14.png");
    const Icon RECORD_ON({
            {":/debugger/images/recordfill.png", Theme::IconsStopColor},
            {":/debugger/images/recordoutline.png", Theme::IconsBaseColor}}, Icon::Tint | Icon::DropShadow);
    const Icon RECORD_OFF({
            {":/debugger/images/recordfill.png", Theme::IconsDisabledColor},
            {":/debugger/images/recordoutline.png", Theme::IconsBaseColor}}, Icon::Tint | Icon::DropShadow);

    const Icon STEP_OVER({
            {":/debugger/images/debugger_stepover_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
    const Icon STEP_OVER_TOOLBAR({
            {":/debugger/images/debugger_stepover_small.png", Theme::IconsBaseColor}});
    const Icon STEP_INTO({
            {":/debugger/images/debugger_stepinto_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
    const Icon STEP_INTO_TOOLBAR({
            {":/debugger/images/debugger_stepinto_small.png", Theme::IconsBaseColor}});
    const Icon STEP_OUT({
            {":/debugger/images/debugger_stepout_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
    const Icon STEP_OUT_TOOLBAR({
            {":/debugger/images/debugger_stepout_small.png", Theme::IconsBaseColor}});
    const Icon RESTART({
            {":/debugger/images/debugger_restart_small.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
    const Icon RESTART_TOOLBAR({
            {":/debugger/images/debugger_restart_small.png", Theme::IconsRunToolBarColor}});
    const Icon SINGLE_INSTRUCTION_MODE({
            {":/debugger/images/debugger_singleinstructionmode.png", Theme::IconsBaseColor}});

    const Icon MODE_DEBUGGER_CLASSIC(
            ":/debugger/images/mode_debug.png");
    const Icon MODE_DEBUGGER_FLAT({
            {":/debugger/images/mode_debug_mask.png", Theme::IconsBaseColor}});
    const Icon MODE_DEBUGGER_FLAT_ACTIVE({
            {":/debugger/images/mode_debug_mask.png", Theme::IconsModeDebugActiveColor}});

    const QString prefix = "Debugger";
    const QList<IconInfo> icons = {
        {RECORD_ON.icon(), "RECORD_ON", prefix,
         ""},
        {RECORD_OFF.icon(), "RECORD_OFF", prefix,
         ""},
        {BREAKPOINT.icon(), "BREAKPOINT", prefix,
         ""},
        {BREAKPOINT_DISABLED.icon(), "BREAKPOINT_DISABLED", prefix,
         ""},
        {BREAKPOINT_PENDING.icon(), "BREAKPOINT_PENDING", prefix,
         ""},
        {BREAKPOINTS.icon(), "BREAKPOINTS", prefix,
         ""},
        {WATCHPOINT.icon(), "WATCHPOINT", prefix,
         ""},
        {TRACEPOINT.icon(), "TRACEPOINT", prefix,
         ""},
        {CONTINUE.icon(), "CONTINUE", prefix,
         ""},
        {CONTINUE_FLAT.icon(), "CONTINUE_FLAT", prefix,
         ""},
        {DEBUG_CONTINUE_SMALL.icon(), "DEBUG_CONTINUE_SMALL", prefix,
         ""},
        {DEBUG_CONTINUE_SMALL_TOOLBAR.icon(), "DEBUG_CONTINUE_SMALL_TOOLBAR", prefix,
         ""},
        {INTERRUPT.icon(), "INTERRUPT", prefix,
         ""},
        {INTERRUPT_FLAT.icon(), "INTERRUPT_FLAT", prefix,
         ""},
        {DEBUG_INTERRUPT_SMALL.icon(), "DEBUG_INTERRUPT_SMALL", prefix,
         ""},
        {DEBUG_INTERRUPT_SMALL_TOOLBAR.icon(), "DEBUG_INTERRUPT_SMALL_TOOLBAR", prefix,
         ""},
        {DEBUG_EXIT_SMALL.icon(), "DEBUG_EXIT_SMALL", prefix,
         ""},
        {DEBUG_EXIT_SMALL_TOOLBAR.icon(), "DEBUG_EXIT_SMALL_TOOLBAR", prefix,
         ""},
        {LOCATION.icon(), "LOCATION", prefix,
         ""},
        {REVERSE_MODE.icon(), "REVERSE_MODE", prefix,
         ""},
        {APP_ON_TOP.icon(), "APP_ON_TOP", prefix,
         ""},
        {APP_ON_TOP_TOOLBAR.icon(), "APP_ON_TOP_TOOLBAR", prefix,
         ""},
        {SELECT.icon(), "SELECT", prefix,
         ""},
        {SELECT_TOOLBAR.icon(), "SELECT_TOOLBAR", prefix,
         ""},
        {EMPTY.icon(), "EMPTY", prefix,
         ""},

        {STEP_OVER.icon(), "STEP_OVER", prefix,
         ""},
        {STEP_OVER_TOOLBAR.icon(), "STEP_OVER_TOOLBAR", prefix,
         ""},
        {STEP_INTO.icon(), "STEP_INTO", prefix,
         ""},
        {STEP_INTO_TOOLBAR.icon(), "STEP_INTO_TOOLBAR", prefix,
         ""},
        {STEP_OUT.icon(), "STEP_OUT", prefix,
         ""},
        {STEP_OUT_TOOLBAR.icon(), "STEP_OUT_TOOLBAR", prefix,
         ""},
        {RESTART.icon(), "RESTART", prefix,
         ""},
        {RESTART_TOOLBAR.icon(), "RESTART_TOOLBAR", prefix,
         ""},
        {SINGLE_INSTRUCTION_MODE.icon(), "SINGLE_INSTRUCTION_MODE", prefix,
         ""},

        {MODE_DEBUGGER_CLASSIC.icon(), "MODE_DEBUGGER_CLASSIC", prefix,
         ""},
        {MODE_DEBUGGER_FLAT.icon(), "MODE_DEBUGGER_FLAT", prefix,
         ""},
        {MODE_DEBUGGER_FLAT_ACTIVE.icon(), "MODE_DEBUGGER_FLAT_ACTIVE", prefix,
         ""}
    };
    m_icons.append(icons);
}

void IconLister::addUtilsIcons()
{
    using namespace Utils::Icons;
    const QString prefix = "Utils";
    const QString resPath = ":/utils/images/";
    const QList<IconInfo> icons = {
        {HOME.icon(), "HOME", prefix,
         "'About Qt Creator' dialog."},
        {HOME_TOOLBAR.icon(), "HOME_TOOLBAR", prefix,
         ""},
        {EDIT_CLEAR.icon(), "EDIT_CLEAR", prefix,
         ""},
        {EDIT_CLEAR_TOOLBAR.icon(), "EDIT_CLEAR_TOOLBAR", prefix,
         ""},
        {LOCKED_TOOLBAR.icon(), "LOCKED_TOOLBAR", prefix,
         ""},
        {LOCKED.icon(), "LOCKED", prefix,
         ""},
        {UNLOCKED_TOOLBAR.icon(), "UNLOCKED_TOOLBAR", prefix,
         ""},
        {NEXT.icon(), "NEXT", prefix,
         ""},
        {NEXT_TOOLBAR.icon(), "NEXT_TOOLBAR", prefix,
         ""},
        {PREV.icon(), "PREV", prefix,
         ""},
        {PREV_TOOLBAR.icon(), "PREV_TOOLBAR", prefix,
         ""},
        {PROJECT.icon(), "PROJECT", prefix,
         ""},
        {ZOOM.icon(), "ZOOM", prefix,
         ""},
        {ZOOM_TOOLBAR.icon(), "ZOOM_TOOLBAR", prefix,
         ""},
        {ZOOMIN_TOOLBAR.icon(), "ZOOMIN_TOOLBAR", prefix,
         ""},
        {ZOOMOUT_TOOLBAR.icon(), "ZOOMOUT_TOOLBAR", prefix,
         ""},
        {FITTOVIEW_TOOLBAR.icon(), "FITTOVIEW_TOOLBAR", prefix,
         ""},
        {OK.icon(), "OK", prefix,
         ""},
        {NOTLOADED.icon(), "NOTLOADED", prefix,
         ""},
        {BROKEN.icon(), "BROKEN", prefix,
         ""},
        {BOOKMARK.icon(), "BOOKMARK", prefix,
         ""},
        {BOOKMARK_TOOLBAR.icon(), "BOOKMARK_TOOLBAR", prefix,
         ""},
        {BOOKMARK_TEXTEDITOR.icon(), "BOOKMARK_TEXTEDITOR", prefix,
         ""},
        {SNAPSHOT_TOOLBAR.icon(), "SNAPSHOT_TOOLBAR", prefix,
         ""},
        {NEWSEARCH_TOOLBAR.icon(), "NEWSEARCH_TOOLBAR", prefix,
         ""},

        {NEWFILE.icon(), "NEWFILE", prefix,
         ""},
        {OPENFILE.icon(), "OPENFILE", prefix,
         ""},
        {OPENFILE_TOOLBAR.icon(), "OPENFILE_TOOLBAR", prefix,
         ""},
        {SAVEFILE.icon(), "SAVEFILE", prefix,
         ""},
        {SAVEFILE_TOOLBAR.icon(), "SAVEFILE_TOOLBAR", prefix,
         ""},

        {EXPORTFILE_TOOLBAR.icon(), "EXPORTFILE_TOOLBAR", prefix,
         ""},
        {MULTIEXPORTFILE_TOOLBAR.icon(), "MULTIEXPORTFILE_TOOLBAR", prefix,
         ""},

        {UNDO.icon(), "UNDO", prefix,
         ""},
        {UNDO_TOOLBAR.icon(), "UNDO_TOOLBAR", prefix,
         ""},
        {REDO.icon(), "REDO", prefix,
         ""},
        {REDO_TOOLBAR.icon(), "REDO_TOOLBAR", prefix,
         ""},
        {COPY.icon(), "COPY", prefix,
         ""},
        {COPY_TOOLBAR.icon(), "COPY_TOOLBAR", prefix,
         ""},
        {PASTE.icon(), "PASTE", prefix,
         ""},
        {PASTE_TOOLBAR.icon(), "PASTE_TOOLBAR", prefix,
         ""},
        {CUT.icon(), "CUT", prefix,
         ""},
        {CUT_TOOLBAR.icon(), "CUT_TOOLBAR", prefix,
         ""},
        {RESET.icon(), "RESET", prefix,
         ""},
        {RESET_TOOLBAR.icon(), "RESET_TOOLBAR", prefix,
         ""},

        {ARROW_UP.icon(), "ARROW_UP", prefix,
         ""},
        {ARROW_DOWN.icon(), "ARROW_DOWN", prefix,
         ""},
        {MINUS.icon(), "MINUS", prefix,
         ""},
        {PLUS_TOOLBAR.icon(), "PLUS_TOOLBAR", prefix,
         ""},
        {PLUS.icon(), "PLUS", prefix,
         ""},
        {MAGNIFIER.icon(), "MAGNIFIER", prefix,
         ""},
        {CLEAN.icon(), "CLEAN", prefix,
         ""},
        {CLEAN_TOOLBAR.icon(), "CLEAN_TOOLBAR", prefix,
         ""},
        {RELOAD.icon(), "RELOAD", prefix,
         ""},
        {TOGGLE_LEFT_SIDEBAR.icon(), "TOGGLE_LEFT_SIDEBAR", prefix,
         ""},
        {TOGGLE_LEFT_SIDEBAR_TOOLBAR.icon(), "TOGGLE_LEFT_SIDEBAR_TOOLBAR", prefix,
         ""},
        {TOGGLE_RIGHT_SIDEBAR.icon(), "TOGGLE_RIGHT_SIDEBAR", prefix,
         ""},
        {TOGGLE_RIGHT_SIDEBAR_TOOLBAR.icon(), "TOGGLE_RIGHT_SIDEBAR_TOOLBAR", prefix,
         ""},
        {CLOSE_TOOLBAR.icon(), "CLOSE_TOOLBAR", prefix,
         ""},
        {CLOSE_FOREGROUND.icon(), "CLOSE_FOREGROUND", prefix,
         ""},
        {CLOSE_BACKGROUND.icon(), "CLOSE_BACKGROUND", prefix,
         ""},
        {SPLIT_HORIZONTAL.icon(), "SPLIT_HORIZONTAL", prefix,
         ""},
        {SPLIT_HORIZONTAL_TOOLBAR.icon(), "SPLIT_HORIZONTAL_TOOLBAR", prefix,
         ""},
        {SPLIT_VERTICAL.icon(), "SPLIT_VERTICAL", prefix,
         ""},
        {SPLIT_VERTICAL_TOOLBAR.icon(), "SPLIT_VERTICAL_TOOLBAR", prefix,
         ""},
        {CLOSE_SPLIT_TOP.icon(), "CLOSE_SPLIT_TOP", prefix,
         ""},
        {CLOSE_SPLIT_BOTTOM.icon(), "CLOSE_SPLIT_BOTTOM", prefix,
         ""},
        {CLOSE_SPLIT_LEFT.icon(), "CLOSE_SPLIT_LEFT", prefix,
         ""},
        {CLOSE_SPLIT_RIGHT.icon(), "CLOSE_SPLIT_RIGHT", prefix,
         ""},
        {FILTER.icon(), "FILTER", prefix,
         ""},
        {LINK.icon(), "LINK", prefix,
         ""},
        {LINK_TOOLBAR.icon(), "LINK_TOOLBAR", prefix,
         ""},

        {INFO.icon(), "INFO", prefix,
         ""},
        {INFO_TOOLBAR.icon(), "INFO_TOOLBAR", prefix,
         ""},
        {WARNING.icon(), "WARNING", prefix,
         ""},
        {WARNING_TOOLBAR.icon(), "WARNING_TOOLBAR", prefix,
         ""},
        {CRITICAL.icon(), "CRITICAL", prefix,
         ""},
        {CRITICAL_TOOLBAR.icon(), "CRITICAL_TOOLBAR", prefix,
         ""},

        {ERROR_TASKBAR.icon(), "ERROR_TASKBAR", prefix,
         ""},
        {EXPAND_ALL_TOOLBAR.icon(), "EXPAND_ALL_TOOLBAR", prefix,
         ""},
        {TOOLBAR_EXTENSION.icon(), "TOOLBAR_EXTENSION", prefix,
         ""},
        {RUN_SMALL.icon(), "RUN_SMALL", prefix,
         ""},
        {RUN_SMALL_TOOLBAR.icon(), "RUN_SMALL_TOOLBAR", prefix,
         ""},
        {STOP_SMALL.icon(), "STOP_SMALL", prefix,
         ""},
        {STOP_SMALL_TOOLBAR.icon(), "STOP_SMALL_TOOLBAR", prefix,
         ""},
        {INTERRUPT_SMALL.icon(), "INTERRUPT_SMALL", prefix,
         ""},
        {INTERRUPT_SMALL_TOOLBAR.icon(), "INTERRUPT_SMALL_TOOLBAR", prefix,
         ""},
        {BOUNDING_RECT.icon(), "BOUNDING_RECT", prefix,
         ""},
        {EYE_OPEN_TOOLBAR.icon(), "EYE_OPEN_TOOLBAR", prefix,
         ""},
        {EYE_CLOSED_TOOLBAR.icon(), "EYE_CLOSED_TOOLBAR", prefix,
         ""},
        {REPLACE.icon(), "REPLACE", prefix,
         ""},
        {EXPAND.icon(), "EXPAND", prefix,
         ""},
        {EXPAND_TOOLBAR.icon(), "EXPAND_TOOLBAR", prefix,
         ""},
        {COLLAPSE.icon(), "COLLAPSE", prefix,
         ""},
        {COLLAPSE_TOOLBAR.icon(), "COLLAPSE_TOOLBAR", prefix,
         ""},
        {PAN_TOOLBAR.icon(), "PAN_TOOLBAR", prefix,
         ""},
        {EMPTY14.icon(), "EMPTY14", prefix,
         ""},
        {EMPTY16.icon(), "EMPTY16", prefix,
         ""},
        {OVERLAY_ADD.icon(), "OVERLAY_ADD", prefix,
         ""},
        {OVERLAY_WARNING.icon(), "OVERLAY_WARNING", prefix,
         ""},
        {OVERLAY_ERROR.icon(), "OVERLAY_ERROR", prefix,
         ""},

        {CODEMODEL_ERROR.icon(), "CODEMODEL_ERROR", prefix,
         ""},
        {CODEMODEL_WARNING.icon(), "CODEMODEL_WARNING", prefix,
         ""},
        {CODEMODEL_DISABLED_ERROR.icon(), "CODEMODEL_DISABLED_ERROR", prefix,
         ""},
        {CODEMODEL_DISABLED_WARNING.icon(), "CODEMODEL_DISABLED_WARNING", prefix,
         ""},
        {CODEMODEL_FIXIT.icon(), "CODEMODEL_FIXIT", prefix,
         ""},

        {Icon({{resPath + "crumblepath-segment-first.png", Theme::IconsBaseColor}}).icon(), "crumblepath-segment-first", prefix,
         ""},
        {Icon({{resPath + "crumblepath-segment-middle.png", Theme::IconsBaseColor}}).icon(), "crumblepath-segment-middle", prefix,
         ""},
        {Icon({{resPath + "crumblepath-segment-last.png", Theme::IconsBaseColor}}).icon(), "crumblepath-segment-last", prefix,
         ""},
        {Icon({{resPath + "crumblepath-segment-single.png", Theme::IconsBaseColor}}).icon(), "crumblepath-segment-single", prefix,
         ""},
        {Icon({{resPath + "crumblepath-segment-first-hover.png", Theme::IconsBaseColor}}).icon(), "crumblepath-segment-first-hover", prefix,
         ""},
        {Icon({{resPath + "crumblepath-segment-middle-hover.png", Theme::IconsBaseColor}}).icon(), "crumblepath-segment-middle-hover", prefix,
         ""},
        {Icon({{resPath + "crumblepath-segment-last-hover.png", Theme::IconsBaseColor}}).icon(), "crumblepath-segment-last-hover", prefix,
         ""},
        {Icon({{resPath + "crumblepath-segment-single-hover.png", Theme::IconsBaseColor}}).icon(), "crumblepath-segment-single-hover", prefix,
         ""},

        {Icon({{resPath + "progressindicator_small.png", Theme::PanelTextColorMid}}, Icon::Tint).icon(), "ProgressIndicatorSize_Small", prefix,
         ""},
        {Icon({{resPath + "progressindicator_medium.png", Theme::PanelTextColorMid}}, Icon::Tint).icon(), "ProgressIndicatorSize_Medium", prefix,
         ""},
        {Icon({{resPath + "progressindicator_big.png", Theme::PanelTextColorMid}}, Icon::Tint).icon(), "ProgressIndicatorSize_Large", prefix,
         ""}, // TODO: Align "big" vs. "large"

        {QIcon(":/utils/tooltip/images/f1.png"), "f1", prefix,
         ""},

        {QIcon(resPath + "inputfield.png"), "inputfield", prefix,
         ""},
        {QIcon(resPath + "inputfield_disabled.png"), "inputfield_disabled", prefix,
         ""},

        {QIcon(resPath + "panel_button.png"), "panel_button", prefix,
         ""},
        {QIcon(resPath + "panel_button_checked.png"), "panel_button_checked", prefix,
         ""},
        {QIcon(resPath + "panel_button_checked_hover.png"), "panel_button_checked_hover", prefix,
         ""},
        {QIcon(resPath + "panel_button_hover.png"), "panel_button_hover", prefix,
         ""},
        {QIcon(resPath + "panel_button_pressed.png"), "panel_button_pressed", prefix,
         ""},
        {QIcon(resPath + "panel_manage_button.png"), "panel_manage_button", prefix,
         ""},

        {QIcon(resPath + "progressbar.png"), "progressbar", prefix,
         ""},

        {QIcon(resPath + "Desktop.png"), "Desktop", prefix,
         ""},

        {QIcon(resPath + "arrow.png"), "arrow", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::addCPlusPlusIcons()
{
    using namespace Utils::CodeModelIcon;
    const QString prefix = "CPlusPlus";
    const QList<IconInfo> icons = {
        {iconForType(Class), "Class", prefix,
         ""},
        {iconForType(Struct), "Struct", prefix,
         ""},
        {iconForType(Enum), "Enum", prefix,
         ""},
        {iconForType(Enumerator), "Enumerator", prefix,
         ""},
        {iconForType(FuncPublic), "FuncPublic", prefix,
         ""},
        {iconForType(FuncProtected), "FuncProtected", prefix,
         ""},
        {iconForType(FuncPrivate), "FuncPrivate", prefix,
         ""},
        {iconForType(FuncPublicStatic), "FuncPublicStatic", prefix,
         ""},
        {iconForType(FuncProtectedStatic), "FuncProtectedStatic", prefix,
         ""},
        {iconForType(FuncPrivateStatic), "FuncPrivateStatic", prefix,
         ""},
        {iconForType(Namespace), "Namespace", prefix,
         ""},
        {iconForType(VarPublic), "VarPublic", prefix,
         ""},
        {iconForType(VarProtected), "VarProtected", prefix,
         ""},
        {iconForType(VarPrivate), "VarPrivate", prefix,
         ""},
        {iconForType(VarPublicStatic), "VarPublicStatic", prefix,
         ""},
        {iconForType(VarProtectedStatic), "VarProtectedStatic", prefix,
         ""},
        {iconForType(VarPrivateStatic), "VarPrivateStatic", prefix,
         ""},
        {iconForType(Signal), "Signal", prefix,
         ""},
        {iconForType(SlotPublic), "SlotPublic", prefix,
         ""},
        {iconForType(SlotProtected), "SlotProtected", prefix,
         ""},
        {iconForType(SlotPrivate), "SlotPrivate", prefix,
         ""},
        {iconForType(Keyword), "Keyword", prefix,
         ""},
        {iconForType(Macro), "Macro", prefix,
         ""},
        {iconForType(Property), "Property", prefix,
         ""}
    };
    m_icons.append(icons);
}

void IconLister::addQmlDesignerIcons()
{
    Q_INIT_RESOURCE(componentcore);
    Q_INIT_RESOURCE(formeditor);
    Q_INIT_RESOURCE(navigator);
    Q_INIT_RESOURCE(resources);

    using namespace QmlDesigner::Icons;
    const QString prefix = "QmlDesigner";
    const QString designerImgPath =
            QLatin1String(IDE_SOURCE_TREE) + "/src/plugins/qmldesigner/components/resources/images/";
    const QString ewImgPath =
            QLatin1String(IDE_SOURCE_TREE) + "/src/libs/qmleditorwidgets/images/";
    const QString peImgPath =
            QLatin1String(IDE_SOURCE_TREE) + "/share/qtcreator/qmldesigner/propertyEditorQmlSources/imports/HelperWidgets/images/";
    const QList<IconInfo> icons = {
        {ARROW_UP.icon(), "ARROW_UP", prefix,
         ""},
        {ARROW_RIGHT.icon(), "ARROW_RIGHT", prefix,
         ""},
        {ARROW_DOWN.icon(), "ARROW_DOWN", prefix,
         ""},
        {ARROW_LEFT.icon(), "ARROW_LEFT", prefix,
         ""},

        {EXPORT_UNCHECKED.icon(), "EXPORT_UNCHECKED", prefix,
         ""},
        {EXPORT_CHECKED.icon(), "EXPORT_CHECKED", prefix,
         ""},
        {SNAPPING.icon(), "SNAPPING", prefix,
         ""},
        {NO_SNAPPING.icon(), "NO_SNAPPING", prefix,
         ""},
        {NO_SNAPPING_AND_ANCHORING.icon(), "NO_SNAPPING_AND_ANCHORING", prefix,
         ""},

        {Icon({{designerImgPath + "spliteditorhorizontally.png", Theme::IconsBaseColor}}).icon(), "spliteditorhorizontally", prefix,
         ""},
        {Icon({{designerImgPath + "spliteditorvertically.png", Theme::IconsBaseColor}}).icon(), "spliteditorvertically", prefix,
         ""},

        {Icon({{":/qmldesigner/icon/designeractions/images/column.png", Theme::IconsBaseColor}}).icon(), "column", prefix,
         ""},
        {Icon({{":/qmldesigner/icon/designeractions/images/row.png", Theme::IconsBaseColor}}).icon(), "row", prefix,
         ""},
        {Icon({{":/qmldesigner/icon/designeractions/images/grid.png", Theme::IconsBaseColor}}).icon(), "grid", prefix,
         ""},
        {Icon({{":/qmldesigner/icon/designeractions/images/lower.png", Theme::IconsBaseColor}}).icon(), "lower", prefix,
         ""},
        {Icon({{":/qmldesigner/icon/designeractions/images/raise.png", Theme::IconsBaseColor}}).icon(), "raise", prefix,
         ""},

        {Icon({{ewImgPath + "alignment_top.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "alignment-top", prefix,
         ""},
        {Icon({{ewImgPath + "alignment_top.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).icon(), "alignment-top-h", prefix,
         ""},
        {Icon({{ewImgPath + "alignment_middle.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "alignment-middle", prefix,
         ""},
        {Icon({{ewImgPath + "alignment_middle.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).icon(), "alignment-middle-h", prefix,
         ""},
        {Icon({{ewImgPath + "alignment_bottom.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "alignment-bottom", prefix,
         ""},
        {Icon({{ewImgPath + "alignment_bottom.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).icon(), "alignment-bottom-h", prefix,
         ""},

        {Icon({{ewImgPath + "alignment_left.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "alignment-left", prefix,
         ""},
        {Icon({{ewImgPath + "alignment_left.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).icon(), "alignment-left-h", prefix,
         ""},
        {Icon({{ewImgPath + "alignment_center.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "alignment-center", prefix,
         ""},
        {Icon({{ewImgPath + "alignment_center.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).icon(), "alignment-center-h", prefix,
         ""},
        {Icon({{ewImgPath + "alignment_right.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "alignment-right", prefix,
         ""},
        {Icon({{ewImgPath + "alignment_right.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).icon(), "alignment-right-h", prefix,
         ""},

        {Icon({
             { ewImgPath + "anchor_top.png", Theme::IconsBaseColor},
             { ewImgPath + "anchoreditem.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "anchor-top", prefix,
         ""},
        {Icon({
             { ewImgPath + "anchor_right.png", Theme::IconsBaseColor},
             { ewImgPath + "anchoreditem.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "anchor-right", prefix,
         ""},
        {Icon({
             { ewImgPath + "anchor_bottom.png", Theme::IconsBaseColor},
             { ewImgPath + "anchoreditem.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "anchor-bottom", prefix,
         ""},
        {Icon({
             { ewImgPath + "anchor_left.png", Theme::IconsBaseColor},
             { ewImgPath + "anchoreditem.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "anchor-left", prefix,
         ""},
        {Icon({
             { ewImgPath + "anchor_horizontal.png", Theme::IconsBaseColor},
             { ewImgPath + "anchoreditem.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "anchor-horizontal", prefix,
         ""},
        {Icon({
             { ewImgPath + "anchor_vertical.png", Theme::IconsBaseColor},
             { ewImgPath + "anchoreditem.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "anchor-vertical", prefix,
         ""},
        {Icon({{ewImgPath + "anchor_fill.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "anchor-fill", prefix,
         ""},

        {Icon({{ewImgPath + "anchor_fill.png", Utils::Theme::IconsBaseColor},
             {  ":/utils/images/iconoverlay_reset.png", Utils::Theme::IconsStopToolBarColor}}).icon(), "AnchorsReset", prefix,
         ""},
        {Icon({{":/utils/images/pan.png", Utils::Theme::IconsBaseColor},
             {  ":/utils/images/iconoverlay_reset.png", Utils::Theme::IconsStopToolBarColor}}).icon(), "ResetPosition", prefix,
         ""},
        {Icon({{":/utils/images/fittoview.png", Utils::Theme::IconsBaseColor},
             {  ":/utils/images/iconoverlay_reset.png", Utils::Theme::IconsStopToolBarColor}}).icon(), "ResetSize", prefix,
         ""},

        {Icon({{ewImgPath + "style_bold.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "style-bold", prefix,
         ""},
        {Icon({{ewImgPath + "style_bold.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).icon(), "style-bold-h", prefix,
         ""},
        {Icon({{ewImgPath + "style_italic.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "style-italic", prefix,
         ""},
        {Icon({{ewImgPath + "style_italic.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).icon(), "style-italic-h", prefix,
         ""},
        {Icon({{ewImgPath + "style_underline.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "style-underline", prefix,
         ""},
        {Icon({{ewImgPath + "style_underline.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).icon(), "style-underline-h", prefix,
         ""},
        {Icon({{ewImgPath + "style_strikeout.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "style-strikeout", prefix,
         ""},
        {Icon({{ewImgPath + "style_strikeout.png", Theme::QmlDesigner_HighlightColor}}, Icon::Tint).icon(), "style-strikeout-h", prefix,
         ""},

        {Icon({{ewImgPath + "checkbox_indicator.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "checkbox-indicator", prefix,
         ""},
        {Icon({{ewImgPath + "tr.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "tr", prefix,
         ""},
        {Icon({{peImgPath + "expression.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "expression", prefix,
         ""},
        {Icon({{peImgPath + "placeholder.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "placeholder", prefix,
         ""},
        {Icon({{peImgPath + "submenu.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "submenu", prefix,
         ""},
        {Icon({{peImgPath + "up-arrow.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "up-arrow", prefix,
         ""},
        {Icon({{peImgPath + "down-arrow.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "down-arrow", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::addProfilerTimelineIcons()
{
    Q_INIT_RESOURCE(tracing);
    using namespace Utils;
    const QString prefix = "Profiler";
    const QList<IconInfo> icons = {
        {Icon({{":/tracing/ico_rangeselection.png", Theme::IconsBaseColor}}).icon(), "rangeselection", prefix,
         ""},
        {Icon({{":/tracing/ico_rangeselected.png", Theme::IconsBaseColor}}).icon(), "rangeselected", prefix,
         ""},
        {Icon({{":/tracing/ico_selectionmode.png", Theme::IconsBaseColor}}).icon(), "selectionmode", prefix,
         ""},
        {Icon({{":/tracing/ico_edit.png", Theme::IconsBaseColor}}).icon(), "edit", prefix,
         ""},
        {Icon({{":/tracing/range_handle.png", Theme::IconsBaseColor}}).icon(), "range_handle", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::addWizardIcons()
{
    const QString wizardsPath = QLatin1String(IDE_SOURCE_TREE) + "/share/qtcreator/templates/wizards/";
    const QString prefix = "Wizards";
    const QList<IconInfo> icons = {
        {QIcon(wizardsPath + "autotest/autotest.png"), "autotest_project", prefix,
         ""},
        {QIcon(wizardsPath + "autotest/autotest.png"), "autotest_project", prefix,
         ""},
        {QIcon(wizardsPath + "global/consoleapplication.png"), "consoleapplication_project", prefix,
         ""},
        {QIcon(wizardsPath + "global/guiapplication.png"), "guiapplication_project", prefix,
         ""},
        {QIcon(wizardsPath + "qtcreatorplugin/qtcreatorplugin.png"), "qtcreatorplugin_project", prefix,
         ""},
        {QIcon(wizardsPath + "qtquick2-extension/lib.png"), "qtquick2-extension_project", prefix,
         ""},
        {QIcon(wizardsPath + "projects/nim/icon.png"), "nim_project", prefix,
         ""},
        {QIcon(wizardsPath + "projects/qtquickapplication/canvas3d/icon.png"), "qtquick_canvas3d_project", prefix,
         ""},
        {QIcon(wizardsPath + "projects/qtquickapplication/empty/icon.png"), "qtquick_empty_project", prefix,
         ""},
        {QIcon(wizardsPath + "projects/qtquickapplication/scroll/icon.png"), "qtquick_scroll_project", prefix,
         ""},
        {QIcon(wizardsPath + "projects/qtquickapplication/stack/icon.png"), "qtquick_stack_project", prefix,
         ""},
        {QIcon(wizardsPath + "projects/qtquickapplication/swipe/icon.png"), "qtquick_swipe_project", prefix,
         ""},
        {QIcon(wizardsPath + "projects/qtquickuiprototype/qtquickuiprototype.png"), "qtquickuiprototype_project", prefix,
         ""},
        {QIcon(wizardsPath + "projects/vcs/bazaar/icon.png"), "bazaar_vcs", prefix,
         ""},
        {QIcon(wizardsPath + "projects/vcs/cvs/icon.png"), "cvs_vcs", prefix,
         ""},
        {QIcon(wizardsPath + "projects/vcs/git/icon.png"), "git_vcs", prefix,
         ""},
        {QIcon(wizardsPath + "projects/vcs/mercurial/icon.png"), "mercurial_vcs", prefix,
         ""},
        {QIcon(wizardsPath + "projects/vcs/subversion/icon.png"), "subversion_vcs", prefix,
         ""},
        {QIcon(wizardsPath + "qtquickstyleicons/default.png"), "default_qtquickstyle", prefix,
         ""},
        {QIcon(wizardsPath + "qtquickstyleicons/material-dark.png"), "material-dark_qtquickstyle", prefix,
         ""},
        {QIcon(wizardsPath + "qtquickstyleicons/material-light.png"), "material-light_qtquickstyle", prefix,
         ""},
        {QIcon(wizardsPath + "qtquickstyleicons/universal-dark.png"), "universal-dark_qtquickstyle", prefix,
         ""},
        {QIcon(wizardsPath + "qtquickstyleicons/universal-light.png"), "universal-light_qtquickstyle", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::addWelcomeIcons()
{
    const QString wizardsPath = ":/welcome/images/";
    const QString prefix = "Welcome";
    const QList<IconInfo> icons = {
        {Icon({{wizardsPath + "qtaccount.png", Theme::Welcome_ForegroundPrimaryColor}}, Icon::Tint).icon(), "qtaccount", prefix,
         ""},
        {Icon({{wizardsPath + "community.png", Theme::Welcome_ForegroundPrimaryColor}}, Icon::Tint).icon(), "community", prefix,
         ""},
        {Icon({{wizardsPath + "blogs.png", Theme::Welcome_ForegroundPrimaryColor}}, Icon::Tint).icon(), "blogs", prefix,
         ""},
        {Icon({{wizardsPath + "userguide.png", Theme::Welcome_ForegroundPrimaryColor}}, Icon::Tint).icon(), "userguide", prefix,
         ""},
        {Icon({{wizardsPath + "session.png", Theme::Welcome_ForegroundSecondaryColor}}, Icon::Tint).icon(), "session", prefix,
         ""},
        {Icon({{wizardsPath + "project.png", Theme::Welcome_ForegroundSecondaryColor}}, Icon::Tint).icon(), "project", prefix,
         ""},
        {Icon({{wizardsPath + "open.png", Theme::Welcome_ForegroundSecondaryColor}}, Icon::Tint).icon(), "open", prefix,
         ""},
        {Icon({{wizardsPath + "new.png", Theme::Welcome_ForegroundSecondaryColor}}, Icon::Tint).icon(), "new", prefix,
         ""},
        {Icon({{wizardsPath + "expandarrow.png", Theme::Welcome_ForegroundSecondaryColor}}, Icon::Tint).icon(), "expandarrow", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::addAndroidIcons()
{
    Q_INIT_RESOURCE(android);
    const QString qrcPath = ":/android/images/";
    const QString prefix = "Android";
    const QList<IconInfo> icons = {
        {Icon({{qrcPath + "androiddevice.png", Theme::IconsBaseColor}}).icon(), "androiddevice", prefix,
         ""},
        {Icon({{qrcPath + "androiddevicesmall.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "androiddevicesmall", prefix,
         ""},
        {Icon(qrcPath + "download.png").icon(), "download", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::addIosIcons()
{
    Q_INIT_RESOURCE(ios);
    const QString qrcPath = ":/ios/images/";
    const QString prefix = "iOS";
    const QList<IconInfo> icons = {
        {Icon({{qrcPath + "iosdevice.png", Theme::IconsBaseColor}}).icon(), "iosdevice", prefix,
         ""},
        {Icon({{qrcPath + "iosdevicesmall.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "iosdevicesmall", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::addBareMetalIcons()
{
    Q_INIT_RESOURCE(baremetal);
    const QString qrcPath = ":/baremetal/images/";
    const QString prefix = "BareMetal";
    const QList<IconInfo> icons = {
        {Icon({{qrcPath + "baremetaldevice.png", Theme::IconsBaseColor}}).icon(), "baremetaldevice", prefix,
         ""},
        {Icon({{qrcPath + "baremetaldevicesmall.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "baremetaldevicesmall", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::addQnxIcons()
{
    Q_INIT_RESOURCE(qnx);
    const QString qrcPath = ":/qnx/images/";
    const QString prefix = "QNX";
    const QList<IconInfo> icons = {
        {Icon({{qrcPath + "qnxdevice.png", Theme::IconsBaseColor}}).icon(), "qnxdevice", prefix,
         ""},
        {Icon({{qrcPath + "qnxdevicesmall.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "qnxdevicesmall", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::addWinRTIcons()
{
    Q_INIT_RESOURCE(winrt);
    const QString qrcPath = ":/winrt/images/";
    const QString prefix = "WinRT";
    const QList<IconInfo> icons = {
        {Icon({{qrcPath + "winrtdevice.png", Theme::IconsBaseColor}}).icon(), "winrtdevice", prefix,
         ""},
        {Icon({{qrcPath + "winrtdevicesmall.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "winrtdevicesmall", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::addBoot2QtIcons()
{
    const QString imagePath = QLatin1String(IDE_SOURCE_TREE) + "/../boot2qt/common/images/";
    if (!QDir(imagePath).exists())
        return;
    const QString prefix = "Boot2Qt";
    const QList<IconInfo> icons = {
        {Icon({{imagePath + "boot2qtdevice.png", Theme::IconsBaseColor}}).icon(), "boot2qtdevice", prefix,
         ""},
        {Icon({{imagePath + "boot2qtdevicesmall.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "boot2qtdevicesmall", prefix,
         ""},
        {Icon({{imagePath + "boot2qtemulator.png", Theme::IconsBaseColor}}).icon(), "boot2qtemulator", prefix,
         ""},
        {Icon({{imagePath + "boot2qtemulatorsmall.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "boot2qtemulatorsmall", prefix,
         ""},
        {Icon({{imagePath + "boot2qtstartvm.png", Theme::IconsBaseColor}}).icon(), "boot2qtstartvm", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::addVxWorksIcons()
{
    const QString imagePath = QLatin1String(IDE_SOURCE_TREE) + "/../vxworks/plugins/vxworks/images/";
    if (!QDir(imagePath).exists())
        return;
    const QString prefix = "VxWorks";
    const QList<IconInfo> icons = {
        {Icon({{imagePath + "vxworksqtdevice.png", Theme::IconsBaseColor}}).icon(), "vxworksqtdevice", prefix,
         ""},
        {Icon({{imagePath + "vxworksdevicesmall.png", Theme::IconsBaseColor}}, Icon::Tint).icon(), "vxworksdevicesmall", prefix,
         ""},
    };
    m_icons.append(icons);
}

void IconLister::saveIcons(ThemeKind theme, int dpr) const
{
    QDir().mkpath(QLatin1String(ICONS_PATH));
    for (const IconInfo &icon : m_icons) {
        const QPixmap pixmap = icon.icon.pixmap(QSize(9999, 9999));
        pixmap.save(iconFileName(icon.id, icon.idContext, theme, dpr));
    }
}
