// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "utilsicons.h"

namespace Utils {
namespace Icons {

const Icon HOME({
        {":/utils/images/home.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon HOME_TOOLBAR({
        {":/utils/images/home.png", Theme::IconsBaseColor}});
const Icon EDIT_CLEAR({
        {":/utils/images/editclear.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon EDIT_CLEAR_TOOLBAR({
        {":/utils/images/editclear.png", Theme::IconsBaseColor}});
const Icon LOCKED_TOOLBAR({
        {":/utils/images/locked.png", Theme::IconsBaseColor}});
const Icon LOCKED({
        {":/utils/images/locked.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon UNLOCKED_TOOLBAR({
        {":/utils/images/unlocked.png", Theme::IconsBaseColor}});
const Icon UNLOCKED({
        {":/utils/images/unlocked.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon PINNED({
        {":/utils/images/pinned.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon PINNED_SMALL({
        {":/utils/images/pinned_small.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon NEXT({
        {":/utils/images/next.png", Theme::IconsWarningColor}}, Icon::MenuTintedStyle);
const Icon NEXT_TOOLBAR({
        {":/utils/images/next.png", Theme::IconsNavigationArrowsColor}});
const Icon PREV({
        {":/utils/images/prev.png", Theme::IconsWarningColor}}, Icon::MenuTintedStyle);
const Icon PREV_TOOLBAR({
        {":/utils/images/prev.png", Theme::IconsNavigationArrowsColor}});
const Icon PROJECT({
        {":/utils/images/project.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon ZOOM({
        {":/utils/images/zoom.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon ZOOM_TOOLBAR({
        {":/utils/images/zoom.png", Theme::IconsBaseColor}});
const Icon ZOOMIN_TOOLBAR({
        {":/utils/images/zoom.png", Theme::IconsBaseColor},
        {":/utils/images/zoomin_overlay.png", Theme::IconsBaseColor}});
const Icon ZOOMOUT_TOOLBAR({
        {":/utils/images/zoom.png", Theme::IconsBaseColor},
        {":/utils/images/zoomout_overlay.png", Theme::IconsBaseColor}});
const Icon FITTOVIEW_TOOLBAR({
        {":/utils/images/fittoview.png", Theme::IconsBaseColor}});
const Icon OK({
        {":/utils/images/ok.png", Theme::IconsRunColor}}, Icon::Tint);
const Icon NOTLOADED({
        {":/utils/images/notloaded.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon BROKEN({
        {":/utils/images/broken.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon CRITICAL({
        {":/utils/images/warningfill.png", Theme::BackgroundColorNormal},
        {":/utils/images/error.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon BOOKMARK({
        {":/utils/images/bookmark.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon BOOKMARK_TOOLBAR({
        {":/utils/images/bookmark.png", Theme::IconsBaseColor}});
const Icon BOOKMARK_TEXTEDITOR({
        {":/utils/images/bookmark.png", Theme::Bookmarks_TextMarkColor}}, Icon::Tint);
const Icon SNAPSHOT({
        {":/utils/images/snapshot.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon SNAPSHOT_TOOLBAR({
        {":/utils/images/snapshot.png", Theme::IconsBaseColor}});
const Icon NEWSEARCH_TOOLBAR({
        {":/utils/images/zoom.png", Theme::IconsBaseColor},
        {":/utils/images/iconoverlay_add_small.png", Theme::IconsRunColor}});
const Icon SETTINGS({
        {":/utils/images/settings.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon SETTINGS_TOOLBAR({
        {":/utils/images/settings.png", Theme::IconsBaseColor}});

const Icon NEWFILE({
        {":/utils/images/filenew.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon OPENFILE({
        {":/utils/images/fileopen.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon OPENFILE_TOOLBAR({
        {":/utils/images/fileopen.png", Theme::IconsBaseColor}});
const Icon SAVEFILE({
        {":/utils/images/filesave.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon SAVEFILE_TOOLBAR({
        {":/utils/images/filesave.png", Theme::IconsBaseColor}});

const Icon EXPORTFILE_TOOLBAR({
        {":/utils/images/fileexport.png", Theme::IconsBaseColor}});
const Icon MULTIEXPORTFILE_TOOLBAR({
        {":/utils/images/filemultiexport.png", Theme::IconsBaseColor}});

const Icon DIR({
        {":/utils/images/dir.png", Theme::IconsBaseColor}});
const Icon HELP({
        {":/utils/images/help.png", Theme::IconsBaseColor}});
const Icon UNKNOWN_FILE({
        {":/utils/images/unknownfile.png", Theme::IconsBaseColor}});

const Icon UNDO({
        {":/utils/images/undo.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon UNDO_TOOLBAR({
        {":/utils/images/undo.png", Theme::IconsBaseColor}});
const Icon REDO({
        {":/utils/images/redo.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon REDO_TOOLBAR({
        {":/utils/images/redo.png", Theme::IconsBaseColor}});
const Icon COPY({
        {":/utils/images/editcopy.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon COPY_TOOLBAR({
        {":/utils/images/editcopy.png", Theme::IconsBaseColor}});
const Icon PASTE({
        {":/utils/images/editpaste.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon PASTE_TOOLBAR({
        {":/utils/images/editpaste.png", Theme::IconsBaseColor}});
const Icon CUT({
        {":/utils/images/editcut.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon CUT_TOOLBAR({
        {":/utils/images/editcut.png", Theme::IconsBaseColor}});
const Icon RESET({
        {":/utils/images/reset.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon RESET_TOOLBAR({
        {":/utils/images/reset.png", Theme::IconsBaseColor}});

const Icon ARROW_UP({
        {":/utils/images/arrowup.png", Theme::IconsBaseColor}});
const Icon ARROW_UP_TOOLBAR({
        {":/utils/images/arrowup.png", Theme::IconsNavigationArrowsColor}});
const Icon ARROW_DOWN({
        {":/utils/images/arrowdown.png", Theme::IconsBaseColor}});
const Icon ARROW_DOWN_TOOLBAR({
        {":/utils/images/arrowdown.png", Theme::IconsNavigationArrowsColor}});
const Icon MINUS_TOOLBAR({
        {":/utils/images/minus.png", Theme::IconsBaseColor}});
const Icon MINUS({
        {":/utils/images/minus.png", Theme::PaletteText}}, Icon::Tint);
const Icon PLUS_TOOLBAR({
        {":/utils/images/plus.png", Theme::IconsBaseColor}});
const Icon PLUS({
        {":/utils/images/plus.png", Theme::PaletteText}}, Icon::Tint);
const Icon MAGNIFIER({
        {":/utils/images/magnifier.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon CLEAN({
        {":/utils/images/clean_pane_small.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon CLEAN_TOOLBAR({
        {":/utils/images/clean_pane_small.png", Theme::IconsBaseColor}});
const Icon RELOAD({
        {":/utils/images/reload_gray.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon RELOAD_TOOLBAR({
        {":/utils/images/reload_gray.png", Theme::IconsBaseColor}});
const Icon TOGGLE_LEFT_SIDEBAR({
        {":/utils/images/leftsidebaricon.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon TOGGLE_LEFT_SIDEBAR_TOOLBAR({
        {":/utils/images/leftsidebaricon.png", Theme::IconsBaseColor}});
const Icon TOGGLE_RIGHT_SIDEBAR({
        {":/utils/images/rightsidebaricon.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon TOGGLE_RIGHT_SIDEBAR_TOOLBAR({
        {":/utils/images/rightsidebaricon.png", Theme::IconsBaseColor}});
const Icon CLOSE_TOOLBAR({
        {":/utils/images/close.png", Theme::IconsBaseColor}});
const Icon CLOSE_FOREGROUND({
        {":/utils/images/close.png", Theme::PanelTextColorDark}}, Icon::Tint);
const Icon CLOSE_BACKGROUND({
        {":/utils/images/close.png", Theme::PanelTextColorLight}}, Icon::Tint);
const Icon SPLIT_HORIZONTAL({
        {":/utils/images/splitbutton_horizontal.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon SPLIT_HORIZONTAL_TOOLBAR({
        {":/utils/images/splitbutton_horizontal.png", Theme::IconsBaseColor}});
const Icon SPLIT_VERTICAL({
        {":/utils/images/splitbutton_vertical.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon SPLIT_VERTICAL_TOOLBAR({
        {":/utils/images/splitbutton_vertical.png", Theme::IconsBaseColor}});
const Icon CLOSE_SPLIT_TOP({
        {":/utils/images/splitbutton_closetop.png", Theme::IconsBaseColor}});
const Icon CLOSE_SPLIT_BOTTOM({
        {":/utils/images/splitbutton_closebottom.png", Theme::IconsBaseColor}});
const Icon CLOSE_SPLIT_LEFT({
        {":/utils/images/splitbutton_closeleft.png", Theme::IconsBaseColor}});
const Icon CLOSE_SPLIT_RIGHT({
        {":/utils/images/splitbutton_closeright.png", Theme::IconsBaseColor}});
const Icon FILTER({
        {":/utils/images/filtericon.png", Theme::IconsBaseColor},
        {":/utils/images/toolbuttonexpandarrow.png", Theme::IconsBaseColor}});
const Icon LINK({
        {":/utils/images/linkicon.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon LINK_TOOLBAR({
        {":/utils/images/linkicon.png", Theme::IconsBaseColor}});
const Icon SORT_ALPHABETICALLY_TOOLBAR({
        {":/utils/images/sort_alphabetically.png", Theme::IconsBaseColor}});
const Icon TOGGLE_PROGRESSDETAILS_TOOLBAR({
        {":/utils/images/toggleprogressdetails.png", Theme::IconsBaseColor}});
const Icon ONLINE({
        {":/utils/images/online.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon ONLINE_TOOLBAR({
        {":/utils/images/online.png", Theme::IconsBaseColor}});
const Icon DOWNLOAD({
        {":/utils/images/download.png", Theme::PanelTextColorMid}}, Icon::Tint);

const Icon WARNING({
        {":/utils/images/warningfill.png", Theme::BackgroundColorNormal},
        {":/utils/images/warning.png", Theme::IconsWarningColor}}, Icon::Tint);
const Icon WARNING_TOOLBAR({
        {":/utils/images/warning.png", Theme::IconsWarningToolBarColor}});
const Icon CRITICAL_TOOLBAR({
        {":/utils/images/error.png", Theme::IconsErrorToolBarColor}});
const Icon ERROR_TASKBAR({
        {":/utils/images/compile_error_taskbar.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon INFO({
        {":/utils/images/warningfill.png", Theme::BackgroundColorNormal},
        {":/utils/images/info.png", Theme::IconsInfoColor}}, Icon::Tint);
const Icon INFO_TOOLBAR({
        {":/utils/images/info.png", Theme::IconsInfoToolBarColor}});
const Icon EXPAND_ALL_TOOLBAR({
        {":/find/images/expand.png", Theme::IconsBaseColor}});
const Icon TOOLBAR_EXTENSION({
        {":/utils/images/extension.png", Theme::IconsBaseColor}});
const Icon RUN_SMALL({
        {":/utils/images/run_small.png", Theme::IconsRunColor}}, Icon::MenuTintedStyle);
const Icon RUN_SMALL_TOOLBAR({
        {":/utils/images/run_small.png", Theme::IconsRunToolBarColor}});
const Icon STOP_SMALL({
        {":/utils/images/stop_small.png", Theme::IconsStopColor}}, Icon::MenuTintedStyle);
const Icon STOP_SMALL_TOOLBAR({
        {":/utils/images/stop_small.png", Theme::IconsStopToolBarColor}});
const Icon INTERRUPT_SMALL({
        {":/utils/images/interrupt_small.png", Theme::IconsInterruptColor}}, Icon::MenuTintedStyle);
const Icon INTERRUPT_SMALL_TOOLBAR({
        {":/utils/images/interrupt_small.png", Theme::IconsInterruptToolBarColor}});
const Icon CONTINUE_SMALL({
        {":/utils/images/continue_1_small.png", Theme::IconsInterruptColor},
        {":/utils/images/continue_2_small.png", Theme::IconsRunColor}}, Icon::MenuTintedStyle);
const Icon CONTINUE_SMALL_TOOLBAR({
        {":/utils/images/continue_1_small.png", Theme::IconsInterruptToolBarColor},
        {":/utils/images/continue_2_small.png", Theme::IconsRunToolBarColor}});
const Icon BOUNDING_RECT({
        {":/utils/images/boundingrect.png", Theme::IconsBaseColor}});
const Icon EYE_OPEN({
        {":/utils/images/eye_open.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon EYE_OPEN_TOOLBAR({
        {":/utils/images/eye_open.png", Theme::IconsBaseColor}});
const Icon EYE_CLOSED_TOOLBAR({
        {":/utils/images/eye_closed.png", Theme::IconsBaseColor}});
const Icon REPLACE({
        {":/utils/images/replace_a.png", Theme::PanelTextColorMid},
        {":/utils/images/replace_b.png", Theme::IconsInfoColor}}, Icon::Tint);
const Icon EXPAND({
        {":/utils/images/expand.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon EXPAND_TOOLBAR({
        {":/utils/images/expand.png", Theme::IconsBaseColor}});
const Icon COLLAPSE({
        {":/utils/images/collapse.png", Theme::PanelTextColorMid}}, Icon::Tint);
const Icon CLOCK_BLACK({{":/utils/images/clock.png", Theme::Token_Basic_Black}}, Icon::Tint);
const Icon COLLAPSE_TOOLBAR({
        {":/utils/images/collapse.png", Theme::IconsBaseColor}});
const Icon PAN_TOOLBAR({
        {":/utils/images/pan.png", Theme::IconsBaseColor}});
const Icon EMPTY14(":/utils/images/empty14.png");
const Icon EMPTY16(":/utils/images/empty16.png");
const Icon OVERLAY_ADD({
        {":/utils/images/iconoverlay_add_background.png", Theme::BackgroundColorNormal},
        {":/utils/images/iconoverlay_add.png", Theme::IconsRunColor}}, Icon::Tint);
const Icon OVERLAY_WARNING({
        {":/utils/images/iconoverlay_warning_background.png", Theme::BackgroundColorNormal},
        {":/utils/images/iconoverlay_warning.png", Theme::IconsWarningColor}}, Icon::Tint);
const Icon OVERLAY_ERROR({
        {":/utils/images/iconoverlay_error_background.png", Theme::BackgroundColorNormal},
        {":/utils/images/iconoverlay_error.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon RUN_FILE({
        {":/utils/images/run_small.png", Theme::IconsRunColor},
        {":/utils/images/run_file.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon RUN_FILE_TOOLBAR({
        {":/utils/images/run_small.png", Theme::IconsRunToolBarColor},
        {":/utils/images/run_file.png", Theme::IconsBaseColor}});
const Icon RUN_SELECTED({
        {":/utils/images/run_small.png", Theme::IconsRunColor},
        {":/utils/images/runselected_boxes.png", Theme::PanelTextColorMid},
        {":/utils/images/runselected_tickmarks.png", Theme::PanelTextColorMid}}, Icon::MenuTintedStyle);
const Icon RUN_SELECTED_TOOLBAR({
        {":/utils/images/run_small.png", Theme::IconsRunToolBarColor},
        {":/utils/images/runselected_boxes.png", Theme::IconsBaseColor},
        {":/utils/images/runselected_tickmarks.png", Theme::IconsBaseColor}});

const Icon CODEMODEL_ERROR({
        {":/utils/images/codemodelerror.png", Theme::IconsErrorColor}}, Icon::Tint);
const Icon CODEMODEL_WARNING({
        {":/utils/images/codemodelwarning.png", Theme::IconsWarningColor}}, Icon::Tint);
const Icon CODEMODEL_DISABLED_ERROR({
        {":/utils/images/codemodelerror.png", Theme::IconsDisabledColor}}, Icon::Tint);
const Icon CODEMODEL_DISABLED_WARNING({
        {":/utils/images/codemodelwarning.png", Theme::IconsDisabledColor}}, Icon::Tint);
const Icon CODEMODEL_FIXIT({
        {":/utils/images/lightbulbcap.png", Theme::PanelTextColorMid},
        {":/utils/images/lightbulb.png", Theme::IconsWarningColor}}, Icon::Tint);

const Icon MACOS_TOUCHBAR_BOOKMARK(
        ":/utils/images/macos_touchbar_bookmark.png");
const Icon MACOS_TOUCHBAR_CLEAR(
        ":/utils/images/macos_touchbar_clear.png");

#define MAKE_ENTRY(ICON) {#ICON, ICON}
static QHash<QString, Icon> s_nameToIcon = {
    MAKE_ENTRY(ARROW_DOWN_TOOLBAR),
    MAKE_ENTRY(ARROW_DOWN),
    MAKE_ENTRY(ARROW_UP_TOOLBAR),
    MAKE_ENTRY(ARROW_UP),
    MAKE_ENTRY(BOOKMARK_TEXTEDITOR),
    MAKE_ENTRY(BOOKMARK_TOOLBAR),
    MAKE_ENTRY(BOOKMARK),
    MAKE_ENTRY(BOUNDING_RECT),
    MAKE_ENTRY(BROKEN),
    MAKE_ENTRY(CLEAN_TOOLBAR),
    MAKE_ENTRY(CLEAN),
    MAKE_ENTRY(CLOSE_BACKGROUND),
    MAKE_ENTRY(CLOSE_FOREGROUND),
    MAKE_ENTRY(CLOSE_SPLIT_BOTTOM),
    MAKE_ENTRY(CLOSE_SPLIT_LEFT),
    MAKE_ENTRY(CLOSE_SPLIT_RIGHT),
    MAKE_ENTRY(CLOSE_SPLIT_TOP),
    MAKE_ENTRY(CLOSE_TOOLBAR),
    MAKE_ENTRY(CODEMODEL_DISABLED_ERROR),
    MAKE_ENTRY(CODEMODEL_DISABLED_WARNING),
    MAKE_ENTRY(CODEMODEL_ERROR),
    MAKE_ENTRY(CODEMODEL_FIXIT),
    MAKE_ENTRY(CODEMODEL_WARNING),
    MAKE_ENTRY(CLOCK_BLACK),
    MAKE_ENTRY(COLLAPSE_TOOLBAR),
    MAKE_ENTRY(COLLAPSE),
    MAKE_ENTRY(CONTINUE_SMALL_TOOLBAR),
    MAKE_ENTRY(CONTINUE_SMALL),
    MAKE_ENTRY(COPY_TOOLBAR),
    MAKE_ENTRY(COPY),
    MAKE_ENTRY(CRITICAL_TOOLBAR),
    MAKE_ENTRY(CRITICAL),
    MAKE_ENTRY(CUT_TOOLBAR),
    MAKE_ENTRY(CUT),
    MAKE_ENTRY(DIR),
    MAKE_ENTRY(DOWNLOAD),
    MAKE_ENTRY(EDIT_CLEAR_TOOLBAR),
    MAKE_ENTRY(EDIT_CLEAR),
    MAKE_ENTRY(EMPTY14),
    MAKE_ENTRY(EMPTY16),
    MAKE_ENTRY(ERROR_TASKBAR),
    MAKE_ENTRY(EXPAND_ALL_TOOLBAR),
    MAKE_ENTRY(EXPAND_TOOLBAR),
    MAKE_ENTRY(EXPAND),
    MAKE_ENTRY(EXPORTFILE_TOOLBAR),
    MAKE_ENTRY(EYE_CLOSED_TOOLBAR),
    MAKE_ENTRY(EYE_OPEN_TOOLBAR),
    MAKE_ENTRY(EYE_OPEN),
    MAKE_ENTRY(FILTER),
    MAKE_ENTRY(FITTOVIEW_TOOLBAR),
    MAKE_ENTRY(HELP),
    MAKE_ENTRY(HOME_TOOLBAR),
    MAKE_ENTRY(HOME),
    MAKE_ENTRY(INFO_TOOLBAR),
    MAKE_ENTRY(INFO),
    MAKE_ENTRY(INTERRUPT_SMALL_TOOLBAR),
    MAKE_ENTRY(INTERRUPT_SMALL),
    MAKE_ENTRY(LINK_TOOLBAR),
    MAKE_ENTRY(LINK),
    MAKE_ENTRY(LOCKED_TOOLBAR),
    MAKE_ENTRY(LOCKED),
    MAKE_ENTRY(MACOS_TOUCHBAR_BOOKMARK),
    MAKE_ENTRY(MACOS_TOUCHBAR_CLEAR),
    MAKE_ENTRY(MAGNIFIER),
    MAKE_ENTRY(MINUS_TOOLBAR),
    MAKE_ENTRY(MINUS),
    MAKE_ENTRY(MULTIEXPORTFILE_TOOLBAR),
    MAKE_ENTRY(NEWFILE),
    MAKE_ENTRY(NEWSEARCH_TOOLBAR),
    MAKE_ENTRY(NEXT_TOOLBAR),
    MAKE_ENTRY(NEXT),
    MAKE_ENTRY(NOTLOADED),
    MAKE_ENTRY(OK),
    MAKE_ENTRY(ONLINE_TOOLBAR),
    MAKE_ENTRY(ONLINE),
    MAKE_ENTRY(OPENFILE_TOOLBAR),
    MAKE_ENTRY(OPENFILE),
    MAKE_ENTRY(OVERLAY_ADD),
    MAKE_ENTRY(OVERLAY_ERROR),
    MAKE_ENTRY(OVERLAY_WARNING),
    MAKE_ENTRY(PAN_TOOLBAR),
    MAKE_ENTRY(PASTE_TOOLBAR),
    MAKE_ENTRY(PASTE),
    MAKE_ENTRY(PINNED_SMALL),
    MAKE_ENTRY(PINNED),
    MAKE_ENTRY(PLUS_TOOLBAR),
    MAKE_ENTRY(PLUS),
    MAKE_ENTRY(PREV_TOOLBAR),
    MAKE_ENTRY(PREV),
    MAKE_ENTRY(PROJECT),
    MAKE_ENTRY(REDO_TOOLBAR),
    MAKE_ENTRY(REDO),
    MAKE_ENTRY(RELOAD_TOOLBAR),
    MAKE_ENTRY(RELOAD),
    MAKE_ENTRY(REPLACE),
    MAKE_ENTRY(RESET_TOOLBAR),
    MAKE_ENTRY(RESET),
    MAKE_ENTRY(RUN_FILE_TOOLBAR),
    MAKE_ENTRY(RUN_FILE),
    MAKE_ENTRY(RUN_SELECTED_TOOLBAR),
    MAKE_ENTRY(RUN_SELECTED),
    MAKE_ENTRY(RUN_SMALL_TOOLBAR),
    MAKE_ENTRY(RUN_SMALL),
    MAKE_ENTRY(SAVEFILE_TOOLBAR),
    MAKE_ENTRY(SAVEFILE),
    MAKE_ENTRY(SETTINGS_TOOLBAR),
    MAKE_ENTRY(SETTINGS),
    MAKE_ENTRY(SNAPSHOT_TOOLBAR),
    MAKE_ENTRY(SNAPSHOT),
    MAKE_ENTRY(SORT_ALPHABETICALLY_TOOLBAR),
    MAKE_ENTRY(SPLIT_HORIZONTAL_TOOLBAR),
    MAKE_ENTRY(SPLIT_HORIZONTAL),
    MAKE_ENTRY(SPLIT_VERTICAL_TOOLBAR),
    MAKE_ENTRY(SPLIT_VERTICAL),
    MAKE_ENTRY(STOP_SMALL_TOOLBAR),
    MAKE_ENTRY(STOP_SMALL),
    MAKE_ENTRY(TOGGLE_LEFT_SIDEBAR_TOOLBAR),
    MAKE_ENTRY(TOGGLE_LEFT_SIDEBAR),
    MAKE_ENTRY(TOGGLE_PROGRESSDETAILS_TOOLBAR),
    MAKE_ENTRY(TOGGLE_RIGHT_SIDEBAR_TOOLBAR),
    MAKE_ENTRY(TOGGLE_RIGHT_SIDEBAR),
    MAKE_ENTRY(TOOLBAR_EXTENSION),
    MAKE_ENTRY(UNDO_TOOLBAR),
    MAKE_ENTRY(UNDO),
    MAKE_ENTRY(UNKNOWN_FILE),
    MAKE_ENTRY(UNLOCKED_TOOLBAR),
    MAKE_ENTRY(UNLOCKED),
    MAKE_ENTRY(WARNING_TOOLBAR),
    MAKE_ENTRY(WARNING),
    MAKE_ENTRY(ZOOM_TOOLBAR),
    MAKE_ENTRY(ZOOM),
    MAKE_ENTRY(ZOOMIN_TOOLBAR),
    MAKE_ENTRY(ZOOMOUT_TOOLBAR),
};

std::optional<Icon> fromString(const QString &name)
{
    auto it = s_nameToIcon.find(name);
    if (it != s_nameToIcon.end())
        return it.value();
    return std::nullopt;
}

} // namespace Icons

QIcon CodeModelIcon::iconForType(CodeModelIcon::Type type)
{
    static const IconMaskAndColor classRelationIcon {
        ":/codemodel/images/classrelation.png", Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor classRelationBackgroundIcon {
        ":/codemodel/images/classrelationbackground.png", Theme::IconsCodeModelOverlayBackgroundColor};
    static const IconMaskAndColor classMemberFunctionIcon {
        ":/codemodel/images/classmemberfunction.png", Theme::IconsCodeModelFunctionColor};
    static const IconMaskAndColor classMemberVariableIcon {
        ":/codemodel/images/classmembervariable.png", Theme::IconsCodeModelVariableColor};
    static const IconMaskAndColor functionIcon {
        ":/codemodel/images/member.png", Theme::IconsCodeModelFunctionColor};
    static const IconMaskAndColor variableIcon {
        ":/codemodel/images/member.png", Theme::IconsCodeModelVariableColor};
    static const IconMaskAndColor signalIcon {
        ":/codemodel/images/signal.png", Theme::IconsCodeModelFunctionColor};
    static const IconMaskAndColor slotIcon {
        ":/codemodel/images/slot.png", Theme::IconsCodeModelFunctionColor};
    static const IconMaskAndColor propertyIcon {
        ":/codemodel/images/property.png", Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor propertyBackgroundIcon {
        ":/codemodel/images/propertybackground.png", Theme::IconsCodeModelOverlayBackgroundColor};
    static const IconMaskAndColor protectedIcon {
        ":/codemodel/images/protected.png", Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor protectedBackgroundIcon {
        ":/codemodel/images/protectedbackground.png", Theme::IconsCodeModelOverlayBackgroundColor};
    static const IconMaskAndColor privateIcon {
        ":/codemodel/images/private.png", Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor privateBackgroundIcon {
        ":/codemodel/images/privatebackground.png", Theme::IconsCodeModelOverlayBackgroundColor};
    static const IconMaskAndColor staticIcon {
        ":/codemodel/images/static.png", Theme::IconsCodeModelOverlayForegroundColor};
    static const IconMaskAndColor staticBackgroundIcon {
        ":/codemodel/images/staticbackground.png", Theme::IconsCodeModelOverlayBackgroundColor};

    switch (type) {
    case Class: {
        const static QIcon icon(Icon({
            classRelationBackgroundIcon, classRelationIcon,
            {":/codemodel/images/classparent.png", Theme::IconsCodeModelClassColor},
            classMemberFunctionIcon, classMemberVariableIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case Struct: {
        const static QIcon icon(Icon({
            classRelationBackgroundIcon, classRelationIcon,
            {":/codemodel/images/classparent.png", Theme::IconsCodeModelStructColor},
            classMemberFunctionIcon, classMemberVariableIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case Enum: {
        const static QIcon icon(Icon({
            {":/codemodel/images/enum.png", Theme::IconsCodeModelEnumColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case Enumerator: {
        const static QIcon icon(Icon({
            {":/codemodel/images/enumerator.png", Theme::IconsCodeModelEnumColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncPublic: {
        const static QIcon icon(Icon({
                functionIcon}, Icon::Tint).icon());
        return icon;
    }
    case FuncProtected: {
        const static QIcon icon(Icon({
                functionIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncPrivate: {
        const static QIcon icon(Icon({
            functionIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncPublicStatic: {
        const static QIcon icon(Icon({
            functionIcon, staticBackgroundIcon, staticIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncProtectedStatic: {
        const static QIcon icon(Icon({
            functionIcon, staticBackgroundIcon, staticIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case FuncPrivateStatic: {
        const static QIcon icon(Icon({
            functionIcon, staticBackgroundIcon, staticIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case Namespace: {
        const static QIcon icon(Icon({
            {":/utils/images/namespace.png", Theme::IconsCodeModelKeywordColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case VarPublic: {
        const static QIcon icon(Icon({
            variableIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarProtected: {
        const static QIcon icon(Icon({
            variableIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarPrivate: {
        const static QIcon icon(Icon({
            variableIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarPublicStatic: {
        const static QIcon icon(Icon({
            variableIcon, staticBackgroundIcon, staticIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarProtectedStatic: {
        const static QIcon icon(Icon({
            variableIcon, staticBackgroundIcon, staticIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case VarPrivateStatic: {
        const static QIcon icon(Icon({
            variableIcon, staticBackgroundIcon, staticIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case Signal: {
        const static QIcon icon(Icon({
            signalIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case SlotPublic: {
        const static QIcon icon(Icon({
            slotIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case SlotProtected: {
        const static QIcon icon(Icon({
            slotIcon, protectedBackgroundIcon, protectedIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case SlotPrivate: {
        const static QIcon icon(Icon({
            slotIcon, privateBackgroundIcon, privateIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case Keyword: {
        const static QIcon icon(Icon({
            {":/codemodel/images/keyword.png", Theme::IconsCodeModelKeywordColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case Macro: {
        const static QIcon icon(Icon({
            {":/codemodel/images/macro.png", Theme::IconsCodeModelMacroColor}
        }, Icon::Tint).icon());
        return icon;
    }
    case Property: {
        const static QIcon icon(Icon({
            variableIcon, propertyBackgroundIcon, propertyIcon
        }, Icon::Tint).icon());
        return icon;
    }
    case Unknown: {
        const static QIcon icon(Icons::EMPTY16.icon());
        return icon;
    }
    default:
        break;
    }
    return QIcon();
}

} // namespace Utils
